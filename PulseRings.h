#ifndef PULSE_RINGS_H
#define PULSE_RINGS_H
#ifndef PI
#define PI 3.14159265f
#endif
static float pulse_env = 0;
static int pulse_color = 0;
inline void pulse_set_color(int c) { pulse_color = c % 3; }
inline int  pulse_get_color() { return pulse_color; }

static uint16_t pulse_col(int ring) {
  if (pulse_color == 1) return 0x07FF;
  if (pulse_color == 2) return 0xFFE0;
  uint16_t t[] = {0x07E0, 0xFFE0, 0xF800, 0xF81F, 0x07FF};
  return t[ring % 5];
}

inline void pulse_init(LTDC_F746_Discovery &tft, int main_area_w) {
  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);
  pulse_env = 0;
  tft.setTextColor(LTDC_CYAN, LTDC_BLACK);
  tft.setTextSize(1);
  tft.setCursor(4, 14);
  tft.print("PULSE RINGS");
}

inline void pulse_update(float* left, int n, LTDC_F746_Discovery &tft,
                         int main_area_w, float gain) {
  float peak = 0;
  for (int i = 0; i < n; i++) {
    float a = left[i]; if (a < 0) a = -a;
    if (a > peak) peak = a;
  }
  float e = peak * gain * 0.00008f;
  if (e > 1) e = 1;
  pulse_env = pulse_env * 0.7f + e * 0.3f;

  int cx = main_area_w / 2, cy = 150;
  tft.fillRect(0, 26, main_area_w, 240, LTDC_BLACK);
  int rings = 3 + (int)(pulse_env * 5);
  for (int r = 0; r < rings; r++) {
    int rad = 15 + r * 18 + (int)(pulse_env * 25);
    uint16_t c = pulse_col(r);
    // circonferenza approssimata
    for (int a = 0; a < 360; a += 4) {
      float rads = a * PI / 180.0f;
      int x = cx + (int)(cosf(rads) * rad);
      int y = cy + (int)(sinf(rads) * rad);
      if (x >= 0 && x < main_area_w && y >= 26 && y < 266)
        tft.drawPixel(x, y, c);
    }
  }
}
#endif
