#ifndef RADIAL_SPECTRUM_H
#define RADIAL_SPECTRUM_H
#ifndef PI
#define PI 3.14159265f
#endif
static int rad_color = 0;
static int rad_prev_h[48];
static bool rad_inited = false;

inline void radial_set_color(int c) { rad_color = c % 3; rad_inited = false; }
inline int  radial_get_color() { return rad_color; }

static uint16_t rad_col(int i, int n, float h) {
  if (rad_color == 1) return 0x07FF;
  if (rad_color == 2) {
    int v = (int)(h); if (v > 31) v = 31;
    return (uint16_t)((31 << 11) | (v << 5));
  }
  float t = (float)i / (float)n;
  int r = (int)(31 * fabsf(sinf(t * PI)));
  int g = (int)(63 * fabsf(sinf(t * PI + 2)));
  int b = (int)(31 * fabsf(sinf(t * PI + 4)));
  return (uint16_t)((r << 11) | (g << 5) | b);
}

// range bin FFT logaritmico per settore i (40 Hz .. 4 kHz @ 16 kHz)
static void rad_log_range(int i, int n, int maxB, int* s, int* e) {
  const float fmin = 40.0f, fmax = 4000.0f;
  float t0 = (float)i / (float)n;
  float t1 = (float)(i + 1) / (float)n;
  float hz0 = expf(logf(fmin) + (logf(fmax) - logf(fmin)) * t0);
  float hz1 = expf(logf(fmin) + (logf(fmax) - logf(fmin)) * t1);
  *s = (int)(hz0 * 1024.0f / 16000.0f);
  *e = (int)(hz1 * 1024.0f / 16000.0f);
  if (*s < 1) *s = 1;
  if (*e <= *s) *e = *s + 1;
  if (*e > maxB) *e = maxB;
}

inline void radial_init(LTDC_F746_Discovery &tft, int main_area_w) {
  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);
  memset(rad_prev_h, 0, sizeof(rad_prev_h));
  rad_inited = false;
  tft.setTextColor(LTDC_CYAN, LTDC_BLACK);
  tft.setTextSize(1);
  tft.setCursor(4, 14);
  tft.print("RADIAL LOG");
  int cx = main_area_w / 2, cy = 150;
  tft.fillCircle(cx, cy, 8, LTDC_WHITE);
}

inline void radial_update(float* real, int signalLength, LTDC_F746_Discovery &tft,
                          int main_area_w, float gain) {
  int cx = main_area_w / 2, cy = 150;
  const int n = 48;
  int maxB = signalLength / 2;

  for (int i = 0; i < n; i++) {
    int s, e;
    rad_log_range(i, n, maxB, &s, &e);
    float sum = 0; int c = 0;
    for (int k = s; k < e; k++) { sum += real[k]; c++; }
    int h = (int)((c ? sum / c : 0) * gain * 0.002f);
    if (h > 90) h = 90;
    if (h < 0) h = 0;

    float ang = -PI / 2 + (float)i / (float)n * 2 * PI;
    float ca = cosf(ang), sa = sinf(ang);
    int x0 = cx + (int)(ca * 20);
    int y0 = cy + (int)(sa * 20);

    if (rad_inited && rad_prev_h[i] > 0) {
      int xo = cx + (int)(ca * (20 + rad_prev_h[i]));
      int yo = cy + (int)(sa * (20 + rad_prev_h[i]));
      tft.drawLine(x0, y0, xo, yo, LTDC_BLACK);
    }
    if (h > 0) {
      int x1 = cx + (int)(ca * (20 + h));
      int y1 = cy + (int)(sa * (20 + h));
      tft.drawLine(x0, y0, x1, y1, rad_col(i, n, (float)h));
    }
    rad_prev_h[i] = h;
  }
  rad_inited = true;
  tft.fillCircle(cx, cy, 8, LTDC_WHITE);
}
#endif
