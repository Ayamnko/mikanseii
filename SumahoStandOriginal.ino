#include <Wire.h>

#include <Adafruit_PWMServoDriver.h>

#include <WiFi.h>

#include <PubSubClient.h>

#include <ArduinoJson.h>

#include <NewPing.h>



// — Wi‑Fi / MQTT —

const char* ssid = "anpanman";

const char* password = "baikinman";

const char* mqtt_server = "mqtt.beebotte.com";

const char* mqtt_topic = "RobotControl/move"; // HTML→ESP32 (アーム角度)

const char* mqtt_send = "RobotControl/robotState"; // ESP32→HTML (センサー状態)

const char* mqtt_token = "token_EUPsOMBWypIHwQ94";



WiFiClient espClient;

PubSubClient client(espClient);

StaticJsonDocument<128> doc;



// — センサー設定 —

#define PIN_SONAR_TRIGGER 13

#define PIN_SONAR_ECHO 12

#define MAX_DISTANCE 200

NewPing sonar(PIN_SONAR_TRIGGER, PIN_SONAR_ECHO, MAX_DISTANCE);

int distBuf[3]={0,0,0}, idx=0;

bool detected=false; // 障害物検知状態



// --- HTMLからのアーム角度制御用変数 ---

// HTMLから最後に受信したアームの角度 (0-180) - ここでは新しい基準の角度

volatile int htmlArmAngle = 0;

// HTMLからのアーム角度コマンドが最後にいつ受信されたか

unsigned long lastHtmlArmCommandTime = 0;

// HTMLからのアームコマンドを優先するタイムアウト時間（ms）

const unsigned long HTML_COMMAND_TIMEOUT = 5000; // 例: 5秒間優先



// — サーボ設定 & バイアス —

#define DUTY_MIN 0.022

#define DUTY_MAX 0.130

Adafruit_PWMServoDriver pwm;

double bias[16]={-6,0,0,0, 0,0,0,0, 0,0,3,0, 0,0,0,0};



#define L1 1 //左腰サーボ(L1)を繋げたチャンネル

#define L2 0 //左足サーボ(L2)を繋げたチャンネル

#define R1 15 //右腰サーボ(R1)を繋げたチャンネル

#define R2 14 //右足サーボ(R2)を繋げたチャンネル



#define LH 3 //左手サーボ(LH)を繋げたチャンネル

#define HD 8 //頭サーボ(HD)を繋げたチャンネル

#define RH 12 //右手サーボ(RH)を繋げたチャンネル



// チャンネル[ch]に接続されているサーボの回転角度を[t]に設定する関数

// [t]は -90度(最左/下) から 90度(最右/上) の範囲で指定されることを想定

void setArm(int ch, int t){

int a=constrain(t,-90,90)+bias[ch]; // ここで -90〜90 の範囲に制限し、バイアスを加算

a=constrain(a,-90,90); // 再度 -90〜90 の範囲に制限

int p=a+90; // ここで 0〜180 の範囲に変換（サーボのパルス幅計算用）

double duty=(DUTY_MIN+(p/180.0)*(DUTY_MAX-DUTY_MIN))*4096;

pwm.setPWM(ch,0,(int)duty);

}



void sendData(int d){

char buf[64];

sprintf(buf,"{\"data\":%d, \"ispublic\":true}",d);

client.publish(mqtt_send,buf);

Serial.println(buf);

}



// MQTTメッセージ受信時に呼び出されるコールバック関数

void callback(char* tpc, byte* pl, unsigned int len){

deserializeJson(doc,pl,len);

// HTMLから送られてくる角度(0-180)を直接格納。これはあなたの新しい基準の角度

htmlArmAngle = doc["data"];

Serial.printf("CB received angle (new standard)=%d\n", htmlArmAngle);

lastHtmlArmCommandTime = millis(); // 受信時刻を更新

}



void reconnect(){

while(!client.connected()){

Serial.print("MQTT...");

if(client.connect("ESP32",mqtt_token,"")){

client.subscribe(mqtt_topic);

Serial.println("OK");

} else {

Serial.printf("fail rc=%d\n", client.state());

delay(3000);

}

}

}



void setup(){

Serial.begin(115200);

pwm = Adafruit_PWMServoDriver();

pwm.begin();

pwm.setPWMFreq(50);



// 起動時：アームは0度（真下）、それ以外も0度

// setArmは -90〜90 を期待するので、真下0度 → -90度 にマッピング

setArm(LH, -90); // LHを真下

setArm(RH, -90); // RHを真下

// 他のモーターも必要に応じてここで0度に設定

setArm(L1,0); setArm(L2,0);

setArm(R1,0); setArm(R2,0);

setArm(HD,0);



WiFi.begin(ssid,password);

while(WiFi.status()!=WL_CONNECTED){delay(300); Serial.print(". ");}

Serial.println("\nWiFi OK");



client.setServer(mqtt_server,1883);

client.setCallback(callback);

reconnect();



// 初期状態として未検知を通知

sendData(0);

}



void loop(){

if(!client.connected()) reconnect();

client.loop();



// --- センサー状態の更新 ---

distBuf[idx]=sonar.convert_cm(sonar.ping());

idx=(idx+1)%3;

bool prevDetected=detected; // 以前の検知状態を保存



// 障害物検知の判定ロジック

if(distBuf[0] && distBuf[1] && distBuf[2] &&

distBuf[0]<10 && distBuf[1]<10 && distBuf[2]<10){

detected=true;

} else if((distBuf[0]==0 || distBuf[0]>=50) &&

(distBuf[1]==0 || distBuf[1]>=50) &&

(distBuf[2]==0 || distBuf[2]>=50)){

detected=false;

}



// --- センサー状態変化時の処理 ---

if(detected != prevDetected){

sendData(detected ? 1 : 0); // HTMLに状態を通知



// 状態変化時にアームの初期位置を設定 (新しい基準に合わせて調整)

if (detected) {

// 障害物ありになったらデフォルト45度（新しい基準） → setArmでは -45度

setArm(LH, -45);

setArm(RH, -45);

} else {

// 障害物なしになったらアームを真横0度（新しい基準） → setArmでは -90度

setArm(LH, -90);

setArm(RH, -90);

}

// ここで他のモーターを0度にリセットしても良い

setArm(HD, 0);

}



// --- アームの動作ロジック ---

if (detected) {

// 障害物がある場合: HTMLからのコマンドを優先

if (millis() - lastHtmlArmCommandTime < HTML_COMMAND_TIMEOUT) {

// タイムアウト時間内であれば、HTMLからの角度（新しい基準）を setArmの基準に変換

// 新しい基準の0-180を、setArmの-90-90に変換

int armAngleToSet = map(htmlArmAngle, 0, 180, -90, 90);

setArm(LH, armAngleToSet);

setArm(RH, armAngleToSet);

} else {

// タイムアウトした場合は、デフォルトの45度（新しい基準）に戻す -> setArmでは-45度

setArm(LH, -45);

setArm(RH, -45);

}

} else {

// 障害物がない場合: アームを常に真横（0度、新しい基準）に固定 -> setArmでは-90度

setArm(LH, -90);

setArm(RH, -90);

}



// その他のモーター（頭など）は、HTMLからの制御がない限り常に0度（標準サーボ基準）

setArm(HD, 0);

// 必要であれば他のモーターも



delay(50); // メインループの更新間隔

}