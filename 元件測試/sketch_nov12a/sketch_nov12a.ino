#define HEADER1 0x53
#define HEADER2 0x59
#define TAIL1   0x54
#define TAIL2   0x43

// 跌倒相關封包
// 查詢跌倒監控功能開關的命令
const byte queryCmd[] = { 0x53, 0x59, 0x83, 0x80, 0x00, 0x01, 0x0F, 0xBF, 0x54, 0x43 };
// 開啟跌倒模式的命令
const byte enableCmd[] = { 0x53, 0x59, 0x83, 0x00, 0x00, 0x01, 0x01, 0x31, 0x54, 0x43 };
// 設定靈敏度（最高3）的命令
// 設定靈敏度（最高3）的命令
//const byte sensitivityCmd[] = { 0x53, 0x59, 0x83, 0x0D, 0x00, 0x01, 0x01, 0x3E, 0x54, 0x43 };//靈敏度1
const byte sensitivityCmd[] = { 0x53, 0x59, 0x83, 0x0D, 0x00, 0x01, 0x02, 0x3F, 0x54, 0x43 };//靈敏度2
//const byte sensitivityCmd[] = { 0x53, 0x59, 0x83, 0x0D, 0x00, 0x01, 0x03, 0x40, 0x54, 0x43 };//靈敏度3
// 設定跌倒判定時間的命令（例如 10秒）
const byte fallTimeCmd[] = { 0x53, 0x59, 0x83, 0x0C, 0x00, 0x04, 0x00, 0x00, 0x00, 0x05, 0x44, 0x54, 0x43 };
// 設定高度（安裝高度）的命令

//const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x01, 0x5E, 0x15, 0x54, 0x43};//3.5m
//const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x01, 0x54, 0x0B, 0x54, 0x43};//3.4m
//const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x01, 0x4A, 0x01, 0x54, 0x43};//3.3m
//const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x01, 0x40, 0xF7, 0x54, 0x43};//3.2m
//const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x01, 0x36, 0xED, 0x54, 0x43};//3.1m
//const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x01, 0x2C, 0xE3, 0x54, 0x43};//3m
//const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x01, 0x22, 0xD9, 0x54, 0x43};//2.9m
// ===== 2.70m ~ 2.80m =====
//const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x01, 0x18, 0xCF, 0x54, 0x43 }; // 2.80m
//const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x01, 0x17, 0xCE, 0x54, 0x43 }; // 2.79m
//const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x01, 0x16, 0xCD, 0x54, 0x43 }; // 2.78m
//const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x01, 0x15, 0xCC, 0x54, 0x43 }; // 2.77m
//const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x01, 0x14, 0xCB, 0x54, 0x43 }; // 2.76m
const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x01, 0x13, 0xCA, 0x54, 0x43 }; // 2.75m
//const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x01, 0x12, 0xC9, 0x54, 0x43 }; // 2.74m
//const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x01, 0x11, 0xC8, 0x54, 0x43 }; // 2.73m
//const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x01, 0x10, 0xC7, 0x54, 0x43 }; // 2.72m
//const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x01, 0x0F, 0xC6, 0x54, 0x43 }; // 2.71m
//const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x01, 0x0E, 0xC5, 0x54, 0x43 }; // 2.70m
//const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x00, 0xFA, 0xB0, 0x54, 0x43};//2.5M
//const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x00, 0xE6, 0x9C, 0x54, 0x43};//2.3M
//const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x00, 0xD2, 0x88, 0x54, 0x43};//2.1M
//const byte heightCmd[] = { 0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x00, 0x8C, 0x7E, 0x54, 0x43};//2M

const int MAX_PACKET_LEN = 64;

// 封包暫存與狀態機
void processIncomingByte(byte data);
bool isFallPacket(const byte *buf, int len);
void printPacket(const byte *buf, int len);

void setup() {
  Serial.begin(115200);
  Serial4.begin(115200); // 改為 Serial4
  Serial.println("UART4 ready...");
  delay(2000);

  sendAndReceive(queryCmd, sizeof(queryCmd), "Query Fall Function");
  delay(1000);
  sendAndReceive(enableCmd, sizeof(enableCmd), "Enable Fall Detection");
  delay(1000);
  sendAndReceive(sensitivityCmd, sizeof(sensitivityCmd), "Set Sensitivity = 3");
  delay(1000);
  sendAndReceive(fallTimeCmd, sizeof(fallTimeCmd), "Set fallTimeCmd = 10s");
  delay(1000);
  sendAndReceive(heightCmd, sizeof(heightCmd), "Set Height = 2.7m");

  Serial.println("All commands sent and responses received. Entering continuous receive mode...");
}

void loop() {
  // 持續監聽 UART4 的資料流，交給封包解析器處理
  while (Serial4.available() > 0) {  // 改為 Serial4
    byte data = Serial4.read();  // 改為 Serial4
    processIncomingByte(data);
  }
}

// 傳送封包並立即接收回覆（也用同一套封包解析）
void sendAndReceive(const byte *cmd, int len, const char *desc) {
  Serial.print("Sending: ");
  Serial.println(desc);

  Serial.print("TX Data: ");
  for (int i = 0; i < len; i++) {
    Serial4.write(cmd[i]);  // 改為 Serial4
    Serial.print("0x");
    if (cmd[i] < 0x10) Serial.print("0");
    Serial.print(cmd[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // 等待模組回覆
  unsigned long start = millis();
  Serial.println("RX Data:");
  while (millis() - start < 1000) {  // 最多等 1 秒
    if (Serial4.available() > 0) {  // 改為 Serial4
      byte data = Serial4.read();  // 改為 Serial4
      processIncomingByte(data);
    }
  }
  Serial.println("------------------------------------------------");
}

// 解析串流資料：組封包、每包一行、偵測跌倒封包
void processIncomingByte(byte data) {
  static bool inPacket = false;
  static byte packetBuf[MAX_PACKET_LEN];
  static int packetIndex = 0;
  static byte lastByte = 0;

  if (!inPacket) {
    // 找封包開頭 53 59
    if (lastByte == HEADER1 && data == HEADER2) {
      inPacket = true;
      packetIndex = 0;
      packetBuf[packetIndex++] = HEADER1;
      packetBuf[packetIndex++] = HEADER2;
    }
  } else {
    // 已經在封包內，持續收資料
    if (packetIndex < MAX_PACKET_LEN) {
      packetBuf[packetIndex++] = data;
    }

    // 判斷是否遇到結尾 54 43
    if (packetIndex >= 2 &&
        packetBuf[packetIndex - 2] == TAIL1 &&
        packetBuf[packetIndex - 1] == TAIL2) {
      // 收到完整封包：印出一行
      printPacket(packetBuf, packetIndex);

      // 檢查是不是跌倒狀態封包
      if (isFallPacket(packetBuf, packetIndex)) {
        Serial.println(">>> FALL DETECTED <<<");
      }

      // 重置狀態機
      inPacket = false;
      packetIndex = 0;
    }
  }
  // 記錄上一個位元組，用來判斷 53 59 開頭
  lastByte = data;
}

// 判斷是否為「跌倒狀態」封包：53 59 83 01 00 01 01 xx 54 43
bool isFallPacket(const byte *buf, int len) {
  if (len < 10) return false;

  if (buf[0] != HEADER1 || buf[1] != HEADER2) return false;
  if (buf[2] != 0x83) return false;  // 回報類型
  if (buf[3] != 0x01) return false;  // 跌倒狀態功能碼
  if (buf[4] != 0x00 || buf[5] != 0x01) return false; // 長度 1
  if (buf[len - 2] != TAIL1 || buf[len - 1] != TAIL2) return false;

  // buf[6] = 狀態：0x00 未跌倒，0x01 跌倒
  if (buf[6] == 0x01) {
    return true;
  }
  return false;
}

// 每一包印成一行 HEX
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
