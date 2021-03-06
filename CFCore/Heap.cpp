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
/**
 * @file Heap.cpp
 *
 * Implements a Heap
 */

#include <string.h>
#include <CF/Heap.h>

using namespace CF;

Heap::Heap(UInt32 inStartSize) : fFreeIndex(1) {
  fArraySize = inStartSize < 2 ? 2 : inStartSize;
  fHeap = new HeapElem *[fArraySize];
}

void Heap::ShiftUp(UInt32 inIndex) {
  UInt32 swapPos = inIndex;
  while (swapPos > 1) {
    // move up the chain until we get to the root, bubbling this new element
    // to its proper place in the tree
    UInt32 nextSwapPos = swapPos >> 1U;

    // if this child is greater than it's parent, we need to do the old
    // switcheroo
    if (fHeap[swapPos]->fValue < fHeap[nextSwapPos]->fValue) {
      HeapElem *temp = fHeap[swapPos];
      fHeap[swapPos] = fHeap[nextSwapPos];
      fHeap[nextSwapPos] = temp;
      swapPos = nextSwapPos;
    } else {
      // if not, we are done!
      break;
    }
  }
}

void Heap::ShiftDown(UInt32 inIndex) {
  UInt32 parent = inIndex;
  while (parent < fFreeIndex) {
    // which is smaller? parent or left child?
    UInt32 greatest = parent;
    UInt32 leftChild = parent * 2;
    if ((leftChild < fFreeIndex) &&
        (fHeap[leftChild]->fValue < fHeap[parent]->fValue))
      greatest = leftChild;

    // which is smaller? the biggest so far or the right child?
    UInt32 rightChild = (parent * 2) + 1;
    if ((rightChild < fFreeIndex) &&
        (fHeap[rightChild]->fValue < fHeap[greatest]->fValue))
      greatest = rightChild;

    // if the parent is in fact smaller than its two children, we have bubbled
    // this element down far enough
    if (greatest == parent)
      break;

    // parent is not smaller than at least one of its two children, so swap the
    // parent with the largest item.
    HeapElem *temp = fHeap[parent];
    fHeap[parent] = fHeap[greatest];
    fHeap[greatest] = temp;

    // now heapify the remaining chain
    parent = greatest;
  }
}

void Heap::Insert(HeapElem *inElem) {
  Assert(inElem != nullptr);

  // extend memory
  if ((fHeap == nullptr) || (fFreeIndex >= fArraySize)) {
    fArraySize *= 2;
    auto **tempArray = new HeapElem *[fArraySize];
    if ((fHeap != nullptr) && (fFreeIndex > 1))
      ::memcpy(tempArray, fHeap, sizeof(HeapElem *) * fFreeIndex);

    delete[] fHeap;
    fHeap = tempArray;
  }

  Assert(fHeap != nullptr);
  Assert(inElem->fCurrentHeap == nullptr);
  Assert(fArraySize > fFreeIndex);

#if CF_HEAP_TESTING
  sanityCheck(1);
#endif

  // insert the element into the last leaf of the tree
  fHeap[fFreeIndex] = inElem;

  // bubble the new element up to its proper place in the Heap
  this->ShiftUp(fFreeIndex);  // start at the last leaf of the tree

  inElem->fCurrentHeap = this;
  fFreeIndex++;
}

HeapElem *Heap::Extract(UInt32 inIndex) {
  if ((fHeap == nullptr) || (fFreeIndex <= inIndex))
    return nullptr;

#if CF_HEAP_TESTING
  sanityCheck(1);
#endif

  // store a reference to the element we want to extract
  HeapElem *victim = fHeap[inIndex];
  Assert(victim->fCurrentHeap == this);
  victim->fCurrentHeap = nullptr;

  // but now we need to preserve this heuristic. We do this by taking
  // the last leaf, putting it at the empty position, then heapifying that chain
  fHeap[inIndex] = fHeap[fFreeIndex - 1];
  fFreeIndex--;

  // The following is an implementation of the Heapify algorithm (CLR 7.1 pp
  // 143) The gist is that this new item at the top of the Heap needs to be
  // bubbled down until it is smaller than its two children, therefore
  // maintaining the Heap property.
  this->ShiftDown(inIndex);

  return victim;
}

HeapElem *Heap::Remove(HeapElem *inElem) {
  if ((fHeap == nullptr) || (fFreeIndex <= 1) || inElem == nullptr)
    return nullptr;

#if CF_HEAP_TESTING
  sanityCheck(1);
#endif

  // elem 是自由的，不是任何堆的成员
  if (!inElem->IsMemberOfAnyHeap()) return nullptr;

  // first attempt to locate this element in the Heap
  UInt32 theIndex = 1;
  for (; theIndex < fFreeIndex; theIndex++)
    if (inElem == fHeap[theIndex]) break;

  // either we've found it, or this is a bogus element
  if (theIndex == fFreeIndex) return nullptr;

  return Extract(theIndex);
}

void Heap::Update(HeapElem *inElem, SInt64 inValue, UInt32 inFlag) {
  if ((fHeap == nullptr) || (fFreeIndex <= 1) || inElem == nullptr)
    return;

  // first attempt to locate this element in the Heap
  UInt32 theIndex = 1;
  for (; theIndex < fFreeIndex; theIndex++)
    if (inElem == fHeap[theIndex]) break;

  // either we've found it, or this is a bogus element
  if (theIndex == fFreeIndex) return;

  if (inValue < fHeap[theIndex]->fValue) {
    if (heapUpdateFlagExpectDown & inFlag) return;
    fHeap[theIndex]->fValue = inValue;
    this->ShiftUp(theIndex);
  } else if (inValue > fHeap[theIndex]->fValue) {
    if (heapUpdateFlagExpectUp & inFlag) return;
    fHeap[theIndex]->fValue = inValue;
    this->ShiftDown(theIndex);
  }
}

#if CF_HEAP_TESTING

void Heap::sanityCheck(UInt32 root) {
  //make sure root is greater than both its children. Do so recursively
  if (root < fFreeIndex) {
    if ((root * 2) < fFreeIndex) {
      Assert(fHeap[root]->fValue <= fHeap[root * 2]->fValue);
      sanityCheck(root * 2);
    }
    if (((root * 2) + 1) < fFreeIndex) {
      Assert(fHeap[root]->fValue <= fHeap[(root * 2) + 1]->fValue);
      sanityCheck((root * 2) + 1);
    }
  }
}

bool Heap::Test() {
  Heap victim(2);
  HeapElem elem1;
  HeapElem elem2;
  HeapElem elem3;
  HeapElem elem4;
  HeapElem elem5;
  HeapElem elem6;
  HeapElem elem7;
  HeapElem elem8;
  HeapElem elem9;

  HeapElem *max = victim.ExtractMin();
  if (max != nullptr)
    return false;

  elem1.SetValue(100);
  victim.Insert(&elem1);

  max = victim.ExtractMin();
  if (max != &elem1)
    return false;
  max = victim.ExtractMin();
  if (max != nullptr)
    return false;

  elem1.SetValue(100);
  elem2.SetValue(80);

  victim.Insert(&elem1);
  victim.Insert(&elem2);

  max = victim.ExtractMin();
  if (max != &elem2)
    return false;
  max = victim.ExtractMin();
  if (max != &elem1)
    return false;
  max = victim.ExtractMin();
  if (max != nullptr)
    return false;

  victim.Insert(&elem2);
  victim.Insert(&elem1);

  max = victim.ExtractMin();
  if (max != &elem2)
    return false;
  max = victim.ExtractMin();
  if (max != &elem1)
    return false;

  elem3.SetValue(70);
  elem4.SetValue(60);

  victim.Insert(&elem3);
  victim.Insert(&elem1);
  victim.Insert(&elem2);
  victim.Insert(&elem4);

  max = victim.ExtractMin();
  if (max != &elem4)
    return false;
  max = victim.ExtractMin();
  if (max != &elem3)
    return false;
  max = victim.ExtractMin();
  if (max != &elem2)
    return false;
  max = victim.ExtractMin();
  if (max != &elem1)
    return false;

  elem5.SetValue(50);
  elem6.SetValue(40);
  elem7.SetValue(30);
  elem8.SetValue(20);
  elem9.SetValue(10);

  victim.Insert(&elem5);
  victim.Insert(&elem3);
  victim.Insert(&elem1);

  max = victim.ExtractMin();
  if (max != &elem5)
    return false;

  victim.Insert(&elem4);
  victim.Insert(&elem2);

  max = victim.ExtractMin();
  if (max != &elem4)
    return false;
  max = victim.ExtractMin();
  if (max != &elem3)
    return false;

  victim.Insert(&elem2);

  max = victim.ExtractMin();
  if (max != &elem2)
    return false;

  victim.Insert(&elem2);
  victim.Insert(&elem6);

  max = victim.ExtractMin();
  if (max != &elem6)
    return false;

  victim.Insert(&elem6);
  victim.Insert(&elem3);
  victim.Insert(&elem4);
  victim.Insert(&elem5);

  max = victim.ExtractMin();
  if (max != &elem6)
    return false;
  max = victim.ExtractMin();
  if (max != &elem5)
    return false;

  victim.Insert(&elem8);
  max = victim.ExtractMin();
  if (max != &elem8)
    return false;
  max = victim.ExtractMin();
  if (max != &elem4)
    return false;

  victim.Insert(&elem5);
  victim.Insert(&elem4);
  victim.Insert(&elem9);
  victim.Insert(&elem7);
  victim.Insert(&elem8);
  victim.Insert(&elem6);

  max = victim.ExtractMin();
  if (max != &elem9)
    return false;
  max = victim.ExtractMin();
  if (max != &elem8)
    return false;
  max = victim.ExtractMin();
  if (max != &elem7)
    return false;
  max = victim.ExtractMin();
  if (max != &elem6)
    return false;
  max = victim.ExtractMin();
  if (max != &elem5)
    return false;
  max = victim.ExtractMin();
  if (max != &elem4)
    return false;
  max = victim.ExtractMin();
  if (max != &elem3)
    return false;
  max = victim.ExtractMin();
  if (max != &elem2)
    return false;
  max = victim.ExtractMin();
  if (max != &elem2)
    return false;
  max = victim.ExtractMin();
  if (max != &elem1)
    return false;
  max = victim.ExtractMin();
  if (max != nullptr)
    return false;

  victim.Insert(&elem1);
  victim.Insert(&elem2);
  victim.Insert(&elem3);
  victim.Insert(&elem4);
  victim.Insert(&elem5);
  victim.Insert(&elem6);
  victim.Insert(&elem7);
  victim.Insert(&elem8);
  victim.Insert(&elem9);

  max = victim.Remove(&elem7);
  if (max != &elem7)
    return false;
  max = victim.Remove(&elem9);
  if (max != &elem9)
    return false;
  max = victim.ExtractMin();
  if (max != &elem8)
    return false;
  max = victim.Remove(&elem2);
  if (max != &elem2)
    return false;
  max = victim.Remove(&elem2);
  if (max != nullptr)
    return false;
  max = victim.Remove(&elem8);
  if (max != nullptr)
    return false;
  max = victim.Remove(&elem5);
  if (max != &elem5)
    return false;
  max = victim.Remove(&elem6);
  if (max != &elem6)
    return false;
  max = victim.Remove(&elem1);
  if (max != &elem1)
    return false;
  max = victim.ExtractMin();
  if (max != &elem4)
    return false;
  max = victim.Remove(&elem1);
  if (max != nullptr)
    return false;

  return true;
}
#endif
