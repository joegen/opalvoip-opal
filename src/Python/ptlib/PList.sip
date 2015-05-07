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

/**This class is a collection of objects which are descendents of the
   #PObject# class. It is implemeted as a doubly linked list.

   The implementation of a list allows very fast inserting and deleting of
   objects in the collection, but has severe penalties for random access. All
   object access should be done sequentially to avoid these speed penalties.

   The class remembers the last accessed element. This state information is
   used to optimise access by the "virtual array" model of collections. If
   access via ordinal index is made sequentially there is little overhead.

   The PAbstractList class would very rarely be descended from directly by
   the user. The #PDECLARE_LIST# and #PLIST# macros would normally
   be used to create descendent classes. They will instantiate the template
   based on #PList# or directly declare and define the class (using
   inline functions) if templates are not being used.

   The #PList# class or #PDECLARE_LIST# macro will define the
   correctly typed operators for subscript access (#operator[]#).
 */
 
template<TYPE>
%MappedType PList<TYPE>
{
%TypeHeaderCode
#include <ptlib.h>
#include <ptlib/lists.h>
%End

%ConvertFromTypeCode
    // Create the list.
    PyObject *l;
	int i = 0;

    if ((l = PyList_New(sipCpp->GetSize())) == NULL)
        return NULL;
		
    // Go through each element in the C++ instance and convert it to a
    // wrapped TYPE.
    for (PList<TYPE>::iterator iter = sipCpp->begin(); iter != sipCpp->end(); iter++)
    {
        PyObject *pobj;
        TYPE & obj = *iter;

       // Get the Python wrapper for the Type instance, creating a new
       // one if necessary, and handle any ownership transfer.
       if ((pobj = sipConvertFromType(&obj, sipType_TYPE, NULL)) == NULL)
       {
           // There was an error so garbage collect the Python list.
           Py_DECREF(l);
           return NULL;
       }
		
		// Add the wrapper to the list.
        PyList_SET_ITEM(l, i++, pobj);
	}
	
	// Return the Python list.
    return l;
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
   
	PList<TYPE> *plist = new PList<TYPE>;
 
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
			sipConvertToType(item, sipType_TYPE, NULL, SIP_NOT_NONE, &state, sipIsErr));

	   if (!*sipIsErr)
		   plist->Append(reinterpret_cast<PObject*>(p));

	   sipReleaseType(p, sipType_TYPE, state);
	   Py_DECREF(item);
	}

	Py_DECREF(iterator);

	if (*sipIsErr)
	{
	   delete plist;
	   return 0;
	}

	*sipCppPtr = plist;
	return sipGetState(sipTransferObj);
%End
};
