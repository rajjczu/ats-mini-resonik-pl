#ifndef WEATHER_H
#define WEATHER_H

#include <stdint.h>

void weatherOpen();
void weatherTurn(int16_t enc);
bool weatherFetch();
void drawWeatherPage();

#endif // WEATHER_H
