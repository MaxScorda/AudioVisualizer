#ifndef BEAT_METER_H
#define BEAT_METER_H

static float beat_env = 0;
static float beat_avg = 0.01f;
static uint8_t beat_flash = 0;

inline void beat_meter_init(LTDC_F746_Discovery &tft, int main_area_w) {
  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);
  beat_env = 0; beat_avg = 0.01f; beat_flash = 0;

  // Una sola riga di aiuto, fissa, ben separata dalla zona dinamica
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

  // Zona dinamica: tutto sotto y=30 (header intatto)
  const int top = 30;
  tft.fillRect(0, top, main_area_w, 272 - top, LTDC_BLACK);

  // Stato grande e pulito
  tft.setTextSize(3);
  tft.setTextColor(beat_flash ? LTDC_RED : 0x4208, LTDC_BLACK);
  tft.setCursor(main_area_w / 2 - 50, 40);
  tft.print(beat_flash ? "BEAT" : "----");

  // Barra
  int h = (int)(beat_env * 100.0f);
  if (h > 180) h = 180;
  if (h < 0) h = 0;
  uint16_t barCol = beat_flash ? 0xF800 : 0xFFE0;
  int bx = main_area_w / 2 - 35;
  tft.fillRect(bx, 271 - h, 70, h, barCol);

  // Linea media
  int th = (int)(beat_avg * 100.0f);
  if (th > 180) th = 180;
  tft.drawFastHLine(bx - 15, 271 - th, 100, 0x07FF);
}

#endif
