/*
	Copyright (c) 2012-2016 EasyDarwin.ORG.  All rights reserved.
	Github: https://github.com/EasyDarwin
	WEChat: EasyDarwin
	Website: http://www.easydarwin.org
*/
/*
	File:       HTTPClientResponseStream.h

	Contains:   Object that provides a "buffered WriteV" service. Clients
				can call this function to write to a Socket, and buffer flow
				controlled data in different ways.

				It is derived from StringFormatter, which it uses as an output
				stream buffer. The buffer may grow infinitely.
*/

#ifndef __HTTPClient_RESPONSE_STREAM_H__
#define __HTTPClient_RESPONSE_STREAM_H__

#include <CF/CFDef.h>
#include <CF/ResizeableStringFormatter.h>
#include <CF/Net/Socket/ClientSocket.h>
#include <CF/Thread/TimeoutTask.h>

namespace CF {
namespace Net {

class HTTPClientResponseStream : public ResizeableStringFormatter {
 public:

  // This object provides some flow control buffering services.
  // It also refreshes the timeout whenever there is a successful write
  // on the Socket.
  HTTPClientResponseStream(ClientSocket *inSocket,
                           Thread::TimeoutTask *inTimeoutTask)
      : ResizeableStringFormatter(fOutputBuf, kOutputBufferSizeInBytes),
        fSocket(inSocket),
        fBytesSentInBuffer(0),
        fTimeoutTask(inTimeoutTask),
        fPrintMsg(true) {}

  virtual ~HTTPClientResponseStream() {}

  // WriteV
  //
  // This function takes an input ioVec and writes it to the Socket. If any
  // data has been written to this stream via Put, that data gets written first.
  //
  // In the event of flow control on the Socket, less data than what was
  // requested, or no data at all, may be sent. Specify what you want this
  // function to do with the unsent data via inSendType.
  //
  // kAlwaysBuffer:   Buffer any unsent data internally.
  // kAllOrNothing:   If no data could be sent, return EWOULDBLOCK. Otherwise,
  //                  buffer any unsent data.
  // kDontBuffer:     Never buffer any data.
  //
  // If some data ends up being buffered, outLengthSent will = inTotalLength,
  // and the return value will be QTSS_NoErr

  enum {
    kDontBuffer = 0,
    kAllOrNothing = 1,
    kAlwaysBuffer = 2
  };

  CF_Error WriteV(iovec *inVec, UInt32 inNumVectors, UInt32 inTotalLength,
                  UInt32 *outLengthSent, UInt32 inSendType);

  // Flushes any buffered data to the Socket. If all data could be sent,
  // this returns QTSS_NoErr, otherwise, it returns EWOULDBLOCK
  CF_Error Flush();

  void ShowMsg(bool enable) { fPrintMsg = enable; }

  // Use a different TCPSocket to read request data
  void AttachToSocket(ClientSocket *sock) {
    ResetResponseBuffer();
    fSocket = sock;
  }

  void ResetResponseBuffer();

 private:

  enum {
    kOutputBufferSizeInBytes = 64 * 1024 - 1  //64k Buffer UInt32
  };

  //The default buffer size is allocated inline as part of the object. Because this size
  //is good enough for 99.9% of all requests, we avoid the dynamic memory allocation in most
  //cases. But if the response is too big for this buffer, the BufferIsFull function will
  //allocate a larger buffer.
  char fOutputBuf[kOutputBufferSizeInBytes];
  ClientSocket *fSocket;
  UInt32 fBytesSentInBuffer;
  Thread::TimeoutTask *fTimeoutTask;
  bool fPrintMsg;     // debugging printfs

  friend class HTTPPacket;
};

} // namespace Net
} // namespace CF

#endif // __HTTPClient_RESPONSE_STREAM_H__
