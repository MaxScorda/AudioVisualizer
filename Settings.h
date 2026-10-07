#ifndef SETTINGS_H
#define SETTINGS_H

// Solo EEPROM — NESSUNA SD nel visualizer (SD.begin blocca l'audio)
// La SD si testa a parte con SdTest_F746

#include <EEPROM.h>
#include <string.h>

#define SETTINGS_MAGIC 0xA8
#define SETTINGS_EEPROM_ADDR 0

static bool settings_dirty = false;
static bool settings_eeprom_ok = false;
static uint32_t settings_dirty_ms = 0;

extern int viewMode;
extern int eq_color_mode;
extern int onda_color_mode;
extern int spectro_res;
extern bool eq_peak_enable;
extern float view_gain[];

static const char* settings_status_text() {
  return settings_eeprom_ok ? "EEPROMok" : "NoSave";
}

static void settings_mark_dirty() {
  settings_dirty = true;
  settings_dirty_ms = millis();
}

struct SettingsBlob {
  uint8_t magic;
  uint8_t view;
  uint8_t eq_color;
  uint8_t onda_color;
  uint8_t spectro_res;
  uint8_t eq_peak;
  float gains[16];
};

static bool settings_eeprom_save() {
  SettingsBlob b;
  memset(&b, 0, sizeof(b));
  b.magic = SETTINGS_MAGIC;
  b.view = (uint8_t)viewMode;
  b.eq_color = (uint8_t)eq_color_mode;
  b.onda_color = (uint8_t)onda_color_mode;
  b.spectro_res = (uint8_t)spectro_res;
  b.eq_peak = eq_peak_enable ? 1 : 0;
  for (int i = 0; i < VIEW_COUNT && i < 16; i++) b.gains[i] = view_gain[i];

  const uint8_t* p = (const uint8_t*)&b;
  for (size_t i = 0; i < sizeof(b); i++) {
    uint8_t old = EEPROM.read(SETTINGS_EEPROM_ADDR + i);
    if (old != p[i]) {
      EEPROM.write(SETTINGS_EEPROM_ADDR + i, p[i]);
    }
  }
  settings_eeprom_ok = true;
  return true;
}

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
  eq_color_mode = constrain(b.eq_color, 0, 2);
  onda_color_mode = constrain(b.onda_color, 0, 2);
  spectro_res = constrain(b.spectro_res, 0, 2);
  eq_peak_enable = (b.eq_peak != 0);
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

// Autosave: max ogni 3s, solo EEPROM (niente SD)
static void settings_autosave_poll() {
  if (!settings_dirty) return;
  if (millis() - settings_dirty_ms < 3000) return;
  settings_save();
}

#endif
