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

%ModuleHeaderCode
#include <ptlib.h>
#include <ptlib/array.h>
%End

template<TYPE>
%MappedType PArray<TYPE>
{
%ConvertFromTypeCode
    // Create the list.
    PyObject *l;
	PINDEX len = sipCpp->GetSize();

    if ((l = PyList_New(len)) == NULL)
        return NULL;
		
    // Go through each element in the C++ instance and convert it to a
    // wrapped TYPE.
    for (PINDEX i = 0; i < len; i++)
    {
		TYPE *cpp = new TYPE((*sipCpp)[i]);
        PyObject *pobj;

       // Get the Python wrapper for the Type instance, creating a new
       // one if necessary, and transfer ownership to Python.
       if ((pobj = sipConvertFromNewType(cpp, sipType_TYPE, Py_None)) == NULL)
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
	PArray<TYPE> *array = new PArray<TYPE>;
	PyObject *iterator = PyObject_GetIter(sipPy);
	PyObject *item;
 

	while ((item = PyIter_Next(iterator)))
	{
	   if (!sipCanConvertToType(item, sipType_TYPE, SIP_NOT_NONE))
	   {
		   PyErr_SetString(PyExc_TypeError, "object in array cannot be converted to TYPE");
		   *sipIsErr = 1;
		   break;
	   }

	   int state;
	   PObject *p = reinterpret_cast<PObject*>(
			sipConvertToType(item, sipType_TYPE, NULL, SIP_NOT_NONE, &state, sipIsErr));

	   if (!*sipIsErr)
		   array->Append(p);

	   sipReleaseType(p, sipType_TYPE, state);
	   Py_DECREF(item);
	}

	Py_DECREF(iterator);

	if (*sipIsErr)
	{
	   delete array;
	   return 0;
	}

	*sipCppPtr = array;
	return sipGetState(sipTransferObj);
%End
};

template<TYPE>
%MappedType PScalarArray<TYPE>
{
%ConvertFromTypeCode
    // Create the list.
    PyObject *l;

    if ((l = PyList_New(sipCpp->GetSize())) == NULL)
        return NULL;
		
    // Go through each element in the C++ instance and convert it to a
    // wrapped PString.
    for (PINDEX i = 0; i < sipCpp->GetSize(); i++)
    {
		TYPE *data = new TYPE(sipCpp->GetAt(i));
        PyObject *pobj;

       // Get the Python wrapper for the Type instance, creating a new
       // one if necessary, and transfer ownership to Python.
       if ((pobj = sipConvertFromNewType(data, sipType_TYPE, Py_None)) == NULL)
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
	PINDEX i = 0;
   
	PScalarArray<TYPE> *array = new PScalarArray<TYPE>;
 
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
		   array->SetAt(i++, *p);
	
	   sipReleaseType(p, sipType_TYPE, state);
	   Py_DECREF(item);
	}

	Py_DECREF(iterator);

	if (*sipIsErr)
	{
	   delete array;
	   return 0;
	}

	*sipCppPtr = array;
	return sipGetState(sipTransferObj);
%End
};

%MappedType PBYTEArray
{
%ConvertFromTypeCode
    // Create the list.
    PyObject *l;

    if ((l = PyList_New(sipCpp->GetSize())) == NULL)
        return NULL;
		
    // Go through each element in the C++ instance and convert it to a
    // wrapped PString.
    for (PINDEX i = 0; i < sipCpp->GetSize(); i++)
    {
		BYTE *byte = new BYTE(sipCpp->GetAt(i));

		// Add the wrapper to the list.
        PyList_SET_ITEM(l, i, PyUnicode_FromString((const char *)byte));
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
   
	PBYTEArray *array = new PBYTEArray;
 
	while ((item = PyIter_Next(iterator)))
	{
	   BYTE *p = (BYTE*)PyString_AsString(item);

	   if(p != NULL)
			array->SetAt(array->GetSize(), *p);

	   Py_DECREF(item);
	}

	Py_DECREF(iterator);

	*sipCppPtr = array;
	return sipGetState(sipTransferObj);
%End
};

%MappedType PCharArray
{
%ConvertFromTypeCode
    // Create the list.
    PyObject *l;

    if ((l = PyList_New(sipCpp->GetSize())) == NULL)
        return NULL;
		
    // Go through each element in the C++ instance and convert it to a
    // wrapped PString.
    for (PINDEX i = 0; i < sipCpp->GetSize(); i++)
    {
		char *c = new char(sipCpp->GetAt(i));

		// Add the wrapper to the list.
        PyList_SET_ITEM(l, i, PyUnicode_FromString(c));
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
   
	PCharArray *array = new PCharArray;
 
	while ((item = PyIter_Next(iterator)))
	{
	   char *c = PyString_AsString(item);

	   if(c != NULL)
			array->SetAt(array->GetSize(), *c);

	   Py_DECREF(item);
	}

	Py_DECREF(iterator);

	*sipCppPtr = array;
	return sipGetState(sipTransferObj);
%End
};

%MappedType PUnsignedArray
{
%ConvertFromTypeCode
    // Create the list.
    PyObject *l;

    if ((l = PyList_New(sipCpp->GetSize())) == NULL)
        return NULL;
		
    // Go through each element in the C++ instance and convert it to Python unsigned long type.
    for (PINDEX i = 0; i < sipCpp->GetSize(); i++)
    {
		unsigned data = sipCpp->GetAt(i);
		// Add the wrapper to the list.
        PyList_SET_ITEM(l, i, PyLong_FromUnsignedLong(data));
	}
	
	// Return the Python list.
    return l;
%End

%ConvertToTypeCode
	// Check the type if that is all that is required.
	if (sipIsErr == NULL)
		return PyList_Check(sipPy);

	PScalarArray<unsigned> *array = new PScalarArray<unsigned>;

	for(SIP_SSIZE_T i = 0; i < PyList_Size(sipPy); i++)
	{
		PyObject *item = PyList_GetItem(sipPy, i);

		if( item != NULL )
		{
			unsigned data = (unsigned)PyLong_AsUnsignedLong(item);
			array->SetAt(i, data);
			Py_DECREF(item);
		}
		else
		{
			delete array;
			return NULL;
		}
	}
	
	*sipCppPtr = array;
	return sipGetState(sipTransferObj);
%End
};



%MappedType PDWORDArray
{
%ConvertFromTypeCode
    // Create the list.
    PyObject *l;

    if ((l = PyList_New(sipCpp->GetSize())) == NULL)
        return NULL;
		
    // Go through each element in the C++ instance and convert it to Python unsigned long type.
    for (PINDEX i = 0; i < sipCpp->GetSize(); i++)
    {
		DWORD data = sipCpp->GetAt(i);
		// Add the wrapper to the list.
        PyList_SET_ITEM(l, i, PyLong_FromUnsignedLong(data));
	}
	
	// Return the Python list.
    return l;
%End

%ConvertToTypeCode
	// Check the type if that is all that is required.
	if (sipIsErr == NULL)
		return PyList_Check(sipPy);

	PScalarArray<DWORD> *array = new PScalarArray<DWORD>;

	for(SIP_SSIZE_T i = 0; i < PyList_Size(sipPy); i++)
	{
		PyObject *item = PyList_GetItem(sipPy, i);

		if( item != NULL )
		{
			DWORD data = (DWORD)PyLong_AsUnsignedLong(item);
			array->SetAt(i, data);
			Py_DECREF(item);
		}
		else
		{
			delete array;
			return NULL;
		}
	}
	
	*sipCppPtr = array;
	return sipGetState(sipTransferObj);
%End
};

%MappedType PShortArray
{
%ConvertFromTypeCode
    // Create the list.
    PyObject *l;

    if ((l = PyList_New(sipCpp->GetSize())) == NULL)
        return NULL;
		
    // Go through each element in the C++ instance and convert it to Python long type.
    for (PINDEX i = 0; i < sipCpp->GetSize(); i++)
    {
		short data = sipCpp->GetAt(i);
		// Add the wrapper to the list.
        PyList_SET_ITEM(l, i, PyInt_FromLong((long)data));
	}
	
	// Return the Python list.
    return l;
%End

%ConvertToTypeCode
	// Check the type if that is all that is required.
	if (sipIsErr == NULL)
		return PyList_Check(sipPy);

	PScalarArray<short> *array = new PScalarArray<short>;

	for(SIP_SSIZE_T i = 0; i < PyList_Size(sipPy); i++)
	{
		PyObject *item = PyList_GetItem(sipPy, i);

		if( item != NULL )
		{
			short data = (short)PyInt_AsLong(item);
			array->SetAt(i, data);
			Py_DECREF(item);
		}
		else
		{
			delete array;
			return NULL;
		}
	}
	
	*sipCppPtr = array;
	return sipGetState(sipTransferObj);
%End
};

%MappedType PIntArray
{
%ConvertFromTypeCode
    // Create the list.
    PyObject *l;

    if ((l = PyList_New(sipCpp->GetSize())) == NULL)
        return NULL;
		
    // Go through each element in the C++ instance and convert it to Python long type.
    for (PINDEX i = 0; i < sipCpp->GetSize(); i++)
    {
		int data = sipCpp->GetAt(i);
		// Add the wrapper to the list.
        PyList_SET_ITEM(l, i, PyInt_FromLong((long)data));
	}
	
	// Return the Python list.
    return l;
%End

%ConvertToTypeCode
	// Check the type if that is all that is required.
	if (sipIsErr == NULL)
		return PyList_Check(sipPy);

	PScalarArray<int> *array = new PScalarArray<int>;

	for(SIP_SSIZE_T i = 0; i < PyList_Size(sipPy); i++)
	{
		PyObject *item = PyList_GetItem(sipPy, i);

		if( item != NULL )
		{
			int data = (int)PyInt_AsLong(item);
			array->SetAt(i, data);
			Py_DECREF(item);
		}
		else
		{
			delete array;
			return NULL;
		}
	}
	
	*sipCppPtr = array;
	return sipGetState(sipTransferObj);
%End
};