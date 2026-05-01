static void processRadarByte(byte data) {
  static bool inPacket = false;
  static byte buf[MAX_PACKET_LEN];
  static int idx = 0;
  static byte last = 0;

  static bool fallDetected = false;  // 標記是否已經發送過跌倒事件

  if (!inPacket) {
    if (last == HEADER1 && data == HEADER2) {
      inPacket = true;
      idx = 0;
      buf[idx++] = HEADER1;
      buf[idx++] = HEADER2;
    }
  } else {
    if (idx < MAX_PACKET_LEN) {
      buf[idx++] = data;
    } else {
      inPacket = false;
      idx = 0;
    }

    if (idx >= 2 && buf[idx - 2] == TAIL1 && buf[idx - 1] == TAIL2) {

      // 偵測跌倒事件，並且只發送一次
      if (isFallPacket(buf, idx) && !fallDetected) {
        sendToServer("1\r\n");  // 發送 "1" 到伺服器
        fallDetected = true;  // 設置已發送過，避免重複發送
      }

      // 偵測身體移動事件
      if (isBodyMovementPacket(buf, idx)) {
        int movementValue = buf[6];
        unsigned long now = millis();
        if ((now - lastMoveMs > 300) && (movementValue != lastMoveVal)) {
          char msg[32];
          snprintf(msg, sizeof(msg), "[7,%d]\r\n", movementValue);
          sendToServer(msg);
          lastMoveMs = now;
          lastMoveVal = movementValue;
        }
      }

      inPacket = false;
      idx = 0;
    }
  }

  last = data;
}
