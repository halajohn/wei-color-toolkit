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
gtk_tree_model_iter_invalid(GtkTreeIter * const iter)
{
  iter->stamp = 0;
}

bool
gtk_tree_model_iter_is_valid(GtkTreeIter * const iter)
{
  if (iter->stamp != 0)
  {
    return true;
  }
  else
  {
    return false;
  }
}
