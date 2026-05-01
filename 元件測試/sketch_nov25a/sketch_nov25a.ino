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

void setup() {
    Amg.begin();  // 初始化 AMG8833 感應器
    Serial.begin(115200);  // 設定序列埠波特率為9600
    Serial.println("======== BMS26M833 8*8 pixels Text ========");  // 顯示初始化信息
}

void loop() {
    Amg.readPixels(TempMat);  // 讀取 8x8 溫度像素矩陣
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
        if (TempMat[x - 1] > temp-1) {  // 假設人的體溫高於環境溫度
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
