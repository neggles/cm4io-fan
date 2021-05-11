#!/usr/bin/env bash
set -e
datestamp=`date -I`

# version/name strings
DRV_NAME="cm4io-fan"
DRV_VERSION="0.1.0-2"
DRV_DIR="/usr/src/${DRV_NAME}-${DRV_VERSION}"

# check kernel arch
KERN_ARCH=$(uname -m)
# uncomment this to override kernel arch check, I don't think this works on 32-bit, but maybe?
#KERN_ARCH="aarch64"

if [[ ${KERN_ARCH} -ne "aarch64" ]]; then
    echo "Error! aarch64 kernel not detected"
    echo "If you'd like to continue anyway, uncomment the override in install.sh"
    exit 1
fi

# make git archive the repo to the right spot
echo -n "Copying source to ${DRV_DIR}..."
[[ -d ${DRV_DIR} ]] && rm -r ${DRV_DIR}
mkdir -p ${DRV_DIR} && git archive HEAD | tar -x -C "/usr/src/${DRV_NAME}-${DRV_VERSION}"
echo "Done"

# run dkms
echo "Running DKMS install..."
sudo dkms install "${DRV_NAME}/${DRV_VERSION}"


# add line to /boot/config.txt
echo -n "Update config.txt... "
if (grep -q "^dtoverlay=${DRV_NAME}" /boot/config.txt); then
    echo "line already present, no change"
else
    sudo echo "dtoverlay=cm4io-fan,minrpm=500,maxrpm=2500" >> /boot/config.txt
    echo "line added, edit to adjust rpm/temp settings."
fi

echo "Done!"
exit 0
