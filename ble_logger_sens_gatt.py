#!/usr/bin/env python3
# coding: utf-8

################################################################################
# BLE Logger for ble_sensor
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
        sensors['Temperature'] = -45 + 175 * payval(val,4,2) / 65536
        sensors['Pressure'] = payval(val,6,3) / 2048
        sensors['SEQ'] = payval(val,9)
        sensors['Accelerometer X'] = payval(val,10,2,True) / 4096
        sensors['Accelerometer Y'] = payval(val,12,2,True) / 4096
        sensors['Accelerometer Z'] = payval(val,14,2,True) / 4096
        sensors['Accelerometer'] = (sensors['Accelerometer X'] ** 2\
                                  + sensors['Accelerometer Y'] ** 2\
                                  + sensors['Accelerometer Z'] ** 2) ** 0.5
        sensors['Geomagnetic X'] = payval(val,16,2,True) / 10
        sensors['Geomagnetic Y'] = payval(val,18,2,True) / 10
        sensors['Geomagnetic Z'] = payval(val,20,2,True) / 10
        sensors['Geomagnetic']  = (sensors['Geomagnetic X'] ** 2\
                                 + sensors['Geomagnetic Y'] ** 2\
                                 + sensors['Geomagnetic Z'] ** 2) ** 0.5
        sensors['RSSI'] = dev.rssi

        # 画面へ表示
        print('    ID            =',sensors['ID'])
        print('    SEQ           =',sensors['SEQ'])
        print('    Temperature   =',round(sensors['Temperature'],2),'℃')
        print('    Pressure      =',round(sensors['Pressure'],3),'hPa')
        print('    Accelerometer =',round(sensors['Accelerometer'],3),'g (',\
                                    round(sensors['Accelerometer X'],3),\
                                    round(sensors['Accelerometer Y'],3),\
                                    round(sensors['Accelerometer Z'],3),'g)')
        print('    Geomagnetic   =',round(sensors['Geomagnetic'],1),'uT (',\
                                    round(sensors['Geomagnetic X'],1),\
                                    round(sensors['Geomagnetic Y'],1),\
                                    round(sensors['Geomagnetic Z'],1),'uT)')
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
