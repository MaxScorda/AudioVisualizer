#ifndef PERSISTENZA_H
#define PERSISTENZA_H

#define PS_W 200
#define PS_H 65
static uint8_t ps_buf[PS_W * PS_H];
static int ps_line = 0;

inline void persistenza_init(LTDC_F746_Discovery &tft, int main_area_w) {
  memset(ps_buf, 0, sizeof(ps_buf));
  ps_line = 0;
  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);
  tft.setTextColor(LTDC_CYAN, LTDC_BLACK);
  tft.setTextSize(1);
  tft.setCursor(4, 14);
  tft.print("PERSISTENCE (trail decay)");
}

inline void persistenza_update(float* real, int signalLength, LTDC_F746_Discovery &tft,
                               int main_area_w, float gain) {
  int bins = PS_H;
  if (bins > signalLength / 2) bins = signalLength / 2;

  // Decay LENTO: la scia resta visibile a lungo (diverso dallo spettrogramma)
  for (int i = 0; i < PS_W * PS_H; i++) {
    if (ps_buf[i] > 1) ps_buf[i] -= 1;
    else ps_buf[i] = 0;
  }

  float Amin = 1e10f, Amax = 1.0f;
  for (int n = 0; n < bins; n++) {
    if (real[n] < Amin) Amin = real[n];
    if (real[n] > Amax) Amax = real[n];
  }

  int w = main_area_w / 2;
  if (w > PS_W) w = PS_W;

  for (int n = 0; n < bins; n++) {
    int val = (int)(gain * 4.0f * (real[n] - Amin));
    if (val > 255) val = 255;
    if (val < 0) val = 0;
    // boost se vicino al max
    if (Amax > Amin && real[n] > Amax * 0.7f) val = (val + 255) / 2;
    int idx = n * PS_W + ps_line;
    if (val > ps_buf[idx]) ps_buf[idx] = (uint8_t)val;
  }

  // Ridisegna TUTTE le colonne (mostra scia completa)
  for (int x = 0; x < w; x++) {
    for (int n = 0; n < bins; n++) {
      uint8_t v = ps_buf[n * PS_W + x];
      if (v < 4) continue;
      int y = 30 + (bins - 1 - n) * 3;
      if (y > 268) y = 268;
      int px = x * 2;
      if (px < main_area_w) {
        tft.drawPixel(px, y, heatmap[v]);
        if (px + 1 < main_area_w) tft.drawPixel(px + 1, y, heatmap[v]);
      }
    }
  }

  if (++ps_line >= w) ps_line = 0;
}

#endif
