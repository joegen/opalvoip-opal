%module OPAL

  %rename("%(strip:[m_])s") ""; // Remove al the member variables m_XXXX

  %{
    /* Includes the header in the wrapper code */
    #include "opal.h"
  %}

  %include "typemaps.i"

  /* Parse the header file to generate wrappers */
  %include "opal.h"
