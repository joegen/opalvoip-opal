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

class PQoS :  PObject
{
%TypeHeaderCode
#include <ptlib.h>
#include <ptlib/qos.h>
%End
public:
    PQoS();
    PQoS(unsigned long avgBytesPerSec,
         unsigned long winServiceType,
         int DSCPalternative = -1,
         unsigned long maxFrameBytes = 1500,
         unsigned long peakBytesPerSec = QOS_NOT_SPECIFIED);
    PQoS(int DSCPvalue);

    void SetAvgBytesPerSec(unsigned long avgBytesPerSec);
    void SetWinServiceType(unsigned long winServiceType);
    void SetDSCP(int DSCPvalue);
    void SetMaxFrameBytes(unsigned long maxFrameBytes);
    void SetPeakBytesPerSec(unsigned long peakBytesPerSec);

    unsigned long GetTokenRate() const;
    unsigned long GetTokenBucketSize() const;
    unsigned long GetPeakBandwidth() const;
    unsigned long GetServiceType() const;
    int GetDSCP() const;

    static void SetDSCPAlternative(unsigned long winServiceType,
                                   unsigned int dscp);
    static char bestEffortDSCP;
    static char controlledLoadDSCP;
    static char guaranteedDSCP;

};