# 【内容】  
ローム製Bluetooth LE Add-onボードSPRESENSE-BLE-EVK-701と、センサAdd-onボード
SPRESENSE-SENSOR-EVK-701 を ソニーセミコンダクタソリューションズ製 Spresenseへ
接続し、各センサ値をBLE送信するためのプログラムです。

	ble_sensor:
	フォルダごと、パソコンにダウンロードし、Arduino IDEでble_sensorフォルダ内のble_sensor.inoを開き、Spresenseへ書き込んでください。
	
	ble_logger.py:
	Raspberry PiなどでBLEスキャンを行うツール
	
	ble_logger_sens_gatt.py:
	Raspberry Piなどでble_sensorからGATTでデータ取得を行うツール

# 【送信データ】  
センサから得られた、温度、気圧、加速度、地磁気などのデータは、疑似シリアル通信（VSSPP）で送信します。  
ビーコンのScan Responseでも送信しますが、10バイト（データ6バイト）の容量に抑えており、温度と気圧しか送信できません。  
  
# 【ライセンス】
本プログラムやレポジトリに下記からダウンロードしたソースリストが含まれます。  
https://github.com/RohmSemiconductor/Arduino
  
元の権利は Rohm と KokiOkada に帰属し、改変部の権利は国野亘に帰属します。  
配布時はファイル「LISENSE」を添付ください。
https://github.com/bokunimowakaru/rohm_iot_for_spresense/blob/master/ble_sensor/LICENSE
