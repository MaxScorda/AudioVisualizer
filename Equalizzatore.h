#ifndef EQUALIZZATORE_H
#define EQUALIZZATORE_H

#define EQ_MAX_BLOCKS 12
#define eq_xmin 0
#define eq_ymin 11
#define eq_ymax 271

static float band_values[48] = {0};
static float peak_values[48] = {0};

static const uint16_t eq_colors[16] = {
  0x780F, 0x9011, 0xA814, 0xF812, 0xF800, 0xFA00, 0xFD00, 0xFFE0,
  0x07E0, 0x07E6, 0x07EF, 0x03FF, 0x01FF, 0x001F, 0x0018, 0x801F
};
static const uint16_t vu_colors[12] = {
  0x07E0, 0x07E0, 0x07E0, 0x07E0,
  0x07E0, 0x07E0, 0xFFE0, 0xFFE0,
  0xFC00, 0xFC00, 0xF800, 0xF800
};
static const uint16_t winamp_colors[12] = {
  0x0010, 0x0015, 0x001A, 0x001F, 0x01DF, 0x039F,
  0x055F, 0x071F, 0x07FF, 0x47FF, 0x87FF, 0xC7FF
};

inline void equalizzatore_init(LTDC_F746_Discovery &tft, int main_area_w) {
  tft.fillRect(eq_xmin, eq_ymin, main_area_w, eq_ymax - eq_ymin + 1, LTDC_BLACK);
  for (int i = 0; i < 48; i++) { band_values[i] = 0; peak_values[i] = 0; }
}

inline void equalizzatore_update(float* real, int signalLength, LTDC_F746_Discovery &tft,
                                 int main_area_w, int eq_bands, float eq_gain, int color_mode, bool peak_enable) {
  int maxBins = signalLength / 2;
  int screenHeight = eq_ymax - eq_ymin + 1;
  int gapX = (eq_bands > 32) ? 1 : 2;
  int gapY = 2;
  int barWidth = (main_area_w - (gapX * (eq_bands + 1))) / eq_bands;
  int blockHeight = (screenHeight - (gapY * (EQ_MAX_BLOCKS + 1))) / EQ_MAX_BLOCKS;

  for (int b = 0; b < eq_bands; b++) {
    int startBin = (int)pow(2.0, (double)b * log2(maxBins) / eq_bands);
    int endBin   = (int)pow(2.0, (double)(b + 1) * log2(maxBins) / eq_bands);
    if (endBin <= startBin) endBin = startBin + 1;
    if (endBin > maxBins) endBin = maxBins;

    float sum = 0; int count = 0;
    for (int k = startBin; k < endBin; k++) { sum += real[k]; count++; }
    float avg = (count > 0) ? (sum / count) : 0;
    float level = avg * eq_gain;
    if (level > EQ_MAX_BLOCKS) level = EQ_MAX_BLOCKS;

    if (level > band_values[b]) band_values[b] = level;
    else {
      band_values[b] -= 0.6f;
      if (band_values[b] < 0) band_values[b] = 0;
    }

    int activeBlocks = (int)band_values[b];
    int peakBlk = 0;
    if (peak_enable) {
      if (band_values[b] > peak_values[b]) peak_values[b] = band_values[b];
      else {
        peak_values[b] -= 0.08f;
        if (peak_values[b] < 0) peak_values[b] = 0;
      }
      peakBlk = (int)peak_values[b];
      if (peakBlk >= EQ_MAX_BLOCKS) peakBlk = EQ_MAX_BLOCKS - 1;
    } else {
      peak_values[b] = 0;
    }

    int x = eq_xmin + gapX + b * (barWidth + gapX);

    for (int blk = 0; blk < EQ_MAX_BLOCKS; blk++) {
      int y = eq_ymax - (blk + 1) * (blockHeight + gapY);
      if (blk < activeBlocks) {
        uint16_t color;
        if (color_mode == 1) color = vu_colors[blk];
        else if (color_mode == 2) color = winamp_colors[blk];
        else color = eq_colors[(b * 16) / eq_bands];
        tft.fillRect(x, y, barWidth, blockHeight, color);
      } else {
        tft.fillRect(x, y, barWidth, blockHeight, LTDC_BLACK);
      }
    }

    if (peak_enable && peakBlk > 0) {
      int py = eq_ymax - (peakBlk + 1) * (blockHeight + gapY);
      tft.fillRect(x, py, barWidth, 2, LTDC_WHITE);
    }
  }
}

#endif
