/*****************************************************************
   WiFi TCP + AMG8833 + MR60FDA1 跌倒偵測 + 音樂播放（發出聲音）
******************************************************************/
//1209_2=1209_1送活動量+接收伺服器的數字
#include "TCP.h"
#include <Wire.h>
#include <BMS26M833.h>
#include "BMV31T001.h" 
#include "voice_cmd_list.h" // Contains a library of voice information
BMC81M001 Wifi(&Serial);   // WiFi 模組走 UART0
// === AMG START ===
BMS26M833 Amg;
float g_thermistor = 0.0f;
float g_pixels[64];
// === AMG END ===

// === BMV31T001 音頻播放 ===
BMV31T001 myBMV31T001; // Create an object for the audio module
#define VOICE_TOTAL_NUMBER 10 // Number of available voice files
const uint8_t voice_table[VOICE_TOTAL_NUMBER] = {
    VOC_1, VOC_2, VOC_3, VOC_4, VOC_5, VOC_6, VOC_7, VOC_8, VOC_9, VOC_10
};
#define DEFAULT_VOLUME 6 // Default volume level
uint8_t volume = DEFAULT_VOLUME; // Current volume
uint8_t playNum = 0; // Track number being played
// === END BMV31T001 音頻播放 ===

// === 雷達 UART4 設定 ===
#define HEADER1 0x53
#define HEADER2 0x59
#define TAIL1   0x54
#define TAIL2   0x43
#define MAX_PACKET_LEN 128

// 跌倒相關封包
// 查詢跌倒監控功能開關的命令
const byte queryCmd[] = { 0x53, 0x59, 0x83, 0x80, 0x00, 0x01, 0x0F, 0xBF, 0x54, 0x43 };
// 開啟跌倒模式的命令
const byte enableCmd[] = { 0x53, 0x59, 0x83, 0x00, 0x00, 0x01, 0x01, 0x31, 0x54, 0x43 };
// 設定靈敏度（最高3）的命令
const byte sensitivityCmd[] = { 0x53, 0x59, 0x83, 0x0D, 0x00, 0x01, 0x03, 0x40, 0x54, 0x43 };
// 設定跌倒判定時間的命令（例如 10秒）
const byte fallTimeCmd[] = { 0x53, 0x59, 0x83, 0x0C, 0x00, 0x04, 0x00, 0x00, 0x00, 0x05, 0x44, 0x54, 0x43 };
// 設定高度（安裝高度）的命令
//const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x01, 0x0E, 0xC5, 0x54, 0x43 };//2.7M
const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x00, 0xFA, 0xB0, 0x54, 0x43 };//2.5M
//const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x00, 0xE6, 0x9C, 0x54, 0x43 };//2.3M// 狀態
bool fallDetected = false;

// 暫存
void processIncomingByte(byte data);
bool isFallPacket(const byte *buf, int len);
bool isBodyMovementPacket(const byte *buf, int len);
void printPacket(const byte *buf, int len);
void sendAndReceive(const byte *cmd, int len, const char *desc);
int parseServerNumber(const String &msg);

// ========================================================
// ================ SETUP =================================
// ========================================================
void setup() {
  Serial.begin(115200);
  Serial4.begin(115200);
  Serial.println("UART4 Ready...");
  delay(2000);

  // ====== WiFi 開始連線 ======
  Serial.println("WiFi init...");
  Wifi.begin();

  if (!Wifi.connectToAP(WIFI_SSID, WIFI_PASS)) {
    Serial.println("WiFi FAIL");
  } else {
    Serial.println("WiFi OK");
  }

  if (!Wifi.connectTCP(IP, IP_Port)) {
    Serial.println("TCP FAIL");
  } else {
    Serial.println("TCP OK");
  }

  // ====== 雷達初始化 ======
  sendAndReceive(queryCmd, sizeof(queryCmd), "Query Fall");
  delay(500);
  sendAndReceive(enableCmd, sizeof(enableCmd), "Enable Fall");
  delay(500);
  sendAndReceive(sensitivityCmd, sizeof(sensitivityCmd), "Sensitivity");
  delay(500);
  sendAndReceive(fallTimeCmd, sizeof(fallTimeCmd), "Fall Time = 10s");
  delay(500);
  sendAndReceive(heightCmd, sizeof(heightCmd), "Height = 2.7m");

  // ====== AMG 開始 ======
  Amg.begin();
  Serial.println("AMG8833 Ready");

  // ====== BMV31T001 音頻播放初始化 ======
  myBMV31T001.begin(); // Initialize BMV31T001
  myBMV31T001.setPower(BMV31T001_POWER_ENABLE); // Power on the BMV31T001
  myBMV31T001.setVolume(DEFAULT_VOLUME); // Set initial volume
  myBMV31T001.initAudioUpdate(); // Initialize audio update if needed
  delay(100); // Delay for the audio source to power on
}



// ========================================================
// ================ LOOP ==================================
// ========================================================
void loop() {

  // ===== 接收伺服器傳來的指令 =====
  String recv = Wifi.readDataTcp();
  if (recv.length() > 0) {
   recv.trim();
    Serial.print("From Server: ");
    Serial.println(recv);
    int value = parseServerNumber(recv);
    if (value >= 0) {
      Serial.print("Parsed Number = ");
      Serial.println(value);
    // 在這裡加入伺服器控制邏輯
    // 例如控制 T16、Servo、Relay、播放音效等等
    }
  } 

  // ===== 讀雷達 UART4 =====
  while (Serial4.available() > 0) {
    byte data = Serial4.read();
    processIncomingByte(data);
  }

  // ===== 讀 AMG8833 =====
  Amg.readPixels(g_pixels);
  g_thermistor = Amg.readThermistorTemp();

  // ===== 溫度資料打包成 TCP 字串 =====
  char buffer[256];
  int len = snprintf(buffer, sizeof(buffer),
                    "THERM=%.2f", g_thermistor);

  Wifi.writeDataTcp(len, buffer);  // 發送到伺服器

  delay(500);
}



// ========================================================
// ========== 處理雷達封包 =================================
// ========================================================
void processIncomingByte(byte data) {
  static bool inPacket = false;
  static byte buf[MAX_PACKET_LEN];
  static int idx = 0;
  static byte last = 0;

  if (!inPacket) {
    if (last == HEADER1 && data == HEADER2) {
      inPacket = true;
      idx = 0;
      buf[idx++] = HEADER1;
      buf[idx++] = HEADER2;
    }
  } else {
    buf[idx++] = data;

    if (idx >= 2 &&
        buf[idx - 2] == TAIL1 &&
        buf[idx - 1] == TAIL2) {
      printPacket(buf, idx);

      // 檢查是否為跌倒封包
      if (isFallPacket(buf, idx)) {
        // 傳送 1 給伺服器表示跌倒
        Wifi.writeDataTcp(1, "1");  // 傳送「跌倒」事件
        // 播放跌倒語音（例如：VOC_1）
        myBMV31T001.playVoice(voice_table[0]); // 播放跌倒警告聲音
      }

      // 檢查是否為「Body movement parameters」封包
      if (isBodyMovementPacket(buf, idx)) {
        // 傳送活動量數據，僅回傳「活動量」部分（[7, 活動量]）
        int movementValue = buf[6]; // 活動量數據

        char buffer[256];
        int len = snprintf(buffer, sizeof(buffer),"[7, %d]", movementValue); // 活動量前面加上數字7

        Wifi.writeDataTcp(len, buffer);  // 發送到伺服器
      }

      inPacket = false;
      idx = 0;
    }
  }
  last = data;
}


// ========================================================
// ========== 判斷跌倒封包 =================================
// ========================================================
bool isFallPacket(const byte *buf, int len) {
  if (len < 10) return false;

  if (buf[2] == 0x83 &&
      buf[3] == 0x01 &&
      buf[6] == 0x01) {
    return true;
  }
  return false;
}


// ========================================================
// ========== 判斷 Body Movement 封包 =======================
// ========================================================
// 檢查是否為 Body Movement (活動量) 封包
bool isBodyMovementPacket(const byte *buf, int len) {
  // 固定長度 10 Bytes
  if (len != 10) return false;
  // 固定 header
  if (buf[0] != 0x53 || buf[1] != 0x59) return false;

  // Ctrl = 0x80（人體存在）, Cmd = 0x03（活動量）
  if (buf[2] != 0x80 || buf[3] != 0x03) return false;

  // Data length 固定 = 0x0001
  if (buf[4] != 0x00 || buf[5] != 0x01) return false;

  // 尾碼固定 0x54 0x43
  if (buf[8] != 0x54 || buf[9] != 0x43) return false;

  // 以上都符合就算是 Body Movement 封包
  return true;
}


// ========================================================
// ========== 印出封包 =====================================
// ========================================================
void printPacket(const byte *buf, int len) {
  Serial.print("RX: ");
  for (int i = 0; i < len; i++) {
    Serial.print("0x");
    if (buf[i] < 0x10) Serial.print("0");
    Serial.print(buf[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}
// ========================================================
// ========== 功能：發送封包 + 接收回應 =====================
// ========================================================
void sendAndReceive(const byte *cmd, int len, const char *desc) {

  Serial.print("Send: "); Serial.println(desc);

  for (int i = 0; i < len; i++) {
    Serial4.write(cmd[i]);
    Serial.print("0x"); Serial.print(cmd[i], HEX); Serial.print(" ");
  }
  Serial.println();
}
// 從伺服器傳來的格式 ",12]" 中取出數字
int parseServerNumber(const String &msg) {
  int start = msg.indexOf(',');   // 找到逗號的位置
  int end   = msg.indexOf(']');   // 找到 ] 的位置

  // 如果找不到逗號或 ]，或位置反了，就表示格式錯
  if (start == -1 || end == -1 || end <= start) {
    return -1; // 格式錯誤（回傳 -1）
  }

  // 取出逗號與 ] 之間的字串（例如 "12"）
  String numStr = msg.substring(start + 1, end);

  // 把字串轉成真正的整數
  return numStr.toInt();
}
