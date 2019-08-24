Requirements
------------

This plugin requires an external source code project, SpanDSP.

Spandsp is a LGPL library and suite of programs that implement a faxmodem. A
known version, compatible with OPAL, is available from the following URL:

    https://www.soft-switch.org/downloads/spandsp/spandsp-0.0.6.tar.gz



Building on Windows
-------------------

The plugins build solution, opal\plugins\plugins_XXXX.sln, has been set up
to automatically download and build the necessary libraries. Note that this
means you need to be connected to the internet to do the first time build.

After building, copy the following two files from:

	opal\bin\plugins\Release\SpanDSPFax_ptplugin.dll
	opal\plugins\fax\fax_spandsp\Release\libspandsp.dll

to C:\Program Files\PTLib Plugins, or wherever you have set the PTLIBPLUGINDIR
environment variable. It should work in the same directory as your application
as well.


Building on Linux or Mac
------------------------

As a rule, SpanDSP can be installed via "yum" or "apt-get" for flavours of Linux,
and "port" or "brew" for Mac. The OPAL configure will locate the system installed
libraries. Alternatively, you can download, configure and build SpanDSP yourself
from the taball at the top of this ReadMe. You then use arguments on OPAL
configure to point to this version.


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
