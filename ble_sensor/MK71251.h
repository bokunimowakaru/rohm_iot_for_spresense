// 下記のライセンスに基づいて改変しました。元の権利はRohmに帰属、改変部の権利は国野亘に帰属。

/*****************************************************************************
  MK71251.h
  
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

#ifndef _MK71251_H_
#define _MK71251_H_

class MK71251
{
	public:
		MK71251(void);
		byte payload[34];
		int payload_n;
		int init(boolean lowpower=false);
		int status(const char *s);
		int write(unsigned char *data,int n);
		int read(char *s, int n);
		int start(void);
		int waitConnect(void);
		int disconnect(int mode=0);
		int sendScanResponse(unsigned char *data, int n);
		int sendAt(const char *data);
	private:
		int at_status;
		void writeByte(unsigned char in);
		int waitKey(const char *key, int max=127);
		int waitCTS(void);
		int sendVSSPP(void);
};

#endif // _MK71251_H_
