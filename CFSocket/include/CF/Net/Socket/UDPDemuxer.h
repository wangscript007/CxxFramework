/*
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * Copyright (c) 1999-2008 Apple Inc.  All Rights Reserved.
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 *
 */
/*
    File:       UDPDemuxer.h

    Contains:   Provides a "Listener" Socket for UDP. Blocks on a local IP & port,
                waiting for data. When it gets data, it passes it off to a UDPDemuxerTask
                object depending on where it came from.


*/

#ifndef __UDPDEMUXER_H__
#define __UDPDEMUXER_H__

#include <CF/StrPtrLen.h>
#include <CF/HashTable.h>
#include <CF/Core/Mutex.h>

namespace CF {
namespace Net {

class Task;
class UDPDemuxerKey;

//IMPLEMENTATION ONLY:
//HASH TABLE CLASSES USED ONLY IN IMPLEMENTATION

class UDPDemuxerUtils {
 private:

  static UInt32 ComputeHashValue(UInt32 inRemoteAddr, UInt16 inRemotePort) {
    return ((inRemoteAddr << 16) + inRemotePort);
  }

  friend class UDPDemuxerTask;
  friend class UDPDemuxerKey;
};

class UDPDemuxerTask {
 public:

  UDPDemuxerTask()
      : fRemoteAddr(0), fRemotePort(0),
        fHashValue(0), fNextHashEntry(nullptr) {}
  virtual ~UDPDemuxerTask() = default;

  UInt32 GetRemoteAddr() { return fRemoteAddr; }

 private:

  void set(UInt32 inRemoteAddr, UInt16 inRemotePort) {
    fRemoteAddr = inRemoteAddr;
    fRemotePort = inRemotePort;
    fHashValue = UDPDemuxerUtils::ComputeHashValue(fRemoteAddr, fRemotePort);
  }

  //key values
  UInt32 fRemoteAddr;
  UInt16 fRemotePort;

  //precomputed for performance
  UInt32 fHashValue;

  UDPDemuxerTask *fNextHashEntry;

  friend class UDPDemuxerKey;
  friend class UDPDemuxer;
  friend class HashTable<UDPDemuxerTask, UDPDemuxerKey>;
};

class UDPDemuxerKey {
 private:

  //CONSTRUCTOR / DESTRUCTOR:
  UDPDemuxerKey(UInt32 inRemoteAddr, UInt16 inRemotePort)
      : fRemoteAddr(inRemoteAddr), fRemotePort(inRemotePort) {
    fHashValue = UDPDemuxerUtils::ComputeHashValue(inRemoteAddr, inRemotePort);
  }

  ~UDPDemuxerKey() {}

 private:

  //PRIVATE ACCESSORS:
  UInt32 GetHashKey() { return fHashValue; }

  //these functions are only used by the hash table itself. This constructor
  //will break the "Set" functions.
  UDPDemuxerKey(UDPDemuxerTask *elem) : fRemoteAddr(elem->fRemoteAddr),
                                        fRemotePort(elem->fRemotePort),
                                        fHashValue(elem->fHashValue) {}

  friend int operator==(const UDPDemuxerKey &key1, const UDPDemuxerKey &key2) {
    if ((key1.fRemoteAddr == key2.fRemoteAddr) &&
        (key1.fRemotePort == key2.fRemotePort))
      return true;
    return false;
  }

  //data:
  UInt32 fRemoteAddr;
  UInt16 fRemotePort;
  UInt32 fHashValue;

  friend class HashTable<UDPDemuxerTask, UDPDemuxerKey>;
  friend class UDPDemuxer;
};

//CLASSES USED ONLY IN IMPLEMENTATION
typedef HashTable<UDPDemuxerTask, UDPDemuxerKey> UDPDemuxerHashTable;

class UDPDemuxer {
 public:

  UDPDemuxer() : fHashTable(kMaxHashTableSize), fMutex() {}
  ~UDPDemuxer() = default;

  //These functions grab the Mutex and are therefore premptive safe

  // Return values: OS_NoErr, or EPERM if there is already a task registered
  // with this address combination
  OS_Error RegisterTask(UInt32 inRemoteAddr, UInt16 inRemotePort, UDPDemuxerTask *inTaskP);

  // Return values: OS_NoErr, or EPERM if this task / address combination
  // is not registered
  OS_Error UnregisterTask(UInt32 inRemoteAddr, UInt16 inRemotePort, UDPDemuxerTask *inTaskP);

  //Assumes that parent has grabbed the Mutex!
  UDPDemuxerTask *GetTask(UInt32 inRemoteAddr, UInt16 inRemotePort);

  bool AddrInMap(UInt32 inRemoteAddr, UInt16 inRemotePort) {
    return (this->GetTask(inRemoteAddr, inRemotePort) != nullptr);
  }

  Core::Mutex *GetMutex() { return &fMutex; }
  UDPDemuxerHashTable *GetHashTable() { return &fHashTable; }

 private:

  enum {
    kMaxHashTableSize = 2747//is this prime? it should be... //UInt32
  };
  UDPDemuxerHashTable fHashTable;
  Core::Mutex fMutex;//this data structure is shared!
};

} // namespace Net
} // namespace CF

#endif // __UDPDEMUXER_H__
