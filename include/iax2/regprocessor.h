#ifndef REGPROCESSOR_H
#define REGPROCESSOR_H

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <ptclib/random.h>
#include <opal/connection.h>

#include <iax2/frame.h>
#include <iax2/processor.h>
#include <iax2/iedata.h>
#include <iax2/remote.h>
#include <iax2/safestrings.h>
#include <iax2/sound.h>

class IAX2EndPoint;
class IAX2Connection;

class IAX2RegProcessor : public IAX2Processor
{
  PCLASSINFO(IAX2RegProcessor, IAX2Processor);
  
 public:
  /**Construct this class */
  IAX2RegProcessor( IAX2EndPoint & ep, 
                    const PString & host, 
                    const PString & username,
                    const PString & password,
                    PINDEX inRegistrationRefreshTime
                    );

  /**Destructor */
  virtual ~IAX2RegProcessor(); 
  
  /**Unregister from the remote iax2 server.  This method is synchronous. and could
     take until the Timeout peroid to return.  This will also shutdown 
     the main thread.*/
  void Unregister();
  
  PString GetHost() const { return host; };
  PString GetUserName() const { return userName; };
  PString GetPassword() const { return password; };
  
 protected:
  PString host;
  PString userName;
  PString password;
  
  INT registrationRefreshTime;
  
  /** bit mask of the different flags to indicate call status*/
  enum RegistrationState {
    registrationStart    =  1,  /*!< Intial state of registration*/
    registrationHappening,      /*!< The registration process is happening*/
    registrationUnregisterStart,/*!< The unregistration process is about to begin*/
    registrationUnregistering,  /*!< The unregistration process is happening*/
    registrationUnregistered,   
    /*!< The unregistration process is complete and is waiting for termination*/
    registrationWait            /*!< Waiting for the refresh peroid*/
  };
  
  /** The current state of the registration */
  BOOL registrationState;
  
  /** A mutex to protect the registrationState variable and the flow
      of the RegProcessor.*/      
  PMutex stateMutex;
  
#ifdef DOC_PLUS_PLUS
  /**A pwlib callback function to start the registration process again */
  void OnDoRegistration(PTimer &, INT);
#else
  PDECLARE_NOTIFIER(PTimer, IAX2RegProcessor, OnDoRegistration);
#endif

  /** The timer which is used to send a registration message */
  PTimer registrationTimer;
  
  /** Process an authentication request when registering*/
  void ProcessIaxCmdRegAuth(IAX2FullFrameProtocol * src);
  
  /** Process an acknowledgement of a sucessful registration*/
  void ProcessIaxCmdRegAck(IAX2FullFrameProtocol * src);
  
  /** Process a registration rejection*/
  void ProcessIaxCmdRegRej(IAX2FullFrameProtocol * src);
  
  /** Process an authentication request when
      registering doing a registration release*/
  void ProcessIaxCmdUnRegAuth(IAX2FullFrameProtocol * src);
  
  /** Process an acknowledgement of a registration release*/
  void ProcessIaxCmdUnRegAck(IAX2FullFrameProtocol * src);
  
  /** Process a registration release rejection*/
  void ProcessIaxCmdUnRegRej(IAX2FullFrameProtocol * src);
  
  /**This callback is called when a packet fails to get an
     Acknowledgment */
  void OnNoResponseTimeout();
  
  /**A method to cause some of the values in this class to be formatted
     into a printable stream */
  void PrintOn(ostream & strm) const;
  
  /**Go through the three lists for incoming data (ethernet/sound/UI commands.  */
  virtual void ProcessLists();
  
  /**Processes a full frame, and sends it off to the relevant processor. This RegistrationProcessor instance worries about those types 
   common to registration, so is concerned about cmdRegAuth, cmdRegAck etc.*/
  virtual void ProcessFullFrame(IAX2FullFrame & fullFrame);
  
  /**Handles a mini frame - so copes with media.*/
  virtual void ProcessNetworkFrame(IAX2MiniFrame * src);
  
  /**Processes a protocol full frame, well, those components relating to
   * registration.*/
  virtual BOOL ProcessNetworkFrame(IAX2FullFrameProtocol * src);
  
  /**Reset the call, ie: get a new call source number, 
     put the sequence numbers to 0 and reset the timer.
     This solves having to create a new thread for every 
     seperate call.
     */
   void ResetCall();
  
  /**A class that generates random values used for varying 
     the registration timer*/
  PRandom regRandom;
};

#endif // REGPROCESSOR_H
