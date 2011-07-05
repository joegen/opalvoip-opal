# Copyright 2011 Demetrius Cassidy
class CallEndReason:
       
    #maps a c{int} cause code to a python c{str} name.
    @classmethod
    def causeCodeToString(cls, code):
        if code < len(cls._causeCodeMap):
            return cls._causeCodeMap[code]
        return None
    
    _causeCodeMap = [
    "EndedByLocalUser",
    "EndedByNoAccept",
    "EndedByAnswerDenied",
    "EndedByRemoteUser",
    "EndedByRefusal",
    "EndedByNoAnswer",
    "EndedByCallerAbort",
    "EndedByTransportFail",
    "EndedByConnectFail",
    "EndedByGatekeeper",
    "EndedByNoUser",
    "EndedByNoBandwidth",
    "EndedByCapabilityExchange",
    "EndedByCallForwarded",
    "EndedBySecurityDenial",
    "EndedByLocalBusy",
    "EndedByLocalCongestion",
    "EndedByRemoteBusy",
    "EndedByRemoteCongestion",
    "EndedByUnreachable",
    "EndedByNoEndPoint",
    "EndedByHostOffline",
    "EndedByTemporaryFailure",
    "EndedByQ931Cause",
    "EndedByDurationLimit",
    "EndedByInvalidConferenceID",
    "EndedByNoDialTone",
    "EndedByNoRingBackTone",
    "EndedByOutOfService",
    "EndedByAcceptingCallWaiting",
    "EndedByGkAdmissionFailed",
    ]
    
class SendUserInputModes:
    #maps a c{int} cause code to a python c{str} name.
    @classmethod
    def userInputModeToString(cls, mode):
        if mode < len(cls._userInputModeMap):
            return cls._userInputModeMap[mode]
        return None
    
    _userInputModeMap = [
      "SendUserInputAsQ931",
      "SendUserInputAsString",
      "SendUserInputAsTone",
      "SendUserInputAsRFC2833",
      "SendUserInputInBand",
      "SendUserInputAsProtocolDefault"]