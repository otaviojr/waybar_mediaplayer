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

#include <glib.h>
#include <gio/gio.h>

#define MPRIS_PATH "/org/mpris/MediaPlayer2"
#define IFACE_PLAYER "org.mpris.MediaPlayer2.Player"
#define IFACE_PROPS "org.freedesktop.DBus.Properties"

typedef enum _GMprisMediaPlayerState
{
  G_MPRIS_MEDIA_PLAYER_STATE_IDLE,
  G_MPRIS_MEDIA_PLAYER_STATE_PAUSED,
  G_MPRIS_MEDIA_PLAYER_STATE_STOPPED,
  G_MPRIS_MEDIA_PLAYER_STATE_PLAYING,
} GMprisMediaPlayerState;

GType g_mpris_media_player_state_get_type(void);
#define G_TYPE_MPRIS_MEDIA_PLAYER_STATE (g_mpris_media_player_state_get_type())

G_BEGIN_DECLS

typedef struct _GMprisMediaPlayer
    GMprisMediaPlayer;

typedef struct _GMprisMediaPlayerClass
    GMprisMediaPlayerClass;

#define G_TYPE_MPRIS_MEDIA_PLAYER                         (g_mpris_media_player_get_type ())
#define G_IS_MPRIS_MEDIA_PLAYER(obj)                      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_TYPE_MPRIS_MEDIA_PLAYER))
#define G_IS_MPRIS_MEDIA_PLAYER_CLASS(klass)              (G_TYPE_CHECK_CLASS_TYPE ((klass), G_TYPE_MPRIS_MEDIA_PLAYER))
#define G_MPRIS_MEDIA_PLAYER_GET_CLASS(obj)               (G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_MPRIS_MEDIA_PLAYER, GMprisMediaPlayerClass))
#define G_MPRIS_MEDIA_PLAYER(obj)                         (G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_MPRIS_MEDIA_PLAYER, GMprisMediaPlayer))
#define G_MPRIS_MEDIA_PLAYER_CLASS(klass)                 (G_TYPE_CHECK_CLASS_CAST ((klass), G_MPRIS_MEDIA_PLAYER, GMprisMediaPlayerClass))
#define G_MPRIS_MEDIA_PLAYER_CAST(obj)                    ((GtkMprisMediaPlayer*)(obj))

GType g_mpris_media_player_get_type(void);
GMprisMediaPlayer* g_mpris_media_player_new(GDBusConnection*, const char*);
gboolean g_mpris_media_player_is_iface(GMprisMediaPlayer*, const char*);

int g_mpris_media_player_compare(const void* a, const void* b);
gboolean g_is_mpris_media_player_available(GMprisMediaPlayer* self);

void g_mpris_media_player_play(GMprisMediaPlayer* self);
void g_mpris_media_player_pause(GMprisMediaPlayer* self);
void g_mpris_media_player_stop(GMprisMediaPlayer* self);
void g_mpris_media_player_play_pause(GMprisMediaPlayer* self);
void g_mpris_media_player_next(GMprisMediaPlayer* self);
void g_mpris_media_player_previous(GMprisMediaPlayer* self);

G_END_DECLS
