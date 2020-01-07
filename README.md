# Traverse Sensor/Hwmon sensor driver staging

These hwmon drivers are for new devices currently not in the mainline Linux kernel.

These drivers are installed via [dkms](https://github.com/dell/dkms) so you do not need to build a full custom kernel to use them. Eventually these drivers will be submitted for inclusion upstream.

## Microchip EMC17XX
* Known issues: Needs attribute rework.

## Microchip PAC1934
* Known issues: Needs attribute rework. A reference IIO driver is available on the Microchip website

## Microchip EMC181X
* Draft version - only tested on EMC1813 so far.
