/*****************************************************************
File:         readTemperaturePixels.ino
Description:  重複獲取熱敏電阻溫度（單位：℃），
              並通過 I2C 獲取 8*8 溫度像素矩陣（單位：℃）。
              然後將這些數據顯示在序列埠上。
              增加了基於溫度值來偵測是否有人。
******************************************************************/

#include <BMS26M833.h>  // 引入 AMG8833 的庫
BMS26M833 Amg;  // 創建 AMG8833 物件
float temp;  // 熱敏電阻的溫度
float TempMat[64];  // 8x8 溫度矩陣 (共64個像素點)

// 定義跌倒監控封包
#define HEADER1 0x53
#define HEADER2 0x59
#define TAIL1   0x54
#define TAIL2   0x43

// 查詢跌倒監控功能開關
const byte queryCmd[]  = {0x53, 0x59, 0x83, 0x80, 0x00, 0x01, 0x0F, 0xBF, 0x54, 0x43};
// 開啟跌倒模式
const byte enableCmd[] = {0x53, 0x59, 0x83, 0x00, 0x00, 0x01, 0x01, 0x31, 0x54, 0x43};
// 設定靈敏度（最高 3）
const byte sensitivityCmd[]  = {0x53, 0x59, 0x83, 0x0D, 0x00, 0x01, 0x03, 0x40, 0x54, 0x43};
// 設定跌倒判定時間（10 秒）
const byte fallTimeCmd[] = {0x53, 0x59, 0x83, 0x0C, 0x00, 0x04, 0x00, 0x00, 0x00, 0x0A, 0x49, 0x54, 0x43};
// 設定安裝高度（2.7 公尺 = 270 cm）
const byte heightCmd[] = {0x53, 0x59, 0x06, 0x02, 0x00, 0x02, 0x01, 0x0E, 0xC5, 0x54, 0x43};

const int MAX_PACKET_LEN = 64;

// 封包暫存與狀態機
void processIncomingByte(byte data);
bool isFallPacket(const byte *buf, int len);
void printPacket(const byte *buf, int len);

// 跌倒偵測標記
bool fallDetected = false;

void setup() {
  Serial.begin(115200);
  Serial4.begin(115200); // 改為 Serial4
  Serial.println("UART4 ready...");
  delay(2000);

  // 發送跌倒相關命令
  sendAndReceive(queryCmd, sizeof(queryCmd), "Query Fall Function");
  delay(1000);
  sendAndReceive(enableCmd, sizeof(enableCmd), "Enable Fall Detection");
  delay(1000);
  sendAndReceive(sensitivityCmd, sizeof(sensitivityCmd), "Set Sensitivity = 3");
  delay(1000);
  sendAndReceive(fallTimeCmd, sizeof(fallTimeCmd), "Set fallTimeCmd = 10s");
  delay(1000);
  sendAndReceive(heightCmd, sizeof(heightCmd), "Set Height = 2.7m");

  // 初始化溫度感應器
  Amg.begin();  // 初始化 AMG8833 感應器
  Serial.println("======== BMS26M833 8*8 pixels Text ========");  // 顯示初始化信息
}

void loop() {
  // 如果已經偵測到跌倒，則停止後續操作
  if (fallDetected) {
    Serial.println("跌倒已經偵測，停止操作");
    return; // 停止進一步執行
  }

  // 持續監聽 UART4 的資料流，交給封包解析器處理
  while (Serial4.available() > 0) {  // 改為 Serial4
    byte data = Serial4.read();  // 改為 Serial4
    processIncomingByte(data);
  }

  // 讀取 8x8 溫度像素矩陣
  Amg.readPixels(TempMat);  
  temp = Amg.readThermistorTemp();  // 讀取熱敏電阻的溫度

  Serial.println("\t\t====== Unit:℃ ======");  // 顯示單位
  Serial.print("Thermistor Temp= ");  // 顯示熱敏電阻溫度
  Serial.println(temp);  // 輸出熱敏電阻溫度

  bool personDetected = false;  // 用來判斷是否偵測到人（預設為未偵測到）

  // 輸出 8x8 溫度像素矩陣，並檢查是否有人的存在
  for (int x = 1; x <= 64; x++) {
    Serial.print(TempMat[x - 1], 1);  // 輸出每個像素的溫度值，保留1位小數
    Serial.print(",  ");
    
    // 如果某個像素的溫度超過閾值，則判定為偵測到人
    if (TempMat[x - 1] > temp - 1) {  // 假設人的體溫高於環境溫度
      personDetected = true;  // 設置標記為偵測到人
    }

    // 每8個像素換行顯示
    if (x % 8 == 0) {
      Serial.println();
    }
  }

  // 偵測到人時，顯示人所在區域的溫度，四捨五入到整數
  if (personDetected) {
    int roundedTemp = round(temp);  // 四捨五入為整數
    Serial.print("Person detected! Room Temp: ");
    Serial.println(roundedTemp);  // 顯示室溫，例如 27
  } else {
    Serial.println("No person detected.");  // 未偵測到人
  }

  Serial.println();  // 輸出空行，便於閱讀
  delay(500);  // 延遲500毫秒再進行下一次讀取
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
        fallDetected = true;  // 設置跌倒偵測旗標為 true
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
