%module OPAL

  %{
    /* Includes the header in the wrapper code */
    #include "opal.h"
    int opal_java_swig_wrapper_link;
  %}

  %include "typemaps.i"

  // We need some tweaking to access INOUT variables which would be immutable c pointers by default. 
  OpalHandle OpalInitialise(unsigned * INOUT, const char * INPUT);
  void OpalShutDown(OpalHandle IN);
  OpalMessage * OpalGetMessage(OpalHandle IN, unsigned IN);
  OpalMessage * OpalSendMessage(OpalHandle IN, const OpalMessage * IN);
  void OpalFreeMessage(OpalMessage * IN);

  /* Parse the header file to generate wrappers */
  %include "opal.h"
