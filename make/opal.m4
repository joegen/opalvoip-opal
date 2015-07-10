dnl
dnl OPAL macros
dnl

dnl Assumes my_macros.m4 has been incldued in configure.ac


dnl OPAL_DETERMINE_DIRECTORIES
dnl Check for non-installed "environment variable" mode
AC_DEFUN([OPAL_DETERMINE_DIRECTORIES],[
   AC_MSG_CHECKING([OPAL directory])
   AS_IF([test "x$prefix" != "x" && test "x$prefix" != "xNONE"],[
      AS_IF(test `cd $prefix ; pwd` = `cd $1 ; pwd`, [
         INTERNAL_OPALDIR="$prefix"
         AC_MSG_RESULT([prefix is current directory $INTERNAL_OPALDIR])
      ])
   ],[
      AS_VAR_SET_IF([OPALDIR],[
         ac_default_prefix=$OPALDIR
         INTERNAL_OPALDIR="$OPALDIR"
         AC_MSG_RESULT([using OPALDIR environment variable $INTERNAL_OPALDIR])
      ])
   ])

   AS_VAR_SET_IF([INTERNAL_OPALDIR],[
      libdir+=_$target
      datarootdir="\${exec_prefix}"
      makedir="\${datarootdir}/make"
      VERSION_DIR="$INTERNAL_OPALDIR"
   ],[
      makedir="\${datarootdir}/opal/make"
      VERSION_DIR=`cd $1 ; pwd`
      AC_MSG_RESULT([local source and prefix install])
   ])

   AC_SUBST(makedir)
])


dnl OPAL_SIMPLE_OPTION
dnl Change a given variable according to arguments and subst and define it
dnl $1 name of configure option
dnl $2 the variable to change, subst and define
dnl $3 the configure argument description
dnl $4..$6 dependency variable(s)
AC_DEFUN([OPAL_SIMPLE_OPTION],[
   MY_ARG_ENABLE(
      [$1],
      [$3],
      [$$2],
      [
         AC_SUBST($2, yes)
         AC_DEFINE([$2], [1], [$3])
      ],
      [$2=no],
      [$4],
      [$5],
      [$6]
   )
])


dnl OPAL_CHECK_PTLIB_OPTION
dnl Check if has a specified option
dnl $1 Name of feature
dnl $2 Variable to set & define
dnl $3 Error message
AC_DEFUN([OPAL_CHECK_PTLIB_OPTION],[
   AC_MSG_CHECKING([PTLib for $1])
   OPAL_$2=`pkg-config --variable=$2 ptlib`
   AC_MSG_RESULT([${OPAL_$2}])
   AC_SUBST(OPAL_$2)

   m4_ifnblank([$3],[
      AS_VAR_IF([OPAL_$2],[no],[
         AC_MSG_ERROR([$3])
      ])
  ])
])


dnl OPAL_MODULE_OPTION
dnl as MY_MODULE_OPTION but defines OPAL_xxx for make and define
AC_DEFUN([OPAL_MODULE_OPTION],[
   MY_MODULE_OPTION([$1],[$2],[$3],[$4],[$5],[$6],[$7],[$8],[$9],[$10],[$11],[$12],[$13],[$14])
   AS_VAR_IF([$1[_USABLE]],[yes],[
      AC_DEFINE(OPAL_$1, 1)
   ])
   AC_SUBST(OPAL_$1, $$1[_USABLE])
])


dnl OPAL_DETERMINE_PLUGIN_DIR
dnl Determine plugin install directory
dnl $OPAL_VERSION
dnl $PLUGIN_DIR
AC_DEFUN([OPAL_DETERMINE_PLUGIN_DIR],[
   AC_MSG_CHECKING(where plugins are being installed)

   AC_ARG_WITH(
      [plugin-installdir],
      AS_HELP_STRING([--with-plugin-installdir=DIR],[Location where plugins are installed]),
      [OPAL_PLUGIN_DIR="\${libdir}/$withval"],
      [
         AS_VAR_SET_IF([INTERNAL_OPALDIR],[
            OPAL_PLUGIN_DIR="$INTERNAL_OPALDIR/lib_$target/plugins"
         ],[
            OPAL_PLUGIN_DIR="\${libdir}/opal-${OPAL_VERSION}"
         ])
      ]
   )

   AC_MSG_RESULT("$OPAL_PLUGIN_DIR")
   AC_SUBST(OPAL_PLUGIN_DIR)
])


dnl OPAL_SIMPLE_PLUGIN
dnl $1 module name
dnl $2 command line name
dnl $3 command line help
dnl $4 source directory
dnl $5 optional extra test
AC_DEFUN([OPAL_SIMPLE_PLUGIN],[
   AS_IF([test -d "$4"],[
      AC_ARG_ENABLE(
         [$2],
         [AC_HELP_STRING([--disable-$2],[disable $3])],
         [
            AS_VAR_IF([enableval],[no],[
               HAVE_$1="no (disabled by user)"
            ],[
               HAVE_$1="yes"
            ])
         ],
         [
            AS_VAR_SET_IF([HAVE_$1],[],[HAVE_$1="yes"])
         ]
      )
      m4_ifnblank([$5],[$5])
      AS_VAR_IF([HAVE_$1],[yes],[
         PLUGIN_SUBDIRS="$PLUGIN_SUBDIRS $4"
      ])
   ],[
      HAVE_$1="no (missing directory $4)"
   ])
])


dnl OPAL_SYSTEM_PLUGIN
dnl as MY_MODULE_OPTION but defines OPAL_xxx for make and define
AC_DEFUN([OPAL_SYSTEM_PLUGIN],[
   AS_IF([test -d "$4"],[
      dnl MY_MODULE_OPTION adds to LIBS and normally this right, but for plugins
      dnl we want each plugin to only have its libs so don't add it for everyone
      OPAL_SYSTEM_PLUGIN_LIBS="$LIBS"
      MY_MODULE_OPTION([$1],[$2],[$3],[$5],[$6],[$7],[$8],[$9],[$10],[$11],[$12],[$13],[$14],[$15])
      LIBS="$OPAL_SYSTEM_PLUGIN_LIBS"
      AS_VAR_IF($1[_USABLE],[yes],[
         AS_VAR_IF($1[_SYSTEM], [no],[
            PLUGIN_SUBDIRS="$PLUGIN_SUBDIRS $4"
            HAVE_$1="yes (internal)"
         ],[
            PLUGIN_SUBDIRS="$PLUGIN_SUBDIRS $4"
            HAVE_$1="yes (system)"
            AC_SUBST($1[_CFLAGS])
            AC_SUBST($1[_LIBS])
         ])
      ],[
         HAVE_$1="no (package $5 not found)"
      ])
   ],[
      HAVE_$1="no (missing directory $4)"
   ])
])


