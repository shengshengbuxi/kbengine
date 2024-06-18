# 配置开发环境

# 第一步：安装必要的软件，如果已经安装则跳过
# 1、升级OpenSSL，因为CentOS 7自带的OpenSSL版本太低，导致编译安装内置Python3.12.3时报错
# 2、需要1.1.1以上的版本，这里安装3.0.13版本（Python3.12.3对应的OpenSSL版本是3.0.13）
cd /mnt
mkdir openssl
cd openssl
sudo yum groupinstall "Development Tools"
sudo yum install perl-core zlib-devel
wget https://www.openssl.org/source/openssl-3.0.13.tar.gz
tar -xvzf openssl-3.0.13.tar.gz
cd openssl-3.0.13
./config --prefix=/usr/local/openssl --openssldir=/usr/local/openssl
make
sudo make install
sudo mv /usr/bin/openssl /usr/bin/openssl.bak
sudo ln -s /usr/local/openssl/bin/openssl /usr/bin/openssl
sudo sh -c "echo '/usr/local/openssl/lib' > /etc/ld.so.conf.d/openssl-3.0.13.conf"
sudo ldconfig
openssl version


# 第二步：安装必要的软件，如果已经安装则跳过
sudo yum groupinstall -y "Development Tools"
sudo yum install -y openssl-devel bzip2-devel libffi-devel zlib-devel libtirpc-devel
sudo yum install -y  mysql-devel
sudo yum install -y ncurses-compat-libs
sudo yum install -y openssl-libs
dnf install -y compat-openssl10
sudo yum install -y readline readline-devel


# 第三步：创建虚拟内存，编译引擎需要3GB以上的内存，否则会编译失败
swapon --show
# 创建一个4GB的交换文件
sudo fallocate -l 4G /swapfile
sudo dd if=/dev/zero of=/swapfile bs=1M count=2048
# 将这个文件设置为交换空间
sudo mkswap /swapfile
# 启用交换空间
sudo swapon /swapfile
# 再次检查交换分区是否启用
swapon --show
# 将交换文件添加到/etc/fstab以便开机自动挂载
echo '/swapfile none swap sw 0 0' | sudo tee -a /etc/fstab

