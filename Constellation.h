#ifndef CONSTELLATION_H
#define CONSTELLATION_H
#ifndef PI
#define PI 3.14159265f
#endif
#define CST_N 36
static float cst_ang[CST_N];
static int cst_color = 0;
inline void const_set_color(int c) { cst_color = c % 3; }
inline int  const_get_color() { return cst_color; }

inline void const_init(LTDC_F746_Discovery &tft, int main_area_w) {
  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);
  for (int i = 0; i < CST_N; i++) cst_ang[i] = (float)i / CST_N * 2 * PI;
  tft.setTextColor(LTDC_CYAN, LTDC_BLACK);
  tft.setTextSize(1);
  tft.setCursor(4, 14);
  tft.print("CONSTELLATION");
}

inline void const_update(float* real, int signalLength, LTDC_F746_Discovery &tft,
                         int main_area_w, float gain) {
  int maxB = signalLength / 2;
  int cx = main_area_w / 2, cy = 150;
  tft.fillRect(0, 26, main_area_w, 240, LTDC_BLACK);
  for (int i = 0; i < CST_N; i++) {
    int bin = 1 + (i * (maxB - 2)) / CST_N;
    float mag = real[bin] * gain * 0.0015f;
    if (mag > 100) mag = 100;
    cst_ang[i] += 0.02f + mag * 0.0003f;
    if (cst_ang[i] > 2 * PI) cst_ang[i] -= 2 * PI;
    int rad = 20 + (int)mag;
    int x = cx + (int)(cosf(cst_ang[i]) * rad);
    int y = cy + (int)(sinf(cst_ang[i]) * rad);
    uint16_t c = (cst_color == 1) ? 0x07FF : (cst_color == 2) ? 0xFFE0 :
      (uint16_t)(((i * 3) << 11) | ((63 - i) << 5) | (i & 31));
    if (x >= 0 && x < main_area_w && y >= 26 && y < 266)
      tft.fillCircle(x, y, 2, c);
  }
}
#endif
