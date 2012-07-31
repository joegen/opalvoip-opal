AC_INIT(configure.ac)

dnl extract recommended PTLIB version from parent directory
if test \! -e ../configure ; then
  echo "ERROR: unable to find '../configure'"
else
  PTLIB_REC_VERSION=`grep '^PTLIB_REC_VERSION=' ../configure | sed -e 's/^PTLIB_REC_VERSION=//' -e 's/\"//g'`
  echo PTLIB_REC_VERSION $PTLIB_REC_VERSION
  if test "x$PTLIB_REC_VERSION" = "x" ; then
    echo "ERROR: unable to extract PTLIB_REC_VERSION from ../configure"
    exit -1
  fi
fi

PKG_PROG_PKG_CONFIG()
AC_CANONICAL_TARGET()

m4_include(../opal.m4)

OPAL_FIND_PTLIB()

CFLAGS="$CFLAGS $PTLIB_CFLAGS"
CXXFLAGS="$CXXFLAGS $PTLIB_CXXFLAGS -D__STDC_CONSTANT_MACROS" 

AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_CHECK_TOOL([NM], [nm], [:])
AC_CHECK_TOOL([LDD], [ldd], [:])
AC_CHECK_TOOL([OBJDUMP], [objdump], [:])

AC_HEADER_STDC
AC_CHECK_HEADERS([stddef.h limits.h float.h fcntl.h])

dnl Enable compiler warnings
dnl if test "x$ac_cv_c_compiler_gnu" = xyes; then
dnl for later...
dnl  CFLAGS="$CFLAGS -Wall -Wextra -Winit-self -Wswitch-default -Wswitch-enum -Wstrict-aliasing=2 -Wfloat-equal -Wshadow -Wno-comment -Wno-missing-field-initializers -Wno-unused"
dnl  CXXFLAGS="$CXXFLAGS -Wall -Wextra -Winit-self -Wswitch-default -Wswitch-enum -Wstrict-aliasing=2 -Wfloat-equal -Wshadow -Wno-comment -Wno-missing-field-initializers  -Wno-unused"
dnl fi

dnl ########################################################################
dnl Directories
dnl ########################################################################

OPAL_DETERMINE_VERSION([..])
OPAL_DETERMINE_PLUGIN_DIR()

AC_PLUGIN_DIR="${PLUGIN_DIR}/codecs/audio"
VC_PLUGIN_DIR="${PLUGIN_DIR}/codecs/video"
LID_PLUGIN_DIR="${PLUGIN_DIR}/lid"
FAX_PLUGIN_DIR="${PLUGIN_DIR}/fax"

PLUGINDIR=`pwd`

AC_SUBST(prefix)
AC_SUBST(exec_prefix)
AC_SUBST(libdir)
AC_SUBST(OSTYPE)
AC_SUBST(MACHTYPE)
AC_SUBST(PLUGINDIR)
AC_SUBST(AC_PLUGIN_DIR)
AC_SUBST(VC_PLUGIN_DIR)
AC_SUBST(LID_PLUGIN_DIR)
AC_SUBST(FAX_PLUGIN_DIR)

dnl ########################################################################
dnl Platform specific section
dnl ########################################################################
WIN32=no
LDSO=
case "$target_os" in
       solaris*)
          CFLAGS="$CFLAGS -KPIC"
          LDSO='-Bdynamic -G -h $@'
          PLUGINEXT=so
          AC_CHECK_HEADERS([sys/time.h sys/file.h netinet/in.h])
          AC_FUNC_FORK()
        ;;

        darwin*)  
         CFLAGS="$CFLAGS -fno-common -dynamic"
          LDSO='-dynamiclib -multiply_defined suppress'
          CFLAGS="$CFLAGS -fPIC -D__MACOSX__"
          case "$target_cpu" in
            x86 | i686 | i586 | i486 | i386 ) CFLAGS="${CFLAGS} -m486" ;;
          esac
          PLUGINEXT=dylib 
          AC_CHECK_HEADERS([sys/time.h sys/file.h netinet/in.h])
          AC_FUNC_FORK()
        ;;

        cygwin*|mingw**)
          LDSO='-module -shared'
          PLUGINEXT=dll
          WIN32=yes
          AC_CHECK_HEADERS([malloc.h])
          AC_DEFINE([PLUGIN_CODEC_DLL_EXPORTS], [1], [For win32 build])
        ;;

        *)
          CFLAGS="$CFLAGS -fPIC"
          LDSO='-shared -Wl,-soname,$(SONAME)'
          PLUGINEXT=so
          AC_CHECK_HEADERS([sys/time.h sys/file.h netinet/in.h])
          AC_FUNC_FORK()
        ;;
esac
AC_SUBST(LDSO)
AC_SUBST(PLUGINEXT)
AC_SUBST(CFLAGS)
AC_SUBST(WIN32)

OPAL_FIND_LIBDL([HAVE_LIBDL=yes], [HAVE_LIBDL=no])
AC_SUBST(HAVE_LIBDL)
AC_SUBST(DL_LIBS)


dnl ########################################################################
dnl Defaults
dnl ########################################################################

HAVE_VPB=no
HAVE_IXJ=no
HAVE_SPANDSP=yes
DEFAULT_TO_FULL_CAPABILITIES=yes
LIBAVCODEC_STACKALIGN_HACK=no

SUBDIRS=""
AC_SUBST(SUBDIRS)


AC_MSG_CHECKING([if linker accepts -Wl,--no-as-needed])
AC_LANG_CONFTEST([AC_LANG_PROGRAM([[]],[[]])])
$CC -o conftest$ac_exeext $CPPFLAGS $CFLAGS $LDFLAGS -Wl,--no-as-needed conftest.$ac_ext > /dev/null 2>&1
if test $? = 0 ; then
   AC_MSG_RESULT(yes)
   NO_AS_NEEDED_LDFLAG="-Wl,--no-as-needed"
else
   AC_MSG_RESULT(no)
fi



dnl ########################################################################
dnl Audio Codecs
dnl ########################################################################

dnl #########

if test -d audio/GSM0610 ; then
  AC_ARG_ENABLE([gsm],
                [AC_HELP_STRING([--disable-gsm],[Disable GSM 06.10])],
                [HAVE_GSM0610=$enableval],
                [HAVE_GSM0610=yes])
else
  HAVE_GSM0610=no
fi
if test "x${HAVE_GSM0610}" = "xyes" ; then
  AC_CONFIG_FILES(audio/GSM0610/Makefile)
  SUBDIRS+=" audio/GSM0610"

  AC_ARG_ENABLE([localgsm],
                [AC_HELP_STRING([--enable-localgsm],[Force use local version of GSM library rather than system version])],
                [localgsm=$enableval],
                [localgsm=])

  if test "x${localgsm}" = "xyes" ; then
    AC_MSG_NOTICE(Forcing use of local GSM sources)
    GSM_SYSTEM=no

  elif test "x${localgsm}" = "xno" ; then
    AC_MSG_NOTICE(Forcing use of system GSM library)
    OPAL_FIND_GSM([HAVE_GSM=yes], [HAVE_GSM=no])
    if test "x${HAVE_GSM}" != "xyes" ; then
      AC_MSG_ERROR([cannot find system GSM])
    else
      GSM_SYSTEM=yes
    fi

  else
    OPAL_FIND_GSM([HAVE_GSM=yes], [HAVE_GSM=no])
    if test "x${HAVE_GSM}" = "xyes" ; then
      GSM_SYSTEM=yes
    else
      GSM_SYSTEM=no
    fi
  fi

  if test "x${GSM_SYSTEM}" = "xyes" ; then
    AC_SUBST(GSM_CFLAGS)
    AC_SUBST(GSM_LIBS)
    AC_MSG_RESULT(using system GSM)
  fi
  AC_SUBST(GSM_SYSTEM)
fi


dnl #########

if test -d audio/gsm-amr ; then
  AC_ARG_ENABLE([gsmamr],
                [AC_HELP_STRING([--disable-gsmamr],[Disable GSM-AMR])],
                [HAVE_GSMAMR=$enableval],
                [HAVE_GSMAMR=yes])
else
  HAVE_GSMAMR=no
fi
if test "x${HAVE_GSMAMR}" = "xyes" ; then
  AC_CONFIG_FILES(audio/gsm-amr/Makefile)
  SUBDIRS+=" audio/gsm-amr"
fi

dnl #########

if test -d audio/iLBC ; then
  AC_ARG_ENABLE([iLBC],
                [AC_HELP_STRING([--disable-iLBC],[Disable iLBC])],
                [HAVE_iLBC=$enableval],
                [HAVE_iLBC=yes])
else
  HAVE_iLBC=no
fi
if test "x${HAVE_iLBC}" = "xyes" ; then
  AC_CONFIG_FILES(audio/iLBC/Makefile)
  SUBDIRS+=" audio/iLBC"

  OPAL_DETERMINE_ILBC()
  AC_SUBST(ILBC_SYSTEM)
  AC_SUBST(ILBC_CFLAGS)
  AC_SUBST(ILBC_LIBS)
fi

dnl #########

if test -d audio/Speex ; then
  AC_ARG_ENABLE([speex],
                [AC_HELP_STRING([--disable-speex],[Disable Speex])],
                [HAVE_SPEEX=$enableval],
                [HAVE_SPEEX=yes])
else
  HAVE_SPEEX=no
fi
if test "x${HAVE_SPEEX}" = "xyes" ; then
  AC_OUTPUT(audio/Speex/libspeex/speex_config_types.h)
  AC_CONFIG_FILES(audio/Speex/Makefile)
  SUBDIRS+=" audio/Speex"

  OPAL_SPEEX_TYPES()
  AC_SUBST(SIZE16)
  AC_SUBST(SIZE32)
  OPAL_DETERMINE_SPEEX()
  AC_SUBST(SPEEX_SYSTEM)
  AC_SUBST(SPEEX_CFLAGS)
  AC_SUBST(SPEEX_LIBS)
fi

dnl #########

if test -d audio/G722 ; then
  AC_ARG_ENABLE([g.722],
                [AC_HELP_STRING([--disable-g.722],[Disable G.722])],
                [HAVE_G722=$enableval],
                [HAVE_G722=yes])
else
  HAVE_G722=no
fi
if test "x${HAVE_G722}" = "xyes" ; then
  AC_CONFIG_FILES(audio/G722/Makefile)
  SUBDIRS+=" audio/G722"
fi

dnl #########

if test -d audio/G.722.1 ; then
  AC_ARG_ENABLE([g.722.1],
                [AC_HELP_STRING([--disable-g.722.1],[Disable G.722.1])],
                [HAVE_G7221=$enableval],
                [HAVE_G7221=yes])
else
  HAVE_G7221=no
fi
if test "x${HAVE_G7221}" = "xyes" ; then
  AC_CONFIG_FILES(audio/G.722.1/Makefile)
  SUBDIRS+=" audio/G.722.1"
fi

dnl #########

if test -d audio/G.722.2 ; then
  AC_ARG_ENABLE([g.722.2],
                [AC_HELP_STRING([--disable-g.722.2],[Disable G.722.2])],
                [HAVE_G7222=$enableval],
                [HAVE_G7222=yes])
else
  HAVE_G7222=no
fi
if test "x${HAVE_G7222}" = "xyes" ; then
  AC_CONFIG_FILES(audio/G.722.2/Makefile)
  SUBDIRS+=" audio/G.722.2"
fi

dnl #########

if test -d audio/G726 ; then
  AC_ARG_ENABLE([g.726],
                [AC_HELP_STRING([--disable-g.726],[Disable G.726])],
                [HAVE_G726=$enableval],
                [HAVE_G726=yes])
else
  HAVE_G726=no
fi
if test "x${HAVE_G726}" = "xyes" ; then
  AC_CONFIG_FILES(audio/G726/Makefile)
  SUBDIRS+=" audio/G726"
fi

dnl #########

if test -d audio/LPC_10 ; then
  AC_ARG_ENABLE([lpc10],
                [AC_HELP_STRING([--disable-lpc10],[Disable LPC-10])],
                [HAVE_LPC10=$enableval],
                [HAVE_LPC10=yes])
else
  HAVE_LPC10=no
fi
if test "x${HAVE_LPC10}" = "xyes" ; then
  AC_CONFIG_FILES(audio/LPC_10/Makefile)
  SUBDIRS+=" audio/LPC_10"
fi

dnl #########

if test -d audio/IMA_ADPCM ; then
  AC_ARG_ENABLE([ima-adpcm],
                [AC_HELP_STRING([--disable-ima-adpcm],[Disable IMA ADPCM])],
                [HAVE_IMA_ADPCM=$enableval],
                [HAVE_IMA_ADPCM=yes])
else
  HAVE_IMA_ADPCM=no
fi
if test "x${HAVE_IMA_ADPCM}" = "xyes" ; then
  AC_CONFIG_FILES(audio/IMA_ADPCM/Makefile)
  SUBDIRS+=" audio/IMA_ADPCM"
fi

dnl #########
dnl sbc,Bluetooth SBC for telephony with CD quality

if test -d audio/SBC -a -d audio/SBC/bluez_sbc ; then
  AC_ARG_ENABLE([sbc],
                [AC_HELP_STRING([--disable-sbc],[Disable Bluetooth SBC])],
                [HAVE_SBC=$enableval],
                [HAVE_SBC=yes])
  if test "x${HAVE_SBC}" = "xyes" ; then
    AC_CHECK_LIB(samplerate,src_get_version,has_samplerate=yes,has_samplerate=no)
    HAVE_SBC=$has_samplerate
  fi
else
  HAVE_SBC=no
fi
if test "x${HAVE_SBC}" = "xyes" ; then
  AC_CONFIG_FILES(audio/SBC/Makefile)
  SUBDIRS+=" audio/SBC"
fi

dnl #########

if test -d audio/celt ; then
  AC_ARG_ENABLE([celt],
                [AC_HELP_STRING([--disable-celt],[Disable CELT])],
                [HAVE_CELT=$enableval],
                [HAVE_CELT=yes])
  PKG_CHECK_MODULES([CELT],
                    [celt],
                    [
                      AC_SUBST(CELT_CFLAGS)
                      AC_SUBST(CELT_LIBS)
                    ],
                    [
                      HAVE_CELT=no
                    ])
else
  HAVE_CELT=no
fi
if test "x${HAVE_CELT}" = "xyes" ; then
  CELT_CFLAGS="${CELT_CFLAGS} -DCELT_VERSION=\"`pkg-config --modversion celt | \
                  sed -e "s/\./*10000+/" -e "s/\./*100+/"`\""
  AC_CONFIG_FILES(audio/celt/Makefile)
  SUBDIRS+=" audio/celt"
fi

dnl #########

if test -d audio/SILK ; then
  AC_ARG_ENABLE([silk],
                [AC_HELP_STRING([--disable-silk],[Disable SILK])],
                [HAVE_SILK=$enableval],
                [HAVE_SILK=yes])
else
  HAVE_SILK=no
fi
if test "x${HAVE_SILK}" = "xyes" ; then
  AC_CONFIG_FILES(audio/SILK/Makefile)
  SUBDIRS+=" audio/SILK"
fi


dnl ########################################################################
dnl Video Codecs
dnl ########################################################################

if test -d video/H.261-vic ; then
  AC_ARG_ENABLE([h.261],
                [AC_HELP_STRING([--disable-h.261],[Disable H.261 (vic)])],
                [HAVE_H261=$enableval],
                [HAVE_H261=yes])
else
  HAVE_H261=no
fi
if test "x${HAVE_H261}" = "xyes" ; then
  AC_CONFIG_FILES(video/H.261-vic/Makefile)
  SUBDIRS+=" video/H.261-vic"
fi


dnl     ########################
dnl     Capabilities option
dnl     ########################
OPAL_SIMPLE_OPTION([default-to-full-capabilties],[DEFAULT_TO_FULL_CAPABILITIES], [default to full capabilities at empty fmtp lines])

dnl     ########################
dnl     libavcodec (ffmpeg)
dnl     ########################
AC_ARG_ENABLE([libavcodec],
              [AC_HELP_STRING([--disable-libavcodec],[disable libavcodec support])],
              [HAVE_LIBAVCODEC=$enableval])

if test "x$HAVE_LIBAVCODEC" = "xno" ; then
  AC_MSG_RESULT(Manually disabled libavcodec support)
else
  PKG_CHECK_MODULES([LIBAVCODEC],
                    [libavcodec >= 51.11.0],
                    [
                     AC_SUBST(LIBAVCODEC_CFLAGS)
                     AC_SUBST(LIBAVCODEC_LIBS)
                     AC_DEFINE([HAVE_LIBAVCODEC], [1], [ffmpeg])
                     OPAL_GET_LIBNAME([LIBAVCODEC], [libavcodec], [$LIBAVCODEC_LIBS])
                     AC_SUBST(LIBAVCODEC_LIB_NAME)
                     OPAL_LIBAVCODEC_HACK
                     case "$target_os" in
                        cygwin* | mingw* ) 
                                           LIBAVCODEC_LIB_NAME="avcodec.dll"
                                           ;;
                     esac
                     HAVE_LIBAVCODEC=yes
                    ],
                    [
                     HAVE_LIBAVCODEC=no
                    ]
                   )
fi

if test "x$HAVE_LIBAVCODEC" = "xyes" ; then
  OPAL_LIBAVCODEC_SOURCE()

  PKG_CHECK_MODULES(
    [LIBAVUTIL],
    [libavutil],
    [
      AC_SUBST(LIBAVUTIL_CFLAGS)
      AC_SUBST(LIBAVUTIL_LIBS)
      OPAL_GET_LIBNAME([LIBAVUTIL], [libavutil], [$LIBAVUTIL_LIBS])
      AC_SUBST(LIBAVUTIL_LIB_NAME)
    ]
  )

  AC_MSG_CHECKING(if libavcodec has rtp_mode);
  AC_COMPILE_IFELSE(
    [
      AC_LANG_PROGRAM(
        [[#include "libavcodec/avcodec.h"]],
        [[AVCodecContext c; c.rtp_mode = 1; ]]
      )
    ],
    [
      HAVE_LIBAVCODEC_RTP_MODE=1
      AC_MSG_RESULT(yes);
    ],
    [
      AC_MSG_RESULT(no)
    ]
  )

  AC_MSG_CHECKING(if libavutil has AV_PICTURE_TYPE_NONE);
  AC_COMPILE_IFELSE(
    [
      AC_LANG_PROGRAM(
        [[#include "libavcodec/avcodec.h"]],
        [[enum AVPictureType t = AV_PICTURE_TYPE_NONE; ]]
      )
    ],
    [
      AC_MSG_RESULT(yes);
    ],
    [
      AC_MSG_RESULT(no)
      AC_MSG_CHECKING(if libavutil has AVPictureType);
      AC_COMPILE_IFELSE(
        [
          AC_LANG_PROGRAM(
            [[#include "libavcodec/avcodec.h"]],
            [[enum AVPictureType t; ]]
          )
        ],
        [
          AC_MSG_RESULT(yes);
          AC_DEFINE([AV_PICTURE_TYPE_NONE], [(AVPictureType)0])
        ],
        [
          AC_MSG_RESULT(no)
          AC_DEFINE([AV_PICTURE_TYPE_NONE], [0])
        ]
      )
    ]
  )
fi

AC_SUBST(HAVE_LIBAVCODEC)
AC_SUBST(LIBAVCODEC_SOURCE_DIR)
AC_SUBST(HAVE_LIBAVCODEC_RTP_MODE)

if test "x${HAVE_LIBAVCODEC}" = "xyes"; then
  AC_CONFIG_FILES(video/H.263-1998/Makefile)
  SUBDIRS+=" video/H.263-1998"
fi

if test "x${HAVE_LIBAVCODEC}" = "xyes"; then
  AC_CONFIG_FILES(video/MPEG4-ffmpeg/Makefile)
  SUBDIRS+=" video/MPEG4-ffmpeg"
fi


dnl     ########################
dnl     x264
dnl     ########################
HAVE_H264=no
IS_H264_LICENSED=no
AC_SUBST(IS_H264_LICENSED)

if test "x${HAVE_LIBAVCODEC}" = "xno"; then
  AC_MSG_RESULT(H.264 not built since libavcodec support is disabled)
else
  AC_ARG_ENABLE([x264],
                [AC_HELP_STRING([--disable-x264],[disable x264 support])],
                [HAVE_H264=$enableval],
                [PKG_CHECK_MODULES([X264],
                                   [x264 >= 0.102],
                                   [
				     HAVE_H264=yes
                                     AC_SUBST(X264_CFLAGS)
                                     AC_SUBST(X264_LIBS)
                                     OPAL_GET_LIBNAME([X264], [libx264], [$X264_LIBS])
                                     AC_SUBST(X264_LIB_NAME)
                                     OPAL_X264_LINKAGE
                                   ],
                                   [
                                     HAVE_H264=no
                                   ])
                ])
fi

if test "x$HAVE_H264" = "xyes" ; then
  AC_ARG_ENABLE([x264-licensed],
                [AC_HELP_STRING([--enable-x264-licensed],[enable commercially licensed x264 support])],
                [IS_H264_LICENSED=$enableval])

  SUBDIRS+=" video/H.264"
  AC_CONFIG_FILES(video/H.264/Makefile)
  AC_CONFIG_FILES(video/H.264/gpl/Makefile)
fi


dnl     ########################
dnl     theora
dnl     ########################
if test -d video/THEORA ; then
  AC_ARG_ENABLE([theora],
                [AC_HELP_STRING([--disable-theora],[disable Theora support])],
                [HAVE_THEORA=$enableval],
                [HAVE_THEORA=yes])
  PKG_CHECK_MODULES([THEORA],
                    [theora],
                    [
                     AC_SUBST(THEORA_CFLAGS)
                     AC_SUBST(THEORA_LIBS)
                    ],
                    [
                     HAVE_THEORA=no
                    ])
else
  HAVE_THEORA=no
fi
if test "x$HAVE_THEORA" = "xyes" ; then
  AC_CONFIG_FILES(video/THEORA/Makefile)
  SUBDIRS+=" video/THEORA"
fi


dnl ########################################################################
dnl LID Devices
dnl ########################################################################

dnl     ########################
dnl     Quicknet xJACK cards
dnl     #######################
if test -d LID/IxJ ; then
  AC_ARG_ENABLE([ixj],
                [AC_HELP_STRING([--enable-ixj],[enable xJack cards])],
                [HAVE_IXJ=$enableval])
  if test "x$HAVE_IXJ" = "xyes" ; then
    OPAL_MSG_CHECK([IXJ], [$HAVE_IXJ])
    if test "x$HAVE_IXJ" = "xyes" ; then
      AC_CHECK_HEADERS([linux/telephony.h], 
                     [HAVE_IXJ=yes],
                     [AC_CHECK_HEADERS([ sys/telephony.h], [HAVE_IXJ=yes], [HAVE_IXJ=no])])
      if test "x$HAVE_IXJ" = "xyes" ; then
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <linux/ixjuser.h>]], [[ return 0; ]])],
                          [HAVE_IXJ=yes], [HAVE_IXJ=no])
      fi
    fi
  fi
  OPAL_MSG_CHECK([IXJ], [$HAVE_IXJ])
else
  HAVE_IXJ=no
fi

if test "x$HAVE_IXJ" = "xyes" ; then
  AC_CONFIG_FILES(LID/IxJ/Makefile)
  SUBDIRS+=" LID/IxJ"
fi

dnl     ########################
dnl     VoiceTronics VPB card
dnl     ########################
if test -d LID/VPB ; then
  OPAL_SIMPLE_OPTION([vpb],[HAVE_VPB], [enable voicetronix vpb])
fi
if test "x$HAVE_VPB" = "xyes" ; then
  AC_CONFIG_FILES(LID/VPB/Makefile)
  SUBDIRS+=" LID/VPB"
fi



dnl ########################################################################
dnl FAX Devices
dnl ########################################################################

dnl     ########################
dnl     spanDSP
dnl     ########################
if test -d fax/fax_spandsp ; then
  AC_ARG_ENABLE([spandsp],
                [AC_HELP_STRING([--enable-spandsp],[enable spanDSP-based fax support])],
                [HAVE_SPANDSP=$enableval])
  if test "x${HAVE_SPANDSP}" = "xyes" ; then
    OPAL_FIND_SPANDSP([HAVE_SPANDSP=yes], [HAVE_SPANDSP=no])
  fi
else
  HAVE_SPANDSP=no
fi

if test "x$HAVE_SPANDSP" = "xyes" ; then
  AC_CONFIG_FILES(fax/fax_spandsp/Makefile)
  SUBDIRS+=" fax/fax_spandsp"
fi


dnl ########################################################################
dnl Various checks
dnl ########################################################################

AC_LANG_PUSH([C++])
AC_MSG_CHECKING(for posix_memalign)
AC_COMPILE_IFELSE([
  AC_LANG_PROGRAM([[#include <stdlib.h>]], 
                  [[
                  void * p; posix_memalign(&p, 16, 16); return 0; 
                  ]]
                  )
                  ],
  AC_MSG_RESULT(yes)
  AC_DEFINE(HAVE_POSIX_MEMALIGN, 1, [Is posix_memalign supported]),
  AC_MSG_RESULT(no)
)
AC_LANG_POP([C++])

dnl ########################################################################

AC_CONFIG_FILES(Makefile)

dnl ########################################################################
dnl Header file

dnl ########################################################################
dnl Output configured files

AC_CONFIG_FILES(plugin-inc.mak)
AC_CONFIG_HEADERS(plugin-config.h)
AC_OUTPUT()

dnl ########################################################################
dnl Summary
dnl ########################################################################
echo ""
echo "=================== Plugin configuration ======================"
echo " prefix                           : $prefix"
echo " exec_prefix                      : $exec_prefix"
echo " libdir                           : $libdir"
echo " Audio Codec plugin install dir   : "'${libdir}'"/$AC_PLUGIN_DIR"
echo " Video Codec plugin install prefix: "'${libdir}'"/$VC_PLUGIN_DIR"
echo " LID plugin install prefix        : "'${libdir}'"/$LID_PLUGIN_DIR"
echo " FAX plugin install prefix        : "'${libdir}'"/$FAX_PLUGIN_DIR"

echo ""

if test "x${HAVE_GSM0610}" = "xno" ; then
echo "                              GSM :  no"
elif test "x${GSM_SYSTEM}" = "xyes" ; then
echo "                              GSM :  yes (system)"
else
echo "                              GSM :  yes (internal)"
fi

echo "                          GSM-AMR :  ${HAVE_GSMAMR}"

if test "x${HAVE_iLBC}" = "xno" ; then
echo "                             iLBC :  no"
elif test "x${ILBC_SYSTEM}" = "xyes" ; then
echo "                             iLBC :  yes (system)"
else
echo "                             iLBC :  yes (internal)"
fi

if test "x${HAVE_SPEEX}" = "xno" ; then
echo "                            Speex :  no"
elif test "x${SPEEX_SYSTEM}" = "xyes" ; then
echo "                            Speex :  yes (system, version $SPEEX_SYSTEM_VERSION)"
else
echo "                            Speex :  yes (internal, version $SPEEX_INTERNAL_VERSION)"
fi

echo "                            G.722 :  ${HAVE_G722}"
echo "                          G.722.1 :  ${HAVE_G7221}"
echo "                          G.722.2 :  ${HAVE_G7222}"
echo "                            G.726 :  ${HAVE_G726}"
echo "                            LPC10 :  ${HAVE_LPC10}"
echo "                        IMA_ADPCM :  ${HAVE_IMA_ADPCM}"
echo "                              SBC :  ${HAVE_SBC}"
echo "                             CELT :  ${HAVE_CELT}"
echo "                             SILK :  ${HAVE_SILK}"

echo ""

echo "                           H.261  :  ${HAVE_H261} (vic)"
echo "                     H.263/H.263+ :  ${HAVE_LIBAVCODEC} (ffmpeg)"

if test "x${HAVE_H264}" = "xno" ; then
echo "                           H.264  :  no"
elif test "x${IS_H264_LICENSED}" = "xno" ; then
echo "                           H.264  :  yes (GPL x264)"
else
echo "                           H.264  :  yes (Licensed x264)"
fi

echo "                          THEORA  :  ${HAVE_THEORA}"

if test "x${HAVE_LIBAVCODEC}" = "xyes" ; then
if test "x${LIBAVCODEC_SOURCE_DIR}" = "x" ; then
echo "                    MPEG4 Part 2  :  yes (no rate control correction)"
else
echo "                    MPEG4 Part 2  :  yes"
fi
else
echo "                    MPEG4 Part 2  :  no"
fi

echo ""
echo "                     SpanDSP FAX  :  $HAVE_SPANDSP"

echo ""

echo "           Quicknet xJACK support :  $HAVE_IXJ"
echo "         Voicetronics VPB support :  $HAVE_VPB"

echo ""

echo "                         OS Type  :  $target_os"
echo "                    Machine Type  :  $target_cpu"
echo ""
echo " If all settings are OK, type make and make install "
echo "========================================================"
