#ifndef ONDA_H
#define ONDA_H

// 0 = Verde (default), 1 = Ciano, 2 = Giallo
static const uint16_t onda_colors[3] = {
  0x07E0, // verde
  0x07FF, // ciano
  0xFFE0  // giallo
};

inline void onda_init(LTDC_F746_Discovery &tft, int main_area_w) {
  tft.fillRect(0, 11, main_area_w, 272 - 11, LTDC_BLACK);
  // Linea di zero continua
  tft.drawFastHLine(0, 140, main_area_w, LTDC_WHITE);
}

inline void onda_update(float* real, int signalLength, LTDC_F746_Discovery &tft, int main_area_w, float gain, int color_mode) {
  if (color_mode < 0) color_mode = 0;
  if (color_mode > 2) color_mode = 2;
  uint16_t color = onda_colors[color_mode];

  const int y0 = 11;
  const int y1 = 271;
  const int centerY = 140;

  // Cancella area
  tft.fillRect(0, y0, main_area_w, 272 - y0, LTDC_BLACK);

  // Linea centrale continua (bianca) — un solo HLine, costo trascurabile
  tft.drawFastHLine(0, centerY, main_area_w, LTDC_WHITE);

  int prev_y = centerY;
  for (int x = 0; x < main_area_w; x++) {
    int idx = (signalLength <= 1) ? 0 : (x * (signalLength - 1)) / (main_area_w - 1);
    int y = centerY - (int)(real[idx] * gain);
    if (y < y0) y = y0;
    if (y > y1) y = y1;

    if (x > 0) tft.drawLine(x - 1, prev_y, x, y, color);
    else       tft.drawPixel(x, y, color);
    prev_y = y;
  }
}

#endif
