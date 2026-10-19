#ifndef PIXEL_RAIN_H
#define PIXEL_RAIN_H
#define RAIN_COLS 40
static float rain_y[RAIN_COLS];
static float rain_sp[RAIN_COLS];
static int rain_color = 0;
inline void rain_set_color(int c) { rain_color = c % 3; }
inline int  rain_get_color() { return rain_color; }

static int rain_log_bin(int i, int nCols, int maxB) {
  const float fmin = 40.0f, fmax = 4000.0f;
  float t = (float)i / (float)(nCols > 1 ? nCols - 1 : 1);
  float hz = expf(logf(fmin) + (logf(fmax) - logf(fmin)) * t);
  int b = (int)(hz * 1024.0f / 16000.0f);
  if (b < 1) b = 1;
  if (b >= maxB) b = maxB - 1;
  return b;
}

// intensità 0..1 da velocità (lento=scuro, veloce=chiaro)
static uint16_t rain_color_from_speed(float sp, int trail_k, int trail_len) {
  // sp tipico ~1..17
  float bright = (sp - 1.0f) / 16.0f;
  if (bright < 0.12f) bright = 0.12f;
  if (bright > 1.0f) bright = 1.0f;
  // coda più scura verso l'alto
  if (trail_len > 1)
    bright *= (1.0f - 0.65f * (float)trail_k / (float)trail_len);

  int v5 = (int)(bright * 31.0f);
  int v6 = (int)(bright * 63.0f);
  if (v5 < 1) v5 = 1;
  if (v6 < 2) v6 = 2;

  if (rain_color == 1) {
    // cyan: G+B
    return (uint16_t)((0 << 11) | (v6 << 5) | v5);
  }
  if (rain_color == 2) {
    // rosso
    return (uint16_t)((v5 << 11) | (0 << 5) | 0);
  }
  // verde
  return (uint16_t)((0 << 11) | (v6 << 5) | 0);
}

inline void rain_init(LTDC_F746_Discovery &tft, int main_area_w) {
  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);
  for (int i = 0; i < RAIN_COLS; i++) {
    rain_y[i] = (float)(i * 17 % 220);
    rain_sp[i] = 1.5f + (i % 5);
  }
  tft.setTextColor(LTDC_CYAN, LTDC_BLACK);
  tft.setTextSize(1);
  tft.setCursor(4, 14);
  tft.print("PIXEL RAIN LOG");
}

inline void rain_update(float* real, int signalLength, LTDC_F746_Discovery &tft,
                        int main_area_w, float gain) {
  int maxB = signalLength / 2;
  int colW = main_area_w / RAIN_COLS;
  if (colW < 2) colW = 2;
  tft.fillRect(0, 26, main_area_w, 240, LTDC_BLACK);
  for (int i = 0; i < RAIN_COLS; i++) {
    int bin = rain_log_bin(i, RAIN_COLS, maxB);
    float mag = real[bin] * gain * 0.002f;
    if (mag > 20) mag = 20;
    rain_sp[i] = 1.0f + mag * 0.8f;   // più energia → più veloce → più chiaro
    rain_y[i] += rain_sp[i];
    if (rain_y[i] > 240) rain_y[i] = 0;
    int x = i * colW + colW / 2;
    int y = 26 + (int)rain_y[i];
    int len = 4 + (int)mag;
    for (int k = 0; k < len; k++) {
      int yy = y - k * 3;
      if (yy >= 26 && yy < 266) {
        uint16_t c = rain_color_from_speed(rain_sp[i], k, len);
        tft.fillRect(x - 1, yy, colW - 2, 2, c);
      }
    }
  }
}
#endif
