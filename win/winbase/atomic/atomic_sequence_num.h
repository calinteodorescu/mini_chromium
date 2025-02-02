// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINLIB_WINBASE_ATOMIC_SEQUENCE_NUM_H_
#define WINLIB_WINBASE_ATOMIC_SEQUENCE_NUM_H_

#include <atomic>

#include "winbase\macros.h"

namespace winbase {

// AtomicSequenceNumber is a thread safe increasing sequence number generator.
// Its constructor doesn't emit a static initializer, so it's safe to use as a
// global variable or static member.
class AtomicSequenceNumber {
 public:
  constexpr AtomicSequenceNumber() = default;

  AtomicSequenceNumber(const AtomicSequenceNumber&) = delete;
  AtomicSequenceNumber& operator=(const AtomicSequenceNumber&) = delete;

  // Returns an increasing sequence number starts from 0 for each call.
  // This function can be called from any thread without data race.
  inline int GetNext() { return seq_.fetch_add(1, std::memory_order_relaxed); }

 private:
  std::atomic_int seq_{0};
};

}  // namespace winbase

#endif  // WINLIB_WINBASE_ATOMIC_SEQUENCE_NUM_H_