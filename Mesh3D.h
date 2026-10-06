#ifndef MESH3D_H
#define MESH3D_H

#define MESH_HIST 8
#define MESH_BINS 48
static float mesh_hist[MESH_HIST][MESH_BINS];
static int mesh_pos = 0;

inline void mesh3d_init(LTDC_F746_Discovery &tft, int main_area_w) {
  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);
  memset(mesh_hist, 0, sizeof(mesh_hist));
  mesh_pos = 0;
}

inline void mesh3d_update(float* real, int signalLength, LTDC_F746_Discovery &tft,
                          int main_area_w, float gain) {
  int maxBins = signalLength / 2;
  float scale = gain * 2.0f; // gain piu' sensibile

  for (int b = 0; b < MESH_BINS; b++) {
    int s = (b * maxBins) / MESH_BINS;
    int e = ((b + 1) * maxBins) / MESH_BINS;
    if (e <= s) e = s + 1;
    float sum = 0; int c = 0;
    for (int k = s; k < e; k++) { sum += real[k]; c++; }
    mesh_hist[mesh_pos][b] = (c > 0) ? sum / c : 0;
  }
  mesh_pos = (mesh_pos + 1) % MESH_HIST;

  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);

  for (int t = 0; t < MESH_HIST; t++) {
    int idx = (mesh_pos + t) % MESH_HIST;
    int yOff = 20 + t * 14;
    int xOff = 8 + t * 10;
    uint16_t col = (t == MESH_HIST - 1) ? 0xFFFF : (uint16_t)(0x0010 + t * 0x00C0);

    int prevx = xOff, prevy = 250 - yOff;
    for (int b = 0; b < MESH_BINS; b++) {
      int x = xOff + (b * (main_area_w - 50 - MESH_HIST * 10)) / MESH_BINS;
      int h = (int)(mesh_hist[idx][b] * scale * 0.04f);
      if (h > 100) h = 100;
      if (h < 0) h = 0;
      int y = 250 - yOff - h;
      if (y < 12) y = 12;
      if (b > 0) tft.drawLine(prevx, prevy, x, y, col);
      prevx = x; prevy = y;
    }
  }
}

#endif
