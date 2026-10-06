#ifndef SPETTRO_LINEARE_H
#define SPETTRO_LINEARE_H

#define SL_BANDS 32

inline void spettro_lineare_init(LTDC_F746_Discovery &tft, int main_area_w) {
  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);
}

inline void spettro_lineare_update(float* real, int signalLength, LTDC_F746_Discovery &tft,
                                   int main_area_w, float gain) {
  int maxBins = signalLength / 2;
  int gap = 2;
  int barW = (main_area_w - gap * (SL_BANDS + 1)) / SL_BANDS;
  int baseY = 271;
  int maxH = 260;

  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);

  for (int b = 0; b < SL_BANDS; b++) {
    // Bande lineari (non log)
    int startBin = (b * maxBins) / SL_BANDS;
    int endBin = ((b + 1) * maxBins) / SL_BANDS;
    if (endBin <= startBin) endBin = startBin + 1;

    float sum = 0; int c = 0;
    for (int k = startBin; k < endBin; k++) { sum += real[k]; c++; }
    float avg = (c > 0) ? sum / c : 0;
    int h = (int)(avg * gain);
    if (h > maxH) h = maxH;
    if (h < 0) h = 0;

    int x = gap + b * (barW + gap);
    // gradiente semplice blu->ciano->bianco
    uint16_t col = (h > maxH * 2 / 3) ? 0xFFFF : (h > maxH / 3) ? 0x07FF : 0x001F;
    tft.fillRect(x, baseY - h, barW, h, col);
  }
}

#endif
