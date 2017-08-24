/*
	File:       HTTPSessionInterface.h
	Contains:
*/

#ifndef __HTTP_SESSION_INTERFACE_H__
#define __HTTP_SESSION_INTERFACE_H__

#include <CF.h>
#include <atomic>
#include <TCPSocket.h>
#include <TimeoutTask.h>
#include "HTTPRequestStream.h"
#include "HTTPResponseStream.h"

class HTTPSessionInterface : public Task {
 public:

  static void Initialize();

  HTTPSessionInterface();
  virtual ~HTTPSessionInterface();

  bool IsLiveSession() { return fSocket.IsConnected() && fLiveSession; }

  void RefreshTimeout() { fTimeoutTask.RefreshTimeout(); }

  void IncrementObjectHolderCount() { ++fObjectHolders; }
  void DecrementObjectHolderCount();

  HTTPRequestStream *GetInputStream() { return &fInputStream; }
  HTTPResponseStream *GetOutputStream() { return &fOutputStream; }
  TCPSocket *GetSocket() { return &fSocket; }
  OSMutex *GetSessionMutex() { return &fSessionMutex; }

  UInt32 GetSessionIndex() { return fSessionIndex; }

  void SetRequestBodyLength(SInt32 inLength) { fRequestBodyLen = inLength; }
  SInt32 GetRemainingReqBodyLen() { return fRequestBodyLen; }

  // QTSS STREAM FUNCTIONS

  // Allows non-buffered writes to the client. These will flow control.

  // THE FIRST ENTRY OF THE IOVEC MUST BE BLANK!!!
  virtual CF_Error WriteV(iovec *inVec,
                            UInt32 inNumVectors,
                            UInt32 inTotalLength,
                            UInt32 *outLenWritten);
  virtual CF_Error Write(void *inBuffer,
                           UInt32 inLength,
                           UInt32 *outLenWritten,
                           UInt32 inFlags);
  virtual CF_Error Read(void *ioBuffer, UInt32 inLength, UInt32 *outLenRead);
  virtual CF_Error RequestEvent(CF_EventType inEventMask);

  virtual CF_Error SendHTTPPacket(StrPtrLen *contentXML,
                                    bool connectionClose,
                                    bool decrement);

  enum {
    kMaxUserNameLen = 32,
    kMaxUserPasswordLen = 32
  };

  // SERVER NAME & VERSION

  static StrPtrLen &GetServerName() { return sServerNameStr; }
  static StrPtrLen &GetServerVersion() { return sServerVersionStr; }
  static StrPtrLen &GetServerPlatform() { return sServerPlatformStr; }
  static StrPtrLen &GetServerBuildDate() { return sServerBuildDateStr; }
  static StrPtrLen &GetServerHeader() { return sServerHeaderStr; }
  static StrPtrLen &GetServerBuild() { return sServerBuildStr; }
  static StrPtrLen &GetServerComment() { return sServerCommentStr; }

 protected:
  enum {
    kFirstHTTPSessionID = 1,    //UInt32
  };

  //Each http session has a unique number that identifies it.

  char fUserNameBuf[kMaxUserNameLen];
  char fUserPasswordBuf[kMaxUserPasswordLen];

  TimeoutTask fTimeoutTask; //allows the session to be timed out

  HTTPRequestStream fInputStream;
  HTTPResponseStream fOutputStream;

  // Any RTP session sending interleaved data on this RTSP session must
  // be prevented from writing while an RTSP request is in progress
  OSMutex fSessionMutex;

  //+rt  socket we get from "accept()"
  TCPSocket fSocket;
  TCPSocket *fOutputSocketP;
  TCPSocket *fInputSocketP;  // <-- usually same as fSocketP, unless we're HTTP Proxying

  void SnarfInputSocket(HTTPSessionInterface *fromHTTPSession);

  bool fLiveSession;
  atomic_int fObjectHolders;

  UInt32 fSessionIndex;
  UInt32 fLocalAddr;
  UInt32 fRemoteAddr;
  SInt32 fRequestBodyLen;

  UInt16 fLocalPort;
  UInt16 fRemotePort;

  bool fAuthenticated;

  //static unsigned int	sSessionIndexCounter;
  static std::atomic_uint sSessionIndexCounter;

  // Dictionary support Param retrieval function
  static void *SetupParams(HTTPSessionInterface *inSession, UInt32 *outLen);


  enum {
    kMaxServerHeaderLen = 1000
  };

  static UInt32 sServerAPIVersion;
  static StrPtrLen sServerNameStr;
  static StrPtrLen sServerVersionStr;
  static StrPtrLen sServerBuildStr;
  static StrPtrLen sServerCommentStr;
  static StrPtrLen sServerPlatformStr;
  static StrPtrLen sServerBuildDateStr;
  static char sServerHeader[kMaxServerHeaderLen];
  static StrPtrLen sServerHeaderStr;
};

#endif // __HTTP_SESSION_INTERFACE_H__

