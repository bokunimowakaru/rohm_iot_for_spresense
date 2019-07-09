/*******************************************************************************
BLE Sensor
for Rohm SPRESENSE-BLE-EVK-701 + SPRESENSE-SENSOR-EVK-701 + Sony Spresense

https://github.com/bokunimowakaru/rohm_iot_for_spresense

                                                Copyright (c) 2019 Wataru KUNINO
********************************************************************************
【内容】
  ローム製Bluetooth LE Add-onボードSPRESENSE-BLE-EVK-701と、センサAdd-onボード
  SPRESENSE-SENSOR-EVK-701 を ソニーセミコンダクタソリューションズ製 Spresenseへ
  接続し、各センサ値をBLE送信するためのプログラムです。
  
【不具合】
  - VSSPP受信動作は製作途中
  - Scan Response 送信の容量が10バイト（データ6バイト）しか送信できない。
  
【参考文献】
  
  本プログラムやレポジトリに下記からダウンロードしたソースリストが含まれます。
  https://github.com/RohmSemiconductor/Arduino
  
  元の権利は Rohm と KokiOkada に帰属し、改変部の権利は国野亘に帰属します。
  
  下記は Rohm ならびに KokiOkada による元の権利表示です。
*/
/*******************************************************************************
https://github.com/RohmSemiconductor/Arduino/blob/master/LICENSE
KokiOkada-Rohm
********************************************************************************
  MIT License

Copyright (c) 2018 KokiOkada

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
/*****************************************************************************
https://github.com/RohmSemiconductor/Arduino/blob/master/Sensors-Add-on-Demo
ROHM Co.,Ltd.
******************************************************************************
  Sensors-Add-on-Demo.ino
 Copyright (c) 2018 ROHM Co.,Ltd.
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
*******************************************************************************/

#include "Arduino.h"
#include <Wire.h>
#include "KX122.h"
#include "KX126.h"
#include "BM1422AGMV.h"
#include "BM1383AGLV.h"
#include "MK71251.h"
#include <string.h>

KX122 kx122(KX122_DEVICE_ADDRESS_1F);
KX126 kx126(KX126_DEVICE_ADDRESS_1F);
BM1422AGMV bm1422agmv(BM1422AGMV_DEVICE_ADDRESS_0F);
BM1383AGLV bm1383aglv;
MK71251 mk71251;

bool KX122_found = false;
bool KX126_found = false;

void setup(){
    byte rc;

    Serial.begin(115200);
    while(!Serial);

    Wire.begin();

    rc = kx122.init();
    if(rc != 0) Serial.flush();
    else KX122_found = true;

    rc = kx126.init();
    if(rc != 0) Serial.flush();
    else KX126_found = true;

    Serial.print("Accelerometer KX122/KX126 initialization ");
    if(!KX122_found && !KX126_found) Serial.println("FAILED");
    else Serial.println("success");
    
    rc = bm1422agmv.init();
    Serial.print("Magneto-sensor BM1422AGMV initialization ");
    if(rc != 0){ Serial.println("FAILED"); Serial.flush();}
    else Serial.println("success");

    rc = bm1383aglv.init();
    Serial.print("Pressure sensor BM1383AGLV initialization ");
    if(rc != 0){ Serial.println("FAILED"); Serial.flush();}
    else Serial.println("success");

    rc = mk71251.init();
    Serial.print("BLE Module MK71251 initialization ");
    if(rc != 0){ Serial.println("FAILED"); Serial.flush();}
    else Serial.println("success");
}

byte seq=0;

int d_append(byte *array,int i, byte d){
    array[i]=d;
    return i+1;
}

int d_append_uint16(byte *array,int i, uint16_t d){
    array[i] = (byte)(d & 0xFF); 
    array[i+1] = (byte)(d >> 8);
    return i+2;
}

int d_append_uint24(byte *array,int i, uint32_t d){
    array[i] = (byte)(d & 0xFF); 
    array[i+1] = (byte)((d >>8)&0xFF);
    array[i+2] = (byte)((d >>16)&0xFF);
    return i+3;
}

void sensors_log(float temp, float press, float *acc, float *mag, byte *rc){
    if(!rc[0]){
        Serial.print("Temperature    =  ");
        Serial.print(temp);
        Serial.println(" [degrees Celsius]");
    }
    if(!rc[2]){
        Serial.print("Pressure        = ");
        Serial.print(press);
        Serial.println(" [hPa]");
    }
    if(!rc[3]){
        Serial.write("Accelerometer X = ");
        Serial.print(acc[0]);
        Serial.println(" [g]");
        Serial.write("Accelerometer Y = ");
        Serial.print(acc[1]);
        Serial.println(" [g]");
        Serial.write("Accelerometer Z = ");
        Serial.print(acc[2]);
        Serial.println(" [g]");
    }
    if(!rc[4]){
        Serial.print("Geomagnetic X   = ");
        Serial.print(mag[0], 3);
        Serial.println("[uT]");
        Serial.print("Geomagnetic Y   = ");
        Serial.print(mag[1], 3);
        Serial.println("[uT]");
        Serial.print("Geomagnetic Z   = ");
        Serial.print(mag[2], 3);
        Serial.println("[uT]");
    }
}

int sensors_data(
    unsigned char *data,
    float temp, float press, float *acc, float *mag, byte *rc
){
    int len =0;
    unsigned long val_ui;
    if(!rc[0]){
	    val_ui = (unsigned long)((temp + 45.) * 374.5);
	    len = d_append_uint16(data,len,(uint16_t)(val_ui));
    }else len = d_append_uint16(data,len,0);
    if(!rc[2]){
	    val_ui = (unsigned long)(press * 2048);
	    len = d_append_uint24(data,len,(uint32_t)(val_ui));
    }else len = d_append_uint24(data,len,0);
    
    len = d_append(data,len,seq);
    if(!rc[3]){
	    val_ui = (unsigned long)(acc[0] * 4096);
	    len = d_append_uint16(data,len,(uint16_t)(val_ui));
	    val_ui = (unsigned long)(acc[1] * 4096);
	    len = d_append_uint16(data,len,(uint16_t)(val_ui));
	    val_ui = (unsigned long)(acc[2] * 4096);
	    len = d_append_uint16(data,len,(uint16_t)(val_ui));
    }else{
		len = d_append_uint24(data,len,0);
		len = d_append_uint24(data,len,0);
	}
    if(!rc[4]){
	    val_ui = (unsigned long)(mag[0] * 10);
	    len = d_append_uint16(data,len,(uint16_t)(val_ui));
	    val_ui = (unsigned long)(mag[1] * 10);
	    len = d_append_uint16(data,len,(uint16_t)(val_ui));
	    val_ui = (unsigned long)(mag[2] * 10);
	    len = d_append_uint16(data,len,(uint16_t)(val_ui));
    }else{
		len = d_append_uint24(data,len,0);
		len = d_append_uint24(data,len,0);
	}
	return len;
}

void loop(){
    byte rc[5];
    float acc[3], mag[3], press = 0, temp = 0;
        
    /* センサ値の取得 */
    rc[0] = bm1383aglv.get_val(&press, &temp);
    rc[1] = 255;
    rc[2] = rc[0];
    if(KX122_found){
        rc[3]= kx122.get_val(acc);
    }else if(KX126_found){
        rc[3] = kx126.get_val(acc);
    }
    rc[4] = bm1422agmv.get_val(mag);
    
    /* データ送信 */
    unsigned char data[32];
    int len = sensors_data(data, temp, press, acc, mag, rc);
    mk71251.sendScanResponse(data,len);
    
    if(!(seq%8)) sensors_log(temp, press, acc, mag, rc);
    
    /* 受信 */
    char s[32];
    if( mk71251.read(s,32) > 0 ){
        Serial.print("Read ");
        Serial.println(s);
    }
    
    /* 次回の送信待ち（待機） */
    delay(5000);
    Serial.println("\n!---------- Wake up ----------!");
    seq++;
    /* 待機完了 */
}
