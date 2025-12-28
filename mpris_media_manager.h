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

#define MPRIS_PREFIX      "org.mpris.MediaPlayer2."
#define MPRIS_PATH        "/org/mpris/MediaPlayer2"
#define IFACE_PLAYER      "org.mpris.MediaPlayer2.Player"
#define IFACE_PROPS       "org.freedesktop.DBus.Properties"
#define IFACE_DBUS        "org.freedesktop.DBus"
#define DBUS_NAME         "org.freedesktop.DBus"
#define DBUS_PATH         "/org/freedesktop/DBus"

G_BEGIN_DECLS

typedef struct _GMprisMediaManager
    GMprisMediaManager;

typedef struct _GMprisMediaManagerClass
    GMprisMediaManagerClass;

#define G_TYPE_MPRIS_MEDIA_MANAGER                         (g_mpris_media_manager_get_type ())
#define G_IS_MPRIS_MEDIA_MANAGER(obj)                      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_TYPE_MPRIS_MEDIA_MANAGER))
#define G_IS_MPRIS_MEDIA_MANAGER_CLASS(klass)              (G_TYPE_CHECK_CLASS_TYPE ((klass), G_TYPE_MPRIS_MEDIA_MANAGER))
#define G_MPRIS_MEDIA_MANAGER_GET_CLASS(obj)               (G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_MPRIS_MEDIA_MANAGER, GMprisMediaManagerClass))
#define G_MPRIS_MEDIA_MANAGER(obj)                         (G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_MPRIS_MEDIA_MANAGER, GMprisMediaManager))
#define G_MPRIS_MEDIA_MANAGER_CLASS(klass)                 (G_TYPE_CHECK_CLASS_CAST ((klass), G_MPRIS_MEDIA_MANAGER, GMprisMediaManagerClass))
#define G_MPRIS_MEDIA_MANAGER_CAST(obj)                    ((GtkMprisMediaManager*)(obj))

GType g_mpris_media_manager_get_type(void);
GMprisMediaManager* g_mpris_media_manager_new();

void g_mpris_media_manager_start(GMprisMediaManager* self);

G_END_DECLS
