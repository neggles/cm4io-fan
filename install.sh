#!/usr/bin/env bash
set -e

# version/name strings
DRV_NAME="cm4io-fan"
DRV_VERSION="0.1.0-1"

# check kernel arch
KERN_ARCH=$(uname -m)
# uncomment this to override kernel arch check
# I don't think this works on 32-bit, but maybe?
#KERN_ARCH="aarch64"

# kernel arch check
if [[ ${KERN_ARCH} -ne "aarch64" ]]; then
    echo "Error! aarch64 kernel not detected"
    echo "If you'd like to continue anyway, uncomment the override in install.sh"
    exit 1
fi

# make git archive the repo to the right spot
git archive HEAD | tar -x -C "/usr/src/${DRV_NAME}-${DRV_VERSION}"

# run dkms
sudo dkms install "${DRV_NAME}/${DRV_VERSION}"

# add line to /boot/config.txt
if (grep -q "^dtoverlay=${DRV_NAME}" /boot/config.txt); then
    echo "config.txt param already present"
else
    echo "Adding line to config.txt..."
    sudo echo "dtoverlay=cm4io-fan,minrpm=500,maxrpm=2500" >> /boot/config.txt
fi

exit 0
