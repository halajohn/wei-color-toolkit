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

#include "wcl_colorlib/colorlib.hpp"

#include "mainwnd.hpp"
#include "option.hpp"

int
main(int argc, char **argv)
{
  Wcl::Colorlib::init();
  
  // Initialize i18n support
  gtk_set_locale();
  
  // Initialize the widget set
  gtk_init(&argc, &argv);
  
  create_top_level_wnd();
  
  // Enter the main event loop, and wait for user interaction
  gtk_main();
  
  // The user lost interest
  return 0;
}
