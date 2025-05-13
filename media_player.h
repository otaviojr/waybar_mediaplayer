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

#pragma once

#include <playerctl.h>

#include "media_controller.h"

typedef struct _MediaPlayerManagerCallbacks {
  void (*on_name_added)(PlayerctlPlayerManager*, PlayerctlPlayerName*, gpointer);
  void (*on_name_removed)(PlayerctlPlayerManager*, PlayerctlPlayerName*, gpointer);
  void (*on_player_added)(PlayerctlPlayerManager*, PlayerctlPlayer*, gpointer);
  void (*on_player_removed)(PlayerctlPlayerManager*, PlayerctlPlayer*, gpointer);
} MediaPlayerManagerCallbacks;

typedef struct _MediaPlayerCallbacks {
  void (*on_playback_status)(PlayerctlPlayer*, PlayerctlPlaybackStatus, gpointer);
  void (*on_play)(PlayerctlPlayer*, gpointer);
  void (*on_stop)(PlayerctlPlayer*, gpointer);
  void (*on_pause)(PlayerctlPlayer*, gpointer);
  void (*on_seeked)(PlayerctlPlayer*, gint64, gpointer);
  void (*on_meta)(PlayerctlPlayer*, GVariant*, gpointer);
  void (*on_exit)(PlayerctlPlayer*, gpointer);
} MediaPlayerCallbacks;

typedef struct _GtkMediaPlayer {
  GtkMediaController* parent;

  PlayerctlPlayer* player;
  gboolean available;
  PlayerctlPlaybackStatus status;
  guint64 length;
} GtkMediaPlayer;

PlayerctlPlayerManager* media_player_new(MediaPlayerManagerCallbacks*, gpointer);
gboolean media_player_init(PlayerctlPlayerManager*, MediaPlayerCallbacks*, gpointer);
void media_player_add_player_by_name(PlayerctlPlayerManager*, PlayerctlPlayerName*, MediaPlayerCallbacks*, GError**, gpointer);

GtkMediaPlayer* gtk_media_player_new(GtkMediaController*, PlayerctlPlayer*);
void gtk_media_player_destroy(void*);
int gtk_media_player_compare(const void*, const void*);
guint64 gtk_media_player_get_length(PlayerctlPlayer*);
