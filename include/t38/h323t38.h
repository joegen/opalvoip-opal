/*
 * h323t38.h
 *
 * H.323 T.38 logical channel establishment
 *
 * Open H323 Library
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: h323t38.h,v $
 * Revision 1.2001  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.2  2001/07/24 02:25:57  robertj
 * Added UDP, dual TCP and single TCP modes to T.38 capability.
 *
 * Revision 1.1  2001/07/17 04:44:29  robertj
 * Partial implementation of T.120 and T.38 logical channels.
 *
 */

#ifndef __T38_H323T38_H
#define __T38_H323T38_H

#ifdef __GNUC__
#pragma interface
#endif


#include <h323/h323caps.h>


class OpalT38Protocol;


///////////////////////////////////////////////////////////////////////////////

/**This class describes the T.38 logical channel.
 */
class H323_T38Capability : public H323DataCapability
{
    PCLASSINFO(H323_T38Capability, H323DataCapability);
  public:
  /**@name Construction */
  //@{
    enum TransportMode {
      e_UDP,
      e_DualTCP,
      e_SingleTCP,
      NumTransportModes
    };

    /**Create a new channel.
     */
    H323_T38Capability(
      TransportMode mode
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Compare two capability instances. This compares the main and sub-types
       of the capability.
     */
    Comparison Compare(const PObject & obj) const;

    /**Create a copy of the object.
      */
    virtual PObject * Clone() const;
  //@}

  /**@name Identification functions */
  //@{
    /**Get the sub-type of the capability. This is a code dependent on the
       main type of the capability.

       This returns the e_t38fax enum value from the protocol ASN
       H245_DataApplicationCapability_application class.
     */
    virtual unsigned GetSubType() const;

    /**Get the name of the media data format this class represents.
     */
    virtual PString GetFormatName() const;
  //@}

  /**@name Operations */
  //@{
    /**Create the channel instance, allocating resources as required.
     */
    virtual H323Channel * CreateChannel(
      H323Connection & connection,    /// Owner connection for channel
      H323Channel::Directions dir,    /// Direction of channel
      unsigned sessionID,             /// Session ID for RTP channel
      const H245_H2250LogicalChannelParameters * param
                                      /// Parameters for channel
    ) const;
  //@}

  /**@name Protocol manipulation */
  //@{
    /**This function is called whenever and outgoing TerminalCapabilitySet
       or OpenLogicalChannel PDU is being constructed for the control channel.
       It allows the capability to set the PDU fields from information in
       members specific to the class.

       The default behaviour sets the data rate field in the PDU.
     */
    virtual BOOL OnSendingPDU(
      H245_DataApplicationCapability & pdu
    ) const;

    /**This function is called whenever and incoming TerminalCapabilitySet
       or OpenLogicalChannel PDU has been used to construct the control
       channel. It allows the capability to set from the PDU fields,
       information in members specific to the class.

       The default behaviour gets the data rate field from the PDU.
     */
    virtual BOOL OnReceivedPDU(
      const H245_DataApplicationCapability & pdu  /// PDU to set information on
    );
  //@}

    TransportMode GetTransportMode() const { return mode; }

  protected:
    TransportMode mode;
};


/**This class describes the T.38 logical channel.
 */
class H323_T38Channel : public H323DataChannel
{
    PCLASSINFO(H323_T38Channel, H323DataChannel);
  public:
  /**@name Construction */
  //@{
    /**Create a new channel.
     */
    H323_T38Channel(
      H323Connection & connection,           /// Connection to endpoint for channel
      const H323_T38Capability & capability, /// Capability channel is using
      Directions direction                   /// Direction of channel
    );
  //@}

  /**@name Overrides from class H323Channel */
  //@{
    /**Handle channel data reception.

       This is called by the thread started by the Start() function and is
       a loop reading from the transport and calling HandlePacket() for each
       PDU read.
      */
    virtual void Receive();

    /**Handle channel data transmission.

       This is called by the thread started by the Start() function and is
       typically a loop reading from the codec and writing to the transport
       (eg an RTP_session).
      */
    virtual void Transmit();

    /**Create the OpalListener class to be used.
       This is called on receipt of an OpenLogicalChannel request.

       The default behaviour creates a compatible listener using the
       connections control channel as a basis and returns TRUE if successful.
      */
    virtual BOOL CreateListener();

    /**Create the OpalTransport class to be used.
       This is called on receipt of an OpenLogicalChannelAck response. It
       should not return TRUE unless the transport member variable is set.

       The default behaviour uses the connection signalling channel to create
       the transport and returns TRUE if successful.
      */
    virtual BOOL CreateTransport();
  //@}

  protected:
    BOOL              usesTCP;
    OpalT38Protocol * t38handler;
};


#endif // __T38_H323T38_H


/////////////////////////////////////////////////////////////////////////////
