/*******************************************************************************
BLE Sensor
for Rohm SPRESENSE-BLE-EVK-701 + Sony Spresense

https://github.com/bokunimowakaru/rohm_iot_for_spresense

                                           Copyright (c) 2019-2021 Wataru KUNINO
********************************************************************************
【内容】
  ローム製Bluetooth LE Add-onボードSPRESENSE-BLE-EVK-701を
  ソニーセミコンダクタソリューションズ製 Spresenseへ接続し、
  GPS値をBLE送信するためのプログラムです。
  
【制約】
  - Scan Response 送信の容量が10バイト（データ6バイト）しか送信できない。
  
【ライセンス】
  本プログラムやレポジトリに下記からダウンロードしたソースリストが含まれます。
  https://developer.sony.com/ja/develop/spresense
  https://github.com/RohmSemiconductor/Arduino
  
  元の権利は Sony Semiconductor Solutions Corporationと Rohm, KokiOkada に帰属し
  改変部の権利は国野亘に帰属します。詳細は各ソースコードをご覧ください。

  gnss.ino - GNSS example application
  Copyright 2018 Sony Semiconductor Solutions Corporation

  MK71251.cpp, MK71251.h
  Copyright (c) 2018 ROHM Co.,Ltd.
*/

#include "Arduino.h"
#include "MK71251.h"
#include <string.h>
#include <GNSS.h>
#include <RTC.h>

MK71251 mk71251;

byte seq=0;

void setup(){
    byte rc;

    Serial.begin(115200);
    while(!Serial);
    
    gnssSetup();

    seq = (byte)RTC.getTime();
    Serial.print("!---------- Wake up (sec=");
    Serial.print((int)seq);
    Serial.println(")----------!");
    
    rc = mk71251.init();
    Serial.print("BLE Module MK71251 initialization ");
    if(rc != 0){ Serial.println("FAILED"); Serial.flush();}
    else Serial.println("success");
}

void Serial_printHex24(byte *in){
    Serial.print("(0x");
    for(int i=0;i<3;i++){
        if(in[i] < 0x10) Serial.print("0");
        Serial.print(in[i],HEX);
    }
    Serial.print(')');
}

void loop(){
    byte gps[9];
        
    /* GPSからデータ取得 */
    gnssGet(gps);
    gps[6] = seq;
    
    /* データ送信 */
    mk71251.sendScanResponse(gps, 9);
    
    /* 送信データの表示 */
    int32_t q;
    q = gps[0] + (gps[1]<<8) + (gps[2]<<16);
    if(q > 0x7fffff) q |= 0xff0000;
    double lat = (float)q / 8388607. * 90.;
    
    q = gps[3] + (gps[4]<<8) + (gps[5]<<16);
    if(q > 0x7fffff) q |= 0xff0000;
    double lon = (float)q / 8388607. * 180.;
    
    /*
    Serial.printf("lat=%f(0x%02X%02X%02X), lon=%f(0x%02X%02X%02X)\n",
    	lat, gps[2], gps[1], gps[0],
    	lon, gps[5], gps[4], gps[3],
    );
    */
    Serial.print("lat=");
    Serial.print(lat,6);
    Serial_printHex24(gps);
    
    Serial.print(", lon=");
    Serial.print(lon,6);
    Serial_printHex24(gps + 3);
    Serial.println();
    
    Serial.print("#---------- Done (seq=");
    Serial.print((int)seq);
    Serial.print(", mode=");
    Serial.print("delay");
    Serial.println(")----------#");
    delay(1000);
    seq++;
}
