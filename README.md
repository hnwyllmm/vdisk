# vdisk
写一个块设备驱动(赵磊著)
所有代码在Ubuntu 14.04 linux 3.13.0上测试

```bash
make                             # 编译
insmod ./vdisk.ko                # 安装
mkfs -t ext4 /dev/vdisk          # 安装文件系统（格式化）
mkdir -p /mnt/vdisk              # 创建挂载路径
mount /dev/vdisk /mnt/vdisk      # 挂载磁盘
echo hello > /mnt/vdisk/test     # 测试
umount /dev/vdisk                # 卸载
rmmod vdisk                      # 删除模块
```

# FDISK
```
root@ubuntu:/dev# fdisk vdisk
Device contains neither a valid DOS partition table, nor Sun, SGI or OSF disklabel
Building a new DOS disklabel with disk identifier 0xfb557ae6.
Changes will remain in memory only, until you decide to write them.
After that, of course, the previous content won't be recoverable.

Warning: invalid flag 0x0000 of partition table 4 will be corrected by w(rite)

Command (m for help): p

Disk vdisk: 536 MB, 536870912 bytes
32 heads, 32 sectors/track, 1024 cylinders, total 1048576 sectors
Units = sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disk identifier: 0xfb557ae6

Device Boot      Start         End      Blocks   Id  System

Command (m for help): q
```
