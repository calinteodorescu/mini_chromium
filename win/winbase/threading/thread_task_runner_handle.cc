// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "winbase\threading\thread_task_runner_handle.h"

#include <utility>

#include "winbase\functional\bind.h"
#include "winbase\lazy_instance.h"
#include "winbase\logging.h"
#include "winbase\memory\ptr_util.h"
#include "winbase\run_loop.h"
#include "winbase\threading\sequenced_task_runner_handle.h"
#include "winbase\threading\thread_local.h"

namespace winbase {

namespace {

winbase::LazyInstance<
    winbase::ThreadLocalPointer<ThreadTaskRunnerHandle>>::Leaky
        thread_task_runner_tls = WINBASE_LAZY_INSTANCE_INITIALIZER;

}  // namespace

// static
scoped_refptr<SingleThreadTaskRunner> ThreadTaskRunnerHandle::Get() {
  ThreadTaskRunnerHandle* current = thread_task_runner_tls.Pointer()->Get();
  WINBASE_CHECK(current)
      << "Error: This caller requires a single-threaded context "
         "(i.e. the current task needs to run from a "
         "SingleThreadTaskRunner).";
  return current->task_runner_;
}

// static
bool ThreadTaskRunnerHandle::IsSet() {
  return !!thread_task_runner_tls.Pointer()->Get();
}

// static
ScopedClosureRunner ThreadTaskRunnerHandle::OverrideForTesting(
    scoped_refptr<SingleThreadTaskRunner> overriding_task_runner) {
  // OverrideForTesting() is not compatible with a SequencedTaskRunnerHandle
  // being set (but SequencedTaskRunnerHandle::IsSet() includes
  // ThreadTaskRunnerHandle::IsSet() so that's discounted as the only valid
  // excuse for it to be true). Sadly this means that tests that merely need a
  // SequencedTaskRunnerHandle on their main thread can be forced to use a
  // ThreadTaskRunnerHandle if they're also using test task runners (that
  // OverrideForTesting() when running their tasks from said main thread). To
  // solve this: sequence_task_runner_handle.cc and thread_task_runner_handle.cc
  // would have to be merged into a single impl file and share TLS state. This
  // was deemed unecessary for now as most tests should use higher level
  // constructs and not have to instantiate task runner handles on their own.
  WINBASE_DCHECK(!SequencedTaskRunnerHandle::IsSet() || IsSet());

  if (!IsSet()) {
    auto top_level_ttrh = std::make_unique<ThreadTaskRunnerHandle>(
        std::move(overriding_task_runner));
    return ScopedClosureRunner(winbase::BindOnce(
        [](std::unique_ptr<ThreadTaskRunnerHandle> ttrh_to_release) {},
        std::move(top_level_ttrh)));
  }

  ThreadTaskRunnerHandle* ttrh = thread_task_runner_tls.Pointer()->Get();
  // Swap the two (and below bind |overriding_task_runner|, which is now the
  // previous one, as the |task_runner_to_restore|).
  ttrh->task_runner_.swap(overriding_task_runner);

  auto no_running_during_override =
      std::make_unique<RunLoop::ScopedDisallowRunningForTesting>();

  return ScopedClosureRunner(winbase::BindOnce(
      [](scoped_refptr<SingleThreadTaskRunner> task_runner_to_restore,
         SingleThreadTaskRunner* expected_task_runner_before_restore,
         std::unique_ptr<RunLoop::ScopedDisallowRunningForTesting>
             no_running_during_override) {
        ThreadTaskRunnerHandle* ttrh = thread_task_runner_tls.Pointer()->Get();

        WINBASE_DCHECK_EQ(expected_task_runner_before_restore,
                          ttrh->task_runner_.get())
            << "Nested overrides must expire their ScopedClosureRunners "
               "in LIFO order.";

        ttrh->task_runner_.swap(task_runner_to_restore);
      },
      std::move(overriding_task_runner),
      winbase::Unretained(ttrh->task_runner_.get()),
      std::move(no_running_during_override)));
}

ThreadTaskRunnerHandle::ThreadTaskRunnerHandle(
    scoped_refptr<SingleThreadTaskRunner> task_runner)
    : task_runner_(std::move(task_runner)) {
  WINBASE_DCHECK(task_runner_->BelongsToCurrentThread());
  // No SequencedTaskRunnerHandle (which includes ThreadTaskRunnerHandles)
  // should already be set for this thread.
  WINBASE_DCHECK(!SequencedTaskRunnerHandle::IsSet());
  thread_task_runner_tls.Pointer()->Set(this);
}

ThreadTaskRunnerHandle::~ThreadTaskRunnerHandle() {
  WINBASE_DCHECK(task_runner_->BelongsToCurrentThread());
  WINBASE_DCHECK_EQ(thread_task_runner_tls.Pointer()->Get(), this);
  thread_task_runner_tls.Pointer()->Set(nullptr);
}

}  // namespace winbase