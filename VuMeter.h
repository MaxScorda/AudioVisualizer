#ifndef VU_METER_H
#define VU_METER_H

static float vu_l = 0, vu_r = 0;
static float vu_lp = 0, vu_rp = 0;

inline void vu_meter_init(LTDC_F746_Discovery &tft, int main_area_w) {
  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);
  vu_l = vu_r = vu_lp = vu_rp = 0;
  tft.setTextColor(LTDC_WHITE, LTDC_BLACK);
  tft.setTextSize(2);
  int barW = main_area_w / 2 - 20;
  tft.setCursor(15 + barW / 2 - 6, 14);
  tft.print("L");
  tft.setCursor(main_area_w / 2 + 5 + barW / 2 - 6, 14);
  tft.print("R");
}

inline void vu_meter_update(float* left, float* right, int nSamples, LTDC_F746_Discovery &tft,
                            int main_area_w, float gain) {
  float el = 0, er = 0;
  for (int i = 0; i < nSamples; i++) {
    float a = left[i]; if (a < 0) a = -a; el += a;
    float b = right[i]; if (b < 0) b = -b; er += b;
  }
  el = (el / nSamples) * gain;
  er = (er / nSamples) * gain;
  if (el > 1) el = 1; if (er > 1) er = 1;

  vu_l = vu_l * 0.70f + el * 0.30f;
  vu_r = vu_r * 0.70f + er * 0.30f;
  if (vu_l > vu_lp) vu_lp = vu_l; else vu_lp -= 0.006f;
  if (vu_r > vu_rp) vu_rp = vu_r; else vu_rp -= 0.006f;
  if (vu_lp < 0) vu_lp = 0; if (vu_rp < 0) vu_rp = 0;

  tft.fillRect(0, 40, main_area_w, 232, LTDC_BLACK);

  int barW = main_area_w / 2 - 20;
  int maxH = 220;
  int base = 270;

  for (int ch = 0; ch < 2; ch++) {
    int x0 = (ch == 0) ? 15 : (main_area_w / 2 + 5);
    float v = (ch == 0) ? vu_l : vu_r;
    float pk = (ch == 0) ? vu_lp : vu_rp;
    int h = (int)(v * maxH);
    int ph = (int)(pk * maxH);
    // rettangolo pieno (no linee frammentate)
    int gH = (int)(maxH * 0.6f);
    int yH = (int)(maxH * 0.85f);
    if (h > 0) {
      int h1 = (h < gH) ? h : gH;
      tft.fillRect(x0, base - h1, barW, h1, 0x07E0);
      if (h > gH) {
        int h2 = (h < yH) ? (h - gH) : (yH - gH);
        tft.fillRect(x0, base - gH - h2, barW, h2, 0xFFE0);
      }
      if (h > yH) {
        tft.fillRect(x0, base - h, barW, h - yH, 0xF800);
      }
    }
    if (ph > 2) tft.fillRect(x0, base - ph - 1, barW, 3, LTDC_WHITE);
  }
}

#endif
