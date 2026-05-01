/*****************************************************************
   WiFi TCP + AMG8833 + MR60FDA1 跌倒偵測 + 音樂播放（發出聲音）
******************************************************************/
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
//BMV31T001 音頻播放
BMV31T001 myBMV31T001; // Create an object for the audio module
#define VOICE_TOTAL_NUMBER 10 // Number of available voice files
const uint8_t voice_table[VOICE_TOTAL_NUMBER] = {
    VOC_1, VOC_2, VOC_3, VOC_4, VOC_5, VOC_6, VOC_7, VOC_8, VOC_9, VOC_10
};
#define DEFAULT_VOLUME 6 // Default volume level
uint8_t volume = DEFAULT_VOLUME; // Current volume
uint8_t playNum = 0; // Track number being played
//END BMV31T001 音頻播放
//雷達 UART4 設定
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
//const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x00, 0xFA, 0xB0, 0x54, 0x43 };//2.5M
const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x00, 0xE6, 0x9C, 0x54, 0x43 };//2.3M// 狀態
bool fallDetected = false;
// 暫存
// 伺服器下發的靜止門檻秒數
int targetSeconds = 0;
int idleCounter = 0;

// ---- Helper ----
int parseSecondValueInBracket(const String &s);
bool readRadarPacket(uint16_t timeoutMs, byte *outBuf, int &outLen);


void processIncomingByte(byte data);
bool isFallPacket(const byte *buf, int len);
bool isBodyMovementPacket(const byte *buf, int len);
void printPacket(const byte *buf, int len);
void sendAndReceive(const byte *cmd, int len, const char *desc);
//禁止時間的變數
int targetSeconds = 0;      // 伺服器給的時間
int idleCounter = 0;        // 活動量低於 10 的累積秒數
bool readingTime = false;   // 用來讀取 [XX] 的狀態
char timeBuf[16];           // 暫存伺服器送過來的字串
int timeIdx = 0;
void setup() {
  Serial.begin(115200);
  Serial4.begin(115200);
  Serial.println("UART4 Ready...");
  delay(2000);
  //WiFi 開始連線
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
  //雷達初始化
  sendAndReceive(queryCmd, sizeof(queryCmd), "Query Fall");
  delay(500);
  sendAndReceive(enableCmd, sizeof(enableCmd), "Enable Fall");
  delay(500);
  sendAndReceive(sensitivityCmd, sizeof(sensitivityCmd), "Sensitivity");
  delay(500);
  sendAndReceive(fallTimeCmd, sizeof(fallTimeCmd), "Fall Time = 10s");
  delay(500);
  sendAndReceive(heightCmd, sizeof(heightCmd), "Height = 2.7m");
  //AMG 開始
  Amg.begin();
  Serial.println("AMG8833 Ready");
  //BMV31T001 音頻播放初始化
  myBMV31T001.begin(); // Initialize BMV31T001
  myBMV31T001.setPower(BMV31T001_POWER_ENABLE); // Power on the BMV31T001
  myBMV31T001.setVolume(DEFAULT_VOLUME); // Set initial volume
  myBMV31T001.initAudioUpdate(); // Initialize audio update if needed
  delay(100); // Delay for the audio source to power on
}
void loop() {
  // --- 接收伺服器傳來的 TCP 資料 ---
  String tcpBuff = Wifi.readDataTcp();
  if (tcpBuff.length() > 0) {
    Serial.print("TCP recv: ");
    Serial.println(tcpBuff);

    int t = parseSecondValueInBracket(tcpBuff);
    if (t >= 0) {
      targetSeconds = t;
      Serial.print("Server Time = ");
      Serial.println(targetSeconds);
    }
  }

  // --- 讀雷達 UART4 ---
  while (Serial4.available() > 0) {
    byte data = Serial4.read();
    processIncomingByte(data);
  }

  // --- 讀 AMG8833 ---
  Amg.readPixels(g_pixels);
  g_thermistor = Amg.readThermistorTemp();

  // --- 溫度資料上傳 ---
  char buffer[256];
  int len = snprintf(buffer, sizeof(buffer), "THERM=%.2f", g_thermistor);
  Wifi.writeDataTcp(len, buffer);

  delay(500);
}

//處理雷達封包
void processIncomingByte(byte data) {
  static bool inPacket = false;
  static byte buf[MAX_PACKET_LEN];
  static int idx = 0;
  static byte last = 0;

  // 防呆：溢位就重置
  if (idx >= MAX_PACKET_LEN) {
    inPacket = false;
    idx = 0;
  }

  if (!inPacket) {
    if (last == HEADER1 && data == HEADER2) {
      inPacket = true;
      idx = 0;
      buf[idx++] = HEADER1;
      buf[idx++] = HEADER2;
    }
  } else {
    buf[idx++] = data;

    // 封包結尾判斷
    if (idx >= 2 && buf[idx - 2] == TAIL1 && buf[idx - 1] == TAIL2) {

      printPacket(buf, idx);

      // ---- 跌倒封包 ----
      if (isFallPacket(buf, idx)) {
        Wifi.writeDataTcp(1, "1");
        myBMV31T001.playVoice(voice_table[0]);
      }

      // ---- Body Movement ----
      if (isBodyMovementPacket(buf, idx)) {
        // 53 59 80 03 00 01 [data] [chk] 54 43
        uint8_t movementValue = buf[6];

        // 回傳活動量
        char out[32];
        int outLen = snprintf(out, sizeof(out), "[7, %u]", movementValue);
        Wifi.writeDataTcp(outLen, out);

        // 靜止計時
        if (movementValue < 10) {
          idleCounter++;
          Serial.print(idleCounter);
          Serial.println("s");

          // 觸發門檻 -> 送 8
          if (targetSeconds > 0 && idleCounter >= targetSeconds) {
            Wifi.writeDataTcp(1, "8");
            Serial.println("Send 8 to server (Idle timeout)");
            idleCounter = 0;
            myBMV31T001.playVoice(voice_table[0]);
          }
        } else {
          if (idleCounter != 0) {
            Serial.println("reset");
          }
          idleCounter = 0;
        }
      }

      // 重置收包狀態
      inPacket = false;
      idx = 0;
    }
  }

  last = data;
}


//判斷跌倒封包
bool isFallPacket(const byte *buf, int len) {
  if (len < 10) return false;

  if (buf[2] == 0x83 &&
      buf[3] == 0x01 &&
      buf[6] == 0x01) {
    return true;
  }
  return false;
}
//判斷 Body Movement 封包
bool isBodyMovementPacket(const byte *buf, int len) {
  if (len != 10) return false;

  if (buf[0] == HEADER1 &&
      buf[1] == HEADER2 &&
      buf[2] == 0x80 &&
      buf[3] == 0x03 &&
      buf[4] == 0x00 &&
      buf[5] == 0x01 &&
      buf[8] == TAIL1 &&
      buf[9] == TAIL2) {
    return true;
  }
  return false;
}
//印出封包
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

//功能：發送封包 + 接收回應
void sendAndReceive(const byte *cmd, int len, const char *desc) {
  Serial.print("Send: ");
  Serial.println(desc);

  // 清掉殘留資料
  while (Serial4.available() > 0) {
    Serial4.read();
  }

  // 發送命令
  for (int i = 0; i < len; i++) {
    Serial4.write(cmd[i]);
    Serial.print("0x");
    if (cmd[i] < 0x10) Serial.print("0");
    Serial.print(cmd[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // 讀回應封包（簡單等待）
  byte resp[MAX_PACKET_LEN];
  int respLen = 0;

  if (readRadarPacket(250, resp, respLen)) {
    Serial.print("Resp ");
    Serial.print(desc);
    Serial.println(":");
    printPacket(resp, respLen);
  } else {
    Serial.print("Resp ");
    Serial.print(desc);
    Serial.println(": (no packet)");
  }
}
int parseSecondValueInBracket(const String &s) {
  int lb = s.indexOf('[');
  int comma = s.indexOf(',', lb + 1);
  int rb = s.indexOf(']', comma + 1);
  if (lb < 0 || comma < 0 || rb < 0) return -1;

  String num = s.substring(comma + 1, rb);
  num.trim();
  if (num.length() == 0) return -1;

  // 只接受 0-9 與負號開頭（避免爆字串）
  for (int i = 0; i < num.length(); i++) {
    char c = num[i];
    if (!(c == '-' || (c >= '0' && c <= '9'))) {
      return -1;
    }
  }

  return num.toInt();
}
