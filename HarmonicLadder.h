#ifndef HARMONIC_LADDER_H
#define HARMONIC_LADDER_H
// =============================================================================
// Harmonic Ladder — cerca una frequenza fondamentale (f0) nello spettro e
// mostra l'energia sulle armoniche H1=f0, H2=2*f0, H3=3*f0, ...
// Utile per voce/strumenti monofonici: vedi quali armoniche sono forti.
// Label H1..H8 disegnate UNA volta in init (fisse).
// =============================================================================
static int harm_color = 0;
static int harm_prev_w[8];
static bool harm_ready = false;

inline void harm_set_color(int c) { harm_color = c % 3; harm_ready = false; }
inline int  harm_get_color() { return harm_color; }

inline void harm_init(LTDC_F746_Discovery &tft, int main_area_w) {
  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);
  memset(harm_prev_w, 0, sizeof(harm_prev_w));
  harm_ready = false;
  tft.setTextColor(LTDC_CYAN, LTDC_BLACK);
  tft.setTextSize(1);
  tft.setCursor(4, 14);
  tft.print("HARMONIC LADDER");

  // Label fisse H1..H8
  const int barH = 22;
  tft.setTextColor(LTDC_WHITE, LTDC_BLACK);
  for (int h = 1; h <= 8; h++) {
    int y = 40 + (h - 1) * (barH + 4);
    tft.setCursor(4, y + 6);
    char buf[8];
    snprintf(buf, sizeof(buf), "H%d", h);
    tft.print(buf);
  }
}

inline void harm_update(float* real, int signalLength, float sampleRate,
                        LTDC_F746_Discovery &tft, int main_area_w, float gain) {
  int maxB = signalLength / 2;
  int best = 2;
  float bestV = 0;
  int lim = maxB / 4;
  if (lim < 10) lim = maxB / 2;
  for (int k = 2; k < lim; k++) {
    if (real[k] > bestV) { bestV = real[k]; best = k; }
  }
  float f0 = best * sampleRate / (float)signalLength;
  if (f0 < 40) f0 = 40;

  // f0 in alto a destra (solo testo, area piccola)
  tft.fillRect(main_area_w - 70, 14, 66, 10, LTDC_BLACK);
  tft.setTextColor(LTDC_YELLOW, LTDC_BLACK);
  tft.setTextSize(1);
  tft.setCursor(main_area_w - 70, 14);
  tft.print((int)f0);
  tft.print("Hz");

  const int steps = 8;
  const int barH = 22;
  for (int h = 1; h <= steps; h++) {
    float freq = f0 * h;
    int bin = (int)(freq * signalLength / sampleRate);
    if (bin < 1) bin = 1;
    if (bin >= maxB) bin = maxB - 1;
    float e = real[bin];
    if (bin > 1) e += real[bin - 1] * 0.5f;
    if (bin + 1 < maxB) e += real[bin + 1] * 0.5f;
    float w = e * gain * 0.001f;
    if (w > 1) w = 1;
    int bw = (int)(w * (main_area_w - 80));
    if (bw < 2) bw = 2;
    int y = 40 + (h - 1) * (barH + 4);
    int ph = harm_ready ? harm_prev_w[h - 1] : 0;
    uint16_t c;
    if (harm_color == 1) c = 0x07FF;
    else if (harm_color == 2) c = 0xFFE0;
    else c = (h == 1) ? 0xFFFF : (uint16_t)((31 - h * 2) << 11 | ((20 + h * 4) << 5) | h * 2);

    // differenziale barra
    if (bw > ph)
      tft.fillRect(60 + ph, y, bw - ph, barH - 2, c);
    else if (bw < ph)
      tft.fillRect(60 + bw, y, ph - bw, barH - 2, LTDC_BLACK);

    harm_prev_w[h - 1] = bw;
  }
  harm_ready = true;
}
#endif
