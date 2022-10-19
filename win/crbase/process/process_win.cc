// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crbase/process/process.h"

#include "crbase/logging.h"
#include "crbase/memory/scoped_ptr.h"
#include "crbase/numerics/safe_conversions.h"
#include "crbase/process/kill.h"
#include "crbase/win/windows_version.h"

namespace {

DWORD kBasicProcessAccess =
  PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION | SYNCHRONIZE;

} // namespace

namespace crbase {

Process::Process(ProcessHandle handle)
    : is_current_process_(false),
      process_(handle) {
  CR_CHECK_NE(handle, ::GetCurrentProcess());
}

Process::Process(Process&& other)
    : is_current_process_(other.is_current_process_),
      process_(other.process_.Take()) {
  other.Close();
}

Process::~Process() {
}

Process& Process::operator=(Process&& other) {
  CR_DCHECK_NE(this, &other);
  process_.Set(other.process_.Take());
  is_current_process_ = other.is_current_process_;
  other.Close();
  return *this;
}

// static
Process Process::Current() {
  Process process;
  process.is_current_process_ = true;
  return process.Pass();
}

// static
Process Process::Open(ProcessId pid) {
  return Process(::OpenProcess(kBasicProcessAccess, FALSE, pid));
}

// static
Process Process::OpenWithExtraPrivileges(ProcessId pid) {
  DWORD access = kBasicProcessAccess | PROCESS_DUP_HANDLE | PROCESS_VM_READ;
  return Process(::OpenProcess(access, FALSE, pid));
}

// static
Process Process::OpenWithAccess(ProcessId pid, DWORD desired_access) {
  return Process(::OpenProcess(desired_access, FALSE, pid));
}

// static
Process Process::DeprecatedGetProcessFromHandle(ProcessHandle handle) {
  CR_DCHECK_NE(handle, ::GetCurrentProcess());
  ProcessHandle out_handle;
  if (!::DuplicateHandle(GetCurrentProcess(), handle,
                         GetCurrentProcess(), &out_handle,
                         0, FALSE, DUPLICATE_SAME_ACCESS)) {
    return Process();
  }
  return Process(out_handle);
}

// static
bool Process::CanBackgroundProcesses() {
  return true;
}

bool Process::IsValid() const {
  return process_.IsValid() || is_current();
}

ProcessHandle Process::Handle() const {
  return is_current_process_ ? GetCurrentProcess() : process_.Get();
}

Process Process::Duplicate() const {
  if (is_current())
    return Current();

  ProcessHandle out_handle;
  if (!IsValid() || !::DuplicateHandle(GetCurrentProcess(),
                                       Handle(),
                                       GetCurrentProcess(),
                                       &out_handle,
                                       0,
                                       FALSE,
                                       DUPLICATE_SAME_ACCESS)) {
    return Process();
  }
  return Process(out_handle);
}

ProcessId Process::Pid() const {
  CR_DCHECK(IsValid());
  return GetProcId(Handle());
}

bool Process::is_current() const {
  return is_current_process_;
}

void Process::Close() {
  is_current_process_ = false;
  if (!process_.IsValid())
    return;

  process_.Close();
}

bool Process::Terminate(int exit_code, bool wait) const {
  CR_DCHECK(IsValid());
  bool result = (::TerminateProcess(Handle(), exit_code) != FALSE);
  if (result && wait) {
    // The process may not end immediately due to pending I/O
    if (::WaitForSingleObject(Handle(), 60 * 1000) != WAIT_OBJECT_0)
      CR_DPLOG(ERROR) << "Error waiting for process exit";
  } else if (!result) {
    CR_DPLOG(ERROR) << "Unable to terminate process";
  }
  return result;
}

bool Process::WaitForExit(int* exit_code) {
  return WaitForExitWithTimeout(TimeDelta::FromMilliseconds(INFINITE),
                                exit_code);
}

bool Process::WaitForExitWithTimeout(TimeDelta timeout, int* exit_code) {
  // Limit timeout to INFINITE.
  DWORD timeout_ms = saturated_cast<DWORD>(timeout.InMilliseconds());
  if (::WaitForSingleObject(Handle(), timeout_ms) != WAIT_OBJECT_0)
    return false;

  DWORD temp_code;  // Don't clobber out-parameters in case of failure.
  if (!::GetExitCodeProcess(Handle(), &temp_code))
    return false;

  if (exit_code)
    *exit_code = temp_code;
  return true;
}

bool Process::IsProcessBackgrounded() const {
  CR_DCHECK(IsValid());
  DWORD priority = GetPriority();
  if (priority == 0)
    return false;  // Failure case.
  return ((priority == BELOW_NORMAL_PRIORITY_CLASS) ||
          (priority == IDLE_PRIORITY_CLASS));
}

bool Process::SetProcessBackgrounded(bool value) {
  CR_DCHECK(IsValid());
  // Vista and above introduce a real background mode, which not only
  // sets the priority class on the threads but also on the IO generated
  // by it. Unfortunately it can only be set for the calling process.
  DWORD priority;
  if ((crbase::win::GetVersion() >= crbase::win::VERSION_VISTA) && 
      (is_current())) {
    priority = value ? PROCESS_MODE_BACKGROUND_BEGIN :
                       PROCESS_MODE_BACKGROUND_END;
  } else {
    priority = value ? IDLE_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS;
  }

  return (::SetPriorityClass(Handle(), priority) != 0);
}

int Process::GetPriority() const {
  CR_DCHECK(IsValid());
  return ::GetPriorityClass(Handle());
}

}  // namespace crbase