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
    File:       OSMutex.cpp

    Contains:

*/

#include <CF/Core/RWMutex.h>

using namespace CF::Core;

#if DEBUG_MUTEXRW
int RWMutex::fCount = 0;
int RWMutex::fMaxCount = 0;
#endif

#if DEBUG_MUTEXRW
void RWMutex::CountConflict(int i) {
  fCount += i;
  if (i == -1) s_printf("Num Conflicts: %d\n", fMaxCount);
  if (fCount > fMaxCount)
    fMaxCount = fCount;

}
#endif

void RWMutex::LockRead() {
  MutexLocker locker(&fInternalLock);
#if DEBUG_MUTEXRW
  if (fState != 0) {
    s_printf(
        "LockRead(conflict) fState = %d active readers = %d, waiting writers = %d, waiting readers=%d\n",
        fState,
        fActiveReaders,
        fWriteWaiters,
        fReadWaiters);
    CountConflict(1);
  }

#endif

  addReadWaiter();
  while (activeWriter() || waitingWriters()) {
    // active writer so wait, reader must wait for write waiters

    fReadersCond.Wait(&fInternalLock, RWMutex::eMaxWait);
  }

  removeReadWaiter();
  addActiveReader(); // add 1 to active readers
  fActiveReaders = fState;

#if DEBUG_MUTEXRW
  //  s_printf("LockRead(conflict) fState = %d active readers = %d, waiting writers = %d, waiting readers=%d\n",fState,  fActiveReaders, fWriteWaiters, fReadWaiters);

#endif
}

void RWMutex::LockWrite() {
  MutexLocker locker(&fInternalLock);
  addWriteWaiter();       //  1 writer queued
#if DEBUG_MUTEXRW

  if (active()) {
    s_printf(
        "LockWrite(conflict) state = %d active readers = %d, waiting writers = %d, waiting readers=%d\n",
        fState,
        fActiveReaders,
        fWriteWaiters,
        fReadWaiters);
    CountConflict(1);
  }

  s_printf(
      "LockWrite 'waiting' fState = %d locked active readers = %d, waiting writers = %d, waiting readers=%d\n",
      fState,
      fActiveReaders,
      fReadWaiters,
      fWriteWaiters);
#endif

  while (activeReaders()) { // active readers
    fWritersCond.Wait(&fInternalLock, RWMutex::eMaxWait);
  }

  removeWriteWaiter(); // remove from waiting writers
  setState(RWMutex::eActiveWriterState);    // this is the active writer
  fActiveReaders = fState;
#if DEBUG_MUTEXRW
  //  s_printf("LockWrite 'locked' fState = %d locked active readers = %d, waiting writers = %d, waiting readers=%d\n",fState, fActiveReaders, fReadWaiters, fWriteWaiters);
#endif

}

void RWMutex::Unlock() {
  MutexLocker locker(&fInternalLock);
#if DEBUG_MUTEXRW
  //  s_printf("Unlock active readers = %d, waiting writers = %d, waiting readers=%d\n", fActiveReaders, fReadWaiters, fWriteWaiters);

#endif

  if (activeWriter()) {
    setState(RWMutex::eNoWriterState); // this was the active writer
    if (waitingWriters()) { // there are waiting writers
      fWritersCond.Signal();
    } else {
      fReadersCond.Broadcast();
    }
#if DEBUG_MUTEXRW
    s_printf(
        "Unlock(writer) active readers = %d, waiting writers = %d, waiting readers=%d\n",
        fActiveReaders,
        fReadWaiters,
        fWriteWaiters);
#endif
  } else {
    removeActiveReader(); // this was a reader
    if (!activeReaders()) { // no active readers
      setState(RWMutex::eNoWriterState); // this was the active writer now no actives threads
      fWritersCond.Signal();
    }
  }
  fActiveReaders = fState;

}

// Returns true on successful grab of the lock, false on failure
int RWMutex::TryLockWrite() {
  int status = EBUSY;
  MutexLocker locker(&fInternalLock);

  if (!active() && !waitingWriters()) {
    // no writers, no readers, no waiting writers

    this->LockWrite();
    status = 0;
  }

  return status;
}

int RWMutex::TryLockRead() {
  int status = EBUSY;
  MutexLocker locker(&fInternalLock);

  if (!activeWriter() && !waitingWriters()) {
    // no current writers but other readers ok
    this->LockRead();
    status = 0;
  }

  return status;
}



