#ifndef SETTINGS_H
#define SETTINGS_H

#if defined(__has_include)
#  if __has_include(<STM32SD.h>)
#    include <STM32SD.h>
#    define SETTINGS_HAS_SD 1
#  else
#    define SETTINGS_HAS_SD 0
#  endif
#else
#  define SETTINGS_HAS_SD 0
#endif

static bool settings_sd_ok = false;
static bool settings_sd_checked = false;
static bool settings_dirty = false;
static bool settings_loaded_ok = false;
static uint32_t settings_dirty_ms = 0;
static uint32_t settings_last_retry_ms = 0;

extern int viewMode;
extern int eq_color_mode;
extern int onda_color_mode;
extern int spectro_res;
extern bool eq_peak_enable;
extern float view_gain[];

// Testo stato per la riga messaggi
static const char* settings_status_text() {
#if !SETTINGS_HAS_SD
  return "No SD lib";
#else
  if (settings_sd_ok) return settings_loaded_ok ? "SD OK" : "SD OK (new)";
  return "No SD";
#endif
}

static void settings_mark_dirty() {
  settings_dirty = true;
  settings_dirty_ms = millis();
}

#if SETTINGS_HAS_SD

static bool settings_begin_sd() {
  if (settings_sd_ok) return true;
  if (settings_sd_checked && (millis() - settings_last_retry_ms < 5000)) return false;
  settings_last_retry_ms = millis();
  settings_sd_checked = true;
  if (!SD.begin()) {
    settings_sd_ok = false;
    return false;
  }
  settings_sd_ok = true;
  return true;
}

static void settings_sd_lost() {
  settings_sd_ok = false;
  settings_sd_checked = false;
}

static void settings_apply_key(const char* key, const char* val) {
  if (strcmp(key, "view") == 0) {
    int v = atoi(val);
    if (v >= 0 && v < VIEW_COUNT) viewMode = v;
  } else if (strcmp(key, "eq_color") == 0) {
    eq_color_mode = constrain(atoi(val), 0, 2);
  } else if (strcmp(key, "onda_color") == 0) {
    onda_color_mode = constrain(atoi(val), 0, 2);
  } else if (strcmp(key, "spectro_res") == 0) {
    spectro_res = constrain(atoi(val), 0, 2);
  } else if (strcmp(key, "eq_peak") == 0) {
    eq_peak_enable = (atoi(val) != 0);
  } else if (strncmp(key, "gain", 4) == 0) {
    int idx = atoi(key + 4);
    if (idx >= 0 && idx < VIEW_COUNT) {
      view_gain[idx] = atof(val);
      if (view_gain[idx] < 1e-6f) view_gain[idx] = 1e-6f;
    }
  }
}

static bool settings_save() {
  if (!settings_begin_sd()) return false;
  if (SD.exists("SETTINGS.TXT")) SD.remove("SETTINGS.TXT");
  File f = SD.open("SETTINGS.TXT", FILE_WRITE);
  if (!f) { settings_sd_lost(); return false; }

  f.println("# AudioVisualizer");
  f.print("view="); f.println(viewMode);
  f.print("eq_color="); f.println(eq_color_mode);
  f.print("onda_color="); f.println(onda_color_mode);
  f.print("spectro_res="); f.println(spectro_res);
  f.print("eq_peak="); f.println(eq_peak_enable ? 1 : 0);
  for (int i = 0; i < VIEW_COUNT; i++) {
    f.print("gain"); f.print(i); f.print("=");
    f.println(view_gain[i], 6);
  }
  f.close();
  settings_dirty = false;
  Serial.println("Settings SAVED");
  return true;
}

static bool settings_load() {
  settings_loaded_ok = false;
  if (!settings_begin_sd()) return false;
  if (!SD.exists("SETTINGS.TXT")) return false;
  File f = SD.open("SETTINGS.TXT", FILE_READ);
  if (!f) { settings_sd_lost(); return false; }

  char line[64];
  while (f.available()) {
    int n = 0;
    while (f.available() && n < 62) {
      char c = f.read();
      if (c == '\n' || c == '\r') break;
      line[n++] = c;
    }
    line[n] = 0;
    if (n == 0 || line[0] == '#') continue;
    char* eq = strchr(line, '=');
    if (!eq) continue;
    *eq = 0;
    const char* key = line;
    const char* val = eq + 1;
    while (*key == ' ') key++;
    while (*val == ' ') val++;
    settings_apply_key(key, val);
  }
  f.close();
  settings_dirty = false;
  settings_loaded_ok = true;
  return true;
}

static void settings_autosave_poll() {
  if (!settings_dirty) return;
  if (millis() - settings_dirty_ms < 1500) return;
  settings_save();
}

#else
static bool settings_save() { settings_dirty = false; return false; }
static bool settings_load() { return false; }
static void settings_autosave_poll() { settings_dirty = false; }
#endif

#endif
