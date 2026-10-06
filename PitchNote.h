#ifndef PITCH_NOTE_H
#define PITCH_NOTE_H

static const char* note_names[12] = {
  "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};
static float pitch_hz_smooth = 0;
static int pitch_skip = 0;

inline void pitch_note_init(LTDC_F746_Discovery &tft, int main_area_w) {
  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);
  pitch_hz_smooth = 0;
  pitch_skip = 0;
  tft.setTextColor(LTDC_CYAN, LTDC_BLACK);
  tft.setTextSize(1);
  tft.setCursor(10, 14);
  tft.print("PITCH  (GAIN = stabilita')");
}

inline void pitch_note_update(float* real, int signalLength, float sampleRate,
                              LTDC_F746_Discovery &tft, int main_area_w, float gain) {
  // gain alto = piu' lento/stabile (piu' frame saltati + smoothing forte)
  // gain basso = reattivo
  int hold = (int)(gain * 8.0f); // tipico gain 1 -> hold 8
  if (hold < 1) hold = 1;
  if (hold > 30) hold = 30;

  pitch_skip++;
  if (pitch_skip < hold) return; // non aggiornare display
  pitch_skip = 0;

  int maxBins = signalLength / 2;
  int peakBin = 2;
  float peakVal = 0;
  for (int n = 2; n < maxBins; n++) {
    if (real[n] > peakVal) { peakVal = real[n]; peakBin = n; }
  }

  float hz = (peakBin * sampleRate) / (float)signalLength;
  float alpha = 0.15f / (gain + 0.1f); // gain alto = alpha basso = piu' lento
  if (alpha > 0.5f) alpha = 0.5f;
  if (alpha < 0.02f) alpha = 0.02f;
  if (pitch_hz_smooth < 1) pitch_hz_smooth = hz;
  else pitch_hz_smooth = pitch_hz_smooth * (1 - alpha) + hz * alpha;

  float n = 12.0f * log2f(pitch_hz_smooth / 440.0f) + 69.0f;
  int midi = (int)(n + 0.5f);
  if (midi < 0) midi = 0;
  if (midi > 127) midi = 127;
  int note = midi % 12;
  int octave = (midi / 12) - 1;

  // clear solo area valori (header resta)
  tft.fillRect(0, 40, main_area_w, 232, LTDC_BLACK);

  int barH = (int)(peakVal * 0.02f * (gain + 0.5f));
  if (barH > 180) barH = 180;
  if (barH < 0) barH = 0;
  tft.fillRect(20, 271 - barH, 50, barH, 0x07E0);

  tft.setTextColor(LTDC_WHITE, LTDC_BLACK);
  tft.setTextSize(4);
  tft.setCursor(100, 70);
  tft.print(note_names[note]);
  tft.print(octave);

  tft.setTextSize(2);
  tft.setCursor(100, 140);
  tft.print((int)(pitch_hz_smooth + 0.5f));
  tft.print(" Hz");

  tft.setTextSize(1);
  tft.setCursor(100, 180);
  tft.print("MIDI ");
  tft.print(midi);
  tft.setCursor(100, 200);
  tft.print("hold x");
  tft.print(hold);
}

#endif
