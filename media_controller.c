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

#define G_LOG_DOMAIN "waybarmediaplayer.media-controller"

#include <math.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <cairo.h>
#include <pango/pango.h>
#include <glib-object.h>
#include <glib.h>

#include "media_controller.h"

#include "mpris_media_manager.h"
#include "mpris_media_player.h"

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

  /* new MPRIS functions */
  GMprisMediaManager* media_manager;
  GMprisMediaPlayer* current_player;
  GList* media_players;

  /* old playerctl functions */
  //PlayerctlPlayerManager* player_manager;
  //PlayerctlPlayer* current_player;
  //GList* media_players;

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
  GTK_MEDIA_CONTROLLER_PROP_CONFIG,
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

    case GTK_MEDIA_CONTROLLER_PROP_CONFIG:
      self->config = g_value_get_pointer(value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gtk_media_controller_finalize(GObject * object)
{
  printf("gtk_media_controller_finalize entered\n");
  GtkMediaController *self = GTK_MEDIA_CONTROLLER(object);

  self->state = GTK_MEDIA_CONTROLLER_STATE_STOPPED;

  g_source_remove(self->scroll_timeout);

  if (self->config){
    g_free(self->config->btn_play);
    g_free(self->config->btn_pause);
    g_free(self->config->btn_next);
    g_free(self->config->btn_prev);
    g_free(self->config->ignored_players);
    g_free(self->config);
  }

  if (self->media_players) {
    g_list_free_full(self->media_players, g_object_unref);
    self->media_players = NULL;
  }

  if(self->media_manager){
    g_object_unref(self->media_manager);
    self->media_manager = NULL;
  }

  g_object_unref(self->container);
  self->container = NULL;

  G_OBJECT_CLASS
      (gtk_media_controller_parent_class)->finalize(object);

  printf("gtk_media_controller_finalized exited\n");
}

static void 
gtk_media_controller_update(GtkMediaController* self) {
  printf("gtk_media_controller_update entered\n");

  if(!self->container) return;

  guint pos = 0;
  guint size = 0;
  for(struct {int idx; GList* item; } loop = {0, g_list_first(self->media_players)}; loop.item; loop.item = loop.item->next){
    GMprisMediaPlayer* player = (GMprisMediaPlayer*)loop.item->data;

    if(g_is_mpris_media_player_available(player)) { 
      size ++; loop.idx++; 
    } else { 
      continue; 
    };

    if(g_mpris_media_player_compare(player, self->current_player) == 0){
      pos = loop.idx; 
    }
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
      gchar* artist = NULL;
      gchar* title = NULL;
      g_object_get(G_OBJECT(self->current_player), 
                    "artist", &artist,
                    "title", &title,
                    NULL); 

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

        g_free(final_title);
      } else {
        gtk_label_set_text(self->title, "No Media");
      }

      PangoLayout* layout = gtk_label_get_layout(GTK_LABEL(self->title));
      gint min_width, min_height; 
      pango_layout_get_pixel_size(layout, &min_width, &min_height);
      if(min_width > self->config->title_max_width) min_width = self->config->title_max_width;
      gtk_widget_set_size_request(GTK_WIDGET(self->title_scroll), min_width, 0);

      if(artist)
        g_free(artist);
      if(title)
        g_free(title);
    }

    if(self->media_players && self->current_player){
      GList* item = g_list_find_custom(g_list_first(self->media_players), self->current_player, g_mpris_media_player_compare);
      if(item){
        GMprisMediaPlayer* player = (GMprisMediaPlayer*)item->data;

        GMprisMediaPlayerState state;
        g_object_get(G_OBJECT(player), "state", &state, NULL); 

        if(state == G_MPRIS_MEDIA_PLAYER_STATE_PLAYING){
          gtk_button_set_label(self->btn_play, self->config->btn_pause);
        } else {
          gtk_button_set_label(self->btn_play, self->config->btn_play);
        }

        gboolean val = FALSE;
        g_object_get(G_OBJECT(player), "can-go-previous", &val, NULL);
        printf("can-go-previous => %s\n", (val ? "TRUE" : "FALSE"));
        if(!val)
          gtk_widget_hide(GTK_WIDGET(self->btn_prev));
        else
          gtk_widget_show(GTK_WIDGET(self->btn_prev));

        g_object_get(G_OBJECT(player), "can-go-next", &val, NULL);
        printf("can-go-next => %s\n", (val ? "TRUE" : "FALSE"));
        if(!val)
          gtk_widget_hide(GTK_WIDGET(self->btn_next));
        else
          gtk_widget_show(GTK_WIDGET(self->btn_next));
      }
    }
  }
  printf("gtk_media_controller_update exited\n");
}

static void 
gtk_media_controller_reset_title_scroll(GtkMediaController* self, gboolean reversed){
  self->reversed_scroll = reversed;

  if(self->config)
    self->scroll_timer = self->config->scroll_before_timeout*(1000/self->config->scroll_interval);

  if(self->container && gtk_widget_get_parent(GTK_WIDGET(self->container)) && !reversed){
    GtkAdjustment* adjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(self->title_scroll));
    if(adjustment != NULL)
      gtk_adjustment_set_value(adjustment, 0);
  }
}

static void
gtk_media_controller_set_player(GtkMediaController* self, GMprisMediaPlayer* player){
  if(self->current_player != NULL && player != NULL && g_mpris_media_player_compare(self->current_player,player) != 0){
    gtk_media_controller_reset_title_scroll(self, FALSE);
  }

  self->current_player = player;

  if(player != NULL){
    gchar* iface; 
    g_object_get(G_OBJECT(player), "iface", &iface, NULL);
    g_debug("Player selected: %s\n", iface);
    g_free(iface);
  } else {
    g_debug("No next player to be selected.\n");
  }
}

static void 
gtk_media_controller_player_add(GtkMediaController* self, GMprisMediaPlayer* player){
  printf("gtk_media_controller_player_add entered\n");

  GList* item = g_list_find_custom(g_list_first(self->media_players), player, g_mpris_media_player_compare);
  if(item == NULL){
    self->media_players = g_list_append(self->media_players, player);

    printf("New player added to media controller \n");

    GMprisMediaPlayerState state;
    g_object_get(G_OBJECT(player), "state", &state, NULL);

    if((g_is_mpris_media_player_available(player) && self->current_player == NULL) || state == G_MPRIS_MEDIA_PLAYER_STATE_PLAYING)
      gtk_media_controller_set_player(self, player);
  } else {

    GMprisMediaPlayer* player = (GMprisMediaPlayer*)item->data;

    GMprisMediaPlayerState state;
    g_object_get(G_OBJECT(player), "state", &state, NULL);

    if((g_is_mpris_media_player_available(player) && self->current_player == NULL) || state == G_MPRIS_MEDIA_PLAYER_STATE_PLAYING)
      gtk_media_controller_set_player(self, player);
  }
  printf("gtk_media_controller_player_add exited\n");
}

static void 
gtk_media_controller_select_next_player(GtkMediaController* self, GMprisMediaPlayer* player) {

  if(player){
    GList* item = g_list_find_custom(self->media_players, player, g_mpris_media_player_compare);
    if(item){
      GMprisMediaPlayer* next_player = NULL;
      for(GList* next = item->next; next; next = next->next){
        GMprisMediaPlayer* p = (GMprisMediaPlayer*)next->data;
        if(g_is_mpris_media_player_available(p)) {
          next_player = p;
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
    GMprisMediaPlayer* next_player = NULL;
    for(GList* next = item; next; next = next->next){
      GMprisMediaPlayer* p = (GMprisMediaPlayer*)next->data;
      if(g_is_mpris_media_player_available(p)) {
        next_player = p;
        break;
      }
    }
    gtk_media_controller_set_player(self, next_player);
  }
  return;
}

static void 
gtk_media_controller_player_remove(GtkMediaController* self, GMprisMediaPlayer* player) {
  g_debug("gtk_media_controller_player_remove entered\n");

  if(self->media_players != NULL){
    GList* item = g_list_find_custom(g_list_first(self->media_players), player, g_mpris_media_player_compare);
    if(item != NULL){
      gtk_media_controller_select_next_player(self, player);
      self->media_players = g_list_remove_link(self->media_players, item);
      g_list_free_full(item, g_object_unref);
    }
  } else {
      gchar* iface = NULL;
      g_object_get(G_OBJECT(player), "iface", &iface, NULL);
      g_warning("Media player not found to remove - %s", iface);
      g_free(iface);
      gtk_media_controller_set_player(self, NULL);
  }
  gtk_media_controller_update(self);
  g_debug("gtk_media_controller_player_remove exit\n");
}

static void 
gtk_media_controller_on_player_property_changed(GMprisMediaPlayer* player, gpointer user_data) {
  g_debug("gtk_media_controller_on_player_property_changed entered\n");

  GtkMediaController* self = GTK_MEDIA_CONTROLLER(user_data);

  gtk_media_controller_update(self);

  if(!g_is_mpris_media_player_available(player)){
    gtk_media_controller_select_next_player(self, player);
    gtk_media_controller_update(self);
  }

  g_debug("gtk_media_controller_on_player_property_changed exited\n");
}

static void 
gtk_media_controller_on_player_state_changed(GMprisMediaPlayer* player, gpointer user_data) {
  g_debug("gtk_media_controller_on_player_state_changed entered\n");

  GtkMediaController* self = GTK_MEDIA_CONTROLLER(user_data);

  GMprisMediaPlayerState state;
  g_object_get(G_OBJECT(player), "state", &state, NULL);

  if(g_is_mpris_media_player_available(player) && 
    state == G_MPRIS_MEDIA_PLAYER_STATE_PLAYING)
      gtk_media_controller_set_player(self, player); 

  g_debug("gtk_media_controller_on_player_state_changed exited\n");
}


static void mpris_on_player_added(GMprisMediaManager* manager, GMprisMediaPlayer* player, gpointer user_data){
  g_debug("mpris_on_player_added entered\n");

  GtkMediaController* self = GTK_MEDIA_CONTROLLER(user_data);

  //TODO: Improve this code somewhere else... create the list when reading the
  //configuration file...
  if(self->config && self->config->ignored_players && strlen(self->config->ignored_players) > 0) {
    gchar* iface = NULL;
    g_object_get(G_OBJECT(player), "iface", &iface, NULL);

    if (iface) {
      gchar* iface_lower = g_ascii_strdown(iface, -1);
      gchar* ignored_players_lower = g_ascii_strdown(self->config->ignored_players, -1);

      gchar** ignored_list = g_strsplit(ignored_players_lower, ",", -1);
      gboolean should_ignore = FALSE;

      printf("Searching %s in %s\n", ignored_players_lower, iface_lower);

      for (gint i = 0; ignored_list[i] != NULL; i++) {
        gchar* trimmed_name = g_strstrip(g_strdup(ignored_list[i]));

        if (strlen(trimmed_name) > 0 && strstr(iface, trimmed_name) != NULL) {
          should_ignore = TRUE;
          printf("Ignoring player with interface: %s (matches ignored pattern: %s)\n", iface, trimmed_name);
          g_free(trimmed_name);
          break;
        }
        g_free(trimmed_name);
      }

      g_free(ignored_players_lower);
      g_strfreev(ignored_list);
      g_free(iface_lower);
      g_free(iface);

      // If player should be ignored, return early without adding it
      if (should_ignore) {
        g_warning("mpris_on_player_added exited early (player ignored)\n");
        return;
      }
    }
  }

  g_object_ref(player);

  g_signal_connect(player, "property-changed", 
        G_CALLBACK(gtk_media_controller_on_player_property_changed), self);

  g_signal_connect(player, "state-changed",
        G_CALLBACK(gtk_media_controller_on_player_state_changed), self);

  gtk_media_controller_player_add(self, player);
  gtk_media_controller_update(self);

  g_debug("mpris_on_player_added exited\n");
}

static void mpris_on_player_removed(GMprisMediaManager* manager, GMprisMediaPlayer* player, gpointer user_data){
  g_debug("mpris_on_player_removed entered\n");
  GtkMediaController* self = GTK_MEDIA_CONTROLLER(user_data);
  gtk_media_controller_player_remove(self,player);
  gtk_media_controller_update(self);
  g_debug("mpris_on_player_removed exited\n");
}

static void
gtk_media_controller_constructed(GObject* object)
{
  g_debug("gtk_media_controller_constructed\n");
  GtkMediaController *self = GTK_MEDIA_CONTROLLER(object);

  self->media_manager = g_mpris_media_manager_new();

  g_signal_connect(self->media_manager, "player-added", G_CALLBACK(mpris_on_player_added), self);

  g_signal_connect(self->media_manager, "player-removed", G_CALLBACK(mpris_on_player_removed), self);

  g_mpris_media_manager_start(self->media_manager);

  g_debug("gtk_media_controller_constructed fnished\n");
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
        g_param_spec_enum ("state", "State",
        "Media Controller current state", GTK_TYPE_MEDIA_CONTROLLER_STATE,
        GTK_MEDIA_CONTROLLER_STATE_IDLE,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

  gtk_media_controller_param_specs[GTK_MEDIA_CONTROLLER_PROP_CONFIG] =
        g_param_spec_pointer("config", "Config",
        "Media Player Configuration",
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

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
  g_info("Initializing player\n");
  self->media_players = NULL;
  self->current_player = NULL;
}

void static 
gtk_media_controller_on_title_bp(GtkLabel* title, GdkEventButton* event, gpointer user_data){
  GtkMediaController* self = GTK_MEDIA_CONTROLLER(user_data);

  if(event->button == 1){
    if(self->current_player) {
      g_mpris_media_player_play_pause(self->current_player);
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

    g_debug("Left mouse click on player detected\n");
  }
}

static void 
gtk_media_controller_on_prev_click(GtkButton* btn, gpointer user_data) {
  g_debug("gtk_media_controller_on_prev_click entered\n");
  GtkMediaController* self = GTK_MEDIA_CONTROLLER(user_data);

  g_mpris_media_player_previous(self->current_player);

  gtk_media_controller_reset_title_scroll(self, FALSE);
  g_debug("gtk_media_controller_on_prev_click exited\n");
 }

static void 
gtk_media_controller_on_play_click(GtkButton* btn, gpointer user_data) {
  g_debug("gtk_media_controller_on_play_click entered\n");
  GtkMediaController* self = GTK_MEDIA_CONTROLLER(user_data);
  g_mpris_media_player_play_pause(self->current_player);
  g_debug("gtk_media_controller_on_play_click exited\n");
 }

static void 
gtk_media_controller_on_next_click(GtkButton* btn, gpointer user_data) {
  g_debug("gtk_media_controller_on_next_click entered\n");
  GtkMediaController* self = GTK_MEDIA_CONTROLLER(user_data);
  g_mpris_media_player_next(self->current_player);
  gtk_media_controller_reset_title_scroll(self, FALSE);
  g_debug("gtk_media_controller_on_next_click exited\n");
 }

static gboolean 
gtk_media_controller_on_draw_progress(GtkWidget* widget, cairo_t* cr, gpointer user_data){
  GtkMediaController* self = GTK_MEDIA_CONTROLLER(user_data);

  if(self->current_player && self->media_players){
    GtkStyleContext* context = gtk_widget_get_style_context(widget);

    gint width = gtk_widget_get_allocated_width(widget);
    gint height = gtk_widget_get_allocated_height(widget);

    GList* item = g_list_find_custom(g_list_first(self->media_players), self->current_player, g_mpris_media_player_compare);
    if(item){
      GMprisMediaPlayer* player = (GMprisMediaPlayer*)item->data;

      GMprisMediaPlayerState state;
      g_object_get(G_OBJECT(player), "state", &state, NULL);

      if(state == G_MPRIS_MEDIA_PLAYER_STATE_PLAYING || 
         state == G_MPRIS_MEDIA_PLAYER_STATE_PAUSED){

        gint64 pos = 0;
        gint64 length = 0;
        g_object_get(G_OBJECT(player),"position", &pos, "length", &length, NULL);

        if(length > 0){
          guint64 por = (pos*100)/length;
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
  }
  return FALSE;
}

static gboolean 
gtk_media_controller_title_scroll(gpointer user_data){
  GtkMediaController* self = GTK_MEDIA_CONTROLLER(user_data);

  if(!self || !self->title_scroll || !self->container || !gtk_widget_get_parent(GTK_WIDGET(self->container))) return TRUE;

  if(!gtk_widget_is_visible(GTK_WIDGET(self->container)) || !gtk_widget_is_visible(GTK_WIDGET(self->title_scroll))) return TRUE;

  if(self->scroll_timer > 0){
    self->scroll_timer--;
    return TRUE;
  }

  GtkAdjustment* adjustment =  gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(self->title_scroll));
  double horizontal_position = gtk_adjustment_get_value(adjustment);
  double upper_limit = gtk_adjustment_get_upper(adjustment) - gtk_adjustment_get_page_size(adjustment);

  if(upper_limit <= 5) return TRUE;

  if(self->reversed_scroll){
    horizontal_position -= self->config->scroll_step;
    if(horizontal_position < 0) {
      horizontal_position = 0;
      gtk_media_controller_reset_title_scroll(self, FALSE);
    }
  } else {
    horizontal_position += self->config->scroll_step;
    if(horizontal_position >= upper_limit){
      horizontal_position = upper_limit;
      gtk_media_controller_reset_title_scroll(self, TRUE);
    }
  }

  gtk_adjustment_set_value(adjustment, horizontal_position);
  return TRUE;
}

gboolean
gtk_media_controller_on_query_tooltip(GtkWidget* widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip* tooltip, gpointer user_data){
  g_debug("gtk_media_controller_on_query_tooltip entered\n");
  
  GtkMediaController* self = GTK_MEDIA_CONTROLLER(user_data);
  GError* err = NULL;

  if(self->current_player){
    gchar* art_url = NULL;
    g_object_get(G_OBJECT(self->current_player), "arturl", & art_url, NULL);

    g_debug("Album art url = %s\n", art_url);

    GdkPixbuf* pixbuf = NULL;
    if (g_str_has_prefix(art_url, "file://")) {
      pixbuf = gdk_pixbuf_new_from_file(art_url + strlen("file://"), &err);
      if(err != NULL){
        g_error("Error reading album art image: %s", err->message);
        g_free(art_url);
        return FALSE;
      }
    }
    g_free(art_url);

    if(pixbuf){
      gint width, height, image_width, image_height;

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
        g_error("Pixbuf can not be read.\n");
        return FALSE;
      }
      g_object_unref(pixbuf);
    } else {
      return FALSE;
    }
  }

  return TRUE;
}

GtkMediaController*
gtk_media_controller_new(MediaPlayerModConfig* config){
  g_debug("gtk_media_controller_new entered\n");

  if(!config) return NULL;

  GtkMediaController* self = g_object_new(GTK_TYPE_MEDIA_CONTROLLER, 
                                  "config", config,
                                  "state", GTK_MEDIA_CONTROLLER_STATE_IDLE,
                                  NULL);


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
    gtk_media_controller_reset_title_scroll(self, FALSE);

   self->scroll_timeout = g_timeout_add(self->config->scroll_interval,gtk_media_controller_title_scroll, self);
  }


  gtk_media_controller_update(self);

  return self;
}

