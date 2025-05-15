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

#include "waybar_cffi_module.h"

typedef struct _GtkMediaController GtkMediaController;

typedef struct {
  wbcffi_module* waybar_module;
  GtkMediaController* container;
} MediaPlayerMod;

typedef struct {
  gboolean scroll_title;
  gint title_max_width;
  gint scroll_before_timeout;
  gint scroll_interval;
  gint scroll_step;
  gboolean tooltip;
  gboolean tooltip_image_width;
  gboolean tooltip_image_height;
} MediaPlayerModConfig;


