logicvc framebuffer driver RELEASE NOTES
===========================================


History
--------
This history table shows versions of all subcomponents used by the xylonfb in the moment of the release.


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