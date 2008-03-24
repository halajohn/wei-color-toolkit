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

#include "gtk_combo_box_hack.hpp"
#include "gtk_tree_model_hack.hpp"

#define MAX_DST_COLOR_SPACE_COUNT 10.0
#define MIN_DST_COLOR_SPACE_COUNT 1.0

namespace
{
  // ===================================================
  //                  Color Space Info
  // ===================================================
  enum ColorSpaceInfoType
  {
    COLOR_SPACE_INFO_TYPE_SRC,
    COLOR_SPACE_INFO_TYPE_DST
  };
  typedef enum ColorSpaceInfoType ColorSpaceInfoType;
  
  struct ColorSpaceInfo
  {
  protected:
    
    ColorSpaceInfo(ColorSpaceInfoType const type)
      : m_type(type),
        mp_container_box(0),
        active_index(-1)
    {
      gtk_tree_model_iter_invalid(&active_iter);
    }
    
  public:
    
    virtual ~ColorSpaceInfo() {}
    
    ColorSpaceInfoType m_type;
    GtkBox *mp_container_box;
    
    GtkComboBox *mp_combo;
    GtkEntry *mp_fullname;
    GtkTreeIter active_iter;
    gint active_index;
    
    virtual Wcl::Colorlib::ColorSpaceConverter *converter() const = 0;
  };
  typedef struct ColorSpaceInfo ColorSpaceInfo;
  
  // ===================================================
  //               Src Color Space Info
  // ===================================================
  struct SrcColorSpaceInfo : public ColorSpaceInfo
  {
    SrcColorSpaceInfo()
      : ColorSpaceInfo(COLOR_SPACE_INFO_TYPE_SRC)
    { }
    
    Wcl::Colorlib::ColorSpaceConverter *converter() const
    { return 0; }
  };
  typedef struct SrcColorSpaceInfo SrcColorSpaceInfo;
  
  // ===================================================
  //               Dst Color Space Info
  // ===================================================
  struct DstColorSpaceInfo : public ColorSpaceInfo
  {
    Wcl::Colorlib::ColorSpaceConverter *mp_converter;
    
    DstColorSpaceInfo()
      : ColorSpaceInfo(COLOR_SPACE_INFO_TYPE_DST)
    {
      mp_converter = ::new Wcl::Colorlib::ColorSpaceConverter;
    }
    
    ~DstColorSpaceInfo()
    {
      assert(mp_converter != 0);
      
      ::delete mp_converter;
    }
    
    void set_container_box(GtkBox * const container_box)
    { mp_container_box = container_box; }
    
    Wcl::Colorlib::ColorSpaceConverter *converter() const
    { return mp_converter; }
  };
  typedef struct DstColorSpaceInfo DstColorSpaceInfo;
  
  SrcColorSpaceInfo *mp_src_info;
  std::list<DstColorSpaceInfo *> mp_dst_info;
  
  // ===================================================
  //              Convert pixel page codes
  // ===================================================
  GtkTreeModel *
  create_color_space_model()
  {
    GtkTreeStore * const color_space_model = gtk_tree_store_new(1, G_TYPE_STRING);
    
    Wcl::Colorlib::PluginTreeNode<Wcl::Colorlib::ColorSpaceConverterPlugin> * const root =
      Wcl::Colorlib::ColorSpaceConverter::get_plugin_tree_root();
    
    std::list<GtkTreeIter> stack;
    
    // Push a dummy GtkTreeIter to represent the init plugin
    // tree node, so that the pop_back() codes below can pop
    // up one node when I return to the init plugin tree
    // node.
    stack.push_back(GtkTreeIter());
    
    for (Wcl::Colorlib::PluginTreeNode<Wcl::Colorlib::ColorSpaceConverterPlugin>::iterator plugin_tree_iter = root->begin();
         plugin_tree_iter != root->end();
         ++plugin_tree_iter)
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
      
        gtk_tree_store_append(GTK_TREE_STORE(color_space_model), curr_iter, group_iter);
      
        char *plugin_name = fmtstr_wcstombs((*plugin_tree_iter)->name().c_str(),
                                            (*plugin_tree_iter)->name().size());
        assert(plugin_name != 0);
      
        gtk_tree_store_set(GTK_TREE_STORE(color_space_model), curr_iter,
                           0, plugin_name,
                           -1);
      
        fmtstr_delete(plugin_name);
      }
    }
    
    return GTK_TREE_MODEL(color_space_model);
  }
    
  void
  on_color_space_combo_changed(GtkComboBox *widget,
                               gpointer     user_data)
  {
    GtkTreeIter iter;

    if (TRUE == gtk_combo_box_get_active_iter(widget, &iter))
    {
      // I have select a different source color space.
      
      ColorSpaceInfo * const color_space_info = static_cast<ColorSpaceInfo *>(user_data);
      assert(color_space_info != 0);
      
      GtkBox * const subcomponent_vbox = color_space_info->mp_container_box;
      assert(subcomponent_vbox != 0);
      
      GtkTreeModel * const model = gtk_combo_box_get_model(widget);
      
      std::list<std::wstring> color_space_plugin_name_list;
      gchar *fullname;
      
      // Add parent color space name list to the active
      // tree model entry.
      gtk_combo_box_collect_parent_node_name_list(model, &iter, &color_space_plugin_name_list, &fullname);
      
      // If the modified color space combo is the source
      // color space combo, then I have to update the
      // destination color space info.
      //
      // 1) If there exists at least one dst color space
      // info, then I have to update the converter's source
      // plugin info contained in them.
      //
      // 2) If there exists no dst color space info, I
      // have to create a dst color space info and put it
      // into mp_dst_info.
      uint32_t subcomponent_count;
      
      switch (color_space_info->m_type)
      {
      case COLOR_SPACE_INFO_TYPE_SRC:
        // The change combo is a source color space combo.
        assert(mp_dst_info.size() != 0);
        
        {
          Wcl::Colorlib::ColorSpaceConverterPlugin *plugin;
          
          BOOST_FOREACH(DstColorSpaceInfo * const dst_info, mp_dst_info)
          {
            assert(dst_info->converter() != 0);
            
            try
            {
              dst_info->converter()->assign_src_color_space(color_space_plugin_name_list);
              plugin = dst_info->converter()->get_src_plugin();
            }
            catch (Wcl::Colorlib::SrcAndDstColorSpaceConverterPluginCanNotBeDiffCustomAtTheSameTime &)
            {
              GtkWidget * const dialog = gtk_message_dialog_new(
                GTK_WINDOW(mainwnd),
                GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                "If the Source and target color space types are all belong to the \"Custom\" type,\
 then they can not be different one.");
              gtk_dialog_run(GTK_DIALOG(dialog));
              gtk_widget_destroy(dialog);
            
              // Restore the combo box to the previous selected one.
              if (-1 == color_space_info->active_index)
              {
                gtk_combo_box_set_active(widget, -1);
              }
              else
              {
                gtk_combo_box_set_active_iter(widget, &(color_space_info->active_iter));
              }
            }
          }
          
          subcomponent_count = plugin->get_subcomponent_count();
        }
        break;
        
      case COLOR_SPACE_INFO_TYPE_DST:
        {
          assert(color_space_info->converter() != 0);
          
          try
          {
            color_space_info->converter()->assign_dst_color_space(color_space_plugin_name_list);
          }
          catch (Wcl::Colorlib::SrcAndDstColorSpaceConverterPluginCanNotBeDiffCustomAtTheSameTime &)
          {
            GtkWidget * const dialog = gtk_message_dialog_new(
              GTK_WINDOW(mainwnd),
              GTK_DIALOG_DESTROY_WITH_PARENT,
              GTK_MESSAGE_ERROR,
              GTK_BUTTONS_CLOSE,
              "If the Source and target color space types are all belong to the \"Custom\" type,\
 then they can not be different one.");
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            
            // Restore the combo box to the previous selected one.
            if (-1 == color_space_info->active_index)
            {
              gtk_combo_box_set_active(widget, -1);
            }
            else
            {
              gtk_combo_box_set_active_iter(widget, &(color_space_info->active_iter));
            }
            return;
          }
          
          subcomponent_count =
            color_space_info->converter()->get_dst_plugin()->get_subcomponent_count();
        }
        break;
        
      default:
        assert(0);
        subcomponent_count = 0;
        break;
      }
      
      color_space_info->active_iter = iter;
      color_space_info->active_index = gtk_combo_box_get_active(widget);
      
      gtk_entry_set_text(color_space_info->mp_fullname, fullname);
      
      g_free(fullname);
      
      // remove the old content in the subcomponent vbox
      {
        GList * const list = gtk_container_get_children(GTK_CONTAINER(subcomponent_vbox));
        
        if (list != 0)
        {
          // This container DO have children.
          
          assert(0 == list->prev);
          
          for (GList *curr = list; curr != 0; curr = curr->next)
          {
            assert(curr->data != 0);
            
            gtk_widget_destroy(GTK_WIDGET(curr->data));
          }
        }
      }
      
      // Add new content in the subcomponet vbox
      {
        for (uint32_t i = 0; i < subcomponent_count; ++i)
        {
          GtkWidget * const sb = gtk_entry_new();
          
          switch (color_space_info->m_type)
          {
          case COLOR_SPACE_INFO_TYPE_DST:
            gtk_editable_set_editable(GTK_EDITABLE(sb), FALSE);
            break;
            
          case COLOR_SPACE_INFO_TYPE_SRC:
            break;
            
          default:
            assert(0);
            break;
          }
          
          gtk_box_pack_start(subcomponent_vbox, sb, FALSE, FALSE, 0);
        }
      }
      
      gtk_widget_show_all(GTK_WIDGET(subcomponent_vbox));
    }
  }
    
  void
  on_convert_button_clicked(GtkButton * /* widget */,
                            gpointer   /* user_data */)
  {
    assert(mp_dst_info.size() != 0);
    assert(mp_dst_info.front() != 0);
    assert(mp_dst_info.front()->converter() != 0);
    
    Wcl::Colorlib::ColorSpaceConverterPlugin * const src_plugin =
      mp_dst_info.front()->converter()->get_src_plugin();
    
    if (0 == src_plugin)
    {
      GtkWidget * const dialog = gtk_message_dialog_new(
        GTK_WINDOW(mainwnd),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_CLOSE,
        "You does not specify the source color space.");
      gtk_dialog_run(GTK_DIALOG(dialog));
      gtk_widget_destroy(dialog);
    }
    else
    {
      std::vector<Wcl::Colorlib::ColorSpaceBasicUnitValue> src_data;
      std::vector<Wcl::Colorlib::ColorSpaceBasicUnitValue> dst_data;
      
      bool success_read_input = true;
      
      assert(mp_src_info->mp_container_box != 0);
      
      GList *src_list = gtk_container_get_children(
        GTK_CONTAINER(mp_src_info->mp_container_box));
      
      Wcl::Colorlib::ColorSpaceConverter * const converter = mp_dst_info.front()->converter();
      
      uint32_t idx = 0;
      
      while (src_list != 0)
      {
        assert(src_list->data != 0);
        
        // Strange things happened !!!
        //
        // According to the spec, the type of 'src_list->data'
        // should be GtkBoxChild, however, the real type of it
        // is the attached widget.
#if 0
        GtkBoxChild * const child = static_cast<GtkBoxChild *>(src_list->data);
        assert(child->widget != 0);
        GtkEntry * const entry = GTK_ENTRY(child->widget);
#else
        GtkEntry * const entry = GTK_ENTRY(src_list->data);
#endif
      
        gchar const * const src_number = gtk_entry_get_text(entry);
        
        Wcl::Colorlib::ColorSpaceBasicUnitValue data;
        
        if (0 == src_number)
        {
          GtkWidget * const dialog = gtk_message_dialog_new(GTK_WINDOW(mainwnd),
                                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                                            GTK_MESSAGE_ERROR,
                                                            GTK_BUTTONS_CLOSE,
                                                            "Empty input data at input index %d",
                                                            idx + 1);
          gtk_dialog_run(GTK_DIALOG(dialog));
          gtk_widget_destroy(dialog);
        
          success_read_input = false;
        
          break;
        }
      
        try
        {
          switch (converter->get_input_data_basic_unit_value_type())
          {
          case Wcl::Colorlib::COLOR_SPACE_BASIC_UNIT_VALUE_TYPE_DOUBLE:
            data.set_value(boost::lexical_cast<double_t>(src_number));
            break;
            
          case Wcl::Colorlib::COLOR_SPACE_BASIC_UNIT_VALUE_TYPE_UINT8:
            {
              uint32_t const value_32 = boost::lexical_cast<uint32_t>(src_number);
              
              try
              {
                data.set_value(boost::numeric_cast<uint8_t>(value_32));
              }
              catch (boost::bad_numeric_cast &)
              {
                GtkWidget * const dialog = gtk_message_dialog_new(GTK_WINDOW(mainwnd),
                                                                  GTK_DIALOG_DESTROY_WITH_PARENT,
                                                                  GTK_MESSAGE_ERROR,
                                                                  GTK_BUTTONS_CLOSE,
                                                                  "Input data %d is out of range",
                                                                  (idx + 1));
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
                
                success_read_input = false;
                break;
              }
            }
            break;
            
          default:
            assert(0);
            break;
          }
        }
        catch (boost::bad_lexical_cast &)
        {
          GtkWidget * const dialog = gtk_message_dialog_new(GTK_WINDOW(mainwnd),
                                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                                            GTK_MESSAGE_ERROR,
                                                            GTK_BUTTONS_CLOSE,
                                                            "Invalid input data: %s at input index %d",
                                                            src_number, idx + 1);
          gtk_dialog_run(GTK_DIALOG(dialog));
          gtk_widget_destroy(dialog);
        
          success_read_input = false;
          
          break;
        }
      
        try
        {
          converter->check_valid_input_value(idx, data);
        }
        catch (Wcl::Colorlib::InvalidInputValueException &)
        {
          GtkWidget * const dialog = gtk_message_dialog_new(GTK_WINDOW(mainwnd),
                                                            GTK_DIALOG_DESTROY_WITH_PARENT,
                                                            GTK_MESSAGE_ERROR,
                                                            GTK_BUTTONS_CLOSE,
                                                            "Input data %d is out of range",
                                                            (idx + 1));
          gtk_dialog_run(GTK_DIALOG(dialog));
          gtk_widget_destroy(dialog);
          
          success_read_input = false;
          break;
        }
        
        src_data.push_back(data);
        
        ++idx;
        
        src_list = src_list->next;
      }
      
      if (true == success_read_input)
      {
        BOOST_FOREACH(DstColorSpaceInfo * const dst_info, mp_dst_info)
        {
          assert(dst_info->converter() != 0);
        
          // Check if I specify the dst color space plugin.
          if (dst_info->converter()->get_dst_plugin() != 0)
          {
            // =======================================
            //         Perform the conversion
            // =======================================
            dst_info->converter()->convert(src_data, dst_data);
            
            std::vector<Wcl::Colorlib::ColorSpaceBasicUnitValue>::const_iterator iter = dst_data.begin();
            
            GList *dst_list = gtk_container_get_children(
              GTK_CONTAINER(dst_info->mp_container_box));
            
            while (dst_list != 0)
            {
              assert(dst_list->data != 0);
              
              try
              {
                std::string output_string;
                
                switch (converter->get_output_data_basic_unit_value_type())
                {
                case Wcl::Colorlib::COLOR_SPACE_BASIC_UNIT_VALUE_TYPE_DOUBLE:
                  output_string = boost::lexical_cast<std::string>((*iter).get_value<double_t>());
                  break;
                  
                case Wcl::Colorlib::COLOR_SPACE_BASIC_UNIT_VALUE_TYPE_UINT8:
                  {
                    uint32_t const value = static_cast<uint32_t>((*iter).get_value<uint8_t>());
                    output_string = boost::lexical_cast<std::string>(value);
                  }
                  break;
                  
                default:
                  assert(0);
                  break;
                }
                
                // Strange things happened !!!
                //
                // According to the spec, the type of 'src_list->data'
                // should be GtkBoxChild, however, the real type of it
                // is the attached widget.
#if 0
                GtkBoxChild * const child = static_cast<GtkBoxChild *>(dst_list->data);
                assert(child->widget != 0);
                GtkEntry * const entry = GTK_ENTRY(child->widget);
#else
                GtkEntry * const entry = GTK_ENTRY(dst_list->data);
#endif
                
                gtk_entry_set_text(entry, output_string.c_str());
              }
              catch (boost::bad_lexical_cast &)
              {
                assert(0);
              }
              
              dst_list = dst_list->next;
              ++iter;
            }
        
            assert(iter == dst_data.end());
          
            dst_data.clear();
          }
        }
      }
    }
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
  create_dst_color_space_ui()
  {
    // Create dst color space info
    {
      mp_dst_info.push_back(::new DstColorSpaceInfo());
      
      Wcl::Colorlib::ColorSpaceConverterPlugin * const plugin =
        mp_dst_info.front()->converter()->get_src_plugin();
      
      if (plugin != 0)
      {
        mp_dst_info.back()->converter()->assign_src_plugin(
          mp_dst_info.front()->converter()->get_src_plugin_info());
      }
    }
    
    GtkWidget * const each_hbox = gtk_hbox_new(FALSE, 0);
    
    {
      GtkWidget * const chooser_vbox = gtk_vbox_new(FALSE, 0);
      gtk_box_pack_start(GTK_BOX(each_hbox), chooser_vbox, TRUE, TRUE, 0);
      
      GtkWidget * const dst_color_space_frame = gtk_frame_new("Target color space");
      gtk_box_pack_start(GTK_BOX(chooser_vbox), dst_color_space_frame, FALSE, FALSE, 0);
    
      GtkWidget * const dst_color_space_frame_hbox = gtk_hbox_new(FALSE, 0);
      gtk_container_add(GTK_CONTAINER(dst_color_space_frame), dst_color_space_frame_hbox);
    
      GtkWidget * const dst_color_space_chooser_vbox = gtk_vbox_new(FALSE, 0);
      gtk_box_pack_start(GTK_BOX(dst_color_space_frame_hbox), dst_color_space_chooser_vbox, TRUE, TRUE, 0);
    
      GtkWidget * const dst_color_space_select_hbox = gtk_hbox_new(FALSE, 0);
      gtk_box_pack_start(GTK_BOX(dst_color_space_chooser_vbox), dst_color_space_select_hbox, FALSE, FALSE, 0);
    
      GtkWidget * const label = gtk_label_new("Select:");
      gtk_box_pack_start(GTK_BOX(dst_color_space_select_hbox), label, FALSE, FALSE, 0);
    
      {
        GtkTreeModel * const dst_color_space_model = create_color_space_model();
        mp_dst_info.back()->mp_combo = GTK_COMBO_BOX(gtk_combo_box_new_with_model(dst_color_space_model));
        g_object_unref(dst_color_space_model);
        gtk_box_pack_start(GTK_BOX(dst_color_space_select_hbox), GTK_WIDGET(mp_dst_info.back()->mp_combo), TRUE, TRUE, 0);
            
        GtkCellRenderer * const renderer = gtk_cell_renderer_text_new();
      
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(mp_dst_info.back()->mp_combo),
                                   renderer,
                                   TRUE);
        gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(mp_dst_info.back()->mp_combo),
                                       renderer,
                                       "text", 0,
                                       NULL);
        gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(mp_dst_info.back()->mp_combo),
                                           renderer,
                                           set_combo_sensitive,
                                           NULL, NULL);
      
        {
          GtkWidget * const dst_color_space_fullname_hbox = gtk_hbox_new(FALSE, 0);
          gtk_box_pack_start(GTK_BOX(dst_color_space_chooser_vbox), dst_color_space_fullname_hbox, FALSE, FALSE, 0);
          
          GtkWidget * const label = gtk_label_new("Full name:");
          gtk_box_pack_start(GTK_BOX(dst_color_space_fullname_hbox), label, FALSE, FALSE, 0);
          
          mp_dst_info.back()->mp_fullname = GTK_ENTRY(gtk_entry_new());
          gtk_box_pack_start(GTK_BOX(dst_color_space_fullname_hbox),
                             GTK_WIDGET(mp_dst_info.back()->mp_fullname),
                             TRUE, TRUE, 0);
          
          gtk_editable_set_editable(GTK_EDITABLE(mp_dst_info.back()->mp_fullname), FALSE);
        }
      
        {
          mp_dst_info.back()->mp_container_box = GTK_BOX(gtk_vbox_new(FALSE, 0));
        
          gtk_box_pack_start(GTK_BOX(dst_color_space_frame_hbox),
                             GTK_WIDGET(mp_dst_info.back()->mp_container_box),
                             FALSE, FALSE, 0);
        }
      
        g_signal_connect(G_OBJECT(mp_dst_info.back()->mp_combo), "changed",
                         G_CALLBACK(on_color_space_combo_changed), mp_dst_info.back());
      }
    }
    
    return each_hbox;
  }
  
  void
  on_dst_color_space_count_spin_value_changed(GtkSpinButton * const spin,
                                              gpointer user_data)
  {
    uint32_t desired_count;
    
    {
      gint const desired_count_gtk = gtk_spin_button_get_value_as_int(spin);
      
      desired_count = boost::numeric_cast<uint32_t>(desired_count_gtk);
    }
    
    assert((desired_count >= static_cast<gint>(MIN_DST_COLOR_SPACE_COUNT)) &&
           (desired_count <= static_cast<gint>(MAX_DST_COLOR_SPACE_COUNT)));
    
    std::list<DstColorSpaceInfo *>::size_type const curr_count = mp_dst_info.size();
    
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
      GtkWidget * const each_dst_ui = create_dst_color_space_ui();
      gtk_box_pack_start(GTK_BOX(dst_vbox), each_dst_ui, FALSE, FALSE, 0);
      
      gtk_widget_show_all(GTK_WIDGET(each_dst_ui));
    }
  }
}

// Main function for creating color space page.
GtkWidget *
create_color_space_page()
{
  GtkWidget * const outer_box = gtk_vbox_new(FALSE, 0);
  
  GtkWidget *spin;
  
  // Create dst color space _count_ UI
  {
    GtkWidget * const alignment = gtk_alignment_new(1.0, 0.0, 0.0, 0.0);
    gtk_box_pack_start(GTK_BOX(outer_box), alignment, FALSE, FALSE, 0);
    
    GtkWidget * const hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(alignment), hbox);
    
    GtkWidget * const label = gtk_label_new("Dst color space count:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    
    GtkObject * const adjustment = gtk_adjustment_new(MIN_DST_COLOR_SPACE_COUNT,
                                                      MIN_DST_COLOR_SPACE_COUNT,
                                                      MAX_DST_COLOR_SPACE_COUNT,
                                                      1.0, 1.0, 1.0);
    spin = gtk_spin_button_new(GTK_ADJUSTMENT(adjustment), 1.0, 0);
    gtk_box_pack_start(GTK_BOX(hbox), spin, FALSE, FALSE, 0);
    
    gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin), GTK_UPDATE_IF_VALID);
  }
  
  GtkWidget * const conversion_hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(outer_box), conversion_hbox, TRUE, TRUE, 0);
  
  // Create source color space UI
  {
    mp_src_info = ::new SrcColorSpaceInfo();
    
    GtkWidget * const chooser_vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(conversion_hbox), chooser_vbox, TRUE, TRUE, 0);
    
    GtkWidget * const src_color_space_frame = gtk_frame_new("Source color space");
    gtk_box_pack_start(GTK_BOX(chooser_vbox), src_color_space_frame, FALSE, FALSE, 0);
    
    GtkWidget * const src_color_space_frame_hbox = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(src_color_space_frame), src_color_space_frame_hbox);
    
    GtkWidget * const src_color_space_chooser_vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(src_color_space_frame_hbox), src_color_space_chooser_vbox, TRUE, TRUE, 0);
    
    GtkWidget * const src_color_space_select_hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(src_color_space_chooser_vbox), src_color_space_select_hbox, FALSE, FALSE, 0);
    
    GtkWidget * const label = gtk_label_new("Select:");
    gtk_box_pack_start(GTK_BOX(src_color_space_select_hbox), label, FALSE, FALSE, 0);
    
    {
      GtkTreeModel * const src_color_space_model = create_color_space_model();
      mp_src_info->mp_combo = GTK_COMBO_BOX(gtk_combo_box_new_with_model(src_color_space_model));
      g_object_unref(src_color_space_model);
      gtk_box_pack_start(GTK_BOX(src_color_space_select_hbox), GTK_WIDGET(mp_src_info->mp_combo), TRUE, TRUE, 0);
            
      GtkCellRenderer * const renderer = gtk_cell_renderer_text_new();
      
      gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(mp_src_info->mp_combo),
                                 renderer,
                                 TRUE);
      gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(mp_src_info->mp_combo),
                                     renderer,
                                     "text", 0,
                                     NULL);
      gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(mp_src_info->mp_combo),
                                         renderer,
                                         set_combo_sensitive,
                                         NULL, NULL);
      
      {
        GtkWidget * const src_color_space_fullname_hbox = gtk_hbox_new(FALSE, 0);
        gtk_box_pack_start(GTK_BOX(src_color_space_chooser_vbox), src_color_space_fullname_hbox, FALSE, FALSE, 0);
        
        GtkWidget * const label = gtk_label_new("Full name:");
        gtk_box_pack_start(GTK_BOX(src_color_space_fullname_hbox), label, FALSE, FALSE, 0);
        
        mp_src_info->mp_fullname = GTK_ENTRY(gtk_entry_new());
        gtk_box_pack_start(GTK_BOX(src_color_space_fullname_hbox),
                           GTK_WIDGET(mp_src_info->mp_fullname),
                           TRUE, TRUE, 0);
        
        gtk_editable_set_editable(GTK_EDITABLE(mp_src_info->mp_fullname), FALSE);
      }
      
      {
        mp_src_info->mp_container_box = GTK_BOX(gtk_vbox_new(FALSE, 0));
        
        gtk_box_pack_start(GTK_BOX(src_color_space_frame_hbox),
                           GTK_WIDGET(mp_src_info->mp_container_box),
                           FALSE, FALSE, 0);
      }
      
      g_signal_connect(G_OBJECT(mp_src_info->mp_combo), "changed",
                       G_CALLBACK(on_color_space_combo_changed), mp_src_info);
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
  
  // create dst color space UI
  {
    GtkWidget * const dst_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(conversion_hbox), dst_scrolled_window, TRUE, TRUE, 0);
    
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(dst_scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    
    GtkWidget * const dst_vbox = gtk_vbox_new(FALSE, 0);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(dst_scrolled_window), dst_vbox);
    
    GtkWidget * const each_dst_ui = create_dst_color_space_ui();
    gtk_box_pack_start(GTK_BOX(dst_vbox), each_dst_ui, FALSE, FALSE, 0);
    
    g_signal_connect(G_OBJECT(spin), "value-changed",
                     G_CALLBACK(on_dst_color_space_count_spin_value_changed),
                     dst_vbox);
  }
  
  return outer_box;
}
