// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "crbase/win/scoped_ole_initializer.h"

#include "crbase/logging.h"

namespace crbase {
namespace win {

ScopedOleInitializer::ScopedOleInitializer()
    :
#ifndef NDEBUG
      // Using the windows API directly to avoid dependency on platform_thread.
      thread_id_(GetCurrentThreadId()),
#endif
      hr_(OleInitialize(NULL)) {
#ifndef NDEBUG
  if (hr_ == S_FALSE) {
    LOG(ERROR) << "Multiple OleInitialize() calls for thread " << thread_id_;
  } else {
    CR_DCHECK_NE(OLE_E_WRONGCOMPOBJ, hr_) << "Incompatible DLLs on machine";
    CR_DCHECK_NE(RPC_E_CHANGED_MODE, hr_) << "Invalid COM thread model change";
  }
#endif
}

ScopedOleInitializer::~ScopedOleInitializer() {
#ifndef NDEBUG
  CR_DCHECK_EQ(thread_id_, GetCurrentThreadId());
#endif
  if (SUCCEEDED(hr_))
    OleUninitialize();
}

}  // namespace win
}  // namespace crbase