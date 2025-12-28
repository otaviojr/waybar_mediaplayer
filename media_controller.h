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

#include <gtk/gtk.h>

#include "waybar_mediaplayer.h"

G_BEGIN_DECLS

typedef enum _GtkMediaControllerState
{
  GTK_MEDIA_CONTROLLER_STATE_IDLE,
  GTK_MEDIA_CONTROLLER_STATE_PAUSED,
  GTK_MEDIA_CONTROLLER_STATE_STOPPED,
  GTK_MEDIA_CONTROLLER_STATE_PLAYING,
} GtkMediaControllerState;

typedef struct _GtkMediaController
    GtkMediaController;

typedef struct _GtkMediaControllerClass
    GtkMediaControllerClass;

#define GTK_TYPE_MEDIA_CONTROLLER                         (gtk_media_controller_get_type ())
#define GTK_IS_MEDIA_CONTROLLER(obj)                      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_MEDIA_CONTROLLER))
#define GTK_IS_MEDIA_CONTROLLER_CLASS(klass)              (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_MEDIA_CONTROLLER))
#define GTK_MEDIA_CONTROLLER_GET_CLASS(obj)               (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_MEDIA_CONTROLLER, GtkMediaControllerClass))
#define GTK_MEDIA_CONTROLLER(obj)                         (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_MEDIA_CONTROLLER, GtkMediaController))
#define GTK_MEDIA_CONTROLLER_CLASS(klass)                 (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_MEDIA_CONTROLLER, GtkMediaControllerClass))
#define GTK_MEDIA_CONTROLLER_CAST(obj)                    ((GtkMediaController*)(obj))

GType gtk_media_controller_get_type(void);
GtkMediaController* gtk_media_controller_new(MediaPlayerModConfig*);
gboolean gtk_media_controller_pause(GtkMediaController* self);
gboolean gtk_media_controller_play(GtkMediaController* self);
gboolean gtk_media_controller_toogle(GtkMediaController* self);

G_END_DECLS
