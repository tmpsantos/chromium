// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_TEST_FILE_UTIL_H_
#define CHROME_TEST_TEST_FILE_UTIL_H_

// File utility functions used only by tests.

#include <string>

namespace file_util {

// Like CopyFileNoCache but recursively copies all files and subdirectories
// in the given input directory to the output directory. Any files in the
// destination that already exist will be overwritten.
//
// Returns true on success. False means there was some error copying, so the
// state of the destination is unknown.
bool CopyRecursiveDirNoCache(const std::wstring& source_dir,
                             const std::wstring& dest_dir);

}  // namespace file_util

#endif  // CHROME_TEST_TEST_FILE_UTIL_H_

