#!/usr/bin/env python3
# coding: utf-8

################################################################################
# BLE Logger for ble_sensor
#
#                                               Copyright (c) 2019 Wataru KUNINO
################################################################################

#【インストール方法】
#   bluepy (Bluetooth LE interface for Python)をインストールしてください
#       sudo pip3 install bluepy
#
#【実行方法】
#   実行するときは sudoを付与してください
#       sudo ./ble_logger.py &
#
#【参考文献】
#   本プログラムを作成するにあたり下記を参考にしました
#   https://ianharvey.github.io/bluepy-doc/notifications.html

from bluepy import btle
from bluepy.btle import Peripheral, DefaultDelegate
from sys import argv
import getpass

address = 'xx:xx:xx:xx:xx:xx' # BLEデバイス（ペリフェラル側）のアドレスを記入

class MyDelegate(DefaultDelegate):
    def __init__(self, params):
        DefaultDelegate.__init__(self)
        # ... initialise here

    def handleNotification(self, cHandle, data):
        # ... perhaps check cHandle
        # ... process 'data'
        print( data )

if address == 'xx:xx:xx:xx:xx:xx':
    scanner = btle.Scanner()
    # BLE受信処理
    try:
        devices = scanner.scan(5)
    except Exception as e:
        print("ERROR",e)
        if getpass.getuser() != 'root':
            print('使用方法: sudo', argv[0])
        exit()
    # 受信データについてBLEデバイス毎の処理
    for dev in devices:
        for (adtype, desc, val) in dev.getScanData():
            if adtype == 8 and val[0:10] == 'LapisDev':
                print("\nDevice %s (%s), RSSI=%d dB, Connectable=%s" % (dev.addr, dev.addrType, dev.rssi, dev.connectable))
                print("  %3d %s = %s" % (adtype, desc, val))
                address = dev.addr
if address == 'xx:xx:xx:xx:xx:xx':
    print("no LapisDev found")
    exit()

p = Peripheral(address, addrType='random')
p.setDelegate(MyDelegate(DefaultDelegate))

# Setup to turn notifications on
#   svc = p.getServiceByUUID( service_uuid )
#   ch = svc.getCharacteristics( char_uuid )[0]
#   ch.write( setup_data )
p.writeCharacteristic(0x0404, b'\x01\x00', True)

# Main
print('Waiting...')
p.waitForNotifications(10)
