// =============================================================================
// AudioVisualizer — STM32F746G-DISCOVERY
// =============================================================================
// Visualizzatore audio in tempo reale:
//   - ingresso: microfono digitale a bordo (SAI2 + codec WM8994, DMA circolare)
//   - display: LCD 480x272 via LTDC + libreria LTDC_F746_Discovery
//   - touch: FT5336 (sidebar comandi)
//   - impostazioni: EEPROM (Settings.h) — niente SD in questo sketch
//
// Viste (viewMode):
//   0  Spettrogramma          8  Waterfall + peak
//   1  EQ 8 bande             9  Pitch / nota
//   2  EQ 16 bande           10  Lissajous XY
//   3  EQ 32 bande           11  Persistenza
//   4  EQ 48 bande           12  Beat meter
//   5  Onda (waveform)       13  Mesh 3D
//   6  Spettro lineare       14  VU a lancette
//   7  VU L/R a barre
// =============================================================================

#include <arduinoFFT.h>
#include <stm32f7xx_hal.h>
#include <string.h>
#include <math.h>

// ---------------------------------------------------------------------------
// Rimuove macro LED del core che collidono con l'enum del BSP Discovery
// ---------------------------------------------------------------------------
#ifdef LED_GREEN
#undef LED_GREEN
#endif
#ifdef LED_BUILTIN
#undef LED_BUILTIN
#endif
#ifdef LED_ORANGE
#undef LED_ORANGE
#endif
#ifdef LED_RED
#undef LED_RED
#endif
#ifdef LED_BLUE
#undef LED_BLUE
#endif

#include "stm32746g_discovery.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_audio.h"
#include "stm32746g_discovery_ts.h"

// Tasto USER blu sulla Discovery (nome enum dipende dalla versione BSP)
#if defined(BUTTON_USER)
  #define USER_BUTTON BUTTON_USER
#elif defined(BUTTON_KEY)
  #define USER_BUTTON BUTTON_KEY
#else
  #define USER_BUTTON ((Button_TypeDef)0)
#endif

#include "heat565.h"                 // colormap per spettrogramma / persistenza
#include <LTDC_F746_Discovery.h>

LTDC_F746_Discovery tft;

// Moduli visualizzazione (un file .h per vista)
#include "spettrogramma.h"
#include "equalizzatore.h"
#include "onda.h"
#include "SpettroLineare.h"
#include "VuMeter.h"
#include "WaterfallPeak.h"
#include "PitchNote.h"
#include "Lissajous.h"
#include "Persistenza.h"
#include "BeatMeter.h"
#include "Mesh3D.h"
#include "VuNeedle.h"

// ---------------------------------------------------------------------------
// Layout schermo: area grafica a sinistra, sidebar touch a destra
// ---------------------------------------------------------------------------
const int sidebar_w   = 80;
const int main_area_w = 480 - sidebar_w;   // 400 px

// ---------------------------------------------------------------------------
// Stato UI
// ---------------------------------------------------------------------------
#define VIEW_COUNT 15

int  viewMode        = 0;     // vista attiva
int  OldviewMode     = -1;    // per ridisegnare il nome vista solo al cambio

int  eq_color_mode   = 2;     // 0 arcobaleno, 1 VU, 2 Winamp
int  onda_color_mode = 0;     // palette onda
int  spectro_res     = 0;     // 0 normal, 1 fast, 2 slow
bool eq_peak_enable  = true;  // peak-hold sulle barre EQ

bool     auto_cycle    = false;  // avanzamento automatico viste
uint32_t auto_cycle_ms = 0;      // timestamp ultimo cambio AUTO

// Gain indipendente per ogni vista (valori di default sensati)
float view_gain[VIEW_COUNT] = {
  0.05f,   // 0  spettrogramma
  0.001f,  // 1  EQ 8
  0.001f,  // 2  EQ 16
  0.001f,  // 3  EQ 32
  0.001f,  // 4  EQ 48
  0.280f,  // 5  onda
  0.02f,   // 6  spettro lineare
  0.002f,  // 7  VU barre
  0.08f,   // 8  waterfall + peak
  1.5f,    // 9  pitch (alto = più lento/stabile)
  0.28f,   // 10 lissajous
  0.08f,   // 11 persistenza
  0.02f,   // 12 beat
  0.08f,   // 13 mesh 3D
  0.050f   // 14 VU lancette
};

// Dopo view_gain / VIEW_COUNT: Settings usa queste variabili
#include "Settings.h"

// ---------------------------------------------------------------------------
// Audio: buffer DMA stereo interleaved, FFT su canale L
// ---------------------------------------------------------------------------
#define SIGNAL_LENGTH 1024

int16_t audio_dma_buffer[SIGNAL_LENGTH * 2];  // L,R,L,R,... (half + full callback)
float   real[SIGNAL_LENGTH];
float   imag[SIGNAL_LENGTH];
float   sample_hist[SIGNAL_LENGTH];           // storia L per FFT continua
float   sample_hist_r[SIGNAL_LENGTH];         // storia R

volatile bool     audio_ready = false;  // nuovo blocco DMA pronto
volatile uint32_t irq_count   = 0;      // diagnostica IRQ
volatile uint8_t  dma_section = 0;      // 0 = half, 1 = full

#define AUDIO_INPUT_SOURCE  INPUT_DEVICE_DIGITAL_MICROPHONE_2
#define AUDIO_FREQ          AUDIO_FREQUENCY_16K

extern SAI_HandleTypeDef haudio_in_sai;

// ---------------------------------------------------------------------------
// Callback audio BSP + handler DMA/SAI
// ---------------------------------------------------------------------------
extern "C" {

  void BSP_AUDIO_IN_TransferComplete_CallBack(void) {
    dma_section = 1;
    audio_ready = true;
    irq_count++;
  }

  void BSP_AUDIO_IN_HalfTransfer_CallBack(void) {
    dma_section = 0;
    audio_ready = true;
    irq_count++;
  }

  void BSP_AUDIO_IN_Error_CallBack(void) {
    irq_count = 0xDEAD;
  }

  void DMA2_Stream7_IRQHandler(void) {
    if (haudio_in_sai.hdmarx) HAL_DMA_IRQHandler(haudio_in_sai.hdmarx);
  }

  void DMA2_Stream4_IRQHandler(void) {
    if (haudio_in_sai.hdmarx) HAL_DMA_IRQHandler(haudio_in_sai.hdmarx);
  }

  void SAI2_IRQHandler(void) {
    HAL_SAI_IRQHandler(&haudio_in_sai);
  }
}

ArduinoFFT<float> FFT = ArduinoFFT<float>(
  real, imag, SIGNAL_LENGTH, (float)AUDIO_FREQ
);

// ---------------------------------------------------------------------------
// Helper UI
// ---------------------------------------------------------------------------
static bool is_eq_mode()     { return (viewMode >= 1 && viewMode <= 4); }
static bool is_color_mode()  { return is_eq_mode() || (viewMode == 5); }
static bool is_spectro_mode(){ return (viewMode == 0); }

static const char* view_name(int v) {
  switch (v) {
    case 0:  return "Spectogram   ";
    case 1:  return "EQ 8 Bande   ";
    case 2:  return "EQ 16 Bande  ";
    case 3:  return "EQ 32 Bande  ";
    case 4:  return "EQ 48 Bande  ";
    case 5:  return "Waveform     ";
    case 6:  return "Spec Linear  ";
    case 7:  return "VU L/R       ";
    case 8:  return "WF + Peak    ";
    case 9:  return "Pitch/Note   ";
    case 10: return "Lissajous XY ";
    case 11: return "Persistence  ";
    case 12: return "Beat Meter   ";
    case 13: return "Mesh 3D      ";
    case 14: return "VU Needle    ";
    default: return "???          ";
  }
}

// Riga superiore: stato EEPROM + eventuale flag AUTO
void draw_status_line() {
  tft.fillRect(0, 0, main_area_w, 10, LTDC_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(settings_eeprom_ok ? LTDC_GREEN : LTDC_YELLOW, LTDC_BLACK);
  tft.setCursor(0, 1);
  tft.print(settings_status_text());
  if (auto_cycle) {
    tft.setTextColor(LTDC_CYAN, LTDC_BLACK);
    tft.print(" | AUTO");
  }
}

// Sidebar touch a destra (VIEW / GAIN / AUTO / COLOR|RES / PK)
void draw_sidebar() {
  tft.drawLine(main_area_w, 0, main_area_w, 272, LTDC_WHITE);
  tft.fillRect(main_area_w + 1, 0, sidebar_w - 1, 272, LTDC_BLACK);

  const int btn_w = sidebar_w - 2;
  const int btn_x = main_area_w + 1;

  tft.setTextSize(1);
  tft.setTextColor(LTDC_WHITE);

  // VIEW +
  tft.fillRect(btn_x, 4, btn_w, 28, 0x3186);
  tft.setCursor(btn_x + 5, 12);
  tft.print("VIEW +");

  // VIEW -
  tft.fillRect(btn_x, 36, btn_w, 28, 0x3186);
  tft.setCursor(btn_x + 5, 44);
  tft.print("VIEW -");

  // GAIN +
  tft.fillRect(btn_x, 68, btn_w, 28, 0x0410);
  tft.setCursor(btn_x + 5, 76);
  tft.print("GAIN +");

  // GAIN -
  tft.fillRect(btn_x, 100, btn_w, 28, 0x0410);
  tft.setCursor(btn_x + 5, 108);
  tft.print("GAIN -");

  // AUTO
  tft.fillRect(btn_x, 132, btn_w, 28, auto_cycle ? 0x0540 : 0x4208);
  tft.setCursor(btn_x + 5, 140);
  tft.print(auto_cycle ? "AUTO ON" : "AUTO OFF");

  // COLOR (EQ / onda) oppure RES (spettrogramma)
  if (is_color_mode()) {
    tft.fillRect(btn_x, 164, btn_w, 28, 0x8800);
    tft.setCursor(btn_x + 5, 172);
    tft.print("COLOR");
    tft.setCursor(btn_x + 2, 196);
    tft.print("C:");
    tft.print(viewMode == 5 ? onda_color_mode : eq_color_mode);

    if (is_eq_mode()) {
      tft.fillRect(btn_x, 210, btn_w, 24, eq_peak_enable ? 0x0540 : 0x4208);
      tft.setCursor(btn_x + 5, 216);
      tft.print(eq_peak_enable ? "PK ON" : "PK OFF");
    }
  } else if (is_spectro_mode()) {
    tft.fillRect(btn_x, 164, btn_w, 28, 0x8010);
    tft.setCursor(btn_x + 5, 172);
    tft.print("RES");
    tft.setCursor(btn_x + 2, 196);
    if (spectro_res == 0)      tft.print("R:NOR");
    else if (spectro_res == 1) tft.print("R:FAST");
    else                       tft.print("R:SLOW");
  }

  // Vista e gain correnti
  tft.fillRect(btn_x, 238, btn_w, 34, LTDC_BLACK);
  tft.setTextColor(LTDC_WHITE, LTDC_BLACK);
  tft.setCursor(btn_x + 2, 242);
  tft.print("V:");
  tft.print(viewMode);
  tft.setCursor(btn_x + 2, 256);
  tft.print("G:");
  tft.print(view_gain[viewMode], 4);
}

// Inizializza la vista attiva e ridisegna sidebar + status
void trigger_view_change() {
  switch (viewMode) {
    case 0:  spettrogramma_init(tft, main_area_w); break;
    case 1:
    case 2:
    case 3:
    case 4:  equalizzatore_init(tft, main_area_w); break;
    case 5:  onda_init(tft, main_area_w); break;
    case 6:  spettro_lineare_init(tft, main_area_w); break;
    case 7:  vu_meter_init(tft, main_area_w); break;
    case 8:  waterfall_peak_init(tft, main_area_w); break;
    case 9:  pitch_note_init(tft, main_area_w); break;
    case 10: lissajous_init(tft, main_area_w); break;
    case 11: persistenza_init(tft, main_area_w); break;
    case 12: beat_meter_init(tft, main_area_w); break;
    case 13: mesh3d_init(tft, main_area_w); break;
    case 14: vu_needle_init(tft, main_area_w); break;
    default: break;
  }
  draw_sidebar();
  draw_status_line();
}

// Tasto fisico USER: attiva/disattiva AUTO
void handle_user_button() {
  static uint32_t last = 0;
  static uint8_t  prev = 0;

  uint8_t now = (BSP_PB_GetState(USER_BUTTON) == SET) ? 1 : 0;

  if (now && !prev && (millis() - last > 400)) {
    last = millis();
    auto_cycle    = !auto_cycle;
    auto_cycle_ms = millis();
    settings_mark_dirty(true);  // salva solo lo stato AUTO
    draw_sidebar();
    draw_status_line();
    Serial.println(auto_cycle ? "USER: AUTO ON" : "USER: AUTO OFF");
  }
  prev = now;
}

// Touch sulla sidebar
void handle_touch() {
  static uint32_t last_touch = 0;
  TS_StateTypeDef ts;
  BSP_TS_GetState(&ts);

  if (!ts.touchDetected) return;
  if (millis() - last_touch <= 300) return;

  int tx = ts.touchX[0];
  int ty = ts.touchY[0];
  if (tx <= main_area_w) return;  // fuori sidebar

  if (ty >= 4 && ty <= 32) {
    // VIEW +
    viewMode++;
    if (viewMode >= VIEW_COUNT) viewMode = 0;
    settings_mark_dirty();
    trigger_view_change();
  } else if (ty >= 36 && ty <= 64) {
    // VIEW -
    viewMode--;
    if (viewMode < 0) viewMode = VIEW_COUNT - 1;
    settings_mark_dirty();
    trigger_view_change();
  } else if (ty >= 68 && ty <= 96) {
    // GAIN +
    view_gain[viewMode] *= 1.5f;
    settings_mark_dirty();
    draw_sidebar();
  } else if (ty >= 100 && ty <= 128) {
    // GAIN -
    view_gain[viewMode] /= 1.5f;
    if (view_gain[viewMode] < 1e-6f) view_gain[viewMode] = 1e-6f;
    settings_mark_dirty();
    draw_sidebar();
  } else if (ty >= 132 && ty <= 160) {
    // AUTO
    auto_cycle    = !auto_cycle;
    auto_cycle_ms = millis();
    settings_mark_dirty(true);
    draw_sidebar();
    draw_status_line();
  } else if (ty >= 164 && ty <= 192 && is_color_mode()) {
    // COLOR
    if (viewMode == 5) {
      onda_color_mode++;
      if (onda_color_mode > 2) onda_color_mode = 0;
    } else {
      eq_color_mode++;
      if (eq_color_mode > 2) eq_color_mode = 0;
    }
    settings_mark_dirty();
    draw_sidebar();
  } else if (ty >= 164 && ty <= 192 && is_spectro_mode()) {
    // RES
    spectro_res++;
    if (spectro_res > 2) spectro_res = 0;
    settings_mark_dirty();
    draw_sidebar();
  } else if (ty >= 210 && ty <= 234 && is_eq_mode()) {
    // Peak hold EQ
    eq_peak_enable = !eq_peak_enable;
    settings_mark_dirty();
    draw_sidebar();
  }

  last_touch = millis();
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Boot");
  Serial.flush();

  // LCD BSP
  Serial.println("LCD...");
  BSP_LCD_Init();
  BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
  BSP_LCD_SelectLayer(0);
  BSP_LCD_Clear(LCD_COLOR_BLACK);
  Serial.println("LCD OK");

  // Touch + tasto USER
  Serial.println("TS...");
  BSP_TS_Init(480, 272);
  BSP_PB_Init(USER_BUTTON, BUTTON_MODE_GPIO);
  Serial.println("TS OK");

  // Framebuffer GFX
  Serial.println("TFT...");
  uint16_t *buffer = (uint16_t *)malloc(
    LTDC_F746_ROKOTECH.width * LTDC_F746_ROKOTECH.height * sizeof(uint16_t)
  );
  if (!buffer) {
    Serial.println("MALLOC FAIL");
    while (1) delay(1000);
  }
  tft.begin(buffer);
  tft.fillScreen(LTDC_BLACK);
  tft.setRotation(0);
  tft.setTextWrap(false);
  tft.setTextColor(LTDC_GREEN);
  tft.setTextSize(1);
  tft.setCursor(0, 2);
  tft.print("Init Audio...");
  Serial.println("TFT OK");

  // Audio in (microfono digitale)
  uint8_t st = BSP_AUDIO_IN_InitEx(
    AUDIO_INPUT_SOURCE,
    AUDIO_FREQ,
    DEFAULT_AUDIO_IN_BIT_RESOLUTION,
    DEFAULT_AUDIO_IN_CHANNEL_NBR
  );
  Serial.print("InitEx=");
  Serial.println(st);

  if (st == AUDIO_OK) {
    BSP_AUDIO_IN_SetVolume(90);
    HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);
    HAL_NVIC_SetPriority(DMA2_Stream4_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream4_IRQn);
    HAL_NVIC_SetPriority(SAI2_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(SAI2_IRQn);

    uint8_t r = BSP_AUDIO_IN_Record(
      (uint16_t *)audio_dma_buffer, SIGNAL_LENGTH
    );
    Serial.print("Record=");
    Serial.println(r);
    tft.setCursor(0, 10);
    tft.print("OK rec=");
    tft.println(r);
  } else {
    tft.setCursor(0, 10);
    tft.setTextColor(LTDC_RED);
    tft.print("Audio ERROR ");
    tft.println(st);
  }

  delay(800);
  tft.fillScreen(LTDC_BLACK);
  memset(sample_hist, 0, sizeof(sample_hist));
  memset(sample_hist_r, 0, sizeof(sample_hist_r));

  // Carica impostazioni da EEPROM (se presenti)
  Serial.println("Load settings (EEPROM)...");
  if (settings_load()) Serial.println("Settings LOADED");
  else                 Serial.println("Using defaults");
  Serial.println(settings_status_text());

  delay(400);
  tft.fillScreen(LTDC_BLACK);
  trigger_view_change();
}

// ---------------------------------------------------------------------------
// Loop principale
// ---------------------------------------------------------------------------
void loop() {
  static bool printed = false;

  handle_touch();
  handle_user_button();
  settings_autosave_poll();

  // AUTO: ogni 60 s passa alla vista successiva (senza salvare ogni passo)
  if (auto_cycle && (millis() - auto_cycle_ms >= 60000UL)) {
    auto_cycle_ms = millis();
    viewMode++;
    if (viewMode >= VIEW_COUNT) viewMode = 0;
    trigger_view_change();
  }

  // Attende un nuovo blocco DMA (timeout diagnostico)
  uint32_t twait = millis();
  while (!audio_ready) {
    if (millis() - twait > 2000) {
      tft.setCursor(0, 1);
      tft.setTextColor(LTDC_YELLOW);
      tft.print("IRQ=");
      tft.print(irq_count);
      return;
    }
  }
  audio_ready = false;

  // Copia il blocco stereo corrente (half o full)
  const int frames = SIGNAL_LENGTH / 2;
  int16_t *src = (dma_section == 0)
    ? &audio_dma_buffer[0]
    : &audio_dma_buffer[SIGNAL_LENGTH];

  float rawL[frames];
  float rawR[frames];

  for (int n = 0; n < frames; n++) {
    rawL[n] = (float)src[n * 2];
    rawR[n] = (float)src[n * 2 + 1];
  }

  // Scorri la storia e inserisci i nuovi campioni
  memmove(sample_hist, sample_hist + frames,
          (SIGNAL_LENGTH - frames) * sizeof(float));
  memmove(sample_hist_r, sample_hist_r + frames,
          (SIGNAL_LENGTH - frames) * sizeof(float));
  for (int n = 0; n < frames; n++) {
    sample_hist[SIGNAL_LENGTH - frames + n]   = rawL[n];
    sample_hist_r[SIGNAL_LENGTH - frames + n] = rawR[n];
  }

  // DC remove + copia in buffer FFT
  float avgL = 0, avgR = 0;
  for (int n = 0; n < SIGNAL_LENGTH; n++) {
    avgL += sample_hist[n];
    avgR += sample_hist_r[n];
  }
  avgL /= SIGNAL_LENGTH;
  avgR /= SIGNAL_LENGTH;

  for (int n = 0; n < SIGNAL_LENGTH; n++) {
    real[n] = sample_hist[n] - avgL;
    imag[n] = 0;
  }
  // rawL/rawR restano l'ultimo blocco (per onda, VU, lissajous)
  for (int n = 0; n < frames; n++) {
    rawL[n] = sample_hist[SIGNAL_LENGTH - frames + n] - avgL;
    rawR[n] = sample_hist_r[SIGNAL_LENGTH - frames + n] - avgR;
  }

  // FFT
  uint32_t t0 = micros();
  FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.compute(FFT_FORWARD);
  FFT.complexToMagnitude();
  uint32_t dt = micros() - t0;

  // Una tantum: stampa durata FFT
  if (!printed) {
    tft.setCursor(100, 1);
    tft.setTextColor(LTDC_GREEN, LTDC_BLACK);
    tft.print("FFT ");
    tft.print(dt);
    tft.print("us");
    printed = true;
    delay(600);
  }

  // Nome vista in alto a destra (solo al cambio)
  if (OldviewMode != viewMode) {
    OldviewMode = viewMode;
    tft.setTextColor(LTDC_GREEN, LTDC_BLACK);
    tft.setCursor(220, 1);
    tft.print("              ");
    tft.setCursor(220, 1);
    tft.print(view_name(viewMode));
  }

  const float g = view_gain[viewMode];

  // Dispatch vista attiva
  switch (viewMode) {
    case 0: {
      // Spettrogramma: RES controlla quanto spesso si aggiunge una colonna
      static uint8_t slow_div = 0;
      bool do_col = false;
      if (spectro_res == 1) {
        do_col = true;                         // FAST: ogni blocco
      } else if (spectro_res == 0) {
        do_col = (dma_section == 1);           // NOR: solo full
      } else if (dma_section == 1) {
        slow_div++;
        if (slow_div >= 2) {                   // SLOW: ogni 2 full
          slow_div = 0;
          do_col = true;
        }
      }
      if (do_col) spettrogramma_update(real, SIGNAL_LENGTH, tft, main_area_w, g);
      break;
    }
    case 1: equalizzatore_update(real, SIGNAL_LENGTH, tft, main_area_w, 8,  g, eq_color_mode, eq_peak_enable); break;
    case 2: equalizzatore_update(real, SIGNAL_LENGTH, tft, main_area_w, 16, g, eq_color_mode, eq_peak_enable); break;
    case 3: equalizzatore_update(real, SIGNAL_LENGTH, tft, main_area_w, 32, g, eq_color_mode, eq_peak_enable); break;
    case 4: equalizzatore_update(real, SIGNAL_LENGTH, tft, main_area_w, 48, g, eq_color_mode, eq_peak_enable); break;
    case 5: onda_update(rawL, frames, tft, main_area_w, g, onda_color_mode); break;
    case 6: spettro_lineare_update(real, SIGNAL_LENGTH, tft, main_area_w, g); break;
    case 7: vu_meter_update(rawL, rawR, frames, tft, main_area_w, g); break;
    case 8:
      if (dma_section == 1)
        waterfall_peak_update(real, SIGNAL_LENGTH, tft, main_area_w, g);
      break;
    case 9:  pitch_note_update(real, SIGNAL_LENGTH, (float)AUDIO_FREQ, tft, main_area_w, g); break;
    case 10: lissajous_update(rawL, rawR, frames, tft, main_area_w, g); break;
    case 11:
      if (dma_section == 1)
        persistenza_update(real, SIGNAL_LENGTH, tft, main_area_w, g);
      break;
    case 12: beat_meter_update(real, SIGNAL_LENGTH, tft, main_area_w, g); break;
    case 13: mesh3d_update(real, SIGNAL_LENGTH, tft, main_area_w, g); break;
    case 14: vu_needle_update(rawL, rawR, frames, tft, main_area_w, g); break;
    default: break;
  }
}
