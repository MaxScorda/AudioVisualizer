#ifndef MIRROR_SPECTRUM_H
#define MIRROR_SPECTRUM_H
static int mir_color = 0;
static int mir_prev[400]; // altezze per colonna
static bool mir_ready = false;

inline void mirror_set_color(int c) { mir_color = c % 3; mir_ready = false; }
inline int  mirror_get_color() { return mir_color; }

static uint16_t mir_col(int x, int w) {
  if (mir_color == 1) return 0x07E0;
  if (mir_color == 2) return 0xF81F;
  int t = (x * 31) / (w > 0 ? w : 1);
  return (uint16_t)((t << 11) | ((63 - t) << 5) | (31 - t / 2));
}

// bin FFT logaritmico per colonna x
static int mir_log_bin(int x, int w, int maxB) {
  const float fmin = 40.0f, fmax = 4000.0f;
  float t = (float)x / (float)(w > 1 ? w - 1 : 1);
  float hz = expf(logf(fmin) + (logf(fmax) - logf(fmin)) * t);
  // assume 16 kHz, N=1024 → bin = hz * N / SR
  int b = (int)(hz * 1024.0f / 16000.0f);
  if (b < 1) b = 1;
  if (b >= maxB) b = maxB - 1;
  return b;
}

inline void mirror_init(LTDC_F746_Discovery &tft, int main_area_w) {
  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);
  memset(mir_prev, 0, sizeof(mir_prev));
  mir_ready = false;
  tft.setTextColor(LTDC_CYAN, LTDC_BLACK);
  tft.setTextSize(1);
  tft.setCursor(4, 14);
  tft.print("MIRROR LOG");
  tft.drawFastHLine(0, 140, main_area_w, 0x4208);
}

inline void mirror_update(float* real, int signalLength, LTDC_F746_Discovery &tft,
                          int main_area_w, float gain) {
  int maxB = signalLength / 2;
  int mid = 140;
  int w = main_area_w;
  if (w > 400) w = 400;

  for (int x = 0; x < main_area_w; x++) {
    int bin = mir_log_bin(x, main_area_w, maxB);
    int h = (int)(real[bin] * gain * 0.003f);
    if (h > 100) h = 100;
    if (h < 0) h = 0;
    int ph = mir_ready ? mir_prev[x < 400 ? x : 399] : 0;
    uint16_t c = mir_col(x, main_area_w);

    // differenziale sopra e sotto la linea
    if (h > ph) {
      tft.drawFastVLine(x, mid - h, h - ph, c);
      tft.drawFastVLine(x, mid + 1 + ph, h - ph, c);
    } else if (h < ph) {
      tft.drawFastVLine(x, mid - ph, ph - h, LTDC_BLACK);
      tft.drawFastVLine(x, mid + 1 + h, ph - h, LTDC_BLACK);
    }
    if (x < 400) mir_prev[x] = h;
  }
  mir_ready = true;
  tft.drawFastHLine(0, mid, main_area_w, 0x4208);
}
#endif
