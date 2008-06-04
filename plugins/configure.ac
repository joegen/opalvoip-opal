AC_INIT(configure.ac)
AC_PROG_CXX
AC_HEADER_STDC

PKG_PROG_PKG_CONFIG()

dnl ########################################################################
dnl set the PREFIX accordingly
if test "x$prefix" = "xNONE"; then
   INSTALLPREFIX="/usr/local"
else
   INSTALLPREFIX="${prefix}"
fi

AC_SUBST(INSTALLPREFIX)

if test \! -e ./aclocal.m4 ; then
  echo "ERROR: you must run 'aclocal' before 'autoconf'"
  exit
fi


dnl ########################################################################
dnl set LIBDIR accordingly
LIBDIR="${libdir}"

AC_SUBST(LIBDIR)

dnl ########################################################################
AC_CANONICAL_TARGET()
LDSO=
PLUGINEXT=so
STDCCFLAGS=-fPIC
case "$target_os" in
	solaris*) LDSO="-Bdynamic -G -h" ;;
        darwin*)  
	  CFLAGS="-fno-common -dynamic"
          LDSO="-dynamiclib -multiply_defined suppress"
	  STDCCFLAGS="$STDCCFLAGS -D__MACOSX__"
	  case "$target_cpu" in
	   x86 | i686 | i586 | i486 | i386 ) STDCCFLAGS="${STDCCFLAGS} -m486" ;;
          esac
	  PLUGINEXT=dylib 
        ;;
        *)LDSO='-shared -Wl,-soname,$(SONAME)';;
esac
AC_SUBST(LDSO, ${LDSO})
AC_SUBST(PLUGINEXT, ${PLUGINEXT})
AC_SUBST(STDCCFLAGS, ${STDCCFLAGS})

dnl ########################################################################
dnl Macro Definition Section
dnl ########################################################################

AC_DEFUN([OPAL_GET_LIBNAME],
         [
dnl check macro arguments
          AC_MSG_CHECKING(filename of $2 library)
          AC_LANG_CONFTEST([int main () {}])
          $CC -o conftest$ac_exeext $CFLAGS $CPPFLAGS $LDFLAGS conftest.$ac_ext $LIBS $3>&AS_MESSAGE_LOG_FD
          if test \! -x conftest$ac_exeext ; then
            AC_MSG_RESULT(cannot determine - using defaults)
          else
            $1_LIB_NAME=`ldd ./conftest | grep $2 | awk '{print @S|@1; }'`
            AC_MSG_RESULT($$1_LIB_NAME)
            AC_DEFINE_UNQUOTED([$1_LIB_NAME], ["$$1_LIB_NAME"], [Filename of the $2 library])
            AC_SUBST([$1_LIB_NAME], ["$$1_LIB_NAME"])
          fi
         ])

dnl     ########################
dnl     LIBAVCODEC
dnl     ########################
AC_DEFUN([OPAL_LIBAVCODEC_HACK],
         [
          AC_ARG_ENABLE([libavcodec-stackalign-hack],
                        [AC_HELP_STRING([--enable-libavcodec-stackalign-hack], [Stack alignment hack for libavcodec library])],
                        [LIBAVCODEC_STACKALIGN_HACK=$enableval],
                        [LIBAVCODEC_STACKALIGN_HACK=no])
          if test x$LIBAVCODEC_STACKALIGN_HACK = xyes; then
            AC_MSG_NOTICE(libavcodec stack align hack enabled)
            AC_DEFINE([LIBAVCODEC_STACKALIGN_HACK], [1], [Stack alignment hack for libavcodec library])
          else
            AC_MSG_NOTICE(libavcodec stack align hack disabled)
          fi
         ])

AC_DEFUN([OPAL_LIBAVCODEC_SOURCE],
         [
          AC_MSG_CHECKING(libavcodec source)
          AC_ARG_WITH([libavcodec-source-dir],
                      [AC_HELP_STRING([--with-libavcodec-source-dir],[Directory with libavcodec source code, for MPEG4 rate control correction])])
          if test -f "$with_libavcodec_source_dir/libavcodec/avcodec.h"
          then
            AC_MSG_RESULT(enabled)
            LIBAVCODEC_SOURCE_DIR="$with_libavcodec_source_dir"
            AC_DEFINE([LIBAVCODEC_HAVE_SOURCE_DIR], [1], [Directory with libavcodec source code, for MPEG4 rate control correction])
          else
            LIBAVCODEC_SOURCE_DIR=
            AC_MSG_RESULT(disabled)
          fi
          AC_SUBST(LIBAVCODEC_SOURCE_DIR)
         ])

AC_DEFUN([OPAL_LIBAVCODEC_HEADER],
         [LIBAVCODEC_HEADER=
          old_CFLAGS="$CFLAGS"
          CFLAGS="$CFLAGS $LIBAVCODEC_CFLAGS"
          AC_CHECK_HEADER([libavcodec/avcodec.h], 
                          [
                           AC_DEFINE([LIBAVCODEC_HEADER], 
                                     ["libavcodec/avcodec.h"],
                                     [The libavcodec header file])
                           LIBAVCODEC_HEADER="libavcodec/avcodec.h"
                          ],
                          [])
          if test x$LIBAVCODEC_HEADER = x; then
            AC_CHECK_HEADER([ffmpeg/avcodec.h], 
                            [
                             AC_DEFINE([LIBAVCODEC_HEADER], 
                                       ["ffmpeg/avcodec.h"],
                                       [The libavcodec header file])
                             LIBAVCODEC_HEADER="ffmpeg/avcodec.h"
                            ])
          fi
          if test x$LIBAVCODEC_HEADER = x; then
            AC_MSG_ERROR([Cannot find libavcodec header file])
          fi
          CFLAGS="$old_CFLAGS"
         ])

dnl     ########################
dnl     x264
dnl     ########################
AC_DEFUN([OPAL_X264_LINKAGE],
         [
          AC_ARG_ENABLE([x264-link-static],
                        [AC_HELP_STRING([--enable-x264-link-static], [Statically link x264 to the plugin. Default for win32.])],
                        [X264_LINK_STATIC=$enableval],
                        [X264_LINK_STATIC=no])
          if test x$X264_LINK_STATIC = xyes; then
            AC_MSG_NOTICE(x264 static linking enabled)
            AC_DEFINE([X264_LINK_STATIC], 
                      [1],
                      [Statically link x264 to the plugin. Default for win32.])
          else
            AC_MSG_NOTICE(x264 static linking disabled)
          fi
         ])

dnl     ########################
dnl     speex
dnl     ########################
AC_DEFUN([OPAL_SPEEX_TYPES],
         [
          AC_CHECK_SIZEOF(short)
          AC_CHECK_SIZEOF(int)
          AC_CHECK_SIZEOF(long)
          AC_CHECK_SIZEOF(long long)

          case 2 in
                  $ac_cv_sizeof_short) SIZE16="short";;
                  $ac_cv_sizeof_int) SIZE16="int";;
          esac

          case 4 in
                  $ac_cv_sizeof_int) SIZE32="int";;
                  $ac_cv_sizeof_long) SIZE32="long";;
                  $ac_cv_sizeof_short) SIZE32="short";;
          esac

          AC_SUBST(SIZE16)
          AC_SUBST(SIZE32)
         ])

dnl ########################################################################
dnl look for amr-nb_float

H323_GSMAMR_NB_FLOAT=0
if test -r "./audio/gsm-amr/src/interf_dec.h" ; then
  AC_MSG_NOTICE(Enabling GSM-AMR plugin)
  H323_GSMAMR_NB_FLOAT=1
else
  AC_MSG_NOTICE(GSM-AMR plugin disabled (no 3GPP source))
fi
AC_SUBST(H323_GSMAMR_NB_FLOAT, $H323_GSMAMR_NB_FLOAT)


dnl ########################################################################
dnl Look for system GSM
dnl
dnl Note that we can only use llibgsm if it is compiled with WAV49
dnl

H323_SYSTEM_GSM=0
localgsm="xxx"
AC_ARG_ENABLE(localgsm,
       [  --enable-localgsm         use local version of GSM library rather than system version],
       localgsm=$enableval)

if test "${localgsm}" = "yes" ; then
  AC_MSG_NOTICE(Forcing use of local GSM sources)

elif test "${localgsm}" = "no" ; then
  AC_MSG_NOTICE(Forcing use of system GSM library)
  H323_SYSTEM_GSM=1

else
  AC_CHECK_LIB(gsm, gsm_create, GSM=1)
  if test "x$GSM" != "x"; then
    AC_MSG_CHECKING(if system GSM library has WAV49)
    OLD_LIBS=$LIBS
    LIBS="-lgsm"
    AC_RUN_IFELSE(
    [AC_LANG_PROGRAM([[
    #include <gsm.h>
]],[[
    int option = 0;
    gsm handle = gsm_create();
    return (gsm_option(handle, GSM_OPT_WAV49, &option) == -1) ? 1 : 0;
]])], GSM=2) 
    if test "x$GSM" == "x2"; then
      H323_SYSTEM_GSM=1
      AC_MSG_RESULT(yes)
      AC_MSG_NOTICE(Using system gsm)
    else
      AC_MSG_RESULT(no)
    fi
    LIBS=$OLD_LIBS
  fi
fi
AC_SUBST(H323_SYSTEM_GSM, $H323_SYSTEM_GSM)
AC_DEFINE_UNQUOTED(H323_SYSTEM_GSM, $H323_SYSTEM_GSM)

dnl     ########################
dnl     speex (system/local)
dnl     ########################

OPAL_SPEEX_TYPES()

AC_ARG_ENABLE([localspeex],
              [AC_HELP_STRING([--enable-localspeex],[Use local version of Speex library rather than system version])],
              [localspeex=$enableval],
              [localspeex=])

AC_MSG_CHECKING(internal Speex version)
SPEEX_INTERNAL_VERSION=`grep "#define SPEEX_VERSION" ./audio/Speex/libspeex/misc.h | sed -e 's/^.*speex\-//' -e 's/\".*//'`
AC_MSG_RESULT($SPEEX_INTERNAL_VERSION)

if test "x${localspeex}" = "xyes" ; then
  AC_MSG_NOTICE(forcing use of local Speex sources)
  H323_SYSTEM_SPEEX=

elif test "x${localspeex}" = "xno" ; then
  AC_MSG_NOTICE(forcing use of system Speex library)
  PKG_CHECK_MODULES([SPEEX],
                    [speex],
                    [H323_SYSTEM_SPEEX=1],
                    [
                     AC_MSG_ERROR([cannot find system speex])
                    ])

else

  AC_MSG_NOTICE(checking whether system Speex or internal Speex is more recent)
  PKG_CHECK_MODULES([SPEEX],
                    [speex >= $SPEEX_INTERNAL_VERSION],
                    [H323_SYSTEM_SPEEX=1],
                    [
                     H323_SYSTEM_SPEEX=
                     AC_MSG_RESULT(internal Speex version is more recent than system Speex or system Speex not found)
                    ])
fi

if test "x${H323_SYSTEM_SPEEX}" = "x1" ; then
  AC_SUBST(SPEEX_CFLAGS)
  AC_SUBST(SPEEX_LIBS)
  SPEEX_SYSTEM_VERSION=`$PKG_CONFIG speex --modversion`
  AC_MSG_RESULT(using system Speex version $SPEEX_SYSTEM_VERSION)
fi

AC_SUBST(H323_SYSTEM_SPEEX)

dnl H323_SYSTEM_SPEEX -> SPEEX_SYSTEM

dnl ########################################################################
dnl
dnl ffmpeg for H.263
dnl 

HAS_FFMPEG_H263=1
AC_SUBST(HAS_FFMPEG_H263)


dnl ########################################################################
dnl
dnl Quicknet xJACK cards
dnl 

HAS_IXJ=0
case "$target_os" in
        darwin*) ;;
        *)
          AC_CHECK_HEADERS(linux/telephony.h sys/telephony.h /usr/local/include/sys/telephony.h, HAS_IXJ=1)
        ;;
esac
if test "x$HAS_IXJ" != "x" ; then
  AC_MSG_CHECKING(if ixjuser.h actually compiles)
  AC_TRY_COMPILE([#include <linux/ixjuser.h>],
                 [int main(int ac,char **av) { return 0; }], HAS_IXJ=1, HAS_IXJ=)
  if test "x$HAS_IXJ" != "x" ; then
    AC_MSG_RESULT(yes)
    HAS_IXJ=1
  else
    AC_MSG_RESULT(no)
  fi
fi
AC_SUBST(HAS_IXJ)


dnl ########################################################################
dnl
dnl VoiceTronics VPB card
dnl 

HAS_VPB=0
AC_MSG_CHECKING(Voicetronix vpb)
case "$target_os" in
        darwin*) vpb=no ;;
        *)
          vpb=yes
      	  AC_ARG_ENABLE(vpb,
	       [  --enable-vpb            enable voicetronix vpb],
	       vpb=$enableval)
          ;;
esac

if test "$vpb" = "yes" ; then
  HAS_VPB=1
  AC_MSG_RESULT(enabled)
else
  AC_MSG_RESULT(disabled)
fi
AC_SUBST(HAS_VPB)


dnl ########################################################################
dnl
dnl CAPI
dnl 

AC_CHECK_HEADERS(capi20.h, HAS_CAPI20_H=1)
HAS_CAPI=0
AC_MSG_CHECKING(CAPI)
case "$target_os" in
        darwin*) capi=no ;;
        *)
          capi=yes
	  AC_ARG_ENABLE(capi,
	       [  --enable-capi            enable CAPI],
	       capi=$enableval)
          ;;
esac

if test "$capi" = "yes" ; then
  if test "x$HAS_CAPI20_H" != "x" ; then
    HAS_CAPI=1
    AC_MSG_RESULT(enabled)
  else
    AC_MSG_RESULT(not available)
  fi
else
  AC_MSG_RESULT(disabled)
fi
AC_SUBST(HAS_CAPI)

dnl ########################################################################
dnl check for dynamic library code (dlopen and friends)

HAS_LIBDL=0
AC_CHECK_HEADERS([dlfcn.h], [HAS_DLFCN=yes], [HAS_DLFCN=no])
if test "$HAS_DLFCN" = yes ; then
  AC_MSG_CHECKING(if dlopen is available)
  AC_TRY_COMPILE([#include <dlfcn.h>],
                  [void * p = dlopen("lib", 0);], [HAS_DLOPEN=yes], [HAS_DLOPEN=no])
  if test "$HAS_DLOPEN" = no ; then
    AC_MSG_RESULT(no)
  else
    AC_MSG_RESULT(yes)
    if test `uname -s` = FreeBSD; then
        AC_CHECK_LIB([c],[dlopen],
                     [
                      HAS_LIBDL=1
                      DL_LIBS="-lc"
                     ],
                     [HAS_LIBDL=0])
    else
        AC_CHECK_LIB([dl],[dlopen],
                     [
                      HAS_LIBDL=1
                      DL_LIBS="-ldl"
                     ],
                     [HAS_LIBDL=0])
    fi
  fi
fi
AC_SUBST(HAS_LIBDL)
AC_SUBST(DL_LIBS)

AC_MSG_CHECKING(whether to default to full capabilities at empty fmtp lines)
AC_ARG_ENABLE([default-to-full-capabilties],
              [AC_HELP_STRING([--enable-default-to-full-capabilties],[Default to full capabilities at empty fmtp lines])],
              [DEFAULT_TO_FULL_CAPABILITIES=$enableval],
              [DEFAULT_TO_FULL_CAPABILITIES=no])
if test x$DEFAULT_TO_FULL_CAPABILITIES = xyes; then
  AC_DEFINE([DEFAULT_TO_FULL_CAPABILITIES], [1], [Use RFC-compliant defaults for empty fmtp lines])
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
fi

dnl     ########################
dnl     libavcodec (ffmpeg)
dnl     ########################
PKG_CHECK_MODULES([LIBAVCODEC],
                  [libavcodec >= 51.11.0],
                  [
                   AC_SUBST(LIBAVCODEC_CFLAGS)
                   AC_SUBST(LIBAVCODEC_LIBS)
		   AC_DEFINE([HAVE_LIBAVCODEC], [1], [ffmpeg])
     	           HAVE_LIBAVCODEC=yes
                   OPAL_LIBAVCODEC_HACK
                   OPAL_GET_LIBNAME([LIBAVCODEC], [libavcodec], [$LIBAVCODEC_LIBS])
                   OPAL_LIBAVCODEC_HEADER
                  ],
                  [
                   HAVE_LIBAVCODEC=no
                  ]
                 )
AC_SUBST(HAVE_LIBAVCODEC)

OPAL_LIBAVCODEC_SOURCE


dnl     ########################
dnl     x264
dnl     ########################
PKG_CHECK_MODULES([X264],
                  [x264 >= 0.48],
                  [
                   AC_SUBST(X264_CFLAGS)
                   AC_SUBST(X264_LIBS)
                   HAVE_X264=yes
                   OPAL_GET_LIBNAME([X264], [libx264], [$X264_LIBS])
                   OPAL_X264_LINKAGE
                  ],
                  [
                   HAVE_X264=no
                  ]
                 )
AC_SUBST(X264_LINK_STATIC)
AC_SUBST(HAVE_X264)

dnl     ########################
dnl     theora
dnl     ########################
PKG_CHECK_MODULES([THEORA],
                  [theora],
                  [
                   AC_SUBST(THEORA_CFLAGS)
                   AC_SUBST(THEORA_LIBS)
                   AC_DEFINE([HAVE_THEORA], [1], [theora])
                   HAVE_THEORA=yes
                  ],
                  [
                   HAVE_THEORA=no
                  ])
AC_SUBST(HAVE_THEORA)

dnl ########################################################################

HAS_SPANDSP=0

AC_ARG_ENABLE(spandsp, [  --disable-spandsp        Disable spandsp-based fax support],,)
if test "x${enable_spandsp}" != "xno" ; then
    enable_spandsp=yes

    saved_LIBS="$LIBS"
    LIBS="$LIBS -lspandsp"
    AC_CHECK_LIB(spandsp, t38_indicator, [spandsp_libs=yes], [spandsp_libs=no])
    LIBS=$saved_LIBS

    if test "x${spandsp_libs}" != "xno"; then
	SPANDSP_LIBS="-lspandsp"
        AC_CHECK_HEADERS([spandsp.h], [spandsp_headers=true], [spandsp_headers=false])
        if test "x${spandsp_headers}" = "xtrue"; then
            HAS_SPANDSP=1
        fi
    fi
fi
AC_SUBST(HAS_SPANDSP)
AC_SUBST(SPANDSP_LIBS)
dnl ########################################################################

if test -d audio/GSM0610 ; then
  AC_CONFIG_FILES(audio/GSM0610/Makefile)
fi
if test -d audio/gsm-amr ; then
  AC_CONFIG_FILES(audio/gsm-amr/Makefile)
fi
if test -d audio/iLBC ; then
  AC_CONFIG_FILES(audio/iLBC/Makefile)
fi
if test -d audio/Speex ; then
  AC_CONFIG_FILES(audio/Speex/Makefile)
fi
if test -d audio/LPC_10 ; then
  AC_CONFIG_FILES(audio/LPC_10/Makefile)
fi
if test -d audio/G726 ; then
  AC_CONFIG_FILES(audio/G726/Makefile)
fi
if test -d audio/IMA_ADPCM ; then
  AC_CONFIG_FILES(audio/IMA_ADPCM/Makefile)
fi
if test -d video/H.261-vic ; then
  AC_CONFIG_FILES(video/H.261-vic/Makefile)
fi
if test -d video/H.263-ffmpeg ; then
  AC_CONFIG_FILES(video/H.263-ffmpeg/Makefile)
fi
if test -d video/H.263-1998 ; then
  AC_CONFIG_FILES(video/H.263-1998/Makefile)
fi
if test -d video/MPEG4-ffmpeg ; then
  AC_CONFIG_FILES(video/MPEG4-ffmpeg/Makefile)
fi
if test -d video/H.264 ; then
  AC_CONFIG_FILES(video/H.264/Makefile)
fi
if test -d video/H.264/gpl; then
  AC_CONFIG_FILES(video/H.264/gpl/Makefile)
fi
if test -d video/THEORA ; then
  AC_CONFIG_FILES(video/THEORA/Makefile)
fi
if test -d LID/IxJ ; then
  AC_CONFIG_FILES(LID/IxJ/Makefile)
fi
if test -d LID/VPB ; then
  AC_CONFIG_FILES(LID/VPB/Makefile)
fi
if test -d LID/CAPI ; then
  AC_CONFIG_FILES(LID/CAPI/Makefile)
fi
if test -d fax/fax_spandsp -a -d fax/fax_spandsp/spandsp_util ; then
  AC_CONFIG_FILES(fax/fax_spandsp/Makefile)
fi

AC_CONFIG_FILES(Makefile)

dnl ########################################################################
dnl Header file

dnl ########################################################################
dnl Output configured files

AC_OUTPUT(audio/Speex/libspeex/speex_config_types.h)
AC_CONFIG_HEADERS(plugin-config.h)
AC_OUTPUT()

dnl ########################################################################
dnl Summary
dnl ########################################################################
echo ""
echo "================ Codec Plugin configuration ==================="
echo "          Installing into prefix  :  $INSTALLPREFIX"
echo ""
echo "                           H.261  :  enabled"
if test "x${HAS_FFMPEG_H263}" = "x1" ; then
echo "                           H.263  :  enabled"
else
echo "                           H.263  :  disabled"
fi
if test "x${HAVE_LIBAVCODEC}" = "xyes" ; then
echo "                           H.263+ :  enabled"
else
echo "                           H.263+ :  disabled (needs libavcodec)"
fi
if test "x${HAS_LIBDL}" = "x1" ; then
if test "x${HAVE_X264}" = "xyes" ; then
if test "x${HAVE_LIBAVCODEC}" = "xyes" ; then
echo "                           H.264  :  enabled"
else
echo "                           H.264  :  disabled (needs libavcodec)"
fi
else
echo "                           H.264  :  disabled (needs libx264)"
fi
else
echo "                           H.264  :  disabled (needs libdl)"
fi
if test "x${HAVE_THEORA}" = "xyes" ; then
echo "                          THEORA  :  enabled"
else
echo "                          THEORA  :  disabled"
fi
if test "x${HAVE_LIBAVCODEC}" = "xyes" ; then

if test "x${LIBAVCODEC_SOURCE_DIR}" = "x" ; then
echo "                    MPEG4 Part 2  :  enabled (no rate control correction)"
else
echo "                    MPEG4 Part 2  :  enabled"
fi
else
echo "                    MPEG4 Part 2  :  disabled (needs libavcodec)"
fi
echo ""
echo "                         OS Type  :  $target_os"
echo "                    Machine Type  :  $target_cpu"
echo ""
echo " If all settings are OK, type make and make install "
echo "========================================================"
