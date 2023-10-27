#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath "$(dirname "$0")")
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
TOOLCHAIN_DIR=$(realpath "$(dirname "$(command -v "${CROSS_COMPILE}gcc")")/..")

if [ $# -lt 1 ]; then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p "${OUTDIR}"

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e "${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image" ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all 
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

echo "Adding the Image in outdir"
cp "${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image" "${OUTDIR}/Image"

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]; then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf "${OUTDIR}/rootfs"
fi

# Create FHS-conform rootfs directory structure
mkdir -p "${OUTDIR}/rootfs"
cd "${OUTDIR}/rootfs"
mkdir bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir usr/bin usr/lib usr/sbin var/log

cd "${OUTDIR}"
if [ ! -d "${OUTDIR}/busybox" ]; then
    git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    make distclean
    make defconfig
else
    cd busybox
fi

make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} CONFIG_PREFIX="${OUTDIR}/rootfs" install

cd "${OUTDIR}/rootfs"

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

cd "${TOOLCHAIN_DIR}/${CROSS_COMPILE%-}/libc"
cp "lib/ld-linux-aarch64.so.1" "${OUTDIR}/rootfs/lib"
cp "lib64/libm.so.6" "lib64/libresolv.so.2" "lib64/libc.so.6" "${OUTDIR}/rootfs/lib64"

cd "${OUTDIR}/rootfs"
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 666 dev/console c 5 1

cd "${FINDER_APP_DIR}"
make clean
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} writer

# on the target rootfs
cp autorun-qemu.sh finder.sh finder-test.sh writer "${OUTDIR}/rootfs/home/"
cp -LR conf "${OUTDIR}/rootfs/home/"

sudo chown -R root: "${OUTDIR}/rootfs"

cd "${OUTDIR}/rootfs"
sudo chmod -R o+rx home lib lib64
find . | cpio -H newc -ov --owner root:root > "${OUTDIR}/initramfs.cpio"
gzip -f "${OUTDIR}/initramfs.cpio"
