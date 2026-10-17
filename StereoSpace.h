#ifndef STEREO_SPACE_H
#define STEREO_SPACE_H
static int stereo_color = 0;
static int stereo_prevL = 0, stereo_prevR = 0;
static int stereo_prevBx = -1, stereo_prevBy = -1;
static bool stereo_ready = false;

inline void stereo_set_color(int c) { stereo_color = c % 3; stereo_ready = false; }
inline int  stereo_get_color() { return stereo_color; }

inline void stereo_init(LTDC_F746_Discovery &tft, int main_area_w) {
  tft.fillRect(0, 11, main_area_w, 261, LTDC_BLACK);
  stereo_prevL = stereo_prevR = 0;
  stereo_prevBx = stereo_prevBy = -1;
  stereo_ready = false;
  tft.setTextColor(LTDC_CYAN, LTDC_BLACK);
  tft.setTextSize(1);
  tft.setCursor(4, 14);
  tft.print("STEREO SPACE");
  int mid = main_area_w / 2;
  tft.drawFastVLine(mid, 30, 220, 0x4208);
  tft.setTextColor(LTDC_WHITE, LTDC_BLACK);
  tft.setCursor(mid - 50, 255);
  tft.print("L");
  tft.setCursor(mid + 40, 255);
  tft.print("R");
}

// rettangolo differenziale verticale (come EQ)
static void stereo_bar(LTDC_F746_Discovery &tft, int x, int base, int w, int h, int ph, uint16_t c) {
  if (h > ph) {
    tft.fillRect(x, base - h, w, h - ph, c);
  } else if (h < ph) {
    tft.fillRect(x, base - ph, w, ph - h, LTDC_BLACK);
  }
}

inline void stereo_update(float* left, float* right, int n,
                          LTDC_F746_Discovery &tft, int main_area_w, float gain) {
  double accL = 0, accR = 0;
  for (int i = 0; i < n; i++) {
    accL += (double)left[i] * left[i];
    accR += (double)right[i] * right[i];
  }
  float l = sqrtf((float)(accL / n)) * gain * 0.00015f;
  float r = sqrtf((float)(accR / n)) * gain * 0.00015f;
  if (l > 1) l = 1;
  if (r > 1) r = 1;

  int mid = main_area_w / 2;
  int base = 250;
  int hL = (int)(l * 180), hR = (int)(r * 180);
  uint16_t cL = (stereo_color == 1) ? 0x07FF : (stereo_color == 2) ? 0xFFE0 : 0x07E0;
  uint16_t cR = (stereo_color == 1) ? 0xF81F : (stereo_color == 2) ? 0xF800 : 0x001F;

  if (!stereo_ready) {
    tft.fillRect(0, 30, main_area_w, 220, LTDC_BLACK);
    tft.drawFastVLine(mid, 30, 220, 0x4208);
  }

  // barre L/R differenziali
  stereo_bar(tft, mid - 70, base, 30, hL, stereo_ready ? stereo_prevL : 0, cL);
  stereo_bar(tft, mid + 40, base, 30, hR, stereo_ready ? stereo_prevR : 0, cR);

  // balance dot: cancella vecchio, disegna nuovo
  float bal = (l + r) > 1e-6f ? (r - l) / (l + r) : 0;
  int bx = mid + (int)(bal * (mid - 50));
  int by = base - (int)(((l + r) * 0.5f) * 180);
  if (by < 40) by = 40;
  if (stereo_ready && stereo_prevBx >= 0)
    tft.fillCircle(stereo_prevBx, stereo_prevBy, 5, LTDC_BLACK);
  tft.fillCircle(bx, by, 5, LTDC_WHITE);

  stereo_prevL = hL;
  stereo_prevR = hR;
  stereo_prevBx = bx;
  stereo_prevBy = by;
  stereo_ready = true;
}
#endif
