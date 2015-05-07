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

%MappedType std::string
{
%TypeHeaderCode
using std::string;
#include <string>
%End

%ConvertToTypeCode
	if (sipIsErr == NULL)
		return (sipPy == Py_None || PyString_Check(sipPy) || PyUnicode_Check(sipPy));
		
	if (sipPy == Py_None)
	{
		// None is the only way to create a null (as opposed to empty) PString.
		*sipCppPtr = new string();
		return sipGetState(sipTransferObj);
	}

	if (PyString_Check(sipPy))
	{
		char * s = PyString_AsString(sipPy);
		*sipCppPtr = new string(s);
		return sipGetState(sipTransferObj);
	}
	
	return NULL;
%End

%ConvertFromTypeCode
	PyObject *pyStr = PyString_FromString(sipCpp->c_str());
    return pyStr;
%End 
};