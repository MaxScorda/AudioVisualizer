#ifndef WAVE_GARDEN_H
#define WAVE_GARDEN_H
#ifndef PI
#define PI 3.14159265f
#endif
// Ottimizzato: meno layer, step X=2, niente full clear ogni punto
static int garden_color = 0;
static float garden_phase = 0;
static int garden_prev_y[6][200]; // main_area_w/2 max ~200
static bool garden_ready = false;

inline void garden_set_color(int c) { garden_color = c % 3; garden_ready = false; }
inline int  garden_get_color() { return garden_color; }

inline void garden_init(LTDC_F746_Discovery &tft, int main_area_w) {
  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);
  garden_phase = 0;
  garden_ready = false;
  memset(garden_prev_y, 0, sizeof(garden_prev_y));
  tft.setTextColor(LTDC_CYAN, LTDC_BLACK);
  tft.setTextSize(1);
  tft.setCursor(4, 14);
  tft.print("WAVE GARDEN");
}

inline void garden_update(float* real, int signalLength, LTDC_F746_Discovery &tft,
                          int main_area_w, float gain) {
  int maxB = signalLength / 2;
  const int layers = 4;          // era 6
  const int step = 2;            // disegna ogni 2 pixel
  garden_phase += 0.12f;

  // clear leggero solo area utile (una volta a frame, rettangolo unico)
  tft.fillRect(0, 26, main_area_w, 240, LTDC_BLACK);

  for (int L = 0; L < layers; L++) {
    int bin = 2 + (L * (maxB - 4)) / layers;
    float amp = real[bin] * gain * 0.002f;
    if (amp > 40) amp = 40;
    int y0 = 55 + L * 42;
    uint16_t c;
    if (garden_color == 1) c = 0x07FF;
    else if (garden_color == 2) c = 0x07E0;
    else c = (uint16_t)((5 << 11) | ((20 + L * 8) << 5) | (31 - L * 4));

    int prevx = 0, prevy = y0;
    int col = 0;
    for (int x = 0; x < main_area_w; x += step) {
      float t = (float)x / 30.0f + garden_phase + L * 0.7f;
      int y = y0 + (int)(sinf(t) * (8 + amp));
      if (y < 28) y = 28;
      if (y > 265) y = 265;
      if (x > 0) tft.drawLine(prevx, prevy, x, y, c);
      prevx = x;
      prevy = y;
      col++;
    }
  }
  garden_ready = true;
}
#endif
