// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "winbase\time\time_to_iso8601.h"

#include "winbase\strings\stringprintf.h"
#include "winbase\time\time.h"

namespace winbase {

std::string TimeToISO8601(const Time& t) {
  Time::Exploded exploded;
  t.UTCExplode(&exploded);
  return StringPrintf("%04d-%02d-%02dT%02d:%02d:%02d.%03dZ", exploded.year,
                      exploded.month, exploded.day_of_month, exploded.hour,
                      exploded.minute, exploded.second, exploded.millisecond);
}

}  // namespace winbase