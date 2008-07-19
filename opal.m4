dnl ########################################################################
dnl Generic OPAL Macros
dnl ########################################################################

dnl OPAL_MSG_CHECK
dnl Print out a line of text and a result
dnl Arguments: $1 Message to print out
dnl            $2 Result to print out
dnl Return:    none
AC_DEFUN([OPAL_MSG_CHECK],
         [
          AC_MSG_CHECKING([$1])
          AC_MSG_RESULT([$2])
         ])

dnl OPAL_SIMPLE
dnl Change a given variable according to arguments and subst and define it
dnl Arguments: $1 name of configure option
dnl            $2 the variable to change, subst and define
dnl            $3 the configure argument description
dnl            $4 the checking output and define comment
dnl Return:    $$2 The (possibly) changed variable
AC_DEFUN([OPAL_SIMPLE_OPTION],
         [
          if test "x$$2" = "x"; then
            AC_MSG_ERROR([No default specified for $2, please correct configure.ac])
	  fi          
          AC_ARG_ENABLE([$1],
                        [AC_HELP_STRING([--enable-$1],[$3])],
                        [$2=$enableval])
          OPAL_MSG_CHECK([$3], [$$2])
          AC_SUBST($2)
          if test "x$$2" = "xyes"; then
            AC_DEFINE([$2], [1], [$3])
          fi
         ])

dnl OPAL_GET_LIBNAME
dnl Find out the real name of a library file
dnl Arguments: $1 The prefix for variables referring to the library (e.g. LIBAVCODEC)
dnl            $2 The name of the library (e.g. libavcodec)
dnl            $3 The -l argument(s) of the library (e.g. -lavcodec)
dnl Return:    $$1_LIB_NAME The actual name of the library file
dnl Define:    $1_LIB_NAME The actual name of the library file
AC_DEFUN([OPAL_GET_LIBNAME],
         [
          AC_MSG_CHECKING(filename of $2 library)
          AC_LANG_CONFTEST([int main () {}])
          $CC -o conftest$ac_exeext $CFLAGS $CPPFLAGS $LDFLAGS conftest.$ac_ext $LIBS $3>&AS_MESSAGE_LOG_FD
          if test \! -x conftest$ac_exeext ; then
            AC_MSG_RESULT(cannot determine - using defaults)
          else
            $1_LIB_NAME=`ldd ./conftest | grep $2 | awk '{print @S|@1; }'`
            AC_MSG_RESULT($$1_LIB_NAME)
            AC_DEFINE_UNQUOTED([$1_LIB_NAME], ["$$1_LIB_NAME"], [Filename of the $2 library])
          fi
         ])

dnl OPAL_DETERMINE_DEBUG
dnl Determine desired debug level, default is -g -O2
dnl Arguments: CFLAGS
dnl            CXXFLAGS
dnl Return:    $CFLAGS
dnl            $CXXFLAGS
AC_DEFUN([OPAL_DETERMINE_DEBUG],
         [
          AC_ARG_WITH([with-debug],
                      [AC_HELP_STRING([--with-debug], [ full, symbols or none (default: symbols)])])

          if test "x$with_debug" = xfull; then
            CFLAGS="$CFLAGS -g -O0"
            CXXFLAGS="$CXXFLAGS -g -O0"
            OPAL_MSG_CHECK([debugging support], [full])
          else
            if test "x$with_debug" = xnone; then
              CFLAGS="$CFLAGS -O2"
              CXXFLAGS="$CXXFLAGS -O2"
              OPAL_MSG_CHECK([debugging support], [none])
            else
              CFLAGS="$CFLAGS -g -O2"
              CXXFLAGS="$CXXFLAGS -g -O2"
              OPAL_MSG_CHECK([debugging support], [symbols])
            fi
          fi
         ])

dnl     ########################
dnl     LIBAVCODEC
dnl     ########################

dnl OPAL_LIBAVCODEC_HACK
dnl Whether to activate or deactivate the memory alignment hack for libavcodec
dnl Arguments: $LIBAVCODEC_STACKALIGN_HACK The default value
dnl Return:    $LIBAVCODEC_STACKALIGN_HACK The possibly user-mandated value
dnl Define:    LIBAVCODEC_STACKALIGN_HACK The possibly user-mandated value

AC_DEFUN([OPAL_LIBAVCODEC_HACK],
         [
          AC_ARG_ENABLE([libavcodec-stackalign-hack],
                        [AC_HELP_STRING([--enable-libavcodec-stackalign-hack], [Stack alignment hack for libavcodec library])],
                        [LIBAVCODEC_STACKALIGN_HACK=$enableval])
          if test x$LIBAVCODEC_STACKALIGN_HACK = xyes; then
            AC_MSG_NOTICE(libavcodec stack align hack enabled)
            AC_DEFINE([LIBAVCODEC_STACKALIGN_HACK], [1], [Stack alignment hack for libavcodec library])
          else
            AC_MSG_NOTICE(libavcodec stack align hack disabled)
          fi
         ])

dnl OPAL_LIBAVCODEC_SOURCE
dnl Allow the user to specify the libavcodec source dir for full MPEG4 rate control
dnl Arguments: none
dnl Return:    $LIBAVCODEC_SOURCE_DIR The directory
dnl Define:    LIBAVCODEC_SOURCE_DIR The directory
AC_DEFUN([OPAL_LIBAVCODEC_SOURCE],
         [
          AC_MSG_CHECKING(libavcodec source)
          LIBAVCODEC_SOURCE_DIR=
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
         ])

dnl OPAL_LIBAVCODEC_HEADER
dnl Find out whether libavcodec headers reside in ffmpeg/ (old) or libavcodec/ (new)
dnl Arguments: $LIBAVCODEC_CFLAGS The cflags for compiling apps with libavcodec
dnl Return:    none
dnl Define:    LIBAVCODEC_HEADER The libavcodec header (e.g. libavcodec/avcodec.h)
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

dnl OPAL_X264_LINKAGE
dnl Whether to statically link the H.264 helper to x264 or to load libx264 dynamically when the helper is executed
dnl Arguments: $X264_LINK_STATIC The default value
dnl Return:    $X264_LINK_STATIC The possibly user-mandated value
dnl Define:    X264_LINK_STATIC The possibly user-mandated value
AC_DEFUN([OPAL_X264_LINKAGE],
         [
          AC_ARG_ENABLE([x264-link-static],
                        [AC_HELP_STRING([--enable-x264-link-static], [Statically link x264 to the plugin. Default for win32.])],
                        [X264_LINK_STATIC=$enableval])
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

dnl OPAL_SPEEX_TYPES
dnl Define necessary typedefs for Speex
dnl Arguments: none
dnl Return:    SIZE16 short or int
dnl            SIZE32 short, int or long
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
         ])


dnl OPAL_SPEEX_TYPES
dnl Determine whether to use the system or internal speex (can be forced), uses pkg-config
dnl Arguments: none
dnl Return:    $SPEEX_SYSTEM whether system or interal speex shall be used
dnl            $SPEEX_INTERNAL_VERSION Internal speex version
dnl            $SPEEX_SYSTEM_VERSION System speex version (if found)
dnl            $SPEEX_CFLAGS System speex cflags (if using system speex)
dnl            $SPEEX_LIBS System speex libs (if using system speex)
AC_DEFUN([OPAL_DETERMINE_SPEEX],
         [AC_ARG_ENABLE([localspeex],
                        [AC_HELP_STRING([--enable-localspeex],[Force use local version of Speex library rather than system version])],
                        [localspeex=$enableval],
                        [localspeex=])

          AC_MSG_CHECKING(internal Speex version)
          SPEEX_CFLAGS=
          SPEEX_LIBS=
          if test -f "audio/Speex/libspeex/misc.h"
          then
            SPEEX_INTERNAL_VERSION=`grep "#define SPEEX_VERSION" audio/Speex/libspeex/misc.h | sed -e 's/^.*speex\-//' -e 's/\".*//'`
    	  else
            SPEEX_INTERNAL_VERSION=`grep "#define SPEEX_VERSION" plugins/audio/Speex/libspeex/misc.h | sed -e 's/^.*speex\-//' -e 's/\".*//'`
    	  fi
          AC_MSG_RESULT($SPEEX_INTERNAL_VERSION)

          if test "x${localspeex}" = "xyes" ; then
            AC_MSG_NOTICE(forcing use of local Speex sources)
            SPEEX_SYSTEM=no

          elif test "x${localspeex}" = "xno" ; then
            AC_MSG_NOTICE(forcing use of system Speex library)
            PKG_CHECK_MODULES([SPEEX],
                              [speex],
                              [SPEEX_SYSTEM=yes],
                              [
                              AC_MSG_ERROR([cannot find system speex])
                              ])

          else

            AC_MSG_NOTICE(checking whether system Speex or internal Speex is more recent)
            PKG_CHECK_MODULES([SPEEX],
                              [speex >= $SPEEX_INTERNAL_VERSION],
                              [SPEEX_SYSTEM=yes],
                              [
                              SPEEX_SYSTEM=no
                              AC_MSG_RESULT(internal Speex version is more recent than system Speex or system Speex not found)
                              ])
          fi

          if test "x${SPEEX_SYSTEM}" = "xyes" ; then
            SPEEX_SYSTEM_VERSION=`$PKG_CONFIG speex --modversion`
            AC_MSG_RESULT(using system Speex version $SPEEX_SYSTEM_VERSION)
          fi
         ])

dnl     ########################
dnl     LIBDL
dnl     ########################

dnl OPAL_FIND_LBDL
dnl Try to find a library containing dlopen()
dnl Arguments: $1 action if-found
dnl            $2 action if-not-found
dnl Return:    $DL_LIBS The libs for dlopen()
AC_DEFUN([OPAL_FIND_LIBDL],
         [
          opal_libdl=no
          AC_CHECK_HEADERS([dlfcn.h], [opal_dlfcn=yes], [opal_dlfcn=no])
          if test "$opal_dlfcn" = yes ; then
            AC_MSG_CHECKING(if dlopen is available)
            AC_TRY_COMPILE([#include <dlfcn.h>],
                            [void * p = dlopen("lib", 0);], [opal_dlopen=yes], [opal_dlopen=no])
            if test "$opal_dlopen" = no ; then
              AC_MSG_RESULT(no)
            else
              AC_MSG_RESULT(yes)
              if test `uname -s` = FreeBSD; then
dnl              if test "x${target_os}" = "xfreebsd"; then
                  AC_CHECK_LIB([c],[dlopen],
                              [
                                opal_libdl=yes
                                DL_LIBS="-lc"
                              ],
                              [opal_libdl=no])
              else
                  AC_CHECK_LIB([dl],[dlopen],
                              [
                                opal_libdl=yes
                                DL_LIBS="-ldl"
                              ],
                              [opal_libdl=no])
              fi
            fi
          fi
          AS_IF([test AS_VAR_GET([opal_libdl]) = yes], [$1], [$2])[]
         ])

dnl     ########################
dnl     LIBGSM
dnl     ########################

dnl OPAL_FIND_GSM
dnl Try to find an installed libgsm that is compiled with WAV49
dnl Arguments: $1 action if-found
dnl            $2 action if-not-found
dnl Return:    $GSM_CFLAGS
dnl            $GSM_LIBS
AC_DEFUN([OPAL_FIND_GSM],
         [
          opal_gsm=no
          AC_CHECK_LIB(gsm, gsm_create, opal_gsm=yes)
          if test "x$opal_gsm" = "xyes"; then
            AC_MSG_CHECKING(if system GSM library has WAV49)
            old_LIBS=$LIBS
            opal_gsm=no

            LIBS="$LIBS -lgsm"
            AC_RUN_IFELSE(
            [AC_LANG_PROGRAM([[
            #include <gsm.h>
 ]],[[
            int option = 0;
            gsm handle = gsm_create();
            return (gsm_option(handle, GSM_OPT_WAV49, &option) == -1) ? 1 : 0;
 ]])], opal_gsm=yes) 
            LIBS=$old_LIBS
            AC_MSG_RESULT($opal_gsm)

            if test "x${opal_gsm}" = "xyes" ; then
              GSM_CLFAGS=""
              GSM_LIBS="-lgsm"
            fi
            OPAL_MSG_CHECK([System GSM], [$opal_gsm])
          fi
          AS_IF([test AS_VAR_GET([opal_gsm]) = yes], [$1], [$2])[]
         ])


dnl     ########################
dnl     SPANDSP
dnl     ########################

dnl OPAL_FIND_SPANDSP
dnl Find spandsp
dnl Arguments: $1 action if-found
dnl            $2 action if-not-found
dnl Return:    $SPANDSP_LIBS
AC_DEFUN([OPAL_FIND_SPANDSP],
         [
          saved_LIBS="$LIBS"
          LIBS="$LIBS -lspandsp"
          AC_CHECK_LIB(spandsp, t38_indicator, [opal_spandsp=yes], [opal_spandsp=no])
          LIBS=$saved_LIBS
          if test "x${opal_spandsp}" = "xyes"; then
              SPANDSP_LIBS="-lspandsp"
              AC_CHECK_HEADERS([netdb.h arpa/inet.h sys/ioctl.h sys/socket.h spandsp.h], [opal_spandsp=yes], [opal_spandsp=no])
          fi
          AS_IF([test AS_VAR_GET([opal_spandsp]) = yes], [$1], [$2])[]
         ])
