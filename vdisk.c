#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/genhd.h>
#include <linux/hdreg.h>

#define VDISK_MAJOR 72 // COMPAQ_SMART2_MAJOR
#define VDISK_NAME	"vdisk"

#define VDISK_DATA_SEGORDER		2
#define VDISK_DATA_SEGSHIFT 	(PAGE_SHIFT + VDISK_DATA_SEGORDER)
#define VDISK_DATA_SEGSIZE		(PAGE_SIZE << VDISK_DATA_SEGORDER)
#define VDISK_DATA_SEGMASK		(~(VDISK_DATA_SEGSIZE - 1))

#define VDISK_MAX_PARTITIONS (64) 				// 最大分区数

static long long int disk_size = 16 * 1024 * 1026;
static char *param_disk_size = "16M";
module_param_named(size, param_disk_size, charp, S_IRUGO);

static struct gendisk *vdisk_disk;
static struct request_queue *vdisk_queue;
static struct radix_tree_root vdisk_data;

static int vdisk_blkdev_getgeo(struct block_device *bdev, struct hd_geometry *geo);

static struct block_device_operations vdisk_block_device_ops = {
	.owner  = THIS_MODULE,
	.getgeo = vdisk_blkdev_getgeo,
};

static int getparam(void)
{
	char unit;
	char tailc;

	if (sscanf(param_disk_size, "%lld%c%c", &disk_size, &unit, &tailc) != 2)
	{
		return -EINVAL;
	}

	if (disk_size <= 0)
		return -EINVAL;

	switch (unit)
	{
	case 'g':
	case 'G':
		disk_size <<= 30;
		break;
	case 'm':
	case 'M':
		disk_size <<= 20;
		break;
	case 'k':
	case 'K':
		disk_size <<= 10;
		break;
	case 'b':
	case 'B':
		break;
	default:
		return -EINVAL;
	}

	disk_size = (disk_size + 511) & ~511;
	return 0;
}

static int vdisk_blkdev_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	if (disk_size < 16 * 1024 * 1024)
	{
		geo->heads   = 1;
		geo->sectors = 1;
	}
	else if (disk_size < 512 * 1024 * 1024)
	{
		geo->heads   = 1;
		geo->sectors = 32;
	}
	else if (disk_size < 16ULL * 1024 * 1024 * 1024)
	{
		geo->heads   = 32;
		geo->sectors = 32;
	}
	else
	{
		geo->heads   = 255;
		geo->sectors = 63;
	}
	geo->cylinders = (disk_size >> 9) / geo->heads / geo->sectors;
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
static void vdisk_freemem(void)
{
	int i;
	void *p;
	int pages = (disk_size + PAGE_SIZE - 1) >> PAGE_SHIFT;
	for (i = 0; i < pages; i++)
	{
		p = radix_tree_lookup(&vdisk_data, i);
		radix_tree_delete(&vdisk_data, i);
		memset(p, 0, VDISK_DATA_SEGSIZE);
		free_pages((unsigned long)p, VDISK_DATA_SEGORDER);
	}
}

static int vdisk_allocmem(void)
{
	int ret;
	int i;
	int pages;
	void *p;

	INIT_RADIX_TREE(&vdisk_data, GFP_KERNEL);
	pages = (disk_size + VDISK_DATA_SEGSIZE - 1) >> VDISK_DATA_SEGSHIFT;
	for (i = 0; i < pages; i++)
	{
		p = (void *)__get_free_pages(GFP_KERNEL, VDISK_DATA_SEGORDER);
		if (!p)
		{
			ret = -ENOMEM;
			goto err_alloc;
		}

		ret = radix_tree_insert(&vdisk_data, i, p);
		if (IS_ERR_VALUE(ret))
			goto err_radix_tree_insert;
	}
	return 0;

err_radix_tree_insert:
	free_pages((unsigned long)p, VDISK_DATA_SEGORDER);
err_alloc:
	vdisk_freemem();
	return ret;

}
///////////////////////////////////////////////////////////////////////////////
static void vdisk_blkdev_make_request(struct request_queue *rq, struct bio *bio)
{
	int i;
	struct bio_vec *bvec;
	int offset;

	const int start = bio->bi_sector << 9;
	const int size  = bio->bi_size;
	if (start + size > disk_size)
	{
		printk("vdisk: bad request: start %d, count %d\n", start, size);
		bio_endio(bio, -EIO);
		return ;
	}

	offset = start;
	bio_for_each_segment(bvec, bio, i)
	{
		void *iovec_mem, *disk_mem;
		unsigned int count = 0;
		iovec_mem = kmap(bvec->bv_page) + bvec->bv_offset;
		while (count < bvec->bv_len)
		{
			int this = min(bvec->bv_len - count, 
				(unsigned int)(VDISK_DATA_SEGSIZE - ((offset + count) & ~VDISK_DATA_SEGMASK)));
			disk_mem = radix_tree_lookup(&vdisk_data, (offset + count) >> VDISK_DATA_SEGSHIFT);
			if (!disk_mem)
			{
				printk("vdisk: cannot find memory: %d\n", 
					(offset + count) >> VDISK_DATA_SEGSHIFT);
				bio_endio(bio, -EIO);
				return;
			}

			disk_mem += (offset + count) & ~VDISK_DATA_SEGMASK;
			
			if (WRITE == bio_rw(bio))
			{
				memcpy(disk_mem, iovec_mem + count, this);
			}
			else
			{
				memcpy(iovec_mem + count, disk_mem, this);
			}
			
			count += this;
		}
		kunmap(bvec->bv_page);
		offset += bvec->bv_len;
	}
	bio_endio(bio, 0);
	return ;
}

static int __init vdisk_init(void)
{
	int ret = 0;
	printk("vdisk_init\n");

	if ((ret = getparam()) != 0)
		return ret;

	ret = vdisk_allocmem();
	if (IS_ERR_VALUE(ret))
		goto out;

	// 创建请求队列
	vdisk_queue = blk_alloc_queue(GFP_KERNEL);
	if (!vdisk_queue)
	{
		ret = -ENOMEM;
		goto freemem;
	}

	blk_queue_make_request(vdisk_queue, vdisk_blkdev_make_request);

	// 分配磁盘
	vdisk_disk = alloc_disk(VDISK_MAX_PARTITIONS);
	if (!vdisk_disk)
	{
		ret = -ENOMEM;
		goto clean_queue;
	}

	strcpy(vdisk_disk->disk_name, VDISK_NAME);
	vdisk_disk->major = VDISK_MAJOR;
	vdisk_disk->first_minor = 0;
	vdisk_disk->fops = &vdisk_block_device_ops;
	vdisk_disk->queue = vdisk_queue;
	set_capacity(vdisk_disk, disk_size >> 9);

	// 添加磁盘
	add_disk(vdisk_disk);
out:
	return ret;

clean_queue:
	blk_cleanup_queue(vdisk_queue);

freemem:
	vdisk_freemem();
	goto out;
}

static void __exit vdisk_exit(void)
{
	printk("vdisk_exit\n");

	if (vdisk_disk)
	{
		put_disk(vdisk_disk);
		del_gendisk(vdisk_disk);
	}

	if (vdisk_queue)
	{
		blk_cleanup_queue(vdisk_queue);
	}

	vdisk_freemem();
}

module_init(vdisk_init);
module_exit(vdisk_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hnwyllmm");
MODULE_DESCRIPTION("test");
