#include <time.h>
#include "clock.h"
#include <TFT_eSPI.h>

void refresh_clock(TFT_eSPI *tft, tm *timeinfo)
{
  //See documentation for ezTime options: https://github.com/ropg/ezTime
  tft->loadFont("NotoSansBold20");
  // See available colors at https://github.com/Bodmer/TFT_eSPI/blob/master/TFT_eSPI.h
  tft->setTextColor(TFT_WHITE, TFT_BLACK);
  //Clear the date area
  tft->fillRect(205, 195, 105, 50, TFT_BLACK);

  // date printing
  getLocalTime(timeinfo);

  String year,
         month,
         day;

  year  = String(timeinfo->tm_year + 1900);
  month = ((timeinfo->tm_mon + 1) < 10) ? ("0" + String(timeinfo->tm_mon + 1)) : (String(timeinfo->tm_mon + 1));
  day   = ((timeinfo->tm_mday)    < 10) ? ("0" + String(timeinfo->tm_mday))    : String(timeinfo->tm_mday);
  
  String pDate = year + "-" + month + "-" +  day;

  String hour,
         minute,
         second;

  hour   = (timeinfo->tm_hour < 10) ? ("0" + String(timeinfo->tm_hour)) : String(timeinfo->tm_hour);
  minute = (timeinfo->tm_min  < 10) ? ("0" + String(timeinfo->tm_min)) : String(timeinfo->tm_min);
  second = (timeinfo->tm_sec  < 10) ? ("0" + String(timeinfo->tm_sec)) : String(timeinfo->tm_sec);

  String pTime = hour + ":" + minute + ":" + second;

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