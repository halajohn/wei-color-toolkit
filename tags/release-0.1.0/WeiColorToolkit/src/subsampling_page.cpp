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

#include "mainwnd.hpp"

#include "wct_types.hpp"
#include "wcl_colorlib/colorlib.hpp"
#include "wcl_fmtstr/fmtstr.h"

#include "gtk_combo_box_hack.hpp"
#include "gtk_tree_model_hack.hpp"

#define MAX_DST_SUBSAMPLING_COUNT 10.0
#define MIN_DST_SUBSAMPLING_COUNT 1.0

#define MIN_SOURCE_WIDTH 1.0
#define MAX_SOURCE_WIDTH 65536.0

#define MIN_SOURCE_HEIGHT 1.0
#define MAX_SOURCE_HEIGHT 65536.0

namespace
{
  // ===================================================
  //                  SubSampling Info
  // ===================================================
  enum SubSamplingInfoType
  {
    SUBSAMPLING_INFO_TYPE_SRC,
    SUBSAMPLING_INFO_TYPE_DST
  };
  typedef enum SubSamplingInfoType SubSamplingInfoType;
  
  struct SubSamplingInfo
  {
  protected:
    
    SubSamplingInfo(SubSamplingInfoType const type)
      : m_type(type),
        mp_subsampling_combo(0),
        mp_progress_bar(0),
        m_curr_preview_progress_threshold(0.0)
    { }
    
  public:
    
    virtual ~SubSamplingInfo() {}
    
    SubSamplingInfoType m_type;
    GtkTreeIter active_iter;
    
    GtkComboBox *mp_subsampling_combo;
    GtkEntry *mp_fullname;
    
    GtkProgressBar *mp_progress_bar;
    
    double_t m_curr_preview_progress_threshold;
    
    virtual Wcl::Colorlib::SubSamplingConverter *subsampling_converter() const = 0;
    virtual Wcl::Colorlib::ColorSpaceConverter *colorspace_converter() const = 0;
    
    virtual GtkWidget *savefile_filename_entry() const = 0;
  };
  typedef struct SubSamplingInfo SubSamplingInfo;
  
  // ===================================================
  //               Src SubSampling Info
  // ===================================================
  struct SrcSubSamplingInfo : public SubSamplingInfo
  {
    GtkFileChooser *mp_file_chooser;
    GtkSpinButton *mp_width_spin;
    GtkSpinButton *mp_height_spin;
    
    uint32_t m_preview_area_width;
    uint32_t m_preview_area_height;
    
    uint32_t m_scaled_width;
    uint32_t m_scaled_height;
    
    Wcl::Colorlib::SubSamplingConverter *mp_preview_area_subsampling_converter;
    Wcl::Colorlib::ColorSpaceConverter *mp_preview_area_colorspace_converter;
    
    GtkWidget *mp_preview_area;
    GtkWidget *mp_preview_area_bmp_filename_entry;
    
    // The following field will be clear to empty when a new
    // file is selected to preview.
    std::vector<Wcl::Colorlib::ColorSpaceBasicUnitValue> m_src_buffer;
    std::vector<uint8_t> m_preview_area_rgb_buffer;
    std::vector<uint8_t> m_preview_area_argb_buffer;
    GdkPixbuf *mp_preview_area_orig_pixbuf;
    GdkPixbuf *mp_preview_area_scaled_pixbuf;
    
    bool m_select_custom_src_subsampling;
    
    /** @name life cycle
     */
    //@{
    SrcSubSamplingInfo()
      : SubSamplingInfo(SUBSAMPLING_INFO_TYPE_SRC),
        mp_preview_area_orig_pixbuf(0),
        mp_preview_area_scaled_pixbuf(0),
        mp_preview_area_subsampling_converter(::new Wcl::Colorlib::SubSamplingConverter),
        mp_preview_area_colorspace_converter(::new Wcl::Colorlib::ColorSpaceConverter),
        mp_file_chooser(0),
        mp_width_spin(0),
        mp_height_spin(0),
        mp_preview_area(0),
        m_select_custom_src_subsampling(false)
    {}
    
    ~SrcSubSamplingInfo()
    {
      assert(mp_preview_area_subsampling_converter != 0);
      ::delete mp_preview_area_subsampling_converter;
      
      assert(mp_preview_area_colorspace_converter != 0);
      ::delete mp_preview_area_colorspace_converter;
    }
    //@}
    
    Wcl::Colorlib::SubSamplingConverter *subsampling_converter() const
    { return mp_preview_area_subsampling_converter; }
    
    Wcl::Colorlib::ColorSpaceConverter *colorspace_converter() const
    { return mp_preview_area_colorspace_converter; }
    
    GtkWidget *savefile_filename_entry() const
    { return mp_preview_area_bmp_filename_entry; }
  };
  typedef struct SrcSubSamplingInfo SrcSubSamplingInfo;
  
  // ===================================================
  //               Dst SubSampling Info
  // ===================================================
  struct DstSubSamplingInfo : public SubSamplingInfo
  {
    Wcl::Colorlib::SubSamplingConverter *mp_converter;
    GtkWidget *mp_savefile_filename_entry;
    std::vector<Wcl::Colorlib::ColorSpaceBasicUnitValue> m_converted_data;
    
    /** @name life cycle
     */
    //@{
    DstSubSamplingInfo()
      : SubSamplingInfo(SUBSAMPLING_INFO_TYPE_DST),
        mp_converter(::new Wcl::Colorlib::SubSamplingConverter)
    {}
    
    ~DstSubSamplingInfo()
    {
      assert(mp_converter != 0);
      
      ::delete mp_converter;
    }
    //@}
    
    Wcl::Colorlib::SubSamplingConverter *subsampling_converter() const
    { return mp_converter; }
    
    Wcl::Colorlib::ColorSpaceConverter *colorspace_converter() const
    { return 0; }
    
    GtkWidget *savefile_filename_entry() const
    { return mp_savefile_filename_entry; }
  };
  typedef struct DstSubSamplingInfo DstSubSamplingInfo;
  
  SrcSubSamplingInfo *mp_src_info;
  std::list<DstSubSamplingInfo *> mp_dst_info;
  
  // ===================================================
  //              Convert pixel page codes
  // ===================================================
  bool
  conversion_percentage_cb(void *user_data,
                           double_t const percentage)
  {
    SubSamplingInfo * const info = static_cast<SubSamplingInfo *>(user_data);
    assert(info != 0);
    
    if (percentage > info->m_curr_preview_progress_threshold)
    {
      gtk_progress_bar_set_fraction(info->mp_progress_bar, percentage);
      
      info->m_curr_preview_progress_threshold += 0.1;
    }
    
    while (TRUE == gtk_events_pending())
    {
      gtk_main_iteration();
    }
    
    return true;
  }
  
  bool
  subsampling_conversion_stage_name_cb(void *user_data,
                                       wchar_t const * const name)
  {
    assert(name != 0);
    
    SubSamplingInfo * const info = static_cast<SubSamplingInfo *>(user_data);
    assert(info != 0);
    
    {
      char * const stage_name = fmtstr_wcstombs(name, wcslen(name));
      assert(stage_name != 0);
      
      gtk_progress_bar_set_text(info->mp_progress_bar, stage_name);
      
      fmtstr_delete(stage_name);
    }
    
    info->m_curr_preview_progress_threshold = 0;
    
    while (TRUE == gtk_events_pending())
    {
      gtk_main_iteration();
    }
    
    return true;
  }
  
  GtkTreeModel *
  create_subsampling_model()
  {
    GtkTreeStore * const subsampling_model = gtk_tree_store_new(1, G_TYPE_STRING);
    
    Wcl::Colorlib::PluginTreeNode<Wcl::Colorlib::SubSamplingConverterPlugin> * const root =
      Wcl::Colorlib::SubSamplingConverter::get_plugin_tree_root();
    
    std::list<GtkTreeIter> stack;
    
    // Push a dummy GtkTreeIter to represent the init plugin
    // tree node, so that the pop_back() codes below can pop
    // up one node when I return to the init plugin tree
    // node.
    stack.push_back(GtkTreeIter());
    
    for (Wcl::Colorlib::PluginTreeNode<Wcl::Colorlib::SubSamplingConverterPlugin>::iterator plugin_tree_iter = root->begin();
         plugin_tree_iter != root->end();
         )
    {
      if (true == plugin_tree_iter.return_from_children())
      {
        stack.pop_back();
      }
      else
      {
        GtkTreeIter *group_iter;
        
        if (stack.size() > 1)
        {
          group_iter = &(stack.back());
        }
        else
        {
          group_iter = 0;
        }
        
        GtkTreeIter *curr_iter = 0;
        GtkTreeIter iter;
      
        if ((*plugin_tree_iter)->children().size() != 0)
        {
          // A group node.
          stack.push_back(GtkTreeIter());
        
          curr_iter = &(stack.back());
        }
        else
        {
          // A leaf node.
          curr_iter = &iter;
        }
        
        gtk_tree_store_append(GTK_TREE_STORE(subsampling_model), curr_iter, group_iter);
        
        {
          char * const plugin_name = fmtstr_wcstombs((*plugin_tree_iter)->name().c_str(),
                                                     (*plugin_tree_iter)->name().size());
          assert(plugin_name != 0);
          
          gtk_tree_store_set(GTK_TREE_STORE(subsampling_model), curr_iter,
                             0, plugin_name,
                             -1);
          
          fmtstr_delete(plugin_name);
        }
      }
      
      ++plugin_tree_iter;
    }
    
    return GTK_TREE_MODEL(subsampling_model);
  }
  
  void
  on_subsampling_combo_changed(GtkComboBox *widget,
                               gpointer     user_data)
  {
    SubSamplingInfo * const info = static_cast<SubSamplingInfo *>(user_data);
    assert(info != 0);
    
    GtkTreeIter iter;
    
    if (TRUE == gtk_combo_box_get_active_iter(widget, &iter))
    {
      // I have select a different source subsampling.
      GtkTreeModel * const model = gtk_combo_box_get_model(widget);
      
      std::list<std::wstring> subsampling_plugin_name_list;
      
      gchar *name;
      
      // Add parent color space name list to the active
      // tree model entry.
      gtk_combo_box_collect_parent_node_name_list(model, &iter, &subsampling_plugin_name_list, &name);
      
      info->active_iter = iter;
      
      // If the modified subsampling combo is the source
      // subsampling combo, then I have to update the
      // destination subsampling info.
      //
      // 1) If there exists at least one dst subsampling
      // info, then I have to update the converter's source
      // plugin info contained in them.
      //
      // 2) If there exists no dst subsampling info, I
      // have to create a dst subsampling info and put it
      // into mp_dst_info.
      
      switch (info->m_type)
      {
      case SUBSAMPLING_INFO_TYPE_SRC:
        // The change combo is a source subsampling combo.
        assert(mp_dst_info.size() != 0);
        
        // Config converter for preview area
        info->subsampling_converter()->assign_src_subsampling(subsampling_plugin_name_list);
        
        {
          std::list<std::wstring> src_color_space_name;
          
          info->subsampling_converter()->get_src_plugin()->get_ycbcr_color_space_name(src_color_space_name);
          
          try
          {
            static_cast<SrcSubSamplingInfo *>(info)->colorspace_converter()->assign_src_color_space(src_color_space_name);
            static_cast<SrcSubSamplingInfo *>(info)->m_select_custom_src_subsampling = false;
          }
          catch (Wcl::Colorlib::SrcAndDstColorSpaceConverterPluginCanNotBeDiffCustomAtTheSameTime &)
          {
            static_cast<SrcSubSamplingInfo *>(info)->m_select_custom_src_subsampling = true;
          }
        }
        
        BOOST_FOREACH(DstSubSamplingInfo * const dst_info, mp_dst_info)
        {
          assert(dst_info->subsampling_converter() != 0);
          
          dst_info->subsampling_converter()->assign_src_subsampling(subsampling_plugin_name_list);
        }
        break;
        
      case SUBSAMPLING_INFO_TYPE_DST:
        {
          assert(info->subsampling_converter() != 0);
          
          info->subsampling_converter()->assign_dst_subsampling(subsampling_plugin_name_list);
        }
        break;
        
      default:
        assert(0);
        break;
      }
      
      gtk_entry_set_text(info->mp_fullname, name);
      
      g_free(name);
    }
  }
  
  void
  on_select_file_button_clicked(GtkButton * /* widget */,
                                gpointer  user_data)
  {
    SubSamplingInfo * const info = static_cast<SubSamplingInfo *>(user_data);
    assert(info != 0);
    
    GtkWidget * const dialog = gtk_file_chooser_dialog_new("Save File",
                                                           GTK_WINDOW(mainwnd),
                                                           GTK_FILE_CHOOSER_ACTION_SAVE,
                                                           GTK_STOCK_CANCEL,
                                                           GTK_RESPONSE_CANCEL,
                                                           GTK_STOCK_SAVE,
                                                           GTK_RESPONSE_ACCEPT,
                                                           NULL);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
    
    if (GTK_RESPONSE_ACCEPT == gtk_dialog_run(GTK_DIALOG(dialog)))
    {
      char *filename;
      
      filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
      
      gtk_entry_set_text(GTK_ENTRY(info->savefile_filename_entry()), filename);
      
      g_free(filename);
    }
    
    gtk_widget_destroy(dialog);
  }
    
  void
  on_dst_subsampling_file_save_button_clicked(GtkButton * /* widget */,
                                              gpointer   user_data)
  {
    DstSubSamplingInfo * const info = static_cast<DstSubSamplingInfo *>(user_data);
    assert(info != 0);
    
    gchar const * const filename = gtk_entry_get_text(
      GTK_ENTRY(info->savefile_filename_entry()));
    
    if (0 == g_ascii_strcasecmp(filename, ""))
    {
      GtkWidget * const dialog = gtk_message_dialog_new(
        GTK_WINDOW(mainwnd),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_CLOSE,
        "You does not specify a file to save.");
      gtk_dialog_run(GTK_DIALOG(dialog));
      gtk_widget_destroy(dialog);
      
      return;
    }
    else
    {
      std::ofstream out_file;
      
      out_file.open(filename, std::ios::binary | std::ios::trunc);
      
      // get length of file:
      out_file.seekp(0, std::ios::beg);
      
      if (info->m_converted_data.size() > 0)
      {
        assert(Wcl::Colorlib::COLOR_SPACE_BASIC_UNIT_VALUE_TYPE_UINT8 ==
               info->m_converted_data[0].get_type());
      
        // write data as a block:
        {
          std::vector<uint8_t> dst_buffer;
        
          BOOST_FOREACH(Wcl::Colorlib::ColorSpaceBasicUnitValue const &value,
                        info->m_converted_data)
          {
            dst_buffer.push_back(value.get_value<uint8_t>());
          }
        
          out_file.write(reinterpret_cast<char *>(&(dst_buffer[0])),
                         dst_buffer.size());
        }
      }
      
      assert(false == out_file.fail());
      
      out_file.close();
    }
  }
  
  void
  on_preview_file_save_button_clicked(GtkButton * /* widget */,
                                      gpointer   /*   user_data */)
  {
    gchar const * const filename = gtk_entry_get_text(
      GTK_ENTRY(mp_src_info->mp_preview_area_bmp_filename_entry));
    
    if (0 == g_ascii_strcasecmp(filename, ""))
    {
      GtkWidget * const dialog = gtk_message_dialog_new(
        GTK_WINDOW(mainwnd),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_CLOSE,
        "You does not specify a file to save.");
      gtk_dialog_run(GTK_DIALOG(dialog));
      gtk_widget_destroy(dialog);
      
      return;
    }
    else
    {
      if (0 == mp_src_info->mp_preview_area_orig_pixbuf)
      {
        GtkWidget * const dialog = gtk_message_dialog_new(
          GTK_WINDOW(mainwnd),
          GTK_DIALOG_DESTROY_WITH_PARENT,
          GTK_MESSAGE_ERROR,
          GTK_BUTTONS_CLOSE,
          "You have to preview something before saving.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        
        return;
      }
      else
      {
        gdk_pixbuf_save(mp_src_info->mp_preview_area_orig_pixbuf,
                        filename,
                        "bmp",
                        0,
                        0);
      }
    }
  }
  
  void
  on_convert_button_clicked(GtkButton * /* widget */,
                            gpointer   /* user_data */)
  {
    assert(mp_dst_info.size() != 0);
    assert(mp_dst_info.front() != 0);
    assert(mp_dst_info.front()->subsampling_converter() != 0);
    
    Wcl::Colorlib::SubSamplingConverterPlugin * const src_plugin =
      mp_dst_info.front()->subsampling_converter()->get_src_plugin();
        
#if defined(_DEBUG)
    // Ensure src plugins for each dest info are equal.
    {
      BOOST_FOREACH(DstSubSamplingInfo * const dst_info, mp_dst_info)
      {
        assert(dst_info->subsampling_converter() != 0);
        assert(dst_info->subsampling_converter()->get_src_plugin() == src_plugin);
      }
    }
#endif
    
    if (0 == src_plugin)
    {
      GtkWidget * const dialog = gtk_message_dialog_new(
        GTK_WINDOW(mainwnd),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_CLOSE,
        "You does not specify the source subsampling.");
      gtk_dialog_run(GTK_DIALOG(dialog));
      gtk_widget_destroy(dialog);
      
      return;
    }
    else
    {
      gint const width = gtk_spin_button_get_value_as_int(mp_src_info->mp_width_spin);
      gint const height = gtk_spin_button_get_value_as_int(mp_src_info->mp_height_spin);
      
      uint32_t idx = 1;
      
      BOOST_FOREACH(DstSubSamplingInfo * const info, mp_dst_info)
      {
        if (0 == info->subsampling_converter()->get_dst_plugin())
        {
          GtkWidget * const dialog = gtk_message_dialog_new(
            GTK_WINDOW(mainwnd),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            "You does not specify the destination subsampling of item %d.",
            idx);
          gtk_dialog_run(GTK_DIALOG(dialog));
          gtk_widget_destroy(dialog);
          
          return;
        }
        else
        {
          info->subsampling_converter()->convert_buffer(mp_src_info->m_src_buffer,
                                                        info->m_converted_data,
                                                        width,
                                                        height);
        }
        
        ++idx;
      }
    }
  }
  
  void
  reset_scaled_preview_pixbuf()
  {
    if (mp_src_info->mp_preview_area_scaled_pixbuf != 0)
    {
      g_object_unref(mp_src_info->mp_preview_area_scaled_pixbuf);
      mp_src_info->mp_preview_area_scaled_pixbuf = 0;
      
      gdk_window_invalidate_rect(mp_src_info->mp_preview_area->window, NULL, FALSE);
    }
  }
  
  void
  reset_orig_preview_pixbuf()
  {
    if (mp_src_info->mp_preview_area_orig_pixbuf != 0)
    {
      g_object_unref(mp_src_info->mp_preview_area_orig_pixbuf);
      mp_src_info->mp_preview_area_orig_pixbuf = 0;
    }
    
    reset_scaled_preview_pixbuf();
  }
  
  void
  create_scaled_preview_pixbuf()
  {
    gint const width = gtk_spin_button_get_value_as_int(mp_src_info->mp_width_spin);
    gint const height = gtk_spin_button_get_value_as_int(mp_src_info->mp_height_spin);
    
    double_t const width_ratio = (mp_src_info->m_preview_area_width / boost::numeric_cast<double_t>(width));
    double_t const height_ratio = (mp_src_info->m_preview_area_height / boost::numeric_cast<double_t>(height));
    
    double_t const real_ratio = (width_ratio >= height_ratio) ? height_ratio : width_ratio;
    
    uint32_t const scaled_width = boost::numeric_cast<uint32_t>(width * real_ratio);
    uint32_t const scaled_height = boost::numeric_cast<uint32_t>(height * real_ratio);
        
    mp_src_info->m_scaled_width =
      (scaled_width > mp_src_info->m_preview_area_width)
      ? mp_src_info->m_preview_area_width
      : scaled_width;
        
    mp_src_info->m_scaled_height =
      (scaled_height > mp_src_info->m_preview_area_height)
      ? mp_src_info->m_preview_area_height
      : scaled_height;
        
    mp_src_info->mp_preview_area_scaled_pixbuf =
      gdk_pixbuf_scale_simple(mp_src_info->mp_preview_area_orig_pixbuf,
                              mp_src_info->m_scaled_width,
                              mp_src_info->m_scaled_height,
                              GDK_INTERP_BILINEAR);
    assert(mp_src_info->mp_preview_area_scaled_pixbuf != 0);
    
    gdk_window_invalidate_rect(mp_src_info->mp_preview_area->window, NULL, FALSE);
  }
  
  gboolean
  on_preview_area_configure_event(GtkWidget         * /* widget */,
                                  GdkEventConfigure *event,
                                  gpointer           /*   user_data */)
  {
    mp_src_info->m_preview_area_width = event->width;
    mp_src_info->m_preview_area_height = event->height;
    
    if (mp_src_info->mp_preview_area_orig_pixbuf != 0)
    {
      reset_scaled_preview_pixbuf();      
      create_scaled_preview_pixbuf();
    }
    
    return FALSE;
  }
  
  gboolean
  on_preview_area_expose_event(GtkWidget      * /* widget */,
                               GdkEventExpose *event,
                               gpointer        /* user_data */)
  {
    if (mp_src_info->mp_preview_area_scaled_pixbuf != 0)
    {
      gdk_draw_pixbuf(event->window,
                      NULL,
                      mp_src_info->mp_preview_area_scaled_pixbuf,
                      0, 0,
                      (mp_src_info->m_preview_area_width - mp_src_info->m_scaled_width) / 2,
                      (mp_src_info->m_preview_area_height - mp_src_info->m_scaled_height) / 2,
                      mp_src_info->m_scaled_width,
                      mp_src_info->m_scaled_height,
                      GDK_RGB_DITHER_NORMAL,
                      0, 0);
    }
    
    return FALSE;
  }
  
  void
  set_combo_sensitive(GtkCellLayout   * /* cell_layout */,
                      GtkCellRenderer *cell,
                      GtkTreeModel    *tree_model,
                      GtkTreeIter     *iter,
                      gpointer         /* data */)
  {
    GtkTreeIter child_iter;
    
    // Check if this 'iter' has a child iter.
    gboolean const has_child = gtk_tree_model_iter_nth_child(tree_model, &child_iter, iter, 0);
    
    if (TRUE == has_child)
    {
      g_object_set(cell, "sensitive", FALSE, NULL);
    }
    else
    {
      g_object_set(cell, "sensitive", TRUE, NULL);
    }
  }
  
  GtkWidget *
  create_dst_subsampling_ui()
  {
    GtkWidget * const each_hbox = gtk_hbox_new(FALSE, 0);
    
    GtkWidget * const chooser_vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(each_hbox), chooser_vbox, TRUE, TRUE, 0);
    
    GtkWidget * const frame = gtk_frame_new("Target subsampling");
    gtk_box_pack_start(GTK_BOX(chooser_vbox), frame, TRUE, TRUE, 0);
    
    GtkWidget * const frame_vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame), frame_vbox);
    
    GtkWidget * const select_hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(frame_vbox), select_hbox, FALSE, FALSE, 0);
    
    GtkWidget * const label = gtk_label_new("Select:");
    gtk_box_pack_start(GTK_BOX(select_hbox), label, FALSE, FALSE, 0);
    
    {
      GtkTreeModel * const dst_subsampling_model = create_subsampling_model();
      GtkWidget * const dst_combo = gtk_combo_box_new_with_model(dst_subsampling_model);
      g_object_unref(dst_subsampling_model);
      gtk_box_pack_start(GTK_BOX(select_hbox), dst_combo, TRUE, TRUE, 0);
      
      GtkCellRenderer * const renderer = gtk_cell_renderer_text_new();
      
      gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(dst_combo),
                                 renderer,
                                 TRUE);
      gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(dst_combo),
                                     renderer,
                                     "text", 0,
                                     NULL);
      gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(dst_combo),
                                         renderer,
                                         set_combo_sensitive,
                                         NULL, NULL);
      
      // Create dst color space info
      {
        mp_dst_info.push_back(::new DstSubSamplingInfo());
        
        mp_dst_info.back()->subsampling_converter()->register_percentage_cb(
          conversion_percentage_cb,
          mp_dst_info.back());
        
        mp_dst_info.back()->subsampling_converter()->register_stage_name_cb(
          subsampling_conversion_stage_name_cb,
          mp_dst_info.back());
        
        mp_dst_info.back()->mp_subsampling_combo = GTK_COMBO_BOX(dst_combo);
        gtk_tree_model_iter_invalid(&(mp_dst_info.back()->active_iter));
        
        // Copy the src_plugin from the existed dest
        // subsampling converter to the newly created one.
        {
          Wcl::Colorlib::SubSamplingConverterPlugin * const plugin = 
            mp_dst_info.front()->subsampling_converter()->get_src_plugin();
          
          if (plugin != 0)
          {
            mp_dst_info.back()->subsampling_converter()->assign_src_plugin(
              mp_dst_info.front()->subsampling_converter()->get_src_plugin_info());
          }
        }
      }
      
      g_signal_connect(G_OBJECT(dst_combo), "changed",
                       G_CALLBACK(on_subsampling_combo_changed),
                       mp_dst_info.back());
    }
    
    GtkWidget * const fullname_hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(frame_vbox), fullname_hbox, FALSE, FALSE, 0);
    
    GtkWidget * const fullname_label = gtk_label_new("Fullname:");
    gtk_box_pack_start(GTK_BOX(fullname_hbox), fullname_label, FALSE, FALSE, 0);
    
    mp_dst_info.back()->mp_fullname = GTK_ENTRY(gtk_entry_new());
    gtk_box_pack_start(GTK_BOX(fullname_hbox),
                       GTK_WIDGET(mp_dst_info.back()->mp_fullname),
                       TRUE, TRUE, 0);
    
    gtk_editable_set_editable(GTK_EDITABLE(mp_dst_info.back()->mp_fullname), FALSE);
    
    mp_dst_info.back()->mp_progress_bar = GTK_PROGRESS_BAR(gtk_progress_bar_new());
    gtk_box_pack_start(GTK_BOX(chooser_vbox), GTK_WIDGET(mp_dst_info.back()->mp_progress_bar), FALSE, FALSE, 0);
    
    {
      GtkWidget * const save_to_file_frame = gtk_frame_new("Save to file");
      gtk_box_pack_start(GTK_BOX(chooser_vbox), save_to_file_frame, FALSE, FALSE, 0);
      
      GtkWidget * const save_to_file_vbox = gtk_vbox_new(FALSE, 0);
      gtk_container_add(GTK_CONTAINER(save_to_file_frame), save_to_file_vbox);
      
      GtkWidget * const file_file_path_hbox = gtk_hbox_new(FALSE, 0);
      gtk_box_pack_start(GTK_BOX(save_to_file_vbox), file_file_path_hbox, FALSE, FALSE, 0);      
      
      GtkWidget * const select_file_button = gtk_button_new_with_label("Select file");
      gtk_box_pack_start(GTK_BOX(file_file_path_hbox), select_file_button, FALSE, FALSE, 0);
      
      g_signal_connect(G_OBJECT(select_file_button), "clicked",
                       G_CALLBACK(on_select_file_button_clicked),
                       mp_dst_info.back());
      
      GtkWidget * const file_name_entry = gtk_entry_new();
      gtk_box_pack_start(GTK_BOX(file_file_path_hbox), file_name_entry, TRUE, TRUE, 0);      
      
      gtk_editable_set_editable(GTK_EDITABLE(file_name_entry), FALSE);
      
      mp_dst_info.back()->mp_savefile_filename_entry = file_name_entry;
      
      GtkWidget * const file_save_button = gtk_button_new_with_label("save to file");
      gtk_box_pack_start(GTK_BOX(save_to_file_vbox), file_save_button, FALSE, FALSE, 0);
      
      g_signal_connect(G_OBJECT(file_save_button), "clicked",
                       G_CALLBACK(on_dst_subsampling_file_save_button_clicked),
                       mp_dst_info.back());
    }
    
    return each_hbox;
  }
  
  void
  on_dst_subsampling_count_spin_value_changed(GtkSpinButton * const spin,
                                              gpointer user_data)
  {
    uint32_t desired_count;
    
    {
      gint const desired_count_gtk = gtk_spin_button_get_value_as_int(spin);
      
      desired_count = boost::numeric_cast<uint32_t>(desired_count_gtk);
    }
    
    assert((desired_count >= static_cast<gint>(MIN_DST_SUBSAMPLING_COUNT)) &&
           (desired_count <= static_cast<gint>(MAX_DST_SUBSAMPLING_COUNT)));
    
    std::list<DstSubSamplingInfo *>::size_type const curr_count = mp_dst_info.size();
    
    int32_t const diff_count = desired_count - curr_count;
    
    GtkBox * const dst_vbox = GTK_BOX(user_data);
    
    // Remove item from UI if necessary.
    if (diff_count < 0)
    {
      GList *list = gtk_container_get_children(GTK_CONTAINER(dst_vbox));
      
      // This container DO have children.
      assert(list != 0);
      assert(0 == list->prev);
      
      // Find the last one.
      list = g_list_last(list);
      
      assert(list != 0);
      assert(0 == list->next);
      
      for (int32_t i = diff_count; i != 0; ++i)
      {
        assert(list != 0);
        assert(list->data != 0);
        
        GList * const saved = list->prev;
        
        gtk_widget_destroy(GTK_WIDGET(list->data));
        
        list = saved;
        
        // Remove item from mp_dst_info.
        {
          delete mp_dst_info.back();
          
          mp_dst_info.pop_back();
        }
      }
      
      assert(mp_dst_info.size() == desired_count);
    }
    
    // Add item to UI & mp_dst_info.
    while (mp_dst_info.size() < boost::numeric_cast<uint32_t>(desired_count))
    {
      GtkWidget * const each_dst_ui = create_dst_subsampling_ui();
      gtk_box_pack_start(GTK_BOX(dst_vbox), each_dst_ui, FALSE, FALSE, 0);
      
      gtk_widget_show_all(GTK_WIDGET(each_dst_ui));
    }
  }
  
  void
  free_preview_area_pixbuf_data(guchar * /* pixels */,
                                gpointer /* user_data */)
  {
    mp_src_info->m_preview_area_rgb_buffer.clear();
    mp_src_info->m_preview_area_argb_buffer.clear();
  }
  
  void
  on_preview_button_clicked(GtkButton * /* button */,
                            gpointer /* user_data */)
  {    
    if (0 == mp_src_info->mp_preview_area_subsampling_converter->get_src_plugin())
    {
      GtkWidget * const dialog = gtk_message_dialog_new(
        GTK_WINDOW(mainwnd),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_CLOSE,
        "Don't know how to display preview image becuase of not selecting a source sub sampling type");
      gtk_dialog_run(GTK_DIALOG(dialog));
      gtk_widget_destroy(dialog);
      return;
    }
    
    gint const width = gtk_spin_button_get_value_as_int(mp_src_info->mp_width_spin);
    gint const height = gtk_spin_button_get_value_as_int(mp_src_info->mp_height_spin);
    
    // Clear preview area orig & scaled pixbuf.
    reset_orig_preview_pixbuf();
    
    if (true == mp_src_info->m_select_custom_src_subsampling)
    {
      GtkWidget * const dialog = gtk_message_dialog_new(
        GTK_WINDOW(mainwnd),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_CLOSE,
        "Don't know how to display preview image because of selecting a custom source subsampling type.");
      gtk_dialog_run(GTK_DIALOG(dialog));
      gtk_widget_destroy(dialog);
      return;
    }
    
    mp_src_info->m_curr_preview_progress_threshold = 0.1;
    
    {
      std::vector<Wcl::Colorlib::ColorSpaceBasicUnitValue> yuv444_buffer;
      std::vector<Wcl::Colorlib::ColorSpaceBasicUnitValue> rgb_buffer;
      
      // This is preview area. I will convert the src data
      // into YUV444 data first.
      mp_src_info->subsampling_converter()->convert_buffer(mp_src_info->m_src_buffer,
                                                           yuv444_buffer,
                                                           width,
                                                           height);
      
      {
        gtk_progress_bar_set_text(mp_src_info->mp_progress_bar, "Convert from YUV444 to RGB");
        mp_src_info->m_curr_preview_progress_threshold = 0;
        
        while (TRUE == gtk_events_pending())
        {
          gtk_main_iteration();
        }
      }
      
      // I will then use color space converter to convert
      // YUV444 data to RGB data.
      mp_src_info->colorspace_converter()->convert_buffer(yuv444_buffer, rgb_buffer);
      yuv444_buffer.clear();
      
      {
        gtk_progress_bar_set_text(mp_src_info->mp_progress_bar, "Synthesizing");
        mp_src_info->m_curr_preview_progress_threshold = 0;
        
        while (TRUE == gtk_events_pending())
        {
          gtk_main_iteration();
        }
      }
      
      // The data type in rgb_buffer is double_t, I have to
      // convert(clamp) it to uint8_t.
      BOOST_FOREACH(Wcl::Colorlib::ColorSpaceBasicUnitValue const &value, rgb_buffer)
      {
        mp_src_info->m_preview_area_rgb_buffer.push_back(Wcl::Colorlib::clamp_for_rgb(value.get_value<double_t>()));
      }
    }
    
    // display preview_area_rgb_buffer as an RGB raw data
    // buffer.
    {      
      // Convert RGB buffer to RGBA buffer.
      {
        uint32_t i = 0;
        
        for (std::vector<uint8_t>::iterator iter = mp_src_info->m_preview_area_rgb_buffer.begin();
             iter != mp_src_info->m_preview_area_rgb_buffer.end();
             ++iter)
        {
          mp_src_info->m_preview_area_argb_buffer.push_back(*iter);
          ++i;
          
          if (0 == (i % 3))
          {
            mp_src_info->m_preview_area_argb_buffer.push_back(static_cast<uint8_t>(0xFF));
          }
        }
      }
      
      // GdkPixbuf uses RGBA.
      mp_src_info->mp_preview_area_orig_pixbuf =
        gdk_pixbuf_new_from_data(
          &(mp_src_info->m_preview_area_argb_buffer[0]),
          GDK_COLORSPACE_RGB,
          TRUE,
          8,
          width,
          height,
          width * 4,
          free_preview_area_pixbuf_data,
          0);
      
      // Create the first scaled preview image.
      create_scaled_preview_pixbuf();
    }
  }
  
  void
  on_select_src_file_set(GtkFileChooserButton *filechooserbutton,
                         gpointer /* user_data */)
  {
    // Clear the content of the preview area
    {
      gtk_progress_bar_set_text(mp_src_info->mp_progress_bar, "");
      gtk_progress_bar_set_fraction(mp_src_info->mp_progress_bar, 0.0);
      
      while (TRUE == gtk_events_pending())
      {
        gtk_main_iteration();
      }
      
      mp_src_info->m_curr_preview_progress_threshold = 0.1;
      
      BOOST_FOREACH(DstSubSamplingInfo * const dst_info, mp_dst_info)
      {
        dst_info->m_curr_preview_progress_threshold = 0.1;
      }
      
      mp_src_info->m_preview_area_rgb_buffer.clear();
      mp_src_info->m_preview_area_argb_buffer.clear();
      
      reset_orig_preview_pixbuf();
    }
    
    gchar * const filename =
      gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(filechooserbutton));
    
    if (filename != NULL)
    {
      // read data from FILE to src_buffer.
      {
        std::vector<uint8_t> src_buffer_local;
        std::ifstream src_file;
      
        src_file.open(filename, std::ios::binary);
      
        // get length of file:
        src_file.seekg(0, std::ios::end);
        std::istream::streampos const length = src_file.tellg();
        src_file.seekg(0, std::ios::beg);
      
        // allocate memory:
        src_buffer_local.resize(length);
      
        mp_src_info->m_src_buffer.clear();
        mp_src_info->m_src_buffer.reserve(length);
      
        // read data as a block:
        src_file.read(reinterpret_cast<char *>(&(src_buffer_local[0])), length);
      
        assert(false == src_file.fail());
      
        src_file.close();
      
        BOOST_FOREACH(uint8_t const &value, src_buffer_local)
        {
          mp_src_info->m_src_buffer.push_back(Wcl::Colorlib::ColorSpaceBasicUnitValue(value));
        }
      }
    }
  }
}

// Main function for creating subsampling page.
GtkWidget *
create_subsampling_page()
{
  GtkWidget * const outer_box = gtk_vbox_new(FALSE, 0);
  
  GtkWidget *spin;
  
  // Create dst subsampling _count_ UI
  {
    GtkWidget * const alignment = gtk_alignment_new(1.0, 0.0, 0.0, 0.0);
    gtk_box_pack_start(GTK_BOX(outer_box), alignment, FALSE, FALSE, 0);
    
    GtkWidget * const hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(alignment), hbox);
    
    GtkWidget * const label = gtk_label_new("Dst subsampling count:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    
    GtkObject * const adjustment = gtk_adjustment_new(MIN_DST_SUBSAMPLING_COUNT,
                                                      MIN_DST_SUBSAMPLING_COUNT,
                                                      MAX_DST_SUBSAMPLING_COUNT,
                                                      1.0, 1.0, 1.0);
    spin = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 1.0, 0);
    gtk_box_pack_start(GTK_BOX(hbox), spin, FALSE, FALSE, 0);
    
    gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin), GTK_UPDATE_IF_VALID);
  }
  
  GtkWidget * const conversion_hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(outer_box), conversion_hbox, TRUE, TRUE, 0);
  
  // Create source subsampling UI
  {
    // Create src colro space info
    {
      mp_src_info = ::new SrcSubSamplingInfo();
      gtk_tree_model_iter_invalid(&(mp_src_info->active_iter));
      
      mp_src_info->subsampling_converter()->register_percentage_cb(
        conversion_percentage_cb,
        mp_src_info);
      
      mp_src_info->subsampling_converter()->register_stage_name_cb(
        subsampling_conversion_stage_name_cb,
        mp_src_info);
      
      mp_src_info->colorspace_converter()->register_percentage_cb(
        conversion_percentage_cb,
        mp_src_info);
    }
    
    GtkWidget * const src_vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(conversion_hbox), src_vbox, TRUE, TRUE, 0);
    
    {
      GtkWidget * const file_chooser_button = gtk_file_chooser_button_new("Choose a source file",
                                                                          GTK_FILE_CHOOSER_ACTION_OPEN);
      gtk_box_pack_start(GTK_BOX(src_vbox), file_chooser_button, FALSE, FALSE, 0);
      
      mp_src_info->mp_file_chooser = GTK_FILE_CHOOSER(file_chooser_button);
      
      g_signal_connect(G_OBJECT(file_chooser_button), "file-set",
                       G_CALLBACK(on_select_src_file_set),
                       0);
    }
    
    {
      GtkWidget * const subsampling_hbox = gtk_hbox_new(FALSE, 0);
      gtk_box_pack_start(GTK_BOX(src_vbox), subsampling_hbox, FALSE, FALSE, 0);
      
      GtkWidget * const frame = gtk_frame_new("Source subsampling");
      gtk_box_pack_start(GTK_BOX(subsampling_hbox), frame, TRUE, TRUE, 0);
      
      GtkWidget * const frame_vbox = gtk_vbox_new(FALSE, 0);
      gtk_container_add(GTK_CONTAINER(frame), frame_vbox);
      
      GtkWidget * const select_hbox = gtk_hbox_new(FALSE, 0);
      gtk_box_pack_start(GTK_BOX(frame_vbox), select_hbox, FALSE, FALSE, 0);
      
      GtkWidget * const label = gtk_label_new("Select:");
      gtk_box_pack_start(GTK_BOX(select_hbox), label, FALSE, FALSE, 0);
      
      {
        GtkTreeModel * const src_subsampling_model = create_subsampling_model();
        GtkWidget * const src_combo = gtk_combo_box_new_with_model(src_subsampling_model);
        g_object_unref(src_subsampling_model);
        gtk_box_pack_start(GTK_BOX(select_hbox), src_combo, TRUE, TRUE, 0);
        
        mp_src_info->mp_subsampling_combo = GTK_COMBO_BOX(src_combo);
        
        GtkCellRenderer * const renderer = gtk_cell_renderer_text_new();
        
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(src_combo),
                                   renderer,
                                   TRUE);
        gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(src_combo),
                                       renderer,
                                       "text", 0,
                                       NULL);
        gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(src_combo),
                                           renderer,
                                           set_combo_sensitive,
                                           NULL, NULL);
        
        g_signal_connect(G_OBJECT(src_combo), "changed",
                         G_CALLBACK(on_subsampling_combo_changed),
                         mp_src_info);
      }
      
      GtkWidget * const fullname_hbox = gtk_hbox_new(FALSE, 0);
      gtk_box_pack_start(GTK_BOX(frame_vbox), fullname_hbox, FALSE, FALSE, 0);
      
      GtkWidget * const fullname_label = gtk_label_new("Fullname:");
      gtk_box_pack_start(GTK_BOX(fullname_hbox), fullname_label, FALSE, FALSE, 0);
      
      mp_src_info->mp_fullname = GTK_ENTRY(gtk_entry_new());
      gtk_box_pack_start(GTK_BOX(fullname_hbox),
                         GTK_WIDGET(mp_src_info->mp_fullname),
                         TRUE, TRUE, 0);
      
      gtk_editable_set_editable(GTK_EDITABLE(mp_src_info->mp_fullname), FALSE);
    }
    
    {
      GtkWidget * const width_hbox = gtk_hbox_new(FALSE, 0);
      gtk_box_pack_start(GTK_BOX(src_vbox), width_hbox, FALSE, FALSE, 0);
      
      {
        GtkWidget * const alignment = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
        gtk_box_pack_start(GTK_BOX(width_hbox), alignment, FALSE, FALSE, 0);
        
        GtkWidget * const label = gtk_label_new("Source width:");
        gtk_container_add(GTK_CONTAINER(alignment), label);
      }
      
      {
        GtkObject * const adjustment = gtk_adjustment_new(MIN_SOURCE_WIDTH,
                                                          MIN_SOURCE_WIDTH,
                                                          MAX_SOURCE_WIDTH,
                                                          1.0, 1.0, 1.0);
        GtkWidget * const spin = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 1.0, 0);
        gtk_box_pack_start(GTK_BOX(width_hbox), spin, FALSE, FALSE, 0);
        
        mp_src_info->mp_width_spin = GTK_SPIN_BUTTON(spin);
      }
    }
    
    {
      GtkWidget * const height_hbox = gtk_hbox_new(FALSE, 0);
      gtk_box_pack_start(GTK_BOX(src_vbox), height_hbox, FALSE, FALSE, 0);
      
      {
        GtkWidget * const alignment = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
        gtk_box_pack_start(GTK_BOX(height_hbox), alignment, FALSE, FALSE, 0);
        
        GtkWidget * const label = gtk_label_new("Source height:");
        gtk_container_add(GTK_CONTAINER(alignment), label);
      }
      
      {
        GtkObject * const adjustment = gtk_adjustment_new(MIN_SOURCE_HEIGHT,
                                                          MIN_SOURCE_HEIGHT,
                                                          MAX_SOURCE_HEIGHT,
                                                          1.0, 1.0, 1.0);
        GtkWidget * const spin = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 1.0, 0);
        gtk_box_pack_start(GTK_BOX(height_hbox), spin, FALSE, FALSE, 0);        
        
        mp_src_info->mp_height_spin = GTK_SPIN_BUTTON(spin);
      }
    }
    
    {
      GtkWidget * const preview_frame = gtk_frame_new("Preview");
      gtk_box_pack_start(GTK_BOX(src_vbox), preview_frame, TRUE, TRUE, 0);
      
      GtkWidget * const preview_vbox = gtk_vbox_new(FALSE, 0);
      gtk_container_add(GTK_CONTAINER(preview_frame), preview_vbox);
      
      GtkWidget * const preview_area = gtk_drawing_area_new();
      gtk_box_pack_start(GTK_BOX(preview_vbox), preview_area, TRUE, TRUE, 0);      
      
      mp_src_info->mp_preview_area = preview_area;
      
      {
        gtk_widget_add_events(preview_area, GDK_EXPOSURE_MASK | GDK_STRUCTURE_MASK);
        g_signal_connect(G_OBJECT(preview_area), "configure_event", G_CALLBACK(on_preview_area_configure_event), mp_src_info);
        g_signal_connect(G_OBJECT(preview_area), "expose_event", G_CALLBACK(on_preview_area_expose_event), mp_src_info);
      }
      
      GtkWidget * const preview_button = gtk_button_new_with_label("Generate preview");
      gtk_box_pack_start(GTK_BOX(preview_vbox), preview_button, FALSE, FALSE, 0);
      
      mp_src_info->mp_progress_bar = GTK_PROGRESS_BAR(gtk_progress_bar_new());
      gtk_box_pack_start(GTK_BOX(preview_vbox), GTK_WIDGET(mp_src_info->mp_progress_bar), FALSE, FALSE, 0);
      
      g_signal_connect(G_OBJECT(preview_button), "clicked",
                       G_CALLBACK(on_preview_button_clicked),
                       mp_src_info);
      
      // I use "Custom::YUV444" dest subsampling plugin to
      // get the YUV444 buffer from the source image.
      {
        std::list<std::wstring> dst_names;
        dst_names.push_back(L"Custom");
        dst_names.push_back(L"YUV444");
        
        mp_src_info->mp_preview_area_subsampling_converter->assign_dst_subsampling(dst_names);
      }
      
      // And then, I use "Custom::RGB" dest color space plugin to
      // get the RGB buffer from the source image YUV444 buffer.
      {
        std::list<std::wstring> dst_names;
        dst_names.push_back(L"Custom");
        dst_names.push_back(L"RGB");
        
        mp_src_info->mp_preview_area_colorspace_converter->assign_dst_color_space(dst_names);
      }
    }
    
    {
      GtkWidget * const save_to_bmp_frame = gtk_frame_new("Save to bmp");
      gtk_box_pack_start(GTK_BOX(src_vbox), save_to_bmp_frame, FALSE, FALSE, 0);
      
      GtkWidget * const save_to_bmp_vbox = gtk_vbox_new(FALSE, 0);
      gtk_container_add(GTK_CONTAINER(save_to_bmp_frame), save_to_bmp_vbox);
      
      GtkWidget * const bmp_file_path_hbox = gtk_hbox_new(FALSE, 0);
      gtk_box_pack_start(GTK_BOX(save_to_bmp_vbox), bmp_file_path_hbox, FALSE, FALSE, 0);      
      
      GtkWidget * const select_file_button = gtk_button_new_with_label("Select file");
      gtk_box_pack_start(GTK_BOX(bmp_file_path_hbox), select_file_button, FALSE, FALSE, 0);
      
      g_signal_connect(G_OBJECT(select_file_button), "clicked",
                       G_CALLBACK(on_select_file_button_clicked),
                       mp_src_info);
      
      GtkWidget * const file_name_entry = gtk_entry_new();
      gtk_box_pack_start(GTK_BOX(bmp_file_path_hbox), file_name_entry, TRUE, TRUE, 0);      
      
      gtk_editable_set_editable(GTK_EDITABLE(file_name_entry), FALSE);
      
      mp_src_info->mp_preview_area_bmp_filename_entry = file_name_entry;
      
      GtkWidget * const file_save_button = gtk_button_new_with_label("save to bmp");
      gtk_box_pack_start(GTK_BOX(save_to_bmp_vbox), file_save_button, FALSE, FALSE, 0);
      
      g_signal_connect(G_OBJECT(file_save_button), "clicked",
                       G_CALLBACK(on_preview_file_save_button_clicked),
                       mp_src_info);
    }
  }
  
  // Create conversion button UI
  {
    GtkWidget * const convert_button = gtk_button_new();
    gtk_box_pack_start(GTK_BOX(conversion_hbox), convert_button, FALSE, FALSE, 0);
    
    GtkWidget * const arrow = gtk_image_new_from_stock("gtk-go-forward", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(convert_button), arrow);
    
    g_signal_connect(G_OBJECT(convert_button), "clicked",
                     G_CALLBACK(on_convert_button_clicked), NULL);
  }
  
  // create dst subsampling UI
  {
    GtkWidget * const dst_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(conversion_hbox), dst_scrolled_window, TRUE, TRUE, 0);
    
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(dst_scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    
    GtkWidget * const dst_vbox = gtk_vbox_new(FALSE, 0);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(dst_scrolled_window), dst_vbox);
    
    GtkWidget * const each_dst_ui = create_dst_subsampling_ui();
    gtk_box_pack_start(GTK_BOX(dst_vbox), each_dst_ui, FALSE, FALSE, 0);
    
    g_signal_connect(G_OBJECT(spin), "value-changed",
                     G_CALLBACK(on_dst_subsampling_count_spin_value_changed),
                     dst_vbox);
  }
  
  return outer_box;
}
