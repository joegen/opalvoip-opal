%module OPAL

  %rename("%(strip:[m_])s") ""; // Remove al the member variables m_XXXX

  %{
    /* Includes the header in the wrapper code */
    #include "opal.h"
    int opal_csharp_swig_wrapper_link;
  %}

  %include "typemaps.i"

  /* Parse the header file to generate wrappers */
  %include "opal.h"
