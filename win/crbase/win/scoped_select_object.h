// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MINI_CHROMIUM_CRBASE_WIN_SCOPED_SELECT_OBJECT_H_
#define MINI_CHROMIUM_CRBASE_WIN_SCOPED_SELECT_OBJECT_H_

#include <windows.h>

#include "crbase/logging.h"
#include "crbase/macros.h"

namespace crbase {
namespace win {

// Helper class for deselecting object from DC.
class ScopedSelectObject {
 public:
  ScopedSelectObject(HDC hdc, HGDIOBJ object)
      : hdc_(hdc),
        oldobj_(SelectObject(hdc, object)) {
    CR_DCHECK(hdc_);
    CR_DCHECK(object);
    CR_DCHECK(oldobj_ != NULL && oldobj_ != HGDI_ERROR);
  }

  ~ScopedSelectObject() {
    HGDIOBJ object = SelectObject(hdc_, oldobj_);
    CR_DCHECK((GetObjectType(oldobj_) != OBJ_REGION && object != NULL) ||
              (GetObjectType(oldobj_) == OBJ_REGION && object != HGDI_ERROR));
  }

 private:
  HDC hdc_;
  HGDIOBJ oldobj_;

  CR_DISALLOW_COPY_AND_ASSIGN(ScopedSelectObject);
};

}  // namespace win
}  // namespace crbase

#endif  // MINI_CHROMIUM_CRBASE_WIN_SCOPED_SELECT_OBJECT_H_