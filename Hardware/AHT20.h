#ifndef __AHT20_H
#define __AHT20_H

void AHT20_Init(void);
uint8_t AHT20_ReadData(float *Temperature, float *Humidity);

#endif
