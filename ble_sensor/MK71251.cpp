// 下記のライセンスに基づいて改変しました。元の権利はRohmに帰属、改変部の権利は国野亘に帰属。

/*****************************************************************************
  MK71251.cpp

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
******************************************************************************/

#include "Arduino.h"
#include "MK71251.h"

MK71251::MK71251(void){
	at_status = 0;
	// 0: 初期状態・状態不明
	// 1: Idle Command Mode
	// 2: Connecting Mode
	// 3: On-line Mode
	// 4: On-line Command Mode
	payload[0] = 0x00;
	payload_n=0;
}

int MK71251::status(const char *s){
	int cts = !digitalRead(PIN_D27);
	printf("MK71251(%d,%d): %s", at_status, cts, s);
	if( strlen(s) == 0 ) printf("\n");
	return at_status;
}

int MK71251::init(boolean lp_mode){

	// configure Output for GPIO3 (High: HCI mode, Low: AT command mode)
	digitalWrite(PIN_D21, LOW);
	pinMode(PIN_D21, OUTPUT);
	digitalWrite(PIN_D21, LOW);
	
	// reset BLE chip
	digitalWrite(PIN_D20, HIGH);
	pinMode(PIN_D20, OUTPUT);
	digitalWrite(PIN_D20, HIGH);
	if(!lp_mode){
		delay(500);
		digitalWrite(PIN_D20, LOW);
		delay(10);
		digitalWrite(PIN_D20, HIGH);
		delay(20);
	}

	Serial2.begin(57600);

	while(!waitCTS()){
		status("ERROR CTS is High (Locked)\n");
	}

	if(!lp_mode){
		while(!sendAt("AT&F")) delay(10000);
		at_status = 1;
	}else{
		Serial2.write("\r");
		if(waitKey("NO CARRIER")) at_status = 1;
		else status("ERROR no response\n");
	}
	
	// define advertising data
	// 020106 -> Flags: LE General Discoverable Mode, BR/EDR Not Supported
	// 05030f18180a -> Complete List of 16bit Service UUIDs: 180f, 180a
	// 09084c61706973446576 -> Complete Local Name: LapisDev
	sendAt("ATS150=02010605030f180a1809084c61706973446576");
	
	if(lp_mode){
		sendAt("ATS104=500");
		sendAt("ATS105=100");
		sendAt("ATS107=512");
		sendAt("ATS112=1000");
	}
	// start advertising
	start();
	
	return 0;
}

int MK71251::waitCTS(){
	int cts=1;
	int max = 50;
	int i;
	if( !digitalRead(PIN_D27) ) return 1;
	status("Waiting for CTS:LOW.");
	for(i=0;i<max;i++) {
		cts = !digitalRead(PIN_D27);
		if (cts) break;
		delay(100);
		if(!(i%10)) printf(".");
	}
	if(i == max){
		printf("\n");
		status("WARN CTS = HIGH\n");
		if( at_status == 2 ) at_status = 3;
	}
	else{
		printf("\n");
		status("CTS:LOW read succesfully\n");
	}
	return cts;
}

int MK71251::write(unsigned char *data, int n){
	int rc;
	for(int i=0; i<n;i++){
		rc = Serial2.write( data[i] );
		if (rc == 0) return -i;
	}
	return n;
}

int MK71251::read(char *s, int n){
	char c;
	if(at_status==3){
		memset(s,0,n);
		for(int i=0; i < n-1;i++){
			c = Serial2.read();
			if(c == 255 || c < 16) break;
			s[i]=c;
		}
		return strlen(s);
	}
	return 0;
}

int MK71251::waitKey(const char *key, int max){
	char c = 0;
	char ret[128];
	int n=0,i,cmp=-1;
	if(max>127) max=127;
	if(max<1) max=1;
	boolean send_app_data = false;

	printf(">");
	for (i = 0; i < max; i++){
		c = Serial2.read();
		if (c != 0 && c != 255) {
			ret[n] = c;
			n++;
			if( isPrintable(c) ) printf("%c",c);
			if( c == '\n' ){
				printf(" ");
				if(n >= 2) ret[n - 2] = '\0';
				if( !strcmp(ret, "CONNECT") ){
					if( at_status == 2 ) at_status = 3;
					send_app_data = true;
				}
				if( !strcmp(ret, "ERROR") ){
					at_status = 1;
					send_app_data = false;
				}
				if( !strcmp(ret, "NO CARRIER") ){
					at_status = 1;
					send_app_data = false;
				}
				cmp = strcmp(ret, key);
				if(!cmp) break;
				n=0;
				i=0;
				cmp = -1;		// i=maxで終了するときに前回cmp値の漏れ防止
			}
		}else delay(1);
	}
	printf("\n");
	if( strcmp(key,"CONNECT") && cmp ){
		status("ERROR waitKey, ");
		printf("%s, %d\n",key,cmp);
	}
	if( send_app_data ){
		sendVSSPP();	// CTSの状態に関わらず送信する
	}
	return !cmp;	// 1:成功、0:エラー
}

int MK71251::waitConnect(){
	int ret = waitKey("CONNECT");
	if( at_status == 2 && ret ) at_status = 3;
	return waitKey("CONNECT");
}

int MK71251::sendAt(const char *data){
	int ret = 0;
	status("sendAt ");
	printf("%s\n",data);
	if(waitCTS()>0){
		Serial2.write(data);
		Serial2.write("\r");
		ret = waitKey("OK");
	}
	return ret;
}

int MK71251::sendVSSPP(){
	if( at_status != 3 ) return 0;
	status("send VSSPP app data=");
	for(int j=0; j < payload_n;j++){
		for(int i=1; i >= 0; i--){
			byte v = ((byte)payload[j] >> (4 * i)) & 0x0F;
			if(v < 10) v += (byte)'0';
			else v += (byte)'a' - 10;
			printf("%c",v);
		}
	}
	printf("\n");
	int n = write(payload,payload_n);
	if( n < 0){
		status("ERROR cannot send, ");
		printf("n = %d\n", n);
	}
	delay(100);	// データ区切り
	return 1;
}

int MK71251::start(){
	int ret=0;
	status("start\n");
	if( at_status <= 1 || at_status == 4){
		if(waitCTS()>0){
			Serial2.write("ATD\r");
			at_status = 2;
			/*
			ret = waitKey("OK");
			if(ret){
				status("started advertising\n");
				at_status = 2;
			}
			else status("ERROR start\n");
			*/
		}
	}else status("ERROR status\n");
	return ret;	// 1:成功、0:エラー
}

int MK71251::disconnect(int mode){
	status("disconnect mode=");
	if(mode==0) printf("Off-line(0)\n");
	else		printf("On-line(1)\n");
	
	int ret=1;
	int cts = !digitalRead(PIN_D27);
	if( at_status <= 2 && cts ){
		Serial2.write("\r");
		ret = waitKey("NO CARRIER");
		if(ret) at_status = 1;
		else status("ERROR no response\n");
		status("Idle Command Mode\n");
	}
	if((at_status == 2 || at_status == 3) && !cts){
		status("Waiting for CTS\n");
		at_status = 3;		// Centralからの接続中と判断[要検証]
		return 0;
	}
	if(at_status == 3 && cts){
		ret = sendAt("+++AT");
		if( !ret ){
			status("ERROR cannot send +++AT\n");
			delay(100);
			return 0;
		}
		at_status = 4;
		status("On-line Command Mode\n");
		delay(100);
	}
	if(mode == 0 && at_status == 4 && cts){
		ret = sendAt("ATH");
 		if( !ret ){
	 		status("ERROR cannot send ATH\n");
			delay(100);
			return 0;
		}
		at_status = 1;
		status("Idle Command Mode\n");
	}
	return 1;	// 1:成功、0:エラー
}

void MK71251::writeByte(unsigned char in){
	for(int i=1; i >= 0; i--){
		byte v = ((byte)in >> (4 * i)) & 0x0F;
		if(v < 10) v += (byte)'0';
		else v += (byte)'a' - 10;
		waitCTS();
		Serial2.write(v);
		printf("%c",v);
	}
}

int MK71251::sendScanResponse(unsigned char *data, int n){
	status("sendScanResponse\n");
	int ret;
	char companyIdentifier[]="0100";	// ID = 0001
	byte companyIdentifier_bin[]={0x01,0x00};
	
	if( n > 31 ) n = 31;
	memcpy(payload,companyIdentifier_bin,2);
	memcpy(payload + 2,data,n);
	payload_n = 2 + n;
	
	if(waitKey("CONNECT",14)){
		at_status = 3;
		return 0;
	}
	// 7バイト文字 ＋ \r\n 2バイト×2
	// NO CARRIERが 10バイトなので +3バイトして 14に
	
	if(at_status == 3){
		status("Lapis VSSPP connected\n");
		if( waitCTS() > 0 ) sendVSSPP();
		return 0;
	}
	
	ret = disconnect(1);
	if( !ret ){
		status("ERROR disconnect line\n");
		return 0;
	}

	int i = n * 2 + 15;
	if(i > 76){
		status("WARN ");
		printf("data length = %d, %d\n",n,i);
	}
	waitCTS();
//	sendAt("ATS129=23");
	status("send ATS152=09FF");
	printf("%s",companyIdentifier);
	Serial2.write("ATS152=09FF");	// 09 = AD Type
									// FF = Manufacture Specific
	Serial2.write(companyIdentifier);
									// Company Identifier(2 Octet)
	
	n=6;	// 制約
	
	for(int i=0; i<n;i++){
		writeByte((byte)data[i]);
	}
	Serial2.write('\r');
	printf("\n");
	ret = waitKey("OK");
//	sendAt("ATS153?");
//	sendAt("ATS152?");
	start();
	status("done\n");
	return 1;
}
