#!/bin/bash
systemctl stop ipod-player.service
modprobe i2c-dev

/bin/ipod-iap2_iap1_smoketest.out
