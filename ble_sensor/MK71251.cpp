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

	Serial2.begin(57600);

	printf("MK71251-02: waiting for CTS:LOW...\n");

	while (1) {
		int cts = digitalRead(PIN_D27);
		if (!cts) break;
	}

	printf("MK71251-02: CTS:LOW read succesfully\n");

	// define advertising data
	// 020106 -> Flags: LE General Discoverable Mode, BR/EDR Not Supported
	// 05030f18180a -> Complete List of 16bit Service UUIDs: 180f, 180a
	// 09084c61706973446576 -> Complete Local Name: LapisDev
	Serial2.write("ATS150=02010605030f180a1809084c61706973446576\r");

	// start advertising
	Serial2.write("ATD\r");

	printf("MK71251-02: started advertising\n");
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

byte MK71251::write(unsigned char *data){
	int rc;
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

int MK71251::start(){
	Serial2.write("ATD\r");
	printf("MK71251-02: started advertising\n");
	return 1;
}

int MK71251::waitConnect(){
	return waitKey("CONNECT");
}

int MK71251::waitKey(char *key){
	char c = 0;
	char ret[128];
	int n;

	for (n = 0; n < 128 && c != '\n';)
	{
		c = Serial2.read();
		if (c != 0 && c != 255) {
			ret[n] = c;
			n++;
		}
	}
	ret[n - 2] = '\0';
	int cmp = strcmp(ret, *key);
	return cmp == 0 ? 1 : 0;
}

int MK71251::disconnect(){
	int ret = waitConnect();
	if(ret){
		printf("MK71251-02: disconnect\n");
		write("+++AT\r");
		ret = waitKey("OK");
		if( ret !=0 ){
			printf("MK71251-02: ERROR disconnect line\n");
			delay(100);
			return 1;
		}else{
			printf("MK71251-02: ATH\n");
			write("ATH\r");
			ret = waitKey("OK");
	 		if( ret != 0 ){
		 		printf("MK71251-02: ERROR idle mode\n");
				delay(100);
				return 1;
			}
		}
	}else{
		write("\r");
		ret = waitKey("OK");
		if( ret !=0 ){
			printf("MK71251-02: ERROR disconnect line\n");
			delay(100);
			return 1;
		}
	}
	printf("MK71251-02: idle mode\n");
	return ret;		// 0 = disconnect
}

void MK71251::writeByte(unsigned char in){
	for(int i=1; i >= 0; i--){
		byte v = ((byte)in >> (4 * i)) & 0x0F;
		if(v < 10) v += (byte)'0';
		else v += (byte)'a' - 10;
		Serial2.write(v);
		printf("%c",v);
	}
}

int MK71251::scanResponse(unsigned char *data, int n){
	int ret = disconnect();
	if( ret != 0 ){
		printf("MK71251-02: ERROR disconnect line\n");
		return 0;
	}
	printf("MK71251-02: Scan Response\n");
	Serial2.write("ATS152=09FF0100");	// 09 = AD Type
										// FF = Manufacture Specific
										// Company Identifier(2 Octet)
	printf("ATS152=09FF0100");
	for(int i=0; i<n;i++){
		writeByte((byte)data[i]);
	}
	Serial2.write('\r');
	printf("\n");
	
	start();
	return 1;
}
