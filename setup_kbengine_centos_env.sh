#!/bin/bash

# KBEngine CentOS 7.6 编译环境一键配置脚本
# 适用于 KBEngine 3.1.2.11 + MySQL 5.7 + OpenSSL 1.1.1w
# @GeneratedBy: Trae AI GPT-5

# 强制使用 bash 运行脚本（避免 sh 造成语法错误）
if [ -z "$BASH_VERSION" ]; then
    echo "检测到当前未使用 bash，正在使用 bash 重新执行脚本..."
    exec /bin/bash "$0" "$@"
fi

set -e

# 脚本信息
SCRIPT_NAME="setup_kbengine_centos_env.sh"
SCRIPT_VERSION="1.0"
LOG_FILE="kbengine_env_setup.log"
CURRENT_DIR="$(pwd)"

# 全局状态变量
MYSQL_INSTALLED=false
OPENSSL_INSTALLED=false

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 日志函数
log() {
    local level=$1
    local message=$2
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo -e "${timestamp} [${level}] ${message}" | tee -a "${LOG_FILE}"
}

# 检查函数
check_command() {
    if command -v "$1" >/dev/null 2>&1; then
        return 0
    else
        return 1
    fi
}

# 检查是否以root权限运行
check_root() {
    if [[ $EUID -ne 0 ]]; then
        echo -e "${RED}错误: 此脚本必须以root权限运行${NC}"
        echo "请使用: sudo bash $0"
        exit 1
    fi
}

# 显示欢迎信息
show_welcome() {
    echo -e "${GREEN}"
    echo "==============================================="
    echo "   KBEngine CentOS 7.6 编译环境配置脚本"
    echo "   版本: ${SCRIPT_VERSION}"
    echo "==============================================="
    echo -e "${NC}"
    log "INFO" "开始执行 KBEngine 环境配置"
}

# 安装基础开发工具
install_basic_tools() {
    log "INFO" "安装基础开发工具..."
    
    local packages=(
        "tcl" "openssl-devel" "gcc" "gcc-c++" "kernel-devel" "GNU libtool" "expect"
        "net-tools.x86_64" "libaio" "apr-util" "perl" "perl-devel"
        "autoconf" "automake" "libtool" "git" "wget" "curl" "ca-certificates"
    )
    
    yum install -y "${packages[@]}" 2>>"${LOG_FILE}"
    
    log "INFO" "安装 Development Tools 组..."
    yum groupinstall -y "Development Tools" 2>>"${LOG_FILE}"
}

# 安装开发库
install_development_libs() {
    log "INFO" "安装开发库..."
    
    local libs=(
        "openssl-devel" "bzip2-devel" "ncurses-devel" "readline-devel"
        "sqlite-devel" "tk-devel" "libffi-devel" "xz-devel" "zlib-devel"
        "uuid-devel" "libuuid-devel" "gdbm-devel"
    )
    
    yum install -y "${libs[@]}" 2>>"${LOG_FILE}"
}

# 检查 MySQL 5.7 是否已安装
check_mysql_installed() {
    log "INFO" "检查 MySQL 5.7 安装状态..."
    
    if rpm -qa | grep -q "mysql-community-server-5.7"; then
        log "INFO" "MySQL 5.7 已安装，跳过安装"
        MYSQL_INSTALLED=true
        return 0
    fi
    
    if check_command "mysql"; then
        local mysql_version=$(mysql --version 2>/dev/null | grep -o "5\.[0-9]\.[0-9]" || echo "")
        if [[ "$mysql_version" == "5.7"* ]]; then
            log "INFO" "MySQL 5.7 已安装，跳过安装"
            MYSQL_INSTALLED=true
            return 0
        fi
    fi
    
    log "INFO" "MySQL 5.7 未安装，需要安装"
    MYSQL_INSTALLED=false
    return 0
}

# 安装 MySQL 5.7
install_mysql() {
    if $MYSQL_INSTALLED; then
        return 0
    fi
    
    log "INFO" "开始安装 MySQL 5.7..."
    
    # 下载 MySQL 5.7 仓库配置
    local mysql_repo_url="https://dev.mysql.com/get/mysql57-community-release-el7-11.noarch.rpm"
    local mysql_repo_file="mysql57-community-release-el7-11.noarch.rpm"
    
    if [[ ! -f "$mysql_repo_file" ]]; then
        log "INFO" "下载 MySQL 5.7 仓库配置..."
        wget "$mysql_repo_url" -O "$mysql_repo_file" 2>>"${LOG_FILE}"
    fi
    
    # 安装仓库配置
    rpm -ivh "$mysql_repo_file" --nodeps 2>>"${LOG_FILE}"
    
    # 导入 GPG 密钥
    rpm --import https://repo.mysql.com/RPM-GPG-KEY-mysql 2>>"${LOG_FILE}"
    
    # 安装 MySQL (跳过GPG验证)
    local mysql_packages=(
        "mysql-community-server" 
        "mysql-community-client" 
        "mysql-community-devel" 
        "mysql-community-libs" 
        "mysql-community-common"
    )
    
    yum install -y "${mysql_packages[@]}" --nogpgcheck 2>>"${LOG_FILE}"
    
    log "INFO" "MySQL 5.7 安装完成"
}

# 检查 OpenSSL 1.1.1w 是否已安装
check_openssl_installed() {
    log "INFO" "检查 OpenSSL 安装状态..."
    
    if check_command "openssl"; then
        local openssl_version=$(openssl version 2>/dev/null | awk '{print $2}')
        if [[ "$openssl_version" == "1.1.1w" ]]; then
            log "INFO" "OpenSSL 1.1.1w 已安装，跳过安装"
            OPENSSL_INSTALLED=true
            return 0
        fi
    fi
    
    log "INFO" "OpenSSL 1.1.1w 未安装或版本不匹配"
    OPENSSL_INSTALLED=false
    return 0
}

# 安装 OpenSSL 1.1.1w
install_openssl() {
    if $OPENSSL_INSTALLED; then
        return 0
    fi
    
    log "INFO" "开始安装 OpenSSL 1.1.1w..."
    
    local openssl_url="https://www.openssl.org/source/openssl-1.1.1w.tar.gz"
    local openssl_file="openssl-1.1.1w.tar.gz"
    
    # 下载 OpenSSL
    if [[ ! -f "$openssl_file" ]]; then
        log "INFO" "下载 OpenSSL 1.1.1w 源码（显示下载进度）..."
        # 优先使用 wget 并显示进度，失败则回退到 curl
        if ! wget --progress=dot:mega --tries=3 --timeout=30 "$openssl_url" -O "$openssl_file" 2>>"${LOG_FILE}"; then
            log "WARN" "wget 下载失败，尝试使用 curl（带进度条）"
            curl -L "$openssl_url" -o "$openssl_file" --progress-bar --retry 3 --max-time 180 2>&1 | tee -a "${LOG_FILE}"
        fi
        # 再次确认文件是否下载成功
        if [[ ! -s "$openssl_file" ]]; then
            log "ERROR" "OpenSSL 源码下载失败，请检查网络或证书"
            exit 1
        fi
    fi
    
    # 解压
    tar -xzf "$openssl_file"
    cd "openssl-1.1.1w"
    
    # 编译安装
    ./config --prefix=/usr/local/openssl --openssldir=/usr/local/openssl shared zlib
    make -j$(nproc)
    make install
    
    # 创建符号链接
    ln -sf /usr/local/openssl/bin/openssl /usr/local/bin/openssl
    ln -sf /usr/local/openssl/include/openssl /usr/include/openssl
    
    # 更新动态库配置
    echo "/usr/local/openssl/lib" > /etc/ld.so.conf.d/openssl.conf
    ldconfig
    
    cd "$CURRENT_DIR"
    log "INFO" "OpenSSL 1.1.1w 安装完成"
}

# 配置环境变量
setup_environment() {
    log "INFO" "配置环境变量..."
    
    # 添加到 ~/.bashrc
    cat << 'EOF' >> /etc/profile.d/kbengine_env.sh
export PATH=/usr/local/openssl/bin:$PATH
export LD_LIBRARY_PATH=/usr/local/openssl/lib:$LD_LIBRARY_PATH
export PKG_CONFIG_PATH=/usr/local/openssl/lib/pkgconfig:$PKG_CONFIG_PATH
EOF
    
    source /etc/profile.d/kbengine_env.sh
}

# 验证安装
verify_installation() {
    log "INFO" "验证安装结果..."
    
    echo -e "${BLUE}=== 安装验证 ===${NC}"
    
    # 检查基础工具
    local tools=("gcc" "g++" "make" "autoconf" "automake")
    for tool in "${tools[@]}"; do
        if check_command "$tool"; then
            echo -e "${GREEN}✓ $tool 已安装${NC}"
        else
            echo -e "${RED}✗ $tool 未安装${NC}"
        fi
    done
    
    # 检查 MySQL
    if check_command "mysql"; then
        local mysql_ver=$(mysql --version 2>/dev/null)
        echo -e "${GREEN}✓ MySQL 已安装: $mysql_ver${NC}"
    else
        echo -e "${RED}✗ MySQL 未安装${NC}"
    fi
    
    # 检查 OpenSSL
    if check_command "openssl"; then
        local openssl_ver=$(openssl version 2>/dev/null)
        echo -e "${GREEN}✓ OpenSSL 已安装: $openssl_ver${NC}"
    else
        echo -e "${RED}✗ OpenSSL 未安装${NC}"
    fi
    
    # 检查开发库
    local libs=("libssl.so" "libcrypto.so" "libmysqlclient.so")
    for lib in "${libs[@]}"; do
        if ldconfig -p | grep -q "$lib"; then
            echo -e "${GREEN}✓ $lib 库文件存在${NC}"
        else
            echo -e "${YELLOW}⚠ $lib 库文件可能需要检查${NC}"
        fi
    done
}

# 清理工作
cleanup() {
    log "INFO" "清理临时文件..."
    
    # 保留下载的文件供后续使用
    echo -e "${YELLOW}下载的文件保留在当前目录:${NC}"
    ls -la *.rpm *.tar.gz 2>/dev/null || echo "暂无下载文件"
}

# 显示完成信息
show_completion() {
    echo -e "${GREEN}"
    echo "==============================================="
    echo "   KBEngine 环境配置完成!"
    echo "==============================================="
    echo -e "${NC}"
    
    echo -e "${BLUE}后续步骤:${NC}"
    echo "1. 重启终端或执行: source /etc/profile"
    echo "2. 进入 KBEngine 源码目录: cd kbe/src"
    echo "3. 执行编译: make"
    echo ""
    echo -e "${YELLOW}详细日志请查看: ${LOG_FILE}${NC}"
}

# 主函数
main() {
    check_root
    show_welcome
    
    # 安装基础工具
    install_basic_tools
    install_development_libs
    
    # 检查并安装 MySQL
    check_mysql_installed
    install_mysql
    
    # 检查并安装 OpenSSL
    check_openssl_installed
    install_openssl
    
    # 配置环境
    setup_environment
    
    # 验证安装
    verify_installation
    
    # 清理
    cleanup
    
    # 完成
    show_completion
    
    log "INFO" "KBEngine 环境配置完成"
}

# 异常处理
trap 'log "ERROR" "脚本执行被中断"; exit 1' INT TERM
trap 'log "ERROR" "脚本执行错误: $? at line $LINENO"; exit 1' ERR

# 执行主函数
main "$@"