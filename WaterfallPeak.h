#ifndef WATERFALL_PEAK_H
#define WATERFALL_PEAK_H

static int wp_line = 0;
static int wp_peak_y = 140;

inline void waterfall_peak_init(LTDC_F746_Discovery &tft, int main_area_w) {
  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);
  wp_line = 0;
  wp_peak_y = 140;
}

inline void waterfall_peak_update(float* real, int signalLength, LTDC_F746_Discovery &tft,
                                  int main_area_w, float gain) {
  int bins = 250;
  if (bins > signalLength / 2) bins = signalLength / 2;
  float Amin = 1e10f, Amax = -1e10f;
  int peakBin = 0;
  float peakVal = -1;

  for (int n = 0; n < bins; n++) {
    if (real[n] > Amax) Amax = real[n];
    if (real[n] < Amin) Amin = real[n];
    if (real[n] > peakVal) { peakVal = real[n]; peakBin = n; }
  }

  for (int n = 0; n < bins; n++) {
    int val = (int)(gain * 1.5f * (real[n] - Amin));
    if (val > 255) val = 255;
    if (val < 0) val = 0;
    tft.drawPixel(wp_line, 11 + (bins - 1 - n), heatmap[val]);
  }

  int target = 11 + (bins - 1 - peakBin);
  wp_peak_y = (wp_peak_y * 2 + target) / 3;

  // traccia peak SPESSA e chiara
  for (int dy = -1; dy <= 1; dy++) {
    int y = wp_peak_y + dy;
    if (y >= 11 && y <= 270) {
      tft.drawPixel(wp_line, y, LTDC_WHITE);
      if (wp_line > 0) tft.drawPixel(wp_line - 1, y, LTDC_YELLOW);
    }
  }
  // cursore a sinistra della colonna
  if (wp_line >= 3) {
    tft.drawLine(wp_line - 3, wp_peak_y, wp_line, wp_peak_y, 0xF81F); // magenta
  }

  if (++wp_line >= main_area_w) wp_line = 0;
}

#endif
