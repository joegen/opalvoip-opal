Requirements
------------

This plugin requires an external source code project, SpanDSP.

Spandsp is a LGPL library and suite of programs that implement a faxmodem. A
known version, compatible with OPAL, is available from the following URL:

    http://www.soft-switch.org/downloads/spandsp/spandsp-0.0.6pre7.tgz


After getting the spandsp tar file, unpack it so it is called "spandsp" in
the same directory is the plug in, e.g.

   opal/plugins/fax/fax_spandsp/spandsp-0.0.6



Building on Windows
-------------------

After installing SpanDSP, simply then use the spandsp_fax_2005.sln or
spandsp_fax_2008.sln file to build the plug in. Note Visual Studio 2003 is
not supported.

Note, this will automatically try and download libtiff, another open source
library for TIFF file operations. Thus the first time you build it you should
be connected to teh Internet so the download can proceed.

Then copy the following two files from:
	 opal/plugins/Release/SpanDSPFax_pwplugin.dll
	 opal/plugins/fax/fax_spandsp/spandsp-0.0.6/src/Release/libspandsp.dll

to C:\PTLib_Plugins, or wherever you have set the PTPLUGINDIR environment
variable. It should work in the smae directory as your application as well.


Building on Linux
-----------------

After installing SpanDSP, go to the spandsp directory and go:
	./configure
	make

then go back to the fax_spandsp directory and go:
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
