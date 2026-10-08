// =============================================================================
// SpettroLineare.h — spettro a barre LOGARITMICHE
// =============================================================================
// - Bande equispaziate in log(f) → più dettaglio sulle basse, uso denso dello schermo
// - Linea di base sotto le barre
// - Etichette frequenza al piede (Hz / kHz)
// - Aggiornamento a delta (meno flicker)
// =============================================================================

#ifndef SPETTRO_LINEARE_H
#define SPETTRO_LINEARE_H

#ifndef SL_BANDS
#define SL_BANDS 48
#endif

static int   sl_h[SL_BANDS];
static int   sl_barW, sl_gap, sl_baseY, sl_maxH, sl_areaW;
static bool  sl_inited = false;

// Posizione X della barra b
static inline int sl_bar_x(int b) {
  return sl_gap + b * (sl_barW + sl_gap);
}

// Bin FFT da frequenza (Hz)
static inline int sl_hz_to_bin(float hz, int signalLength, float sampleRate) {
  int bin = (int)(hz * (float)signalLength / sampleRate + 0.5f);
  int maxBins = signalLength / 2;
  if (bin < 1) bin = 1;
  if (bin > maxBins) bin = maxBins;
  return bin;
}

// Etichette fisse lungo l'asse log (Hz)
static const float sl_label_hz[] = { 100.f, 250.f, 500.f, 1000.f, 2000.f, 4000.f, 8000.f };
static const int   sl_label_n   = 7;

static void sl_draw_axis(LTDC_F746_Discovery &tft, float sampleRate) {
  // Linea di base
  tft.drawFastHLine(0, sl_baseY, sl_areaW, 0x8410);

  // Piccole tacche sotto ogni N barre
  for (int b = 0; b < SL_BANDS; b += 4) {
    int x = sl_bar_x(b) + sl_barW / 2;
    tft.drawFastVLine(x, sl_baseY + 1, 3, 0x4208);
  }

  // Etichette frequenza posizionate in scala log
  // f_min ~ 40 Hz (evita bin 0), f_max = Nyquist
  float fmin = 40.0f;
  float fmax = sampleRate * 0.5f;
  if (fmax < fmin * 2) fmax = fmin * 2;

  tft.setTextSize(1);
  tft.setTextColor(0xA614, LTDC_BLACK);

  for (int i = 0; i < sl_label_n; i++) {
    float hz = sl_label_hz[i];
    if (hz >= fmax) break;
    if (hz < fmin) continue;
    // posizione log 0..1
    float t = logf(hz / fmin) / logf(fmax / fmin);
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    int b = (int)(t * (SL_BANDS - 1) + 0.5f);
    int x = sl_bar_x(b);
    // testo centrato sotto
    char buf[8];
    if (hz >= 1000.f) {
      // es. 1k 2k 4k 8k
      int k = (int)(hz / 1000.f + 0.1f);
      snprintf(buf, sizeof(buf), "%dk", k);
    } else {
      snprintf(buf, sizeof(buf), "%d", (int)hz);
    }
    // offset per non sovrapporre troppo
    int tw = (int)strlen(buf) * 6;
    int tx = x + sl_barW / 2 - tw / 2;
    if (tx < 0) tx = 0;
    if (tx + tw > sl_areaW) tx = sl_areaW - tw;
    tft.setCursor(tx, sl_baseY + 5);
    tft.print(buf);
  }
}

inline void spettro_lineare_init(LTDC_F746_Discovery &tft, int main_area_w) {
  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);
  for (int i = 0; i < SL_BANDS; i++) sl_h[i] = 0;

  sl_areaW = main_area_w;
  sl_gap   = 1;
  sl_barW  = (main_area_w - sl_gap * (SL_BANDS + 1)) / SL_BANDS;
  if (sl_barW < 2) sl_barW = 2;
  sl_baseY = 252;                 // spazio sotto per le etichette Hz
  sl_maxH  = sl_baseY - 16;       // area barre sotto l'header
  sl_inited = true;

  // Asse + etichette Hz (niente titolo: le barre occupano anche l'alto)
  sl_draw_axis(tft, 16000.0f);
}

inline void spettro_lineare_update(float* real, int signalLength, float sampleRate,
                                   LTDC_F746_Discovery &tft, int main_area_w, float gain) {
  if (!sl_inited || main_area_w != sl_areaW) {
    spettro_lineare_init(tft, main_area_w);
    sl_draw_axis(tft, sampleRate);
  }

  int maxBins = signalLength / 2;
  float fmin = 40.0f;
  float fmax = sampleRate * 0.5f;
  if (fmax < fmin * 2.f) fmax = fmin * 2.f;
  float logMin = logf(fmin);
  float logSpan = logf(fmax) - logMin;
  if (logSpan < 0.1f) logSpan = 0.1f;

  for (int b = 0; b < SL_BANDS; b++) {
    // Estremi frequenza della banda (log)
    float t0 = (float)b / (float)SL_BANDS;
    float t1 = (float)(b + 1) / (float)SL_BANDS;
    float f0 = expf(logMin + t0 * logSpan);
    float f1 = expf(logMin + t1 * logSpan);

    int i0 = sl_hz_to_bin(f0, signalLength, sampleRate);
    int i1 = sl_hz_to_bin(f1, signalLength, sampleRate);
    if (i1 <= i0) i1 = i0 + 1;
    if (i1 > maxBins) i1 = maxBins;

    // Media energia nella banda
    float sum = 0;
    int cnt = 0;
    for (int k = i0; k < i1; k++) {
      sum += real[k];
      cnt++;
    }
    float avg = (cnt > 0) ? (sum / cnt) : 0.f;

    int h = (int)(avg * gain);
    if (h > sl_maxH) h = sl_maxH;
    if (h < 0) h = 0;

    int x = sl_bar_x(b);
    int oldH = sl_h[b];

    // Colore per altezza
    auto barColor = [&](int hh) -> uint16_t {
      if (hh > sl_maxH * 2 / 3) return 0xFFFF;
      if (hh > sl_maxH / 3)     return 0x07FF;
      return 0x001F;
    };

    // Delta: solo pezzo nuovo o nero sull'eccesso
    if (h > oldH) {
      tft.fillRect(x, sl_baseY - h, sl_barW, h - oldH, barColor(h));
      // se il colore cambia rispetto alla base, ridisegna tutta la barra
      if (barColor(h) != barColor(oldH) && oldH > 0) {
        tft.fillRect(x, sl_baseY - h, sl_barW, h, barColor(h));
      }
    } else if (h < oldH) {
      tft.fillRect(x, sl_baseY - oldH, sl_barW, oldH - h, LTDC_BLACK);
    }
    sl_h[b] = h;
  }

  // Ridisegna la linea di base (eventualmente cancellata dalle barre a h=0)
  tft.drawFastHLine(0, sl_baseY, sl_areaW, 0x8410);
}

#endif
