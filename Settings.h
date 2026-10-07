#ifndef SETTINGS_H
#define SETTINGS_H

// =============================================================================
// Settings.h — salvataggio impostazioni in EEPROM (flash emulata)
// =============================================================================
// Nessuna SD: evita blocchi audio. La SD si testa a parte (SdTest_F746).
//
// Cosa viene salvato:
//   - vista corrente, gain per ogni vista
//   - colori EQ/onda, risoluzione spettrogramma, peak hold EQ
//   - flag AUTO cycle
//
// Comportamento:
//   - In modalità AUTO non si salva a ogni cambio vista (solo al toggle AUTO)
//   - Autosave ritardato di 5 s dopo l'ultima modifica manuale
//   - Si scrivono solo i byte cambiati (meno erase flash → meno freeze)
// =============================================================================

#include <EEPROM.h>
#include <string.h>

// Versione struttura: se cambi il layout, incrementa il magic
#define SETTINGS_MAGIC       0xA9
#define SETTINGS_EEPROM_ADDR 0

// Stato runtime
static bool     settings_dirty      = false;  // true = da salvare
static bool     settings_eeprom_ok  = false;  // true = almeno un load/save ok
static uint32_t settings_dirty_ms   = 0;      // timestamp ultima modifica

// Variabili definite in AudioVisualizer.ino
extern int   viewMode;
extern int   eq_color_mode;
extern int   onda_color_mode;
extern int   spectro_res;
extern bool  eq_peak_enable;
extern bool  auto_cycle;
extern float view_gain[];

// Testo per la riga di stato in alto a sinistra
static const char* settings_status_text() {
  return settings_eeprom_ok ? "EEPROMok" : "NoSave";
}

// -----------------------------------------------------------------------------
// Segna che le impostazioni sono cambiate e vanno salvate.
// force=false (default): in AUTO non fa nulla (evita write continui).
// force=true: salva comunque (usare al toggle AUTO ON/OFF).
// -----------------------------------------------------------------------------
static void settings_mark_dirty(bool force = false) {
  if (auto_cycle && !force) return;
  settings_dirty    = true;
  settings_dirty_ms = millis();
}

// Layout binario in EEPROM (compatto, allineato)
struct SettingsBlob {
  uint8_t magic;       // deve essere SETTINGS_MAGIC
  uint8_t view;        // viewMode 0..VIEW_COUNT-1
  uint8_t eq_color;    // 0..2
  uint8_t onda_color;  // 0..2
  uint8_t spectro_res; // 0..2
  uint8_t eq_peak;     // 0/1
  uint8_t auto_on;     // 0/1
  float   gains[16];   // gain per vista (max 16)
};

// -----------------------------------------------------------------------------
// Salva su EEPROM (solo byte diversi dal contenuto attuale)
// -----------------------------------------------------------------------------
static bool settings_eeprom_save() {
  SettingsBlob b;
  memset(&b, 0, sizeof(b));

  b.magic       = SETTINGS_MAGIC;
  b.view        = (uint8_t)viewMode;
  b.eq_color    = (uint8_t)eq_color_mode;
  b.onda_color  = (uint8_t)onda_color_mode;
  b.spectro_res = (uint8_t)spectro_res;
  b.eq_peak     = eq_peak_enable ? 1 : 0;
  b.auto_on     = auto_cycle ? 1 : 0;

  for (int i = 0; i < VIEW_COUNT && i < 16; i++) {
    b.gains[i] = view_gain[i];
  }

  const uint8_t* p = (const uint8_t*)&b;
  bool any = false;

  for (size_t i = 0; i < sizeof(b); i++) {
    uint8_t old = EEPROM.read(SETTINGS_EEPROM_ADDR + i);
    if (old != p[i]) {
      EEPROM.write(SETTINGS_EEPROM_ADDR + i, p[i]);
      any = true;
    }
  }

  settings_eeprom_ok = true;
  if (any) Serial.println("EEPROM: saved");
  return true;
}

// -----------------------------------------------------------------------------
// Carica da EEPROM; false se vuota o magic non valido
// -----------------------------------------------------------------------------
static bool settings_eeprom_load() {
  SettingsBlob b;
  uint8_t* p = (uint8_t*)&b;

  for (size_t i = 0; i < sizeof(b); i++) {
    p[i] = EEPROM.read(SETTINGS_EEPROM_ADDR + i);
  }

  if (b.magic != SETTINGS_MAGIC) {
    Serial.println("EEPROM: empty");
    return false;
  }

  if (b.view < VIEW_COUNT) viewMode = b.view;
  eq_color_mode   = constrain(b.eq_color, 0, 2);
  onda_color_mode = constrain(b.onda_color, 0, 2);
  spectro_res     = constrain(b.spectro_res, 0, 2);
  eq_peak_enable  = (b.eq_peak != 0);
  auto_cycle      = (b.auto_on != 0);

  for (int i = 0; i < VIEW_COUNT && i < 16; i++) {
    if (b.gains[i] > 1e-6f) view_gain[i] = b.gains[i];
  }

  settings_eeprom_ok = true;
  Serial.println("EEPROM: loaded");
  return true;
}

static bool settings_save() {
  bool ok = settings_eeprom_save();
  settings_dirty = false;
  return ok;
}

static bool settings_load() {
  return settings_eeprom_load();
}

// -----------------------------------------------------------------------------
// Chiamare nel loop(). Salva solo se:
//   - c'è una modifica pendente
//   - NON siamo in AUTO
//   - sono passati almeno 5 secondi dall'ultima modifica
// -----------------------------------------------------------------------------
static void settings_autosave_poll() {
  if (!settings_dirty) return;
  if (auto_cycle) return;
  if (millis() - settings_dirty_ms < 5000) return;
  settings_save();
}

#endif // SETTINGS_H
