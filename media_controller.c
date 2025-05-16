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

#include <math.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cairo.h>
#include <pango/pango.h>
#include <glib-object.h>
#include <glib.h>
#include <playerctl.h>
#include <playerctl-player-name.h>

#include "media_controller.h"
#include "media_player.h"

struct _GtkMediaController
{
  GtkEventBox parent;

  MediaPlayerModConfig* config;
  gboolean reversed_scroll;
  gint scroll_timer;
  gint scroll_timeout;

  GtkContainer* container;
  GtkLabel* player_text;
  GtkScrolledWindow* title_scroll;
  GtkLabel* title;

  GtkButton* btn_prev;
  GtkButton* btn_next;
  GtkButton* btn_play;

  GtkWindow* tooltip_window;
  GtkImage* tooltip_image;

  PlayerctlPlayerManager* player_manager;
  PlayerctlPlayer* current_player;
  GList* media_players;

  gint unavailable_timeout;

  GtkMediaControllerState state;
};

struct _GtkMediaControllerClass
{
  GtkEventBoxClass parent_class;

  void (*media_state_changed)	(GtkMediaController *self);
};

enum
{
  GTK_MEDIA_CONTROLLER_PROP_0,
  GTK_MEDIA_CONTROLLER_PROP_STATE,
  GTK_MEDIA_CONTROLLER_PROP_LAST
};

GType
gtk_media_controller_state_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { GTK_MEDIA_CONTROLLER_STATE_IDLE, "GTK_MEDIA_CONTROLLER_STATE_IDLE", "idle" },
      { GTK_MEDIA_CONTROLLER_STATE_PLAYING, "GTK_MEDIA_CONTROLLER_STATE_PLAYING", "playing" },
      { GTK_MEDIA_CONTROLLER_STATE_PAUSED, "GTK_MEDIA_CONTROLLER_STATE_PAUSED", "paused" },
      { GTK_MEDIA_CONTROLLER_STATE_STOPPED, "GTK_MEDIA_CONTROLLER_STATE_STOPPED", "stopped" },
      { 0, NULL, NULL }
    };
  etype = g_enum_register_static (g_intern_static_string ("GtkMediaControllerState"), values);
  }
  return etype;
}

#define GTK_TYPE_MEDIA_CONTROLLER_STATE (gtk_media_controller_state_get_type ())

enum
{
  GTK_MEDIA_CONTROLLER_SIGNAL_0,
  GTK_MEDIA_CONTROLLER_SIGNAL_STATE_CHANGED,
  GTK_MEDIA_CONTROLLER_SIGNAL_LAST
};

G_DEFINE_TYPE (GtkMediaController, gtk_media_controller, GTK_TYPE_EVENT_BOX);

static GParamSpec
* gtk_media_controller_param_specs[GTK_MEDIA_CONTROLLER_PROP_LAST] = { NULL, };

static guint
gtk_media_controller_signals[GTK_MEDIA_CONTROLLER_SIGNAL_LAST] = {0, };

static void
gtk_media_controller_get_property(GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GtkMediaController *self = GTK_MEDIA_CONTROLLER(object);

  switch (prop_id) {
    case GTK_MEDIA_CONTROLLER_PROP_STATE:
      g_value_set_enum(value, self->state);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gtk_media_controller_set_property(GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GtkMediaController *self = GTK_MEDIA_CONTROLLER(object);

  switch (prop_id) {
    case GTK_MEDIA_CONTROLLER_PROP_STATE:
      printf("Setting up controller state\n");
      GtkMediaControllerState state = g_value_get_enum(value);
      if(self->state != state){
        self->state = state;
        g_signal_emit(self,
                      gtk_media_controller_signals[GTK_MEDIA_CONTROLLER_SIGNAL_STATE_CHANGED],
                      0, self->state);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gtk_media_controller_finalize(GObject * object)
{
  printf("gtk_media_controller_finalize\n");
  GtkMediaController *self = GTK_MEDIA_CONTROLLER(object);

  self->state = GTK_MEDIA_CONTROLLER_STATE_STOPPED;

  g_source_remove(self->scroll_timeout);

  if(self->unavailable_timeout > 0){
    g_source_remove(self->unavailable_timeout);
  }

  if (self->config){
    g_free(self->config->btn_play);
    g_free(self->config->btn_pause);
    g_free(self->config->btn_next);
    g_free(self->config->btn_prev);
    g_free(self->config);
  }

  if (self->media_players) {
    g_list_free_full(self->media_players, gtk_media_player_destroy);
    self->media_players = NULL;
  }

  if(self->player_manager){
    g_object_unref(self->player_manager);
    self->player_manager = NULL;
  }

  g_object_unref(self->container);
  self->container = NULL;

  G_OBJECT_CLASS
      (gtk_media_controller_parent_class)->finalize(object);

  printf("gtk_media_controller_finalized\n");
}

static void 
gtk_media_controller_update(GtkMediaController* self) {
  printf("gtk_media_controller_update entered\n");

  if(!self->container) return;

  guint pos = 0;
  guint size = 0;
  for(struct {int idx; GList* item; } loop = {0, g_list_first(self->media_players)}; loop.item; loop.item = loop.item->next){
    GtkMediaPlayer* media_player = (GtkMediaPlayer*)loop.item->data;
    if(media_player->available) { size ++; loop.idx++; } else { continue; };
    if(media_player->player == self->current_player) pos = loop.idx; 
  }

  if(self->media_players == NULL || size == 0){
    if(gtk_widget_get_parent_window(GTK_WIDGET(self->container)) != NULL){
      gtk_container_remove(GTK_CONTAINER(self), GTK_WIDGET(self->container));
    }
    return;
  } else {
    if(gtk_widget_get_parent_window(GTK_WIDGET(self->container)) == NULL){
      gtk_container_add(GTK_CONTAINER(self), GTK_WIDGET(self->container));
      gtk_widget_show_all(GTK_WIDGET(self->container));
    }
  }

  if(self->player_text != NULL){
    if(self->current_player !=NULL){
      gint mem_size = (ceil(log10(pos || 1))+1+ceil(log10(size || 1))+1+4) * sizeof(gchar);
      gchar* text = g_malloc(mem_size);
      g_snprintf(text, mem_size, "[%d/%d]", pos, size);
      gtk_label_set_text(GTK_LABEL(self->player_text),text);
      g_free(text);
    }
  }
 
  if(self->title){
    if(self->current_player){
      GError* err = NULL;

      gchar* artist = playerctl_player_get_artist(self->current_player, &err);
      if(err != NULL){
        artist = NULL;
        printf("Error getting artist: %s\n", err->message);
        g_clear_error(&err);
        err = NULL;
      }

      gchar* title = playerctl_player_get_title(self->current_player, &err);
      if(err != NULL){
        title = NULL;
        printf("Error getting title: %s\n", err->message);
        g_clear_error(&err);
        err = NULL;
      }

      if((artist && strlen(artist) > 0) || (title && strlen(title) > 0)){
        gint mem_size = 0;
        if(artist && strlen(artist) > 0) mem_size += strlen(artist);
        if(title && strlen(title) > 0) mem_size += strlen(title);

        gint final_size = mem_size+4;
        gchar* final_title = g_malloc(final_size);
        if(artist && strlen(artist) > 0 && title && strlen(title) > 0){
          g_snprintf(final_title,final_size, "%s - %s", artist, title);
        } else if(artist && strlen(artist) > 0) {
          g_snprintf(final_title,final_size, "%s", artist);
        } else if(title && strlen(title) > 0) {
          g_snprintf(final_title,final_size, "%s", title);
        }        

        if(mem_size > 0)
          gtk_label_set_text(self->title, final_title);
        else
          gtk_label_set_text(self->title, "No Name");

        PangoLayout* layout = gtk_label_get_layout(GTK_LABEL(self->title));
        gint min_width, min_height; 
        pango_layout_get_pixel_size(layout, &min_width, &min_height);
        if(min_width > self->config->title_max_width) min_width = self->config->title_max_width;
        gtk_widget_set_size_request(GTK_WIDGET(self->title_scroll), min_width, 0);

        g_free(final_title);
      } else {
        gtk_label_set_text(self->title, "No Media");
      }

      if(artist)
        g_free(artist);
      if(title)
        g_free(title);
    }

    if(self->media_players && self->current_player){
      GList* item = g_list_find_custom(g_list_first(self->media_players), self->current_player, gtk_media_player_compare);
      if(item){
        GtkMediaPlayer* media_player = (GtkMediaPlayer*)item->data;
        if(media_player->status == PLAYERCTL_PLAYBACK_STATUS_PLAYING){
          gtk_button_set_label(self->btn_play, self->config->btn_pause);
        } else {
          gtk_button_set_label(self->btn_play, self->config->btn_play);
        }

        GValue val = G_VALUE_INIT;
        g_value_init(&val, G_TYPE_BOOLEAN);
        g_object_get_property(G_OBJECT(media_player->player), "can-go-previous", &val);
        if(g_value_get_boolean(&val) == FALSE)
          gtk_widget_hide(GTK_WIDGET(self->btn_prev));
        else
          gtk_widget_show(GTK_WIDGET(self->btn_prev));
        g_value_unset(&val);

        g_object_get_property(G_OBJECT(media_player->player), "can-go-next", &val);
        if(g_value_get_boolean(&val) == FALSE)
          gtk_widget_hide(GTK_WIDGET(self->btn_next));
        else
          gtk_widget_show(GTK_WIDGET(self->btn_next));
        g_value_unset(&val);

      }
    }
  }
  printf("gtk_media_controller_update exited\n");
}

static void 
gtk_media_controller_reset_title_scroll(GtkMediaController* self){
  self->reversed_scroll = FALSE;
  if(self->container && gtk_widget_get_parent(GTK_WIDGET(self->container))){
    GtkAdjustment* adjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(self->title_scroll));
    gtk_adjustment_set_value(adjustment, 0);
    self->scroll_timer = self->config->scroll_before_timeout*(1000/self->config->scroll_interval);
  }
}

static void
gtk_media_controller_set_player(GtkMediaController* self, PlayerctlPlayer* player){
  if(self->current_player != NULL && player != NULL && self->current_player != player){
    gtk_media_controller_reset_title_scroll(self);
  }
  self->current_player = player;
}

static void 
gtk_media_controller_player_add(GtkMediaController* self, PlayerctlPlayer* player){
  printf("gtk_media_controller_player_add entered\n");
  GList* item = g_list_find_custom(g_list_first(self->media_players), player, gtk_media_player_compare);
  if(item == NULL){
    GtkMediaPlayer* media_player = gtk_media_player_new(self, player); 
    self->media_players = g_list_append(self->media_players, media_player);
    printf("New player added to media controller %ld\n", (gint64)media_player->player);
    if(media_player->available)
      gtk_media_controller_set_player(self, media_player->player);
  } else {
    GtkMediaPlayer* media_player = (GtkMediaPlayer*)item->data;
    media_player->available = TRUE;
    gtk_media_controller_set_player(self, media_player->player);
  }
  printf("gtk_media_controller_player_add exited\n");
}

static void 
gtk_media_controller_select_next_player(GtkMediaController* self, PlayerctlPlayer* player) {
  if(player){
    GList* item = g_list_find_custom(self->media_players, player, gtk_media_player_compare);
    if(item){
      PlayerctlPlayer* next_player = NULL;
      for(GList* next = item->next; next; next = next->next){
        GtkMediaPlayer* media_player = (GtkMediaPlayer*)next->data;
        if(media_player->available) {
          next_player = media_player->player;
          break;
        }
      }
      if(next_player != NULL){
        gtk_media_controller_set_player(self, next_player);
        return;
      }
    }
  }
  
  GList* item = g_list_first(self->media_players);
  if(item){
    PlayerctlPlayer* next_player = NULL;
    for(GList* next = item; next; next = next->next){
      GtkMediaPlayer* media_player = (GtkMediaPlayer*)next->data;
      if(media_player->available) {
        next_player = media_player->player;
        break;
      }
    }
    gtk_media_controller_set_player(self, next_player);
  }
  return;
}

static void 
gtk_media_controller_player_remove(GtkMediaController* self, PlayerctlPlayer* player) {
  printf("gtk_media_controller_player_remove entered\n");

  if(self->unavailable_timeout > 0){
    g_source_remove(self->unavailable_timeout);
    self->unavailable_timeout = 0;
  }  

  if(self->media_players != NULL){
    GList* item = g_list_find_custom(g_list_first(self->media_players), player, gtk_media_player_compare);
    if(item != NULL){
      self->media_players = g_list_remove_link(self->media_players, item);
      gtk_media_controller_select_next_player(self, player);
      g_list_free_full(item, gtk_media_player_destroy);
    }
  } else {
      gtk_media_controller_set_player(self, NULL);
  }
}

static void 
gtk_media_controller_player_unavailable(gpointer user_data){
  GtkMediaPlayer* media_player = (GtkMediaPlayer*)user_data;
  media_player->available = FALSE;
  media_player->parent->unavailable_timeout = 0;
  gtk_media_controller_select_next_player(media_player->parent, media_player->player);
  gtk_media_controller_update(media_player->parent);
}

static void 
gtk_media_controller_mark_player_unavailable(GtkMediaController* self, GtkMediaPlayer* media_player, gint sec){
  self->unavailable_timeout = g_timeout_add_once(sec, gtk_media_controller_player_unavailable, media_player);
}

static void 
gtk_media_controller_on_playback_status(PlayerctlPlayer* player, PlayerctlPlaybackStatus status, gpointer user_data) {
  printf("gtk_media_controller_on_playback_status entered\n");
  GtkMediaController* self = GTK_MEDIA_CONTROLLER(user_data);

  switch(status){
    case PLAYERCTL_PLAYBACK_STATUS_PLAYING:
    case PLAYERCTL_PLAYBACK_STATUS_PAUSED:
      printf("Status: play/pause\n");
      gtk_media_controller_player_add(self,player);
      gtk_media_controller_set_player(self,player);
      if(self->unavailable_timeout > 0){
        g_source_remove(self->unavailable_timeout);
        self->unavailable_timeout = 0;
      }
      break;

    case PLAYERCTL_PLAYBACK_STATUS_STOPPED:
      printf("Status: stop\n");
      break;
  }

  GList* item = g_list_find_custom(self->media_players, player, gtk_media_player_compare);
  if(item != NULL){
    GtkMediaPlayer* media_player = (GtkMediaPlayer*)item->data;
    media_player->status = status;
    if(status == PLAYERCTL_PLAYBACK_STATUS_STOPPED) {
      gtk_media_controller_mark_player_unavailable(self, media_player, 5000);
    }
  }
  gtk_media_controller_update(self);
}

static void 
gtk_media_controller_on_meta(PlayerctlPlayer* player, GVariant* metadata, gpointer user_data){
  printf("gtk_media_controller_on_meta entered\n");
  GtkMediaController* self = GTK_MEDIA_CONTROLLER(user_data);

  guint64 length = gtk_media_player_get_length(player);

  GList* item = g_list_find_custom(g_list_first(self->media_players), player, gtk_media_player_compare);
  if(item != NULL){
    GtkMediaPlayer* media_player = (GtkMediaPlayer*)item->data;
    media_player->length = length;
    gtk_media_controller_set_player(self, player);
  }

  gtk_media_controller_update(self);
}

static void 
gtk_media_controller_on_seeked(PlayerctlPlayer* player, gint64 position, gpointer user_data){
  gint64 sec = position/1000000;
  printf("gtk_media_controller_on_seek = %ld\n", sec);
  GtkMediaController* self = GTK_MEDIA_CONTROLLER(user_data);
  gtk_media_controller_player_add(self,player);
  gtk_media_controller_set_player(self,player);
  if(sec == 0){
    gtk_media_controller_reset_title_scroll(self);
  }
  gtk_media_controller_update(self);
}

static void 
gtk_media_controller_on_exit(PlayerctlPlayer* player, gpointer user_data){
  printf("gtk_media_controller_on_exit entered\n");
  GtkMediaController* self = GTK_MEDIA_CONTROLLER(user_data);
  if(self->current_player == player){
    gtk_media_controller_set_player(self, NULL);
  }
  gtk_media_controller_update(self);
}

static void 
gtk_media_controller_on_player_added(PlayerctlPlayerManager* player_manager, PlayerctlPlayer* player, gpointer user_data) {
  printf("gtk_media_controller_on_player_added entered\n");
  GtkMediaController* self = GTK_MEDIA_CONTROLLER(user_data);
  gtk_media_controller_player_add(self, player);
  gtk_media_controller_update(self);
}

static void 
gtk_media_controller_on_player_removed(PlayerctlPlayerManager* player_manager, PlayerctlPlayer* player, gpointer user_data) {
  printf("gtk_media_controller_on_player_removed entered\n");
  GtkMediaController* self = GTK_MEDIA_CONTROLLER(user_data);
  gtk_media_controller_player_remove(self,player);
  gtk_media_controller_update(self);
}

static void 
gtk_media_controller_on_name_added(PlayerctlPlayerManager* player_manager, PlayerctlPlayerName* name, gpointer user_data) {
  printf("gtk_media_controller_on_name_added entered\n");
  GtkMediaController* self = GTK_MEDIA_CONTROLLER(user_data);
  GError* err = NULL;

  MediaPlayerCallbacks callbacks = {
    .on_playback_status = gtk_media_controller_on_playback_status,
    .on_meta = gtk_media_controller_on_meta,
    .on_seeked = gtk_media_controller_on_seeked,
    .on_exit = gtk_media_controller_on_exit,
  };
  media_player_add_player_by_name(player_manager, name, &callbacks, &err, self);
}

static void 
gtk_media_controller_on_name_removed(PlayerctlPlayerManager* player_manager, PlayerctlPlayerName* name, gpointer user_data) {
  printf("gtk_media_controller_on_name_removed entered\n");
}

static void
gtk_media_controller_constructed(GObject* object)
{
  GtkMediaController *self = GTK_MEDIA_CONTROLLER(object);
  printf("gtk_media_controller_constructed\n");

  MediaPlayerManagerCallbacks manager_callbacks = {
    .on_player_added = gtk_media_controller_on_player_added,
    .on_player_removed = gtk_media_controller_on_player_removed,
    .on_name_added = gtk_media_controller_on_name_added,
    .on_name_removed = gtk_media_controller_on_name_removed
  };

  MediaPlayerCallbacks callbacks = {
    .on_playback_status = gtk_media_controller_on_playback_status,
    .on_meta = gtk_media_controller_on_meta,
    .on_seeked = gtk_media_controller_on_seeked,
    .on_exit = gtk_media_controller_on_exit,
  };

  self->player_manager = media_player_new(&manager_callbacks, self);
  media_player_init(self->player_manager, &callbacks, self);

  printf("gtk_media_controller_constructed fnished\n");
}

static void
gtk_media_controller_class_init(GtkMediaControllerClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = gtk_media_controller_set_property;
  gobject_class->get_property = gtk_media_controller_get_property;
  gobject_class->finalize = gtk_media_controller_finalize;
  gobject_class->constructed = gtk_media_controller_constructed;

  gtk_media_controller_param_specs[GTK_MEDIA_CONTROLLER_PROP_STATE] =
        g_param_spec_enum ("state", "state",
        "Media Controller current state", GTK_TYPE_MEDIA_CONTROLLER_STATE,
        GTK_MEDIA_CONTROLLER_STATE_IDLE,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

  gtk_media_controller_signals[GTK_MEDIA_CONTROLLER_SIGNAL_STATE_CHANGED] =
    g_signal_new("media-state-changed",
                  G_TYPE_FROM_CLASS(gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                  G_STRUCT_OFFSET(GtkMediaControllerClass, media_state_changed),
                  NULL,
                  NULL,
                  NULL,
                  G_TYPE_NONE,
                  1,
                  GTK_TYPE_MEDIA_CONTROLLER_STATE);

  g_object_class_install_properties (gobject_class,
      GTK_MEDIA_CONTROLLER_PROP_LAST, gtk_media_controller_param_specs);
}

static void
gtk_media_controller_init(GtkMediaController * self)
{
  printf("Initializing player\n");
  self->state = GTK_MEDIA_CONTROLLER_STATE_IDLE;
  self->media_players = NULL;
  self->current_player = NULL;
  self->unavailable_timeout = 0;
}

void static 
gtk_media_controller_on_title_bp(GtkLabel* title, GdkEventButton* event, gpointer user_data){
  GtkMediaController* self = GTK_MEDIA_CONTROLLER(user_data);

  if(event->button == 1){
    if(self->current_player) {
      GError* err = NULL;
      playerctl_player_play_pause(self->current_player, &err);
      if(err != NULL) {
        printf("Error play/pause current player\n");
        g_clear_error(&err);
      }
    }
  }
}

void static 
gtk_media_controller_on_player_bp(GtkLabel* player, GdkEventButton* event, gpointer user_data){
  GtkMediaController* self = GTK_MEDIA_CONTROLLER(user_data);
  
  if(!self->media_players) return;

  if(event->button == 1){
    gtk_media_controller_select_next_player(self, self->current_player);
    gtk_media_controller_update(self);

    printf("Left mouse click on player detected\n");
  }
}

static void 
gtk_media_controller_on_prev_click(GtkButton* btn, gpointer user_data) {
  printf("gtk_media_controller_on_prev_click entered\n");
  GtkMediaController* self = GTK_MEDIA_CONTROLLER(user_data);
  GError* err = NULL;
  playerctl_player_previous(self->current_player, &err);
  gtk_media_controller_reset_title_scroll(self);
}

static void 
gtk_media_controller_on_play_click(GtkButton* btn, gpointer user_data) {
  printf("gtk_media_controller_on_play_click entered\n");
  GtkMediaController* self = GTK_MEDIA_CONTROLLER(user_data);
  GError* err = NULL;
  playerctl_player_play_pause(self->current_player, &err);
}

static void 
gtk_media_controller_on_next_click(GtkButton* btn, gpointer user_data) {
  printf("gtk_media_controller_on_next_click entered\n");
  GtkMediaController* self = GTK_MEDIA_CONTROLLER(user_data);
  GError* err = NULL;
  playerctl_player_next(self->current_player, &err);
  gtk_media_controller_reset_title_scroll(self);
}

static gboolean 
gtk_media_controller_on_draw_progress(GtkWidget* widget, cairo_t* cr, gpointer user_data){
  GtkMediaController* self = GTK_MEDIA_CONTROLLER(user_data);

  if(self->current_player && self->media_players){
    GtkStyleContext* context = gtk_widget_get_style_context(widget);

    gint width = gtk_widget_get_allocated_width(widget);
    gint height = gtk_widget_get_allocated_height(widget);

    GList* item = g_list_find_custom(g_list_first(self->media_players), self->current_player, gtk_media_player_compare);
    if(item){
      GtkMediaPlayer* media_player = (GtkMediaPlayer*)item->data;
      if(media_player->status == PLAYERCTL_PLAYBACK_STATUS_PLAYING || 
        media_player->status == PLAYERCTL_PLAYBACK_STATUS_PAUSED){

        GError* err = NULL;
        guint64 pos = playerctl_player_get_position(self->current_player, &err);
        if(err != NULL) pos=0;

        if(media_player->length <= 0) {
          media_player->length = gtk_media_player_get_length(self->current_player);
          if(media_player->length <= 0) return FALSE;
        }

        guint64 por = (pos*100)/media_player->length;
        guint64 bar_width = (width*por)/100;

        GdkRGBA color;
        gtk_style_context_get_color(context, GTK_STATE_FLAG_NORMAL, &color);

        cairo_set_line_width(cr, 2.0);
        cairo_set_source_rgba(cr, color.red,color.green,color.blue, color.alpha);
        cairo_move_to(cr,0,height);
        cairo_line_to(cr,bar_width,height);
        cairo_stroke(cr);
      }
    }
  }
  return FALSE;
}

static gboolean 
gtk_media_controller_title_scroll(gpointer user_data){
  GtkMediaController* self = GTK_MEDIA_CONTROLLER(user_data);

  if(!self->container || !gtk_widget_get_parent(GTK_WIDGET(self->container))) return TRUE;

  if(self->scroll_timer > 0){
    self->scroll_timer--;
    return TRUE;
  }

  GtkAdjustment* adjustment =  gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(self->title_scroll));
  double horizontal_position = gtk_adjustment_get_value(adjustment);
  double upper_limit = gtk_adjustment_get_upper(adjustment) - gtk_adjustment_get_page_size(adjustment);

  if(upper_limit <= 10) return TRUE;

  if(self->reversed_scroll){
    horizontal_position -= self->config->scroll_step;
    if(horizontal_position < 0) {
      horizontal_position = 0;
      gtk_media_controller_reset_title_scroll(self);
    }
  } else {
    horizontal_position += self->config->scroll_step;
    if(horizontal_position >= upper_limit){
      horizontal_position = upper_limit;
      self->reversed_scroll = TRUE;
    }
  }

  gtk_adjustment_set_value(adjustment, horizontal_position);
  return TRUE;
}

gboolean
gtk_media_controller_on_query_tooltip(GtkWidget* widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip* tooltip, gpointer user_data){
  printf("gtk_media_controller_on_query_tooltip entered\n");
  GtkMediaController* self = GTK_MEDIA_CONTROLLER(user_data);
  GError * err = NULL;

  if(self->current_player){
    gchar* art_url = playerctl_player_print_metadata_prop(self->current_player, "mpris:artUrl", &err);
    if(err != NULL) return FALSE;

    printf("Album art url = %s\n", art_url);

    if (g_str_has_prefix(art_url, "file://")) {
      gint width, height, image_width, image_height;
      GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file(art_url + strlen("file://"), &err);
      
      g_free(art_url);
      
      if(err != NULL){
        printf("Error reading album art image: %s", err->message);
        return FALSE;
      }

      if(pixbuf){
        image_width = gdk_pixbuf_get_width(pixbuf);
        image_height = gdk_pixbuf_get_height(pixbuf);

        gdouble ratio = (gdouble)image_height/(gdouble)image_width;

        width = self->config->tooltip_image_width;
        height = width*ratio;

        GdkPixbuf* pixbuf_scaled = gdk_pixbuf_scale_simple(pixbuf, width, height, GDK_INTERP_BILINEAR);
        if (pixbuf_scaled) {
          gtk_image_set_from_pixbuf(GTK_IMAGE(self->tooltip_image), pixbuf_scaled);
          gtk_widget_set_size_request(GTK_WIDGET(self->tooltip_image), width, height);
          g_object_unref(pixbuf_scaled);
        } else {
          printf("Pixbuf can not be read.\n");
          return FALSE;
        }
        g_object_unref(pixbuf);
      } else {
        return FALSE;
      }
    } else {
      g_free(art_url);
      return FALSE;
    }
  }

  return TRUE;
}

GtkMediaController*
gtk_media_controller_new(MediaPlayerModConfig* config){
  printf("gtk_media_controller_new entered\n");
  GtkMediaController* self = g_object_new(GTK_TYPE_MEDIA_CONTROLLER, NULL);

  if(!config) return NULL;

  self->config = config;

  self->container = GTK_CONTAINER(gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5));
  gtk_widget_set_name(GTK_WIDGET(self->container),"media_player");
  g_signal_connect(self->container,"draw",G_CALLBACK(gtk_media_controller_on_draw_progress), self);

  g_object_ref(self->container);

  if(config->tooltip){
    g_signal_connect(self->container,"query-tooltip", G_CALLBACK(gtk_media_controller_on_query_tooltip), self);

    self->tooltip_window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_POPUP));
    gtk_widget_set_tooltip_window(GTK_WIDGET(self->container), GTK_WINDOW(self->tooltip_window));

    GtkBox* tooltip_container = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL,5));
    gtk_container_add(GTK_CONTAINER(self->tooltip_window), GTK_WIDGET(tooltip_container));

    self->tooltip_image = GTK_IMAGE(gtk_image_new());
    gtk_container_add(GTK_CONTAINER(tooltip_container), GTK_WIDGET(self->tooltip_image));
    gtk_widget_set_size_request(GTK_WIDGET(self->tooltip_image), config->tooltip_image_width, config->tooltip_image_height);

    gtk_widget_show_all(GTK_WIDGET(tooltip_container));
  }

  GtkEventBox* player_event = GTK_EVENT_BOX(gtk_event_box_new());
  gtk_container_add(GTK_CONTAINER(self->container), GTK_WIDGET(player_event));
  g_signal_connect(GTK_WIDGET(player_event), "button-press-event", G_CALLBACK(gtk_media_controller_on_player_bp), self);

  self->player_text = GTK_LABEL(gtk_label_new(""));
  gtk_container_add(GTK_CONTAINER(player_event), GTK_WIDGET(self->player_text));
  GtkStyleContext* context = gtk_widget_get_style_context(GTK_WIDGET(self->player_text));
  gtk_style_context_add_class(context,"players");

  GtkContainer* button_container = GTK_CONTAINER(gtk_box_new(GTK_ORIENTATION_HORIZONTAL,1));
  gtk_container_add(GTK_CONTAINER(self->container), GTK_WIDGET(button_container));

  self->btn_prev = GTK_BUTTON(gtk_button_new_with_label(config->btn_prev));
  gtk_container_add(GTK_CONTAINER(button_container), GTK_WIDGET(self->btn_prev));
  context = gtk_widget_get_style_context(GTK_WIDGET(self->btn_prev));
  gtk_style_context_add_class(context,"button");
  gtk_style_context_add_class(context,"prev-button");
  g_signal_connect(self->btn_prev,"clicked",G_CALLBACK(gtk_media_controller_on_prev_click), self);

  self->btn_play = GTK_BUTTON(gtk_button_new_with_label(config->btn_play));
  gtk_container_add(GTK_CONTAINER(button_container), GTK_WIDGET(self->btn_play));
  context = gtk_widget_get_style_context(GTK_WIDGET(self->btn_play));
  gtk_style_context_add_class(context,"button");
  gtk_style_context_add_class(context,"play-button");
  g_signal_connect(self->btn_play,"clicked",G_CALLBACK(gtk_media_controller_on_play_click), self);

  self->btn_next = GTK_BUTTON(gtk_button_new_with_label(config->btn_next));
  gtk_container_add(GTK_CONTAINER(button_container), GTK_WIDGET(self->btn_next));
  context = gtk_widget_get_style_context(GTK_WIDGET(self->btn_next));
  gtk_style_context_add_class(context,"button");
  gtk_style_context_add_class(context,"next-button");
  g_signal_connect(self->btn_next,"clicked",G_CALLBACK(gtk_media_controller_on_next_click), self);

  GtkEventBox* title_event = GTK_EVENT_BOX(gtk_event_box_new());
  gtk_container_add(GTK_CONTAINER(self->container), GTK_WIDGET(title_event));
  g_signal_connect(GTK_WIDGET(title_event), "button-press-event", G_CALLBACK(gtk_media_controller_on_title_bp), self);

  self->title_scroll = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(
                        gtk_adjustment_new(0, 0, 0, 1.0, 0, 0), 
                        gtk_adjustment_new(0, 0, 0, 0, 0, 0)));
  gtk_container_add(GTK_CONTAINER(title_event), GTK_WIDGET(self->title_scroll));
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(self->title_scroll), GTK_POLICY_EXTERNAL, GTK_POLICY_EXTERNAL);

  self->title = GTK_LABEL(gtk_label_new(""));
  gtk_container_add(GTK_CONTAINER(self->title_scroll), GTK_WIDGET(self->title));
  context = gtk_widget_get_style_context(GTK_WIDGET(self->title));
  gtk_style_context_add_class(context,"title");
  gtk_widget_set_halign (GTK_WIDGET(self->title), GTK_ALIGN_START);

  if(self->config->scroll_title){
    gtk_media_controller_reset_title_scroll(self);

   self->scroll_timeout = g_timeout_add(self->config->scroll_interval,gtk_media_controller_title_scroll, self);
  }


  gtk_media_controller_update(self);

  return self;
}

