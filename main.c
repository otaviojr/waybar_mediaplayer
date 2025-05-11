#include <glib-object.h>
#include <playerctl.h>

#include "waybar_mediaplayer.h"
#include "media_controller.h"

// This static variable is shared between all instances of this module
static int instance_count = 0;

// You must
const size_t wbcffi_version = 2;

void* wbcffi_init(const wbcffi_init_info* init_info, const wbcffi_config_entry* config_entries,
                  size_t config_entries_len) {
  
  printf("waybar_mediapliayer: init config:\n");

  for (size_t i = 0; i < config_entries_len; i++) {
    printf("  %s = %s\n", config_entries[i].key, config_entries[i].value);
  }

  // Allocate the instance object
  MediaPlayerMod* inst = malloc(sizeof(MediaPlayerMod));
  inst->waybar_module = init_info->obj;

  GtkContainer* root = init_info->get_root_widget(init_info->obj);

  inst->container = gtk_media_controller_new();
  gtk_container_add(GTK_CONTAINER(root), GTK_WIDGET(inst->container));

  // Return instance object
  printf("waybar_mediaplayer inst=%p: init success ! (%d total instances)\n", inst, ++instance_count);
  return inst;
}

void wbcffi_deinit(void* instance) {
  printf("waybar_mediaplayer inst=%p: free memory\n", instance);
  free(instance);
}

void wbcffi_update(void* instance) { printf("waybar_mediaplayer inst=%p: Update request\n", instance); }

void wbcffi_refresh(void* instance, int signal) {
}

void wbcffi_doaction(void* instance, const char* name) {
  printf("waybar_mediaplayer inst=%p: doAction(%s)\n", instance, name);
}
