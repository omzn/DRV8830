#!/bin/sh
#/usr/sbin/i2cset -y 1 0x64 0x00 0x7E
/usr/sbin/i2cset -y 1 0x64 0x00 0x7D
sleep 2
/usr/sbin/i2cset -y 1 0x64 0x00 0x00
/usr/sbin/i2cset -y 1 0x64 0x00 0x03
