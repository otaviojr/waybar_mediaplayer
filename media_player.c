/*
 * Copyright (c) 2025 - Otávio Ribeiro <otavio@otavio.guru>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include "glib-object.h"
#include "glib.h"
#include "playerctl.h"

#include "media_player.h"

PlayerctlPlayerManager* 
media_player_new(MediaPlayerManagerCallbacks* callbacks, gpointer user_data) {
  GError * err = NULL;

  PlayerctlPlayerManager* player_manager = playerctl_player_manager_new(&err);

  if(err != NULL){
    printf("Error initializing playerctl manager: %s",err->message);
    g_clear_error(&err);
    return NULL;
  }

  if(callbacks->on_name_added)
    g_signal_connect(player_manager, "name-appeared", G_CALLBACK(callbacks->on_name_added), user_data);

  if(callbacks->on_name_removed)
    g_signal_connect(player_manager, "name-vanished", G_CALLBACK(callbacks->on_name_removed), user_data);

  if(callbacks->on_player_added)
    g_signal_connect(player_manager, "player-appeared", G_CALLBACK(callbacks->on_player_added), user_data);

  if(callbacks->on_player_removed)
    g_signal_connect(player_manager, "player-vanished", G_CALLBACK(callbacks->on_player_removed), user_data);

  return player_manager;
}

gboolean 
media_player_init(PlayerctlPlayerManager* player_manager, MediaPlayerCallbacks* callbacks, gpointer user_data) {
  GError* err = NULL;
  GValue value = G_VALUE_INIT;
  GList* names = NULL;

  g_value_init(&value, G_TYPE_POINTER); 
  g_object_get_property((GObject*)player_manager, "player-names", &value);

  if (G_VALUE_HOLDS_POINTER(&value)) {
    names = (GList*)g_value_get_pointer(&value);
    for(GList* name = g_list_first(names); name; name = name->next){
      PlayerctlPlayerName* player_name = (PlayerctlPlayerName*)name->data;
      printf("Player name found: %s\n", player_name->name);
      media_player_add_player_by_name(player_manager, player_name, callbacks, &err, user_data);
      if( err != NULL){
        printf("Error creating player by name: %s", err->message);
        g_clear_error(&err);
      }
    }
  } else {
    printf("Error getting list\n");
  }

  g_value_unset(&value);

  return TRUE;
}

void 
media_player_add_player_by_name(PlayerctlPlayerManager* player_manager, PlayerctlPlayerName* name, MediaPlayerCallbacks* callbacks, GError** err, gpointer user_data){
  PlayerctlPlayer* player =  playerctl_player_new_from_name(name, err);
  if( *err == NULL){
    if(callbacks->on_playback_status)
      g_signal_connect(player, "playback-status", G_CALLBACK(callbacks->on_playback_status), user_data);

    if(callbacks->on_play)
      g_signal_connect(player, "play", G_CALLBACK(callbacks->on_play), user_data);

    if(callbacks->on_stop)
      g_signal_connect(player, "stop", G_CALLBACK(callbacks->on_stop), user_data);

    if(callbacks->on_pause)
      g_signal_connect(player, "pause", G_CALLBACK(callbacks->on_pause), user_data);

    if(callbacks->on_meta)
      g_signal_connect(player, "metadata", G_CALLBACK(callbacks->on_meta), user_data);

    if(callbacks->on_seeked)
      g_signal_connect(player, "seeked", G_CALLBACK(callbacks->on_seeked), user_data);

    if(callbacks->on_exit)
      g_signal_connect(player, "exit", G_CALLBACK(callbacks->on_exit), user_data);

    if(callbacks->on_properties)
      g_signal_connect(player, "properties-changed", G_CALLBACK(callbacks->on_properties), user_data);

    playerctl_player_manager_manage_player(player_manager, player);
  } 
}

GtkMediaPlayer* 
gtk_media_player_new(GtkMediaController* parent, PlayerctlPlayer* player) {
  GtkMediaPlayer* media_player = (GtkMediaPlayer*)g_malloc(sizeof(GtkMediaPlayer));
  GError* err = NULL;

  media_player->player = player;
  gchar* title = playerctl_player_get_title(player, &err);
  if(err || !title || strlen(title) == 0){
    media_player->available = FALSE;
  }else{
    media_player->available = TRUE;
  }

  printf("media player created with availability= %d\n", media_player->available);

  GValue val = G_VALUE_INIT;
  g_value_init(&val, PLAYERCTL_TYPE_PLAYBACK_STATUS);
  g_object_get_property(G_OBJECT(player), "playback-status", &val);
  PlayerctlPlaybackStatus playback_status = g_value_get_enum(&val);
  media_player->status = playback_status;
  g_value_unset(&val);

  guint64 length = gtk_media_player_get_length(player);
  media_player->length = length;

  media_player->parent = parent;

  return media_player;
}

void 
gtk_media_player_destroy(void* pointer) {
  printf("gtk_media_player_destroy entered\n");
  GtkMediaPlayer* media_player = (GtkMediaPlayer*)pointer;

  if(media_player->player != NULL){
    g_object_unref(media_player->player);
  }

  g_free(media_player);
  printf("gtk_media_player_destroy exited\n");
}

int 
gtk_media_player_compare(const void* a, const void* b) {
  GtkMediaPlayer* a1 = (GtkMediaPlayer*)a;
  if( a1->player == b) return 0;
  return -1;
}

guint64 
gtk_media_player_get_length(PlayerctlPlayer* player) {
  guint64 length = 0;
  GError* err = NULL;
  gchar* length_str = playerctl_player_print_metadata_prop(player, "mpris:length", &err);
  if(err != NULL || length_str == NULL) return length;
  length = g_ascii_strtoull(length_str, NULL, 10);
  g_free(length_str);
  return length;
}
