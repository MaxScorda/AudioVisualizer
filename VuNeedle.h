#ifndef VU_NEEDLE_H
#define VU_NEEDLE_H

#ifndef PI
#define PI 3.14159265f
#endif

static float needle_l = 0, needle_r = 0;
static int prev_nx[2] = {-1, -1};
static int prev_ny[2] = {-1, -1};
static int gauge_cx[2], gauge_cy[2], gauge_r;

// RMS + peak-ish level 0..1 più reattivo
static float level_from_samples(float* s, int n, float gain) {
  float sum = 0;
  float peak = 0;
  for (int i = 0; i < n; i++) {
    float a = s[i];
    if (a < 0) a = -a;
    sum += a;
    if (a > peak) peak = a;
  }
  float mean = sum / (float)n;
  // combina media e picco, gain generoso
  float v = (mean * 0.4f + peak * 0.6f) * gain * 8.0f;
  if (v > 1.0f) v = 1.0f;
  if (v < 0.0f) v = 0.0f;
  return v;
}

static void draw_gauge_face(LTDC_F746_Discovery &tft, int cx, int cy, int radius, const char* label) {
  for (int a = 0; a <= 180; a += 2) {
    float rad = a * PI / 180.0f;
    int x = cx + (int)(cosf(rad) * radius);
    int y = cy - (int)(sinf(rad) * radius);
    uint16_t c = (a < 100) ? 0x07E0 : (a < 140) ? 0xFFE0 : 0xF800;
    tft.drawPixel(x, y, c);
    int x2 = cx + (int)(cosf(rad) * (radius - 2));
    int y2 = cy - (int)(sinf(rad) * (radius - 2));
    tft.drawPixel(x2, y2, c);
  }
  for (int a = 0; a <= 180; a += 30) {
    float rad = a * PI / 180.0f;
    int x1 = cx + (int)(cosf(rad) * (radius - 8));
    int y1 = cy - (int)(sinf(rad) * (radius - 8));
    int x2 = cx + (int)(cosf(rad) * radius);
    int y2 = cy - (int)(sinf(rad) * radius);
    tft.drawLine(x1, y1, x2, y2, LTDC_WHITE);
  }
  tft.fillCircle(cx, cy, 4, LTDC_RED);
  tft.setTextColor(LTDC_WHITE, LTDC_BLACK);
  tft.setTextSize(2);
  tft.setCursor(cx - 6, cy + 12);
  tft.print(label);
}

inline void vu_needle_init(LTDC_F746_Discovery &tft, int main_area_w) {
  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);
  needle_l = needle_r = 0;
  prev_nx[0] = prev_ny[0] = prev_nx[1] = prev_ny[1] = -1;

  gauge_r = 75;
  gauge_cy[0] = gauge_cy[1] = 165;
  gauge_cx[0] = main_area_w / 4;
  gauge_cx[1] = 3 * main_area_w / 4;

  draw_gauge_face(tft, gauge_cx[0], gauge_cy[0], gauge_r, "L");
  draw_gauge_face(tft, gauge_cx[1], gauge_cy[1], gauge_r, "R");
}

static void update_one_needle(LTDC_F746_Discovery &tft, int ch, float value) {
  int cx = gauge_cx[ch];
  int cy = gauge_cy[ch];
  int radius = gauge_r;
  if (value < 0) value = 0;
  if (value > 1) value = 1;

  if (prev_nx[ch] >= 0) {
    tft.drawLine(cx, cy, prev_nx[ch], prev_ny[ch], LTDC_BLACK);
    tft.fillCircle(cx, cy, 4, LTDC_RED);
  }

  float ang = (1.0f - value) * 180.0f;
  float rad = ang * PI / 180.0f;
  int nx = cx + (int)(cosf(rad) * (radius - 12));
  int ny = cy - (int)(sinf(rad) * (radius - 12));
  tft.drawLine(cx, cy, nx, ny, LTDC_WHITE);
  tft.fillCircle(cx, cy, 4, LTDC_RED);

  prev_nx[ch] = nx;
  prev_ny[ch] = ny;
}

inline void vu_needle_update(float* left, float* right, int nSamples,
                             LTDC_F746_Discovery &tft, int main_area_w, float gain) {
  (void)main_area_w;

  float el = level_from_samples(left, nSamples, gain);
  float er = level_from_samples(right, nSamples, gain);

  // smoothing leggero: reattivo ma non tremolio
  needle_l = needle_l * 0.70f + el * 0.30f;
  needle_r = needle_r * 0.70f + er * 0.30f;

  update_one_needle(tft, 0, needle_l);
  update_one_needle(tft, 1, needle_r);
}

#endif
