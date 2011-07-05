pyvoip project is being released under the Python license. All I ask is that
any patches or new features be shared, and give credit where credit is due.
Any questions either email the opal dev list, or email me: dcassidy36@gmail.com.
I will try and answer any questions as I can, but note I cannot offer

These opal python bindings depend on having the latest SIP source installed,
which as of this writing is v4.12.3. You can optionally also use PyQt4, so you
can create C++ classed derived from both Qt and Opal. However, note that Qt
headers MUST included before any opal headers, and Qt classes MUST be inherited
before any opal classes, or you will have compiler errors.

SIP can be acquired from here: http://riverbankcomputing.com

This project also assumes you have the following enviroment variables defined,
which point to the current opal/ptlib folders:

  "$(PTLIBDIR)" 
  "$(OPALDIR)"

Optionally, if building with Qt support, this env variable must point to your Qt
installation folder (note that I have only tested Qt v4.6 and 4.7):
  "$(QTDIR)"

To install:

  python configure.py
  nmake
  nmake install

I have only compiled on WinXP Pro 32bit, with msvc2008. You should be able to
use VS2010, but if you do Python itself has to be built with vs2010 or you will
get weird errors. I can not guarantee you can build with other compilers other
than vs2008, or outside of windows.

If you want to change what gets built, see the global var pyqt_modules in
configure.py. The following functions might also need to be modified to suit
your needs:

  include_dirs
  needed_qt_libs

Last thing to note is that sip is built with -Zc:wchar_t- by default, this mean
wchar_t is not a native type, and attempting to use any unicode methods of
PString and other classes will result in cryptic linker errors. I have changed
the default values of sipconfig build options so wchar_t is built as a native
type.


Usage:

To use opal within Python, you need to instantiate a PProcess instance. This is
simple and straightfoward:

from pyvoip.ptlib import PLibraryProcess
pApp = PLibraryProcess("MyCompanyName", 
                       "MyApp", 
                       1, 
                       0, 
                       PLibraryProcess.ReleaseCode, 
                       0)

To use OpalManager, either create an instance via OpalManager(), or derive from
it and call it's init method:

from pyvoip.opal import OpalManager
class MyManager(OpalManager):
	def __init__(self):
		super(MyManager, self).__init__()


All conversions between C++ types and Python is done natively, so in effect
PList, PDict, PSet, PString, std::string, std::list, std::vector, std::set have
Python equivalents (.e.g dict(), list(), etc.). There are however some
limitations:

You cannot derive from a C++ class which is mapped to a python type. This will
not work:

from pyvoip.ptlib import PString 
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
ImportError: cannot import name PString

neither will this:

	(C++ signature)
	virtual PList* MethodWhichReturnsAList(void);
	
	l = MethodWhichReturnsAList()
	l.Clone()
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
AttributeError: 'list' object has no attribute 'Clone'

Any method which returns a PSafePtr<cls> template, is not implemented. While
PSafePtr itself is implemented, SIP does not support methods which return a
template class, without having a explicit conversion to a python object.

istream* and ostream* classes are not implemented.

Patches are always welcome though!


* Copyright 2011, Demetrius Cassidy
