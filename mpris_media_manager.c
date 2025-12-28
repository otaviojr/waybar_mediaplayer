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
#define G_LOG_DOMAIN "waybarmediaplayer.media-manager"

#include <glib.h>

#include "mpris_media_player.h"
#include "mpris_media_manager.h"

struct _GMprisMediaManager
{
  GObject parent;

  GDBusConnection *conn;

  guint name_owner_sub_id;

  GList* media_players;
};

struct _GMprisMediaManagerClass
{
  GObjectClass parent_class;
};

enum
{
  G_MPRIS_MEDIA_MANAGER_PROP_0,
  G_MPRIS_MEDIA_MANAGER_PROP_LAST
};

enum
{
  G_MPRIS_MEDIA_MANAGER_SIGNAL_0,
  G_MPRIS_MEDIA_MANAGER_SIGNAL_PLAYER_ADDED,
  G_MPRIS_MEDIA_MANAGER_SIGNAL_PLAYER_REMOVED,
  G_MPRIS_MEDIA_MANAGER_SIGNAL_LAST
};

G_DEFINE_TYPE (GMprisMediaManager, g_mpris_media_manager, G_TYPE_OBJECT);

static GParamSpec
* g_mpris_media_manager_param_specs[G_MPRIS_MEDIA_MANAGER_PROP_LAST] = { NULL, };

static guint
g_mpris_media_manager_signals[G_MPRIS_MEDIA_MANAGER_SIGNAL_LAST] = {0, };

static void
g_mpris_media_manager_get_property(GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GMprisMediaManager *self = G_MPRIS_MEDIA_MANAGER(object);
  (void)self;

  switch (prop_id) {
//    case GTK_MEDIA_CONTROLLER_PROP_STATE:
//      g_value_set_enum(value, self->state);
//      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
g_mpris_media_manager_set_property(GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GMprisMediaManager *self = G_MPRIS_MEDIA_MANAGER(object);

  switch (prop_id) {
//    case GTK_MEDIA_CONTROLLER_PROP_STATE:
//      printf("Setting up controller state\n");
//      GtkMediaControllerState state = g_value_get_enum(value);
//      if(self->state != state){
//        self->state = state;
//        g_signal_emit(self,
//                      gtk_media_controller_signals[GTK_MEDIA_CONTROLLER_SIGNAL_STATE_CHANGED],
//                      0, self->state);
//      }
//      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
g_mpris_media_manager_finalize(GObject * object)
{
  g_debug("g_mpris_media_manager_finalize entered");
  GMprisMediaManager *self = G_MPRIS_MEDIA_MANAGER(object);

  if (self->media_players) {
    g_list_free_full(self->media_players, g_object_unref);
    self->media_players = NULL;
  }

  if (self->name_owner_sub_id) {
    g_dbus_connection_signal_unsubscribe(self->conn, self->name_owner_sub_id);
    self->name_owner_sub_id = 0;
  }

  g_object_unref(self->conn);
  self->conn = NULL;

  G_OBJECT_CLASS
      (g_mpris_media_manager_parent_class)->finalize(object);

  g_debug("g_mrpis_media_manager_finalized exited");
}

static void
g_mpris_media_manager_constructed(GObject* object)
{
  GMprisMediaManager *self = G_MPRIS_MEDIA_MANAGER(object);
  g_debug("g_mpris_media_manager_constructed entered");
  g_debug("g_mpris_media_manager_constructed exited");
}

static void
g_mpris_media_manager_class_init(GMprisMediaManagerClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = g_mpris_media_manager_set_property;
  gobject_class->get_property = g_mpris_media_manager_get_property;
  gobject_class->finalize = g_mpris_media_manager_finalize;
  gobject_class->constructed = g_mpris_media_manager_constructed;

  g_mpris_media_manager_signals[G_MPRIS_MEDIA_MANAGER_SIGNAL_PLAYER_ADDED] =
    g_signal_new("player-added",
                  G_TYPE_FROM_CLASS(gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                  0,
                  NULL,
                  NULL,
                  NULL,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_MPRIS_MEDIA_PLAYER);

  g_mpris_media_manager_signals[G_MPRIS_MEDIA_MANAGER_SIGNAL_PLAYER_REMOVED] =
    g_signal_new("player-removed",
                  G_TYPE_FROM_CLASS(gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                  0,
                  NULL,
                  NULL,
                  NULL,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_MPRIS_MEDIA_PLAYER);

  g_object_class_install_properties (gobject_class,
      G_MPRIS_MEDIA_MANAGER_PROP_LAST, g_mpris_media_manager_param_specs);
}

static void
g_mpris_media_manager_init(GMprisMediaManager * self)
{
  g_debug("Initializing media player");
}

GMprisMediaManager*
g_mpris_media_manager_new(){
  g_debug("g_mpris_media_manager_new entered");
  GMprisMediaManager* self = g_object_new(G_TYPE_MPRIS_MEDIA_MANAGER, NULL);

  self->conn = NULL;
  self->name_owner_sub_id = 0;
  self->media_players = NULL;

  return self;
}

static gboolean is_mpris_name(const char *name) {
  return name && g_str_has_prefix(name, MPRIS_PREFIX);
}

static void 
mpris_media_manager_add_player(GMprisMediaManager* self, const char* iface){
  g_return_if_fail(G_IS_MPRIS_MEDIA_MANAGER(self));

  if(!is_mpris_name(iface)){
    return;
  }

  GMprisMediaPlayer* player = g_mpris_media_player_new(self->conn, iface);
  self->media_players = g_list_append(self->media_players, player);

  g_signal_emit(self, g_mpris_media_manager_signals[G_MPRIS_MEDIA_MANAGER_SIGNAL_PLAYER_ADDED], 0, player);
}

static void 
mpris_media_manager_remove_player(GMprisMediaManager* self, const char* iface){
  g_return_if_fail(G_IS_MPRIS_MEDIA_MANAGER(self));

  if(!is_mpris_name(iface)){
    return;
  }

  GList* item = g_list_first(self->media_players);
  if(item){
    for(GList* next = item; next; next = next->next){
      GMprisMediaPlayer* player = (GMprisMediaPlayer*)next->data;

      if(g_mpris_media_player_is_iface(player, iface)){

        g_signal_emit(self, g_mpris_media_manager_signals[G_MPRIS_MEDIA_MANAGER_SIGNAL_PLAYER_REMOVED], 0, player);

        self->media_players = g_list_remove_link(self->media_players, item);
        g_list_free_full(item, g_object_unref);
        break;
      }
    }
  }
}


static void 
on_name_owner_changed(GDBusConnection *c,
                      const gchar *sender_name,
                      const gchar *object_path,
                      const gchar *interface_name,
                      const gchar *signal_name,
                      GVariant *parameters,
                      gpointer user_data) {

  (void)c; (void)sender_name; (void)object_path; (void)interface_name; (void)signal_name;

  g_debug("on_name_owner_changed entered");

  GMprisMediaManager *self = G_MPRIS_MEDIA_MANAGER(user_data);

  const char *name = NULL;
  const char *old_owner = NULL;
  const char *new_owner = NULL;
  g_variant_get(parameters, "(&s&s&s)", &name, &old_owner, &new_owner);


  if(is_mpris_name(name)) {
    if (new_owner[0] != '\0') {
      g_info("mpris player added: %s;%s;%s", name, old_owner, new_owner);
      mpris_media_manager_add_player(self,name);
    } else {
      g_info("mpris player removed: %s;%s;%s", name, old_owner, new_owner);
      mpris_media_manager_remove_player(self,name);
     }
  }
  g_debug("on_name_owner_changed exited");
}


static void collect_all_players(GMprisMediaManager *self) {
  GError *err = NULL;

  GVariant *ret = g_dbus_connection_call_sync(
      self->conn,
      DBUS_NAME,
      DBUS_PATH,
      IFACE_DBUS,
      "ListNames",
      NULL,
      G_VARIANT_TYPE("(as)"),
      G_DBUS_CALL_FLAGS_NONE,
      -1,
      NULL,
      &err);

  if (!ret) {
    g_error("ListNames failed: %s\n", err ? err->message : "unknown");
    g_clear_error(&err);
    return;
  }

  GVariantIter *iter = NULL;
  g_variant_get(ret, "(as)", &iter);

  const char *name = NULL;
  while (g_variant_iter_next(iter, "&s", &name)) {
    if (is_mpris_name(name)) {
      g_info("new mpris player found: %s", name);
      mpris_media_manager_add_player(self, name);
    }
  }

  g_variant_iter_free(iter);
  g_variant_unref(ret);
}

void
g_mpris_media_manager_start(GMprisMediaManager* self){
  g_return_if_fail(G_IS_MPRIS_MEDIA_MANAGER(self));

  if(!self->conn){
    GError *err = NULL;
    self->conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &err);
    if (!self->conn) {
      g_error("failed to connect to session bus: %s", err ? err->message : "unknown");
      g_clear_error(&err);
      return;
    }

    self->name_owner_sub_id = g_dbus_connection_signal_subscribe(
      self->conn,
      DBUS_NAME,
      IFACE_DBUS,
      "NameOwnerChanged",
      DBUS_PATH,
      NULL,
      G_DBUS_SIGNAL_FLAGS_NONE,
      on_name_owner_changed,
      self,
      NULL);
  }

  collect_all_players(self);
}

