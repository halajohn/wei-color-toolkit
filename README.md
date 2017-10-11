This is a tool to do color space conversion. I release it under GNU GPL v3. I create this project to demonstrate the power of WeiCommonLib::Colorlib. The abilities of this application include:

# Color space conversion

It can convert various color space data, ex: sRGB, SMPTE 170M RGB, SMPTE 170M YPbPr, SMPTE 170M YCbCr, ITU-R BT.709 RGB, etc.

The class hierarchy and plugin relationships are as follows:

![](http://lh6.google.com.tw/wei.hu.tw/R-e3iegF8aI/AAAAAAAAAUw/IjrRuxZcP2c/color_space_converter_plugin.png?imgmax=576)

The screenshot of this mechinaism:

![](http://lh4.google.com.tw/wei.hu.tw/R-e5y-gF8hI/AAAAAAAAAV8/DHJ_ZzURC-M/color_space_conversion.png?imgmax=720)

# Decouple Y,Cb,Cr from an image

It can separate Y,Cb,Cr component from an image file, ex: PNG, JPG, BMP, TIFF, etc.

The screenshot of this mechinaism:

![](http://lh4.google.com.tw/wei.hu.tw/R-e5y-gF8iI/AAAAAAAAAWE/df8TtgyCPSw/ycbcr_decoupling.png?imgmax=720)

# Display CIE chromaticity diagram & spectrum image

It can display an CIE chromaticity diagram and draw the triangle corresponding to the selected RGB color space. Moreover, it will display the spectrum, too.

The screenshot of this mechinaism:

![](http://lh3.google.com.tw/wei.hu.tw/R-e5yugF8gI/AAAAAAAAAV0/qmg_Fm1qiyE/cie_page.png?imgmax=720)

# Convert YCbCr subsampling data into various other subsampling type

It can convert data buffer between different YCbCr subsampling format.

The class hierarchy and plugin relationships are as follows:

![](http://lh3.google.com.tw/wei.hu.tw/R-e3iugF8bI/AAAAAAAAAU4/K_zaOtwrRwc/subsampling_converter_plugin.png)

The screenshot of this mechinaism:

![](http://lh5.google.com.tw/wei.hu.tw/R-e5zOgF8jI/AAAAAAAAAWM/k4XQK5xbLUk/subsampling_conversion.png?imgmax=720)
