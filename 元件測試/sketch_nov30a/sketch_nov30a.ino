#define HEADER1 0x53
#define HEADER2 0x59
#define TAIL1   0x54
#define TAIL2   0x43

const int MAX_PACKET_LEN = 64;

// ------------------- 指令封包 -------------------
// 跌倒相關封包
const byte queryCmd[]        = { 0x53, 0x59, 0x83, 0x80, 0x00, 0x01, 0x0F, 0xBF, 0x54, 0x43 };
const byte enableCmd[]       = { 0x53, 0x59, 0x83, 0x00, 0x00, 0x01, 0x01, 0x31, 0x54, 0x43 };
const byte sensitivityCmd[]  = { 0x53, 0x59, 0x83, 0x0D, 0x00, 0x01, 0x03, 0x40, 0x54, 0x43 };
const byte fallTimeCmd[]     = { 0x53, 0x59, 0x83, 0x0C, 0x00, 0x04, 0x00, 0x00, 0x00, 0x05, 0x44, 0x54, 0x43 };
const byte heightCmd[]       = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x01, 0x0E, 0xC5, 0x54, 0x43 };

// Residence Time = 60 秒
const byte residenceTime60Cmd[] = {
  0x53, 0x59, 0x83, 0x0A, 0x00, 0x04,
  0x00, 0x00, 0x00, 0x3C, // 60 秒
  0x79,                   // checksum
  0x54, 0x43
};

// Human Presence Detection 強制開啟
const byte humanPresenceCmd[] = {
  0x53, 0x59, 0x80, 0x00, 0x00, 0x01, 0x01, 0x2E, 0x54, 0x43
};
// checksum 0xD5 = (0x53 + 0x59 + 0x80 + 0x00 + 0x00 + 0x01 + 0x01) & 0xFF

// ------------------- 狀態暫存與解析 -------------------
void processIncomingByte(byte data);
bool isFallPacket(const byte *buf, int len);
bool isResidenceTimeTooLong(const byte *buf, int len, int thresholdSec);
bool isStationary(const byte *buf, int len);
bool getStationaryState(const byte *buf, int len);
void printPacket(const byte *buf, int len);

// ------------------- Setup -------------------
void setup() {
  Serial.begin(115200);
  Serial4.begin(115200);
  Serial.println("UART4 ready...");
  delay(2000);

  // 傳送各項設定指令
  sendAndReceive(queryCmd, sizeof(queryCmd), "Query Fall Function");
  delay(500);
  sendAndReceive(enableCmd, sizeof(enableCmd), "Enable Fall Detection");
  delay(500);
  sendAndReceive(sensitivityCmd, sizeof(sensitivityCmd), "Set Sensitivity = 3");
  delay(500);
  sendAndReceive(fallTimeCmd, sizeof(fallTimeCmd), "Set Fall Time = 10s");
  delay(500);
  sendAndReceive(heightCmd, sizeof(heightCmd), "Set Height = 2.7m");
  delay(500);
  sendAndReceive(residenceTime60Cmd, sizeof(residenceTime60Cmd), "Set Residence Time = 60s");
  delay(500);

  // Human Presence Detection 強制開啟
  sendAndReceive(humanPresenceCmd, sizeof(humanPresenceCmd), "Force Human Presence ON");

  Serial.println("All commands sent. Entering continuous receive mode...");
}

// ------------------- Loop -------------------
void loop() {
  while (Serial4.available() > 0) {
    byte data = Serial4.read();
    processIncomingByte(data);
  }
}

// ------------------- 傳送指令並接收 -------------------
void sendAndReceive(const byte *cmd, int len, const char *desc) {
  Serial.print("Sending: ");
  Serial.println(desc);

  Serial.print("TX Data: ");
  for (int i = 0; i < len; i++) {
    Serial4.write(cmd[i]);
    Serial.print("0x");
    if (cmd[i] < 0x10) Serial.print("0");
    Serial.print(cmd[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  unsigned long start = millis();
  Serial.println("RX Data:");
  while (millis() - start < 1000) {
    if (Serial4.available() > 0) {
      byte b = Serial4.read();
      processIncomingByte(b);
    }
  }
  Serial.println("------------------------------------------------");
}

// ------------------- 封包解析 -------------------
void processIncomingByte(byte data) {
  static bool inPacket = false;
  static byte packetBuf[MAX_PACKET_LEN];
  static int packetIndex = 0;
  static byte lastByte = 0;

  if (!inPacket) {
    if (lastByte == HEADER1 && data == HEADER2) {
      inPacket = true;
      packetIndex = 0;
      packetBuf[packetIndex++] = HEADER1;
      packetBuf[packetIndex++] = HEADER2;
    }
  } else {
    if (packetIndex < MAX_PACKET_LEN) packetBuf[packetIndex++] = data;

    if (packetIndex >= 2 &&
        packetBuf[packetIndex - 2] == TAIL1 &&
        packetBuf[packetIndex - 1] == TAIL2) {

      printPacket(packetBuf, packetIndex);

      if (isFallPacket(packetBuf, packetIndex))
        Serial.println(">>> FALL DETECTED <<<");

      if (isResidenceTimeTooLong(packetBuf, packetIndex, 60))
        Serial.println(">>> RESIDENCE TIME TOO LONG! <<<");

      if (isStationary(packetBuf, packetIndex)) {
        if (getStationaryState(packetBuf, packetIndex))
          Serial.println(">>> STATIONARY RESIDENCY DETECTED <<<");
        else
          Serial.println(">>> NO STATIONARY RESIDENCY <<<");
      }

      inPacket = false;
      packetIndex = 0;
    }
  }
  lastByte = data;
}

// ------------------- 判斷封包 -------------------
bool isFallPacket(const byte *buf, int len) {
  if (len < 10) return false;
  if (buf[0] != HEADER1 || buf[1] != HEADER2) return false;
  if (buf[2] != 0x83 || buf[3] != 0x01) return false;
  if (buf[4] != 0x00 || buf[5] != 0x01) return false;
  return (buf[6] == 0x01);
}

bool isResidenceTimeTooLong(const byte *buf, int len, int thresholdSec) {
  if (len < 12) return false;
  if (buf[2] != 0x83 || buf[3] != 0x0A) return false;
  int timeSec = (buf[7] << 24) | (buf[8] << 16) | (buf[9] << 8) | buf[10];
  return (timeSec > thresholdSec);
}

bool isStationary(const byte *buf, int len) {
  if (len < 10) return false;
  return (buf[2] == 0x83 && buf[3] == 0x05);
}

bool getStationaryState(const byte *buf, int len) {
  return (buf[6] == 0x01);
}

// ------------------- HEX 封包印出 -------------------
void printPacket(const byte *buf, int len) {
  Serial.print("RX Packet: ");
  for (int i = 0; i < len; i++) {
    Serial.print("0x");
    if (buf[i] < 0x10) Serial.print("0");
    Serial.print(buf[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}
