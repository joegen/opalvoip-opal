/* OPAL interface file for Java bindings, @author Nagy Elemer Karoly, @version 0.0.1. */
/* This file is hereby released to the Public Domain - use it on your own risk. */
/* Run me with "swig -java -package org.opalvoip opal.i" */
%module example  // Should be this: %module SimpleOpal
 %{
 /* Includes the header in the wrapper code */
 #include "opal.h"
 %}

 // We need some tweaking to access INOUT variables which would be immutable c pointers by default. 
 %include "typemaps.i"
 OpalHandle OpalInitialise(unsigned * INOUT, const char * INPUT);
 OpalMessage * OpalSendMessage(OpalHandle IN, const OpalMessage * IN);

 /* Parse the header file to generate wrappers */
 %include "opal.h"
