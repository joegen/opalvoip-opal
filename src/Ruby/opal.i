%module OPAL

  %{
    /* Includes the header in the wrapper code */
    #include "opal.h"
  %}

  %include "typemaps.i"

  /* Parse the header file to generate wrappers */
  %include "opal.h"
