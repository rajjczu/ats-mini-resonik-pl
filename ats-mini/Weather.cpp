#include "Common.h"
#include "Themes.h"
#include "Utils.h"
#include "Menu.h"
#include "Draw.h"
#include "Resonik.h"
#include "Weather.h"
#include <HTTPClient.h>
#include <NetworkClientSecure.h>

struct CityCoord
{
  const char *name;
  const char *lat;
  const char *lon;
};

static const CityCoord cities[] =
{
  { "Krakow",     "50.0647", "19.9450" },
  { "Zakopane",   "49.2992", "19.9496" },
  { "Tarnow",     "50.0121", "20.9858" },
  { "Nowy Sacz",  "49.6218", "20.6970" },
  { "Katowice",   "50.2649", "19.0238" },
  { "Warszawa",   "52.2297", "21.0122" },
  { "Wroclaw",    "51.1079", "17.0385" },
  { "Poznan",     "52.4064", "16.9252" },
  { "Gdansk",     "54.3520", "18.6466" },
  { "Rzeszow",    "50.0412", "21.9991" },
  { "Lodz",       "51.7592", "19.4560" },
  { "Szczecin",   "53.4285", "14.5528" },
};

static int8_t cityIdx = 0;

struct WeatherInfo
{
  bool valid = false;
  int8_t city = -1;
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
  if(wxData.valid && wxData.city == cityIdx && millis() - wxFetched < 15UL * 60UL * 1000UL) return true;

  char url[192];
  snprintf(url, sizeof(url),
           "https://api.open-meteo.com/v1/forecast?latitude=%s&longitude=%s"
           "&current=temperature_2m,relative_humidity_2m,pressure_msl,wind_speed_10m,weather_code",
           cities[cityIdx].lat, cities[cityIdx].lon);

  NetworkClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(6000);
  http.setConnectTimeout(6000);
  if(!http.begin(client, url)) return false;
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
        wxData.city = cityIdx;
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
    char title[32];
    snprintf(title, sizeof(title), "Pogoda: %s", cities[cityIdx].name);
    statusShow(title, "Pobieranie...", 0);
    drawScreen();
    if(!weatherFetch()) statusShow(title, "Brak odpowiedzi");
    else statusShow("");
  }
  currentCmd = CMD_WEATHER;
}

void weatherTurn(int16_t enc)
{
  const int n = (int)(sizeof(cities) / sizeof(cities[0]));
  int next = (int)cityIdx + (enc > 0 ? 1 : -1);
  if(next < 0) next = n - 1;
  if(next >= n) next = 0;
  cityIdx = (int8_t)next;
}

void drawWeatherPage()
{
  const uint16_t deep = 0x0108;
  const uint16_t hdr = 0x09C9;
  const uint16_t yellow = 0xFFE0;
  const uint16_t cyan = 0x07FF;
  const uint16_t muted = 0x9CD3;

  spr.fillSprite(deep);
  spr.fillRect(0, 0, 320, 28, hdr);
  spr.setTextDatum(ML_DATUM);
  spr.setTextColor(0xFFFF);
  char hbuf[32];
  snprintf(hbuf, sizeof(hbuf), "Pogoda: %s", cities[cityIdx].name);
  spr.drawString(hbuf, 10, 14, uiFont(false));
  spr.setTextDatum(MR_DATUM);
  spr.setTextColor(cyan);
  spr.drawString("Open-Meteo", 310, 14, uiFont(true));

  if(!wxData.valid || wxData.city != cityIdx)
  {
    spr.setTextDatum(MC_DATUM);
    spr.setTextColor(yellow);
    if(getWiFiStatus() < 2)
    {
      spr.drawString("Brak polaczenia Wi-Fi", 160, 82, uiFont(false));
      spr.setTextColor(muted);
      spr.drawString("Wlacz Wi-Fi: System -> Wi-Fi", 160, 108, uiFont(true));
    }
    else
    {
      spr.drawString(cities[cityIdx].name, 160, 76, uiFont(false));
      spr.setTextColor(muted);
      spr.drawString("Kliknij galke, aby pobrac pogode", 160, 106, uiFont(true));
    }
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
  spr.drawString("Obrot: miasto  Klik: pobierz  Press: wyjscie", 160, 168, uiFont(true));
  spr.pushSprite(0, 0);
}
