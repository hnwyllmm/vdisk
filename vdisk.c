#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/genhd.h>

#define VDISK_MAJOR 72 // COMPAQ_SMART2_MAJOR
#define VDISK_NAME	"vdisk"
#define VDISK_BLKDEV_BYTES (16*1024*1024)

static struct gendisk *vdisk_disk;
static struct request_queue *vdisk_queue;
static char vdisk_data[VDISK_BLKDEV_BYTES];

static struct block_device_operations vdisk_block_device_ops = {
	.owner = THIS_MODULE,
};

static void vdisk_blkdev_do_request(struct request_queue *rq)
{
	#if 1
	struct req_iterator ri;
	struct bio_vec *bvec;
	char *buffer, *disk_mem;
	struct request *req = blk_fetch_request(rq);

	while (req)
	{
		const int start = blk_rq_pos(req) << 9;
		const int size  = blk_rq_cur_bytes(req);
		const int end_offset = start + size;
		if (end_offset > VDISK_BLKDEV_BYTES)
		{
			printk("vdisk: bad request: start %d, count %d\n", start, size);
			__blk_end_request_all(req, -EIO);
			goto fetch_next;
		}

		printk("vdisk: request start %d, count %d\n", start, size);
		disk_mem = vdisk_data + start;
		switch(rq_data_dir(req))
		{
		case READ:
			/*
			memcpy(req->buffer, (char *)vdisk_data + start, size);
			*/
			rq_for_each_segment(bvec, req, ri)
			{
				buffer = kmap(bvec->bv_page) + bvec->bv_offset;
				memcpy(buffer, disk_mem, bvec->bv_len);
				kunmap(bvec->bv_page);
				disk_mem += bvec->bv_len;
			}
			__blk_end_request_all(req, 0);
			break;
		case WRITE:
			/*
			memcpy((char *)vdisk_data + start, req->buffer, size);
			*/
			rq_for_each_segment(bvec, req, ri)
			{
				buffer = kmap(bvec->bv_page) + bvec->bv_offset;
				memcpy(disk_mem, buffer, bvec->bv_len);
				kunmap(bvec->bv_page);
				disk_mem += bvec->bv_len;
			}
			__blk_end_request_all(req, 0);
			break;
		default:
			__blk_end_request_all(req, -EINVAL);
			break;
		}

fetch_next:
		req = blk_fetch_request(rq);
	}
	#endif

	printk("vdisk: vdisk_blkdev_do_request exit\n");
}

static int __init vdisk_init(void)
{
	int ret = 0;
	struct elevator_queue *old_e;
	printk("vdisk_init\n");

	// 创建请求队列
	vdisk_queue = blk_init_queue(vdisk_blkdev_do_request, NULL);
	if (!vdisk_queue)
	{
		ret = -ENOMEM;
		goto out;
	}

	// 设置请求队列调度器
	/* 新版本代码中, 如果request_queue->elevator不是NULL,就不会替换 */
	old_e = vdisk_queue->elevator;
	vdisk_queue->elevator = NULL;
	elevator_exit(old_e);
	if (elevator_init(vdisk_queue, "noop"))
	{
		printk("vdisk: switch to noop elevator failure");
	}
	else
	{
		elevator_init(vdisk_queue, NULL);
	}

	// 分配磁盘
	vdisk_disk = alloc_disk(1);
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
