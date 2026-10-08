#ifndef SETTINGS_H
#define SETTINGS_H

// =============================================================================
// Settings.h — EEPROM (caricare DOPO audio)
// - AUTO viene salvato e ripristinato al boot
// - Durante AUTO non si salva a ogni cambio vista; si salva il toggle AUTO
// =============================================================================

#include <EEPROM.h>
#include <string.h>

#define SETTINGS_MAGIC       0xAD
#define SETTINGS_EEPROM_ADDR 0

static bool     settings_dirty      = false;
static bool     settings_force      = false;  // true = salva anche se AUTO attivo
static bool     settings_eeprom_ok  = false;
static bool     settings_just_saved = false;  // UI: mostra SaveEE in rosso
static uint32_t settings_dirty_ms   = 0;
static uint32_t settings_saved_ms   = 0;

extern int   viewMode;
extern int   eq_color_mode;
extern int   onda_color_mode;
extern int   spectro_res;
extern bool  eq_peak_enable;
extern bool  auto_cycle;
extern float view_gain[];

// Testo stato: SaveEE per ~2s dopo scrittura, poi EEPROMok / NoSave
static const char* settings_status_text() {
  if (settings_just_saved && (millis() - settings_saved_ms < 2000))
    return "SaveEE";
  settings_just_saved = false;
  return settings_eeprom_ok ? "EEPROMok" : "NoSave";
}

static bool settings_status_is_saving() {
  return settings_just_saved && (millis() - settings_saved_ms < 2000);
}

// force=true: salva anche con AUTO on (toggle AUTO, ecc.)
static void settings_mark_dirty(bool force = false) {
  if (auto_cycle && !force) return;
  settings_dirty    = true;
  settings_dirty_ms = millis();
  if (force) settings_force = true;
}

struct SettingsBlob {
  uint8_t magic;
  uint8_t view;
  uint8_t eq_color;
  uint8_t onda_color;
  uint8_t spectro_res;
  uint8_t eq_peak;
  uint8_t auto_on;     // 1 = AUTO attivo al prossimo boot
  float   gains[16];
};

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
  for (int i = 0; i < VIEW_COUNT && i < 16; i++) b.gains[i] = view_gain[i];

  const uint8_t* p = (const uint8_t*)&b;
  for (size_t i = 0; i < sizeof(b); i++) {
    if (EEPROM.read(SETTINGS_EEPROM_ADDR + i) != p[i])
      EEPROM.write(SETTINGS_EEPROM_ADDR + i, p[i]);
  }
  settings_eeprom_ok  = true;
  settings_just_saved = true;
  settings_saved_ms   = millis();
  Serial.println("EEPROM: saved (incl. AUTO)");
  return true;
}

static bool settings_eeprom_load() {
  uint8_t magic = EEPROM.read(SETTINGS_EEPROM_ADDR);
  if (magic != SETTINGS_MAGIC) return false;

  SettingsBlob b;
  uint8_t* p = (uint8_t*)&b;
  for (size_t i = 0; i < sizeof(b); i++)
    p[i] = EEPROM.read(SETTINGS_EEPROM_ADDR + i);

  if (b.magic != SETTINGS_MAGIC) return false;
  if (b.view < VIEW_COUNT) viewMode = b.view;
  eq_color_mode   = constrain(b.eq_color, 0, 2);
  onda_color_mode = constrain(b.onda_color, 0, 2);
  spectro_res     = constrain(b.spectro_res, 0, 2);
  eq_peak_enable  = (b.eq_peak != 0);
  auto_cycle      = (b.auto_on != 0);   // ripristina AUTO
  for (int i = 0; i < VIEW_COUNT && i < 16; i++) {
    if (b.gains[i] > 1e-6f) view_gain[i] = b.gains[i];
  }
  settings_eeprom_ok = true;
  Serial.print("EEPROM: loaded, AUTO=");
  Serial.println(auto_cycle ? "ON" : "OFF");
  return true;
}

static bool settings_save() {
  bool ok = settings_eeprom_save();
  settings_dirty = false;
  settings_force = false;
  return ok;
}

static bool settings_load() {
  return settings_eeprom_load();
}

// Autosave: 5s dopo modifica; con AUTO solo se force (toggle AUTO)
static void settings_autosave_poll() {
  if (!settings_dirty) return;
  if (auto_cycle && !settings_force) return;
  if (millis() - settings_dirty_ms < 5000) return;
  settings_save();
}

#endif
