#include <Arduino.h>
#include <string.h>

#define LED_PIN D4

/* ===================== WiFi (UART0) ===================== */
#define WIFI_SSID        "A54"
#define WIFI_PASSWORD    "remy2271"
#define SERVER_IP        "10.77.252.61"
#define SERVER_PORT_STR  "8000"

#define WIFI_UART  Serial      // WiFi UART0
#define RADAR_UART Serial1     // 雷達 UART1

/* ===================== Button/State pins ===================== */
#define BTN_PIN  7   // D7 (INPUT_PULLUP)
#define OUT_D6   6
#define OUT_D5   5

/* ===================== Radar packet format ===================== */
#define MAX_PACKET_LEN 128
#define HEADER1 0x53
#define HEADER2 0x59
#define TAIL1   0x54
#define TAIL2   0x43

/* ===================== Radar commands ===================== */
const byte queryCmd[]       = {0x53,0x59,0x83,0x80,0x00,0x01,0x0F,0xBF,0x54,0x43};
const byte enableCmd[]      = {0x53,0x59,0x83,0x00,0x00,0x01,0x01,0x31,0x54,0x43};
const byte sensitivityCmd[] = { 0x53, 0x59, 0x83, 0x0D, 0x00, 0x01, 0x03, 0x40, 0x54, 0x43 };//靈敏度3
const byte fallTimeCmd[]    = {0x53,0x59,0x83,0x0C,0x00,0x04,0x00,0x00,0x00,0x05,0x44,0x54,0x43};
const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x01, 0x16, 0xCD, 0x54, 0x43 }; // 2.78m

/* ===================== Prototypes ===================== */
static bool sendToServer(const char* msg);
static void sendRadarCmd(const byte* cmd, int len);
static void processRadarByte(byte data);
static bool isFallPacket(const byte* buf, int len);
static bool isBodyMovementPacket(const byte* buf, int len);

/* ===================== Static variables ===================== */
// ❌ 移除 fallDetected 標記

/* ✅ 活動量矩陣記錄（跟 ESP32 版本一樣）*/
#define MAX_HISTORY_SIZE 200  // 最多記錄 200 秒的歷史數據
static int activityHistory[MAX_HISTORY_SIZE];  // 活動量歷史矩陣
static int historyIndex = 0;  // 當前寫入位置
static int historyCount = 0;  // 已記錄的數據筆數

/* ✅ 伺服器目標秒數（改成由 stateCount 設定） */
static unsigned long serverTargetSeconds = 60;

/* ✅ stateCount 設定 */
static int stateCount = -1;     // 初始 -1（尚未選模式）
static int lastState  = HIGH;   // INPUT_PULLUP 未按下 HIGH

/* ===================== 小工具：依 stateCount 套用秒數 ===================== */
static void applyStateToTargetSeconds() {
  if (stateCount == 0) {
    serverTargetSeconds = 8UL * 3600UL;   // 8 小時
  } else if (stateCount == 1) {
    serverTargetSeconds = 6UL * 3600UL;   // 6 小時
  } else if (stateCount == 2) {
    serverTargetSeconds = 5UL;            // 5 秒
  } else {
    // stateCount = -1 時：維持不變
  }
}

/* ===================== Setup ===================== */
void setup() {
  pinMode(LED_PIN, OUTPUT);

  // ✅ 按鍵/輸出腳位
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(OUT_D6, OUTPUT);
  pinMode(OUT_D5, OUTPUT);
  digitalWrite(OUT_D6, LOW);
  digitalWrite(OUT_D5, LOW);

  WIFI_UART.begin(115200);   // WiFi UART0
  RADAR_UART.begin(115200);  // 雷達 UART1
  delay(500);

  // 連接 WiFi
  WIFI_UART.print("AT+CWJAP=\"" WIFI_SSID "\",\"" WIFI_PASSWORD "\"\r\n");
  delay(5000);

  // 建立 TCP 連線
  WIFI_UART.print("AT+CIPSTART=\"TCP\",\"" SERVER_IP "\"," SERVER_PORT_STR "\r\n");
  delay(2000);

  // Radar 初始化
  sendRadarCmd(queryCmd, sizeof(queryCmd));
  delay(300);
  sendRadarCmd(enableCmd, sizeof(enableCmd));
  delay(300);
  sendRadarCmd(sensitivityCmd, sizeof(sensitivityCmd));
  delay(300);
  sendRadarCmd(fallTimeCmd, sizeof(fallTimeCmd));
  delay(300);
  sendRadarCmd(heightCmd, sizeof(heightCmd));

  // ✅ 初始化活動量歷史記錄
  for (int i = 0; i < MAX_HISTORY_SIZE; i++) {
    activityHistory[i] = 100;  // 初始化為高活動量
  }

  // ✅ 初始套用一次
  applyStateToTargetSeconds();
}

/* ===================== Loop ===================== */
void loop() {
  /* ====== 1) D7 按鍵切換 stateCount，並更新 serverTargetSeconds ====== */
  int currentState = digitalRead(BTN_PIN);

  // 偵測「按下瞬間」：HIGH -> LOW
  if (lastState == HIGH && currentState == LOW) {
    delay(30); // debounce

    stateCount++;
    if (stateCount > 2) stateCount = 0;

    // ✅ 依 stateCount 更新秒數
    applyStateToTargetSeconds();

    // ✅ 依 stateCount 控制 D6 / D5
    if (stateCount == 0) {
      digitalWrite(OUT_D6, HIGH);
      digitalWrite(OUT_D5, LOW);
    } else if (stateCount == 1) {
      digitalWrite(OUT_D6, LOW);
      digitalWrite(OUT_D5, HIGH);
    } else if (stateCount == 2) {
      digitalWrite(OUT_D6, HIGH);
      digitalWrite(OUT_D5, HIGH);
    }

    // ✅ 清空歷史記錄，重新開始計算（跟 ESP32 版本一樣）
    for (int i = 0; i < MAX_HISTORY_SIZE; i++) {
      activityHistory[i] = 100;
    }
    historyIndex = 0;
    historyCount = 0;

    // 發送模式切換訊息到伺服器
    char msg[48];
    snprintf(msg, sizeof(msg), "[mode,%d,%lu]\r\n", stateCount, serverTargetSeconds);
    sendToServer(msg);
  }

  lastState = currentState;

  /* ====== 2) 雷達 UART 處理 ====== */
  while (RADAR_UART.available()) {
    processRadarByte((byte)RADAR_UART.read());
  }
}

/* ===================== WiFi Send ===================== */
static bool sendToServer(const char* msg) {
  if (!msg) return false;
  int len = (int)strlen(msg);
  if (len <= 0) return false;

  WIFI_UART.print("AT+CIPSEND=");
  WIFI_UART.print(len);
  WIFI_UART.print("\r\n");
  delay(50);

  WIFI_UART.write((const uint8_t*)msg, len);
  delay(50);
  return true;
}

/* ===================== Radar helpers ===================== */
static void sendRadarCmd(const byte* cmd, int len) {
  for (int i = 0; i < len; i++) RADAR_UART.write(cmd[i]);
}

static void processRadarByte(byte data) {
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
    if (idx < MAX_PACKET_LEN) buf[idx++] = data;
    else { inPacket = false; idx = 0; }

    if (idx >= 2 && buf[idx - 2] == TAIL1 && buf[idx - 1] == TAIL2) {

      // ====== 跌倒偵測（移除 fallDetected 判斷）======
      if (isFallPacket(buf, idx)) {
        sendToServer("1\r\n");
        for (int i = 0; i < 3; i++) {
          digitalWrite(LED_PIN, HIGH);
          delay(200);
          digitalWrite(LED_PIN, LOW);
          delay(200);
        }
        // ✅ 移除 fallDetected = true; 讓跌倒可以一直偵測
      }

      // ====== 活動量處理（跟 ESP32 版本一樣的矩陣記錄邏輯）======
      if (isBodyMovementPacket(buf, idx)) {
        int movementValue = buf[6];  // 活動量 0~100

        // ✅ 將活動量存入矩陣
        activityHistory[historyIndex] = movementValue;
        historyIndex = (historyIndex + 1) % MAX_HISTORY_SIZE;  // 循環寫入
        if (historyCount < MAX_HISTORY_SIZE) {
          historyCount++;
        }

        // ✅ 檢查連續靜止時間
        if (serverTargetSeconds > 0 && historyCount >= serverTargetSeconds) {
          int consecutiveLowCount = 0;
          
          // 從最新的數據往回檢查 serverTargetSeconds 筆
          for (int i = 0; i < serverTargetSeconds; i++) {
            int checkIndex = (historyIndex - 1 - i + MAX_HISTORY_SIZE) % MAX_HISTORY_SIZE;
            
            if (activityHistory[checkIndex] < 5) {
              consecutiveLowCount++;
            } else {
              break;  // 一旦遇到高活動量就中斷
            }
          }

          // ✅ 如果連續靜止秒數達標 → 觸發警報
          if (consecutiveLowCount >= serverTargetSeconds) {
            sendToServer("8\r\n");
            
            // LED 閃爍警報
            for (int i = 0; i < 3; i++) {
              digitalWrite(LED_PIN, HIGH);
              delay(200);
              digitalWrite(LED_PIN, LOW);
              delay(200);
            }
            
            // ✅ 清空歷史記錄避免連續觸發
            for (int i = 0; i < MAX_HISTORY_SIZE; i++) {
              activityHistory[i] = 100;
            }
            historyIndex = 0;
            historyCount = 0;
          }
        }

        // ✅ 回傳活動量給伺服器（跟 ESP32 版本一樣）
        char msg[32];
        snprintf(msg, sizeof(msg), "[7,%d]\r\n", movementValue);
        sendToServer(msg);
      }

      inPacket = false;
      idx = 0;
    }
  }

  last = data;
}

static bool isFallPacket(const byte* buf, int len) {
  return (len >= 10 && buf[2] == 0x83 && buf[3] == 0x01 && buf[6] == 0x01);
}

static bool isBodyMovementPacket(const byte* buf, int len) {
  return (len == 10 && buf[2] == 0x80 && buf[3] == 0x03);
}