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
#include <ptlib/dict.h>
%End

///////////////////////////////////////////////////////////////////////////////
// PDictionary classes

/**This class is used when an ordinal index value is the key for <code>PSet</code>
   and <code>PDictionary</code> classes.
 */
class POrdinalKey : PObject
{
  public:
  /**@name Construction */
  //@{
    /** Create a new key for ordinal index values.
     */
      POrdinalKey(
      PINDEX newKey = 0   ///< Ordinal index value to use as a key.
    );

  //@}

  /**@name Overrides from class PObject */
  //@{
    /// Create a duplicate of the POrdinalKey.
    virtual PObject * Clone() const;

    /* Get the relative rank of the ordinal index. This is a simpel comparison
       of the objects PINDEX values.

       @return
       comparison of the two objects, <code>EqualTo</code> for same,
       <code>LessThan</code> for \p obj logically less than the
       object and <code>GreaterThan</code> for \p obj logically
       greater than the object.
     */
    virtual PObject::Comparison Compare(const PObject & obj) const;

    /**This function calculates a hash table index value for the implementation
       of <code>PSet</code> and <code>PDictionary</code> classes.

       @return
       hash table bucket number.
     */
    virtual PINDEX HashFunction() const;
  //@}

  /**@name New functions for class */
  //@{

    /**Operator to add the ordinal.
      */
      POrdinalKey & operator+=(PINDEX);

    /**Operator to subtract from the ordinal.
      */
      POrdinalKey & operator-=(PINDEX);
  //@}

};


template<TYPE1,TYPE2>
%MappedType PDictionary<TYPE1,TYPE2>
{
%ConvertFromTypeCode
    // Create the dictionary.
    PyObject *d = PyDict_New();

    if (!d)
        return NULL;

    for (PINDEX i = 0; i < sipCpp->GetSize(); i++)
    {
        TYPE1 *key  = new TYPE1(sipCpp->GetKeyAt(i));
        TYPE2 *data = new TYPE2(sipCpp->GetDataAt(i));

        PyObject *keyObj = sipConvertFromNewType(key, sipType_TYPE1, Py_None);
        PyObject *dataObj = sipConvertFromNewType(data, sipType_TYPE2, Py_None);

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
            if (!sipCanConvertToType(keyObj, sipType_TYPE1, SIP_NOT_NONE))
                return 0;

            if (!sipCanConvertToType(dataObj, sipType_TYPE2, SIP_NOT_NONE))
                return 0;
        } 

        return 1;
    }

    PDictionary<TYPE1, TYPE2> *pdict = new PDictionary<TYPE1,TYPE2>;
 
    while (PyDict_Next(sipPy, &i, &keyObj, &dataObj))
    {
        int state1, state2;

        TYPE1 *t1 = reinterpret_cast<TYPE1*>(sipConvertToType(keyObj, sipType_TYPE1, sipTransferObj, SIP_NOT_NONE, &state1, sipIsErr));
        TYPE2 *t2 = reinterpret_cast<TYPE2*>(sipConvertToType(dataObj, sipType_TYPE2, sipTransferObj, SIP_NOT_NONE, &state2, sipIsErr));
 
        if (*sipIsErr)
        {
            sipReleaseType(t1, sipType_TYPE1, state1);
            sipReleaseType(t2, sipType_TYPE2, state2);

            delete pdict;
            return 0;
        }

        pdict->SetAt(*t1, t2);

        sipReleaseType(t1, sipType_TYPE1, state1);
        sipReleaseType(t2, sipType_TYPE2, state2);
    }
 
    *sipCppPtr = pdict;
 
    return sipGetState(sipTransferObj);
%End
};
