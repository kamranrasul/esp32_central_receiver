//#include <Arduino.h>
#include <TFT_eSPI.h>
#include <time.h>
#ifndef CLOCK_H
#define CLOCK_H

void refresh_clock(TFT_eSPI *tft, tm *timeinfo);
#endif
