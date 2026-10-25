#ifndef __ESP8266_H
#define __ESP8266_H

#include <stdint.h>

void ESP8266_Init(void);
void ESP8266_SendString(char *str);
void ESP8266_ClearBuffer(void);
void ESP8266_SendCommand(char *cmd, char *reply, uint16_t timeout);
uint8_t ESP8266_ConnectWiFi(char *ssid, char *password);
void ESP8266_ConnectBemfa(char *uid, char *topic);
void ESP8266_SendToCloud(char *topic, char *data);

extern char ESP8266_RxBuffer[256];
extern volatile uint16_t ESP8266_RxIndex;

#endif
