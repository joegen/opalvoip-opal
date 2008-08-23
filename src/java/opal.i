%module example
 %{
 /* Includes the header in the wrapper code */
 #include "opal.h"
 %}
 
 /* Parse the header file to generate wrappers */
 %include "opal.h"
