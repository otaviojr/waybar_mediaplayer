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

#define G_LOG_DOMAIN "waybarmediaplayer.media-player"

#include <glib.h>
#include <gio/gio.h>
#include <glib-object.h>

#include "mpris_media_player.h"

struct _GMprisMediaPlayer
{
  GObject parent;
  GDBusConnection *conn;
  const char* iface;

  GDBusProxy *player_proxy;
  guint props_sub_id;
  guint seeked_sub_id;

  GMprisMediaPlayerState state;
  gchar *title;
  gchar *artist;
  gchar *arturl;

  gint64 last_known_position;
  gint64 position;

  gint64 length;
  gboolean can_go_next;
  gboolean can_go_previous;
  gboolean can_play;
  gboolean can_control;

  guint position_timer_id;
  GTimer *position_timer; 
};

struct _GMprisMediaPlayerClass
{
  GObjectClass parent_class;
};

enum
{
  G_MPRIS_MEDIA_PLAYER_PROP_0,
  G_MPRIS_MEDIA_PLAYER_PROP_CONNECTION,
  G_MPRIS_MEDIA_PLAYER_PROP_IFACE,
  G_MPRIS_MEDIA_PLAYER_PROP_STATE,
  G_MPRIS_MEDIA_PLAYER_PROP_ARTIST,
  G_MPRIS_MEDIA_PLAYER_PROP_ARTURL,
  G_MPRIS_MEDIA_PLAYER_PROP_TITLE,
  G_MPRIS_MEDIA_PLAYER_PROP_POSITION,
  G_MPRIS_MEDIA_PLAYER_PROP_LENGTH,
  G_MPRIS_MEDIA_PLAYER_PROP_CAN_GO_NEXT,
  G_MPRIS_MEDIA_PLAYER_PROP_CAN_GO_PREVIOUS,
  G_MPRIS_MEDIA_PLAYER_PROP_CAN_PLAY,
  G_MPRIS_MEDIA_PLAYER_PROP_CAN_CONTROL,
  G_MPRIS_MEDIA_PLAYER_PROP_LAST
};

enum
{
  G_MPRIS_MEDIA_PLAYER_SIGNAL_0,
  G_MPRIS_MEDIA_PLAYER_SIGNAL_PROPERTY_CHANGED,
  G_MPRIS_MEDIA_PLAYER_SIGNAL_STATE_CHANGED,
  G_MPRIS_MEDIA_PLAYER_SIGNAL_META_CHANGED,
  G_MPRIS_MEDIA_PLAYER_SIGNAL_LAST
};

G_DEFINE_TYPE (GMprisMediaPlayer, g_mpris_media_player, G_TYPE_OBJECT);

static GParamSpec
* g_mpris_media_player_param_specs[G_MPRIS_MEDIA_PLAYER_PROP_LAST] = { NULL, };

static guint
g_mpris_media_player_signals[G_MPRIS_MEDIA_PLAYER_SIGNAL_LAST] = {0, };

GType
g_mpris_media_player_state_get_type(void)
{
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    static const GEnumValue values[] = {
      { G_MPRIS_MEDIA_PLAYER_STATE_IDLE, "G_MPRIS_MEDIA_PLAYER_STATE_IDLE", "idle" },
      { G_MPRIS_MEDIA_PLAYER_STATE_STOPPED, "G_MPRIS_MEDIA_PLAYER_STATE_STOPPED", "stopped" },
      { G_MPRIS_MEDIA_PLAYER_STATE_PLAYING, "G_MPRIS_MEDIA_PLAYER_STATE_PLAYING", "playing" },
      { G_MPRIS_MEDIA_PLAYER_STATE_PAUSED, "G_MPRIS_MEDIA_PLAYER_STATE_PAUSED", "paused" },
      { 0, NULL, NULL }
    };
    type = g_enum_register_static("GMprisMediaPlayerState", values);
  }
  return type;
}


static void
g_mpris_media_player_get_property(GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GMprisMediaPlayer *self = G_MPRIS_MEDIA_PLAYER(object);

  switch (prop_id) {
    case G_MPRIS_MEDIA_PLAYER_PROP_CONNECTION:
      g_value_set_object(value, self->conn);
      break;
    case G_MPRIS_MEDIA_PLAYER_PROP_IFACE:
      g_value_set_string(value, self->iface);
      break;
    case G_MPRIS_MEDIA_PLAYER_PROP_STATE:
      g_value_set_enum(value, self->state);
      break;
    case G_MPRIS_MEDIA_PLAYER_PROP_TITLE:
      g_value_set_string(value, self->title);
      break;
    case G_MPRIS_MEDIA_PLAYER_PROP_ARTIST:
      g_value_set_string(value, self->artist);
      break;
    case G_MPRIS_MEDIA_PLAYER_PROP_ARTURL:
      g_value_set_string(value, self->arturl);
      break;
    case G_MPRIS_MEDIA_PLAYER_PROP_POSITION:
      g_value_set_int64(value, self->position);
      break;
    case G_MPRIS_MEDIA_PLAYER_PROP_LENGTH:
      g_value_set_int64(value, self->length);
      break;    
    case G_MPRIS_MEDIA_PLAYER_PROP_CAN_GO_NEXT:
      g_value_set_boolean(value, self->can_go_next);
      break;
    case G_MPRIS_MEDIA_PLAYER_PROP_CAN_GO_PREVIOUS:
      g_value_set_boolean(value, self->can_go_previous);
      break;
    case G_MPRIS_MEDIA_PLAYER_PROP_CAN_PLAY:
      g_value_set_boolean(value, self->can_play);
      break;
    case G_MPRIS_MEDIA_PLAYER_PROP_CAN_CONTROL:
      g_value_set_boolean(value, self->can_control);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
g_mpris_media_player_set_property(GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GMprisMediaPlayer *self = G_MPRIS_MEDIA_PLAYER(object);

  switch (prop_id) {
    case G_MPRIS_MEDIA_PLAYER_PROP_CONNECTION:
      if (self->conn) g_object_unref(self->conn);
      self->conn = g_value_dup_object(value);
      break;
    case G_MPRIS_MEDIA_PLAYER_PROP_IFACE:
      g_free((void*)self->iface);
      self->iface = g_value_dup_string(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
position_update_timer_callback(gpointer user_data)
{
  GMprisMediaPlayer *self = G_MPRIS_MEDIA_PLAYER(user_data);
  
  if (self->state != G_MPRIS_MEDIA_PLAYER_STATE_PLAYING) {
    return G_SOURCE_CONTINUE;
  }
  
  gdouble elapsed_seconds = g_timer_elapsed(self->position_timer, NULL);
  gint64 estimated_position = self->last_known_position + (gint64)(elapsed_seconds * 1000000);
  
  if (llabs(estimated_position - self->position) > 100000) {
    self->position = estimated_position;
    g_object_notify_by_pspec(G_OBJECT(self), 
      g_mpris_media_player_param_specs[G_MPRIS_MEDIA_PLAYER_PROP_POSITION]);
  }
  
  return G_SOURCE_CONTINUE;
}

static void
start_position_timer(GMprisMediaPlayer *self)
{
  if (self->position_timer_id > 0) {
    g_source_remove(self->position_timer_id);
    self->position_timer_id = 0;
  }
  
  if (!self->position_timer) {
    self->position_timer = g_timer_new();
  } else {
    g_timer_reset(self->position_timer);
  }
  
  self->last_known_position = self->position;
  
  // Start timer - update every 250ms (4 times per second)
  self->position_timer_id = g_timeout_add(250, position_update_timer_callback, self);
}

static void
stop_position_timer(GMprisMediaPlayer *self)
{
  if (self->position_timer_id > 0) {
    g_source_remove(self->position_timer_id);
    self->position_timer_id = 0;
  }
}


static void
g_mpris_media_player_finalize(GObject * object)
{
  g_debug("g_mpris_media_player_finalize entered");

  GMprisMediaPlayer *self = G_MPRIS_MEDIA_PLAYER(object);

  stop_position_timer(self);
  if (self->position_timer) {
    g_timer_destroy(self->position_timer);
    self->position_timer = NULL;
  }

  if (self->props_sub_id) {
    g_dbus_connection_signal_unsubscribe(self->conn, self->props_sub_id);
    self->props_sub_id = 0;
  }

  if (self->seeked_sub_id) {
    g_dbus_connection_signal_unsubscribe(self->conn, self->seeked_sub_id);
    self->seeked_sub_id = 0;
  }

  if(self->player_proxy){
    g_object_unref(self->player_proxy);
    self->player_proxy = NULL;
  }

  g_clear_pointer(&self->title, g_free);
  g_clear_pointer(&self->artist, g_free);
  g_clear_pointer(&self->arturl, g_free);

  g_clear_object(&self->conn);
  g_clear_pointer((gpointer*)&self->iface, g_free);

  G_OBJECT_CLASS
      (g_mpris_media_player_parent_class)->finalize(object);

  g_debug("g_mpris_media_player_finalized exited");
}

static void
g_mpris_media_player_constructed(GObject* object)
{
  g_debug("g_mpris_media_player_constructed entered");

  GMprisMediaPlayer *self = G_MPRIS_MEDIA_PLAYER(object);
  (void)self;

  g_debug("g_mpris_media_player_constructed exited");
}

static void
g_mpris_media_player_class_init(GMprisMediaPlayerClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = g_mpris_media_player_set_property;
  gobject_class->get_property = g_mpris_media_player_get_property;
  gobject_class->finalize = g_mpris_media_player_finalize;
  gobject_class->constructed = g_mpris_media_player_constructed;

  g_mpris_media_player_param_specs[G_MPRIS_MEDIA_PLAYER_PROP_CONNECTION] =
    g_param_spec_object("connection",
                        "Connection",
                        "GDBusConnection to use",
                        G_TYPE_DBUS_CONNECTION,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_mpris_media_player_param_specs[G_MPRIS_MEDIA_PLAYER_PROP_IFACE] =
    g_param_spec_string("iface",
                        "Interface",
                        "MPRIS player bus name (org.mpris.MediaPlayer2.*)",
                        NULL,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_mpris_media_player_param_specs[G_MPRIS_MEDIA_PLAYER_PROP_STATE] =
    g_param_spec_enum("state",
                        "State",
                        "MPRIS player state",
                        G_TYPE_MPRIS_MEDIA_PLAYER_STATE,
                        G_MPRIS_MEDIA_PLAYER_STATE_IDLE,
                        G_PARAM_READABLE);

  g_mpris_media_player_param_specs[G_MPRIS_MEDIA_PLAYER_PROP_TITLE] =
    g_param_spec_string("title",
                        "Title",
                        "Current track title",
                        "", // default empty string
                        G_PARAM_READABLE);

  g_mpris_media_player_param_specs[G_MPRIS_MEDIA_PLAYER_PROP_ARTIST] =
    g_param_spec_string("artist",
                        "Artist", 
                        "Current track artist",
                        "", // default empty string
                        G_PARAM_READABLE);

  g_mpris_media_player_param_specs[G_MPRIS_MEDIA_PLAYER_PROP_ARTURL] =
    g_param_spec_string("arturl",
                        "Art URL", 
                        "Current track art url",
                        "", // default empty string
                        G_PARAM_READABLE);

  g_mpris_media_player_param_specs[G_MPRIS_MEDIA_PLAYER_PROP_POSITION] =
    g_param_spec_int64("position",
                       "Position",
                       "Current playback position in microseconds",
                       0,
                       G_MAXINT64,
                       0,
                       G_PARAM_READABLE);
  
  g_mpris_media_player_param_specs[G_MPRIS_MEDIA_PLAYER_PROP_LENGTH] =
    g_param_spec_int64("length",
                       "Length",
                       "Current playback length in microseconds",
                       0,
                       G_MAXINT64,
                       0,
                       G_PARAM_READABLE);

  g_mpris_media_player_param_specs[G_MPRIS_MEDIA_PLAYER_PROP_CAN_GO_NEXT] =
    g_param_spec_boolean("can-go-next",
                       "Can-Go-Next",
                       "If this player support next command",
                       FALSE,
                       G_PARAM_READABLE);

  g_mpris_media_player_param_specs[G_MPRIS_MEDIA_PLAYER_PROP_CAN_GO_PREVIOUS] =
    g_param_spec_boolean("can-go-previous",
                       "Can-Go-Previous",
                       "If this player support previous command",
                       FALSE,
                       G_PARAM_READABLE);

  g_mpris_media_player_param_specs[G_MPRIS_MEDIA_PLAYER_PROP_CAN_PLAY] =
    g_param_spec_boolean("can-play",
                       "Can-Play",
                       "If this player support play command",
                       FALSE,
                       G_PARAM_READABLE);

  g_mpris_media_player_param_specs[G_MPRIS_MEDIA_PLAYER_PROP_CAN_CONTROL] =
    g_param_spec_boolean("can-control",
                       "Can-Control",
                       "If this player support any command",
                       FALSE,
                       G_PARAM_READABLE);

  g_mpris_media_player_signals[G_MPRIS_MEDIA_PLAYER_SIGNAL_PROPERTY_CHANGED] =
    g_signal_new("property-changed",
                  G_TYPE_FROM_CLASS(gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                  0,
                  NULL,
                  NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);

  g_mpris_media_player_signals[G_MPRIS_MEDIA_PLAYER_SIGNAL_STATE_CHANGED] =
    g_signal_new("state-changed",
                  G_TYPE_FROM_CLASS(gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                  0,
                  NULL,
                  NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);

  g_mpris_media_player_signals[G_MPRIS_MEDIA_PLAYER_SIGNAL_META_CHANGED] =
    g_signal_new("meta-changed",
                  G_TYPE_FROM_CLASS(gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                  0,
                  NULL,
                  NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);

  g_object_class_install_properties (gobject_class,
      G_MPRIS_MEDIA_PLAYER_PROP_LAST, g_mpris_media_player_param_specs);
}

static void
on_position_query_complete(GObject *source_object,
                          GAsyncResult *result,
                          gpointer user_data)
{
    GMprisMediaPlayer *self = G_MPRIS_MEDIA_PLAYER(user_data);
    GError *error = NULL;
    GVariant *ret = g_dbus_proxy_call_finish(G_DBUS_PROXY(source_object), result, &error);
    
    if (ret && !error) {
        GVariant *value = NULL;
        g_variant_get(ret, "(v)", &value);
        
        if (value && g_variant_is_of_type(value, G_VARIANT_TYPE_INT64)) {
            gint64 new_position = g_variant_get_int64(value);
            
            g_debug("on_position_query_complete: %ld", new_position);

            if (self->position != new_position) {
                self->position = new_position;
                self->last_known_position = new_position;

                if(self->state == G_MPRIS_MEDIA_PLAYER_STATE_PLAYING && self->position_timer) {
                  g_timer_reset(self->position_timer);
                }
                
                g_object_notify_by_pspec(G_OBJECT(self), 
                    g_mpris_media_player_param_specs[G_MPRIS_MEDIA_PLAYER_PROP_POSITION]);
            }
        }
        
        if (value) g_variant_unref(value);
        g_variant_unref(ret);
    }
    
    if (error) {
        g_warning("Failed to query position: %s", error->message);
        g_error_free(error);
    }
}


static void
query_position_async(GMprisMediaPlayer *self)
{
    g_dbus_proxy_set_cached_property(self->player_proxy, "Position", NULL);
    g_dbus_proxy_call(self->player_proxy,
                     "org.freedesktop.DBus.Properties.Get",
                     g_variant_new("(ss)", IFACE_PLAYER, "Position"),
                     G_DBUS_CALL_FLAGS_NONE,
                     1000, // 1 second timeout
                     NULL,
                     on_position_query_complete,
                     self);
}

static void
g_mpris_media_player_update_info(GMprisMediaPlayer* self){
  g_return_if_fail(G_IS_MPRIS_MEDIA_PLAYER(self));

  GMprisMediaPlayerState new_state = G_MPRIS_MEDIA_PLAYER_STATE_IDLE;

  if (self->player_proxy) {

    query_position_async(self);

    GVariant *playback = g_dbus_proxy_get_cached_property(self->player_proxy, "PlaybackStatus");
    
    const char* status = "";
    if (playback && g_variant_is_of_type(playback, G_VARIANT_TYPE_STRING)) {
      status = g_variant_get_string(playback, NULL);
      
      if (g_strcmp0(status, "Playing") == 0) {
        new_state = G_MPRIS_MEDIA_PLAYER_STATE_PLAYING;
      } else if (g_strcmp0(status, "Paused") == 0) {
        new_state = G_MPRIS_MEDIA_PLAYER_STATE_PAUSED;
      } else if (g_strcmp0(status, "Stopped") == 0) {
        new_state = G_MPRIS_MEDIA_PLAYER_STATE_STOPPED;
      } else {
        new_state = G_MPRIS_MEDIA_PLAYER_STATE_IDLE;
      }
    }

    GVariant *metadata = g_dbus_proxy_get_cached_property(self->player_proxy, "Metadata");

    const char *new_title = "";
    const char *new_artist = "";

    if (metadata && g_variant_is_of_type(metadata, G_VARIANT_TYPE("a{sv}"))) {

      GVariant *v_length = g_variant_lookup_value(metadata, "mpris:length", G_VARIANT_TYPE_INT64);
      gint64 track_length = 0;
      if (v_length) {
        track_length = g_variant_get_int64(v_length);
        g_variant_unref(v_length);
      }

      if(self->length != track_length){
        self->length = track_length;

        g_object_notify_by_pspec(G_OBJECT(self), 
          g_mpris_media_player_param_specs[G_MPRIS_MEDIA_PLAYER_PROP_LENGTH]);
      }

      GVariant *v_title = g_variant_lookup_value(metadata, "xesam:title", G_VARIANT_TYPE_STRING);
      if (v_title) {
        new_title = g_variant_get_string(v_title, NULL);
      }

      GVariant *v_artist = g_variant_lookup_value(metadata, "xesam:artist", G_VARIANT_TYPE("as"));
      if (v_artist && g_variant_n_children(v_artist) > 0) {
        //TODO - iterate over artists in the array
        GVariant *child0 = g_variant_get_child_value(v_artist, 0);
        new_artist = g_variant_get_string(child0, NULL);
        g_variant_unref(child0);
      }

      if (g_strcmp0(self->title, new_title) != 0) {
        g_free(self->title);
        self->title = g_strdup(new_title ? new_title : "");

        g_object_notify_by_pspec(G_OBJECT(self), 
          g_mpris_media_player_param_specs[G_MPRIS_MEDIA_PLAYER_PROP_TITLE]);
      }

      if (g_strcmp0(self->artist, new_artist) != 0) {
        g_free(self->artist);
        self->artist = g_strdup(new_artist ? new_artist : "");

        g_object_notify_by_pspec(G_OBJECT(self), 
          g_mpris_media_player_param_specs[G_MPRIS_MEDIA_PLAYER_PROP_ARTIST]);
      }

      if (v_title) g_variant_unref(v_title);
      if (v_artist) g_variant_unref(v_artist);

      GVariant *v_arturl = g_variant_lookup_value(metadata, "mpris:artUrl", G_VARIANT_TYPE_STRING);
      if (v_arturl) {
        const char *art_url = g_variant_get_string(v_arturl, NULL);

        if (g_strcmp0(self->arturl, art_url) != 0) {
          g_free(self->arturl);
          self->arturl = g_strdup(art_url ? art_url : "");

          g_object_notify_by_pspec(G_OBJECT(self), 
              g_mpris_media_player_param_specs[G_MPRIS_MEDIA_PLAYER_PROP_ARTURL]);

        }

        g_variant_unref(v_arturl);
      }

      g_signal_emit(self, 
            g_mpris_media_player_signals[G_MPRIS_MEDIA_PLAYER_SIGNAL_META_CHANGED], 
            0);
    }


    char *title_artist = g_strdup("");
    if(new_artist[0] && new_title[0]){
      g_free(title_artist);
      title_artist = g_strdup_printf("%s-%s", new_artist, new_title);
    } else if(new_title[0]){
      g_free(title_artist);
      title_artist = g_strdup_printf("%s", new_title);
    } else if(new_artist[0]){
      g_free(title_artist);
      title_artist = g_strdup_printf("%s", new_artist);
    }

    // ---- "Widget output" (replace with GTK label updates etc.) ----
    g_debug("[%-30s] %-7s | %s\n",
            self->iface ? self->iface : "(none)",
            status[0] ? status : "?",
            title_artist[0] ? title_artist : "(no metadata)");

    g_free(title_artist);

    if (playback) g_variant_unref(playback);
    if (metadata) g_variant_unref(metadata);
  }

  if (self->state != new_state) {
    self->state = new_state;


    if(self->state == G_MPRIS_MEDIA_PLAYER_STATE_PLAYING){
      start_position_timer(self);
    } else {
      stop_position_timer(self);
    }

    g_object_notify_by_pspec(G_OBJECT(self), 
          g_mpris_media_player_param_specs[G_MPRIS_MEDIA_PLAYER_PROP_STATE]);

    g_signal_emit(self, 
          g_mpris_media_player_signals[G_MPRIS_MEDIA_PLAYER_SIGNAL_STATE_CHANGED], 
          0);
  }

  gboolean can_play = FALSE;
  GVariant *can_play_variant = g_dbus_proxy_get_cached_property(self->player_proxy, "CanPlay");
  if (can_play_variant && g_variant_is_of_type(can_play_variant, G_VARIANT_TYPE_BOOLEAN)) {
      can_play = g_variant_get_boolean(can_play_variant);
      g_variant_unref(can_play_variant);
  }  

  gboolean can_control = FALSE;
  GVariant *can_control_variant = g_dbus_proxy_get_cached_property(self->player_proxy, "CanControl");
  if (can_control_variant && g_variant_is_of_type(can_control_variant, G_VARIANT_TYPE_BOOLEAN)) {
      can_control = g_variant_get_boolean(can_control_variant);
      g_variant_unref(can_control_variant);
  }

  gboolean can_go_next = FALSE;
  GVariant *can_next_variant = g_dbus_proxy_get_cached_property(self->player_proxy, "CanGoNext");
  if (can_next_variant && g_variant_is_of_type(can_next_variant, G_VARIANT_TYPE_BOOLEAN)) {
      can_go_next = g_variant_get_boolean(can_next_variant);
      g_variant_unref(can_next_variant);
  }

  gboolean can_go_previous = FALSE;
  GVariant *can_prev_variant = g_dbus_proxy_get_cached_property(self->player_proxy, "CanGoPrevious");
  if (can_prev_variant && g_variant_is_of_type(can_prev_variant, G_VARIANT_TYPE_BOOLEAN)) {
      can_go_previous = g_variant_get_boolean(can_prev_variant);
      g_variant_unref(can_prev_variant);
  }

  g_debug("Debug can_ variables: can_play=%s, can_control=%s, can_go_next=%s, can_go_previous=%s",
          can_play ? "TRUE" : "FALSE",
          can_control ? "TRUE" : "FALSE", 
          can_go_next ? "TRUE" : "FALSE",
          can_go_previous ? "TRUE" : "FALSE");

  if(can_play != self->can_play){
    self->can_play = can_play;

    g_object_notify_by_pspec(G_OBJECT(self), 
          g_mpris_media_player_param_specs[G_MPRIS_MEDIA_PLAYER_PROP_CAN_PLAY]);
  }

  if(can_control != self->can_control){
    self->can_control = can_control;

    g_object_notify_by_pspec(G_OBJECT(self), 
          g_mpris_media_player_param_specs[G_MPRIS_MEDIA_PLAYER_PROP_CAN_CONTROL]);
  }

  if(can_go_next != self->can_go_next){
    self->can_go_next = can_go_next;

    g_object_notify_by_pspec(G_OBJECT(self), 
          g_mpris_media_player_param_specs[G_MPRIS_MEDIA_PLAYER_PROP_CAN_GO_NEXT]);
  }

  if(can_go_previous != self->can_go_previous){
    self->can_go_previous = can_go_previous;

    g_object_notify_by_pspec(G_OBJECT(self), 
          g_mpris_media_player_param_specs[G_MPRIS_MEDIA_PLAYER_PROP_CAN_GO_PREVIOUS]);
  }

  g_signal_emit(self, 
      g_mpris_media_player_signals[G_MPRIS_MEDIA_PLAYER_SIGNAL_PROPERTY_CHANGED], 
      0);

}

static void
on_properties_changed(GDBusConnection *connection,
                      const gchar *sender_name,
                      const gchar *object_path,
                      const gchar *interface_name,
                      const gchar *signal_name,
                      GVariant *parameters,
                      gpointer user_data)
{
  GMprisMediaPlayer *self = G_MPRIS_MEDIA_PLAYER(user_data);

  const char *iface = NULL;
  GVariant *changed = NULL;
  g_variant_get(parameters, "(&s@a{sv}as)", &iface, &changed, NULL);

  if (g_strcmp0(iface, IFACE_PLAYER) == 0) {
    g_mpris_media_player_update_info(self);
  }

  if (changed) g_variant_unref(changed);
}

static void
on_seeked(GDBusConnection *connection,
          const gchar *sender_name,
          const gchar *object_path,
          const gchar *interface_name,
          const gchar *signal_name,
          GVariant *parameters,
          gpointer user_data) {

  GMprisMediaPlayer *self = G_MPRIS_MEDIA_PLAYER(user_data);
 
  gint64 new_position = 0;
  g_variant_get(parameters, "(x)", &new_position);
  
  g_debug("Seeked signal received: new position = %ld microseconds", new_position);
  
  if (self->position != new_position) {
    self->position = new_position;
    self->last_known_position = new_position;
    
    if (self->state == G_MPRIS_MEDIA_PLAYER_STATE_PLAYING && self->position_timer) {
      g_timer_reset(self->position_timer);
    }
    
    g_object_notify_by_pspec(G_OBJECT(self), 
        g_mpris_media_player_param_specs[G_MPRIS_MEDIA_PLAYER_PROP_POSITION]);
  }
  
  g_signal_emit(self, 
      g_mpris_media_player_signals[G_MPRIS_MEDIA_PLAYER_SIGNAL_PROPERTY_CHANGED], 
      0);
}


GMprisMediaPlayer*
g_mpris_media_player_new(GDBusConnection* conn, const char* iface){
  g_debug("g_mpris_media_player_new entered");

  GMprisMediaPlayer* self = g_object_new(G_TYPE_MPRIS_MEDIA_PLAYER, "connection", conn, "iface", iface, NULL);

  GError *err = NULL;
  self->player_proxy = g_dbus_proxy_new_sync(
      self->conn,
      G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
      NULL,
      self->iface,
      MPRIS_PATH,
      IFACE_PLAYER,
      NULL,
      &err);

  if (!self->player_proxy) {
    g_error("Failed to create player proxy for %s: %s\n",
               self->iface, err ? err->message : "unknown");
    g_clear_error(&err);
  }

  self->props_sub_id = g_dbus_connection_signal_subscribe(
      self->conn,
      self->iface,
      IFACE_PROPS,
      "PropertiesChanged",
      MPRIS_PATH,
      NULL,
      G_DBUS_SIGNAL_FLAGS_NONE,
      on_properties_changed,
      self,
      NULL);

  self->seeked_sub_id = g_dbus_connection_signal_subscribe(
      self->conn,
      self->iface,
      IFACE_PLAYER,
      "Seeked",
      MPRIS_PATH,
      NULL,
      G_DBUS_SIGNAL_FLAGS_NONE,
      on_seeked,
      self,
      NULL);

  g_mpris_media_player_update_info(self);

  g_debug("g_mpris_media_player_new exited");
  return self;
}

static void
g_mpris_media_player_init(GMprisMediaPlayer * self)
{
  g_debug("g_mpris_media_player_init entered");
  self->player_proxy = NULL;
  self->props_sub_id = 0;
  self->seeked_sub_id = 0;
  self->state = G_MPRIS_MEDIA_PLAYER_STATE_IDLE;
  self->title = g_strdup("");
  self->artist = g_strdup("");
  self->arturl = g_strdup("");
  self->position = 0;

  self->can_go_next = FALSE;
  self->can_go_previous = FALSE;
  self->can_control = FALSE;
  self->can_play = FALSE;

  g_debug("g_mpris_media_player_init exited");
}

int 
g_mpris_media_player_compare(const void* a, const void* b) {
  if(!a || !b) return -1;
  g_return_val_if_fail(G_IS_MPRIS_MEDIA_PLAYER(a) && G_IS_MPRIS_MEDIA_PLAYER(b), -1);

  gchar *iface1 = NULL, *iface2 = NULL;

  g_object_get(G_OBJECT(a), "iface", &iface1, NULL);
  g_object_get(G_OBJECT(b), "iface", &iface2, NULL);

  int ret = g_strcmp0(iface1, iface2);

  g_free(iface1);
  g_free(iface2);

  return ret;
}

gboolean 
g_is_mpris_media_player_available(GMprisMediaPlayer* self) {
  gchar* artist = g_strstrip(g_strdup(self->artist));
  gchar* title = g_strstrip(g_strdup(self->title));

  gboolean ret =  self->state != G_MPRIS_MEDIA_PLAYER_STATE_IDLE && 
          self->can_control && 
          self->can_play && 
          (strlen(artist) > 0 || strlen(title) > 0);

  g_free(artist);
  g_free(title);

  return ret;
}

gboolean 
g_mpris_media_player_is_iface(GMprisMediaPlayer* self, const char* iface) {
  g_return_val_if_fail(G_IS_MPRIS_MEDIA_PLAYER(self), FALSE);

  return g_strcmp0(self->iface, iface) == 0;
}

static void
on_command_complete(GObject *source_object,
                   GAsyncResult *result,
                   gpointer user_data) {
  GError *error = NULL;
  GVariant *ret = g_dbus_proxy_call_finish(G_DBUS_PROXY(source_object), result, &error);
  
  if (error) {
    g_warning("MPRIS command failed: %s", error->message);
    g_error_free(error);
  }
  
  if (ret) g_variant_unref(ret);
}

static void g_mpris_media_player_send_command_async(GMprisMediaPlayer* self, 
                                            const char* method_name) {

  g_return_if_fail(G_IS_MPRIS_MEDIA_PLAYER(self));
  g_return_if_fail(self->player_proxy != NULL);

  g_dbus_proxy_call(self->player_proxy,
                   method_name,
                   NULL,
                   G_DBUS_CALL_FLAGS_NONE,
                   -1,
                   NULL,
                   on_command_complete,
                   self);
}

void g_mpris_media_player_play(GMprisMediaPlayer* self){
  g_mpris_media_player_send_command_async(self, "Play");
}
void g_mpris_media_player_pause(GMprisMediaPlayer* self){
  g_mpris_media_player_send_command_async(self, "Pause");
}
void g_mpris_media_player_stop(GMprisMediaPlayer* self){
  g_mpris_media_player_send_command_async(self, "Stop");
}
void g_mpris_media_player_play_pause(GMprisMediaPlayer* self){
  g_mpris_media_player_send_command_async(self, "PlayPause");
}
void g_mpris_media_player_next(GMprisMediaPlayer* self){
  g_mpris_media_player_send_command_async(self, "Next");
}
void g_mpris_media_player_previous(GMprisMediaPlayer* self){
    g_mpris_media_player_send_command_async(self, "Previous");
}

