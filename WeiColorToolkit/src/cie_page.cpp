#include "precompiled_header.hpp"

// WeiColorToolkit - A color theroy demonstration program.
// Copyright (C) <2007>  Wei Hu <wei.hu.tw@gmail.com>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "wct_types.hpp"
#include "wcl_colorlib/colorlib.hpp"

#define FIX_BG_SIZE

#define CHROMATICITY_DIAGRAM_FIX_WIDTH 400
#define CHROMATICITY_DIAGRAM_FIX_HEIGHT 400
#define SEL_COLOR_SPACE_TREEVIEW_SMALL_ICON_WIDTH 10
#define SEL_COLOR_SPACE_TREEVIEW_SMALL_ICON_HEIGHT 10

namespace
{
  int32_t curr_chromaticity_width;
  int32_t curr_chromaticity_height;
  
  GtkListStore *gp_sel_color_space_liststore = 0;
  GtkWidget *gp_color_button = 0;
  GtkWidget *gp_rgb_color_space_combo = 0;
  GtkWidget *gp_cie_chromaticity_diagram = 0;
  
  enum
  {
    SEL_COLOR_SPACE_TREEVIEW_COLUMN_PIXBUF = 0,
    SEL_COLOR_SPACE_TREEVIEW_COLUMN_RED,
    SEL_COLOR_SPACE_TREEVIEW_COLUMN_GREEN,
    SEL_COLOR_SPACE_TREEVIEW_COLUMN_BLUE,
    SEL_COLOR_SPACE_TREEVIEW_COLUMN_NAME,
    SEL_COLOR_SPACE_TREEVIEW_COLUMN_ACTIVE,
    SEL_COLOR_SPACE_TREEVIEW_COLUMN_SIZE
  };
  
  gboolean
  on_cie_chromaticity_diagram_drawing_area_configure_event(
    GtkWidget *widget,
    GdkEventConfigure *event,
    gpointer /* user_data */)
  {
    if ((curr_chromaticity_width != event->width) ||
        (curr_chromaticity_height != event->height))
    {
      {
        GdkPixbuf * const orig_bg =
          GDK_PIXBUF(g_object_get_data(G_OBJECT(widget), "orig_bg"));
        if (orig_bg != 0)
        {
          g_object_unref(orig_bg);
        }
        
        GdkPixmap * const curr_bg_wo_curr_color_space_sel =
          GDK_PIXMAP(g_object_get_data(G_OBJECT(widget),
                                       "curr_bg_wo_curr_color_space_sel"));
        if (curr_bg_wo_curr_color_space_sel != 0)
        {
          g_object_unref(curr_bg_wo_curr_color_space_sel);
        }
        
        GdkPixmap * const curr_bg =
          GDK_PIXMAP(g_object_get_data(G_OBJECT(widget), "curr_bg"));
        if (curr_bg != 0)
        {
          g_object_unref(curr_bg);
        }
      }
      
      {
#if defined(FIX_BG_SIZE)
        curr_chromaticity_width = CHROMATICITY_DIAGRAM_FIX_WIDTH;
        curr_chromaticity_height = CHROMATICITY_DIAGRAM_FIX_HEIGHT;
#else
        curr_chromaticity_width = event->width;
        curr_chromaticity_height = event->height;
#endif
        
        GdkPixbuf * const orig_bg = gdk_pixbuf_new(
          GDK_COLORSPACE_RGB, TRUE, 8,
          curr_chromaticity_width,
          curr_chromaticity_height);
        
        Wcl::Colorlib::CieChromaticityDiagram::draw_to_buffer(
          gdk_pixbuf_get_pixels(orig_bg),
          Wcl::Colorlib::CieChromaticityDiagram::TYPE_CIE1931,
          curr_chromaticity_width, curr_chromaticity_height, gdk_pixbuf_get_rowstride(orig_bg));
        
        {
          // Newly-created GdkPixbuf structures start with
          // a reference count of one. So that I don't need
          // to add a reference count here.
          g_object_set_data(G_OBJECT(widget), "orig_bg", orig_bg);
        }
        
        {
          GdkPixmap * const curr_bg_wo_curr_color_space_sel =
            gdk_pixmap_new(widget->window, curr_chromaticity_width, curr_chromaticity_height, -1);
          
          gdk_draw_pixbuf(curr_bg_wo_curr_color_space_sel,
                          NULL,
                          orig_bg,
                          0, 0,
                          0, 0,
                          -1, -1,
                          GDK_RGB_DITHER_NONE,
                          0, 0);
          
          // Newly-created GdkPixmap structures start with
          // a reference count of one. So that I don't need
          // to add a reference count here.
          g_object_set_data(G_OBJECT(widget),
                            "curr_bg_wo_curr_color_space_sel",
                            curr_bg_wo_curr_color_space_sel);
        }
        
        {
          GdkPixmap * const curr_bg =
            gdk_pixmap_new(widget->window, curr_chromaticity_width, curr_chromaticity_height, -1);
          
          gdk_draw_pixbuf(curr_bg,
                          NULL,
                          orig_bg,
                          0, 0,
                          0, 0,
                          -1, -1,
                          GDK_RGB_DITHER_NONE,
                          0, 0);
          
          // Newly-created GdkPixmap structures start with
          // a reference count of one. So that I don't need
          // to add a reference count here.
          g_object_set_data(G_OBJECT(widget), "curr_bg", curr_bg);
        }
      }
    }
    
    return TRUE;
  }
  
  gboolean
  on_cie_chromaticity_diagram_drawing_area_expose_event(
    GtkWidget *widget,
    GdkEventExpose * /* event */,
    gpointer /* user_data */)
  {
    //.......................... delete bg when finalize?
    GdkPixmap * const curr_bg =
      GDK_PIXMAP(g_object_get_data(G_OBJECT(widget), "curr_bg"));
    
    gdk_draw_drawable(widget->window,
                      widget->style->fg_gc[widget->state],
                      curr_bg,
                      0, 0,
                      0, 0,
                      -1, -1);
    
    return FALSE;
  }
  
  gboolean
  on_spectrum_drawing_area_configure_event(
    GtkWidget *widget,
    GdkEventConfigure *event,
    gpointer /* user_data */)
  {
    if (0 == g_object_get_data(G_OBJECT(widget), "bg"))
    {
      uint32_t const real_width = event->width;
      uint32_t const real_height = event->height;
      
      GdkPixbuf *spectrum = gdk_pixbuf_new(
        GDK_COLORSPACE_RGB, TRUE, 8,
        real_width,
        real_height);
    
      Wcl::Colorlib::Spectrum::draw_to_buffer(
        gdk_pixbuf_get_pixels(spectrum),
        Wcl::Colorlib::Spectrum::equal_energy_spd_func,
        360, 830,
        real_width, real_height, gdk_pixbuf_get_rowstride(spectrum),
        255);
      
      GdkPixmap * const background_pixmap =
        gdk_pixmap_new(widget->window, real_width, real_height, -1);
    
      gdk_draw_pixbuf(background_pixmap,
                      NULL,
                      spectrum,
                      0, 0,
                      0, 0,
                      -1, -1,
                      GDK_RGB_DITHER_NONE,
                      0, 0);
      
      g_object_set_data(G_OBJECT(widget), "bg", background_pixmap);
    }
    
    return TRUE;
  }
  
  gboolean
  on_spectrum_drawing_area_expose_event(
    GtkWidget *widget,
    GdkEventExpose * /* event */,
    gpointer /* user_data */)
  {
    //.......................... delete bg when finalize?
    GdkPixmap * const background_pixmap =
      GDK_PIXMAP(g_object_get_data(G_OBJECT(widget), "bg"));
    
    gdk_draw_drawable(widget->window,
                      widget->style->fg_gc[widget->state],
                      background_pixmap,
                      0, 0,
                      0, 0,
                      -1, -1);
    
    return FALSE;
  }

  GtkTreeModel *
  create_rgb_color_space_model()
  {
    GtkTreeStore * const rgb_color_space_model = gtk_tree_store_new(1, G_TYPE_STRING);
    GtkTreeIter tree_iter;
    
    gtk_tree_store_append(GTK_TREE_STORE(rgb_color_space_model), &tree_iter, NULL);
    gtk_tree_store_set(GTK_TREE_STORE(rgb_color_space_model), &tree_iter,
                       0, "None",
                       -1);
    
    for (Wcl::Colorlib::ColorSpaceInfo::RGB::iterator iter = Wcl::Colorlib::ColorSpaceInfo::RGB::begin();
         iter != Wcl::Colorlib::ColorSpaceInfo::RGB::end();
         ++iter)
    {
      gtk_tree_store_append(GTK_TREE_STORE(rgb_color_space_model), &tree_iter, NULL);
      
      {
        char * const rgb_color_space_name = fmtstr_wcstombs((*iter).c_str(), (*iter).size());
        assert(rgb_color_space_name != 0);
        
        gtk_tree_store_set(GTK_TREE_STORE(rgb_color_space_model), &tree_iter,
                           0, rgb_color_space_name,
                           -1);
        
        fmtstr_delete(rgb_color_space_name);
      }
    }
    
    return GTK_TREE_MODEL(rgb_color_space_model);
  }
  
  void
  on_rgb_color_space_combo_changed(GtkComboBox *widget,
                                   gpointer     /* user_data */)
  {
    GtkTreeIter iter;
    
    if (TRUE == gtk_combo_box_get_active_iter(widget, &iter))
    {
      GtkTreeModel * const model = gtk_combo_box_get_model(widget);
      gchar *name;
      
      gtk_tree_model_get(model, &iter, 0, &name, -1);
      assert(name != 0);
      
      GdkPixmap * const curr_bg =
        GDK_PIXMAP(g_object_get_data(G_OBJECT(gp_cie_chromaticity_diagram),
                                     "curr_bg"));
      
      GdkPixmap * const curr_bg_wo_curr_color_space_sel =
        GDK_PIXMAP(g_object_get_data(G_OBJECT(gp_cie_chromaticity_diagram),
                                     "curr_bg_wo_curr_color_space_sel"));
      
      GdkGC * const gc = gdk_gc_new(curr_bg);
      
      // restore the curr_bg from
      // curr_bg_wo_curr_color_space_sel.
      gdk_draw_drawable(curr_bg,
                        gc,
                        curr_bg_wo_curr_color_space_sel,
                        0, 0,
                        0, 0,
                        -1, -1);
      
      if (strcmp(name, "None") != 0)
      {
        wchar_t * const name_wide = fmtstr_mbstowcs(name, 0);
        assert(name_wide != 0);
        
        double_t xr, yr, xg, yg, xb, yb, xw, yw;
        
        Wcl::Colorlib::ColorSpaceInfo::RGB::get_chromaticity_data(
          name_wide, xr, yr, xg, yg, xb, yb, xw, yw);
        
        fmtstr_delete(name_wide);
        
        {        
          GdkColor color;
          
          gtk_color_button_get_color(GTK_COLOR_BUTTON(gp_color_button), &color);
          gdk_gc_set_rgb_fg_color(gc, &color);
          
          gdk_draw_line(GDK_DRAWABLE(curr_bg),
                        gc,
                        boost::numeric_cast<gint>(xr * curr_chromaticity_width),
                        boost::numeric_cast<gint>((1 - yr) * curr_chromaticity_height),
                        boost::numeric_cast<gint>(xg * curr_chromaticity_width),
                        boost::numeric_cast<gint>((1 - yg) * curr_chromaticity_height));
        
          gdk_draw_line(GDK_DRAWABLE(curr_bg),
                        gc,
                        boost::numeric_cast<gint>(xg * curr_chromaticity_width),
                        boost::numeric_cast<gint>((1 - yg) * curr_chromaticity_height),
                        boost::numeric_cast<gint>(xb * curr_chromaticity_width),
                        boost::numeric_cast<gint>((1 - yb) * curr_chromaticity_height));
        
          gdk_draw_line(GDK_DRAWABLE(curr_bg),
                        gc,
                        boost::numeric_cast<gint>(xr * curr_chromaticity_width),
                        boost::numeric_cast<gint>((1 - yr) * curr_chromaticity_height),
                        boost::numeric_cast<gint>(xb * curr_chromaticity_width),
                        boost::numeric_cast<gint>((1 - yb) * curr_chromaticity_height));
        }
      }
      
      g_object_unref(gc);
      
      gdk_window_invalidate_rect(gp_cie_chromaticity_diagram->window, NULL, FALSE);
    }
  }
  
  void
  on_add_button_clicked(GtkButton * /* widget */,
                        gpointer  /* user_data */)
  {
    // Add current color space selection to listview.
    {
      GtkTreeIter iter;
      
      if (TRUE == gtk_combo_box_get_active_iter(GTK_COMBO_BOX(gp_rgb_color_space_combo), &iter))
      {
        GtkTreeModel * const model = gtk_combo_box_get_model(GTK_COMBO_BOX(gp_rgb_color_space_combo));
        gchar *name;
        
        gtk_tree_model_get(model, &iter, 0, &name, -1);
        assert(name != 0);
        
        if (strcmp(name, "None") != 0)
        {
          // Ensure the currently selected one is not
          // selected before.
          {
            GtkTreeIter tree_iter;
            gboolean success;
            gchar *list_name;
            
            success = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gp_sel_color_space_liststore),
                                                    &tree_iter);
            if (TRUE == success)
            {
              do
              {
                gtk_tree_model_get(GTK_TREE_MODEL(gp_sel_color_space_liststore),
                                   &tree_iter,
                                   SEL_COLOR_SPACE_TREEVIEW_COLUMN_NAME, &list_name,
                                   -1);
                assert(list_name != 0);
                
                if (0 == strcmp(list_name, name))
                {
                  // I have already added this color space,
                  // so that I don't need to add it again.
                  return;
                }
              } while (TRUE == gtk_tree_model_iter_next(GTK_TREE_MODEL(gp_sel_color_space_liststore),
                                                        &tree_iter));
            }
          }
          
          GtkTreeIter list_iter;
          
          GdkPixbuf * const pixbuf =
            gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8,
                           SEL_COLOR_SPACE_TREEVIEW_SMALL_ICON_WIDTH,
                           SEL_COLOR_SPACE_TREEVIEW_SMALL_ICON_HEIGHT);
          
          guchar *buffer = gdk_pixbuf_get_pixels(pixbuf);
          GdkColor color;
          
          gtk_color_button_get_color(GTK_COLOR_BUTTON(gp_color_button), &color);
          
          uint8_t const red_in_char = boost::numeric_cast<uint8_t>(color.red >> 8);
          uint8_t const green_in_char = boost::numeric_cast<uint8_t>(color.green >> 8);
          uint8_t const blue_in_char = boost::numeric_cast<uint8_t>(color.blue >> 8);
          
          for (uint32_t row = 0; row < SEL_COLOR_SPACE_TREEVIEW_SMALL_ICON_HEIGHT; ++row)
          {
            guchar * const line_start = buffer;
            
            for (uint32_t col = 0; col < SEL_COLOR_SPACE_TREEVIEW_SMALL_ICON_WIDTH; ++col)
            {
              (*(buffer + 0)) = red_in_char;
              (*(buffer + 1)) = green_in_char;
              (*(buffer + 2)) = blue_in_char;
              (*(buffer + 3)) = 0xFF;
              
              buffer += 4;
            }
            
            buffer = line_start + gdk_pixbuf_get_rowstride(pixbuf);
          }
          
          gtk_list_store_append(gp_sel_color_space_liststore, &list_iter);
          gtk_list_store_set(gp_sel_color_space_liststore, &list_iter,
                             SEL_COLOR_SPACE_TREEVIEW_COLUMN_PIXBUF, pixbuf,
                             SEL_COLOR_SPACE_TREEVIEW_COLUMN_RED, red_in_char,
                             SEL_COLOR_SPACE_TREEVIEW_COLUMN_GREEN, green_in_char,
                             SEL_COLOR_SPACE_TREEVIEW_COLUMN_BLUE, blue_in_char,
                             SEL_COLOR_SPACE_TREEVIEW_COLUMN_NAME, name,
                             SEL_COLOR_SPACE_TREEVIEW_COLUMN_ACTIVE, TRUE,
                             -1);
          
          // update 'curr_bg_wo_curr_color_space_sel' from 'curr_bg'.
          {
            GdkPixmap * const curr_bg =
              GDK_PIXMAP(g_object_get_data(G_OBJECT(gp_cie_chromaticity_diagram), "curr_bg"));
      
            GdkPixmap * const curr_bg_wo_curr_color_space_sel =
              GDK_PIXMAP(g_object_get_data(G_OBJECT(gp_cie_chromaticity_diagram),
                                           "curr_bg_wo_curr_color_space_sel"));
      
            GdkGC * const gc = gdk_gc_new(curr_bg);
      
            gdk_draw_drawable(curr_bg_wo_curr_color_space_sel,
                              gc,
                              curr_bg,
                              0, 0,
                              0, 0,
                              -1, -1);
            
            g_object_unref(gc);
            
            gdk_window_invalidate_rect(gp_cie_chromaticity_diagram->window, NULL, FALSE);
          }
        }
        
        // reset the rgb color space combo box to "none" entry.
        gtk_combo_box_set_active(GTK_COMBO_BOX(gp_rgb_color_space_combo),
                                 0);
      }
    }    
  }
  
  gboolean
  on_treeview_clear_all(GtkTreeModel *model,
                        GtkTreePath  * /* path */,
                        GtkTreeIter  *iter,
                        gpointer      /* user_data */)
  {
    GdkPixbuf *pixbuf;
    
    gtk_tree_model_get(model, iter, SEL_COLOR_SPACE_TREEVIEW_COLUMN_PIXBUF, &pixbuf, -1);
    
    g_object_unref(pixbuf);
    
    return FALSE;
  }

  void
  on_clear_all_button_clicked(GtkButton * /* widget */,
                              gpointer  /* user_data */)
  {
    gtk_combo_box_set_active(GTK_COMBO_BOX(gp_rgb_color_space_combo), 0);
    
    gtk_tree_model_foreach(GTK_TREE_MODEL(gp_sel_color_space_liststore),
                           on_treeview_clear_all,
                           0);
    
    gtk_list_store_clear(gp_sel_color_space_liststore);
    
    // Clear the chormaticity diagram to the original one.
    {
      GdkPixbuf * const orig_bg =
        GDK_PIXBUF(g_object_get_data(G_OBJECT(gp_cie_chromaticity_diagram), "orig_bg"));
      
      GdkPixmap * const curr_bg =
        GDK_PIXMAP(g_object_get_data(G_OBJECT(gp_cie_chromaticity_diagram), "curr_bg"));
      
      GdkPixmap * const curr_bg_wo_curr_color_space_sel =
        GDK_PIXMAP(g_object_get_data(G_OBJECT(gp_cie_chromaticity_diagram),
                                     "curr_bg_wo_curr_color_space_sel"));
      
      gdk_draw_pixbuf(curr_bg,
                      NULL,
                      orig_bg,
                      0, 0,
                      0, 0,
                      -1, -1,
                      GDK_RGB_DITHER_NONE,
                      0, 0);
      
      gdk_draw_pixbuf(curr_bg_wo_curr_color_space_sel,
                      NULL,
                      orig_bg,
                      0, 0,
                      0, 0,
                      -1, -1,
                      GDK_RGB_DITHER_NONE,
                      0, 0);
      
      gdk_window_invalidate_rect(gp_cie_chromaticity_diagram->window, NULL, FALSE);
    }
  }
  
  void
  on_color_button_color_set(GtkColorButton * /* widget */,
                            gpointer        /* user_data */)
  {
    // If I select a color space now, than I will redraw the
    // triangle of this color space using the newly selected
    // color.
    GtkTreeIter iter;
    
    if (TRUE == gtk_combo_box_get_active_iter(GTK_COMBO_BOX(gp_rgb_color_space_combo), &iter))
    {
      GtkTreeModel * const model = gtk_combo_box_get_model(GTK_COMBO_BOX(gp_rgb_color_space_combo));
      gchar *name;
      
      gtk_tree_model_get(model, &iter, 0, &name, -1);
      assert(name != 0);
      
      GdkPixmap * const curr_bg =
        GDK_PIXMAP(g_object_get_data(G_OBJECT(gp_cie_chromaticity_diagram),
                                     "curr_bg"));
      
      GdkPixmap * const curr_bg_wo_curr_color_space_sel =
        GDK_PIXMAP(g_object_get_data(G_OBJECT(gp_cie_chromaticity_diagram),
                                     "curr_bg_wo_curr_color_space_sel"));
      
      GdkGC * const gc = gdk_gc_new(curr_bg);
      
      // restore the curr_bg from
      // curr_bg_wo_curr_color_space_sel.
      gdk_draw_drawable(curr_bg,
                        gc,
                        curr_bg_wo_curr_color_space_sel,
                        0, 0,
                        0, 0,
                        -1, -1);
      
      if (strcmp(name, "None") != 0)
      {
        wchar_t * const name_wide = fmtstr_mbstowcs(name, 0);
        assert(name_wide != 0);
        
        double_t xr, yr, xg, yg, xb, yb, xw, yw;
        
        Wcl::Colorlib::ColorSpaceInfo::RGB::get_chromaticity_data(
          name_wide, xr, yr, xg, yg, xb, yb, xw, yw);
        
        fmtstr_delete(name_wide);
        
        {        
          GdkColor color;
        
          gtk_color_button_get_color(GTK_COLOR_BUTTON(gp_color_button), &color);
          gdk_gc_set_rgb_fg_color(gc, &color);
        
          gdk_draw_line(GDK_DRAWABLE(curr_bg),
                        gc,
                        boost::numeric_cast<gint>(xr * curr_chromaticity_width),
                        boost::numeric_cast<gint>((1 - yr) * curr_chromaticity_height),
                        boost::numeric_cast<gint>(xg * curr_chromaticity_width),
                        boost::numeric_cast<gint>((1 - yg) * curr_chromaticity_height));
        
          gdk_draw_line(GDK_DRAWABLE(curr_bg),
                        gc,
                        boost::numeric_cast<gint>(xg * curr_chromaticity_width),
                        boost::numeric_cast<gint>((1 - yg) * curr_chromaticity_height),
                        boost::numeric_cast<gint>(xb * curr_chromaticity_width),
                        boost::numeric_cast<gint>((1 - yb) * curr_chromaticity_height));
        
          gdk_draw_line(GDK_DRAWABLE(curr_bg),
                        gc,
                        boost::numeric_cast<gint>(xr * curr_chromaticity_width),
                        boost::numeric_cast<gint>((1 - yr) * curr_chromaticity_height),
                        boost::numeric_cast<gint>(xb * curr_chromaticity_width),
                        boost::numeric_cast<gint>((1 - yb) * curr_chromaticity_height));
        }
      }
      
      g_object_unref(gc);
      
      gdk_window_invalidate_rect(gp_cie_chromaticity_diagram->window, NULL, FALSE);
    }
  }
  
  gboolean
  on_treeview_toggled(GtkTreeModel *model,
                      GtkTreePath  * /* path */,
                      GtkTreeIter  *iter,
                      gpointer      /* user_data */)
  {
    gboolean active;
    
    gtk_tree_model_get(model, iter, SEL_COLOR_SPACE_TREEVIEW_COLUMN_ACTIVE, &active, -1);
    
    if (TRUE == active)
    {
      gchar *name;
      double_t xr, yr, xg, yg, xb, yb, xw, yw;
      
      gtk_tree_model_get(model, iter, SEL_COLOR_SPACE_TREEVIEW_COLUMN_NAME, &name, -1);
      
      {
        wchar_t * const name_wide = fmtstr_mbstowcs(name, 0);
        assert(name_wide != 0);
        
        Wcl::Colorlib::ColorSpaceInfo::RGB::get_chromaticity_data(
          name_wide, xr, yr, xg, yg, xb, yb, xw, yw);
        
        fmtstr_delete(name_wide);
      }
      
      uint8_t r, g, b;
      
      gtk_tree_model_get(model, iter, SEL_COLOR_SPACE_TREEVIEW_COLUMN_RED, &r, -1);
      gtk_tree_model_get(model, iter, SEL_COLOR_SPACE_TREEVIEW_COLUMN_GREEN, &g, -1);
      gtk_tree_model_get(model, iter, SEL_COLOR_SPACE_TREEVIEW_COLUMN_BLUE, &b, -1);
      
      GdkColor color;
      color.red = (r << 8);
      color.green = (g << 8);
      color.blue = (b << 8);
      
      GdkPixmap * const curr_bg_wo_curr_color_space_sel =
        GDK_PIXMAP(g_object_get_data(G_OBJECT(gp_cie_chromaticity_diagram),
                                     "curr_bg_wo_curr_color_space_sel"));
      
      GdkGC * const gc = gdk_gc_new(curr_bg_wo_curr_color_space_sel);
      gdk_gc_set_rgb_fg_color(gc, &color);
      
      gdk_draw_line(GDK_DRAWABLE(curr_bg_wo_curr_color_space_sel),
                    gc,
                    boost::numeric_cast<gint>(xr * curr_chromaticity_width),
                    boost::numeric_cast<gint>((1 - yr) * curr_chromaticity_height),
                    boost::numeric_cast<gint>(xg * curr_chromaticity_width),
                    boost::numeric_cast<gint>((1 - yg) * curr_chromaticity_height));
      
      gdk_draw_line(GDK_DRAWABLE(curr_bg_wo_curr_color_space_sel),
                    gc,
                    boost::numeric_cast<gint>(xg * curr_chromaticity_width),
                    boost::numeric_cast<gint>((1 - yg) * curr_chromaticity_height),
                    boost::numeric_cast<gint>(xb * curr_chromaticity_width),
                    boost::numeric_cast<gint>((1 - yb) * curr_chromaticity_height));
      
      gdk_draw_line(GDK_DRAWABLE(curr_bg_wo_curr_color_space_sel),
                    gc,
                    boost::numeric_cast<gint>(xr * curr_chromaticity_width),
                    boost::numeric_cast<gint>((1 - yr) * curr_chromaticity_height),
                    boost::numeric_cast<gint>(xb * curr_chromaticity_width),
                    boost::numeric_cast<gint>((1 - yb) * curr_chromaticity_height));
      
      g_object_unref(gc);
    }
    
    return FALSE;
  }
  
  void
  on_sel_color_space_treeview_toggled(GtkCellRendererToggle *cell_renderer,
                                      gchar                 *path,
                                      gpointer               /* user_data */)
  {
    GtkTreeIter iter;
    GtkTreePath * const tree_path = gtk_tree_path_new_from_string(path);
    
    gboolean const success = 
      gtk_tree_model_get_iter(GTK_TREE_MODEL(gp_sel_color_space_liststore),
                              &iter,
                              tree_path);
    assert(TRUE == success);
    
    gtk_tree_path_free(tree_path);
    
    gtk_list_store_set(gp_sel_color_space_liststore,
                       &iter,
                       SEL_COLOR_SPACE_TREEVIEW_COLUMN_ACTIVE,
                       !gtk_cell_renderer_toggle_get_active(cell_renderer),
                       -1);
    
    // Restore curr_bg_wo_curr_color_space_sel from
    // orig_bg.
    {
      GdkPixbuf * const orig_bg =
        GDK_PIXBUF(g_object_get_data(G_OBJECT(gp_cie_chromaticity_diagram), "orig_bg"));
      
      GdkPixmap * const curr_bg_wo_curr_color_space_sel =
        GDK_PIXMAP(g_object_get_data(G_OBJECT(gp_cie_chromaticity_diagram),
                                     "curr_bg_wo_curr_color_space_sel"));
      
      gdk_draw_pixbuf(curr_bg_wo_curr_color_space_sel,
                      NULL,
                      orig_bg,
                      0, 0,
                      0, 0,
                      -1, -1,
                      GDK_RGB_DITHER_NONE,
                      0, 0);
    }
    
    // Draw triangles for each selected color space in the
    // list store.
    {
      gtk_tree_model_foreach(GTK_TREE_MODEL(gp_sel_color_space_liststore),
                             on_treeview_toggled,
                             0);
    }
    
    // Draw triangle for the current selected color space in
    // color space combo.
    {
      on_rgb_color_space_combo_changed(GTK_COMBO_BOX(gp_rgb_color_space_combo), 0);
    }
    
    gdk_window_invalidate_rect(gp_cie_chromaticity_diagram->window, NULL, FALSE);
  }
}

GtkWidget *
create_cie_page()
{
  GtkWidget * const hbox = gtk_hbox_new(FALSE, 0);
  GtkWidget * const vbox = gtk_vbox_new(FALSE, 0);
  
  gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);
  
  GtkWidget * const chromaticity_label = gtk_label_new("CIE Chromaticity Diagram");
  gtk_box_pack_start(GTK_BOX(vbox), chromaticity_label, FALSE, FALSE, 0);
  
  GtkWidget * const table = gtk_table_new(2, 2, FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
  
  {
    gp_cie_chromaticity_diagram = gtk_drawing_area_new();
    gtk_table_attach(GTK_TABLE(table),
                     gp_cie_chromaticity_diagram,
                     1, 2,
                     0, 1,
                     static_cast<GtkAttachOptions>(GTK_EXPAND | GTK_FILL),
                     static_cast<GtkAttachOptions>(GTK_EXPAND | GTK_FILL),
                     0, 0);
    
    gtk_widget_set_events(gp_cie_chromaticity_diagram,
                          GDK_POINTER_MOTION_MASK |
                          GDK_POINTER_MOTION_HINT_MASK);
  }
  
  {
    GtkWidget * const v_ruler = gtk_vruler_new();
    gtk_table_attach(GTK_TABLE(table),
                     v_ruler,
                     0, 1,
                     0, 1,
                     static_cast<GtkAttachOptions>(GTK_SHRINK | GTK_FILL),
                     static_cast<GtkAttachOptions>(GTK_SHRINK | GTK_FILL),
                     0, 0);
    
    gtk_ruler_set_metric(GTK_RULER(v_ruler), GTK_PIXELS);
    gtk_ruler_set_range(GTK_RULER(v_ruler),
                        1.0,
                        0.0,
                        0.0,
                        1.0);
    
    g_signal_connect_swapped(G_OBJECT(gp_cie_chromaticity_diagram),
                             "motion_notify_event",
                             G_CALLBACK(GTK_WIDGET_GET_CLASS(v_ruler)->motion_notify_event),
                             G_OBJECT(v_ruler));
  }
  
  {
    GtkWidget * const h_ruler = gtk_hruler_new();
    gtk_table_attach(GTK_TABLE(table),
                     h_ruler,
                     1, 2,
                     1, 2,
                     static_cast<GtkAttachOptions>(GTK_SHRINK | GTK_FILL),
                     static_cast<GtkAttachOptions>(GTK_SHRINK | GTK_FILL),
                     0, 0);
    
    gtk_ruler_set_metric(GTK_RULER(h_ruler), GTK_PIXELS);
    gtk_ruler_set_range(GTK_RULER(h_ruler),
                        0.0,
                        1.0,
                        0.0,
                        1.0);
    
    g_signal_connect_swapped(G_OBJECT(gp_cie_chromaticity_diagram),
                             "motion_notify_event",
                             G_CALLBACK(GTK_WIDGET_GET_CLASS(h_ruler)->motion_notify_event),
                             G_OBJECT(h_ruler));
  }
  
  {
    gtk_widget_add_events(gp_cie_chromaticity_diagram, GDK_EXPOSURE_MASK);
    
#if defined(FIX_BG_SIZE)
    gtk_widget_set_size_request(gp_cie_chromaticity_diagram,
                                CHROMATICITY_DIAGRAM_FIX_WIDTH,
                                CHROMATICITY_DIAGRAM_FIX_HEIGHT);
#endif
    
    g_signal_connect(gp_cie_chromaticity_diagram, "configure-event",
                     G_CALLBACK(on_cie_chromaticity_diagram_drawing_area_configure_event),
                     NULL);
    
    g_signal_connect(gp_cie_chromaticity_diagram, "expose-event",
                     G_CALLBACK(on_cie_chromaticity_diagram_drawing_area_expose_event),
                     NULL);
  }
  
  GtkWidget * const spectrum_label = gtk_label_new("Spectrum");
  gtk_box_pack_start(GTK_BOX(vbox), spectrum_label, FALSE, FALSE, 0);
  
  {
    GtkWidget * const spectrum = gtk_drawing_area_new();
    gtk_box_pack_start(GTK_BOX(vbox), spectrum, TRUE, TRUE, 0);
    
    gtk_widget_add_events(spectrum, GDK_EXPOSURE_MASK);
    
    g_signal_connect(spectrum, "configure-event",
                     G_CALLBACK(on_spectrum_drawing_area_configure_event), 0);
    
    g_signal_connect(spectrum, "expose-event",
                     G_CALLBACK(on_spectrum_drawing_area_expose_event), 0);
  }
  
  {
    GtkWidget * const rgb_color_space_frame = gtk_frame_new("RGB color space");
    gtk_box_pack_start(GTK_BOX(hbox), rgb_color_space_frame, TRUE, TRUE, 0);
    
    GtkWidget * const rgb_color_space_frame_vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(rgb_color_space_frame), rgb_color_space_frame_vbox);
    
    {
      GtkTreeModel * const rgb_color_space_model = create_rgb_color_space_model();
      gp_rgb_color_space_combo = gtk_combo_box_new_with_model(rgb_color_space_model);
      g_object_unref(rgb_color_space_model);
      gtk_box_pack_start(GTK_BOX(rgb_color_space_frame_vbox), gp_rgb_color_space_combo, FALSE, FALSE, 0);
      
      gtk_combo_box_set_active(GTK_COMBO_BOX(gp_rgb_color_space_combo), 0);
      
      GtkCellRenderer * const renderer = gtk_cell_renderer_text_new();
      
      gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(gp_rgb_color_space_combo),
                                 renderer,
                                 TRUE);
      gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(gp_rgb_color_space_combo),
                                     renderer,
                                     "text", 0,
                                     NULL);
      
      g_signal_connect(G_OBJECT(gp_rgb_color_space_combo), "changed",
                       G_CALLBACK(on_rgb_color_space_combo_changed),
                       NULL);
    }
    
    {
      GtkWidget * const curr_color_hbox = gtk_hbox_new(FALSE, 0);
      gtk_box_pack_start(GTK_BOX(rgb_color_space_frame_vbox), curr_color_hbox, FALSE, FALSE, 0);
      
      GtkWidget * const curr_color_label = gtk_label_new("Current Color: ");
      gtk_box_pack_start(GTK_BOX(curr_color_hbox), curr_color_label, FALSE, FALSE, 0);
      
      gp_color_button = gtk_color_button_new();
      gtk_box_pack_start(GTK_BOX(curr_color_hbox), gp_color_button, TRUE, TRUE, 0);
      
      g_signal_connect(G_OBJECT(gp_color_button), "color-set",
                       G_CALLBACK(on_color_button_color_set),
                       NULL);
    }
    
    GtkWidget * const add_button = gtk_button_new_with_label("Add");
    gtk_box_pack_start(GTK_BOX(rgb_color_space_frame_vbox), add_button, FALSE, FALSE, 0);
    
    g_signal_connect(G_OBJECT(add_button), "clicked",
                     G_CALLBACK(on_add_button_clicked),
                     NULL);
    
    GtkWidget * const clear_all_button = gtk_button_new_with_label("Clear All");
    gtk_box_pack_start(GTK_BOX(rgb_color_space_frame_vbox), clear_all_button, FALSE, FALSE, 0);
    
    g_signal_connect(G_OBJECT(clear_all_button), "clicked",
                     G_CALLBACK(on_clear_all_button_clicked),
                     NULL);
    
    {
      gp_sel_color_space_liststore =
        gtk_list_store_new(SEL_COLOR_SPACE_TREEVIEW_COLUMN_SIZE,
                           GDK_TYPE_PIXBUF,
                           G_TYPE_UCHAR, /* Red */
                           G_TYPE_UCHAR, /* Green */
                           G_TYPE_UCHAR, /* Blue */
                           G_TYPE_STRING,
                           G_TYPE_BOOLEAN);
      
      GtkWidget * const scrolled_window = gtk_scrolled_window_new(NULL, NULL);
      gtk_box_pack_start(GTK_BOX(rgb_color_space_frame_vbox),
                         scrolled_window,
                         TRUE, TRUE, 0);  
      
      gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                     GTK_POLICY_AUTOMATIC,
                                     GTK_POLICY_AUTOMATIC);
      
      GtkWidget * const sel_color_space_treeview = gtk_tree_view_new();
      gtk_container_add(GTK_CONTAINER(scrolled_window), sel_color_space_treeview);
      
      gtk_tree_view_set_model(GTK_TREE_VIEW(sel_color_space_treeview),
                              GTK_TREE_MODEL(gp_sel_color_space_liststore));
      
      gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(sel_color_space_treeview),
                                        TRUE);
      
      GtkCellRenderer *renderer;
      GtkTreeViewColumn *column;
      
      {
        renderer = gtk_cell_renderer_pixbuf_new();
        column = gtk_tree_view_column_new_with_attributes("Displayed color",
                                                          renderer,
                                                          "pixbuf",
                                                          SEL_COLOR_SPACE_TREEVIEW_COLUMN_PIXBUF,
                                                          NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(sel_color_space_treeview), column);
      }
      
      {
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes("Name",
                                                          renderer,
                                                          "text",
                                                          SEL_COLOR_SPACE_TREEVIEW_COLUMN_NAME,
                                                          NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(sel_color_space_treeview), column);
      }
      
      {
        renderer = gtk_cell_renderer_toggle_new();
        column = gtk_tree_view_column_new_with_attributes("Show",
                                                          renderer,
                                                          "active",
                                                          SEL_COLOR_SPACE_TREEVIEW_COLUMN_ACTIVE,
                                                          NULL);
        gtk_cell_renderer_toggle_set_radio(GTK_CELL_RENDERER_TOGGLE(renderer), FALSE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(sel_color_space_treeview), column);
        
        g_signal_connect(renderer, "toggled",
                         G_CALLBACK(on_sel_color_space_treeview_toggled),
                         NULL);
      }
    }
  }
  
  return hbox;
}
