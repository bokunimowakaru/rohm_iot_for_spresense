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
}

byte MK71251::init(void){

	// configure Output for GPIO3 (High: HCI mode, Low: AT command mode)
	pinMode(PIN_D21, OUTPUT);
	digitalWrite(PIN_D21, LOW);

	// reset BLE chip
	delay(500);
	pinMode(PIN_D20, OUTPUT);
	digitalWrite(PIN_D20, LOW);
	delay(10);
	digitalWrite(PIN_D20, HIGH);
	delay(20);
	at_status = 1;

	Serial2.begin(57600);

	while(!waitCTS()){
		printf("MK71251-02: ERROR CTS is High (Locked)\n");
	}

	// define advertising data
	// 020106 -> Flags: LE General Discoverable Mode, BR/EDR Not Supported
	// 05030f18180a -> Complete List of 16bit Service UUIDs: 180f, 180a
	// 09084c61706973446576 -> Complete Local Name: LapisDev
	while(!sendAt("ATS150=02010605030f180a1809084c61706973446576")){
		delay(5000);
		sendAt("AT&F");
		delay(1000);
	}
	
	// start advertising
	start();
	
	/*
	printf("MK71251-02: waiting for connection...\n");

	while (1) {
		int ret = waitConnect();
		if (ret) break;
	}

	printf("MK71251-02: connected succesfully\n");
	*/
	return (0);
}

int MK71251::waitCTS(){
	int cts=1;
	if( !digitalRead(PIN_D27) ) return 1;
	printf("MK71251-02: waiting for CTS:LOW");
	for(int i=0;i<100;i++) {
		cts = digitalRead(PIN_D27);
		if (!cts) break;
		delay(100);
		if(!i%10) printf(".");
	}
	printf("\nMK71251-02: CTS:LOW read succesfully\n");
	return !cts;
}

byte MK71251::write(const char *data){
	int rc;
	waitCTS();
	rc = Serial2.write(*data);
	if (rc != 0) {
		return 0;
	}else {
		return -1;
	}
}

byte MK71251::read(unsigned char *data){
	char c;
	if ((c = Serial2.read()) && c != 255) {
		*data = c;
		return (0);
	}else {
		return (-1);
	}
}

int MK71251::waitKey(const char *key){
	char c = 0;
	char ret[128];
	int n=0,i,cmp=-1;

	printf("> ");
	for (i = 0; i < 1000; i++){
		c = Serial2.read();
		if (c != 0 && c != 255) {
			ret[n] = c;
			n++;
			if( isPrintable(c) ) printf("%c",c);
			if( c == '\n' ){
			//	printf("/");
				if(n >= 2) ret[n - 2] = '\0';
				cmp = strncmp(ret, key, strlen(key));
				if(!cmp) break;
				n=0;
				i=0;
			}
		}else delay(1);
	}
	printf("\n");
	if( strcmp(key,"CONNECT") && cmp ) printf("MK71251-02: ERROR waitKey, %s, %d\n",key,cmp);
	return !cmp;	// 1:成功、0:エラー
}

int MK71251::waitConnect(){
	int ret = waitKey("CONNECT");
	if( at_status == 2 && ret ) at_status = 3;
	return waitKey("CONNECT");
}

int MK71251::sendAt(const char *data){
	int ret;
	printf("MK71251-02: sendAt %s\n",data);
	waitCTS();
	Serial2.write(data);
	Serial2.write("\r");
	ret = waitKey("OK");
	return ret;
}

int MK71251::start(){
	int ret=0;
	printf("MK71251-02: start\n");
	if( at_status <= 1 ){
		Serial2.write("ATD\r");
		/*
		ret = waitKey("OK");
		if(ret){
			printf("MK71251-02: started advertising\n");
			at_status = 2;
		}
		else printf("MK71251-02: ERROR start\n");
		*/
		at_status = 1;
	}else printf("MK71251-02: ERROR status=%d\n",at_status);
	return ret;	// 1:成功、0:エラー
}

int MK71251::disconnect(){
	int ret=1;
	printf("MK71251-02: disconnect, status = %d\n",at_status);
	if( at_status <= 2 ){
		write("\r");
		ret = waitKey("NO CARRIER");
		if(ret) at_status = 1;
		else printf("MK71251-02: ERROR failed disconnecting\n");
	}
	if( at_status == 2 || at_status == 3 ){
		ret = sendAt("+++AT");
		if( !ret ){
			printf("MK71251-02: ERROR disconnect from On-line\n");
			delay(100);
			return 0;
		}else{
			at_status = 4;
			printf("MK71251-02: On-line Command Mode\n");
			delay(100);
		}
	}
	if( at_status == 4 ){
		ret = sendAt("ATH");
 		if( !ret ){
	 		printf("MK71251-02: ERROR idle mode\n");
			delay(100);
			return 0;
		}
		at_status = 1;
	}
	printf("MK71251-02: Idle Command Mode\n");
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
	printf("MK71251-02: sendScanResponse\n");
	int ret = disconnect();
	if( !ret ){
		printf("MK71251-02: ERROR disconnect line\n");
		return 0;
	}else printf("MK71251-02: disconnected\n");

	waitCTS();
	printf("ATS152=09FF0100");
	Serial2.write("ATS152=09FF0100");	// 09 = AD Type
										// FF = Manufacture Specific
										// Company Identifier(2 Octet)
	for(int i=0; i<n;i++){
		writeByte((byte)data[i]);
	}
	Serial2.write('\r');
	printf("\n");
	ret = waitKey("OK");
	start();
	return 1;
}
