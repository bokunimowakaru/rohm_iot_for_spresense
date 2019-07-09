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

address = 'xx:xx:xx:xx:xx:xx' # BLEデバイス（ペリフェラル側）のアドレスを記入

class MyDelegate(DefaultDelegate):
    def __init__(self, params):
        DefaultDelegate.__init__(self)
        # ... initialise here

    def handleNotification(self, cHandle, data):
        # ... perhaps check cHandle
        # ... process 'data'
        print( data )

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
