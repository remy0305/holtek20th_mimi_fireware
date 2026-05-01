#ifndef _BMC81M001_H__
#define _BMC81M001_H__

//*********************************************//
//****************** Header *******************//
//*********************************************//
#include "TCP.h"
#include "BMC81M001.h"

//*********************************************//
//*********** wifi information ****************//
//*********************************************//

/*** 链接的wifi的账户密码*/
#define WIFI_SSID "A54"//热点名称
#define WIFI_PASS "remy2271"//热点密码

/*** 链接的TCP IP及端口信息*/
#define IP "10.81.63.61"  //TCP服务器 IP地址
#define IP_Port 8000  //TCP服务器 端口

//*********************************************//
//************* IO_Port Define ***************//
//*********************************************//
int LED = 13;                         // LED port 
//*********************************************//
//************* Variable Define ***************//
//*********************************************//
#define DEB_CNT     50                //50ms
#define RES_MAX_LENGTH 200            //max buffer length

char  SerialBuff[RES_MAX_LENGTH];   //serial buffer
char  data[30];                       //key data buffer
int   resLen;                         //serial buffer use length
int   nKeyBuf;                        // KEY data buffer
String DATA_BUF;                      //
String tcpBuff;
#endif
