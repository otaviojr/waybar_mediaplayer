#include <glib-object.h>
#include <playerctl.h>
#include <string.h>

#include "glib.h"
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

void* 
wbcffi_init(const wbcffi_init_info* init_info, const wbcffi_config_entry* config_entries,
                  size_t config_entries_len) {
  
  printf("waybar_mediapliayer: init config:\n");

  MediaPlayerModConfig* config = g_malloc(sizeof(MediaPlayerModConfig));
  config->scroll_title = TRUE;
  config->title_max_width = 200;
  config->scroll_before_timeout = 5;
  config->scroll_interval=200;
  config->scroll_step=2;
  config->tooltip = TRUE;
  config->tooltip_image_width = 300;
  config->tooltip_image_height = 300;
  config->btn_play = "";
  config->btn_pause="";
  config->btn_prev="";
  config->btn_next="";

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
      config->btn_play = replace_unicode_escapes(config_entries[i].value);
    } else if(strncasecmp("btn-pause-icon", config_entries[i].key,14)==0){
      config->btn_pause = replace_unicode_escapes(config_entries[i].value); 
    } else if(strncasecmp("btn-next-icon", config_entries[i].key,13)==0){
      config->btn_next = replace_unicode_escapes(config_entries[i].value); 
    } else if(strncasecmp("btn-prev-icon", config_entries[i].key,13)==0){
      config->btn_prev = replace_unicode_escapes(config_entries[i].value); 
    } else {
      printf("Property '%s' ignored\n", config_entries[i].key);
    }
  }

  // Allocate the instance object
  MediaPlayerMod* inst = malloc(sizeof(MediaPlayerMod));
  inst->waybar_module = init_info->obj;

  GtkContainer* root = init_info->get_root_widget(init_info->obj);

  inst->container = gtk_media_controller_new(config);
  gtk_container_add(GTK_CONTAINER(root), GTK_WIDGET(inst->container));

  // Return instance object
  printf("waybar_mediaplayer inst=%p: init success ! (%d total instances)\n", inst, ++instance_count);
  return inst;
}

void 
wbcffi_deinit(void* instance) {
  printf("waybar_mediaplayer inst=%p: free memory\n", instance);
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
