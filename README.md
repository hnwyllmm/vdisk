# vdisk
写一个块设备驱动(赵磊著)
第一章对应代码，在3.13.0 Ubuntu上测试通过
创建了ext4文件系统
make                             # 编译
insmod ./vdisk.ko                # 安装
mkfs -t ext4 /dev/vdisk          # 安装文件系统（格式化）
mkdir -p /mnt/vdisk              # 创建挂载路径
mount /dev/vdisk /mnt/vdisk      # 挂载磁盘
echo hello > /mnt/vdisk/test     # 测试
umount /dev/vdisk                # 卸载
rmmod vdisk                      # 删除模块
