// =============================================================================
// VuNeedle.h — VU a lancette (L / R)
// =============================================================================
// Clear ampio dell'area ago + una sola linea bianca (no doppio tratto = meno "V").
// Scala sensibilità bilanciata (GAIN tipico 0.5 .. 5).
// =============================================================================

#ifndef VU_NEEDLE_H
#define VU_NEEDLE_H

#ifndef PI
#define PI 3.14159265f
#endif

static float needle_l = 0, needle_r = 0;
static int gauge_cx[2], gauge_cy[2], gauge_r;

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
  // gain ~1..15 come da uso reale; *0.00035 tiene headroom
  float v = (rms * 0.4f + peak * 0.6f) * gain * 0.00035f;
  if (v < 0.07f) v = 0;
  if (v > 1.0f) v = 1.0f;
  return v;
}

static void tip(int cx, int cy, int r, float v, int* nx, int* ny) {
  if (v < 0) v = 0;
  if (v > 1) v = 1;
  float ang = (1.0f - v) * PI;
  *nx = cx + (int)(cosf(ang) * (r - 20));
  *ny = cy - (int)(sinf(ang) * (r - 20));
}

static void draw_arc(LTDC_F746_Discovery &tft, int cx, int cy, int r) {
  for (int a = 0; a <= 180; a += 2) {
    float rad = a * PI / 180.0f;
    uint16_t c = (a < 100) ? 0x07E0 : (a < 140) ? 0xFFE0 : 0xF800;
    tft.drawPixel(cx + (int)(cosf(rad) * r), cy - (int)(sinf(rad) * r), c);
    tft.drawPixel(cx + (int)(cosf(rad) * (r - 1)), cy - (int)(sinf(rad) * (r - 1)), c);
  }
  for (int a = 0; a <= 180; a += 45) {
    float rad = a * PI / 180.0f;
    tft.drawLine(
      cx + (int)(cosf(rad) * (r - 12)), cy - (int)(sinf(rad) * (r - 12)),
      cx + (int)(cosf(rad) * r),        cy - (int)(sinf(rad) * r), LTDC_WHITE);
  }
  tft.setTextColor(LTDC_YELLOW, LTDC_BLACK);
  tft.setTextSize(1);
  tft.setCursor(cx - r - 2, cy - 4);
  tft.print("0");
}

static void draw_one(LTDC_F746_Discovery &tft, int ch, float value) {
  int cx = gauge_cx[ch];
  int cy = gauge_cy[ch];
  int r  = gauge_r;

  // Clear PIU' ampio dell'arco (copre tutta la corsa dell'ago)
  tft.fillRect(cx - r - 4, cy - r - 4, 2 * r + 8, r + 12, LTDC_BLACK);

  draw_arc(tft, cx, cy, r);

  tft.setTextColor(LTDC_WHITE, LTDC_BLACK);
  tft.setTextSize(2);
  tft.setCursor(cx - 6, cy + 12);
  tft.print(ch == 0 ? "L" : "R");

  if (value < 0.02f) value = 0;

  int nx, ny;
  tip(cx, cy, r, value, &nx, &ny);

  // UNA sola linea (il secondo tratto creava l'effetto a V)
  tft.drawLine(cx, cy, nx, ny, LTDC_WHITE);
  tft.fillCircle(cx, cy, 5, LTDC_RED);
  tft.fillCircle(cx, cy, 2, LTDC_WHITE);
}

inline void vu_needle_init(LTDC_F746_Discovery &tft, int main_area_w) {
  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);
  needle_l = needle_r = 0;

  gauge_r = 78;
  gauge_cy[0] = gauge_cy[1] = 170;
  gauge_cx[0] = main_area_w / 4;
  gauge_cx[1] = 3 * main_area_w / 4;

  tft.setTextColor(LTDC_CYAN, LTDC_BLACK);
  tft.setTextSize(1);
  tft.setCursor(8, 14);
  tft.print("VU NEEDLE  0=left");

  draw_one(tft, 0, 0);
  draw_one(tft, 1, 0);
}

inline void vu_needle_update(float* left, float* right, int nSamples,
                             LTDC_F746_Discovery &tft, int main_area_w, float gain) {
  (void)main_area_w;

  float el = level_from_samples(left, nSamples, gain);
  float er = level_from_samples(right, nSamples, gain);

  if (el < 0.05f) needle_l *= 0.45f;
  else            needle_l = needle_l * 0.50f + el * 0.50f;
  if (er < 0.05f) needle_r *= 0.45f;
  else            needle_r = needle_r * 0.50f + er * 0.50f;

  if (needle_l < 0.02f) needle_l = 0;
  if (needle_r < 0.02f) needle_r = 0;

  draw_one(tft, 0, needle_l);
  draw_one(tft, 1, needle_r);
}

#endif
