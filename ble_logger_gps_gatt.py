#!/usr/bin/env python3
# coding: utf-8

################################################################################
# BLE Logger for ble_gps
#
# Bluetooth LE拡張ボードを搭載した Spresenseが送信するビーコンを受信し、
# Lapis独自のVSSPPプロファイルでセンサ値を取得するサンプル・プログラムです。
#
#                                               Copyright (c) 2019 Wataru KUNINO
################################################################################

#【インストール方法】
#   bluepy (Bluetooth LE interface for Python)をインストールしてください
#       sudo pip3 install bluepy
#
#【実行方法】
#   実行するときは sudoを付与してください
#       sudo ./ble_logger_sens_gatt.py &
#
#【参考文献】
#   本プログラムを作成するにあたり下記を参考にしました
#   https://ianharvey.github.io/bluepy-doc/notifications.html

from bluepy import btle
from bluepy.btle import Peripheral, DefaultDelegate
from sys import argv
import getpass

address = 'xx:xx:xx:xx:xx:xx' # BLEデバイス（ペリフェラル側）のアドレスを記入

def payval(val,num, bytes=1, sign=False):
    a = 0
    for i in range(0, bytes):
        a += (256 ** i) * int(val[(num - 2 + i) * 2 : (num - 1 + i) * 2],16)
    if sign:
        if a >= 2 ** (bytes * 8 - 1):
            a -= 2 ** (bytes * 8)
    return a

class MyDelegate(DefaultDelegate):
    val = ''

    def __init__(self, params):
        DefaultDelegate.__init__(self)
        # ... initialise here

    def handleNotification(self, cHandle, data):
        # ... perhaps check cHandle
        # ... process 'data'
        val = data.hex()
        print(val)
        sensors = dict()

        # センサ値を辞書型変数sensorsへ代入
        sensors['ID'] = hex(payval(val,2,2))
        sensors['Latitude'] = payval(val,4,3,True) / 8388607 * 90
        sensors['LatLast8'] = payval(val,11,1) / 2147483647 * 90
        sensors['Longitude'] = payval(val,7,3,True) / 8388607 * 180
        sensors['LonLast8'] = payval(val,12,1) / 2147483647 * 180
        sensors['SEQ'] = payval(val,10)
        sensors['RSSI'] = dev.rssi

        # 画面へ表示
        print('    ID            =',sensors['ID'])
        print('    SEQ           =',sensors['SEQ'])
        print('    GPS Latitude  =',round(sensors['Latitude'],5),'°')
        print('          (32bit) =',round(sensors['Latitude']+sensors['LatLast8'],8),'°')
        print('    GPS Longitude =',round(sensors['Longitude'],5),'°')
        print('          (32bit) =',round(sensors['Longitude']+sensors['LonLast8'],7),'°')
        print('    RSSI          =',sensors['RSSI'],'dB')

if address[0] == 'x':
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
if address[0] == 'x':
    print("no LapisDev found")
    exit()

p = Peripheral(address, addrType='random')	# Failed to connect to peripheral
p.setDelegate(MyDelegate(DefaultDelegate))

# Setup to turn notifications on
svc = p.getServiceByUUID('0179bbd0-5351-48b5-bf6d-2167639bc867')
ch = svc.getCharacteristics('0179bbd1-5351-48b5-bf6d-2167639bc867')[0]
ch.handle += 2
#ch.write(b'\x01')
print('uuid =',ch.uuid, 'handle =',hex(ch.handle))
p.writeCharacteristic(ch.handle, b'\x01')

# Main
print('Waiting...')
p.waitForNotifications(20)

'''
（bluepyのダウンロードとインストール）
pi@raspberrypi:~ $ sudo apt-get update
pi@raspberrypi:~ $ sudo pip3 install bluepy

（プログラムのダウンロード）
pi@raspberrypi:~
$ git clone http://github.com/bokunimowakaru/rohm_iot_for_spresense

（プログラムの実行）
pi@raspberrypi:~ $ cd rohm_iot_for_spresense
pi@raspberrypi:~/rohm_iot_for_spresense 
$ sudo ./ble_logger_gps_gatt.py
Device xx:xx:xx:xx:xx:xx (random), RSSI=-83 dB, Connectable=True
    8 Short Local Name = LapisDev
uuid = 0179bbd1-5351-48b5-bf6d-2167639bc867 handle = 0x404
Waiting...
0100a35631825960414fa5
    ID            = 0x1
    SEQ           = 65
    GPS Latitude  = 34.69108 °
          (32bit) = 34.6910873 °
    GPS Longitude = 135.4917 °
          (32bit) = 135.49171108 °
    RSSI          = -81 dB
'''
