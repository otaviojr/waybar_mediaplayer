#define G_LOG_DOMAIN "waybarmediaplayer.main"

#include <glib.h>
#include <glib-object.h>
#include <string.h>

#include "waybar_mediaplayer.h"
#include "media_controller.h"


// This static variable is shared between all instances of this module
static int instance_count = 0;

// You must
const size_t wbcffi_version = 2;

static gchar* 
replace_unicode_escapes(const gchar* input) {
    GString *result = g_string_new("");
    const gchar *p = input;

    while (*p) {
        if (g_str_has_prefix(p, "\\u") &&
            g_ascii_isxdigit(p[2]) && g_ascii_isxdigit(p[3]) &&
            g_ascii_isxdigit(p[4]) && g_ascii_isxdigit(p[5])) {

            gchar hex[5] = {p[2], p[3], p[4], p[5], 0};
            gunichar codepoint = (gunichar)g_ascii_strtoull(hex, NULL, 16);

            gchar utf8[6] = {0};
            gint len = g_unichar_to_utf8(codepoint, utf8);
            utf8[len] = '\0';

            g_string_append(result, utf8);
            p += 6;
        } else {
            p++;
        }
    }
    return g_string_free_and_steal(result);
}

static gchar* replace_string_glib(const gchar* input, const gchar* find, const gchar* replace) {
    // Create GString from gchar*
    GString* gstr = g_string_new(input);
    
    // Replace all occurrences (0 = no limit)
    g_string_replace(gstr, find, replace, 0);
    
    // Extract gchar* and free GString structure (but not the string data)
    return g_string_free(gstr, FALSE);
}

void* 
wbcffi_init(const wbcffi_init_info* init_info, const wbcffi_config_entry* config_entries,
                  size_t config_entries_len) {
  
  g_info("waybar_mediapliayer initialized");

  MediaPlayerModConfig* config = g_malloc(sizeof(MediaPlayerModConfig));
  config->scroll_title = TRUE;
  config->title_max_width = 200;
  config->scroll_before_timeout = 5;
  config->scroll_interval=200;
  config->scroll_step=2;
  config->tooltip = TRUE;
  config->tooltip_image_width = 300;
  config->tooltip_image_height = 300;
  config->btn_play = g_strdup("");
  config->btn_pause = g_strdup("");
  config->btn_prev = g_strdup("");
  config->btn_next = g_strdup("");
  config->ignored_players = g_strdup("");

  for (size_t i = 0; i < config_entries_len; i++) {
    if(strncasecmp("scroll-before-timeout", config_entries[i].key,21)==0){
      config->scroll_before_timeout = g_ascii_strtoull(config_entries[i].value, NULL, 10); 
    } else if(strncasecmp("scroll-interval", config_entries[i].key,15)==0){
      config->scroll_interval = g_ascii_strtoull(config_entries[i].value, NULL, 10);  
    } else if(strncasecmp("scroll-title", config_entries[i].key,12)==0){
      if(strncasecmp("true", config_entries[i].value,4)==0){
        config->scroll_title = TRUE;
      } else {
        config->scroll_title = FALSE;
      }
    } else if(strncasecmp("title-max-width", config_entries[i].key,15)==0){
      config->title_max_width = g_ascii_strtoull(config_entries[i].value, NULL, 10); 
    } else if(strncasecmp("scroll-step", config_entries[i].key,11)==0) {
      config->scroll_step = g_ascii_strtoull(config_entries[i].value, NULL, 10); 
    } else if(strncasecmp("tooltip-image-width", config_entries[i].key,19)==0){
      config->tooltip_image_width = g_ascii_strtoull(config_entries[i].value, NULL, 10); 
    } else if(strncasecmp("tooltip-image-height", config_entries[i].key,20)==0){
      config->tooltip_image_height = g_ascii_strtoull(config_entries[i].value, NULL, 10); 
    } else if(strncasecmp("tooltip", config_entries[i].key,7)==0) {
      if(strncasecmp("true", config_entries[i].value,4)==0){
        config->tooltip = TRUE;
      } else {
        config->tooltip = FALSE;
      }
    } else if(strncasecmp("btn-play-icon", config_entries[i].key,13)==0){
      g_free(config->btn_play);
      config->btn_play = replace_unicode_escapes(config_entries[i].value);
    } else if(strncasecmp("btn-pause-icon", config_entries[i].key,14)==0){
      g_free(config->btn_pause);
      config->btn_pause = replace_unicode_escapes(config_entries[i].value); 
    } else if(strncasecmp("btn-next-icon", config_entries[i].key,13)==0){
      g_free(config->btn_next);
      config->btn_next = replace_unicode_escapes(config_entries[i].value); 
    } else if(strncasecmp("btn-prev-icon", config_entries[i].key,13)==0){
      g_free(config->btn_prev);
      config->btn_prev = replace_unicode_escapes(config_entries[i].value); 
    } else if(strncasecmp("ignored-players", config_entries[i].key,15)==0){
      g_free(config->ignored_players);

      gchar* ignored_players = g_strdup(config_entries[i].value);
      ignored_players = replace_string_glib(ignored_players, "\"", ""); 
      ignored_players = replace_string_glib(ignored_players, "\'", "");
      ignored_players = replace_string_glib(ignored_players, "\n", ""); 

      config->ignored_players = ignored_players;
      } else {
      g_warning("Property '%s' ignored", config_entries[i].key);
    }
  }

  // Allocate the instance object
  MediaPlayerMod* inst = malloc(sizeof(MediaPlayerMod));
  inst->waybar_module = init_info->obj;

  GtkContainer* root = init_info->get_root_widget(init_info->obj);

  inst->container = gtk_media_controller_new(config);
  gtk_container_add(GTK_CONTAINER(root), GTK_WIDGET(inst->container));

  // Return instance object
  g_info("waybar_mediaplayer inst=%p: init success ! (%d total instances)\n", inst, ++instance_count);
  return inst;
}

void 
wbcffi_deinit(void* instance) {
  g_info("waybar_mediaplayer inst=%p: free memory\n", instance);
  free(instance);
}

void 
wbcffi_update(void* instance) { printf("waybar_mediaplayer inst=%p: Update request\n", instance); }

void 
wbcffi_refresh(void* instance, int signal) {
}

void 
wbcffi_doaction(void* instance, const char* name) {
  printf("waybar_mediaplayer inst=%p: doAction(%s)\n", instance, name);
}
