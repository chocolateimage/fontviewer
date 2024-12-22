/*
 * Copyright (C) 2011 Red Hat, Inc.
 * Copyright (C) 2014 Khaled Hosny <khaledhosny@eglug.org>.
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
 *
 * The Sushi project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Sushi. This
 * permission is above and beyond the permissions granted by the GPL license
 * Sushi is covered by.
 *
 * Authors: Cosimo Cecchi <cosimoc@redhat.com>
 *
 */

#include "sushi-font-widget.h"
#include "sushi-font-loader.h"

#include <hb-glib.h>

enum {
  PROP_URI = 1,
  PROP_FACE_INDEX,
  NUM_PROPERTIES
};

enum {
  LOADED,
  ERROR,
  NUM_SIGNALS
};

struct _SushiFontWidget {
  GtkDrawingArea parent_instance;

  gchar *uri;
  gint face_index;

  FT_Library library;
  FT_Face face;
  gchar *face_contents;

  const gchar *lowercase_text;
  const gchar *uppercase_text;
  const gchar *punctuation_text;

  gchar *sample_string;

  gchar *font_name;
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };
static guint signals[NUM_SIGNALS] = { 0, };

G_DEFINE_TYPE (SushiFontWidget, sushi_font_widget, GTK_TYPE_DRAWING_AREA)

#define SURFACE_SIZE 4

static void
text_to_glyphs (cairo_t *cr,
                const gchar *text,
                cairo_glyph_t **glyphs,
                int *num_glyphs)
{
  PangoAttribute *fallback_attr;
  PangoAttrList *attr_list;
  PangoContext *context;
  PangoDirection base_dir;
  GList *items;
  GList *visual_items;
  FT_Face ft_face;
  hb_font_t *hb_font;
  gdouble x = 0, y = 0;
  gint i;
  gdouble x_scale, y_scale;

  *num_glyphs = 0;
  *glyphs = NULL;

  base_dir = PANGO_DIRECTION_NEUTRAL;//pango_find_base_dir (text, -1);

  cairo_scaled_font_t *cr_font = cairo_get_scaled_font (cr);
  ft_face = cairo_ft_scaled_font_lock_face (cr_font);
  hb_font = hb_ft_font_create (ft_face, NULL);

  cairo_surface_t *target = cairo_get_target (cr);
  cairo_surface_get_device_scale (target, &x_scale, &y_scale);

  /* We abuse pango itemazation to split text into script and direction
   * runs, since we use our fonts directly no through pango, we don't
   * bother changing the default font, but we disable font fallback as
   * pango will split runs at font change */
  context = pango_cairo_create_context (cr);
  attr_list = pango_attr_list_new ();
  fallback_attr = pango_attr_fallback_new (FALSE);
  pango_attr_list_insert (attr_list, fallback_attr);
  items = pango_itemize_with_base_dir (context, base_dir,
                                       text, 0, strlen (text),
                                       attr_list, NULL);
  g_object_unref (context);
  pango_attr_list_unref (attr_list);

  /* reorder the items in the visual order */
  visual_items = pango_reorder_items (items);

  while (visual_items) {
    PangoItem *item;
    PangoAnalysis analysis;
    hb_buffer_t *hb_buffer;
    hb_glyph_info_t *hb_glyphs;
    hb_glyph_position_t *hb_positions;
    gint n;

    item = visual_items->data;
    analysis = item->analysis;

    hb_buffer = hb_buffer_create ();
    hb_buffer_add_utf8 (hb_buffer, text, -1, item->offset, item->length);
    hb_buffer_set_script (hb_buffer, hb_glib_script_to_script (analysis.script));
    hb_buffer_set_language (hb_buffer, hb_language_from_string (pango_language_to_string (analysis.language), -1));
    hb_buffer_set_direction (hb_buffer, analysis.level % 2 ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);

    hb_shape (hb_font, hb_buffer, NULL, 0);

    n = hb_buffer_get_length (hb_buffer);
    hb_glyphs = hb_buffer_get_glyph_infos (hb_buffer, NULL);
    hb_positions = hb_buffer_get_glyph_positions (hb_buffer, NULL);

    *glyphs = g_renew (cairo_glyph_t, *glyphs, *num_glyphs + n);

    for (i = 0; i < n; i++) {
      (*glyphs)[*num_glyphs + i].index = hb_glyphs[i].codepoint;
      (*glyphs)[*num_glyphs + i].x = x + (hb_positions[i].x_offset / (64. * x_scale));
      (*glyphs)[*num_glyphs + i].y = y - (hb_positions[i].y_offset / (64. * y_scale));
      x += (hb_positions[i].x_advance / (64. * x_scale));
      y -= (hb_positions[i].y_advance / (64. * y_scale));
    }

    *num_glyphs += n;

    hb_buffer_destroy (hb_buffer);

    visual_items = visual_items->next;
  }

  g_list_free_full (visual_items, (GDestroyNotify) pango_item_free);
  g_list_free_full (items, (GDestroyNotify) pango_item_free);

  hb_font_destroy (hb_font);
  cairo_ft_scaled_font_unlock_face (cr_font);
}

/* adapted from gnome-utils:font-viewer/font-view.c
 *
 * Copyright (C) 2002-2003  James Henstridge <james@daa.com.au>
 * Copyright (C) 2010 Cosimo Cecchi <cosimoc@gnome.org>
 *
 * License: GPLv2+
 */
static void
draw_string (SushiFontWidget *self,
             cairo_t *cr,
             GtkBorder padding,
	     const gchar *text,
	     gint *pos_y)
{
  g_autofree cairo_glyph_t *glyphs = NULL;
  cairo_font_extents_t font_extents;
  cairo_text_extents_t extents;
  GtkTextDirection text_dir;
  gint pos_x;
  gint num_glyphs;
  gint i;

  text_dir = gtk_widget_get_direction (GTK_WIDGET (self));

  text_to_glyphs (cr, text, &glyphs, &num_glyphs);

  cairo_font_extents (cr, &font_extents);
  cairo_glyph_extents (cr, glyphs, num_glyphs, &extents);

  if (pos_y != NULL)
    *pos_y += font_extents.ascent + extents.y_advance;
  if (text_dir == GTK_TEXT_DIR_LTR)
    pos_x = padding.left;
  else {
    pos_x = gtk_widget_get_allocated_width (GTK_WIDGET (self)) -
      extents.x_advance - padding.right;
  }

  for (i = 0; i < num_glyphs; i++) {
    glyphs[i].x += pos_x;
    glyphs[i].y += *pos_y;
  }

  cairo_move_to (cr, pos_x, *pos_y);
  cairo_show_glyphs (cr, glyphs, num_glyphs);
}

static gchar *
build_charlist_for_face (FT_Face face,
                         gint *length)
{
  g_autoptr(GString) string = NULL;
  gulong c;
  guint glyph;
  gint total_chars = 0;

  string = g_string_new (NULL);

  c = FT_Get_First_Char (face, &glyph);

  while (glyph != 0) {
    g_string_append_unichar (string, (gunichar) c);
    c = FT_Get_Next_Char (face, c, &glyph);
    total_chars++;
  }

  if (length)
    *length = total_chars;

  return g_strdup (string->str);
}

static void
select_best_charmap (SushiFontWidget *self)
{
  gchar *chars;
  gint idx, n_chars;

  if (FT_Select_Charmap (self->face, FT_ENCODING_UNICODE) == 0)
    return;

  for (idx = 0; idx < self->face->num_charmaps; idx++) {
    if (FT_Set_Charmap (self->face, self->face->charmaps[idx]) != 0)
      continue;

    chars = build_charlist_for_face (self->face, &n_chars);
    g_free (chars);

    if (n_chars > 0)
      break;
  }
}

static void
build_strings_for_face (SushiFontWidget *self)
{
  select_best_charmap (self);

  self->uppercase_text = NULL;
  self->punctuation_text = NULL;
  self->sample_string = NULL;

  g_free (self->font_name);
  self->font_name = sushi_get_font_name (self->face, FALSE);
}

static gint get_size_from_face(FT_Face face)
{
  gint *sizes = NULL;
  gint i;
  gint n_sizes;

  /* work out what sizes to render */
  if (FT_IS_SCALABLE (face)) {
    return 28;
  } else {
    gint diff = G_MAXINT;
    gint size = G_MAXINT;

    /* use fixed sizes */
    n_sizes = face->num_fixed_sizes;
    sizes = g_new (gint, n_sizes);
    size = 0;

    for (i = 0; i < face->num_fixed_sizes; i++) {
      sizes[i] = face->available_sizes[i].height;

      if ((gint) (abs (sizes[i] - 24)) < diff) {
        diff = (gint) abs (sizes[i] - 24);
        size = sizes[i];
      }
    }
    return size;
  }
}

static void
sushi_font_widget_get_preferred_width (GtkWidget *drawing_area,
                                       gint *minimum_width,
                                       gint *natural_width)
{
  *minimum_width = 0;
  *natural_width = 0;
}

static void
sushi_font_widget_get_preferred_height (GtkWidget *drawing_area,
                                        gint *minimum_height,
                                        gint *natural_height)
{
  *minimum_height = 40;
  *natural_height = 40;
}

static gboolean
sushi_font_widget_draw (GtkWidget *drawing_area,
                        cairo_t *cr)
{
  SushiFontWidget *self = SUSHI_FONT_WIDGET (drawing_area);
  g_autofree gint *sizes = NULL;
  gint font_size, pos_y = 0;
  cairo_font_face_t *font = NULL;
  FT_Face face = self->face;
  GtkStyleContext *context;
  GdkRGBA color;
  GtkBorder padding;
  GtkStateFlags state;
  gint allocated_width, allocated_height;

  if (face == NULL)
    return FALSE;

  context = gtk_widget_get_style_context (drawing_area);
  if (!gtk_style_context_has_class(context,"sushi-font-widget")) {
    gtk_style_context_add_class(context,"sushi-font-widget");
  }
  state = gtk_style_context_get_state (context);


  allocated_width = gtk_widget_get_allocated_width (drawing_area);
  allocated_height = gtk_widget_get_allocated_height (drawing_area);

  gtk_style_context_get_color (context, state, &color);
  gtk_style_context_get_padding (context, state, &padding);

  /* do stuff with text */

  cairo_surface_t* crsurface = cairo_get_target(cr);
  cairo_surface_t* cr2surface = cairo_surface_create_similar_image(
    crsurface,
    CAIRO_FORMAT_ARGB32,
    allocated_width,
    allocated_height
  );
  cairo_t* cr2 = cairo_create(cr2surface);


  gdk_cairo_set_source_rgba (cr2, &color);

  font_size = get_size_from_face (face);

  font = cairo_ft_font_face_create_for_ft_face (face, 0);

  /* draw text */

  cairo_set_font_face (cr2, font);
  cairo_set_font_size (cr2, font_size);


  if (self->lowercase_text != NULL)
    draw_string (self, cr2, padding, self->lowercase_text, &pos_y);

  cairo_pattern_t* linear_gradient = cairo_pattern_create_linear(0,0,allocated_width,allocated_height);
  cairo_pattern_add_color_stop_rgba(linear_gradient,0,1,1,1,1);
  cairo_pattern_add_color_stop_rgba(linear_gradient,0.8,1,1,1,1);
  cairo_pattern_add_color_stop_rgba(linear_gradient,1,1,1,1,0);

  cairo_set_source_surface(cr,cr2surface,0,0);
  cairo_mask(cr,linear_gradient);

  cairo_pattern_destroy(linear_gradient);
  cairo_destroy(cr2);
  cairo_surface_destroy(cr2surface);

  return FALSE;
}

static void
font_face_async_ready_cb (GObject *object,
                          GAsyncResult *result,
                          gpointer user_data)
{
  SushiFontWidget *self = user_data;
  g_autoptr(GError) error = NULL;

  self->face =
    sushi_new_ft_face_from_uri_finish (result,
                                       &self->face_contents,
                                       &error);

  if (error != NULL) {
    g_print ("Can't load the font face: %s\n", error->message);
    g_signal_emit (self, signals[ERROR], 0, error);

    return;
  }

  build_strings_for_face (self);

  gtk_widget_queue_resize (GTK_WIDGET (self));
  g_signal_emit (self, signals[LOADED], 0);
}

void
sushi_font_widget_load (SushiFontWidget *self)
{
  sushi_new_ft_face_from_uri_async (self->library,
                                    self->uri,
                                    self->face_index,
                                    font_face_async_ready_cb,
                                    self);
}

static void
sushi_font_widget_init (SushiFontWidget *self)
{
  FT_Error err = FT_Init_FreeType (&self->library);

  if (err != FT_Err_Ok)
    g_error ("Unable to initialize FreeType");

  gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (self)),
                               GTK_STYLE_CLASS_VIEW);
}

static void
sushi_font_widget_get_property (GObject *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  SushiFontWidget *self = SUSHI_FONT_WIDGET (object);

  switch (prop_id) {
  case PROP_URI:
    g_value_set_string (value, self->uri);
    break;
  case PROP_FACE_INDEX:
    g_value_set_int (value, self->face_index);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
sushi_font_widget_set_property (GObject *object,
                               guint       prop_id,
                               const GValue *value,
                               GParamSpec *pspec)
{
  SushiFontWidget *self = SUSHI_FONT_WIDGET (object);

  switch (prop_id) {
  case PROP_URI:
    self->uri = g_value_dup_string (value);
    break;
  case PROP_FACE_INDEX:
    self->face_index = g_value_get_int (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
sushi_font_widget_finalize (GObject *object)
{
  SushiFontWidget *self = SUSHI_FONT_WIDGET (object);

  g_free (self->uri);

  g_free (self->font_name);
  g_free (self->sample_string);
  g_free (self->face_contents);

  G_OBJECT_CLASS (sushi_font_widget_parent_class)->finalize (object);
}

static void
sushi_font_widget_constructed (GObject *object)
{
  SushiFontWidget *self = SUSHI_FONT_WIDGET (object);

  sushi_font_widget_load (self);

  G_OBJECT_CLASS (sushi_font_widget_parent_class)->constructed (object);
}

static void
sushi_font_widget_class_init (SushiFontWidgetClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wclass = GTK_WIDGET_CLASS (klass);

  oclass->finalize = sushi_font_widget_finalize;
  oclass->set_property = sushi_font_widget_set_property;
  oclass->get_property = sushi_font_widget_get_property;
  oclass->constructed = sushi_font_widget_constructed;

  wclass->draw = sushi_font_widget_draw;
  wclass->get_preferred_width = sushi_font_widget_get_preferred_width;
  wclass->get_preferred_height = sushi_font_widget_get_preferred_height;

  properties[PROP_URI] =
    g_param_spec_string ("uri",
                         "Uri", "Uri",
                         NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  properties[PROP_FACE_INDEX] =
    g_param_spec_int ("face-index",
                      "Face index", "Face index",
                      0, G_MAXINT,
                      0, G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

  signals[LOADED] =
    g_signal_new ("loaded",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
  signals[ERROR] =
    g_signal_new ("error",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_ERROR);

  g_object_class_install_properties (oclass, NUM_PROPERTIES, properties);
}

SushiFontWidget *
sushi_font_widget_new (const gchar *uri, gint face_index)
{
  return g_object_new (SUSHI_TYPE_FONT_WIDGET,
                       "uri", uri,
                       "face-index", face_index,
                       NULL);
}

/**
 * sushi_font_widget_get_ft_face: (skip)
 *
 */
FT_Face
sushi_font_widget_get_ft_face (SushiFontWidget *self)
{
  return self->face;
}

const gchar *
sushi_font_widget_get_uri (SushiFontWidget *self)
{
  return self->uri;
}

void sushi_font_widget_set_text(SushiFontWidget *self, gchar* text) {

  self->lowercase_text = text;
}