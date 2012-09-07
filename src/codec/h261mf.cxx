/*
 * h261mf.cxx
 *
 * H.261 Media Format descriptions
 *
 * Open Phone Abstraction Library
 *
 * Copyright (c) 2012 Vox Lucida Pty. Ltd.
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
 * The Original Code is Open Phone Abstraction Library
 *
 * The Initial Developer of the Original Code is Vox Lucida
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#include <opal/mediafmt.h>
#include <codec/opalpluginmgr.h>
#include <codec/opalplugin.hpp>
#include <asn/h245.h>


static const char H261FormatName[] = OPAL_H261;
static const char QCIF_MPI[] = PLUGINCODEC_QCIF_MPI;
static const char CIF_MPI[]  = PLUGINCODEC_CIF_MPI;


/////////////////////////////////////////////////////////////////////////////

static bool ClampSizes(PluginCodec_OptionMap & original, PluginCodec_OptionMap & changed)
{
  // find bounding box enclosing all MPI values
  int qcif_mpi = original.GetUnsigned(QCIF_MPI, PLUGINCODEC_MPI_DISABLED);
  int  cif_mpi = original.GetUnsigned( CIF_MPI, PLUGINCODEC_MPI_DISABLED);

  changed.SetUnsigned(qcif_mpi != PLUGINCODEC_MPI_DISABLED
                        ? PVideoFrameInfo::QCIFWidth : PVideoFrameInfo::CIFWidth,
                      PLUGINCODEC_OPTION_MIN_RX_FRAME_WIDTH);
  changed.SetUnsigned(qcif_mpi != PLUGINCODEC_MPI_DISABLED
                        ? PVideoFrameInfo::QCIFHeight : PVideoFrameInfo::CIFHeight,
                      PLUGINCODEC_OPTION_MIN_RX_FRAME_HEIGHT);
  changed.SetUnsigned(cif_mpi != PLUGINCODEC_MPI_DISABLED
                        ? PVideoFrameInfo::CIFWidth : PVideoFrameInfo::QCIFWidth,
                      PLUGINCODEC_OPTION_MAX_RX_FRAME_WIDTH);
  changed.SetUnsigned(cif_mpi != PLUGINCODEC_MPI_DISABLED
                        ? PVideoFrameInfo::CIFHeight : PVideoFrameInfo::QCIFHeight,
                      PLUGINCODEC_OPTION_MAX_RX_FRAME_HEIGHT);

  return true;
}



class OpalH261Format : public OpalVideoFormatInternal
{
  public:
    OpalH261Format()
      : OpalVideoFormatInternal(H261FormatName, RTP_DataFrame::DynamicBase, "H261",
                                PVideoFrameInfo::CIFWidth, PVideoFrameInfo::CIFHeight, 30, 1920000)
    {
      OpalMediaOption * option;

      option = new OpalMediaOptionInteger(QCIF_MPI, false, OpalMediaOption::MaxMerge, 1, 1, PLUGINCODEC_MPI_DISABLED);
      OPAL_SET_MEDIA_OPTION_FMTP(option, "QCIF", STRINGIZE(PLUGINCODEC_MPI_DISABLED));
      AddOption(option);

      option = new OpalMediaOptionInteger(CIF_MPI, false, OpalMediaOption::MaxMerge, 1, 1, PLUGINCODEC_MPI_DISABLED);
      OPAL_SET_MEDIA_OPTION_FMTP(option, "CIF", STRINGIZE(PLUGINCODEC_MPI_DISABLED));
      AddOption(option);
    }


    virtual PObject * Clone() const
    {
      return new OpalH261Format(*this);
    }


    virtual bool ToNormalisedOptions()
    {
      return AdjustByOptionMaps(PTRACE_PARAM("ToNormalised",) ClampSizes) && OpalVideoFormatInternal::ToNormalisedOptions();
    }

    virtual bool ToCustomisedOptions()
    {
      return AdjustByOptionMaps(PTRACE_PARAM("ToCustomised",) ClampSizes) && OpalVideoFormatInternal::ToCustomisedOptions();
    }
};


const OpalVideoFormat & GetOpalH261()
{
  static OpalVideoFormat const format(new OpalH261Format());

#if OPAL_H323
  static H323CapabilityFactory::Worker<H323H261Capability> capability(H261FormatName, true);
#endif // OPAL_H323

  return format;
}


// End of File ///////////////////////////////////////////////////////////////
