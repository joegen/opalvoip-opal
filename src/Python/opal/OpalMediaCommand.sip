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

///////////////////////////////////////////////////////////////////////////////

/** This is the base class for a command to a media transcoder and/or media
    stream. The commands are highly context sensitive, for example
    VideoFastUpdate would only apply to a video transcoder.
  */
class OpalMediaCommand : PObject /Abstract/
{
%TypeHeaderCode
#include <ptlib.h>
#include <opal/mediacmd.h>
%End
  public:

    /** Compare the two objects and return their relative rank. This function is
       usually overridden by descendent classes to yield the ranking according
       to the semantics of the object.
       
       The default function is to use the #CompareObjectMemoryDirect()#
       function to do a byte wise memory comparison of the two objects.

       @return
       #LessThan#, #EqualTo# or #GreaterThan#
       according to the relative rank of the objects.
     */
    virtual Comparison Compare(
      const PObject & obj   ///<  Object to compare against.
    ) const;
  //@}

  /**@name Operations */
  //@{
    /**Get the name of the command.
      */
    virtual PString GetName() const = 0;

    /**Get data buffer pointer for transfer to/from codec plug-in.
      */
    virtual void * GetPlugInData() const;

    /**Get data buffer size for transfer to/from codec plug-in.
      */
    /*virtual unsigned int * GetPlugInSize() const;*/
  //@}
};