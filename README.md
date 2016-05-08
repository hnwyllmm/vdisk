# vdisk
写一个块设备驱动(赵磊著)
所有代码在Ubuntu 14.04 linux 3.13.0上测试

**第四章：支持磁盘分区**

```bash
make                             # 编译
insmod ./vdisk.ko                # 安装
fdisk /dev/vdisk                 # 磁盘分区
mkfs -t ext4 /dev/vdisk1         # 安装文件系统（格式化）
mkfs -t ext4 /dev/vdisk2         # 安装文件系统（格式化）
mkdir -p /mnt/vdisk1             # 创建挂载路径
mkdir -p /mnt/vdisk2             # 创建挂载路径
mount /dev/vdisk1 /mnt/vdisk1    # 挂载磁盘
mount /dev/vdisk2 /mnt/vdisk2    # 挂载磁盘
echo hello > /mnt/vdisk1/test    # 测试
umount /dev/vdisk1 /dev/vdisk2   # 卸载
rmmod vdisk                      # 删除模块
```

## 磁盘分区
```
root@ubuntu:~/vdisk# fdisk /dev/vdisk 
Device contains neither a valid DOS partition table, nor Sun, SGI or OSF disklabel
Building a new DOS disklabel with disk identifier 0x1bc85f07.
Changes will remain in memory only, until you decide to write them.
After that, of course, the previous content won't be recoverable.

Warning: invalid flag 0x0000 of partition table 4 will be corrected by w(rite)

Command (m for help): p

Disk /dev/vdisk: 16 MB, 16777216 bytes
255 heads, 63 sectors/track, 2 cylinders, total 32768 sectors
Units = sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disk identifier: 0x1bc85f07

     Device Boot      Start         End      Blocks   Id  System

Command (m for help): n
Partition type:
   p   primary (0 primary, 0 extended, 4 free)
   e   extended
Select (default p): p
Partition number (1-4, default 1): 1
First sector (2048-32767, default 2048): 16384
Last sector, +sectors or +size{K,M,G} (16384-32767, default 32767): 
Using default value 32767

Command (m for help): n
Partition type:
   p   primary (1 primary, 0 extended, 3 free)
   e   extended
Select (default p): p
Partition number (1-4, default 2): 2
First sector (2048-32767, default 2048): 2048
Last sector, +sectors or +size{K,M,G} (2048-16383, default 16383): 
Using default value 16383

Command (m for help): p

Disk /dev/vdisk: 16 MB, 16777216 bytes
255 heads, 63 sectors/track, 2 cylinders, total 32768 sectors
Units = sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disk identifier: 0x1bc85f07

     Device Boot      Start         End      Blocks   Id  System
/dev/vdisk1           16384       32767        8192   83  Linux
/dev/vdisk2            2048       16383        7168   83  Linux

Partition table entries are not in disk order

Command (m for help): w
The partition table has been altered!

Calling ioctl() to re-read partition table.
Syncing disks.
```

> NOTE: 3.13中磁盘分区已经不是按照磁道来划分，书中介绍的知识已经过时。按照fdisk的介绍，分区按照扇区划分
root@ubuntu:~/vdisk# ll /dev/vdisk*
brw-rw---- 1 root disk 72, 0 May  7 19:51 /dev/vdisk
brw-rw---- 1 root disk 72, 1 May  7 19:51 /dev/vdisk1
brw-rw---- 1 root disk 72, 2 May  7 19:51 /dev/vdisk2
brw-rw---- 1 root disk 72, 3 May  7 19:51 /dev/vdisk3

## 查看磁盘分区
```
root@ubuntu:/dev# ls vdisk*
vdisk  vdisk1  vdisk2
root@ubuntu:/dev# ll vdisk*
brw-rw---- 1 root disk 72, 0 May  7 19:19 vdisk
brw-rw---- 1 root disk 72, 1 May  7 19:19 vdisk1
brw-rw---- 1 root disk 72, 2 May  7 19:19 vdisk2
```

##制作文件系统
```
root@ubuntu:~/vdisk# mkfs -t ext4 /dev/vdisk1
mke2fs 1.42.9 (4-Feb-2014)
Filesystem label=
OS type: Linux
Block size=1024 (log=0)
Fragment size=1024 (log=0)
Stride=0 blocks, Stripe width=0 blocks
2048 inodes, 8192 blocks
409 blocks (4.99%) reserved for the super user
First data block=1
Maximum filesystem blocks=8388608
1 block group
8192 blocks per group, 8192 fragments per group
2048 inodes per group

Allocating group tables: done                            
Writing inode tables: done                            
Creating journal (1024 blocks): done
Writing superblocks and filesystem accounting information: done

root@ubuntu:~/vdisk# mkfs -t ext4 /dev/vdisk2
mke2fs 1.42.9 (4-Feb-2014)
Filesystem label=
OS type: Linux
Block size=1024 (log=0)
Fragment size=1024 (log=0)
Stride=0 blocks, Stripe width=0 blocks
1792 inodes, 7168 blocks
358 blocks (4.99%) reserved for the super user
First data block=1
Maximum filesystem blocks=7340032
1 block group
8192 blocks per group, 8192 fragments per group
1792 inodes per group

Allocating group tables: done                            
Writing inode tables: done                            
Creating journal (1024 blocks): done
Writing superblocks and filesystem accounting information: done
```
