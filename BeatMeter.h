#ifndef BEAT_METER_H
#define BEAT_METER_H

static float beat_env = 0;
static float beat_avg = 0.01f;
static uint8_t beat_flash = 0;
static int beat_drawn_h = 0;
static int beat_drawn_th = -1;
static int8_t beat_shown = -1;

inline void beat_meter_init(LTDC_F746_Discovery &tft, int main_area_w) {
  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);
  beat_env = 0; beat_avg = 0.01f; beat_flash = 0;
  beat_drawn_h = 0; beat_drawn_th = -1; beat_shown = -1;

  tft.setTextColor(LTDC_CYAN, LTDC_BLACK);
  tft.setTextSize(1);
  tft.setCursor(8, 14);
  tft.print("BEAT: bar=energy  line=avg  red=kick");
}

inline void beat_meter_update(float* real, int signalLength, LTDC_F746_Discovery &tft,
                              int main_area_w, float gain) {
  int lowN = signalLength / 16;
  if (lowN < 4) lowN = 4;
  float e = 0;
  for (int i = 1; i < lowN; i++) e += real[i];
  e = (e / lowN) * gain;

  beat_avg = beat_avg * 0.92f + e * 0.08f;
  if (e > beat_env) beat_env = e;
  else beat_env *= 0.88f;

  bool beat = (e > beat_avg * 1.4f && e > 0.002f);
  if (beat) beat_flash = 12;
  if (beat_flash > 0) beat_flash--;

  int h = (int)(beat_env * 100.0f);
  if (h > 180) h = 180;
  if (h < 0) h = 0;
  int th = (int)(beat_avg * 100.0f);
  if (th > 180) th = 180;

  int bx = main_area_w / 2 - 35;
  int base = 270;
  uint16_t barCol = beat_flash ? 0xF800 : 0xFFE0;

  int state = beat_flash ? 1 : 0;
  if (state != beat_shown) {
    beat_shown = state;
    tft.fillRect(main_area_w / 2 - 55, 40, 120, 28, LTDC_BLACK);
    tft.setTextSize(3);
    tft.setTextColor(state ? LTDC_RED : 0x4208, LTDC_BLACK);
    tft.setCursor(main_area_w / 2 - 50, 40);
    tft.print(state ? "BEAT" : "----");
  }

  // barra piena: se cambia altezza o colore, ridisegna in modo pulito
  static uint16_t lastCol = 0;
  if (h != beat_drawn_h || barCol != lastCol) {
    // cancella vecchia
    if (beat_drawn_h > 0)
      tft.fillRect(bx, base - beat_drawn_h, 70, beat_drawn_h, LTDC_BLACK);
    if (h > 0)
      tft.fillRect(bx, base - h, 70, h, barCol);
    beat_drawn_h = h;
    lastCol = barCol;
  }

  if (th != beat_drawn_th) {
    if (beat_drawn_th >= 0)
      tft.drawFastHLine(bx - 15, base - beat_drawn_th, 100, LTDC_BLACK);
    tft.drawFastHLine(bx - 15, base - th, 100, 0x07FF);
    beat_drawn_th = th;
  }
}

#endif
