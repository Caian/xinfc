# Building for OpenWRT using docker

Start a new docker instance:

```
sudo docker run --rm -ti --volume "$PWD:/xinfc" ubuntu:20.04 /bin/bash
```

Install requirements and add the build user:

```
cd /

apt update
apt -y install perl build-essential libncurses5-dev libmpfr-dev \
               libgmp-dev libpam-dev liblzma-dev file git rsync gawk wget \
               python3 python3-distutils python3-setuptools unzip swig

useradd openwrt
mkdir openwrt
chmod 777 openwrt
cd openwrt
```

Do the build process as a regular user:

```
su openwrt
```

Grab the OpenWRT source and build it ([dimfishr's build](https://github.com/dimfishr/openwrt)):

```
wget https://github.com/dimfishr/openwrt/archive/refs/tags/filogic-2023-12-27.zip

unzip filogic-2023-12-27.zip

cd openwrt-filogic-2023-12-27

./scripts/feeds update -a
./scripts/feeds install -a

echo "CONFIG_TARGET_mediatek=y\n" > .config
echo "CONFIG_TARGET_mediatek_filogic=y\n" >> .config
echo "CONFIG_TARGET_MULTI_PROFILE=y\n" >> .config
echo "CONFIG_TARGET_PER_DEVICE_ROOTFS=y\n" >> .config
echo "CONFIG_TARGET_DEVICE_mediatek_filogic_DEVICE_xiaomi_mi-router-ax3000t=y\n" >> .config
echo "CONFIG_TARGET_DEVICE_mediatek_filogic_DEVICE_xiaomi_mi-router-ax3000t-ubootmod=y\n" >> .config
echo "CONFIG_PACKAGE_wpad-basic-mbedtls=m\n" >> .config
echo "CONFIG_PACKAGE_libwolfsslcpu-crypto=y\n" >> .config
echo "CONFIG_PACKAGE_wpad-wolfssl=y\n" >> .config
echo "CONFIG_PACKAGE_dnsmasq=m\n" >> .config
echo "CONFIG_PACKAGE_dnsmasq-full=y\n" >> .config
echo "CONFIG_PACKAGE_kmod-nf-nathelper=y\n" >> .config
echo "CONFIG_PACKAGE_kmod-nf-nathelper-extra=y\n" >> .config
echo "CONFIG_PACKAGE_luci=y\n" >> .config
echo "CONFIG_PACKAGE_luci-proto-wireguard=y\n" >> .config
echo "CONFIG_PACKAGE_xl2tpd=y\n" >> .config

make defconfig
make download V=s
make tools/install -j$(nproc) V=s || \
make tools/install V=s
make toolchain/install -j$(nproc) V=s || \
make toolchain/install V=s
make -j$(nproc) V=s || \
make V=s
```

Set the compiler:

```
export PATH="/openwrt/openwrt-filogic-2023-12-27/build_dir/target-aarch64_cortex-a53_musl/linux-mediatek_filogic/linux-5.15.145/scripts/dtc:/openwrt/openwrt-filogic-2023-12-27/staging_dir/toolchain-aarch64_cortex-a53_gcc-12.3.0_musl/bin:/openwrt/openwrt-filogic-2023-12-27/staging_dir/toolchain-aarch64_cortex-a53_gcc-12.3.0_musl/bin:/openwrt/openwrt-filogic-2023-12-27/staging_dir/toolchain-aarch64_cortex-a53_gcc-12.3.0_musl/bin:/openwrt/openwrt-filogic-2023-12-27/staging_dir/toolchain-aarch64_cortex-a53_gcc-12.3.0_musl/bin:/openwrt/openwrt-filogic-2023-12-27/staging_dir/host/bin:/openwrt/openwrt-filogic-2023-12-27/staging_dir/toolchain-aarch64_cortex-a53_gcc-12.3.0_musl/bin:/openwrt/openwrt-filogic-2023-12-27/staging_dir/host/bin:/openwrt/openwrt-filogic-2023-12-27/staging_dir/host/bin:$PATH"
export CXX=/openwrt/openwrt-filogic-2023-12-27/staging_dir/toolchain-aarch64_cortex-a53_gcc-12.3.0_musl/bin/aarch64-openwrt-linux-g++
```

**If you plan to rebuild the code, then this is a good time to `docker commit` the container into an image so you don't have to do this all over again.**

Build xinfc:

```
cd /xinfc
bash build.sh
```