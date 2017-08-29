#!/bin/bash

# YUM_CMD=$(which yum)
APT_GET_CMD=$(which apt-get)
OPKG_CMD=$(which opkg)

HAS_HAVAHI=$(which avahi-daemon)

DEFAULT_NAME=my_iot_device

while [[ "$#" > 1 ]]; do case $1 in
    --name) name="$2";;
    --ignore_install) ignore_install="$2";;
    *) break;;
  esac; shift; shift
done

if [[ ! -z $HAS_HAVAHI ]]; then
  ignore_install=yes
fi

if [ "$ignore_install" != "yes" ];then 
  if [[ ! -z $APT_GET_CMD ]]; then
    sudo apt-get install avahi-daemon cmake -y
  elif [[ ! -z $OPKG_CMD ]]; then
    systemctl stop mdns
    systemctl disable mdns
    systemctl stop edison_config
    systemctl disable edison_config

    cat >> /etc/opkg/base-feeds.conf <<- "EOF"
src/gz all http://repo.opkg.net/edison/repo/all
src/gz edison http://repo.opkg.net/edison/repo/edison
src/gz core2-32 http://repo.opkg.net/edison/repo/core2-32
EOF

    opkg update
    opkg install --force-reinstall avahi
    opkg install cmake
    else
      echo "error can't install avahi package. Install it manually and execute this with argument \'--ignore_install yes'"
      exit 1;
    fi
fi

if [ ! -f /etc/avahi/avahi-daemon.conf ]; then
    echo "File (/etc/avahi/avahi-daemon.conf) not found!"
    echo "The avahi service was not properly installed"
    exit 1;
fi


if [ ! -f /etc/avahi/original_avahi-daemon.conf ]; then
  cp /etc/avahi/avahi-daemon.conf /etc/avahi/original_avahi-daemon.conf
fi

sed -ie "s/.*host-name=.*/host-name=$name\_miletus/g" /etc/avahi/avahi-daemon.conf

if [ -z "$name" ]
  then
    echo "Device name was defined to $DEFAULT_NAME"
    name=$DEFAULT_NAME
fi

if [ ! -f /etc/avahi/services/miletus.service ]; then
  cat >> /etc/avahi/services/miletus.service <<- "EOF"
<?xml version="1.0" standalone='no'?><!--*-nxml-*-->
<!DOCTYPE service-group SYSTEM "avahi-service.dtd">
<service-group>
<name replace-wildcards="yes">%h HTTP</name>
<service>
<type>_http._tcp</type>
<port>80</port>
</service>
</service-group>
EOF
fi

systemctl stop edison_config

echo "RESULT: mDNS server configuration success!"