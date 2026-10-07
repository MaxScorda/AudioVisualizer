// =============================================================================
// VuMeter.h — VU stereo a barre con aggiornamento a DELTA (no flicker scale)
// =============================================================================
// Init: cornici, scale, label (fissi, mai cancellati).
// Update: solo rettangoli barre — se sale disegna sopra, se scende nero sull'eccesso.
// =============================================================================

#ifndef VU_METER_H
#define VU_METER_H

static float vu_l = 0, vu_r = 0;
static float vu_lp = 0, vu_rp = 0;
static int   vu_h[2]  = {0, 0};   // altezza barra disegnata
static int   vu_ph[2] = {0, 0};   // peak disegnato

static int vu_barW, vu_maxH, vu_base, vu_x0[2];

inline void vu_meter_init(LTDC_F746_Discovery &tft, int main_area_w) {
  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);
  vu_l = vu_r = vu_lp = vu_rp = 0;
  vu_h[0] = vu_h[1] = 0;
  vu_ph[0] = vu_ph[1] = 0;

  const int mid = main_area_w / 2;
  vu_maxH = 200;
  vu_base = 240;
  vu_barW = mid - 50;
  vu_x0[0] = 40;
  vu_x0[1] = mid + 40;

  // Header
  tft.setTextColor(LTDC_CYAN, LTDC_BLACK);
  tft.setTextSize(1);
  tft.setCursor(8, 14);
  tft.print("VU METER  L / R");

  // Label L/R (fissi)
  tft.setTextColor(LTDC_WHITE, LTDC_BLACK);
  tft.setTextSize(3);
  tft.setCursor(mid / 2 - 10, 245);
  tft.print("L");
  tft.setCursor(mid + mid / 2 - 10, 245);
  tft.print("R");

  // Scale + cornici (fissi — non si toccano più in update)
  tft.setTextSize(1);
  tft.setTextColor(0x8410, LTDC_BLACK);
  int topY = vu_base - vu_maxH;

  for (int ch = 0; ch < 2; ch++) {
    int x0 = vu_x0[ch];
    int sx = (ch == 0) ? 20 : (mid + 20);
    tft.setCursor(sx - 2, 40);  tft.print("0");
    tft.setCursor(sx - 8, 100); tft.print("-10");
    tft.setCursor(sx - 8, 160); tft.print("-20");
    tft.setCursor(sx - 8, 210); tft.print("-30");

    tft.drawRect(x0 - 1, topY - 1, vu_barW + 2, vu_maxH + 2, 0x4208);

    // Tacche orizzontali fisse sulla cornice
    for (int t = 1; t <= 3; t++) {
      int yy = vu_base - (vu_maxH * t) / 4;
      tft.drawFastHLine(x0 - 4, yy, 3, 0x8410);
      tft.drawFastHLine(x0 + vu_barW + 1, yy, 3, 0x8410);
    }
  }
}

static void vu_draw_segment(LTDC_F746_Discovery &tft, int x0, int yBottom, int h, int barW) {
  // Disegna h pixel di barra colorata da yBottom verso l'alto
  // Zone: verde 0-60%, giallo 60-85%, rosso 85-100% del maxH
  if (h <= 0) return;
  int yGreen  = (int)(vu_maxH * 0.60f);
  int yYellow = (int)(vu_maxH * 0.85f);

  // Disegna riga per riga dal basso per rispettare i colori
  for (int s = 0; s < h; s++) {
    uint16_t c;
    if (s < yGreen)       c = 0x07E0;
    else if (s < yYellow) c = 0xFFE0;
    else                  c = 0xF800;
    tft.drawFastHLine(x0, yBottom - s, barW, c);
  }
}

inline void vu_meter_update(float* left, float* right, int nSamples,
                            LTDC_F746_Discovery &tft, int main_area_w, float gain) {
  (void)main_area_w;

  float el = 0, er = 0, pl = 0, pr = 0;
  for (int i = 0; i < nSamples; i++) {
    float a = left[i];  if (a < 0) a = -a;
    float b = right[i]; if (b < 0) b = -b;
    el += a; er += b;
    if (a > pl) pl = a;
    if (b > pr) pr = b;
  }
  el = (el / nSamples) * gain * 0.6f + pl * gain * 0.4f;
  er = (er / nSamples) * gain * 0.6f + pr * gain * 0.4f;
  if (el > 1) el = 1; if (er > 1) er = 1;

  if (el > vu_l) vu_l = vu_l * 0.3f + el * 0.7f;
  else           vu_l *= 0.88f;
  if (er > vu_r) vu_r = vu_r * 0.3f + er * 0.7f;
  else           vu_r *= 0.88f;

  if (vu_l > vu_lp) vu_lp = vu_l; else vu_lp -= 0.008f;
  if (vu_r > vu_rp) vu_rp = vu_r; else vu_rp -= 0.008f;
  if (vu_lp < 0) vu_lp = 0;
  if (vu_rp < 0) vu_rp = 0;

  for (int ch = 0; ch < 2; ch++) {
    int x0 = vu_x0[ch];
    float v  = (ch == 0) ? vu_l  : vu_r;
    float pk = (ch == 0) ? vu_lp : vu_rp;
    int h  = (int)(v * vu_maxH);
    int ph = (int)(pk * vu_maxH);
    if (h < 0) h = 0;
    if (h > vu_maxH) h = vu_maxH;
    if (ph > vu_maxH) ph = vu_maxH;

    int oldH  = vu_h[ch];
    int oldPh = vu_ph[ch];

    // --- DELTA barra ---
    if (h > oldH) {
      // Solo la parte nuova (dal vecchio top al nuovo)
      for (int s = oldH; s < h; s++) {
        uint16_t c;
        if (s < (int)(vu_maxH * 0.60f))      c = 0x07E0;
        else if (s < (int)(vu_maxH * 0.85f)) c = 0xFFE0;
        else                                 c = 0xF800;
        tft.drawFastHLine(x0, vu_base - s, vu_barW, c);
      }
    } else if (h < oldH) {
      // Nero solo sulla porzione in eccesso
      tft.fillRect(x0, vu_base - oldH, vu_barW, oldH - h, LTDC_BLACK);
    }
    vu_h[ch] = h;

    // --- Peak: cancella vecchio, disegna nuovo solo se cambia ---
    if (oldPh != ph) {
      if (oldPh > 2) {
        // Se il peak vecchio era sopra la barra, lascia nero; se dentro, ridisegna colore barra
        if (oldPh <= h) {
          uint16_t c;
          if (oldPh < (int)(vu_maxH * 0.60f))      c = 0x07E0;
          else if (oldPh < (int)(vu_maxH * 0.85f)) c = 0xFFE0;
          else                                     c = 0xF800;
          tft.fillRect(x0, vu_base - oldPh - 1, vu_barW, 3, c);
        } else {
          tft.fillRect(x0, vu_base - oldPh - 1, vu_barW, 3, LTDC_BLACK);
        }
      }
      if (ph > 2) {
        tft.fillRect(x0, vu_base - ph - 1, vu_barW, 3, LTDC_WHITE);
      }
      vu_ph[ch] = ph;
    }
  }
}

#endif
