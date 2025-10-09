
all:: config.status

CONFIG_ARGS =

CFLAGS += -I/usr/include/openssl
LDFLAGS += -L/usr/lib64 -L/usr/lib -lssl -lcrypto -ldl

# 检测系统OpenSSL文件位置
# 首先检查符号链接指向的实际文件
ifneq ("$(wildcard /usr/local/openssl/lib/libssl.so.1.1)", "")
	CONFIG_SSL = --with-openssl=/usr/local/openssl --with-openssl-rpath=auto
else ifneq ("$(wildcard /usr/local/openssl/lib64/libssl.so.1.1)", "")
	CONFIG_SSL = --with-openssl=/usr/local/openssl --with-openssl-rpath=auto
else ifneq ("$(wildcard /usr/lib64/libssl.so.1.1.1)", "")
	CONFIG_SSL = --with-openssl=/usr --with-openssl-rpath=auto
else ifneq ("$(wildcard /usr/lib64/libssl.so.1.1)", "")
	CONFIG_SSL = --with-openssl=/usr --with-openssl-rpath=auto
else ifneq ("$(wildcard /usr/lib/libssl.so.1.1.1)", "")
	CONFIG_SSL = --with-openssl=/usr --with-openssl-rpath=auto
else ifneq ("$(wildcard /usr/lib/libssl.so.1.1)", "")
	CONFIG_SSL = --with-openssl=/usr --with-openssl-rpath=auto
else ifneq ("$(wildcard /usr/lib64/libssl.so)", "")
	# 检查符号链接是否指向有效文件
	ifneq ("$(wildcard $(shell readlink -f /usr/lib64/libssl.so))", "")
		CONFIG_SSL = --with-openssl=/usr --with-openssl-rpath=auto
	else
		CONFIG_SSL = --with-openssl=$(KBE_ROOT)/kbe/src/lib/dependencies/openssl --with-openssl-rpath=auto
	endif
else ifneq ("$(wildcard /usr/lib/libssl.so)", "")
	# 检查符号链接是否指向有效文件
	ifneq ("$(wildcard $(shell readlink -f /usr/lib/libssl.so))", "")
		CONFIG_SSL = --with-openssl=/usr --with-openssl-rpath=auto
	else
		CONFIG_SSL = --with-openssl=$(KBE_ROOT)/kbe/src/lib/dependencies/openssl --with-openssl-rpath=auto
	endif
else ifneq ("$(wildcard /usr/local/openssl/bin/openssl)", "")
	# 处理软链接情况，使用真实路径
	CONFIG_SSL = --with-openssl=/usr/local/openssl --with-openssl-rpath=auto
else ifneq ("$(wildcard /usr/local/openssl/lib/libssl.a)", "")
	CONFIG_SSL = --with-openssl=/usr/local/openssl --with-openssl-rpath=auto
else ifneq ("$(wildcard /usr/local/openssl/lib64/libssl.a)", "")
	CONFIG_SSL = --with-openssl=/usr/local/openssl --with-openssl-rpath=auto
else
	CONFIG_SSL = --with-openssl=$(KBE_ROOT)/kbe/src/lib/dependencies/openssl --with-openssl-rpath=auto
endif

ifeq ($(pythonbuild), release)
	CONFIG_ARGS += --with-openssl=/usr --enable-optimizations
else
ifneq (,$(findstring 64,$(KBE_CONFIG)))
	CONFIG_ARGS += --enable-optimizations --with-openssl=/usr
endif
endif

config.status:
	@chmod -R 755 .
	export KBE_ROOT=$(KBE_ROOT)
	$(SHELL) configure $(CONFIG_ARGS) $(CONFIG_SSL)
	@make

.PHONY: clean

clean::
	echo "clean"

# KBE modifys:
# 1: PCbuild vcxprojs to KBE-vcxproj

# 2: configure
# OPENSSL_LDFLAGS="-L${KBE_ROOT}/kbe/src/libs"

# 3:Modules/Setup.dist

# 4: setup.py
#       system_lib_dirs = ['/lib64', '/usr/lib64', '/lib', '/usr/lib', '$(KBE_ROOT)/kbe/src/libs']
#       system_include_dirs = ['/usr/include', '$(KBE_ROOT)/kbe/src/lib/dependencies/zlib']
