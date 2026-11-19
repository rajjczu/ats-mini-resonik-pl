#include "Common.h"
#include "Themes.h"
#include "Utils.h"
#include "Menu.h"
#include "Draw.h"
#include "Resonik.h"
#include "Weather.h"
#include <HTTPClient.h>
#include <NetworkClientSecure.h>

extern const uint8_t caBundleStart[] asm("_binary_src_cacert_pem_start");
extern const uint8_t caBundleEnd[]   asm("_binary_src_cacert_pem_end");

static const char *WEATHER_URL =
  "https://api.open-meteo.com/v1/forecast?latitude=50.0647&longitude=19.9450"
  "&current=temperature_2m,relative_humidity_2m,pressure_msl,wind_speed_10m,weather_code";

struct WeatherInfo
{
  bool valid = false;
  float tempC = 0.0f;
  int humidity = 0;
  float pressureHpa = 0.0f;
  float windKmh = 0.0f;
  int code = 0;
};

static WeatherInfo wxData;
static uint32_t wxFetched = 0;

static bool extractJsonFloat(const char *json, const char *key, float *out)
{
  const char *p = strstr(json, key);
  if(!p) return false;
  p += strlen(key);
  while(*p && (*p == ' ' || *p == ':')) p++;
  if(!*p) return false;
  *out = strtof(p, nullptr);
  return true;
}

static const char *wmoDesc(int code)
{
  if(code == 0) return "Bezchmurnie";
  if(code <= 3) return "Zachmurzenie";
  if(code == 45 || code == 48) return "Mgla";
  if(code >= 51 && code <= 57) return "Mzawka";
  if(code >= 61 && code <= 67) return "Deszcz";
  if(code >= 71 && code <= 77) return "Snieg";
  if(code >= 80 && code <= 82) return "Przelotny deszcz";
  if(code >= 85 && code <= 86) return "Opady sniegu";
  if(code >= 95) return "Burza";
  return "Pogoda zmienna";
}

bool weatherFetch()
{
  if(getWiFiStatus() < 2) return false;
  if(wxData.valid && millis() - wxFetched < 15UL * 60UL * 1000UL) return true;

  NetworkClientSecure client;
  client.setCACertBundle(caBundleStart, caBundleEnd - caBundleStart);
  HTTPClient http;
  http.setTimeout(6000);
  http.setConnectTimeout(6000);
  if(!http.begin(client, WEATHER_URL)) return false;
  http.setUserAgent(getUserAgent());
  int code = http.GET();
  bool ok = false;
  if(code == HTTP_CODE_OK)
  {
    String body = http.getString();
    const char *cur = strstr(body.c_str(), "\"current\"");
    if(cur)
    {
      float t = 0, h = 0, p = 0, w = 0, c = 0;
      if(extractJsonFloat(cur, "\"temperature_2m\"", &t) &&
         extractJsonFloat(cur, "\"relative_humidity_2m\"", &h) &&
         extractJsonFloat(cur, "\"pressure_msl\"", &p) &&
         extractJsonFloat(cur, "\"wind_speed_10m\"", &w) &&
         extractJsonFloat(cur, "\"weather_code\"", &c))
      {
        wxData.tempC = t;
        wxData.humidity = (int)(h + 0.5f);
        wxData.pressureHpa = p;
        wxData.windKmh = w;
        wxData.code = (int)(c + 0.5f);
        wxData.valid = true;
        wxFetched = millis();
        ok = true;
      }
    }
  }
  http.end();
  return ok;
}

void weatherOpen()
{
  if(getWiFiStatus() >= 2)
  {
    statusShow("Pogoda Krakow", "Pobieranie...", 0);
    drawScreen();
    if(!weatherFetch()) statusShow("Pogoda Krakow", "Brak odpowiedzi");
    else statusShow("");
  }
  currentCmd = CMD_WEATHER;
}

void drawWeatherPage()
{
  const uint16_t bg = 0x041F;
  const uint16_t deep = 0x0108;
  const uint16_t hdr = 0x09C9;
  const uint16_t yellow = 0xFFE0;
  const uint16_t cyan = 0x07FF;
  const uint16_t muted = 0x9CD3;
  (void)bg;

  spr.fillSprite(deep);
  spr.fillRect(0, 0, 320, 28, hdr);
  spr.setTextDatum(ML_DATUM);
  spr.setTextColor(0xFFFF);
  spr.drawString("Pogoda: Krakow", 10, 14, uiFont(false));
  spr.setTextDatum(MR_DATUM);
  spr.setTextColor(cyan);
  spr.drawString("Open-Meteo", 310, 14, uiFont(true));

  if(!wxData.valid)
  {
    spr.setTextDatum(MC_DATUM);
    spr.setTextColor(yellow);
    spr.drawString(getWiFiStatus() < 2 ? "Brak polaczenia Wi-Fi" : "Nie pobrano danych", 160, 82, uiFont(false));
    spr.setTextColor(muted);
    spr.drawString("Wlacz Wi-Fi: System -> Wi-Fi", 160, 108, uiFont(true));
  }
  else
  {
    char buf[48];
    snprintf(buf, sizeof(buf), "%.1f C", wxData.tempC);
    spr.setTextDatum(TL_DATUM);
    spr.setTextColor(yellow);
    spr.drawString(buf, 14, 40, &lgfx::fonts::FreeSansBold18pt7b);

    spr.setTextColor(0xFFFF);
    spr.drawString(wmoDesc(wxData.code), 14, 78, uiFont(false));

    snprintf(buf, sizeof(buf), "Wilgotnosc: %d%%", wxData.humidity);
    spr.setTextColor(cyan);
    spr.drawString(buf, 165, 42, uiFont(true));

    snprintf(buf, sizeof(buf), "Cisnienie: %.0f hPa", wxData.pressureHpa);
    spr.drawString(buf, 165, 62, uiFont(true));

    snprintf(buf, sizeof(buf), "Wiatr: %.1f km/h", wxData.windKmh);
    spr.drawString(buf, 165, 82, uiFont(true));

    const PropData *pd = propagationData();
    spr.drawFastHLine(10, 108, 300, hdr);
    spr.setTextColor(muted);
    if(pd && pd->valid)
    {
      snprintf(buf, sizeof(buf), "Propagacja KF: SFI %d  K-idx %d", pd->sfi, pd->kIndex);
      spr.drawString(buf, 14, 118, uiFont(true));
    }
    else
    {
      spr.drawString("Propagacja KF: otworz Ham -> Propagacja", 14, 118, uiFont(true));
    }
  }

  spr.setTextDatum(BC_DATUM);
  spr.setTextColor(muted);
  spr.drawString("Klik: odswiez   Przytrzymaj: wyjscie", 160, 168, uiFont(true));
  spr.pushSprite(0, 0);
}
