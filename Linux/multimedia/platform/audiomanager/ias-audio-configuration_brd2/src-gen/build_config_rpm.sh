#!/bin/sh

function usage {
  echo "Usage:"
  echo "./build_config_rpm.sh <SYSROOT_FOLDER>"
  echo "SYSROOT_FOLDER                The folder where the sysroot is located"
}

if [ $# != 1 ]; then
  usage
  exit -1
fi

if [ ! -d $1 ]; then
  echo "The provided sysroot folder does not exist"
  exit -1
fi

if [ ! -e "$1/root" ]; then
  echo "The sysroot folder does not seem to contain a valid sysroot"
  exit -1
fi

VERSION="10.0.0"

SRC_FOLDER="ias-audio-configuration_brd2-$VERSION"
SRC_TAR="ias-audio-configuration_brd2-$VERSION.tar.gz"

tar cvfa $SRC_TAR $SRC_FOLDER

# We need root privileges for the following actions
sudo mkdir -p $1/root/rpmbuild/SPECS
sudo mkdir -p $1/root/rpmbuild/SOURCES
sudo cp ias-audio-configuration_brd2-$VERSION/packaging/ias-audio-configuration_brd2.spec $1/root/rpmbuild/SPECS
sudo cp ias-audio-configuration_brd2-$VERSION.tar.gz $1/root/rpmbuild/SOURCES
sudo chroot $1 rpmbuild -bb --target=i586 /root/rpmbuild/SPECS/ias-audio-configuration_brd2.spec
sudo chmod o+x $1/root
sudo chmod o+rw $1/root/rpmbuild/RPMS/i586/ias-audio-configuration_brd2-$VERSION-0.i586.rpm
cp $1/root/rpmbuild/RPMS/i586/ias-audio-configuration_brd2-$VERSION-0.i586.rpm .
