// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WINLIB_WINBASE_MEMORY_REF_COUNTED_H_
#define WINLIB_WINBASE_MEMORY_REF_COUNTED_H_

#include <stddef.h>

#include <utility>

#include "winbase\base_export.h"
#include "winbase\compiler_specific.h"
///#include "winbase\logging.h"
#include "winbase\macros.h"
#include "winbase\atomic\atomic_ref_count.h"
#include "winbase\memory\scoped_refptr.h"
///#include "winbase\sequence_checker.h"
///#include "winbase\threading\thread_collision_warner.h"
#include "winlib\build_config.h"

namespace winbase {
namespace subtle {

class WINBASE_EXPORT RefCountedBase {
 public:
  bool HasOneRef() const { return ref_count_ == 1; }

 protected:
  explicit RefCountedBase(StartRefCountFromZeroTag) {
///#if DCHECK_IS_ON()
///    sequence_checker_.DetachFromSequence();
///#endif
  }

  explicit RefCountedBase(StartRefCountFromOneTag) : ref_count_(1) {
///#if DCHECK_IS_ON()
///    needs_adopt_ref_ = true;
///    sequence_checker_.DetachFromSequence();
///#endif
  }

  ~RefCountedBase() {
///#if DCHECK_IS_ON()
///    DCHECK(in_dtor_) << "RefCounted object deleted without calling Release()";
///#endif
  }

  RefCountedBase(const RefCountedBase&) = delete;
  RefCountedBase& operator=(const RefCountedBase&) = delete;

  void AddRef() const {
    // TODO(maruel): Add back once it doesn't assert 500 times/sec.
    // Current thread books the critical section "AddRelease"
    // without release it.
    // WINBASE_DFAKE_SCOPED_LOCK_THREAD_LOCKED(add_release_);
///#if DCHECK_IS_ON()
///    DCHECK(!in_dtor_);
///    DCHECK(!needs_adopt_ref_)
///        << "This RefCounted object is created with non-zero reference count."
///        << " The first reference to such a object has to be made by AdoptRef or"
///        << " MakeRefCounted.";
///    if (ref_count_ >= 1) {
///      DCHECK(CalledOnValidSequence());
///    }
///#endif

    AddRefImpl();
  }

  // Returns true if the object should self-delete.
  bool Release() const {
    --ref_count_;

    // TODO(maruel): Add back once it doesn't assert 500 times/sec.
    // Current thread books the critical section "AddRelease"
    // without release it.
    // WINBASE_DFAKE_SCOPED_LOCK_THREAD_LOCKED(add_release_);

///#if DCHECK_IS_ON()
///    DCHECK(!in_dtor_);
///    if (ref_count_ == 0)
///      in_dtor_ = true;
///
///    if (ref_count_ >= 1)
///      DCHECK(CalledOnValidSequence());
///    if (ref_count_ == 1)
///      sequence_checker_.DetachFromSequence();
///#endif

    return ref_count_ == 0;
  }

  // Returns true if it is safe to read or write the object, from a thread
  // safety standpoint. Should be DCHECK'd from the methods of RefCounted
  // classes if there is a danger of objects being shared across threads.
  //
  // This produces fewer false positives than adding a separate SequenceChecker
  // into the subclass, because it automatically detaches from the sequence when
  // the reference count is 1 (and never fails if there is only one reference).
  //
  // This means unlike a separate SequenceChecker, it will permit a singly
  // referenced object to be passed between threads (not holding a reference on
  // the sending thread), but will trap if the sending thread holds onto a
  // reference, or if the object is accessed from multiple threads
  // simultaneously.
  bool IsOnValidSequence() const {
///#if DCHECK_IS_ON()
///    return ref_count_ <= 1 || CalledOnValidSequence();
///#else
    return true;
///#endif
  }

 private:
  template <typename U>
  friend scoped_refptr<U> winbase::AdoptRef(U*);

  void Adopted() const {
///#if DCHECK_IS_ON()
///    DCHECK(needs_adopt_ref_);
///    needs_adopt_ref_ = false;
///#endif
  }

#if defined(ARCH_CPU_64_BITS)
  void AddRefImpl() const;
#else
  void AddRefImpl() const { ++ref_count_; }
#endif

///#if DCHECK_IS_ON()
///  bool CalledOnValidSequence() const;
///#endif

  mutable uint32_t ref_count_ = 0;

///#if DCHECK_IS_ON()
///  mutable bool needs_adopt_ref_ = false;
///  mutable bool in_dtor_ = false;
///  mutable SequenceChecker sequence_checker_;
///#endif

///  WINBASE_DFAKE_MUTEX(add_release_);
};

class WINBASE_EXPORT RefCountedThreadSafeBase {
 public:
  bool HasOneRef() const;

 protected:
  explicit constexpr RefCountedThreadSafeBase(StartRefCountFromZeroTag) {}
  explicit constexpr RefCountedThreadSafeBase(StartRefCountFromOneTag)
      : ref_count_(1) {
///#if DCHECK_IS_ON()
///    needs_adopt_ref_ = true;
///#endif
  }

///#if DCHECK_IS_ON()
///  ~RefCountedThreadSafeBase();
///#else
  ~RefCountedThreadSafeBase() = default;
///#endif

  RefCountedThreadSafeBase(const RefCountedThreadSafeBase&) = delete;
  RefCountedThreadSafeBase& operator=(const RefCountedThreadSafeBase&) = delete;

// Release and AddRef are suitable for inlining on X86 because they generate
// very small code sequences. On other platforms (ARM), it causes a size
// regression and is probably not worth it.
#if defined(ARCH_CPU_X86_FAMILY)
  // Returns true if the object should self-delete.
  bool Release() const { return ReleaseImpl(); }
  void AddRef() const { AddRefImpl(); }
#else
  // Returns true if the object should self-delete.
  bool Release() const;
  void AddRef() const;
#endif

 private:
  template <typename U>
  friend scoped_refptr<U> winbase::AdoptRef(U*);

  void Adopted() const {
///#if DCHECK_IS_ON()
///    DCHECK(needs_adopt_ref_);
///    needs_adopt_ref_ = false;
///#endif
  }

  ALWAYS_INLINE void AddRefImpl() const {
///#if DCHECK_IS_ON()
///    DCHECK(!in_dtor_);
///    DCHECK(!needs_adopt_ref_)
///        << "This RefCounted object is created with non-zero reference count."
///        << " The first reference to such a object has to be made by AdoptRef or"
///        << " MakeRefCounted.";
///#endif
    ref_count_.Increment();
  }

  ALWAYS_INLINE bool ReleaseImpl() const {
///#if DCHECK_IS_ON()
///    DCHECK(!in_dtor_);
///    DCHECK(!ref_count_.IsZero());
///#endif
    if (!ref_count_.Decrement()) {
///#if DCHECK_IS_ON()
///      in_dtor_ = true;
///#endif
      return true;
    }
    return false;
  }

  mutable AtomicRefCount ref_count_{0};
///#if DCHECK_IS_ON()
///  mutable bool needs_adopt_ref_ = false;
///  mutable bool in_dtor_ = false;
///#endif
};

}  // namespace subtle

// ScopedAllowCrossThreadRefCountAccess disables the check documented on
// RefCounted below for rare pre-existing use cases where thread-safety was
// guaranteed through other means (e.g. explicit sequencing of calls across
// execution sequences when bouncing between threads in order). New callers
// should refrain from using this (callsites handling thread-safety through
// locks should use RefCountedThreadSafe per the overhead of its atomics being
// negligible compared to locks anyways and callsites doing explicit sequencing
// should properly std::move() the ref to avoid hitting this check).
// TODO(tzik): Cleanup existing use cases and remove
// ScopedAllowCrossThreadRefCountAccess.
class WINBASE_EXPORT ScopedAllowCrossThreadRefCountAccess final {
 public:
///#if DCHECK_IS_ON()
///  ScopedAllowCrossThreadRefCountAccess();
///  ~ScopedAllowCrossThreadRefCountAccess();
///#else
  ScopedAllowCrossThreadRefCountAccess() {}
  ~ScopedAllowCrossThreadRefCountAccess() {}
///#endif
};

//
// A base class for reference counted classes.  Otherwise, known as a cheap
// knock-off of WebKit's RefCounted<T> class.  To use this, just extend your
// class from it like so:
//
//   class MyFoo : public winbase::RefCounted<MyFoo> {
//    ...
//    private:
//     friend class winbase::RefCounted<MyFoo>;
//     ~MyFoo();
//   };
//
// You should always make your destructor non-public, to avoid any code deleting
// the object accidently while there are references to it.
//
//
// The ref count manipulation to RefCounted is NOT thread safe and has DCHECKs
// to trap unsafe cross thread usage. A subclass instance of RefCounted can be
// passed to another execution sequence only when its ref count is 1. If the ref
// count is more than 1, the RefCounted class verifies the ref updates are made
// on the same execution sequence as the previous ones. The subclass can also
// manually call IsOnValidSequence to trap other non-thread-safe accesses; see
// the documentation for that method.
//
//
// The reference count starts from zero by default, and we intended to migrate
// to start-from-one ref count. 
// Put WINBASE_REQUIRE_ADOPTION_FOR_REFCOUNTED_TYPE() to the ref counted class 
// to opt-in.
//
// If an object has start-from-one ref count, the first scoped_refptr need to be
// created by winbase::AdoptRef() or winbase::MakeRefCounted(). We can use
// winbase::MakeRefCounted() to create create both type of ref counted object.
//
// The motivations to use start-from-one ref count are:
//  - Start-from-one ref count doesn't need the ref count increment for the
//    first reference.
//  - It can detect an invalid object acquisition for a being-deleted object
//    that has zero ref count. That tends to happen on custom deleter that
//    delays the deletion.
//    TODO(tzik): Implement invalid acquisition detection.
//  - Behavior parity to Blink's WTF::RefCounted, whose count starts from one.
//    And start-from-one ref count is a step to merge WTF::RefCounted into
//    winbase::RefCounted.
//
#define WINBASE_REQUIRE_ADOPTION_FOR_REFCOUNTED_TYPE()        \
  static constexpr ::winbase::subtle::StartRefCountFromOneTag \
      kRefCountPreference = ::winbase::subtle::kStartRefCountFromOneTag

template <class T, typename Traits>
class RefCounted;

template <typename T>
struct DefaultRefCountedTraits {
  static void Destruct(const T* x) {
    RefCounted<T, DefaultRefCountedTraits>::DeleteInternal(x);
  }
};

template <class T, typename Traits = DefaultRefCountedTraits<T>>
class RefCounted : public subtle::RefCountedBase {
 public:
  static constexpr subtle::StartRefCountFromZeroTag kRefCountPreference =
      subtle::kStartRefCountFromZeroTag;

  RefCounted() : subtle::RefCountedBase(T::kRefCountPreference) {}

  RefCounted(const RefCounted&) = delete;
  RefCounted& operator=(const RefCounted&) = delete;

  void AddRef() const {
    subtle::RefCountedBase::AddRef();
  }

  void Release() const {
    if (subtle::RefCountedBase::Release()) {
      // Prune the code paths which the static analyzer may take to simulate
      // object destruction. Use-after-free errors aren't possible given the
      // lifetime guarantees of the refcounting system.
      ///ANALYZER_SKIP_THIS_PATH();

      Traits::Destruct(static_cast<const T*>(this));
    }
  }

 protected:
  ~RefCounted() = default;

 private:
  friend struct DefaultRefCountedTraits<T>;
  template <typename U>
  static void DeleteInternal(const U* x) {
    delete x;
  }
};

// Forward declaration.
template <class T, typename Traits> class RefCountedThreadSafe;

// Default traits for RefCountedThreadSafe<T>.  Deletes the object when its ref
// count reaches 0.  Overload to delete it on a different thread etc.
template<typename T>
struct DefaultRefCountedThreadSafeTraits {
  static void Destruct(const T* x) {
    // Delete through RefCountedThreadSafe to make child classes only need to be
    // friend with RefCountedThreadSafe instead of this struct, which is an
    // implementation detail.
    RefCountedThreadSafe<T,
                         DefaultRefCountedThreadSafeTraits>::DeleteInternal(x);
  }
};

//
// A thread-safe variant of RefCounted<T>
//
//   class MyFoo : public winbase::RefCountedThreadSafe<MyFoo> {
//    ...
//   };
//
// If you're using the default trait, then you should add compile time
// asserts that no one else is deleting your object.  i.e.
//    private:
//     friend class winbase::RefCountedThreadSafe<MyFoo>;
//     ~MyFoo();
//
// We can use WINBASE_REQUIRE_ADOPTION_FOR_REFCOUNTED_TYPE() with 
// RefCountedThreadSafe too. See the comment above the RefCounted 
// definition for details.
template <class T, typename Traits = DefaultRefCountedThreadSafeTraits<T> >
class RefCountedThreadSafe : public subtle::RefCountedThreadSafeBase {
 public:
  static constexpr subtle::StartRefCountFromZeroTag kRefCountPreference =
      subtle::kStartRefCountFromZeroTag;

  explicit RefCountedThreadSafe()
      : subtle::RefCountedThreadSafeBase(T::kRefCountPreference) {}

  RefCountedThreadSafe(const RefCountedThreadSafe&) = delete;
  RefCountedThreadSafe& operator=(const RefCountedThreadSafe&) = delete;

  void AddRef() const {
    subtle::RefCountedThreadSafeBase::AddRef();
  }

  void Release() const {
    if (subtle::RefCountedThreadSafeBase::Release()) {
      ///ANALYZER_SKIP_THIS_PATH();
      Traits::Destruct(static_cast<const T*>(this));
    }
  }

 protected:
  ~RefCountedThreadSafe() = default;

 private:
  friend struct DefaultRefCountedThreadSafeTraits<T>;
  template <typename U>
  static void DeleteInternal(const U* x) {
    delete x;
  }
};

//
// A thread-safe wrapper for some piece of data so we can place other
// things in scoped_refptrs<>.
//
template<typename T>
class RefCountedData
    : public winbase::RefCountedThreadSafe< winbase::RefCountedData<T> > {
 public:
  RefCountedData() : data() {}
  RefCountedData(const T& in_value) : data(in_value) {}
  RefCountedData(T&& in_value) : data(std::move(in_value)) {}

  T data;

 private:
  friend class winbase::RefCountedThreadSafe<winbase::RefCountedData<T> >;
  ~RefCountedData() = default;
};

}  // namespace winbase

#endif  // WINLIB_WINBASE_MEMORY_REF_COUNTED_H_