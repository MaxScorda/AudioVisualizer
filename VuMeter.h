#ifndef VU_METER_H
#define VU_METER_H

static float vu_l = 0, vu_r = 0;
static float vu_lp = 0, vu_rp = 0;
static int vu_drawn_h[2] = {0, 0};
static int vu_drawn_ph[2] = {0, 0};

inline void vu_meter_init(LTDC_F746_Discovery &tft, int main_area_w) {
  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);
  vu_l = vu_r = vu_lp = vu_rp = 0;
  vu_drawn_h[0] = vu_drawn_h[1] = 0;
  vu_drawn_ph[0] = vu_drawn_ph[1] = 0;
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

  vu_l = vu_l * 0.75f + el * 0.25f;
  vu_r = vu_r * 0.75f + er * 0.25f;
  if (vu_l > vu_lp) vu_lp = vu_l; else vu_lp -= 0.004f;
  if (vu_r > vu_rp) vu_rp = vu_r; else vu_rp -= 0.004f;
  if (vu_lp < 0) vu_lp = 0; if (vu_rp < 0) vu_rp = 0;

  int barW = main_area_w / 2 - 20;
  int maxH = 220;
  int base = 271;

  for (int ch = 0; ch < 2; ch++) {
    int x0 = (ch == 0) ? 15 : (main_area_w / 2 + 5);
    float v = (ch == 0) ? vu_l : vu_r;
    float pk = (ch == 0) ? vu_lp : vu_rp;
    int h = (int)(v * maxH);
    int ph = (int)(pk * maxH);
    int oldH = vu_drawn_h[ch];
    int oldPh = vu_drawn_ph[ch];

    // delta barra
    if (h > oldH) {
      for (int s = oldH; s < h; s++) {
        uint16_t c = (s < maxH * 0.6f) ? 0x07E0 : (s < maxH * 0.85f) ? 0xFFE0 : 0xF800;
        tft.drawFastHLine(x0, base - s, barW, c);
      }
    } else if (h < oldH) {
      tft.fillRect(x0, base - oldH, barW, oldH - h, LTDC_BLACK);
    }
    vu_drawn_h[ch] = h;

    // peak marker
    if (oldPh > 2) tft.fillRect(x0, base - oldPh - 1, barW, 3, LTDC_BLACK);
    // ridisegna barra sotto il vecchio peak se serve
    if (oldPh > h && oldPh > 2) {
      // gia' nero sopra h
    } else if (oldPh <= h && oldPh > 2) {
      for (int s = oldPh - 1; s <= oldPh + 1 && s < h; s++) {
        if (s < 0) continue;
        uint16_t c = (s < maxH * 0.6f) ? 0x07E0 : (s < maxH * 0.85f) ? 0xFFE0 : 0xF800;
        tft.drawFastHLine(x0, base - s, barW, c);
      }
    }
    if (ph > 2) tft.fillRect(x0, base - ph - 1, barW, 3, LTDC_WHITE);
    vu_drawn_ph[ch] = ph;
  }
}

#endif
