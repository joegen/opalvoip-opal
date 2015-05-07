/*
 * Python bindings.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2011 Demetrius Cassidy
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Open Phone Abstraction Library (OPAL)
 *
 * The Initial Developer of the Original Code is Demetrius Cassidy
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

template<TYPE>
%MappedType std::set<TYPE>
{
%TypeHeaderCode
#include <set>
using std::set;
%End

%ConvertFromTypeCode
   PyObject *s;
   
   // Create the Python list of the correct length.
   if ((s = PySet_New(NULL)) == NULL)
       return NULL;

   // Go through each element in the C++ instance and convert it to a
   // wrapped TYPE.
   for (set<TYPE>::iterator iter = sipCpp->begin(); iter != sipCpp->end(); iter++)
   {
       TYPE *cpp = new TYPE(*iter);
       PyObject *pobj;

       // Get the Python wrapper for the Type instance, creating a new
	  // one if necessary, and handle any ownership transfer.
       if ((pobj = sipConvertFromNewType(cpp, sipType_TYPE, sipTransferObj)) == NULL)
       {
           // There was an error so garbage collect the Python list.
           Py_DECREF(s);
           return NULL;
       }

       // Add the wrapper to the list.
       PySet_Add(s, pobj);
   }

   // Return the Python set.
   return s;
%End

%ConvertToTypeCode
   // Check if type is compatible
   if (sipIsErr == NULL)
   {
       // Must be any iterable
       PyObject *i = PyObject_GetIter(sipPy);
       bool iterable = (i != NULL);
       Py_XDECREF(i);
       return iterable;
   }

   // Iterate over the object
   PyObject *iterator = PyObject_GetIter(sipPy);
   PyObject *item;

   set<TYPE> *s = new set<TYPE>;

   while ((item = PyIter_Next(iterator)))
   {
       if (!sipCanConvertToType(item, sipType_TYPE, SIP_NOT_NONE))
       {
           PyErr_Format(PyExc_TypeError, "object in iterable cannot be converted to TYPE");
           *sipIsErr = 1;
           break;
       }

       int state;
       TYPE* p = reinterpret_cast<TYPE*>(
            sipConvertToType(item, sipType_TYPE, 0, SIP_NOT_NONE, &state, sipIsErr));

       if (!*sipIsErr)
           s->insert(*p);

       sipReleaseType(p, sipType_TYPE, state);
       Py_DECREF(item);
   }

   Py_DECREF(iterator);

   if (*sipIsErr)
   {
       delete s;
       return 0;
   }

   *sipCppPtr = s;
   return sipGetState(sipTransferObj);
%End
};
