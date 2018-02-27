logicvc framebuffer driver RELEASE NOTES
===========================================


History
--------
This history table shows versions of all subcomponents used by the xylonfb in the moment of the release.


v4.3
----
 - release date (YYMMDD):180227
 - released by: GP
 - added param put-vscreeninfo-exact in dts that enables setting the 
   exact video mode over the ioctl FBIOPUT_VSCREENINFO, when not set
   mode is determined only from xres and yres param of fb_var_screeninfo
   
Component SW/HW   | Version     | Note
------------------|-------------|--------
Linux Kernel      | 4.9         |  -
logiCVC IP        | 5.4.1       |  -
u-boot xilinx     | 2017.1      |  -

v4.2
----
 - release date (YYMMDD):170522
 - released by: DS
 - adv7511 driver updated for compiling on 4.5 kernel version
   
Component SW/HW   | Version     | Note
------------------|-------------|--------
Linux Kernel      | 4.9         |  -
logiCVC IP        | 5.1.1       |  -
u-boot xilinx     | 2017.1      |  -


v4.1
----
 - release date (YYMMDD):170411
 - released by: DS
 - pixel variable defined, xylonfb_set_color_hw function
 - added XYLONFB_HW_ACCESS_INT_STAT_REG ioctl
 - removed unnecessary code in pan_display that wrote to the layer position registers.
 - added XYLONFB_HW_ACCESS_CTRL_REG ioctl used in layers synchronization
 - 1. If console-layer is not specified in dts, layer type is not checked for console compatibility
 - 2. video-mode is preferred mode, even if EDID is obtained. EDID-preferred mode is used only if video-mode is not specified in dts 
   
Component SW/HW   | Version     | Note
------------------|-------------|--------
Linux Kernel      | 4.4  		|  Tested on Linux Kernel 4.4
logiCVC IP        | 5.1.1       |  -
u-boot xilinx     |	2016.01	    |  used for testing



v4.0
----
 - release date (YYMMDD):160707
 - released by: TM
 - upgraded to support logiCVC 5.0.1. Maximum resolution expanded form 2048x2048 to 8192x8192.
   Added new color formats such as ARGB3332, ARGB565, XRGB2101010, YUYV_121010, XYUV_8888, 
   XYUV_2101010
 - added new ioctl function reload registers which reads registers value from memory and 
   write it to register
   
Component SW/HW   | Version     | Note
------------------|-------------|--------
Linux Kernel      | 4.4  		|  Tested on Linux Kernel 4.4
logiCVC IP        | 5.0.1       |  -
logiclk		      |	1.1	        |  -
u-boot xilinx     |	16.2	    |  used for testing