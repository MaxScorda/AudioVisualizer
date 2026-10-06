#ifndef LISSAJOUS_H
#define LISSAJOUS_H

inline void lissajous_init(LTDC_F746_Discovery &tft, int main_area_w) {
  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);
}

inline void lissajous_update(float* left, float* right, int nSamples,
                             LTDC_F746_Discovery &tft, int main_area_w, float gain) {
  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);

  int cx = main_area_w / 2;
  int cy = 140;
  // gain 0.28 ~ scala media; GAIN +/- cambia ampiezza XY
  float scale = gain * 350.0f;
  if (scale < 20) scale = 20;
  if (scale > 2000) scale = 2000;

  tft.drawFastHLine(10, cy, main_area_w - 20, 0x2104);
  tft.drawFastVLine(cx, 20, 240, 0x2104);

  int step = nSamples / 200;
  if (step < 1) step = 1;
  int prevx = cx, prevy = cy;
  for (int i = 0; i < nSamples; i += step) {
    int x = cx + (int)(left[i] * scale / 8000.0f);
    int y = cy - (int)(right[i] * scale / 8000.0f);
    if (x < 1) x = 1; if (x >= main_area_w - 1) x = main_area_w - 2;
    if (y < 12) y = 12; if (y > 270) y = 270;
    if (i > 0) tft.drawLine(prevx, prevy, x, y, 0x07E0);
    prevx = x; prevy = y;
  }
}

#endif
