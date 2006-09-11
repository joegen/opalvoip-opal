#ifndef SPECIALPROCESSOR_H
#define SPECIALPROCESSOR_H

#include <ptlib.h>
#include <opal/connection.h>

#include <iax2/processor.h>
#include <iax2/frame.h>
#include <iax2/iedata.h>
#include <iax2/remote.h>
#include <iax2/safestrings.h>
#include <iax2/sound.h>

/**This is the special processor which is created to handle the weirdo iax2 packets
   that are sent outside of a particular call. Examples of weirdo packets are the
   ping/pong/lagrq/lagrp.
  */
class IAX2SpecialProcessor : public IAX2Processor
{
  PCLASSINFO(IAX2SpecialProcessor, IAX2Processor);
  
 public:
  /**Construct this class */
  IAX2SpecialProcessor(IAX2EndPoint & ep);

  /**Destructor */
  virtual ~IAX2SpecialProcessor();
  
 protected:
  /**Go through the three lists for incoming data (ethernet/sound/UI commands.  */
  virtual void ProcessLists();
  
  /**Processes a full frame*/
  virtual void ProcessFullFrame(IAX2FullFrame & fullFrame);
  
  /**Processes are mini frame*/
  virtual void ProcessNetworkFrame(IAX2MiniFrame * src);
  
  /**Print information about the class on to a stream*/
  virtual void PrintOn(ostream & strm) const;
  
  /**Called when there is no response to a request*/
  virtual void OnNoResponseTimeout();
  
  /**Process an IAX2FullFrameProtocol */
  void ProcessNetworkFrame(IAX2FullFrameProtocol * src);
  
  /**Process a poke command*/
  void ProcessIaxCmdPoke(IAX2FullFrameProtocol * src);
};

#endif /*SPECIALPROCESSOR_H*/
