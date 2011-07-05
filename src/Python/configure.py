# configure.py
# Copyright 2011 Demetrius Cassidy
import os
import sys
import shutil
import sipconfig
import getopt

from os.path import abspath, exists, join, normpath

try:
    from PyQt4 import pyqtconfig
    has_pyqt = True
except:
    pyqtconfig = sipconfig
    sipconfig.inform("PyQt4 is not available, Qt support is disabled.")
    has_pyqt = False

sip_min_version = 0x040900

qt_version = 0
qt_edition = ""
qt_licensee = None
qt_dir = None
qt_incdir = None
qt_libdir = None
qt_bindir = None
qt_datadir = None
qt_pluginsdir = None
qt_xfeatures = None
qt_shared = ""
qt_framework = 0

PTLIBDIR = "$(PTLIBDIR)"
OPALDIR = "$(OPALDIR)"
VOICEMANAGERDIR = join("$(PYVOIPDIR)", "voicemanager")
QTDIR = "$(QTDIR)"

INCLUDEDIRS = "include"
LIBDIRS = "lib"

#we have to remove unicode from the defines, as ptlib and opal do not have unicode support
pyqtconfig._default_macros['DEFINES'] = 'WIN32 QT_LARGEFILE_SUPPORT MBCS'
pyqtconfig._default_macros['CXXFLAGS'] = '-nologo -Zm200 -Zc:wchar_t -Zc:forScope'
pyqtconfig._default_macros['CFLAGS'] = '-nologo -Zm200 -Zc:wchar_t -Zc:forScope'

pyqt_modules = ['ptlib', 'opal']

# Get the PyQt configuration information.
pyqtcfg = pyqtconfig.Configuration()

pyqt_modroot = pyqtcfg.default_mod_dir

if has_pyqt:
    qt_sip_flags = pyqtcfg.pyqt_sip_flags
else:
    qt_sip_flags = None

def add_makefile_extras(makefile, extra_include_dirs, extra_lib_dirs, extra_libs):
    """Add any extra include or library directories or libraries to a makefile.

    makefile is the makefile.
    extra_include_dirs is the list of extra include directories.
    extra_lib_dirs is the list of extra library directories.
    extra_libs is the list of extra libraries.
    """
    if extra_include_dirs:
        makefile.extra_include_dirs.extend(extra_include_dirs)

    if extra_lib_dirs:
        makefile.extra_lib_dirs.extend(extra_lib_dirs)

    if extra_libs:
        makefile.extra_libs.extend(extra_libs)


def needed_qt_libs(mname):
    """Return a list of libraries needed by the specified module.
    
       @type: c{str}
       @mname: The name of the module.

       @return: c{list} of library names (.e.g qtcore.lib)
    """
    libs = []

    # The dependencies between the different Qt libraries.  The order within
    # each list is important.  Note that this affects the include directories
    # as well as the libraries.
    LIB_DEPS = {
        "ptlib": ["ws2_32"],
        "opal": ["ptlib"],
        "voicemanager": ["opal", "qtmain", "qtcore4"],
        "ptlibd": ["ws2_32"],
        "opald": ["ptlibd"],
        "voicemanagerd": ["opald", "qtmaind", "qtcore4d"]
    }

    # Handle the dependencies first.
    libs.extend(LIB_DEPS[mname])
    for lib in LIB_DEPS[mname]:
        if lib in LIB_DEPS: #check if this lib has dependencies
            libs.extend(LIB_DEPS[lib])

    if mname not in libs:
        libs.insert(0, mname)
        
    return libs

        
def include_dirs(mname):
    """Return a list of include directories for the specified module"""
    
    if has_pyqt:
        QTDIR_INCLUDE = join(QTDIR, "include", "QtCore")
    
    DIRS = {
        "ptlib" :  join(PTLIBDIR, "include"),
        "opal"  :  join(OPALDIR, "include"),
        "voicemanager" : normpath(VOICEMANAGERDIR)
        }
    
    MODULE_INCLUDES = {
        "ptlib" : [],
        "opal"  : [],
        "voicemanager" : []
        }
    
    for depends_on in needed_qt_libs(mname):
        if depends_on in DIRS:
            MODULE_INCLUDES[mname].append(DIRS[depends_on])
    return MODULE_INCLUDES[mname]


def lib_dirs(mname):
    """Return a list of lib directories for the specified module"""

    DIRS = {
        "ws2_32" : normpath("$(PROGRAMFILES)/Microsoft SDKs/Windows/v6.0A/Lib"),
        "ptlib" :  join(PTLIBDIR, "lib"),
        "opal"  :  join(OPALDIR, "lib"),
        "voicemanager" : join(VOICEMANAGERDIR, "lib")
        }

    MODULE_INCLUDES = {
        "ptlib" : [],
        "opal"  : [],
        "voicemanager" : []
        }

    for depends_on in needed_qt_libs(mname):
        if depends_on in DIRS:
            MODULE_INCLUDES[mname].append(DIRS[depends_on])
    return MODULE_INCLUDES[mname]

    
def mk_clean_dir(name):
    """Create a clean (ie. empty) directory.

    name is the name of the directory.
    """
    try:
        shutil.rmtree(name)
    except:
        pass

    try:
        os.makedirs(name)
    except:
        sipconfig.error("Unable to create the %s directory." % name)


def generate_code(mname, include_dirs=None, lib_dirs=None, libs=None, extra_sip_flags=None, debug=0):
    """Generate the code for a module.

    mname is the name of the module to generate the code for.
    extra_include_dirs is an optional list of additional directories to add to
    the list of include directories.
    extra_lib_dirs is an optional list of additional directories to add to the
    list of library directories.
    extra_libs is an optional list of additional libraries to add to the list
    of libraries.
    extra_sip_flags is an optional list of additional flags to pass to SIP.
    """
    sipconfig.inform("Generating the C++ source for the %s module..." % mname)
    modfile = mname + "mod.sip"
    builddir = abspath(join("build", mname, "release" if not debug else "debug"))

    #sanity check - avoid cleaning a directory full of .sip files!
    if exists(join(builddir, modfile)):
        raise RuntimeError("SIP module target {} exists in build directory {}, aborting!".format(modfile,
                                                                                                 builddir))
    
    #clean the build directory
    mk_clean_dir(builddir)

    # Build the SIP command line.
    argv = ['"' + pyqtcfg.sip_bin + '"']

    argv.append(qt_sip_flags)

    if extra_sip_flags:
        argv.extend(extra_sip_flags)
        
    #enable debugging statements within sip
    if debug:
        argv.append("-r")

    # the directory where our cpp files are placed
    argv.append("-c")
    argv.append(builddir)

    buildfile = join(builddir, mname + ".sbf")
    argv.append("-b")
    argv.append(buildfile)

    argv.append("-I")
    argv.append(pyqtcfg.pyqt_sip_dir)
    
    argv.append("-I")
    argv.append(join(os.getcwdu(), mname))

    # SIP assumes POSIX style path separators.
    argv.append(abspath(join(mname, modfile)))

    cmd = " ".join(argv)
    sys.stdout.write(cmd + "\n")

    #call sip.exe to generate our C++ code
    os.system(cmd)

    # Check the result.
    if not os.access(buildfile, os.F_OK):
        sipconfig.error("Unable to create the C++ code.")

    # Generate the Makefile.
    sipconfig.inform("Creating the Makefile for the %s module..." % mname)

    makefile = sipconfig.SIPModuleMakefile(
        configuration=pyqtcfg,
        build_file=buildfile,
        dir=builddir,
        install_dir=pyqt_modroot,
        qt=True,
        universal=pyqtcfg.universal,
        arch=pyqtcfg.arch,
        debug=debug,
        export_all=True,
        prot_is_public=False,
        threaded=True
    )

    add_makefile_extras(makefile, include_dirs, lib_dirs, libs)
    makefile.extra_cxxflags.append('-EHsc')
    makefile.extra_cflags.append("-Zi")
    makefile.extra_cxxflags.append("-Zi")
    makefile.extra_lflags.append("/DEBUG")
    makefile.extra_lflags.append("/PDB:{}.pdb".format(mname))

    makefile.generate()
    return builddir



def main(argv):
    global pyqt_modroot
    pyqt_modroot = os.path.join(pyqt_modroot, "pyvoip")
    installs=[([join('sip', '__init__.py')], pyqt_modroot)]
    debug=False
    build_dirs = []

    try:
        opts, args = getopt.getopt(argv, "d")
    except getopt.GetoptError:
        print("Usage: 'configure.py -d' to build debug version, otherwise ommit this flag.")
        sys.exit(2)

    if opts:
        for opt, arg in opts:
            if opt == "-d":
                sipconfig.inform("Building debug libraries...")
                debug=True
        
    if not debug:
        sipconfig.inform("Building libraries...")
       
    for module in pyqt_modules:
        mname = module if not debug else mname + "d"
        build_dirs.append(generate_code(module, 
                                        libs=needed_qt_libs(mname), 
                                        lib_dirs=lib_dirs(module),
                                        include_dirs=include_dirs(module),
                                        debug=debug))
        

    # Create the additional Makefiles.
    sipconfig.inform("Creating top level Makefile...")

    sipconfig.ParentMakefile(
        configuration=pyqtcfg,
        subdirs=build_dirs,
        installs=installs
        ).generate()


if __name__ == "__main__":
    main(sys.argv[1:])
    
