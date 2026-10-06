#ifndef SPETTROGRAMMA_H
#define SPETTROGRAMMA_H

#define ss_xmin 0
#define ss_ymin 11
#define ss_ymax 271

const int fft_bins = ss_ymax - ss_ymin;
static int spec_line = ss_xmin;

inline void spettrogramma_init(LTDC_F746_Discovery &tft, int main_area_w) {
  tft.fillRect(ss_xmin, ss_ymin, main_area_w, ss_ymax - ss_ymin, LTDC_BLACK);
  spec_line = ss_xmin;
}

// Come prima: 1 colonna per aggiornamento, scala gain tipo scaleAmp
inline void spettrogramma_update(float* real, int signalLength, LTDC_F746_Discovery &tft, int main_area_w, float gain) {
  int bins = (fft_bins < signalLength / 2) ? fft_bins : (signalLength / 2);
  float Amax = -1e10f, Amin = 1e10f;

  for (int n = 0; n < bins; n++) {
    if (real[n] > Amax) Amax = real[n];
    if (real[n] < Amin) Amin = real[n];
  }

  for (int n = 0; n < bins; n++) {
    int val = (int)(gain * (real[n] - Amin));
    if (val > 255) val = 255;
    if (val < 0) val = 0;
    tft.drawPixel(spec_line, bins - n + ss_ymin, heatmap[val]);
  }

  if (++spec_line >= main_area_w) spec_line = ss_xmin;
}

#endif
