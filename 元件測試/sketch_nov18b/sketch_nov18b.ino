/*****************************************************************
File:         TCP+AMG.ino
Desc:        在原本 MR60FDA1 雷達 → TCP 的流程中，新增
              BMS26M833 紅外線矩陣資料，定期打包經 TCP 傳到伺服器
******************************************************************/
#include "TCP.h"
#include "Arduino.h"
#include <60ghzfalldetection.h>
#include <stdio.h>
#include "BMV31T001.h"

// === AMG START: 紅外線矩陣 ===
#include <Wire.h>
#include <BMS26M833.h>
BMS26M833 Amg;
float g_thermistor = 0.0f;
float g_pixels[64];
// 可調：多久傳一次 AMG（毫秒）
#define AMG_SEND_INTERVAL_MS 1000UL
static unsigned long amg_last_ms = 0;
// === AMG END ===

BMV31T001 myBMV31T001; //Create an object

#define VOICE_TOTAL_NUMBER 10
const uint8_t voice_table[VOICE_TOTAL_NUMBER] = {
  VOC_1, VOC_2, VOC_3, VOC_4, VOC_5, VOC_6, VOC_7, VOC_8, VOC_9, VOC_10
};
uint8_t keyStatus; 
uint8_t lastKeyStatus;
uint8_t playStatus;
uint8_t playNum;
#define DEFAULT_VOLUME 6
uint8_t volume = DEFAULT_VOLUME;
uint8_t keycode = 0;

// === 改動重點 ===
// Wi-Fi → UART0（你原碼把 Wifi 綁在 Serial，維持不變，避免動到控制流程）
BMC81M001 Wifi(&Serial);

// MR60FDA1 雷達 → UART4（維持不變）
FallDetection_60GHz radar = FallDetection_60GHz(&Serial4);

// 你原本程式裡用到的外部緩衝（保留原樣；若已在別處宣告可移除這裡）
#ifndef HAS_SERIALBUFF_DECL
#endif
#ifndef LED
#define LED LED_BUILTIN
#endif

void clearBuff(); // 前向宣告

// === AMG START: 打包與傳送 ===
// 產生一段緊湊的 CSV/JSON 混合字串：amg,<thermistor>,p[64]
static String buildAmgPacket(float therm, const float* px64) {
  String s = "amg,therm=";
  s += String(therm, 2);
  s += ",p=[";
  for (int i = 0; i < 64; ++i) {
    s += String(px64[i], 1);
    if (i != 63) s += ",";
  }
  s += "]";
  return s;
}

static void readAndSendAmgIfDue() {
  unsigned long now = millis();
  if (now - amg_last_ms < AMG_SEND_INTERVAL_MS) return;
  amg_last_ms = now;

  // 讀取紅外線矩陣 + 熱敏
  Amg.readPixels(g_pixels);
  g_thermistor = Amg.readThermistorTemp();

  // 打包並經 TCP 送出（沿用你雷達送法）
  String pkt = buildAmgPacket(g_thermistor, g_pixels);
  if (pkt.length() > 0) {
    Wifi.writeDataTcp(pkt.length(), (char*)pkt.c_str());
  }
}
// === AMG END ===

void setup() {
  // BMV31T001
  myBMV31T001.begin();
  myBMV31T001.setPower(BMV31T001_POWER_ENABLE);
  myBMV31T001.initAudioUpdate();
  delay(100);
  myBMV31T001.setVolume(DEFAULT_VOLUME);

  // 主控台除錯：USB
  Serial.begin(115200);

  // UART0：Wi-Fi（保留你原本用 Serial begin）
  Serial.begin(115200);

  // UART4：MR60FDA1
  Serial4.begin(115200);

  pinMode(LED, OUTPUT);

  // === AMG START: I2C/感測器初始化（新增） ===
  Wire.begin();
  Amg.begin();        // 使用庫預設位址初始化
  // === AMG END ===

  // 初始化 Wi-Fi
  Wifi.begin();
  Serial.print("TCP Connect Result：");

  if (!Wifi.connectToAP(WIFI_SSID, WIFI_PASS)) {
    Serial.print("WIFI fail,");
  } else {
    Serial.print("WIFI success,");
  }

  if (!Wifi.connectTCP(IP, IP_Port)) {
    Serial.print("IP fail");
  } else {
    Serial.print("IP success");
  }

  Serial.println("\nReady");
  Wifi.writeDataTcp(1, "1");
}

void loop() {
  // 讀取雷達數據（UART4）→ 送伺服器（保留原邏輯）
  radar.recvRadarBytes();
  String radarData = radar.getDataAsString();
  if (radarData.length() > 0) {
    // 可在前綴加類型標籤，避免伺服器端混淆
    String pkt = "radar," + radarData;
    Wifi.writeDataTcp(pkt.length(), (char*)pkt.c_str());
  }

  // === AMG START: 週期性傳送 AMG 紅外線矩陣（新增，不影響原流程） ===
  readAndSendAmgIfDue();
  // === AMG END ===

  // 從伺服器接收數據並顯示（保留原邏輯）
  String tcpBuffStr = Wifi.readDataTcp();
  if (tcpBuffStr.length() > 0) {
    Serial.println("Received from server:");
    Serial.println(tcpBuffStr);

    // 檢查是否包含 "fall"
    if (tcpBuffStr.indexOf("OK") >= 0) {
      // 播放語音
      uint8_t idx = (playNum < VOICE_TOTAL_NUMBER) ? playNum : 0;
      myBMV31T001.playVoice(voice_table[idx]);
      Serial.println("Fall detected! Stopping...");
      return; // 只結束本輪 loop()，下輪照常跑
    }
  }

  // 將主控台輸入透過 TCP 轉發（保留原邏輯）
  while (Serial.available() > 0) {
    SerialBuff[resLen++] = Serial.read();
    delay(10);
    if (resLen >= sizeof(SerialBuff)) break;
  }

  if (resLen > 0) {
    digitalWrite(LED, HIGH);
    if (Wifi.writeDataTcp(resLen, SerialBuff)) {
      Serial.println("Send data success");
      digitalWrite(LED, LOW);
    }
    clearBuff();
  }

  delay(200);
}

// 清空緩衝（保留原邏輯）
void clearBuff() {
  memset(SerialBuff, '\0', sizeof(SerialBuff));
  resLen = 0;
}
