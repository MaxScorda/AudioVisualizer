#ifndef PITCH_NOTE_H
#define PITCH_NOTE_H

static const char* note_names[12] = {
  "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};
static float pitch_hz_smooth = 0;
static int pitch_skip = 0;
static int last_midi = -1;
static int last_hz_i = -1;
static int last_bar = -1;

inline void pitch_note_init(LTDC_F746_Discovery &tft, int main_area_w) {
  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);
  pitch_hz_smooth = 0;
  pitch_skip = 0;
  last_midi = last_hz_i = last_bar = -1;
  tft.setTextColor(LTDC_CYAN, LTDC_BLACK);
  tft.setTextSize(1);
  tft.setCursor(10, 14);
  tft.print("PITCH  (GAIN = stabilita')");
}

inline void pitch_note_update(float* real, int signalLength, float sampleRate,
                              LTDC_F746_Discovery &tft, int main_area_w, float gain) {
  int hold = (int)(gain * 8.0f);
  if (hold < 1) hold = 1;
  if (hold > 30) hold = 30;

  pitch_skip++;
  if (pitch_skip < hold) return;
  pitch_skip = 0;

  int maxBins = signalLength / 2;
  int peakBin = 2;
  float peakVal = 0;
  for (int n = 2; n < maxBins; n++) {
    if (real[n] > peakVal) { peakVal = real[n]; peakBin = n; }
  }

  float hz = (peakBin * sampleRate) / (float)signalLength;
  float alpha = 0.15f / (gain + 0.1f);
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
  int hz_i = (int)(pitch_hz_smooth + 0.5f);

  int barH = (int)(peakVal * 0.02f * (gain + 0.5f));
  if (barH > 180) barH = 180;
  if (barH < 0) barH = 0;

  // barra a delta (resta pulita)
  if (barH > last_bar) {
    tft.fillRect(20, 271 - barH, 50, barH - (last_bar > 0 ? last_bar : 0), 0x07E0);
  } else if (barH < last_bar) {
    tft.fillRect(20, 271 - last_bar, 50, last_bar - barH, LTDC_BLACK);
  }
  last_bar = barH;

  if (midi != last_midi || hz_i != last_hz_i) {
    last_midi = midi;
    last_hz_i = hz_i;
    // sfondo pieno sotto testo (niente pixel residui)
    tft.fillRect(90, 55, 300, 180, LTDC_BLACK);

    tft.setTextColor(LTDC_WHITE, LTDC_BLACK);
    tft.setTextSize(4);
    tft.setCursor(100, 70);
    tft.print(note_names[note]);
    tft.print(octave);

    tft.setTextSize(2);
    tft.setCursor(100, 140);
    tft.print(hz_i);
    tft.print(" Hz");

    tft.setTextSize(1);
    tft.setCursor(100, 180);
    tft.print("MIDI ");
    tft.print(midi);
    tft.setCursor(100, 200);
    tft.print("hold x");
    tft.print(hold);
  }
}

#endif
