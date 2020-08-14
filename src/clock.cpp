#include <time.h>
#include "clock.h"
#include <TFT_eSPI.h>

void refresh_clock(TFT_eSPI *tft, tm *timeinfo)
{
  //See documentation for ezTime options: https://github.com/ropg/ezTime
  tft->loadFont("NotoSansBold20");
  // See available colors at https://github.com/Bodmer/TFT_eSPI/blob/master/TFT_eSPI.h
  tft->setTextColor(TFT_GREEN, TFT_WHITE);
  //Clear the date area
  tft->fillRect(205, 195, 105, 50, TFT_WHITE);

  // date printing
  getLocalTime(timeinfo);

  String pDate = String(timeinfo->tm_year + 1900) + "-" +
                 String(timeinfo->tm_mon + 1) + "-" +
                 String(timeinfo->tm_mday);
  String pTime = String(timeinfo->tm_hour) + ":" +
                 String(timeinfo->tm_min) + ":" +
                 String(timeinfo->tm_sec);

  int pDateLen = pDate.length(),
      pTimeLen = pTime.length();

  for (int i = 0; i < (pDateLen - pTimeLen); i++)
  {
    pTime = " " + pTime;
  }

  tft->setCursor(210, 200);
  tft->print(pDate);

  tft->setCursor(210, 220);
  tft->print(pTime);
}