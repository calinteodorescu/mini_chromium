// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINLIB_WINBASE_SYNCHRONIZATION_LOCK_IMPL_H_
#define WINLIB_WINBASE_SYNCHRONIZATION_LOCK_IMPL_H_

#include "winbase\base_export.h"
///#include "winbase\logging.h"
#include "winbase\macros.h"
#include "winlib\build_config.h"


namespace winbase {
namespace internal {

// This class implements the underlying platform-specific spin-lock mechanism
// used for the Lock class.  Most users should not use LockImpl directly, but
// should instead use Lock.
class WINBASE_EXPORT LockImpl {
 public:
  using NativeHandle = struct { void* ptr; };

  LockImpl();
  ~LockImpl();

  LockImpl(const LockImpl&) = delete;
  LockImpl& operator=(const LockImpl&) = delete;

  // If the lock is not held, take it and return true.  If the lock is already
  // held by something else, immediately return false.
  bool Try();

  // Take the lock, blocking until it is available if necessary.
  void Lock();

  // Release the lock.  This must only be called by the lock's holder: after
  // a successful call to Try, or a call to Lock.
  inline void Unlock();

  // Return the native underlying lock.
  // TODO(awalker): refactor lock and condition variables so that this is
  // unnecessary.
  NativeHandle* native_handle() { return &native_handle_; }

 private:
  NativeHandle native_handle_;
};

}  // namespace internal
}  // namespace winbase

#endif  // WINLIB_WINBASE_SYNCHRONIZATION_LOCK_IMPL_H_