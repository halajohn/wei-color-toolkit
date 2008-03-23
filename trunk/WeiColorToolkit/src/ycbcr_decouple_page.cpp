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

namespace
{
  uint32_t g_src_file_orig_width;
  uint32_t g_src_file_orig_height;
  
  Wcl::Colorlib::ColorSpaceConverter *gp_rgb2ycbcr_color_space_converter;
  Wcl::Colorlib::ColorSpaceConverter *gp_ycbcr2rgb_color_space_converter;
  
  struct area_info_t
  {
    uint32_t m_area_width;
    uint32_t m_area_height;
    
    uint32_t m_scaled_width;
    uint32_t m_scaled_height;
    
    GtkWidget *mp_widget;
    std::vector<uint8_t> m_rgba_buffer;
    
    GdkPixbuf *mp_orig_pixbuf;
    GdkPixbuf *mp_scaled_pixbuf;
    
    double_t m_curr_progress_threshold;
    GtkProgressBar *mp_progress_bar;
    
    area_info_t()
      : mp_widget(0),
        mp_orig_pixbuf(0),
        mp_scaled_pixbuf(0),
        m_curr_progress_threshold(0),
        mp_progress_bar(0)
    {}
    
    static bool
    conversion_percentage_cb(void *user_data,
                             double_t const percentage)
    {
      area_info_t * const area_info = static_cast<area_info_t *>(user_data);
      assert(area_info != 0);
      
      if (percentage > area_info->m_curr_progress_threshold)
      {
        gtk_progress_bar_set_fraction(area_info->mp_progress_bar, percentage);
        
        area_info->m_curr_progress_threshold += 0.1;
      }
      
      while (TRUE == gtk_events_pending())
      {
        gtk_main_iteration();
      }
      
      return true;
    }
    
    static void
    stage_begin(area_info_t * const area_info,
                gchar const * const title)
    {
      gtk_progress_bar_set_text(area_info->mp_progress_bar, title);
      gtk_progress_bar_set_fraction(area_info->mp_progress_bar, 0.0);
      area_info->m_curr_progress_threshold = 0;
      
      while (TRUE == gtk_events_pending())
      {
        gtk_main_iteration();
      }
    }

    static void
    stage_end(area_info_t * const area_info)
    {
      gtk_progress_bar_set_fraction(area_info->mp_progress_bar, 1.0);
      
      while (TRUE == gtk_events_pending())
      {
        gtk_main_iteration();
      }
    }
    
    static void
    reset_content(area_info_t * const area_info)
    {
      assert(area_info != 0);
      
      if (area_info->mp_orig_pixbuf != 0)
      {
        if (area_info->mp_scaled_pixbuf != 0)
        {
          g_object_unref(area_info->mp_scaled_pixbuf);
          area_info->mp_scaled_pixbuf = 0;
        }
        
        g_object_unref(area_info->mp_orig_pixbuf);
        area_info->mp_orig_pixbuf = 0;
      }
      
      area_info->m_rgba_buffer.clear();
      
      area_info->m_curr_progress_threshold = 0.0;
      
      gtk_progress_bar_set_text(area_info->mp_progress_bar, "");
      gtk_progress_bar_set_fraction(area_info->mp_progress_bar, 0.0);
      
      gdk_window_invalidate_rect(area_info->mp_widget->window, NULL, FALSE);
    }
    
    static void
    create_scaled_pixbuf(area_info_t * const area_info)
    {
      assert(area_info != 0);
    
      double_t const width_ratio = (area_info->m_area_width /
                                    boost::numeric_cast<double_t>(g_src_file_orig_width));
      double_t const height_ratio = (area_info->m_area_height /
                                     boost::numeric_cast<double_t>(g_src_file_orig_height));
    
      double_t const real_ratio = (width_ratio >= height_ratio) ? height_ratio : width_ratio;
    
      uint32_t const scaled_width = boost::numeric_cast<uint32_t>(g_src_file_orig_width * real_ratio);
      uint32_t const scaled_height = boost::numeric_cast<uint32_t>(g_src_file_orig_height * real_ratio);
    
      area_info->m_scaled_width =
        (scaled_width > area_info->m_area_width)
        ? area_info->m_area_width
        : scaled_width;
    
      area_info->m_scaled_height =
        (scaled_height > area_info->m_area_height)
        ? area_info->m_area_height
        : scaled_height;
    
      if (area_info->mp_scaled_pixbuf != 0)
      {
        g_object_unref(area_info->mp_scaled_pixbuf);
        area_info->mp_scaled_pixbuf = 0;
      }
    
      area_info->mp_scaled_pixbuf =
        gdk_pixbuf_scale_simple(area_info->mp_orig_pixbuf,
                                area_info->m_scaled_width,
                                area_info->m_scaled_height,
                                GDK_INTERP_BILINEAR);
      assert(area_info->mp_scaled_pixbuf != 0);
    
      gdk_window_invalidate_rect(area_info->mp_widget->window, NULL, FALSE);
    }

    static void
    add_content(area_info_t * const area_info,
                std::vector<Wcl::Colorlib::ColorSpaceBasicUnitValue> &yuv_buffer)
    {
      // Convert Y,U,V buffer back to RGB buffer.
      std::vector<Wcl::Colorlib::ColorSpaceBasicUnitValue> rgb_buffer;
      {
        area_info_t::stage_begin(area_info, "Convert YUV444 back to RGB");
        
        {
          gp_ycbcr2rgb_color_space_converter->register_percentage_cb(
            area_info_t::conversion_percentage_cb,
            area_info);
          
          gp_ycbcr2rgb_color_space_converter->convert_buffer(yuv_buffer, rgb_buffer);
          yuv_buffer.clear();
        }
        
        area_info_t::stage_end(area_info);
      }
      
      // The data type of the generated rgb_buffer is
      // double_t, I need to convert it to uint8_t and add
      // alpha channel to fit into GdkPixbuf.
      {
        area_info_t::stage_begin(area_info, "Convert RGB to fit into GdkPixbuf");
        
        {
          uint32_t i = 0;
          
          assert(0 == area_info->m_rgba_buffer.size());
          
          BOOST_FOREACH(Wcl::Colorlib::ColorSpaceBasicUnitValue const &value, rgb_buffer)
          {
            area_info->m_rgba_buffer.push_back(Wcl::Colorlib::clamp_for_rgb(value.get_value<double_t>()));
            ++i;
            
            if (0 == (i % 3))
            {
              area_info->m_rgba_buffer.push_back(static_cast<uint8_t>(0xFF));
            }
            
            {
              double_t const percentage = (static_cast<double_t>(i) / rgb_buffer.size());
              
              (void)area_info_t::conversion_percentage_cb(area_info, percentage);
            }
          }
          rgb_buffer.clear();
        }
        
        area_info_t::stage_end(area_info);
      }
      
      assert(area_info->m_rgba_buffer.size() == g_src_file_orig_width * g_src_file_orig_height * 4);
      
      // Create orig pixbuf.
      {
        area_info_t::stage_begin(area_info, "Create original GdkPixbuf");
        
        {
          assert(0 == area_info->mp_orig_pixbuf);
          
          area_info->mp_orig_pixbuf = gdk_pixbuf_new_from_data(
            &(area_info->m_rgba_buffer[0]),
            GDK_COLORSPACE_RGB,
            TRUE,
            8,
            g_src_file_orig_width,
            g_src_file_orig_height,
            g_src_file_orig_width * 4,
            0,
            0);
        }
        
        area_info_t::stage_end(area_info);
      }
      
      // Create scaled pixbuf.
      {
        area_info_t::stage_begin(area_info, "Create scaled GdkPixbuf");
        
        area_info_t::create_scaled_pixbuf(area_info);
        
        area_info_t::stage_end(area_info);
      }
    }
  };
  typedef struct area_info_t area_info_t;
  
  area_info_t g_preview_area_info;
  area_info_t g_y_area_info;
  area_info_t g_cb_area_info;
  area_info_t g_cr_area_info;
    
  gboolean
  on_area_configure_event(GtkWidget         * /* widget */,
                          GdkEventConfigure *event,
                          gpointer           user_data)
  {
    area_info_t * const area_info = static_cast<area_info_t *>(user_data);
    assert(area_info != 0);
    
    area_info->m_area_width = event->width;
    area_info->m_area_height = event->height;
    
    if (area_info->mp_orig_pixbuf != 0)
    {
      area_info_t::create_scaled_pixbuf(area_info);
    }
    
    return FALSE;
  }
  
  gboolean
  on_area_expose_event(GtkWidget      * /* widget */,
                       GdkEventExpose *event,
                       gpointer        user_data)
  {
    area_info_t * const area_info = static_cast<area_info_t *>(user_data);
    assert(area_info != 0);
    
    if (area_info->mp_scaled_pixbuf != 0)
    {
      gdk_draw_pixbuf(event->window,
                      NULL,
                      area_info->mp_scaled_pixbuf,
                      0, 0,
                      (area_info->m_area_width - area_info->m_scaled_width) / 2,
                      (area_info->m_area_height - area_info->m_scaled_height) / 2,
                      area_info->m_scaled_width,
                      area_info->m_scaled_height,
                      GDK_RGB_DITHER_NORMAL,
                      0, 0);
    }
    
    return FALSE;
  }
  
  void
  on_select_src_file_set(GtkFileChooserButton *filechooserbutton,
                         gpointer /* user_data */)
  {
    gchar * const filename =
      gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(filechooserbutton));
    
    if (filename != NULL)
    {
      // Clear the old content.
      {
        area_info_t::reset_content(&g_preview_area_info);
        area_info_t::reset_content(&g_y_area_info);
        area_info_t::reset_content(&g_cb_area_info);
        area_info_t::reset_content(&g_cr_area_info);
      }
      
      // Create the original pixbuf from file.
      {
        area_info_t::stage_begin(&g_preview_area_info, "Create image from source file");
        
        {
          GError *error = 0;
          g_preview_area_info.mp_orig_pixbuf = gdk_pixbuf_new_from_file(filename, &error);
          
          if (0 == g_preview_area_info.mp_orig_pixbuf)
          {
            return;
          }
        }
        
        g_src_file_orig_width = gdk_pixbuf_get_width(g_preview_area_info.mp_orig_pixbuf);
        g_src_file_orig_height = gdk_pixbuf_get_height(g_preview_area_info.mp_orig_pixbuf);
        
        area_info_t::stage_end(&g_preview_area_info);
      }
      
      // Create the first scaled pixbuf.
      {
        area_info_t::stage_begin(&g_preview_area_info, "Create scaled image from source file");
        
        area_info_t::create_scaled_pixbuf(&g_preview_area_info);
        
        area_info_t::stage_end(&g_preview_area_info);
      }
      
      // Because GdkPixbuf uses RGBA and uint8_t as its data
      // type, I have to remove the alpha channal and
      // convert uint8_t to double_t. Hence,
      // Wcl::Colorlib::ColorSpaceConverter can use it.
      std::vector<Wcl::Colorlib::ColorSpaceBasicUnitValue> rgb_buffer;
      {
        area_info_t::stage_begin(&g_preview_area_info, "Create source RGB buffer");
        
        {
          rgb_buffer.reserve(g_src_file_orig_width * g_src_file_orig_height * 3);
          
          {
            guchar *pixel_buffer = gdk_pixbuf_get_pixels(g_preview_area_info.mp_orig_pixbuf);
            uint32_t progress = 0;
            
            for (uint32_t i = 0; i < g_src_file_orig_height; ++i)
            {
              guchar *curr_line_start = pixel_buffer;
              
              for (uint32_t j = 0; j < g_src_file_orig_width; ++j)
              {
                rgb_buffer.push_back(Wcl::Colorlib::clamp_for_rgb(*pixel_buffer));
                
                ++pixel_buffer;
                rgb_buffer.push_back(Wcl::Colorlib::clamp_for_rgb(*pixel_buffer));
                
                ++pixel_buffer;
                rgb_buffer.push_back(Wcl::Colorlib::clamp_for_rgb(*pixel_buffer));
                
                // Skip the alpha channel.
                ++pixel_buffer;
                ++progress;
                
                {
                  double_t const percentage = (static_cast<double_t>(progress) /
                                               (g_src_file_orig_width * g_src_file_orig_height));
                  
                  (void)area_info_t::conversion_percentage_cb(&g_preview_area_info, percentage);
                }
              }
              
              pixel_buffer = curr_line_start + gdk_pixbuf_get_rowstride(g_preview_area_info.mp_orig_pixbuf);
            }
          }
        }
        
        area_info_t::stage_end(&g_preview_area_info);
      }
      
      // create source YUV444 buffer.
      std::vector<Wcl::Colorlib::ColorSpaceBasicUnitValue> ycbcr_buffer;
      {
        area_info_t::stage_begin(&g_preview_area_info, "Create source YUV444 buffer");
        
        {
          gp_rgb2ycbcr_color_space_converter->register_percentage_cb(
            area_info_t::conversion_percentage_cb,
            &g_preview_area_info);
          
          gp_rgb2ycbcr_color_space_converter->convert_buffer(rgb_buffer, ycbcr_buffer);
          rgb_buffer.clear();
        }
        
        area_info_t::stage_end(&g_preview_area_info);
      }
      
      assert(ycbcr_buffer.size() == g_src_file_orig_width * g_src_file_orig_height * 3);
      
      // create 3 separate Y,U,V buffer.
      std::vector<Wcl::Colorlib::ColorSpaceBasicUnitValue> ycbcr_y_buffer;
      std::vector<Wcl::Colorlib::ColorSpaceBasicUnitValue> ycbcr_cb_buffer;
      std::vector<Wcl::Colorlib::ColorSpaceBasicUnitValue> ycbcr_cr_buffer;
      {
        area_info_t::stage_begin(&g_preview_area_info, "Create Y,U,V separated buffer");
        
        {
          ycbcr_y_buffer.reserve(g_src_file_orig_width * g_src_file_orig_height * 3);
          ycbcr_cb_buffer.reserve(g_src_file_orig_width * g_src_file_orig_height * 3);
          ycbcr_cr_buffer.reserve(g_src_file_orig_width * g_src_file_orig_height * 3);
          
          uint8_t cb_middle_value;
          uint8_t cr_middle_value;
          
          {
            std::vector<std::pair<Wcl::Colorlib::ColorSpaceBasicUnitValue,
              Wcl::Colorlib::ColorSpaceBasicUnitValue> > ranges;
            
            gp_rgb2ycbcr_color_space_converter->get_dst_plugin()->get_input_data_range(1, ranges);
            cb_middle_value = boost::numeric_cast<uint8_t>(
              (static_cast<uint32_t>(ranges[0].first.get_value<uint8_t>()) + 
               static_cast<uint32_t>(ranges[0].second.get_value<uint8_t>())) / 2);
            
            gp_rgb2ycbcr_color_space_converter->get_dst_plugin()->get_input_data_range(2, ranges);            
            cr_middle_value = boost::numeric_cast<uint8_t>(
              (static_cast<uint32_t>(ranges[0].first.get_value<uint8_t>()) + 
               static_cast<uint32_t>(ranges[0].second.get_value<uint8_t>())) / 2);
          }
          
          for (std::vector<Wcl::Colorlib::ColorSpaceBasicUnitValue>::iterator iter = ycbcr_buffer.begin();
               iter != ycbcr_buffer.end();
               ++iter)
          {
            ycbcr_y_buffer.push_back(*iter);
            ycbcr_y_buffer.push_back(Wcl::Colorlib::ColorSpaceBasicUnitValue(cb_middle_value));
            ycbcr_y_buffer.push_back(Wcl::Colorlib::ColorSpaceBasicUnitValue(cr_middle_value));
            
            ++iter;
            
            ycbcr_cb_buffer.push_back(Wcl::Colorlib::ColorSpaceBasicUnitValue(static_cast<uint8_t>(0)));
            ycbcr_cb_buffer.push_back(*iter);
            ycbcr_cb_buffer.push_back(Wcl::Colorlib::ColorSpaceBasicUnitValue(cr_middle_value));
            
            ++iter;
            
            ycbcr_cr_buffer.push_back(Wcl::Colorlib::ColorSpaceBasicUnitValue(static_cast<uint8_t>(0)));
            ycbcr_cr_buffer.push_back(Wcl::Colorlib::ColorSpaceBasicUnitValue(cb_middle_value));
            ycbcr_cr_buffer.push_back(*iter);
          }
          
          ycbcr_buffer.clear();
        }
        
        area_info_t::stage_end(&g_preview_area_info);
      }
      
      assert(ycbcr_y_buffer.size() == g_src_file_orig_width * g_src_file_orig_height * 3);
      assert(ycbcr_cb_buffer.size() == g_src_file_orig_width * g_src_file_orig_height * 3);
      assert(ycbcr_cr_buffer.size() == g_src_file_orig_width * g_src_file_orig_height * 3);
      
      // Create content for Y,U,V separate buffer.
      {
        area_info_t::add_content(&g_y_area_info, ycbcr_y_buffer);
        area_info_t::add_content(&g_cb_area_info, ycbcr_cb_buffer);
        area_info_t::add_content(&g_cr_area_info, ycbcr_cr_buffer);
      }
    }
  }
}

GtkWidget *
create_ycbcr_decouple_page()
{
  // ............................ free it when close
  // application.
  {
    gp_rgb2ycbcr_color_space_converter = ::new Wcl::Colorlib::ColorSpaceConverter;
    gp_ycbcr2rgb_color_space_converter = ::new Wcl::Colorlib::ColorSpaceConverter;
    
    {
      std::list<std::wstring> colorspace_plugin_name_list;
      
      {
        colorspace_plugin_name_list.push_back(L"Custom");
        colorspace_plugin_name_list.push_back(L"RGB");
        
        gp_rgb2ycbcr_color_space_converter->assign_src_color_space(
          colorspace_plugin_name_list);
        
        gp_ycbcr2rgb_color_space_converter->assign_dst_color_space(
          colorspace_plugin_name_list);
      }
      
      colorspace_plugin_name_list.clear();
      
      {
        // The type of YCBCR444 color space is not important,
        // because I will convert it back to "Custom::RGB"
        // again later. (for 3 separate Y,U,V buffer)
        colorspace_plugin_name_list.push_back(L"YCbCr");
        colorspace_plugin_name_list.push_back(L"ITU-R BT.709");
        
        gp_rgb2ycbcr_color_space_converter->assign_dst_color_space(
          colorspace_plugin_name_list);
        
        gp_ycbcr2rgb_color_space_converter->assign_src_color_space(
          colorspace_plugin_name_list);
      }
    }
  }
  
  GtkWidget * const vbox = gtk_vbox_new(FALSE, 0);
  
  {
    GtkWidget * const src_file_chooser_button =
      gtk_file_chooser_button_new("Choose a source file",
                                  GTK_FILE_CHOOSER_ACTION_OPEN);
    gtk_box_pack_start(GTK_BOX(vbox), src_file_chooser_button, FALSE, FALSE, 0);
    
    g_signal_connect(G_OBJECT(src_file_chooser_button), "file-set",
                     G_CALLBACK(on_select_src_file_set),
                     0);
  }
  
  GtkWidget * const preview_area_hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(preview_area_hbox), TRUE, TRUE, 0);
  
  GtkWidget * const preview_area_left_vbox = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(preview_area_hbox), preview_area_left_vbox, TRUE, TRUE, 0);
  
  GtkWidget * const preview_area_center_vbox = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(preview_area_hbox), preview_area_center_vbox, TRUE, TRUE, 0);
  
  {
    GtkWidget * const src_frame = gtk_frame_new("Source Image");
    gtk_box_pack_start(GTK_BOX(preview_area_center_vbox), src_frame, TRUE, TRUE, 0);
    
    GtkWidget * const src_area_vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(src_frame), src_area_vbox);
    
    g_preview_area_info.mp_widget = gtk_drawing_area_new();
    gtk_box_pack_start(GTK_BOX(src_area_vbox), g_preview_area_info.mp_widget, TRUE, TRUE, 0);
    
    gtk_widget_add_events(g_preview_area_info.mp_widget, GDK_EXPOSURE_MASK | GDK_STRUCTURE_MASK);
    g_signal_connect(G_OBJECT(g_preview_area_info.mp_widget), "configure_event",
                     G_CALLBACK(on_area_configure_event), &g_preview_area_info);
    g_signal_connect(G_OBJECT(g_preview_area_info.mp_widget), "expose_event",
                     G_CALLBACK(on_area_expose_event), &g_preview_area_info);
    
    g_preview_area_info.mp_progress_bar = GTK_PROGRESS_BAR(gtk_progress_bar_new());
    gtk_box_pack_start(GTK_BOX(src_area_vbox), GTK_WIDGET(g_preview_area_info.mp_progress_bar), FALSE, FALSE, 0);
  }
  
  GtkWidget * const preview_area_right_vbox = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(preview_area_hbox), preview_area_right_vbox, TRUE, TRUE, 0);
  
  GtkWidget * const ycbcr_hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), ycbcr_hbox, TRUE, TRUE, 0);
  
  {
    GtkWidget * const y_frame = gtk_frame_new("Y");
    gtk_box_pack_start(GTK_BOX(ycbcr_hbox), y_frame, TRUE, TRUE, 0);
    
    GtkWidget * const y_area_vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(y_frame), y_area_vbox);
    
    g_y_area_info.mp_widget = gtk_drawing_area_new();
    gtk_box_pack_start(GTK_BOX(y_area_vbox), g_y_area_info.mp_widget, TRUE, TRUE, 0);
    
    gtk_widget_add_events(g_y_area_info.mp_widget, GDK_EXPOSURE_MASK | GDK_STRUCTURE_MASK);
    g_signal_connect(G_OBJECT(g_y_area_info.mp_widget), "configure_event",
                     G_CALLBACK(on_area_configure_event), &g_y_area_info);
    g_signal_connect(G_OBJECT(g_y_area_info.mp_widget), "expose_event",
                     G_CALLBACK(on_area_expose_event), &g_y_area_info);
    
    g_y_area_info.mp_progress_bar = GTK_PROGRESS_BAR(gtk_progress_bar_new());
    gtk_box_pack_start(GTK_BOX(y_area_vbox), GTK_WIDGET(g_y_area_info.mp_progress_bar), FALSE, FALSE, 0);
  }
  
  {
    GtkWidget * const cb_frame = gtk_frame_new("Cb");
    gtk_box_pack_start(GTK_BOX(ycbcr_hbox), cb_frame, TRUE, TRUE, 0);
    
    GtkWidget * const cb_area_vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(cb_frame), cb_area_vbox);
    
    g_cb_area_info.mp_widget = gtk_drawing_area_new();
    gtk_box_pack_start(GTK_BOX(cb_area_vbox), g_cb_area_info.mp_widget, TRUE, TRUE, 0);
    
    gtk_widget_add_events(g_cb_area_info.mp_widget, GDK_EXPOSURE_MASK | GDK_STRUCTURE_MASK);
    g_signal_connect(G_OBJECT(g_cb_area_info.mp_widget), "configure_event",
                     G_CALLBACK(on_area_configure_event), &g_cb_area_info);
    g_signal_connect(G_OBJECT(g_cb_area_info.mp_widget), "expose_event",
                     G_CALLBACK(on_area_expose_event), &g_cb_area_info);
    
    g_cb_area_info.mp_progress_bar = GTK_PROGRESS_BAR(gtk_progress_bar_new());
    gtk_box_pack_start(GTK_BOX(cb_area_vbox), GTK_WIDGET(g_cb_area_info.mp_progress_bar), FALSE, FALSE, 0);
  }
  
  {
    GtkWidget * const cr_frame = gtk_frame_new("Cr");
    gtk_box_pack_start(GTK_BOX(ycbcr_hbox), cr_frame, TRUE, TRUE, 0);
    
    GtkWidget * const cr_area_vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(cr_frame), cr_area_vbox);
    
    g_cr_area_info.mp_widget = gtk_drawing_area_new();
    gtk_box_pack_start(GTK_BOX(cr_area_vbox), g_cr_area_info.mp_widget, TRUE, TRUE, 0);
    
    gtk_widget_add_events(g_cr_area_info.mp_widget, GDK_EXPOSURE_MASK | GDK_STRUCTURE_MASK);
    g_signal_connect(G_OBJECT(g_cr_area_info.mp_widget), "configure_event",
                     G_CALLBACK(on_area_configure_event), &g_cr_area_info);
    g_signal_connect(G_OBJECT(g_cr_area_info.mp_widget), "expose_event",
                     G_CALLBACK(on_area_expose_event), &g_cr_area_info);
    
    g_cr_area_info.mp_progress_bar = GTK_PROGRESS_BAR(gtk_progress_bar_new());
    gtk_box_pack_start(GTK_BOX(cr_area_vbox), GTK_WIDGET(g_cr_area_info.mp_progress_bar), FALSE, FALSE, 0);
  }
  
  return vbox;
}
