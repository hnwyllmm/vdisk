#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/genhd.h>

#define VDISK_MAJOR 72 // COMPAQ_SMART2_MAJOR
#define VDISK_NAME	"vdisk"
#define VDISK_BLKDEV_BYTES (16*1024*1024)

#define VDISK_MAX_PARTITIONS (64) 				// 最大分区数

static struct gendisk *vdisk_disk;
static struct request_queue *vdisk_queue;
static char vdisk_data[VDISK_BLKDEV_BYTES];

static struct block_device_operations vdisk_block_device_ops = {
	.owner = THIS_MODULE,
};

static void vdisk_blkdev_make_request(struct request_queue *rq, struct bio *bio)
{
	int i;
	struct bio_vec *bvec;
	char *disk_mem;

	const int start = bio->bi_sector << 9;
	const int size  = bio->bi_size;
	if (start + size > VDISK_BLKDEV_BYTES)
	{
		printk("vdisk: bad request: start %d, count %d\n", start, size);
		bio_endio(bio, -EIO);
		return ;
	}

	disk_mem = vdisk_data + start;
	bio_for_each_segment(bvec, bio, i)
	{
		void *iovec_mem;
		iovec_mem = kmap(bvec->bv_page) + bvec->bv_offset;
		if (WRITE == bio_rw(bio))
		{
			memcpy(disk_mem, iovec_mem, bvec->bv_len);
		}
		else
		{
			memcpy(iovec_mem, disk_mem, bvec->bv_len);
		}
		kunmap(bvec->bv_page);
		disk_mem += bvec->bv_len;
	}
	bio_endio(bio, 0);
	return ;
}

static int __init vdisk_init(void)
{
	int ret = 0;
	printk("vdisk_init\n");

	// 创建请求队列
	vdisk_queue = blk_alloc_queue(GFP_KERNEL);
	if (!vdisk_queue)
	{
		ret = -ENOMEM;
		goto out;
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
	set_capacity(vdisk_disk, VDISK_BLKDEV_BYTES >> 9);

	// 添加磁盘
	add_disk(vdisk_disk);
out:
	return ret;

clean_queue:
	blk_cleanup_queue(vdisk_queue);
	goto out;
}

static void __exit vdisk_exit(void)
{
	printk("vdisk_exit\n");

	if (vdisk_disk)
	{
		// 没有使用过get_disk增加引用计数，也就不再使用put_disk减少引用计数
		// put_disk(vdisk_disk);
		del_gendisk(vdisk_disk);
	}

	if (vdisk_queue)
	{
		blk_cleanup_queue(vdisk_queue);
	}
}

module_init(vdisk_init);
module_exit(vdisk_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("hnwyllmm");
MODULE_DESCRIPTION("test");
