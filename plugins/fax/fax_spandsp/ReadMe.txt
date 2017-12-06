Requirements
------------

This plugin requires an external source code project, SpanDSP.

Spandsp is a LGPL library and suite of programs that implement a faxmodem. A
known version, compatible with OPAL, is available from the following URL:

    http://www.soft-switch.org/downloads/spandsp/spandsp-0.0.6.tgz



Building on Windows
-------------------

After getting the spandsp tar file, unpack it so it is called "spandsp" in
the same directory is the plug in, e.g.

	opal\plugins\fax\fax_spandsp\spandsp-0.0.6

After installing SpanDSP, open ...\spandsp-0.0.6\src\libspandsp.2008.sln and
allow it up upgrade to whatever compiler you are using. Depending on which
version of Visual Studio you upgraded to, you may need to edit the
spandsp-0.0.6\src\msvc\config.h file and remove the #define for snprintf. You
may also want to remove the INFINITY from spandsp-0.0.6\src\msvc\inttypes.h.

Right click on "Download TIFF" and build it. This will automatically try and
download libtiff, another open source library for TIFF file operations. Thus
the first time you build it you should be connected to the Internet so the
download can proceed. Then you will need to edit tiff-3.8.2\libtiff\tiffconf.h
and comment out the entries for JPEG_SUPPORT, LZW_SUPPORT, PIXARLOG_SUPPORT
 and ZIP_SUPPORT.

Then build the Debug and Release versions of libspandsp.

After building SpanDSP you can load opal\plugins\plugins_XXXX.sln, enter the
Configuration Manager and enable the SpanDSP Codec. You can then build the
plug in.

Finally, copy the following two files from:

	opal\bin\plugins\Release\SpanDSPFax_ptplugin.dll
	opal\plugins\fax\fax_spandsp\Release\libspandsp.dll

to C:\Program Files\PTLib Plugins, or wherever you have set the PTLIBPLUGINDIR
environment variable. It should work in the same directory as your application
as well.


Building on Linux
-----------------

After getting the spandsp tar file, unpack it and go:

	./configure
	make
	make install

then go to the opal directory and go:

	./configure

then go to the opal/plugins/fax/fax_spandsp directory and go:

	make

Note: you must have libtiff installed on your system.


IMPORTANT NOTE
--------------

The spandsp and libtiff libraries are released under the LGPL license. The
spandsp_fax plugin is released under the MPL license. This means that
distribution of aggregated source code is somewhat murky, which is why we
have instructions on downloading the components and their compilation.

This is also why the libspandsp.dll library is separate, to abide by
the terms of an LGPL library.


   Craig Southeren, 
   craigs@postincrement.com

   Robert Jongbloed
   robertj@voxlucida.com.au
