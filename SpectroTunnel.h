#ifndef SPECTRO_TUNNEL_H
#define SPECTRO_TUNNEL_H
#define TUN_HIST 12
#define TUN_BINS 32
static float tun_hist[TUN_HIST][TUN_BINS];
static int tun_pos = 0;
static int tun_color = 0; // 0 white-ish, 1 cyan, 2 fire, 3 green
inline void tunnel_set_color(int c) { tun_color = c % 4; }
inline int  tunnel_get_color() { return tun_color; }

static uint16_t tun_col(int depth, float h) {
  int v = 8 + depth * 4 + (int)(h * 0.2f);
  if (v > 31) v = 31;
  if (tun_color == 1) return (uint16_t)((0 << 11) | (v * 2 << 5) | v);      // cyan
  if (tun_color == 2) return (uint16_t)((v << 11) | (v << 5) | 0);          // fire
  if (tun_color == 3) return (uint16_t)((0 << 11) | (v * 2 << 5) | 0);      // green
  return (uint16_t)((v << 11) | (v * 2 << 5) | v);                           // grey/white
}

// banda log → range bin FFT
static void tun_log_range(int b, int nBins, int maxB, int* s, int* e) {
  const float fmin = 40.0f, fmax = 4000.0f;
  float t0 = (float)b / (float)nBins;
  float t1 = (float)(b + 1) / (float)nBins;
  float hz0 = expf(logf(fmin) + (logf(fmax) - logf(fmin)) * t0);
  float hz1 = expf(logf(fmin) + (logf(fmax) - logf(fmin)) * t1);
  *s = (int)(hz0 * 1024.0f / 16000.0f);
  *e = (int)(hz1 * 1024.0f / 16000.0f);
  if (*s < 1) *s = 1;
  if (*e <= *s) *e = *s + 1;
  if (*e > maxB) *e = maxB;
}

inline void tunnel_init(LTDC_F746_Discovery &tft, int main_area_w) {
  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);
  memset(tun_hist, 0, sizeof(tun_hist));
  tun_pos = 0;
  tft.setTextColor(LTDC_CYAN, LTDC_BLACK);
  tft.setTextSize(1);
  tft.setCursor(4, 14);
  tft.print("TUNNEL LOG");
}

inline void tunnel_update(float* real, int signalLength, LTDC_F746_Discovery &tft,
                          int main_area_w, float gain) {
  int maxB = signalLength / 2;
  for (int b = 0; b < TUN_BINS; b++) {
    int s, e;
    tun_log_range(b, TUN_BINS, maxB, &s, &e);
    float sum = 0; int c = 0;
    for (int k = s; k < e; k++) { sum += real[k]; c++; }
    tun_hist[tun_pos][b] = (c ? sum / c : 0);
  }
  tun_pos = (tun_pos + 1) % TUN_HIST;

  tft.fillRect(0, 26, main_area_w, 240, LTDC_BLACK);
  int cx = main_area_w / 2, cy = 150;
  for (int t = 0; t < TUN_HIST; t++) {
    int idx = (tun_pos + t) % TUN_HIST;
    float depth = (float)t / (float)TUN_HIST;
    int w = (int)(40 + depth * (main_area_w - 60));
    int hmax = (int)(20 + depth * 100);
    int x0 = cx - w / 2;
    int ybase = cy + hmax / 2;
    for (int b = 0; b < TUN_BINS; b++) {
      float hv = tun_hist[idx][b] * gain * 0.002f;
      if (hv > hmax) hv = hmax;
      int x = x0 + (b * w) / TUN_BINS;
      int bw = w / TUN_BINS - 1;
      if (bw < 1) bw = 1;
      int hh = (int)hv;
      if (hh > 0)
        tft.fillRect(x, ybase - hh, bw, hh, tun_col(t, hv));
    }
  }
}
#endif
