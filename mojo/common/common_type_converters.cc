// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/common/common_type_converters.h"

#include <string>

#include "base/strings/utf_string_conversions.h"
#include "url/gurl.h"

namespace mojo {

// static
String TypeConverter<String, base::StringPiece>::Convert(
    const base::StringPiece& input) {
  if (input.empty()) {
    char c = 0;
    return String(&c, 0);
  }
  return String(input.data(), input.size());
}
// static
base::StringPiece TypeConverter<base::StringPiece, String>::Convert(
    const String& input) {
  return input.get();
}

// static
String TypeConverter<String, base::string16>::Convert(
    const base::string16& input) {
  return TypeConverter<String, base::StringPiece>::Convert(
      base::UTF16ToUTF8(input));
}
// static
base::string16 TypeConverter<base::string16, String>::Convert(
    const String& input) {
  return base::UTF8ToUTF16(input.To<base::StringPiece>());
}

String TypeConverter<String, GURL>::Convert(const GURL& input) {
  return String(input.spec());
}

GURL TypeConverter<GURL, String>::Convert(const String& input) {
  return GURL(input.get());
}

}  // namespace mojo
