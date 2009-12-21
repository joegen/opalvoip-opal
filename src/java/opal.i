%module OPAL

  %{
    /* Includes the header in the wrapper code */
    #include "opal.h"
  %}

  %include "typemaps.i"

  // We need some tweaking to access INOUT variables which would be immutable c pointers by default. 
  OpalHandle OpalInitialise(unsigned * INOUT, const char * INPUT);
  OpalMessage * OpalSendMessage(OpalHandle IN, const OpalMessage * IN);

  /* Parse the header file to generate wrappers */
  %include "opal.h"
