// =============================================================================
// VuNeedle.h — VU a lancette L/R
// Arco statico in init. Solo linea ago: erase spesso + draw.
// =============================================================================

#ifndef VU_NEEDLE_H
#define VU_NEEDLE_H

#ifndef PI
#define PI 3.14159265f
#endif

static float needle_l = 0, needle_r = 0;
static int gauge_cx[2], gauge_cy[2], gauge_r;
static int prev_nx[2] = {0, 0}, prev_ny[2] = {0, 0};
static bool prev_valid[2] = {false, false};

static float level_from_samples(float* s, int n, float gain) {
  double acc = 0;
  float peak = 0;
  for (int i = 0; i < n; i++) {
    float a = s[i];
    if (a < 0) a = -a;
    acc += (double)a * (double)a;
    if (a > peak) peak = a;
  }
  float rms = (float)sqrt(acc / (double)n);
  float v = (rms * 0.4f + peak * 0.6f) * gain * 0.00035f;
  if (v < 0.06f) v = 0;
  if (v > 1.0f) v = 1.0f;
  return v;
}

static void tip(int cx, int cy, int r, float v, int* nx, int* ny) {
  if (v < 0) v = 0;
  if (v > 1) v = 1;
  float ang = (1.0f - v) * PI;
  *nx = cx + (int)(cosf(ang) * (float)(r - 18));
  *ny = cy - (int)(sinf(ang) * (float)(r - 18));
}

// Linea "spessa" (3 tratti) per coprire bene in cancellazione
static void line_thick(LTDC_F746_Discovery &tft, int x0, int y0, int x1, int y1, uint16_t col) {
  tft.drawLine(x0, y0, x1, y1, col);
  tft.drawLine(x0 + 1, y0, x1 + 1, y1, col);
  tft.drawLine(x0, y0 + 1, x1, y1 + 1, col);
}

static void draw_arc_static(LTDC_F746_Discovery &tft, int cx, int cy, int r) {
  for (int a = 0; a <= 180; a += 2) {
    float rad = a * PI / 180.0f;
    float pos = 1.0f - (a / 180.0f);
    uint16_t c;
    if (pos < 0.55f)      c = 0x07E0;
    else if (pos < 0.78f) c = 0xFFE0;
    else                  c = 0xF800;
    tft.drawPixel(cx + (int)(cosf(rad) * r),       cy - (int)(sinf(rad) * r), c);
    tft.drawPixel(cx + (int)(cosf(rad) * (r - 1)), cy - (int)(sinf(rad) * (r - 1)), c);
  }
  for (int a = 0; a <= 180; a += 45) {
    float rad = a * PI / 180.0f;
    tft.drawLine(
      cx + (int)(cosf(rad) * (r - 12)), cy - (int)(sinf(rad) * (r - 12)),
      cx + (int)(cosf(rad) * r),        cy - (int)(sinf(rad) * r), LTDC_WHITE);
  }
  tft.setTextColor(LTDC_WHITE, LTDC_BLACK);
  tft.setTextSize(1);
  tft.setCursor(cx - r - 2, cy - 4);
  tft.print("0");
  tft.setCursor(cx + r - 10, cy - 4);
  tft.print("8");
}

static void draw_needle_only(LTDC_F746_Discovery &tft, int ch, float value) {
  int cx = gauge_cx[ch];
  int cy = gauge_cy[ch];
  int r  = gauge_r;
  if (value < 0.02f) value = 0;

  int nx, ny;
  tip(cx, cy, r, value, &nx, &ny);

  // Cancella vecchia lancetta (tratto spesso nero)
  if (prev_valid[ch]) {
    line_thick(tft, cx, cy, prev_nx[ch], prev_ny[ch], LTDC_BLACK);
  }

  // Nuova lancetta (una linea bianca)
  tft.drawLine(cx, cy, nx, ny, LTDC_WHITE);

  // Perno
  tft.fillCircle(cx, cy, 4, LTDC_RED);
  tft.fillCircle(cx, cy, 2, LTDC_WHITE);

  prev_nx[ch] = nx;
  prev_ny[ch] = ny;
  prev_valid[ch] = true;
}

inline void vu_needle_init(LTDC_F746_Discovery &tft, int main_area_w) {
  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);
  needle_l = needle_r = 0;
  prev_valid[0] = prev_valid[1] = false;

  gauge_r = 78;
  gauge_cy[0] = gauge_cy[1] = 170;
  gauge_cx[0] = main_area_w / 4;
  gauge_cx[1] = 3 * main_area_w / 4;

  tft.setTextColor(LTDC_CYAN, LTDC_BLACK);
  tft.setTextSize(1);
  tft.setCursor(8, 14);
  tft.print("VU NEEDLE");

  for (int ch = 0; ch < 2; ch++) {
    draw_arc_static(tft, gauge_cx[ch], gauge_cy[ch], gauge_r);
    tft.setTextColor(LTDC_WHITE, LTDC_BLACK);
    tft.setTextSize(2);
    tft.setCursor(gauge_cx[ch] - 6, gauge_cy[ch] + 12);
    tft.print(ch == 0 ? "L" : "R");
    draw_needle_only(tft, ch, 0);
  }
}

inline void vu_needle_update(float* left, float* right, int nSamples,
                             LTDC_F746_Discovery &tft, int main_area_w, float gain) {
  (void)main_area_w;
  float el = level_from_samples(left, nSamples, gain);
  float er = level_from_samples(right, nSamples, gain);

  if (el < 0.05f) needle_l *= 0.40f;
  else            needle_l = needle_l * 0.55f + el * 0.45f;
  if (er < 0.05f) needle_r *= 0.40f;
  else            needle_r = needle_r * 0.55f + er * 0.45f;

  if (needle_l < 0.02f) needle_l = 0;
  if (needle_r < 0.02f) needle_r = 0;

  draw_needle_only(tft, 0, needle_l);
  draw_needle_only(tft, 1, needle_r);
}

#endif
