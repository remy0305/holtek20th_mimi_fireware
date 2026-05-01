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
//const byte sensitivityCmd[] = { 0x53, 0x59, 0x83, 0x0D, 0x00, 0x01, 0x02, 0x3F, 0x54, 0x43 };//靈敏度2
const byte sensitivityCmd[] = { 0x53, 0x59, 0x83, 0x0D, 0x00, 0x01, 0x03, 0x40, 0x54, 0x43 };//靈敏度3
const byte fallTimeCmd[]    = {0x53,0x59,0x83,0x0C,0x00,0x04,0x00,0x00,0x00,0x05,0x44,0x54,0x43};
const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x01, 0x16, 0xCD, 0x54, 0x43 }; // 2.78m
//const byte heightCmd[]      = {0x53,0x59,0x06,0x02,0x00,0x02,0x01,0x13,0xCA,0x54,0x43};//2.75m
//const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x01, 0x0E, 0xC5, 0x54, 0x43 }; //2.7m

/* ===================== Prototypes ===================== */
static bool sendToServer(const char* msg);

static void sendRadarCmd(const byte* cmd, int len);
static void processRadarByte(byte data);
static bool isFallPacket(const byte* buf, int len);
static bool isBodyMovementPacket(const byte* buf, int len);

/* ===================== Static variables ===================== */
static unsigned long lastMoveMs = 0;
static int lastMoveVal = 0;
static bool fallDetected = false;

static unsigned long inactivityTime = 0;
static unsigned long lastSendTime = 0;

/* ✅ 伺服器目標秒數（改成由 stateCount 設定） */
static unsigned long serverTargetSeconds = 60;

/* ✅ stateCount 設定（你要的新功能） */
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
    // stateCount = -1 時：你可以要「不觸發」或維持原值
    // 這裡我選擇：維持不變
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

  // 若你想看 stateCount log，可保留；不想看就拿掉這行
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

  // ✅ 初始套用一次（stateCount=-1 會不改）
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

    // ✅ 依 stateCount 控制 D6 / D5（你原本規則）
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

    // 如果你想把目前模式/秒數送到伺服器當作紀錄，也可以保留
    //（不想送就註解掉）
    char msg[48];
    snprintf(msg, sizeof(msg), "[mode,%d,%lu]\r\n", stateCount, serverTargetSeconds);
    sendToServer(msg);
  }

  lastState = currentState;

  /* ====== 2) 雷達 UART 處理（原本保留） ====== */
  while (RADAR_UART.available()) {
    processRadarByte((byte)RADAR_UART.read());
  }

  /* ====== 3) 靜止計時（每秒一次，原本保留） ====== */
  unsigned long now = millis();
  if (now - lastSendTime >= 1000) {
    lastSendTime = now;

    if (lastMoveVal < 5) inactivityTime++;
    else inactivityTime = 0;

    if (inactivityTime >= serverTargetSeconds) {
      sendToServer("8\r\n");

      for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(200);
        digitalWrite(LED_PIN, LOW);
        delay(200);
      }
    }
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

      // 跌倒：送 1（原本保留）
      if (isFallPacket(buf, idx) && !fallDetected) {
        sendToServer("1\r\n");
        for (int i = 0; i < 3; i++) {
          digitalWrite(LED_PIN, HIGH);
          delay(200);
          digitalWrite(LED_PIN, LOW);
          delay(200);
      }
        fallDetected = true;
      }

      // 活動量：送 [7,val]（原本保留）
      if (isBodyMovementPacket(buf, idx)) {
        int val = buf[6];
        unsigned long now = millis();
        if (now - lastMoveMs > 300 && val != lastMoveVal) {
          char msg[32];
          snprintf(msg, sizeof(msg), "[7,%d]\r\n", val);
          sendToServer(msg);
          lastMoveMs = now;
          lastMoveVal = val;
        }
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
