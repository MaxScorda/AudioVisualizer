// =============================================================================
// Mesh3D.h — mesh 3D bande LOG + label fisse
// Isometria originale; FMAX 4 kHz / 8 kHz (tasto sidebar)
// =============================================================================

#ifndef MESH3D_H
#define MESH3D_H

#define MESH_HIST 8
#define MESH_BINS 40
static float mesh_hist[MESH_HIST][MESH_BINS];
static int mesh_pos = 0;
static float mesh_bin_lo[MESH_BINS];
static float mesh_bin_hi[MESH_BINS];
static bool mesh_log_ready = false;
static bool mesh_labels_drawn = false;
static int  mesh_plot_w = 0;
static int  mesh_last_fmax_mode = -1;

extern int mesh_fmax_8k; // 0=4k, 1=8k — nel .ino

static void mesh_build_log_bins(float sampleRate, int fftHalf) {
  const float fmin = 40.0f;
  // 4k oppure fino a ~Nyquist (8k @ 16kHz)
  float fmax = mesh_fmax_8k ? (sampleRate * 0.5f - 50.0f) : 4000.0f;
  if (fmax < fmin * 2) fmax = fmin * 2;

  float logMin = logf(fmin);
  float logMax = logf(fmax);

  for (int b = 0; b < MESH_BINS; b++) {
    float t0 = (float)b / (float)MESH_BINS;
    float t1 = (float)(b + 1) / (float)MESH_BINS;
    mesh_bin_lo[b] = expf(logMin + (logMax - logMin) * t0);
    mesh_bin_hi[b] = expf(logMin + (logMax - logMin) * t1);
  }
  mesh_log_ready = true;
  mesh_last_fmax_mode = mesh_fmax_8k ? 1 : 0;
}

static int mesh_freq_to_fft_bin(float hz, float sampleRate, int fftHalf) {
  int b = (int)(hz * (fftHalf * 2) / sampleRate);
  if (b < 1) b = 1;
  if (b >= fftHalf) b = fftHalf - 1;
  return b;
}

static void mesh_draw_labels(LTDC_F746_Discovery &tft, int main_area_w, int plotW) {
  tft.fillRect(0, 248, main_area_w, 24, LTDC_BLACK);
  tft.setTextColor(LTDC_YELLOW, LTDC_BLACK);
  tft.setTextSize(1);

  float fmin = mesh_bin_lo[0];
  float fmax = mesh_bin_hi[MESH_BINS - 1];
  float logMin = logf(fmin);
  float logMax = logf(fmax);
  int baseX = 8;

  // Due set espliciti (niente puntatori ambigui)
  if (mesh_fmax_8k) {
    const float labels[] = {50, 100, 250, 500, 1000, 2000, 4000, 8000};
    const char* texts[]  = {"50", "100", "250", "500", "1k", "2k", "4k", "8k"};
    for (int i = 0; i < 8; i++) {
      float f = labels[i];
      // tolleranza ampia sull'ultima label
      if (f < fmin * 0.9f) continue;
      if (f > fmax * 1.15f) continue;
      float t = (logf(f) - logMin) / (logMax - logMin);
      if (t < 0) t = 0;
      if (t > 1) t = 1;
      int x = baseX + (int)(t * plotW) - 8;
      if (x < 0) x = 0;
      if (x > main_area_w - 22) x = main_area_w - 22;
      tft.setCursor(x, 252);
      tft.print(texts[i]);
    }
  } else {
    const float labels[] = {50, 100, 250, 500, 1000, 2000, 4000};
    const char* texts[]  = {"50", "100", "250", "500", "1k", "2k", "4k"};
    for (int i = 0; i < 7; i++) {
      float f = labels[i];
      if (f < fmin * 0.9f || f > fmax * 1.15f) continue;
      float t = (logf(f) - logMin) / (logMax - logMin);
      if (t < 0) t = 0;
      if (t > 1) t = 1;
      int x = baseX + (int)(t * plotW) - 8;
      if (x < 0) x = 0;
      if (x > main_area_w - 22) x = main_area_w - 22;
      tft.setCursor(x, 252);
      tft.print(texts[i]);
    }
  }
  mesh_labels_drawn = true;
}

inline void mesh3d_on_range_changed(LTDC_F746_Discovery &tft, int main_area_w) {
  mesh_log_ready = false;
  mesh_labels_drawn = false;
  mesh_last_fmax_mode = -1;

  const float sampleRate = 16000.0f;
  mesh_build_log_bins(sampleRate, 512);

  int plotW = main_area_w - 50 - MESH_HIST * 10;
  if (plotW < 80) plotW = main_area_w - 40;
  mesh_plot_w = plotW;

  // Forza ridisegno label subito
  mesh_draw_labels(tft, main_area_w, plotW);

  // titolo con range attuale
  tft.fillRect(4, 14, 120, 10, LTDC_BLACK);
  tft.setTextColor(LTDC_CYAN, LTDC_BLACK);
  tft.setTextSize(1);
  tft.setCursor(4, 14);
  tft.print(mesh_fmax_8k ? "MESH 3D 8k" : "MESH 3D 4k");
}

inline void mesh3d_init(LTDC_F746_Discovery &tft, int main_area_w) {
  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);
  memset(mesh_hist, 0, sizeof(mesh_hist));
  mesh_pos = 0;
  mesh_log_ready = false;
  mesh_labels_drawn = false;
  mesh_last_fmax_mode = -1;
  mesh_plot_w = main_area_w - 50 - MESH_HIST * 10;
  if (mesh_plot_w < 80) mesh_plot_w = main_area_w - 40;

  tft.setTextColor(LTDC_CYAN, LTDC_BLACK);
  tft.setTextSize(1);
  tft.setCursor(4, 14);
  tft.print(mesh_fmax_8k ? "MESH 3D 8k" : "MESH 3D 4k");
}

inline void mesh3d_update(float* real, int signalLength, LTDC_F746_Discovery &tft,
                          int main_area_w, float gain) {
  const float sampleRate = 16000.0f;
  int maxBins = signalLength / 2;

  if (!mesh_log_ready || mesh_last_fmax_mode != (mesh_fmax_8k ? 1 : 0)) {
    mesh_build_log_bins(sampleRate, maxBins);
    mesh_labels_drawn = false;
  }

  int plotW = main_area_w - 50 - MESH_HIST * 10;
  if (plotW < 80) plotW = main_area_w - 40;
  mesh_plot_w = plotW;

  if (!mesh_labels_drawn)
    mesh_draw_labels(tft, main_area_w, plotW);

  float scale = gain * 2.0f;

  for (int b = 0; b < MESH_BINS; b++) {
    int s = mesh_freq_to_fft_bin(mesh_bin_lo[b], sampleRate, maxBins);
    int e = mesh_freq_to_fft_bin(mesh_bin_hi[b], sampleRate, maxBins);
    if (e <= s) e = s + 1;
    float sum = 0;
    int c = 0;
    for (int k = s; k < e && k < maxBins; k++) {
      sum += real[k];
      c++;
    }
    mesh_hist[mesh_pos][b] = (c > 0) ? sum / c : 0;
  }
  mesh_pos = (mesh_pos + 1) % MESH_HIST;

  tft.fillRect(0, 26, main_area_w, 222, LTDC_BLACK);

  for (int t = 0; t < MESH_HIST; t++) {
    int idx = (mesh_pos + t) % MESH_HIST;
    int yOff = 20 + t * 14;
    int xOff = 8 + t * 10;
    uint16_t col = (t == MESH_HIST - 1) ? 0xFFFF : (uint16_t)(0x0010 + t * 0x00C0);

    int prevx = xOff, prevy = 230 - yOff;
    for (int b = 0; b < MESH_BINS; b++) {
      int x = xOff + (b * plotW) / MESH_BINS;
      int h = (int)(mesh_hist[idx][b] * scale * 0.04f);
      if (h > 90) h = 90;
      if (h < 0) h = 0;
      int y = 230 - yOff - h;
      if (y < 28) y = 28;
      if (y > 246) y = 246;
      if (b > 0) tft.drawLine(prevx, prevy, x, y, col);
      prevx = x;
      prevy = y;
    }
  }
}

#endif
