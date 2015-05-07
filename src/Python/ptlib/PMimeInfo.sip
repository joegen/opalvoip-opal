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
#include <ptclib/mime.h>
%End

//////////////////////////////////////////////////////////////////////////////
// PMIMEInfo

/** This class contains the Multipurpose Internet Mail Extensions parameters
   and variables.
 */


class PMIMEInfo 
{
  private:
	PMIMEInfo(const PMIMEInfo &);
	
  public:
	PMIMEInfo();
	
    PMIMEInfo(
      PInternetProtocol & socket   ///< Application socket to read MIME info.
    );

  // Overrides from class PStringToString
    /**Add a new value to the MIME info. If the value is already in the
       dictionary then this overrides the previous value.

       @return
       true if the object was successfully added.
     */
    PBoolean SetAt(
      const PCaselessString & key,
      const PString value
    );


    /** Determine if the specified key is present in the MIME information
       set.

       @return
       true if the MIME variable is present.
     */
	 
	PBoolean Contains(
      const PString & key       ///< Key into MIME dictionary to get info.
    );

    int __contains__(const char * key) const;
%MethodCode
	sipRes = sipCpp->Contains(a0);
%End


  // New functions for class.
    /** Read MIME information from the socket.

       @return
       true if the MIME information was successfully read.
     */
    PBoolean Read(
      PInternetProtocol & socket   ///< Application socket to read MIME info.
    );

    /** Write MIME information to the socket.

       @return
       true if the MIME information was successfully read.
     */
    PBoolean Write(
      PInternetProtocol & socket   ///< Application socket to write MIME info.
    ) const;

    /**Add a MIME field given a "name: value" format string.
       Note that if the field name was already in the MIME dictionary then
       this will append the new value after a '\\n' character to the previous
       value.

       @return
       true is a field was added.
      */
    bool AddMIME(
      const PString & line
    );
    bool AddMIME(
      const PString & fieldName, ///< MIME field name
      const PString & fieldValue ///< MIME field value
    );
    bool AddMIME(
      const PMIMEInfo & mime
    );

    /** Get a string for the particular MIME info field with checking for
       existance. The <code>dflt</code> parameter is substituted if the field
       does not exist in the MIME information read in.

       @return
       String for the value of the MIME variable.
     */
    PString GetString(
      const PString & key,       ///< Key into MIME dictionary to get info.
      const PString & dflt = PString::Empty() ///< Default value of field if not in MIME info.
    ) const;

    /** Get an integer value for the particular MIME info field with checking
       for existance. The <code>dflt</code> parameter is substituted if the
       field does not exist in the MIME information read in.

       @return
       Integer value for the MIME variable.
     */
    long GetInteger(
      const PString & key,    ///< Key into MIME dictionary to get info.
      long dflt = 0           ///< Default value of field if not in MIME info.
    ) const;

    /** Set an integer value for the particular MIME info field.
     */
    void SetInteger(
      const PCaselessString & key,  ///< Key into MIME dictionary to get info.
      long value                    ///< New value of field.
    );


    /** Get a complex MIME field.
        This will parse a complex MIME field of the general form:

           key: base-value;tag1=token;tag2="string";tag3
           key: <base-value>;tag1=token;tag2="string";tag3

        The base-value will be placed in the dictionary where the key is the
        empty string. If the base-value is quoted with '<', '>' brackets then
        the brackets are removed. Note that the string "<>" can be used to have
        an empty base-value but a field starting with a ';' is illegal and this
        function will return false.
        
        Each tag will be the key for it's entry in the dictionary, if that tag
        has no '=' sign then it will have an empty string as its value. If the
        tag value is quoted using '"', then the RFC822 rules are applied and
        the quotes and '\\' charcters removed.

        IF there are multiple "key" entries in the MIME, or there is an entry
        of the form:

           key: <base-value>;tag=token, <base-value1>, <base-value2>;tag=token

        then the first entry wil be as described above. All subsequent entries
        are includedin the dictionary with the key names having the string
        "n:" prepended, e.g. "1:" would be "base-value1", "2:tag" would be the
        tag value on the third entry.

        Returns true if the field exists and base-value is non-empty or quoted.
      */
    bool GetComplex(
      const PString & key,    ///< Key into MIME dictionary to get info.
      PStringToString & info  ///< Dictionary of information from field
    ) const;

    /// Parse the string as a complex field, see GetComplex()
    static bool ParseComplex(
      const PString & str,      ///< String value from MIME field.
      PStringToString & info    ///< Dictionary of information from field
    );

    /** Decode parts from a multipart body using the field value.
      */
    bool DecodeMultiPartList(
      PMultiPartList & parts,   ///< Extracted parts.
      const PString & body,     ///< Body to decode
      const PString & key       ///< MIME key for multipart info
    ) const;

    static const PString & ContentTypeTag();
    static const PString & ContentDispositionTag();
    static const PString & ContentTransferEncodingTag();
    static const PString & ContentDescriptionTag();
    static const PString & ContentIdTag();

    /** Decode parts from a multipart body using the field value.
      */
	bool DecodeMultiPartList(
      PMultiPartList & parts,   ///< Extracted parts.
      const PString & body     ///< Body to decode
    ) const;



    /** Set an association between a file type and a MIME content type. The
       content type is then sent for any file in the directory sub-tree that
       has the same extension.

       Note that if the <code>merge</code> parameter if true then the
       dictionary is merged into the current association list and is not a
       simple replacement.

       The default values placed in this dictionary are:
<pre><code>

          ".txt", "text/plain"
          ".text", "text/plain"
          ".html", "text/html"
          ".htm", "text/html"
          ".aif", "audio/aiff"
          ".aiff", "audio/aiff"
          ".au", "audio/basic"
          ".snd", "audio/basic"
          ".wav", "audio/wav"
          ".gif", "image/gif"
          ".xbm", "image/x-bitmap"
          ".tif", "image/tiff"
          ".tiff", "image/tiff"
          ".jpg", "image/jpeg"
          ".jpe", "image/jpeg"
          ".jpeg", "image/jpeg"
          ".avi", "video/avi"
          ".mpg", "video/mpeg"
          ".mpeg", "video/mpeg"
          ".qt", "video/quicktime"
          ".mov", "video/quicktime"
</code></pre>


       The default content type will be "application/octet-stream".
     */
    static void SetAssociation(
      const PStringToString & allTypes,  ///< MIME content type associations.
      PBoolean merge = true                  ///< Flag for merging associations.
    );
    static void SetAssociation(
      const PString & fileType,         ///< File type (extension) to match.
      const PString & contentType       ///< MIME content type string.
    );

    /** Look up the file type to MIME content type association dictionary and
       return the MIME content type string. If the file type is not found in
       the dictionary then the string "application/octet-stream" is returned.

       @return
       MIME content type for file type.
     */
    static PString GetContentType(
      const PString & fileType   ///< File type (extension) to look up.
    );
};


//////////////////////////////////////////////////////////////////////////////
// PMultiPartInfo

/** This object describes the information associated with a multi-part bodies.
  */
class PMultiPartInfo : PObject
{
  public:
    PMIMEInfo  m_mime;
    PString    m_textBody;
    PBYTEArray m_binaryBody;
};

class PMultiPartList 
{

  public:
    PMultiPartList();

    bool Decode(
      const PString & body,               ///< Body to extract parts from
      const PStringToString & contentInfo ///< Content-Type info as decoded from PMIMEInfo::GetComplex()
    );
};




// End Of File ///////////////////////////////////////////////////////////////
