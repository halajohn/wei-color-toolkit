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

#include "wcl_curve/wcl_curve.h"
#include "cie_page.hpp"
#include "color_space_page.hpp"
#include "subsampling_page.hpp"
#include "ycbcr_decouple_page.hpp"

GtkWidget *mainwnd = 0;

namespace
{
  gchar const *ui_def_str =
    "  <menubar name='main_menu_bar'>\n"
    "    <menu name='File' action='menu_file_action'>\n"
    "      <menuitem name='Quit' action='menu_quit_action' />\n"
    "    </menu>\n"
    "    <menu name='Help' action='menu_help_action'>\n"
    "      <menuitem name='About' action='menu_about_action' />\n"
    "    </menu>\n"
    "  </menubar>\n";
  
  gchar const * const authors[] = 
  {
    "Wei Hu",
    0
  };
  
  void
  about_this_program(
    GtkAction * /* action */,
    gpointer /* user_data */)
  {
    assert(mainwnd != 0);
    
    gtk_show_about_dialog(GTK_WINDOW(mainwnd),
                          "program-name", "WeiColorToolkit",
                          "website", "http://code.google.com/p/wei-color-toolkit/",
                          "authors", authors,
                          "version", "1.0",
                          "copyright", "Wei Hu <wei.hu.tw@gmail.com>",
                          "license", gpl_v3_str,
                          0);
  }
  
  GtkActionEntry const action_entries[] =
  {
    { "menu_file_action", 0, "_File" },
    { "menu_help_action", 0, "_Help" },
    
    { "menu_quit_action", GTK_STOCK_QUIT, 0, "<control>Q",
      "Quit the application", G_CALLBACK(gtk_main_quit) },
    
    { "menu_about_action", GTK_STOCK_ABOUT, 0, 0,
      "Customise keyboard shortcuts", G_CALLBACK(about_this_program) }
  };
}

void
create_top_level_wnd()
{
  // Create the main window
  mainwnd = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  
  g_signal_connect(G_OBJECT(mainwnd),
                   "destroy", 
                   G_CALLBACK(gtk_main_quit),
                   0);
  
  GtkWidget * const top_level_vbox = gtk_vbox_new(FALSE, 0);
  
  gtk_container_add(GTK_CONTAINER(mainwnd), top_level_vbox);
  
  {
    GtkActionGroup * const action_group = gtk_action_group_new("main_action");
    
    gtk_action_group_add_actions(
      action_group,
      action_entries,
      sizeof(action_entries) / sizeof(action_entries[0]),
      0);
    
    GtkUIManager * const ui = gtk_ui_manager_new();
    
    g_signal_connect_swapped(mainwnd,
                             "destroy",
                             G_CALLBACK(g_object_unref),
                             ui);
    
    gtk_ui_manager_insert_action_group(ui, action_group, 0);
    
    GtkAccelGroup * const accel_group = gtk_ui_manager_get_accel_group(ui);
    
    gtk_window_add_accel_group(GTK_WINDOW(mainwnd), accel_group);
    
    GError *error = 0;
    
    if (0 == gtk_ui_manager_add_ui_from_string(ui, ui_def_str, -1, &error))
    {
      g_message("building menus failed: %s", error->message);
      g_error_free(error);
      
      assert(0);
    }
    
    GtkWidget * const menubar = gtk_ui_manager_get_widget(ui, "/main_menu_bar");
    
    gtk_box_pack_start(GTK_BOX(top_level_vbox), menubar, FALSE, FALSE, 0);
  }
  
  {
    GtkWidget * const notebook = gtk_notebook_new();
    
#if 1
    GtkWidget * const cie_page = create_cie_page();
    GtkWidget * const cie_label = gtk_label_new("CIE");
    
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                             cie_page,
                             cie_label);
#endif
    
    GtkWidget * const color_space_page = create_color_space_page();
    GtkWidget * const color_space_page_label = gtk_label_new("Color Space Conversion");
    
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                             color_space_page,
                             color_space_page_label);
    
    GtkWidget * const subsampling_page = create_subsampling_page();
    GtkWidget * const subsampling_page_label = gtk_label_new("Subsampling Conversion");
    
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                             subsampling_page,
                             subsampling_page_label);
    
    GtkWidget * const ycbcr_decouple_page = create_ycbcr_decouple_page();
    GtkWidget * const ycbcr_decouple_page_label = gtk_label_new("YCbCr Decouple");
    
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                             ycbcr_decouple_page,
                             ycbcr_decouple_page_label);
    
    gtk_box_pack_start(GTK_BOX(top_level_vbox), notebook, TRUE, TRUE, 0);
  }
  
  {
    GtkWidget * const status_bar = gtk_statusbar_new();
    
    gtk_box_pack_start(GTK_BOX(top_level_vbox), status_bar, FALSE, FALSE, 0);
  }
  
  gtk_widget_set_size_request(mainwnd, 800, 600);
  
  // Show the application window
  gtk_widget_show_all(mainwnd);
}
