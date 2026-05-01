#include <Arduino.h>
#include <string.h>
#include <Wire.h>
#include <math.h>

/* ===================== LED ===================== */
#define LED_PIN D4

/* ===================== WiFi (UART0) ===================== */
#define WIFI_SSID        "A54"
#define WIFI_PASSWORD    "remy2271"
#define SERVER_IP        "10.77.252.61"
#define SERVER_PORT_STR  "8000"

#define WIFI_UART  Serial
#define RADAR_UART Serial1

/* ===================== Button/State pins ===================== */
#define BTN_PIN  7
#define OUT_D6   6
#define OUT_D5   5

/* ===================== Radar packet ===================== */
#define MAX_PACKET_LEN 128
#define HEADER1 0x53
#define HEADER2 0x59
#define TAIL1   0x54
#define TAIL2   0x43

/* ===================== Radar commands ===================== */
const byte queryCmd[]       = {0x53,0x59,0x83,0x80,0x00,0x01,0x0F,0xBF,0x54,0x43};
const byte enableCmd[]      = {0x53,0x59,0x83,0x00,0x00,0x01,0x01,0x31,0x54,0x43};
const byte sensitivityCmd[] = {0x53,0x59,0x83,0x0D,0x00,0x01,0x02,0x3F,0x54,0x43};
const byte fallTimeCmd[]    = {0x53,0x59,0x83,0x0C,0x00,0x04,0x00,0x00,0x00,0x05,0x44,0x54,0x43};
const byte heightCmd[]      = {0x53,0x59,0x06,0x02,0x00,0x02,0x01,0x13,0xCA,0x54,0x43};

/* ===================== AMG8833 ===================== */
#define REG_THERMISTOR   0x0E
#define REG_PIXEL_START  0x80

static uint8_t amgAddr = 0;
static float   amgPixels[64];
static unsigned long lastAmgReadMs = 0;

/* ===================== 狀態變數 ===================== */
static unsigned long lastMoveMs = 0;
static int lastMoveVal = 0;
static bool fallDetected = false;

static unsigned long inactivityTime = 0;
static unsigned long lastSendTime = 0;

static unsigned long serverTargetSeconds = 60;

static int stateCount = -1;
static int lastState  = HIGH;

/* ===================== AMG8833 helpers ===================== */
static bool i2cProbe(uint8_t addr) {
  Wire.beginTransmission(addr);
  return (Wire.endTransmission(true) == 0);
}

static inline int16_t amg_raw12(int16_t v) {
  v &= 0x0FFF;
  if (v & 0x0800) v -= 0x1000;
  return v;
}

static inline float pixel_to_c(int16_t raw) {
  return (float)amg_raw12(raw) * 0.25f;
}

static bool readAmgPixels(uint8_t addr, float out[64]) {
  Wire.beginTransmission(addr);
  Wire.write(REG_PIXEL_START);
  if (Wire.endTransmission(false) != 0) return false;

  Wire.requestFrom((uint8_t)addr, (size_t)128, (uint8_t)true);
  if (Wire.available() < 128) return false;

  for (int i = 0; i < 64; i++) {
    uint8_t l = Wire.read();
    uint8_t h = Wire.read();
    int16_t raw = (int16_t)((h << 8) | l);
    float v = pixel_to_c(raw);
    if (!isfinite(v)) v = -999.0f;
    out[i] = v;
  }
  return true;
}

/* ===================== 狀態對應秒數 ===================== */
static void applyStateToTargetSeconds() {
  if (stateCount == 0) serverTargetSeconds = 8UL * 3600UL;
  else if (stateCount == 1) serverTargetSeconds = 6UL * 3600UL;
  else if (stateCount == 2) serverTargetSeconds = 5UL;
}

/* ===================== Radar ===================== */
static void sendRadarCmd(const byte* cmd, int len) {
  for (int i = 0; i < len; i++) RADAR_UART.write(cmd[i]);
}

static bool isFallPacket(const byte* buf, int len) {
  return (len >= 10 && buf[2] == 0x83 && buf[3] == 0x01 && buf[6] == 0x01);
}

static bool isBodyMovementPacket(const byte* buf, int len) {
  return (len == 10 && buf[2] == 0x80 && buf[3] == 0x03);
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
      if (isFallPacket(buf, idx) && !fallDetected) {
        fallDetected = true;
      }

      if (isBodyMovementPacket(buf, idx)) {
        int val = buf[6];
        unsigned long now = millis();
        if (now - lastMoveMs > 300) {
          lastMoveVal = val;
          lastMoveMs = now;
        }
      }
      inPacket = false;
      idx = 0;
    }
  }
  last = data;
}

/* ===================== Setup ===================== */
void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(OUT_D6, OUTPUT);
  pinMode(OUT_D5, OUTPUT);

  WIFI_UART.begin(115200);
  RADAR_UART.begin(115200);

  Wire.begin();
  Wire.setClock(400000);

  if (i2cProbe(0x68)) amgAddr = 0x68;
  else if (i2cProbe(0x69)) amgAddr = 0x69;

  delay(15000); // AMG8833 warm-up

  WIFI_UART.print("AT+CWJAP=\"" WIFI_SSID "\",\"" WIFI_PASSWORD "\"\r\n");
  delay(5000);
  WIFI_UART.print("AT+CIPSTART=\"TCP\",\"" SERVER_IP "\"," SERVER_PORT_STR "\r\n");
  delay(2000);

  sendRadarCmd(queryCmd, sizeof(queryCmd));
  delay(300);
  sendRadarCmd(enableCmd, sizeof(enableCmd));
  delay(300);
  sendRadarCmd(sensitivityCmd, sizeof(sensitivityCmd));
  delay(300);
  sendRadarCmd(fallTimeCmd, sizeof(fallTimeCmd));
  delay(300);
  sendRadarCmd(heightCmd, sizeof(heightCmd));

  applyStateToTargetSeconds();
}

/* ===================== Loop ===================== */
void loop() {
  /* D7 切換 */
  int cur = digitalRead(BTN_PIN);
  if (lastState == HIGH && cur == LOW) {
    delay(30);
    stateCount = (stateCount + 1) % 3;
    applyStateToTargetSeconds();

    digitalWrite(OUT_D6, stateCount != 1);
    digitalWrite(OUT_D5, stateCount != 0);
  }
  lastState = cur;

  /* Radar */
  while (RADAR_UART.available()) {
    processRadarByte((byte)RADAR_UART.read());
  }

  /* 靜止計時 */
  unsigned long now = millis();
  if (now - lastSendTime >= 1000) {
    lastSendTime = now;
    if (lastMoveVal < 5) inactivityTime++;
    else inactivityTime = 0;
  }

  /* AMG8833 背景讀取（不印、不送） */
  if (amgAddr && now - lastAmgReadMs >= 1000) {
    lastAmgReadMs = now;
    readAmgPixels(amgAddr, amgPixels);
  }
}
