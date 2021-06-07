#!/bin/sh

#Requires root
if [ $EUID != 0 ]; then
	echo "ERROR: This script must be run as root!"
	exit -1
fi

echo "removing service ..."
systemctl stop fantable
systemctl disable fantable
echo "done"

echo "removing /usr/sbin/fantable ..."
rm -r /usr/sbin/fantable
echo "done"

echo "removing service from /lib/systemd/system/ ..."
rm /etc/systemd/system/fantable.service
echo "done"

echo "reloading services ..."
systemctl daemon-reload
echo "done"

echo "fantable was completely uninstalled from the system"
