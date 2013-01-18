dnl
dnl OPAL configuration
dnl

AC_INIT(opal, 3)
AC_CONFIG_SRCDIR([include/opal/manager.h])

AC_PREREQ([2.65])

AC_CONFIG_AUX_DIR(make)

m4_include(make/opal.m4)

OBJ_ARCH=
if test "x$OPALDIR" != "x" ; then
  ac_default_prefix=$OPALDIR
  OBJ_ARCH=1
fi

PKG_PROG_PKG_CONFIG()
if test -z "$PKG_CONFIG" ; then
  AC_MSG_ERROR(OPAL must have pkg-config installed, 1)
fi


m4_include(make/canonical.m4)
MY_CANONICAL_TARGET()


dnl Prevent CFLAGS and CXXFLAGS from being overwritten if undefined
if test "x${CXXFLAGS}" = "x" ; then
  CXXFLAGS=
fi
if test "x${CFLAGS}" = "x" ; then
  CFLAGS=
fi
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_HEADER_STDC
AC_CHECK_TOOL([AR], [ar], [:])
AC_SUBST(LD, $CXX)

OPAL_CHECK_BSR([GCC_HAS_CLZ=yes], [GCC_HAS_CLZ=no])
if test "x${GCC_HAS_CLZ}" = "xyes" ; then
    AC_DEFINE([GCC_HAS_CLZ], [1], [bit scan intrinsic])
fi
AC_SUBST(GCC_HAS_CLZ)

dnl Enable compiler warnings
WFLAGS=
if test "x$ac_cv_c_compiler_gnu" = xyes; then
  WFLAGS="-Wall -Wextra -Wstrict-aliasing -Wfloat-equal -Wno-comment -Wno-unused -Winit-self -Wno-missing-field-initializers"
fi

CFLAGS="$CFLAGS $WFLAGS"
CXXFLAGS="$CXXFLAGS $WFLAGS"


dnl ########################################################################
dnl Defaults
dnl ########################################################################

OPAL_H323=yes
OPAL_SIP=yes
OPAL_IAX2=yes
OPAL_VIDEO=yes
OPAL_T38_CAPABILITY=yes
OPAL_FAX=yes
OPAL_HAS_H224=yes
OPAL_HAS_H281=yes
OPAL_H450=yes
OPAL_H460=yes
OPAL_H460_NAT=yes
OPAL_H239=yes
OPAL_H501=yes
OPAL_T120DATA=no
OPAL_LID=yes
OPAL_CAPI=yes
OPAL_STATISTICS=yes
OPAL_RTCP_XR=yes
OPAL_AEC=yes
OPAL_IVR=yes
OPAL_RFC4175=yes
OPAL_RFC2435=no
OPAL_G711PLC=yes
OPAL_PLUGINS=yes
OPAL_SAMPLES=no
OPAL_ZRTP=no
OPAL_JAVA=yes
OPAL_RUBY=yes
OPAL_SCRIPT=yes
OPAL_HAS_MSRP=yes
OPAL_HAS_SIPIM=yes
OPAL_HAS_RFC4103=yes
OPAL_HAS_MIXER=yes
OPAL_HAS_PCSS=yes
PTLIB_REQUIRED_VERSION="2.12.0"

# experimental options
OPAL_DAHDI=no

dnl ########################################################################
dnl set the OPAL directory to the current directory

if test "x$OBJ_ARCH" = "x"; then
   OPALDIR=`pwd`
fi
AC_SUBST(OPALDIR)

dnl ########################################################################
dnl Extract OPAL version number
dnl ########################################################################

dnl MSWIN_DISPLAY version,Version
dnl MSWIN_VERSION version,OPAL_MAJOR=MAJOR_VERSION
dnl MSWIN_VERSION version,OPAL_MINOR=MINOR_VERSION
dnl MSWIN_VERSION version,OPAL_BUILD=BUILD_NUMBER
dnl MSWIN_VERSION version,OPAL_VERSION=VERSION

MAJOR_VERSION=`cat ${OPALDIR}/version.h | grep MAJOR_VERSION | cut -f3 -d' '`
MINOR_VERSION=`cat ${OPALDIR}/version.h | grep MINOR_VERSION | cut -f3 -d' '`
BUILD_NUMBER=`cat ${OPALDIR}/version.h | grep BUILD_NUMBER | cut -f3 -d' '`
OPAL_VERSION="${MAJOR_VERSION}.${MINOR_VERSION}.${BUILD_NUMBER}"
BUILD_TYPE=`cat ${OPALDIR}/version.h | grep BUILD_TYPE | cut -f 3 -d ' ' | sed 's/BetaCode/-beta/' | sed 's/AlphaCode/-alpha/' | sed 's/ReleaseCode/\./'`

AC_DEFINE_UNQUOTED(OPAL_MAJOR,   [${MAJOR_VERSION}], [Major version])
AC_DEFINE_UNQUOTED(OPAL_MINOR,   [${MINOR_VERSION}], [Minor version])
AC_DEFINE_UNQUOTED(OPAL_BUILD,   [${BUILD_NUMBER}], [Build number])
AC_DEFINE_UNQUOTED(OPAL_VERSION, ["$OPAL_VERSION"], [Opal version string])

AC_MSG_NOTICE(Opal version is ${OPAL_VERSION});

#use to set the version in opal.pc
AC_SUBST(OPAL_VERSION)
AC_SUBST(BUILD_TYPE)

dnl ########################################################################
dnl Check for PTLIB
dnl ########################################################################

OPAL_FIND_PTLIB()


PKG_REQUIRES="$PKG_REQUIRES ptlib"
CFLAGS="$CFLAGS $PTLIB_CFLAGS"
LDFLAGS="$LDFLAGS $PTLIB_CFLAGS"
CXXFLAGS="$CXXFLAGS $PTLIB_CFLAGS $PTLIB_CXXFLAGS"
#LIBS="$LIBS $PTLIB_LIBS"


dnl ########################################################################
dnl Architecture dependent stuff
dnl ########################################################################

LDSOOPTS=
case "$target_os" in
	solaris )
          SHAREDLIBEXT=so
          SHARED_CFLAGS="-D__inline=inline"
          OPAL_DETERMINE_LIBNAMES(DEBUG)
          OPAL_DETERMINE_LIBNAMES(RELEASE)
          LDSOOPTS="-Bdynamic -G -h \$(LIB_FILENAME_SHARED_PAT)"

          HAVE_RANLIB=no
        ;;

        Darwin )
          SHAREDLIBEXT=dylib
          SHARED_CFLAGS="-fPIC"
          OPAL_DETERMINE_LIBNAMES(DEBUG)
          OPAL_DETERMINE_LIBNAMES(RELEASE)
          LDSOOPTS="-dynamiclib"

          HAVE_RANLIB=no
        ;;

        cygwin|mingw )
          SHAREDLIBEXT=dll
          SHARED_CFLAGS=
          OPAL_DETERMINE_LIBNAMES(DEBUG)
          OPAL_DETERMINE_LIBNAMES(RELEASE)
          LDSOOPTS="-Wl,--kill-at -shared"
          HAVE_RANLIB=yes
        ;;

        *BSD )
          SHAREDLIBEXT=so
          SHARED_CFLAGS="-fPIC"
          OPAL_DETERMINE_LIBNAMES(DEBUG)
          OPAL_DETERMINE_LIBNAMES(RELEASE)
          LDSOOPTS="-shared -Wl,-soname,\$(LIB_FILENAME_SHARED_PAT)"

          HAVE_RANLIB=yes
        ;;

        *)
          SHAREDLIBEXT=so
          SHARED_CFLAGS="-fPIC"
          OPAL_DETERMINE_LIBNAMES(DEBUG)
          OPAL_DETERMINE_LIBNAMES(RELEASE)
          LDSOOPTS="-shared -Wl,-soname,\$(LIB_FILENAME_SHARED_PAT)"

          HAVE_RANLIB=no
        ;;
esac

dnl ########################################################################
dnl set libdir accordingly
if test "x$OBJ_ARCH" != "x" ; then
   includedir=$OPALDIR/include
   libdir=$OPALDIR/lib_$target
fi


dnl ########################################################################
if test x$HAVE_RANLIB = xyes ; then
  ARCHIVE="${AR} rc"
else
  ARCHIVE="${AR} rcs"
fi
CFLAGS="$CFLAGS $SHARED_CFLAGS"
CXXFLAGS="$CXXFLAGS $SHARED_CFLAGS"

#-fno-exceptions per configure option
# mingw: -mms-bitfields


AC_MSG_CHECKING(if debug build accepts -g3 -ggdb -O0)
AC_LANG_CONFTEST([AC_LANG_PROGRAM([[]],[[]])])
$CXX  $CXXFLAGS  -g3 -ggdb -O0 conftest.$ac_ext > /dev/null 2>&1
if test $? = 0 ; then
  AC_MSG_RESULT(yes)
  DEBUG_CFLAGS="-g3 -ggdb -O0 -D_DEBUG"
else
  AC_MSG_RESULT(no)
  DEBUG_CFLAGS="-g -D_DEBUG"
fi


OPAL_MSG_CHECK([ptlib version], [$PTLIB_VERSION])

OPAL_CHECK_PTLIB([URL],     [PTLIB_URL])
if test "x$OPAL_PTLIB_URL" != "xyes" ; then
   AC_MSG_ERROR([Must have URL functionality enabled in PTLib])
fi

OPAL_CHECK_PTLIB([HTTP],     [PTLIB_HTTP])
if test "x$OPAL_PTLIB_HTTP" != "xyes" ; then
   AC_MSG_ERROR([Must have HTTP functionality enabled in PTLib])
fi

OPAL_CHECK_PTLIB([STUN],         [PTLIB_STUN])
OPAL_CHECK_PTLIB([SSL/TLS],      [PTLIB_SSL])
OPAL_CHECK_PTLIB([SSL/AES],      [PTLIB_SSL_AES])
OPAL_CHECK_PTLIB([ASN.1],        [PTLIB_ASN])
OPAL_CHECK_PTLIB([Expat],        [PTLIB_EXPAT])
OPAL_CHECK_PTLIB([video],        [PTLIB_VIDEO])
OPAL_CHECK_PTLIB([WAV file],     [PTLIB_WAVFILE])
OPAL_CHECK_PTLIB([VXML],         [PTLIB_VXML])
OPAL_CHECK_PTLIB([IP v6],        [PTLIB_IPV6])
OPAL_CHECK_PTLIB([DTMF],         [PTLIB_DTMF])
OPAL_CHECK_PTLIB([LDAP],         [PTLIB_LDAP])
OPAL_CHECK_PTLIB([DNS resolver], [PTLIB_DNS_RESOLVER])
OPAL_CHECK_PTLIB([audio],        [PTLIB_AUDIO])
OPAL_CHECK_PTLIB([config],       [PTLIB_CONFIG_FILE])
OPAL_CHECK_PTLIB([CLI],          [PTLIB_CLI])



dnl ########################################################################
dnl Check for other libraries
dnl ########################################################################


dnl     ########################
dnl     libdl
dnl     ########################

OPAL_FIND_LIBDL([HAVE_LIBDL=yes], [HAVE_LIBDL=no])
if test "x$HAVE_LIBDL" = "xyes" ; then
    OPAL_ADD_CFLAGS_LIBS([$DL_CFLAGS], [$DL_LIBS])
fi


dnl     ########################
dnl     speex (system/local)
dnl     ########################

OPAL_SPEEX_TYPES()
AC_SUBST(SIZE16)
AC_SUBST(SIZE32)

OPAL_DETERMINE_SPEEXDSP()

if test "x$SPEEXDSP_SYSTEM" = "xyes" ; then
  AC_DEFINE(OPAL_SYSTEM_SPEEX, [1], [Has system speex dsp library])
  OPAL_SPEEX_FLOAT( [AC_DEFINE([OPAL_SPEEX_FLOAT_NOISE], [1], [Speex has float])] )
  OPAL_ADD_CFLAGS_LIBS([$SPEEXDSP_CFLAGS], [$SPEEXDSP_LIBS], [speexdsp])
fi

AC_SUBST(SPEEXDSP_SYSTEM)

dnl     ########################
dnl     ZRTP
dnl     ########################
dnl XMSWIN_DISPLAY     libzrtp,libZRTP support
dnl XMSWIN_CHECK_FILE  libzrtp,include\zrtp.h,OPAL_ZRTP=1
dnl XMSWIN_DIR_SYMBOL  libzrtp,LIBZRTP_DIR

AC_ARG_ENABLE([zrtp],
              [AC_HELP_STRING([--enable-zrtp],[enable ZRTP protocol support])],
              [OPAL_ZRTP=$enableval])

if test "x$OPAL_ZRTP" = "xyes" ; then
  if test "x$OPAL_SIP" != "xyes" ; then
    echo "  ERROR: SIP protocol is disabled";
    exit 1; 
  fi
  OPAL_FIND_LIBZRTP([OPAL_ZRTP=yes], [OPAL_ZRTP=no])
  if test "x$OPAL_ZRTP" = "xyes" ; then
    AC_DEFINE([OPAL_ZRTP], [1], [libzrtp support])
    OPAL_ADD_CFLAGS_LIBS([$ZRTP_CFLAGS], [$ZRTP_LIBS])
  fi
fi
OPAL_MSG_CHECK([ZRTP protocol support],[$OPAL_ZRTP])

AC_SUBST(OPAL_ZRTP)

dnl     ########################
dnl     SRTP
dnl     ########################
dnl MSWIN_DISPLAY libsrtp,Cisco libSRTP support
dnl MSWIN_DEFINE  libsrtp,OPAL_SRTP

AC_ARG_ENABLE([srtp],
              [AC_HELP_STRING([--enable-srtp],[enable SRTP protocol support])],
              [OPAL_SRTP=$enableval])

SRTP_LIBS=
SRTP_CFLAGS=
if test "x$OPAL_SRTP" = "xno" ; then
  AC_MSG_NOTICE([disabled by user])
else
  OPAL_FIND_LIBSRTP([OPAL_SRTP=yes], [OPAL_SRTP=no])
  if test "x${OPAL_SRTP}" = "xyes" ; then
    AC_DEFINE([OPAL_SRTP], [1], [libsrtp support])
    OPAL_ADD_CFLAGS_LIBS([$SRTP_CFLAGS], [$SRTP_LIBS])
  fi
  OPAL_MSG_CHECK([libsrtp support],[$OPAL_SRTP])
fi
AC_SUBST(OPAL_SRTP)


dnl ########################################################################

dnl MSWIN_DISPLAY capi,CAPI (isdn) Line Interface Device
dnl MSWIN_DEFINE  capi,OPAL_CAPI

AC_ARG_ENABLE([capi],
              [AC_HELP_STRING([--enable-capi],[enable CAPI])],
              [OPAL_CAPI=$enableval])

if test "x$OPAL_CAPI" = "xyes" ; then
  AC_CHECK_HEADERS([capi20.h], [OPAL_CAPI=yes], [OPAL_CAPI=no])
  if test "x$OPAL_CAPI" = "xyes" ; then
    AC_DEFINE([OPAL_CAPI], [1])
    OPAL_ADD_CFLAGS_LIBS([], [-lcapi20])
  fi
fi

AC_SUBST(OPAL_CAPI)


dnl ########################################################################
dnl See if DAHDI interface is available

AC_ARG_ENABLE([dahdi],
              [AC_HELP_STRING([--enable-dahdi],[enable DAHDI])],
              [OPAL_DAHDI=$enableval])

if test "x$OPAL_DAHDI" = "xyes" ; then
  AC_CHECK_HEADERS([dahdi/user.h], [OPAL_DAHDI=yes], [OPAL_DAHDI=no])
  if test "x$OPAL_DAHDI" = "xyes" ; then
    AC_DEFINE([OPAL_DAHDI], [1])
  fi
fi

AC_SUBST(OPAL_DAHDI)


dnl ########################################################################
dnl We should use SWIG if it is there
AC_PATH_PROG(SWIG, swig)


dnl     ########################
dnl     JAVA Support
dnl     ########################
dnl MSWIN_DISPLAY     java,Java interface
dnl MSWIN_CHECK_FILE  java,include\jni.h,OPAL_JAVA=1
dnl MSWIN_DIR_SYMBOL  java,JAVA_DIR
dnl MSWIN_CHECK_DIR   java,%JAVA_HOME%\
dnl MSWIN_CHECK_DIR   java,%ProgramFiles%\java\jdk*
dnl MSWIN_CHECK_DIR   java,%ProgramW6432%\java\jdk*

AC_ARG_ENABLE([java],
              [AC_HELP_STRING([--enable-java],[enable Java JNI support])],
              [OPAL_JAVA=$enableval])

if test "x$OPAL_JAVA" = "xyes" ; then
  OPAL_FIND_JAVA([OPAL_JAVA=yes], [OPAL_JAVA=no])
  if test "x$OPAL_JAVA" = "xyes" ; then
    CFLAGS="$CFLAGS $JAVA_CFLAGS"
    CXXFLAGS="$CXXFLAGS $JAVA_CFLAGS"
  fi
fi
AC_SUBST(OPAL_JAVA)


dnl ########################
dnl Ruby Support
dnl ########################
dnl MSWIN_DISPLAY    ruby,Ruby interface
dnl MSWIN_DEFAULT    ruby,Disabled
dnl MSWIN_CHECK_FILE ruby,ruby.h,OPAL_RUBY=1
dnl MSWIN_DIR_SYMBOL ruby,RUBY_DIR
dnl MSWIN_CHECK_DIR  ruby,%ProgramFiles%\Ruby\include\*
dnl MSWIN_CHECK_DIR  ruby,%ProgramW6432%\Ruby\include\*
dnl MSWIN_CHECK_DIR  ruby,c:\Ruby\include\*
dnl MSWIN_CHECK_DIR  ruby,c:\tools\Ruby\include\*

AC_ARG_ENABLE([ruby],
              [AC_HELP_STRING([--enable-ruby],[enable Ruby support])],
              [OPAL_RUBY=$enableval])

if test "x$OPAL_RUBY" = "xyes" ; then
  AC_CHECK_PROG(HAS_RUBY,ruby,yes)
  if test "x$HAS_RUBY" = "xyes" ; then
    RUBY_CFLAGS=-I`ruby -e 'puts $:.join(" -I")'`
    oldCFLAGS="$CFLAGS"
    oldCPPFLAGS="$CPPFLAGS"
    CFLAGS="$CFLAGS $RUBY_CFLAGS"
    CPPFLAGS="$CFLAGS $RUBY_CFLAGS"
    AC_CHECK_HEADER(ruby.h,HAS_RUBY=yes,HAS_RUBY=no)
    CFLAGS="$oldCFLAGS"
    CPPFLAGS="$oldCPPFLAGS"
    
    if test "x$HAS_RUBY" = "xyes" ; then
      CFLAGS="$CFLAGS $RUBY_CFLAGS"
      CXXFLAGS="$CXXFLAGS $RUBY_CFLAGS"
    fi
  fi
fi
AC_SUBST(OPAL_RUBY)


dnl ########################################################################
dnl MSWIN unconditionally needed LIBS
dnl ########################################################################
case "$target_os" in
        cygwin|mingw)
          OPAL_ADD_CFLAGS_LIBS([], [-lavifil32])
        ;;
esac


dnl ########################################################################
dnl Check for optional features
dnl ########################################################################

dnl MSWIN_DISPLAY  video,Video
dnl MSWIN_DEFINE   video,OPAL_VIDEO
OPAL_SIMPLE_OPTION(video,OPAL_VIDEO, [whether to enable video codec support], [OPAL_PTLIB_VIDEO])

dnl MSWIN_DISPLAY  sip,SIP support
dnl MSWIN_DEFINE   sip,OPAL_SIP
OPAL_SIMPLE_OPTION(sip,OPAL_SIP, [whether to enable SIP protocol support])

dnl MSWIN_DISPLAY  h323,H.323 support
dnl MSWIN_DEFINE   h323,OPAL_H323
OPAL_SIMPLE_OPTION(h323,OPAL_H323, [whether to enable H.323 protocol support])

if test "x$OPAL_H323" = "xyes" -a "x$OPAL_PTLIB_ASN" != "xyes" ; then
  AC_MSG_NOTICE([Must have ASN.1 for H.323])
  OPAL_H323=no
fi

dnl MSWIN_DISPLAY  iax2,IAX2 support
dnl MSWIN_DEFINE   iax2,OPAL_IAX2
OPAL_SIMPLE_OPTION(iax2,OPAL_IAX2, [whether to enable IAX2 protocol support])

dnl MSWIN_DISPLAY  h224,H.224 support
dnl MSWIN_DEFINE   h224,OPAL_HAS_H224
OPAL_SIMPLE_OPTION(h224,OPAL_HAS_H224, [whether to enable H.224 protocol support], [OPAL_H323])

dnl MSWIN_DISPLAY  h281,H.281 (Far End Camera Control) support
dnl MSWIN_DEFINE   h281,OPAL_HAS_H281
OPAL_SIMPLE_OPTION(h281,OPAL_HAS_H281, [whether to enable H.281 (Far End Camera Control) protocol support], [OPAL_HAS_H224])

dnl MSWIN_DISPLAY  t38,T.38 capability
dnl MSWIN_DEFINE   t38,OPAL_T38_CAPABILITY
OPAL_SIMPLE_OPTION(t38,OPAL_T38_CAPABILITY, [whether to enable T.38 capability support])

dnl MSWIN_DISPLAY  msrp,MSRP capability
dnl MSWIN_DEFINE   msrp,OPAL_HAS_MSRP
OPAL_SIMPLE_OPTION(msrp,OPAL_HAS_MSRP, [whether to enable MSRP support])

dnl if test "x$OPAL_HAS_MSRP" = "xyes" ; then
dnl   AC_MSG_NOTICE([WARNING: MSRP is highly experimental and is recommended for developers only. Please don't complain if this does not compile or breaks your system])
dnl fi

dnl MSWIN_DISPLAY  sipim,SIPIM capability
dnl MSWIN_DEFINE   sipim,OPAL_HAS_SIPIM
OPAL_SIMPLE_OPTION(sipim,OPAL_HAS_SIPIM, [whether to enable SIPIM session support])

dnl MSWIN_DISPLAY  rfc4103,RFC4103 capability
dnl MSWIN_DEFINE   rfc4103,OPAL_HAS_RFC4103
OPAL_SIMPLE_OPTION(rfc4103,OPAL_HAS_RFC4103, [whether to enable RFC4103 support])

dnl MSWIN_DISPLAY  statistics,Statistics gathering support
dnl MSWIN_DEFINE   statistics,OPAL_STATISTICS
OPAL_SIMPLE_OPTION(statistics,OPAL_STATISTICS, [whether to enable statistics gathering support])

dnl MSWIN_DISPLAY  fax,T.38 FAX protocol
dnl MSWIN_DEFINE   fax,OPAL_FAX
OPAL_SIMPLE_OPTION(fax,OPAL_FAX, [whether to enable T.38 FAX protocol support],[OPAL_T38_CAPABILITY],[OPAL_PTLIB_ASN],[OPAL_STATISTICS])

dnl MSWIN_DISPLAY    h450,H.450
dnl MSWIN_IF_FEATURE h450,h323
dnl MSWIN_DEFINE     h450,OPAL_H450
OPAL_SIMPLE_OPTION(  h450,OPAL_H450, [whether to enable H.450], [OPAL_H323])

dnl MSWIN_DISPLAY    h460,H.460
dnl MSWIN_IF_FEATURE h460,h323
dnl MSWIN_DEFINE     h460,OPAL_H460
OPAL_SIMPLE_OPTION(  h460,OPAL_H460, [whether to enable H.460], [OPAL_H323])

dnl MSWIN_DISPLAY    h460nat,H.460 NAT traversal
dnl MSWIN_IF_FEATURE h460nat,h323
dnl MSWIN_DEFINE     h460nat,OPAL_H460_NAT
OPAL_SIMPLE_OPTION(  h460nat,OPAL_H460_NAT, [whether to enable H.460.24], [OPAL_H323])

dnl MSWIN_DISPLAY    h239,H.239
dnl MSWIN_IF_FEATURE h239,h323
dnl MSWIN_DEFINE     h239,OPAL_H239
OPAL_SIMPLE_OPTION(  h239,OPAL_H239, [whether to enable H.239], [OPAL_H323])

dnl MSWIN_DISPLAY    h501,H.501
dnl MSWIN_IF_FEATURE h501,h323
dnl MSWIN_DEFINE     h501,OPAL_H501
OPAL_SIMPLE_OPTION(  h501,OPAL_H501, [whether to enable H.501], [OPAL_H323])

dnl T.120 is disabled
dnl _MSWIN_DISPLAY    t120,T.120
dnl _MSWIN_IF_FEATURE t120,h323
dnl _MSWIN_DEFINE     t120,OPAL_T120DATA
OPAL_SIMPLE_OPTION(   t120,OPAL_T120DATA, [whether to enable T.120], [OPAL_H323])

dnl MSWIN_DISPLAY  lid,Line Interface Device
dnl MSWIN_DEFINE   lid,OPAL_LID
OPAL_SIMPLE_OPTION(lid,OPAL_LID, [whether to enable LID support])

dnl MSWIN_DISPLAY  ivr,IVR support
dnl MSWIN_DEFINE   ivr,OPAL_IVR
OPAL_SIMPLE_OPTION(ivr,OPAL_IVR, [whether to enable IVR support], [OPAL_PTLIB_EXPAT], [OPAL_PTLIB_VXML])

dnl MSWIN_DISPLAY  rfc4175,RFC4175 Raw Video support
dnl MSWIN_DEFINE   rfc4175,OPAL_RFC4175
OPAL_SIMPLE_OPTION(rfc4175,OPAL_RFC4175, [whether to enable RFC4175 support], [OPAL_VIDEO])

dnl MSWIN_DISPLAY    rfc2435,RFC2435 JPEG Video support (experimental)
dnl MSWIN_DEFAULT    rfc2435,Disabled
dnl MSWIN_CHECK_FILE rfc2435,jpeglib.h,OPAL_RFC2435=1
dnl MSWIN_DIR_SYMBOL rfc2435,JPEGLIB_DIR
dnl MSWIN_CHECK_DIR  rfc2435,%ProgramFiles%\GnuWin32\include\
dnl MSWIN_CHECK_DIR  rfc2435,%ProgramW6432%\GnuWin32\include\
OPAL_SIMPLE_OPTION(rfc2435,OPAL_RFC2435, [whether to enable RFC2435 support (experimental)], [OPAL_VIDEO])

dnl MSWIN_DISPLAY  aec,Accoustic Echo Cancellation
dnl MSWIN_DEFINE   aec,OPAL_AEC
OPAL_SIMPLE_OPTION(aec,OPAL_AEC, [whether to enable accoustic echo cancellation support])

dnl MSWIN_DISPLAY  g711plc,Packet Loss Concealment for G.711
dnl MSWIN_DEFINE   g711plc,OPAL_G711PLC
OPAL_SIMPLE_OPTION(g711plc,OPAL_G711PLC, [whether to enable Packet Loss Concealment for G.711])

dnl MSWIN_DISPLAY  rtcpxr,RTCP Extended Reports support
dnl MSWIN_DEFINE   rtcpxr,OPAL_RTCP_XR
OPAL_SIMPLE_OPTION(rtcpxr,OPAL_RTCP_XR, [whether to enable RTCP Extended Reports support])

dnl MSWIN_DISPLAY  mixer,Media mixing
dnl MSWIN_DEFINE   mixer,OPAL_HAS_MIXER
OPAL_SIMPLE_OPTION(mixer,OPAL_HAS_MIXER, [whether to enable media mixing support])

dnl MSWIN_DISPLAY  pcss,PC Sound System Endpoint
dnl MSWIN_DEFINE   pcss,OPAL_HAS_PCSS
OPAL_SIMPLE_OPTION(pcss,OPAL_HAS_PCSS, [whether to enable PC sound system support])

dnl MSWIN_DISPLAY  script,Script language interface
dnl MSWIN_DEFINE   script,OPAL_SCRIPT
OPAL_SIMPLE_OPTION(script,OPAL_SCRIPT, [whether to enable script language support])


dnl ########################################################################
dnl Compile plugins and sample directories
dnl ########################################################################

OPAL_SIMPLE_OPTION([plugins],[OPAL_PLUGINS], [whether to enable plugin support])
if test "x$OPAL_PLUGINS" = "xyes" ; then
  AC_CONFIG_SUBDIRS(plugins)
fi

OPAL_SIMPLE_OPTION([samples],[OPAL_SAMPLES], [whether to enable samples build])

dnl Construct the plugins install path

AC_MSG_CHECKING(where plugins are being installed)
AC_ARG_WITH([plugin-installdir],
             AS_HELP_STRING([--with-plugin-installdir=DIR],[Location where plugins are installed, starting at the lib dir]),
             [PLUGIN_DIR="$withval"],
             [PLUGIN_DIR="opal-${OPAL_VERSION}"]
           )

EXPANDED_PLUGIN_DIR="${libdir}/${PLUGIN_DIR}"
if test "x${exec_prefix}" = "xNONE" ; then
  if test "x${prefix}" = "xNONE" ; then
    EXPANDED_PLUGIN_DIR=`echo ${EXPANDED_PLUGIN_DIR} | sed s#\\${exec_prefix}#/usr/local#`
  else
    EXPANDED_PLUGIN_DIR=`echo ${EXPANDED_PLUGIN_DIR} | sed s#\\${exec_prefix}#${prefix}#`
  fi
else
  EXPANDED_PLUGIN_DIR=`echo ${EXPANDED_PLUGIN_DIR} | sed s#\\${exec_prefix}#${exec_prefix}#`
fi

case "$target_os" in
  cygwin | mingw )
    EXPANDED_PLUGIN_DIR="plugins"
  ;;
esac

AC_DEFINE_UNQUOTED([OPAL_PLUGIN_DIR], ["${EXPANDED_PLUGIN_DIR}"], [Default plugin dir])
AC_MSG_RESULT([${EXPANDED_PLUGIN_DIR}])



dnl ########################################################################
dnl Output make directives
dnl ########################################################################

AC_SUBST(RELEASE_CFLAGS)
AC_SUBST(DEBUG_CFLAGS)

AC_SUBST(RELEASE_LIBS)
AC_SUBST(DEBUG_LIBS)
AC_SUBST(DEBUG_BUILD)

AC_SUBST(PKG_REQUIRES)
AC_SUBST(PKG_LIBS)
AC_SUBST(PKG_CFLAGS)
AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(LIBS)
AC_SUBST(LDFLAGS)
AC_SUBST(LDSOOPTS)
AC_SUBST(INSTALL)
AC_SUBST(AR)
AC_SUBST(ARCHIVE)
AC_SUBST(HAVE_RANLIB)


dnl ########################################################################
dnl Output files
dnl ########################################################################

cfg_prefix=$libdir
if test "$exec_prefix" = "NONE" ; then
  cfg_prefix=./include
fi

AC_CONFIG_FILES(make/opal_defs.mak)
AC_CONFIG_FILES(make/toplevel.mak)
AC_CONFIG_HEADERS(${cfg_prefix}/opal/buildopts.h:include/opal/buildopts.h.in)
AC_CONFIG_FILES(opal.pc)
AC_CONFIG_FILES(opal.spec)
AC_CONFIG_FILES(opal_cfg.dxy)

AC_CONFIG_FILES([src/codec/speex/libspeex/speex_config_types.h])
AC_OUTPUT
