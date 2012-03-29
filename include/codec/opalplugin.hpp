/*
 * opalplugins.hpp
 *
 * OPAL codec plugins handler (C++ version)
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (C) 2010 Vox Lucida
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.

 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_CODEC_OPALPLUGIN_HPP
#define OPAL_CODEC_OPALPLUGIN_HPP

#include "opalplugin.h"

#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include <map>
#include <string>


/////////////////////////////////////////////////////////////////////////////

#ifndef PLUGINCODEC_TRACING
  #define PLUGINCODEC_TRACING 1
#endif

#if PLUGINCODEC_TRACING
  extern PluginCodec_LogFunction PluginCodec_LogFunctionInstance;
  extern int PluginCodec_SetLogFunction(const PluginCodec_Definition *, void *, const char *, void * parm, unsigned * len);

#define PLUGINCODEC_CONTROL_LOG_FUNCTION_DEF \
  PluginCodec_LogFunction PluginCodec_LogFunctionInstance; \
  int PluginCodec_SetLogFunction(const PluginCodec_Definition *, void *, const char *, void * parm, unsigned * len) \
  { \
    if (len == NULL || *len != sizeof(PluginCodec_LogFunction)) \
      return false; \
 \
    PluginCodec_LogFunctionInstance = (PluginCodec_LogFunction)parm; \
    if (PluginCodec_LogFunctionInstance != NULL) \
      PluginCodec_LogFunctionInstance(4, __FILE__, __LINE__, "Plugin", "Started logging."); \
 \
    return true; \
  } \

  #define PLUGINCODEC_CONTROL_LOG_FUNCTION_INC { PLUGINCODEC_CONTROL_SET_LOG_FUNCTION, PluginCodec_SetLogFunction },
#else
  #define PLUGINCODEC_CONTROL_LOG_FUNCTION_DEF
  #define PLUGINCODEC_CONTROL_LOG_FUNCTION_INC
#endif

#if !defined(PTRACE)
  #if PLUGINCODEC_TRACING
    #include <sstream>
    #define PTRACE_CHECK(level) \
        (PluginCodec_LogFunctionInstance != NULL && PluginCodec_LogFunctionInstance(level, NULL, 0, NULL, NULL))
    #define PTRACE(level, section, args) \
      if (PTRACE_CHECK(level)) { \
        std::ostringstream strm; strm << args; \
        PluginCodec_LogFunctionInstance(level, __FILE__, __LINE__, section, strm.str().c_str()); \
      } else (void)0
  #else
    #define PTRACE_CHECK(level)
    #define PTRACE(level, section, expr)
  #endif
#endif


/////////////////////////////////////////////////////////////////////////////

class PluginCodec_MediaFormat
{
  public:
    typedef struct PluginCodec_Option const * const * OptionsTable;
    typedef std::map<std::string, std::string> OptionMap;

  protected:
    OptionsTable m_options;

  protected:
    PluginCodec_MediaFormat(OptionsTable options)
      : m_options(options)
    {
    }

  public:
    virtual ~PluginCodec_MediaFormat()
    {
    }


    const void * GetOptionsTable() const { return m_options; }

    /// Determine if codec is valid for the specified protocol
    virtual bool IsValidForProtocol(const char * /*protocol*/)
    {
      return true;
    }


    /// Utility function to adjust option strings, used by ToNormalised()/ToCustomised().
    bool AdjustOptions(void * parm, unsigned * parmLen, bool (PluginCodec_MediaFormat:: * adjuster)(OptionMap & original, OptionMap & changed))
    {
      if (parmLen == NULL || parm == NULL || *parmLen != sizeof(char ***)) {
        PTRACE(1, "Plugin", "Invalid parameters to AdjustOptions.");
        return false;
      }

      OptionMap originalOptions;
      for (const char * const * option = *(const char * const * *)parm; *option != NULL; option += 2)
        originalOptions[option[0]] = option[1];

      OptionMap changedOptions;
      if (!(this->*adjuster)(originalOptions, changedOptions)) {
        PTRACE(1, "Plugin", "Could not normalise/customise options.");
        return false;
      }

      char ** options = (char **)calloc(changedOptions.size()*2+1, sizeof(char *));
      *(char ***)parm = options;
      if (options == NULL) {
        PTRACE(1, "Plugin", "Could not allocate new option lists.");
        return false;
      }

      for (OptionMap::iterator i = changedOptions.begin(); i != changedOptions.end(); ++i) {
        *options++ = strdup(i->first.c_str());
        *options++ = strdup(i->second.c_str());
      }

      return true;
    }


    /// Adjust normalised options calculated from codec specific options.
    virtual bool ToNormalised(OptionMap & original, OptionMap & changed) = 0;


    // Adjust codec specific options calculated from normalised options.
    virtual bool ToCustomised(OptionMap & original, OptionMap & changed) = 0;


    static void Change(const char * value,
                       OptionMap  & original,
                       OptionMap  & changed,
                       const char * option)
    {
      OptionMap::iterator it = original.find(option);
      if (it != original.end() && it->second != value)
        changed[option] = value;
    }


    static unsigned String2Unsigned(const std::string & str)
    {
      return strtoul(str.c_str(), NULL, 10);
    }


    static void AppendUnsigned2String(unsigned value, std::string & str)
    {
      // Not very efficient, but really, really simple
      if (value > 9)
        AppendUnsigned2String(value/10, str);
      str += (char)(value%10 + '0');
    }


    static void Unsigned2String(unsigned value, std::string & str)
    {
      str.clear();
      AppendUnsigned2String(value,str);
    }


    static void Change(unsigned     value,
                       OptionMap  & original,
                       OptionMap  & changed,
                       const char * option)
    {
      if (String2Unsigned(original[option]) != value)
        Unsigned2String(value, changed[option]);
    }


    static void ClampMax(unsigned     maximum,
                         OptionMap  & original,
                         OptionMap  & changed,
                         const char * option)
    {
      unsigned value = String2Unsigned(original[option]);
      if (value > maximum)
        Unsigned2String(maximum, changed[option]);
    }


    static void ClampMin(unsigned     minimum,
                         OptionMap  & original,
                         OptionMap  & changed,
                         const char * option)
    {
      unsigned value = String2Unsigned(original[option]);
      if (value < minimum)
        Unsigned2String(minimum, changed[option]);
    }

    virtual void AdjustForVersion(unsigned version, const PluginCodec_Definition * /*definition*/)
    {
      if (version < PLUGIN_CODEC_VERSION_INTERSECT) {
        for (PluginCodec_Option ** options = (PluginCodec_Option **)m_options; *options != NULL; ++options) {
          if (strcmp((*options)->m_name, PLUGINCODEC_MEDIA_PACKETIZATIONS) == 0) {
            *options = NULL;
            break;
          }
        }
      }
    }

    static void AdjustAllForVersion(unsigned version, const PluginCodec_Definition * definitions, size_t size)
    {
      while (size-- > 0) {
        PluginCodec_MediaFormat * info = (PluginCodec_MediaFormat *)definitions->userData;
        if (info != NULL)
          info->AdjustForVersion(version, definitions);
        ++definitions;
      }
    }
};


/////////////////////////////////////////////////////////////////////////////

template<typename NAME>
class PluginCodec
{
  protected:
    PluginCodec(const PluginCodec_Definition * defn)
      : m_definition(defn)
      , m_optionsSame(false)
      , m_maxBitRate(defn->bitsPerSec)
      , m_frameTime((defn->sampleRate/1000*defn->usPerFrame)/1000) // Odd way of calculation to avoid 32 bit integer overflow
    {
      PTRACE(3, "Plugin", "Codec created: \"" << defn->descr
             << "\", \"" << defn->sourceFormat << "\" -> \"" << defn->destFormat << '"');
    }


  public:
    virtual ~PluginCodec()
    {
    }


    /// Complete construction of the plug in codec.
    virtual bool Construct()
    {
      return true;
    }


    /** Terminate operation of plug in codec.
        This is generally not needed but sometimes (e.g. fax) there is some
        clean up required to be done on completion of the codec run.
      */
    static bool Terminate()
    {
      return true;
    }


    /// Convert from one media format to another.
    virtual bool Transcode(const void * fromPtr,
                             unsigned & fromLen,
                                 void * toPtr,
                             unsigned & toLen,
                             unsigned & flags) = 0;


    /// Gather any statistics as a string into the provide buffer.
    virtual bool GetStatistics(char * /*bufferPtr*/, unsigned /*bufferSize*/)
    {
      return true;
    }


    /// Get the required output buffer size to be passed into Transcode.
    virtual size_t GetOutputDataSize()
    {
      return 576-20-16; // Max safe MTU size (576 bytes as per RFC879) minus IP & UDP headers
    }


    /** Set the instance ID for the codec.
        This is used to match up the encode and decoder pairs of instances for
        a given call. While most codecs like G.723.1 are purely unidirectional,
        some a bidirectional and have information flow between encoder and
        decoder.
      */
    virtual bool SetInstanceID(const char * /*idPtr*/, unsigned /*idLen*/)
    {
      return true;
    }


    /// Set all the options for the codec.
    virtual bool SetOptions(const char * const * options)
    {
      m_optionsSame = true;

      // get the media format options after adjustment from protocol negotiation
      for (const char * const * option = options; *option != NULL; option += 2) {
        if (!SetOption(option[0], option[1])) {
          PTRACE(1, "Plugin", "Could not set option \"" << option[0] << "\" to \"" << option[1] << '"');
          return false;
        }
      }

      if (m_optionsSame)
        return true;

      return OnChangedOptions();
    }


    /// Callback for if any options are changed.
    virtual bool OnChangedOptions()
    {
      return true;
    }


    /// Set an individual option of teh given name.
    virtual bool SetOption(const char * optionName, const char * optionValue)
    {
      if (strcasecmp(optionName, PLUGINCODEC_OPTION_TARGET_BIT_RATE) == 0)
        return SetOptionUnsigned(m_maxBitRate, optionValue, 1, m_definition->bitsPerSec);

      if (strcasecmp(optionName, PLUGINCODEC_OPTION_FRAME_TIME) == 0)
        return SetOptionUnsigned(m_frameTime, optionValue, m_definition->sampleRate/1000, m_definition->sampleRate); // 1ms to 1 second

      return true;
    }


    template <typename T>
    bool SetOptionUnsigned(T & oldValue, const char * optionValue, unsigned minimum, unsigned maximum = UINT_MAX)
    {
      unsigned newValue = oldValue;
      if (!SetOptionUnsigned(newValue, optionValue, minimum, maximum))
        return false;
      oldValue = (T)newValue;
      return true;
    }


    bool SetOptionUnsigned(unsigned & oldValue, const char * optionValue, unsigned minimum, unsigned maximum = UINT_MAX)
    {
      char * end;
      unsigned newValue = strtoul(optionValue, &end, 10);
      if (*end != '\0')
        return false;

      if (newValue < minimum)
        newValue = minimum;
      else if (newValue > maximum)
        newValue = maximum;

      if (oldValue != newValue) {
        oldValue = newValue;
        m_optionsSame = false;
      }

      return true;
    }


    template <typename T>
    bool SetOptionBoolean(T & oldValue, const char * optionValue)
    {
      bool opt = oldValue != 0;
      if (!SetOptionBoolean(opt, optionValue))
        return false;
      oldValue = (T)opt;
      return true;
    }


    bool SetOptionBoolean(bool & oldValue, const char * optionValue)
    {
      bool newValue;
      if (     strcasecmp(optionValue, "0") == 0 ||
               strcasecmp(optionValue, "n") == 0 ||
               strcasecmp(optionValue, "f") == 0 ||
               strcasecmp(optionValue, "no") == 0 ||
               strcasecmp(optionValue, "false") == 0)
        newValue = false;
      else if (strcasecmp(optionValue, "1") == 0 ||
               strcasecmp(optionValue, "y") == 0 ||
               strcasecmp(optionValue, "t") == 0 ||
               strcasecmp(optionValue, "yes") == 0 ||
               strcasecmp(optionValue, "true") == 0)
        newValue = true;
      else
        return false;

      if (oldValue != newValue) {
        oldValue = newValue;
        m_optionsSame = false;
      }

      return true;
    }


    bool SetOptionBit(int & oldValue, unsigned bit, const char * optionValue)
    {
      return SetOptionBit((unsigned &)oldValue, bit, optionValue);
    }


    bool SetOptionBit(unsigned & oldValue, unsigned bit, const char * optionValue)
    {
      bool newValue;
      if (strcmp(optionValue, "0") == 0)
        newValue = false;
      else if (strcmp(optionValue, "1") == 0)
        newValue = true;
      else
        return false;

      if (((oldValue&bit) != 0) != newValue) {
        if (newValue)
          oldValue |= bit;
        else
          oldValue &= ~bit;
        m_optionsSame = false;
      }

      return true;
    }


    template <class CodecClass> static void * Create(const PluginCodec_Definition * defn)
    {
      CodecClass * codec = new CodecClass(defn);
      if (codec != NULL && codec->Construct())
        return codec;

      PTRACE(1, "Plugin", "Could not open codec, no context being returned.");
      delete codec;
      return NULL;
    }


    static void Destroy(const PluginCodec_Definition * /*defn*/, void * context)
    {
      delete (PluginCodec *)context;
    }


    static int Transcode(const PluginCodec_Definition * /*defn*/,
                                                 void * context,
                                           const void * fromPtr,
                                             unsigned * fromLen,
                                                 void * toPtr,
                                             unsigned * toLen,
                                         unsigned int * flags)
    {
      if (context != NULL && fromPtr != NULL && fromLen != NULL && toPtr != NULL && toLen != NULL && flags != NULL)
        return ((PluginCodec *)context)->Transcode(fromPtr, *fromLen, toPtr, *toLen, *flags);

      PTRACE(1, "Plugin", "Invalid parameter to Transcode.");
      return false;
    }


    static int GetOutputDataSize(const PluginCodec_Definition *, void * context, const char *, void *, unsigned *)
    {
      return context != NULL ? ((PluginCodec *)context)->GetOutputDataSize() : 0;
    }


    static int ToNormalised(const PluginCodec_Definition * defn, void *, const char *, void * parm, unsigned * len)
    {
      return defn->userData != NULL ? ((PluginCodec_MediaFormat *)defn->userData)->AdjustOptions(parm, len, &PluginCodec_MediaFormat::ToNormalised) : -1;
    }


    static int ToCustomised(const PluginCodec_Definition * defn, void *, const char *, void * parm, unsigned * len)
    {
      return defn->userData != NULL ? ((PluginCodec_MediaFormat *)defn->userData)->AdjustOptions(parm, len, &PluginCodec_MediaFormat::ToCustomised) : -1;
    }


    static int FreeOptions(const PluginCodec_Definition *, void *, const char *, void * parm, unsigned * len)
    {
      if (parm == NULL || len == NULL || *len != sizeof(char ***))
        return false;

      char ** strings = (char **)parm;
      for (char ** string = strings; *string != NULL; string++)
        free(*string);
      free(strings);
      return true;
    }


    static int GetOptions(const struct PluginCodec_Definition * codec, void *, const char *, void * parm, unsigned * len)
    {
      if (parm == NULL || len == NULL || *len != sizeof(struct PluginCodec_Option **))
        return false;

      *(const void **)parm = codec->userData != NULL ? ((PluginCodec_MediaFormat *)codec->userData)->GetOptionsTable() : NULL;
      *len = 0;
      return true;
    }


    static int SetOptions(const PluginCodec_Definition *, void * context, const char *, void * parm, unsigned * len)
    {
      PluginCodec * codec = (PluginCodec *)context;
      return len != NULL && *len == sizeof(const char **) && parm != NULL &&
             codec != NULL && codec->SetOptions((const char * const *)parm);
    }

    static int ValidForProtocol(const PluginCodec_Definition * defn, void *, const char *, void * parm, unsigned * len)
    {
      return len != NULL && *len == sizeof(const char *) && parm != NULL && defn->userData != NULL &&
             ((PluginCodec_MediaFormat *)defn->userData)->IsValidForProtocol((const char *)parm);
    }

    static int SetInstanceID(const PluginCodec_Definition *, void * context, const char *, void * parm, unsigned * len)
    {
      PluginCodec * codec = (PluginCodec *)context;
      return len != NULL && parm != NULL &&
             codec != NULL && codec->SetInstanceID((const char *)parm, *len);
    }

    static int GetStatistics(const PluginCodec_Definition *, void * context, const char *, void * parm, unsigned * len)
    {
      PluginCodec * codec = (PluginCodec *)context;
      return len != NULL && parm != NULL &&
             codec != NULL && codec->GetStatistics((char *)parm, *len);
    }

    static int Terminate(const PluginCodec_Definition *, void * context, const char *, void *, unsigned *)
    {
      PluginCodec * codec = (PluginCodec *)context;
      return codec != NULL && codec->Terminate();
    }

    static struct PluginCodec_ControlDefn * GetControls()
    {
      static PluginCodec_ControlDefn ControlsTable[] = {
        { PLUGINCODEC_CONTROL_GET_OUTPUT_DATA_SIZE,  PluginCodec::GetOutputDataSize },
        { PLUGINCODEC_CONTROL_TO_NORMALISED_OPTIONS, PluginCodec::ToNormalised },
        { PLUGINCODEC_CONTROL_TO_CUSTOMISED_OPTIONS, PluginCodec::ToCustomised },
        { PLUGINCODEC_CONTROL_SET_CODEC_OPTIONS,     PluginCodec::SetOptions },
        { PLUGINCODEC_CONTROL_GET_CODEC_OPTIONS,     PluginCodec::GetOptions },
        { PLUGINCODEC_CONTROL_FREE_CODEC_OPTIONS,    PluginCodec::FreeOptions },
        { PLUGINCODEC_CONTROL_VALID_FOR_PROTOCOL,    PluginCodec::ValidForProtocol },
        { PLUGINCODEC_CONTROL_SET_INSTANCE_ID,       PluginCodec::SetInstanceID },
        { PLUGINCODEC_CONTROL_GET_STATISTICS,        PluginCodec::GetStatistics },
        { PLUGINCODEC_CONTROL_TERMINATE_CODEC,       PluginCodec::Terminate },
        PLUGINCODEC_CONTROL_LOG_FUNCTION_INC
        { NULL }
      };
      return ControlsTable;
    }

  protected:
    const PluginCodec_Definition * m_definition;

    bool     m_optionsSame;
    unsigned m_maxBitRate;
    unsigned m_frameTime;
};


#endif // OPAL_CODEC_OPALPLUGIN_HPP
