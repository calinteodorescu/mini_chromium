// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINLIB_WINBASE_SYNCHRONIZATION_LOCK_H_
#define WINLIB_WINBASE_SYNCHRONIZATION_LOCK_H_

#include "winbase\base_export.h"
///#include "winbase\logging.h"
#include "winbase\macros.h"
#include "winbase\synchronization\lock_impl.h"
///#include "winbase\threading\platform_thread.h"
#include "winlib\build_config.h"

namespace winbase {

// A convenient wrapper for an OS specific critical section.  The only real
// intelligence in this class is in debug mode for the support for the
// AssertAcquired() method.
class WINBASE_EXPORT Lock {
 public:
///#if !DCHECK_IS_ON()
   // Optimized wrapper implementation
  Lock() : lock_() {}
  ~Lock() {}
  void Acquire() { lock_.Lock(); }
  void Release() { lock_.Unlock(); }

  // If the lock is not held, take it and return true. If the lock is already
  // held by another thread, immediately return false. This must not be called
  // by a thread already holding the lock (what happens is undefined and an
  // assertion may fail).
  bool Try() { return lock_.Try(); }

  // Null implementation if not debug.
  void AssertAcquired() const {}
///#else
///  Lock();
///  ~Lock();
///  
///  // NOTE: We do not permit recursive locks and will commonly fire a DCHECK() if
///  // a thread attempts to acquire the lock a second time (while already holding
///  // it).
///  void Acquire() {
///    lock_.Lock();
///    CheckUnheldAndMark();
///  }
///  void Release() {
///    CheckHeldAndUnmark();
///    lock_.Unlock();
///  }
///
///  bool Try() {
///    bool rv = lock_.Try();
///    if (rv) {
///      CheckUnheldAndMark();
///    }
///    return rv;
///  }
///
///  void AssertAcquired() const;
///#endif  // DCHECK_IS_ON()

  Lock(const Lock&) = delete;
  Lock& operator=(const Lock&) = delete;

  // Whether Lock mitigates priority inversion when used from different thread
  // priorities.
  static bool HandlesMultipleThreadPriorities() {
    // Windows mitigates priority inversion by randomly boosting the priority of
    // ready threads.
    // https://msdn.microsoft.com/library/windows/desktop/ms684831.aspx
    return true;
  }

  // Both Windows and POSIX implementations of ConditionVariable need to be
  // able to see our lock and tweak our debugging counters, as they release and
  // acquire locks inside of their condition variable APIs.
  friend class ConditionVariable;

 private:
///#if DCHECK_IS_ON()
///  // Members and routines taking care of locks assertions.
///  // Note that this checks for recursive locks and allows them
///  // if the variable is set.  This is allowed by the underlying implementation
///  // on windows but not on Posix, so we're doing unneeded checks on Posix.
///  // It's worth it to share the code.
///  void CheckHeldAndUnmark();
///  void CheckUnheldAndMark();
///
///  // All private data is implicitly protected by lock_.
///  // Be VERY careful to only access members under that lock.
///  winbase::PlatformThreadRef owning_thread_ref_;
///#endif  // DCHECK_IS_ON()

  // Platform specific underlying lock implementation.
  internal::LockImpl lock_;
};

// A helper class that acquires the given Lock while the AutoLock is in scope.
class AutoLock {
 public:
  struct AlreadyAcquired {};

  explicit AutoLock(Lock& lock) : lock_(lock) {
    lock_.Acquire();
  }

  AutoLock(Lock& lock, const AlreadyAcquired&) : lock_(lock) {
    lock_.AssertAcquired();
  }

  ~AutoLock() {
    lock_.AssertAcquired();
    lock_.Release();
  }

  AutoLock(const AutoLock&) = delete;
  AutoLock& operator=(const AutoLock&) = delete;

 private:
  Lock& lock_;
};

// AutoUnlock is a helper that will Release() the |lock| argument in the
// constructor, and re-Acquire() it in the destructor.
class AutoUnlock {
 public:
  explicit AutoUnlock(Lock& lock) : lock_(lock) {
    // We require our caller to have the lock.
    lock_.AssertAcquired();
    lock_.Release();
  }

  ~AutoUnlock() {
    lock_.Acquire();
  }

  AutoUnlock(const AutoUnlock&) = delete;
  AutoUnlock& operator=(const AutoUnlock&) = delete;

 private:
  Lock& lock_;
};

}  // namespace winbase

#endif  // WINLIB_WINBASE_SYNCHRONIZATION_LOCK_H_