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
#include "option.hpp"

#include "wcl_registry/registry.hpp"
#include "wcl_colorlib/colorlib.hpp"

#if 0
namespace
{
  template<typename T>
  void
  load_plugin(HKEY const key)
  {
    uint32_t root_dir_str_length;
    
    Wcl::Registry::read_uint32(key,
                               T::PLUGIN_ROOT_DIR_STR_LENGTH_REGISTRY_KEY_NAME,
                               root_dir_str_length);
    
    wchar_t * const root_dir = ::new wchar_t[root_dir_str_length + 1];
    
    Wcl::Registry::read_string(key,
                               T::PLUGIN_ROOT_DIR_REGISTRY_KEY_NAME,
                               root_dir,
                               (root_dir_str_length + 1) * sizeof(wchar_t));
    
    T::load_plugin(root_dir);
    
    ::delete[] root_dir;
  }
}

void
registry_setting_load()
{
  HKEY key;
  
  bool const result = Wcl::Registry::open_key(Wcl::Colorlib::REGISTRY_PARENT_KEY,
                                              Wcl::Colorlib::REGISTRY_KEY_NAME,
                                              Wcl::Registry::MODE_READ,
                                              &key);
  assert(true == result);
  
  // Load color space converter plugin
  load_plugin<Wcl::Colorlib::ColorSpaceConverter>(key);
  
  // Load subsampling converter plugin
  load_plugin<Wcl::Colorlib::SubSamplingConverter>(key);
  
  Wcl::Registry::close_key(key);
}
  
void
registry_setting_save()
{
  HKEY key;
  
  bool const result = Wcl::Registry::open_key(Wcl::Colorlib::REGISTRY_PARENT_KEY,
                                              Wcl::Colorlib::REGISTRY_KEY_NAME,
                                              Wcl::Registry::MODE_WRITE,
                                              &key);
  assert(true == result);
  
  {
    std::wstring const &root_dir = Wcl::Colorlib::ColorSpaceConverter::get_plugin_root_dir();
    size_t root_dir_str_length = root_dir.size();
    
    Wcl::Registry::write_uint32(key,
                                Wcl::Colorlib::ColorSpaceConverter::PLUGIN_ROOT_DIR_STR_LENGTH_REGISTRY_KEY_NAME,
                                root_dir_str_length);
    
    Wcl::Registry::write_string(key,
                                Wcl::Colorlib::ColorSpaceConverter::PLUGIN_ROOT_DIR_REGISTRY_KEY_NAME,
                                root_dir);
  }
  
  {
    std::wstring const &root_dir = Wcl::Colorlib::SubSamplingConverter::get_plugin_root_dir();
    size_t root_dir_str_length = root_dir.size();
    
    Wcl::Registry::write_uint32(key,
                                Wcl::Colorlib::SubSamplingConverter::PLUGIN_ROOT_DIR_STR_LENGTH_REGISTRY_KEY_NAME,
                                root_dir_str_length);
    
    Wcl::Registry::write_string(key,
                                Wcl::Colorlib::SubSamplingConverter::PLUGIN_ROOT_DIR_REGISTRY_KEY_NAME,
                                root_dir);
  }
  
  Wcl::Registry::close_key(key);
}
#endif
