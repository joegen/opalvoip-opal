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
#include <ptlib/dict.h>
#include <ptlib/lists.h>
#include <ptlib/array.h>
#include <ptlib/pstring.h>
%End

%MappedType PString /AllowNone/
{
%ConvertToTypeCode
	if (sipIsErr == NULL)
		return (sipPy == Py_None || PyString_Check(sipPy) || PyUnicode_Check(sipPy));
		
	if (sipPy == Py_None)
	{
		// None is the only way to create a null (as opposed to empty) PString.
		*sipCppPtr = new PString();
		return sipGetState(sipTransferObj);
	}
	
	if (PyUnicode_Check(sipPy))
	{
		Py_UNICODE *w = PyUnicode_AsUnicode(sipPy);
		SIP_SSIZE_T len = PyUnicode_GET_DATA_SIZE(sipPy);
		
		PWCharArray ucs2((PINDEX)PyUnicode_GET_SIZE(sipPy)+1);
		memcpy(ucs2.GetPointer(), w, len);
		*sipCppPtr = new PString(ucs2);
		
		return sipGetState(sipTransferObj);
	}
	
	if (PyString_Check(sipPy))
	{
		*sipCppPtr = new PString(PyString_AS_STRING(sipPy));
		return sipGetState(sipTransferObj);
	}
	
	return 0;
%End

%ConvertFromTypeCode
	PWCharArray ucs2 = sipCpp->AsUCS2();
	Py_ssize_t size = ucs2.GetSize();
	
	if (size > 0 && ucs2[size-1] == 0) // Stip off trailing NULL if present
		size--;

	PyObject *uobj = PyUnicode_FromWideChar(ucs2.GetPointer(), size);
    return uobj;
%End 
};


%MappedType PCaselessString /AllowNone/
{
%ConvertToTypeCode
	if (sipIsErr == NULL)
		return (sipPy == Py_None || PyString_Check(sipPy) || PyUnicode_Check(sipPy));
		
	if (sipPy == Py_None)
	{
		// None is the only way to create a null (as opposed to empty) PString.
		*sipCppPtr = new PCaselessString();
		return sipGetState(sipTransferObj);
	}

	if (PyUnicode_Check(sipPy))
	{
		Py_UNICODE *w = PyUnicode_AsUnicode(sipPy);
		SIP_SSIZE_T len = PyUnicode_GET_DATA_SIZE(sipPy);
		
		PWCharArray ucs2((PINDEX)PyUnicode_GET_SIZE(sipPy)+1); //allow for trailing null if present
		memcpy(ucs2.GetPointer(), w, len);
		*sipCppPtr = new PCaselessString(ucs2);
		
		return sipGetState(sipTransferObj);
	}
	
	if (PyString_Check(sipPy))
	{
		*sipCppPtr = new PCaselessString(PyString_AsString(sipPy));
		return sipGetState(sipTransferObj);
	}
	
	return 0;
%End

%ConvertFromTypeCode
	PWCharArray ucs2 = sipCpp->AsUCS2();
	Py_ssize_t size = ucs2.GetSize();
	
	if (size > 0 && ucs2[size-1] == 0) // Stip off trailing NUL if present
		size--;

	PyObject *uobj = PyUnicode_FromWideChar(ucs2.GetPointer(), size);
    return uobj;
%End 
};


%MappedType PStringToString
{
%ConvertFromTypeCode
    // Create the dictionary.
    PyObject *d = PyDict_New();

    if (!d)
        return NULL;

    for (PINDEX i = 0; i < sipCpp->GetSize(); i++)
    {
        PCaselessString *key  = new PCaselessString(sipCpp->GetKeyAt(i));
        PString *data = new PString(sipCpp->GetDataAt(i));

        PyObject *keyObj = sipConvertFromNewType(key, sipType_PCaselessString, NULL);
        PyObject *dataObj = sipConvertFromNewType(data, sipType_PString, NULL);

        if (keyObj == NULL || dataObj == NULL || PyDict_SetItem(d, keyObj, dataObj) < 0)
        {
            Py_DECREF(d);
            return NULL;
        }
    }

    return d;
%End

%ConvertToTypeCode
    PyObject *keyObj, *dataObj;
    SIP_SSIZE_T i = 0;

    // Check the type if that is all that is required.
    if (sipIsErr == NULL)
    {
        if (!PyDict_Check(sipPy))
            return 0;

        while (PyDict_Next(sipPy, &i, &keyObj, &dataObj))
        {
            if (!sipCanConvertToType(keyObj, sipType_PCaselessString, SIP_NOT_NONE))
                return 0;

            if (!sipCanConvertToType(dataObj, sipType_PString, SIP_NOT_NONE))
                return 0;
        } 

        return 1;
    }

    PStringToString *pstr = new PStringToString;
 
    while (PyDict_Next(sipPy, &i, &keyObj, &dataObj))
    {
        int state1, state2;

        PCaselessString *t1 = reinterpret_cast<PCaselessString *>(sipConvertToType(keyObj, sipType_PCaselessString, Py_None, SIP_NOT_NONE, &state1, sipIsErr));
        PString *t2 = reinterpret_cast<PString *>(sipConvertToType(dataObj, sipType_PString, Py_None, SIP_NOT_NONE, &state2, sipIsErr));
 
        if (*sipIsErr)
        {
            sipReleaseType(t1, sipType_PCaselessString, state1);
            sipReleaseType(t2, sipType_PString, state2);

            delete pstr;
            return 0;
        }

        pstr->SetAt(*t1, *t2);

        sipReleaseType(t1, sipType_PCaselessString, state1);
        sipReleaseType(t2, sipType_PString, state2);
    }
 
    *sipCppPtr = pstr;
 
    return sipGetState(sipTransferObj);
%End
};

%MappedType PStringOptions
{
%ConvertFromTypeCode
    // Create the dictionary.
    PyObject *d = PyDict_New();

    if (!d)
        return NULL;

    for (PINDEX i = 0; i < sipCpp->GetSize(); i++)
    {
        PCaselessString *key  = new PCaselessString(sipCpp->GetKeyAt(i));
        PString *data = new PString(sipCpp->GetDataAt(i));

        PyObject *keyObj = sipConvertFromNewType(key, sipType_PCaselessString, NULL);
        PyObject *dataObj = sipConvertFromNewType(data, sipType_PString, NULL);

        if (keyObj == NULL || dataObj == NULL || PyDict_SetItem(d, keyObj, dataObj) < 0)
        {
            Py_DECREF(d);
            return NULL;
        }
    }

    return d;
%End

%ConvertToTypeCode
    PyObject *keyObj, *dataObj;
    SIP_SSIZE_T i = 0;

    // Check the type if that is all that is required.
    if (sipIsErr == NULL)
    {
        if (!PyDict_Check(sipPy))
            return 0;

        while (PyDict_Next(sipPy, &i, &keyObj, &dataObj))
        {
            if (!sipCanConvertToType(keyObj, sipType_PCaselessString, SIP_NOT_NONE))
                return 0;

            if (!sipCanConvertToType(dataObj, sipType_PString, SIP_NOT_NONE))
                return 0;
        } 

        return 1;
    }

    PStringOptions *pStrOpts = new PStringOptions;
 
    while (PyDict_Next(sipPy, &i, &keyObj, &dataObj))
    {
        int state1, state2;

        PCaselessString *t1 = reinterpret_cast<PCaselessString *>(sipConvertToType(keyObj, sipType_PCaselessString, Py_None, SIP_NOT_NONE, &state1, sipIsErr));
        PString *t2 = reinterpret_cast<PString *>(sipConvertToType(dataObj, sipType_PString, Py_None, SIP_NOT_NONE, &state2, sipIsErr));
 
        if (*sipIsErr)
        {
            sipReleaseType(t1, sipType_PCaselessString, state1);
            sipReleaseType(t2, sipType_PString, state2);

            delete pStrOpts;
            return 0;
        }

        pStrOpts->SetAt(*t1, *t2);

        sipReleaseType(t1, sipType_PCaselessString, state1);
        sipReleaseType(t2, sipType_PString, state2);
    }
 
    *sipCppPtr = pStrOpts;
 
    return sipGetState(sipTransferObj);
%End
};

%MappedType PWCharArray
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
		unsigned short data = sipCpp->GetAt(i);
		// Add the wrapper to the list.
        PyList_SET_ITEM(l, i, PyLong_FromUnsignedLong(data));
	}
	
	// Return the Python list.
    return l;
%End

%ConvertToTypeCode
	PWCharArray *array = new PWCharArray;

	for(SIP_SSIZE_T i = 0; i < PyList_Size(sipPy); i++)
	{
		PyObject *item = PyList_GetItem(sipPy, i);

		if( item != NULL )
		{
			unsigned short data = (unsigned short)PyLong_AsUnsignedLong(item);
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

typedef PList<PString> PStringList;

%MappedType PStringArray
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
		 PString *pStr = new PString((*sipCpp)[i]);
         PyObject *obj;
		
		 if ((obj = sipConvertFromNewType(pStr, sipType_PString, Py_None)) == NULL)
		 {
			 // There was an error so garbage collect the Python list.
             Py_DECREF(l);
             return NULL;
		 }
		
		 // Add the wrapper to the list.
         PyList_SET_ITEM(l, i, obj);
	 }

     return l;
%End

%ConvertToTypeCode
     // Check the type if that is all that is required.
     if (sipIsErr == NULL)
         return PyList_Check(sipPy);

     PStringArray *pArr = new PStringArray();
	 int state;
 
     for (SIP_SSIZE_T i = 0; i < PyList_GET_SIZE(sipPy); ++i)
	 {
		 if (sipCanConvertToType(PyList_GetItem(sipPy, i), sipType_PString, SIP_NOT_NONE))
		 {
			 PString *t1 = reinterpret_cast<PString *>(sipConvertToType(
																 PyList_GetItem(sipPy, i), 
																 sipType_PString, NULL, 
																 SIP_NOT_NONE, 
																 &state, sipIsErr));
			
			 if (*sipIsErr)
			 {
				 sipReleaseType(t1, sipType_PString, state);
				 delete pArr;
				 return 0;
			 }

			 pArr->AppendString(*t1);
			 sipReleaseType(t1, sipType_PString, state);
		 }		
		 else
		 {
			 delete pArr;			
			 return 0;
		 }
	 }
 
     *sipCppPtr = pArr;
     return sipGetState(sipTransferObj);
%End
};


%MappedType PStringSet
{
%ConvertFromTypeCode
    // Create the list.
    PyObject *s;

    if ((s = PySet_New(NULL)) == NULL)
        return NULL;
		
    // Go through each element in the C++ instance and convert it to a
    // wrapped TYPE.
    for (PINDEX i = 0; i != sipCpp->GetSize(); i++)
    {
		PString *data = reinterpret_cast<PString *>(sipCpp->GetKeyAt(i).Clone());
        PyObject *pobj;

       // Get the Python wrapper for the Type instance, creating a new
       // one if necessary, and transfer ownership to Python.
       if ((pobj = sipConvertFromNewType(data, sipType_PString, NULL)) == NULL)
       {
           // There was an error so garbage collect the Python set.
           Py_DECREF(s);
           return NULL;
       }
		
		// Add the wrapped type to the set.
        PySet_Add(s, pobj);
	}
	
	// Return the Python list.
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
   
	PStringSet *pSet = new PStringSet;
 
	while ((item = PyIter_Next(iterator)))
	{
	   if (!sipCanConvertToType(item, sipType_PString, SIP_NOT_NONE))
	   {
		   PyErr_Format(PyExc_TypeError, "object in iterable cannot be converted to PString");
		   *sipIsErr = 1;
		   break;
	   }

	   int state;
	   PString *pStr = reinterpret_cast<PString *>(
			sipConvertToType(item, sipType_PString, sipTransferObj, SIP_NOT_NONE, &state, sipIsErr));

	   if (!*sipIsErr)
		   pSet->Append(reinterpret_cast<PObject *>(pStr));

	   sipReleaseType(pStr, sipType_PString, state);
	   Py_DECREF(item);
	}

	Py_DECREF(iterator);

	if (*sipIsErr)
	{
	   delete pSet;
	   return NULL;
	}

	*sipCppPtr = pSet;
	return sipGetState(sipTransferObj);
%End
};

/**This class is a standard C++ stream class descendent for reading or writing
   streamed data to or from a <code>PString</code> class.
   
   All of the standard stream I/O operators, manipulators etc will operate on
   the PStringStream class.
 */
class PStringStream
{
  private:
	PStringStream(const PStringStream &);
	
  public:
    /**Create a new, empty, string stream. Data may be output to this stream,
       but attempts to input from it will return end of file.

       The internal string is continually grown as required during output.
     */
    PStringStream();

    /**Create a new, empty, string stream of a fixed size. Data may be output
       to this stream, but attempts to input from it will return end of file.
       When the fixed size is reached then no more data may be output to it.
     */
    PStringStream(
      PINDEX fixedBufferSize
    );

    /**Create a new string stream and initialise it to the provided value. The
       string stream references the same string buffer as the <code>str</code>
       parameter until any output to the string stream is attempted. The
       reference is then broken and the instance of the string stream becomes
       a unique reference to a string buffer.
     */
    PStringStream(
      const PString & str   ///< Initial value for string stream.
    );

    /**Create a new string stream and initialise it with the provided value.
       The stream may be read or written from. Writes will append to the end of
       the string.
     */
    PStringStream(
      const char * cstr   ///< Initial value for the string stream.
    );

    /**Make the current string empty
      */
    virtual PString & MakeEmpty();


    /// Destroy the string stream, deleting the stream buffer
    virtual ~PStringStream();


  protected:
    virtual void AssignContents(const PContainer & cont);
};


/**A class representing a regular expression that may be used for locating
   patterns in strings. The regular expression string is "compiled" into a
   form that is more efficient during the matching. This compiled form
   exists for the lifetime of the PRegularExpression instance.
 */
class PRegularExpression : PObject
{
  public:
  /**@name Constructors & destructors */
  //@{
    /// Flags for compiler options.
    enum {
      /// Use extended regular expressions
      Extended = 1,
      /// Ignore case in search.
      IgnoreCase = 2,
      /**If this bit is set, then anchors do not match at newline
         characters in the string. If not set, then anchors do match
         at newlines.
        */
      AnchorNewLine = 4
    };
    /// Flags for execution options.
    enum {
      /**If this bit is set, then the beginning-of-line operator doesn't match
         the beginning of the string (presumably because it's not the
         beginning of a line).
         If not set, then the beginning-of-line operator does match the
         beginning of the string.
      */
      NotBeginningOfLine = 1,
      /**Like <code>NotBeginningOfLine</code>, except for the end-of-line.  */
      NotEndofLine = 2
    };

    /// Create a new, empty, regular expression
    PRegularExpression();

    /** Create and compile a new regular expression pattern.
     */
    PRegularExpression(
      const PString & pattern,    ///< Pattern to compile
      int flags = IgnoreCase      ///< Pattern match options
    );

    /** Create and compile a new regular expression pattern.
     */
    PRegularExpression(
      const char * cpattern,      ///< Pattern to compile
      int flags = IgnoreCase      ///< Pattern match options
    );

    /**
      * Copy a regular expression
      */
    PRegularExpression(
      const PRegularExpression &
    );

    // /**
      // * Assign a regular expression
      // */
    // PRegularExpression & operator =(
      // const PRegularExpression &
    // );

    /// Release storage for the compiled regular expression.
    ~PRegularExpression();
  //@}


  /**@name Status functions */
  //@{
    /// Error codes.
    enum ErrorCodes {
      /// Success.
      NoError = 0,    
      /// Didn't find a match (for regexec).
      NoMatch,      

      // POSIX regcomp return error codes.  (In the order listed in the standard.)
      /// Invalid pattern.
      BadPattern,  
      /// Not implemented.
      CollateError,  
      /// Invalid character class name.
      BadClassType,  
      /// Trailing backslash.
      BadEscape,    
      /// Invalid back reference.
      BadSubReg,
      /// Unmatched left bracket.
      UnmatchedBracket, 
      /// Parenthesis imbalance.
      UnmatchedParen,
      /// Unmatched <b>\\</b>.
      UnmatchedBrace,
      /// Invalid contents of <b>\\</b>.
      BadBR,        
      /// Invalid range end.
      RangeError,  
      /// Ran out of memory.
      OutOfMemory,
      /// No preceding re for repetition op.
      BadRepitition,

      /* Error codes we've added.  */
      /// Premature end.
      PrematureEnd,
      /// Compiled pattern bigger than 2^16 bytes.
      TooBig,
      /// Unmatched ) or \\); not returned from regcomp.
      UnmatchedRParen,
      /// Miscellaneous error
      NotCompiled
    };

    /**Get the error code for the last Compile() or Execute() operation.

       @return
       Error code.
     */
    ErrorCodes GetErrorCode() const;

    /**Get the text description for the error of the last Compile() or
       Execute() operation.

       @return
       Error text string.
     */
    PString GetErrorText() const;

    /** Return the string which represents the pattern matched by the regular expression. */
    const PString & GetPattern() const;
  //@}

  /**@name Compile & Execute functions */
  //@{
    /** Compiler pattern. */
    PBoolean Compile(
      const PString & pattern,    ///< Pattern to compile
      int flags = IgnoreCase      ///< Pattern match options
    );
    /**Compiler pattern.
       The pattern is compiled into an internal format to speed subsequent
       execution of the pattern match algorithm.

       @return
       true if successfully compiled.
     */
    PBoolean Compile(
      const char * cpattern,      ///< Pattern to compile
      int flags = IgnoreCase      ///< Pattern match options
    );

	
    /** Execute regular expression */
    PBoolean Execute(
      const PString & str,    ///< Source string to search
      PINDEX & start,         ///< First match locations
      PINDEX & len,           ///< Length of match
      int flags           ///< Pattern match options
    ) const;

    /**Execute regular expression.
       Execute the pattern match algorithm using the previously compiled
       pattern.

       The <code>starts</code> array is filled with as many matches as will fit
       into the array. If the the array size is zero then it is set to at least
       one for the first match found.
       
       The <code>ends</code> array is set to the ending position of each
       substring whose start is returned in the <code>starts</code> array. This
       will always be set to the same size as that array.

       @return
       true if successfully compiled.
     */
    PBoolean Execute(
      const char * cstr,      ///< Source string to search
      PIntArray & starts,     ///< List of match locations
      PIntArray & ends,       ///< List of match ends
      int flags = 0           ///< Pattern match options
    ) const;
  //@}

  /**@name Miscellaneous functions */
  //@{
    /**Escape all characters in the <code>str</code> parameter that have a
       special meaning within a regular expression.

       @return
       String with additional escape ('\\') characters.
     */
    static PString EscapeString(
      const PString & str     ///< String to add esacpes to.
    );
  //@} 
};


template<TYPE>
%MappedType PStringDictionary<TYPE>
{
%ConvertFromTypeCode
    // Create the dictionary.
    PyObject *d = PyDict_New();

    if (!d)
        return NULL;

    for (PINDEX i = 0; i < sipCpp->GetSize(); i++)
    {
        TYPE *key  = new TYPE(sipCpp->GetKeyAt(i));
        PString *data = reinterpret_cast<PString*>(sipCpp->GetAt(*key)->Clone());

        PyObject *keyObj = sipConvertFromNewType(key, sipType_TYPE, Py_None);
        PyObject *dataObj = sipConvertFromNewType(data, sipType_PString, Py_None);

        if (keyObj == NULL || dataObj == NULL || PyDict_SetItem(d, keyObj, dataObj) < 0)
        {
            Py_DECREF(d);
            return NULL;
        }
    }

    return d;
%End

%ConvertToTypeCode
    PyObject *keyObj, *dataObj;
    SIP_SSIZE_T i = 0;

    // Check the type if that is all that is required.
    if (sipIsErr == NULL)
    {
        if (!PyDict_Check(sipPy))
            return 0;

        while (PyDict_Next(sipPy, &i, &keyObj, &dataObj))
        {
            if (!sipCanConvertToType(keyObj, sipType_TYPE, SIP_NOT_NONE))
                return 0;

            if (!sipCanConvertToType(dataObj, sipType_PString, SIP_NOT_NONE))
                return 0;
        } 

        return 1;
    }

    PStringDictionary<TYPE> *pdict = new PStringDictionary<TYPE>;
 
    while (PyDict_Next(sipPy, &i, &keyObj, &dataObj))
    {
        int state1, state2;

        TYPE *t1 = reinterpret_cast<TYPE1*>(sipConvertToType(keyObj, sipType_TYPE, sipTransferObj, SIP_NOT_NONE, &state1, sipIsErr));
        PString *t2 = reinterpret_cast<PString*>(sipConvertToType(dataObj, sipType_PString, sipTransferObj, SIP_NOT_NONE, &state2, sipIsErr));
 
        if (*sipIsErr)
        {
            sipReleaseType(t1, sipType_TYPE, state1);
            sipReleaseType(t2, sipType_PString, state2);

            delete pdict;
            return 0;
        }

        pdict->SetAt(*t1, *t2);

        sipReleaseType(t1, sipType_TYPE, state1);
        sipReleaseType(t2, sipType_PString, state2);
    }
 
    *sipCppPtr = pdict;
 
    return sipGetState(sipTransferObj);
%End
};
