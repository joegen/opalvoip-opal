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

/**
 This class describes a Universal Resource Locator.
 This is the desciption of a resource location as used by the World Wide
 Web and the #PHTTPSocket# class.
 */
class PURL : PObject
{
%TypeHeaderCode
#include <ptlib.h>
#include <ptclib/url.h> 
%End
  public:
    /**Construct a new URL object from the URL string. */
    PURL();
	
    /**Construct a new URL object from the URL string. */
    PURL(
      const char * cstr,    ///< C string representation of the URL.
      const char * defaultScheme = "http" ///< Default scheme for URL
    );
	
    /**Construct a new URL object from the URL string. */
    PURL(
      const PString & str,  ///< String representation of the URL.
      const char * defaultScheme = "http" ///< Default scheme for URL
    );
	
    /**Construct a new URL object from the file path. */
    PURL(
      const PFilePath & path   ///< File path to turn into a "file:" URL.
    );

    PURL(const PURL & other);
	
    //PURL & operator=(const PURL & other);

  /**@name Overrides from class PObject */
  //@{
    /**Compare the two URLs and return their relative rank.

     @return
       #LessThan#, #EqualTo# or #GreaterThan#
       according to the relative rank of the objects.
     */
    virtual Comparison Compare(
      const PObject & obj   ///< Object to compare against.
    ) const;

    /**This function yields a hash value required by the #PDictionary#
       class. A descendent class that is required to be the key of a dictionary
       should override this function. The precise values returned is dependent
       on the semantics of the class. For example, the #PString# class
       overrides it to provide a hash function for distinguishing text strings.

       The default behaviour is to return the value zero.

       @return
       hash function value for class instance.
     */
    virtual PINDEX HashFunction() const;
 
  /**@name New functions for class. */
  //@{
    /**Parse the URL string into the fields in the object instance. */
    // inline PBoolean Parse(
      // const char * cstr,   ///< URL as a string to parse.
      // const char * defaultScheme = NULL ///< Default scheme for URL
    // );
	
    /**Parse the URL string into the fields in the object instance. */
    PBoolean Parse(
      const PString & str, ///< URL as a string to parse.
      const char * defaultScheme = NULL ///< Default scheme for URL
    );

    /**Print/String output representation formats. */
    enum UrlFormat {
      /// Translate to a string as a full URL
      FullURL,      
      /// Translate to a string as only path
      PathOnly,     
      /// Translate to a string with no scheme or host
      URIOnly,      
      /// Translate to a string with scheme and host/port
      HostPortOnly  
    };

    /**Convert the URL object into its string representation. The parameter
       indicates whether a full or partial representation os to be produced.

       @return
       String representation of the URL.
     */
    PString AsString(
      UrlFormat fmt = FullURL   ///< The type of string to be returned.
    ) const;

    /**Get the "file:" URL as a file path.
       If the URL is not a "file:" URL then returns an empty string.
      */
    PFilePath AsFilePath() const;

    /// Type for translation of strings to URL format,
    enum TranslationType {
      /// Translate a username/password field for a URL.
      LoginTranslation,
      /// Translate the path field for a URL.
      PathTranslation,
      /// Translate the query parameters field for a URL.
      QueryTranslation
    };

    /**Translate a string from general form to one that can be included into
       a URL. All reserved characters for the particular field type are
       escaped.

       @return
       String for the URL ready translation.
     */
    static PString TranslateString(
      const PString & str,    ///< String to be translated.
      TranslationType type    ///< Type of translation.
    );

    /**Untranslate a string from a form that was included into a URL into a
       normal string. All reserved characters for the particular field type
       are unescaped.

       @return
       String from the URL untranslated.
     */
    static PString UntranslateString(
      const PString & str,    ///< String to be translated.
      TranslationType type    ///< Type of translation.
    );

    /** Split a string to a dictionary of names and values. */
    static void SplitVars(
      const PString & str,    ///< String to split into variables.
      PStringToString & vars, ///< Dictionary of variable names and values.
      char sep1,              ///< Separater between pairs
      char sep2               ///< Separater between key and value
    );

    /** Split a string in &= form to a dictionary of names and values. */
    static void SplitQueryVars(
      const PString & queryStr,   ///< String to split into variables.
      PStringToString & queryVars ///< Dictionary of variable names and values.
    );

    /// Get the scheme field of the URL.
    const PCaselessString & GetScheme() const;

    /// Set the scheme field of the URL
    void SetScheme(const PString & scheme);

    /// Get the username field of the URL.
    const PString & GetUserName() const;

    /// Set the username field of the URL.
    void SetUserName(const PString & username);

    /// Get the password field of the URL.
    const PString & GetPassword() const;

    /// Set the password field of the URL.
    void SetPassword(const PString & password);

    /// Get the hostname field of the URL.
    const PCaselessString & GetHostName() const;

    /// Set the hostname field of the URL.
    void SetHostName(const PString & hostname);

    /// Get the port field of the URL.
    WORD GetPort() const;

    /// Set the port field in the URL.
    void SetPort(WORD newPort);
    
    /// Get if explicit port is specified.
    PBoolean GetPortSupplied() const;

    /// Get if path is relative or absolute
    PBoolean GetRelativePath() const;

    /// Get the path field of the URL as a string.
    PString GetPathStr() const;

    /// Set the path field of the URL as a string.
    void SetPathStr(const PString & pathStr);

    /// Get the path field of the URL as a string array.
    const PStringArray & GetPath() const;

    /// Set the path field of the URL as a string array.
    void SetPath(const PStringArray & path);

    /// Append segment to the path field of the URL.
    void AppendPath(const PString & segment);

    /// Get the parameter (;) field of the URL.
    PString GetParameters() const;

    /// Set the parameter (;) field of the URL.
    void SetParameters(const PString & parameters);

    /// Get the parameter (;) field(s) of the URL as a string dictionary.
    /// Note the values have already been translated using UntranslateString
    const PStringToString & GetParamVars() const;

    /// Set the parameter (;) field(s) of the URL as a string dictionary.
    /// Note the values will be translated using TranslateString
    void SetParamVars(const PStringToString & paramVars);

    /// Set the parameter (;) field of the URL as a string dictionary.
    /// Note the values will be translated using TranslateString
    void SetParamVar(
      const PString & key,          ///< Key to add/delete
      const PString & data,         ///< Vlaue to add at key, if empty string may be removed
      bool emptyDataDeletes = true  ///< If true, and data empty string, key is removed
    );

    /// Get the fragment (##) field of the URL.
    const PString & GetFragment() const;

    /// Get the Query (?) field of the URL as a string.
    PString GetQuery() const;

    /// Set the Query (?) field of the URL as a string.
    /// Note the values will be translated using UntranslateString
    void SetQuery(const PString & query);

    /// Get the Query (?) field of the URL as a string dictionary.
    /// Note the values have already been translated using UntranslateString
    const PStringToString & GetQueryVars() const;

    /// Set the Query (?) field(s) of the URL as a string dictionary.
    /// Note the values will be translated using TranslateString
    void SetQueryVars(const PStringToString & queryVars);

    /// Set the Query (?) field of the URL as a string dictionary.
    /// Note the values will be translated using TranslateString
    void SetQueryVar(const PString & key, const PString & data);

    /// Get the contents of URL, data left after all elemetns are parsed out
    const PString & GetContents() const;

    /// Set the contents of URL, data left after all elemetns are parsed out
    void SetContents(const PString & str);

    /// Return PTrue if the URL is an empty string.
    PBoolean IsEmpty() const;

    /**Open the URL in a browser.

       @return
       The browser was successfully opened. This does not mean the URL exists and was
       displayed.
     */
    static PBoolean OpenBrowser(
      const PString & url   ///< URL to open
    );
  //@}

    PBoolean LegacyParse(const PString & url, const PURLLegacyScheme * schemeInfo);
    PString LegacyAsString(PURL::UrlFormat fmt, const PURLLegacyScheme * schemeInfo) const;

  protected:
    void CopyContents(const PURL & other);
    virtual PBoolean InternalParse(
      const char * cstr,         ///< URL as a string to parse.
      const char * defaultScheme ///< Default scheme for URL
    );
    void Recalculate();
};



//////////////////////////////////////////////////////////////////////////////
// PURLScheme

class PURLScheme : PObject /Abstract/
{
  public:
    virtual PString GetName() const = 0;
    virtual PBoolean Parse(const PString & url, PURL & purl) const = 0;
    virtual PString AsString(PURL::UrlFormat fmt, const PURL & purl) const = 0;
};

//////////////////////////////////////////////////////////////////////////////
// PURLLegacyScheme

class PURLLegacyScheme : PURLScheme
{
  public:
    PURLLegacyScheme(
      const char * s,
      bool user    = false,
      bool pass    = false,
      bool host    = false,
      bool def     = false,
      bool defhost = false,
      bool query   = false,
      bool params  = false,
      bool frags   = false,
      bool path    = false,
      bool rel     = false,
      WORD port    = 0
    );

    bool Parse(const PString & url, PURL & purl) const;

    PString AsString(PURL::UrlFormat fmt, const PURL & purl) const;

    PString GetName() const;

    PString scheme;
    bool hasUsername;
    bool hasPassword;
    bool hasHostPort;
    bool defaultToUserIfNoAt;
    bool defaultHostToLocal;
    bool hasQuery;
    bool hasParameters;
    bool hasFragments;
    bool hasPath;
    bool relativeImpliesScheme;
    WORD defaultPort;
};