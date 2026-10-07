#ifndef VU_NEEDLE_H
#define VU_NEEDLE_H

#ifndef PI
#define PI 3.14159265f
#endif

static float needle_l = 0, needle_r = 0;
static int gauge_cx[2], gauge_cy[2], gauge_r;
static int main_w_cache = 400;

static float level_from_samples(float* s, int n, float gain) {
  float sum = 0, peak = 0;
  for (int i = 0; i < n; i++) {
    float a = s[i];
    if (a < 0) a = -a;
    sum += a;
    if (a > peak) peak = a;
  }
  float mean = sum / (float)n;
  float v = (mean * 0.3f + peak * 0.7f) * gain * 20.0f;
  // noise floor: sotto soglia = silenzio → lancetta a 0
  if (v < 0.04f) v = 0;
  if (v > 1.0f) v = 1.0f;
  return v;
}

static void draw_gauge_face(LTDC_F746_Discovery &tft, int cx, int cy, int radius, const char* label) {
  for (int a = 0; a <= 180; a += 2) {
    float rad = a * PI / 180.0f;
    uint16_t c = (a < 100) ? 0x07E0 : (a < 140) ? 0xFFE0 : 0xF800;
    int x = cx + (int)(cosf(rad) * radius);
    int y = cy - (int)(sinf(rad) * radius);
    tft.drawPixel(x, y, c);
    tft.drawPixel(cx + (int)(cosf(rad) * (radius - 2)),
                  cy - (int)(sinf(rad) * (radius - 2)), c);
  }
  for (int a = 0; a <= 180; a += 30) {
    float rad = a * PI / 180.0f;
    tft.drawLine(
      cx + (int)(cosf(rad) * (radius - 8)), cy - (int)(sinf(rad) * (radius - 8)),
      cx + (int)(cosf(rad) * radius), cy - (int)(sinf(rad) * radius), LTDC_WHITE);
  }
  tft.fillCircle(cx, cy, 5, LTDC_RED);
  tft.setTextColor(LTDC_WHITE, LTDC_BLACK);
  tft.setTextSize(2);
  tft.setCursor(cx - 6, cy + 14);
  tft.print(label);
}

static void draw_needle_only(LTDC_F746_Discovery &tft, int cx, int cy, int radius, float value) {
  if (value < 0) value = 0;
  if (value > 1) value = 1;
  // 0 = sinistra (180°), 1 = destra (0°)
  float ang = (1.0f - value) * 180.0f;
  float rad = ang * PI / 180.0f;
  int nx = cx + (int)(cosf(rad) * (radius - 12));
  int ny = cy - (int)(sinf(rad) * (radius - 12));
  tft.drawLine(cx, cy, nx, ny, LTDC_WHITE);
  tft.drawLine(cx + 1, cy, nx + 1, ny, LTDC_WHITE);
  tft.fillCircle(cx, cy, 5, LTDC_RED);
}

inline void vu_needle_init(LTDC_F746_Discovery &tft, int main_area_w) {
  main_w_cache = main_area_w;
  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);
  needle_l = needle_r = 0;
  gauge_r = 75;
  gauge_cy[0] = gauge_cy[1] = 165;
  gauge_cx[0] = main_area_w / 4;
  gauge_cx[1] = 3 * main_area_w / 4;
  draw_gauge_face(tft, gauge_cx[0], gauge_cy[0], gauge_r, "L");
  draw_gauge_face(tft, gauge_cx[1], gauge_cy[1], gauge_r, "R");
  // lancette a riposo (zero = sinistra)
  draw_needle_only(tft, gauge_cx[0], gauge_cy[0], gauge_r, 0);
  draw_needle_only(tft, gauge_cx[1], gauge_cy[1], gauge_r, 0);
}

inline void vu_needle_update(float* left, float* right, int nSamples,
                             LTDC_F746_Discovery &tft, int main_area_w, float gain) {
  (void)main_area_w;
  float el = level_from_samples(left, nSamples, gain);
  float er = level_from_samples(right, nSamples, gain);

  // smoothing: a silenzio torna a 0
  if (el < 0.02f) needle_l *= 0.85f;
  else needle_l = needle_l * 0.70f + el * 0.30f;
  if (er < 0.02f) needle_r *= 0.85f;
  else needle_r = needle_r * 0.70f + er * 0.30f;
  if (needle_l < 0.01f) needle_l = 0;
  if (needle_r < 0.01f) needle_r = 0;

  // Ridisegna area gauge (niente doppie lancette residue)
  // cancella interno archi e ridisegna faccia + lancetta
  for (int ch = 0; ch < 2; ch++) {
    int cx = gauge_cx[ch];
    int cy = gauge_cy[ch];
    int r = gauge_r;
    // clear box intorno all'arco
    tft.fillRect(cx - r - 2, cy - r - 2, 2 * r + 4, r + 20, LTDC_BLACK);
    draw_gauge_face(tft, cx, cy, r, (ch == 0) ? "L" : "R");
    draw_needle_only(tft, cx, cy, r, (ch == 0) ? needle_l : needle_r);
  }
}

#endif
