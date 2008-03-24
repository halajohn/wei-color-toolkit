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

void
gtk_combo_box_collect_parent_node_name_list(
  GtkTreeModel * const model,
  GtkTreeIter * const iter,
  std::list<std::wstring> * const name_list,
  gchar ** const fullname)
{
  gchar *name;
  
  gtk_tree_model_get(model, iter, 0, &name, -1);
  assert(name != 0);
  
  if (name_list != 0)
  {
    wchar_t * const name_wide = fmtstr_mbstowcs(name, 0);
    
    name_list->push_front(name_wide);
    
    fmtstr_delete(name_wide);
  }
  
  {
    // Find all parent iterators of this active iterator,
    // so that I can change the combobox text to something
    // like "RGB::sRGB".
    gchar *full_name;
    
    full_name = g_strdup(name);
    
    GtkTreeIter curr_iter = (*iter);
    GtkTreeIter parent_iter;
    
    for (;;)
    {        
      gboolean const success = gtk_tree_model_iter_parent(model, &parent_iter, &curr_iter);
      if (FALSE == success)
      {
        // Already reach the top level.
        break;
      }
      else
      {
        gchar *parent_name;
        gchar *new_full_name;
        
        gtk_tree_model_get(model, &parent_iter, 0, &parent_name, -1);
        assert(parent_name != 0);
        
        if (name_list != 0)
        {
          wchar_t * const parent_name_wide = fmtstr_mbstowcs(parent_name, 0);
          
          name_list->push_front(parent_name_wide);
          
          fmtstr_delete(parent_name_wide);
        }
        
        new_full_name = g_strjoin("::", parent_name, full_name, NULL);
            
        g_free(full_name);
        full_name = new_full_name;
        
        curr_iter = parent_iter;
      }
    }
    
    if (fullname != 0)
    {
      (*fullname) = g_strdup(full_name);
    }
    else
    {
      g_free(full_name);
    }
  }
}
