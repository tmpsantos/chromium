# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Autocompletion config for YouCompleteMe in Chromium.
#
# USAGE:
#
#   1. Install YCM [https://github.com/Valloric/YouCompleteMe]
#          (Googlers should check out [go/ycm])
#
#   2. Point to this config file in your .vimrc:
#          let g:ycm_global_ycm_extra_conf =
#              '<chrome_depot>/src/tools/vim/chromium.ycm_extra_conf.py'
#
#   3. Profit
#
#
# Usage notes:
#
#   * You must use ninja & clang to build Chromium.
#
#   * You must have run gyp_chromium and built Chromium recently.
#
#
# Hacking notes:
#
#   * The purpose of this script is to construct an accurate enough command line
#     for YCM to pass to clang so it can build and extract the symbols.
#
#   * Right now, we only pull the -I and -D flags. That seems to be sufficient
#     for everything I've used it for.
#
#   * That whole ninja & clang thing? We could support other configs if someone
#     were willing to write the correct commands and a parser.
#
#   * This has only been tested on gPrecise.


import os
import os.path
import subprocess


# Flags from YCM's default config.
flags = [
'-DUSE_CLANG_COMPLETER',
'-std=c++11',
'-x',
'c++',
]


def PathExists(*args):
  return os.path.exists(os.path.join(*args))


def FindChromeSrcFromFilename(filename):
  """Searches for the root of the Chromium checkout.

  Simply checks parent directories until it finds .gclient and src/.

  Args:
    filename: (String) Path to source file being edited.

  Returns:
    (String) Path of 'src/', or None if unable to find.
  """
  curdir = os.path.normpath(os.path.dirname(filename))
  while not (PathExists(curdir, 'src') and PathExists(curdir, 'src', 'DEPS')
             and (PathExists(curdir, '.gclient')
                  or PathExists(curdir, 'src', '.git'))):
    nextdir = os.path.normpath(os.path.join(curdir, '..'))
    if nextdir == curdir:
      return None
    curdir = nextdir
  return os.path.join(curdir, 'src')


# Largely copied from ninja-build.vim (guess_configuration)
def GetNinjaOutputDirectory(chrome_root):
  """Returns <chrome_root>/<output_dir>/(Release|Debug).

  The configuration chosen is the one most recently generated/built. Detects
  a custom output_dir specified by GYP_GENERATOR_FLAGS."""

  output_dir = 'out'
  generator_flags = os.getenv('GYP_GENERATOR_FLAGS', '').split(' ')
  for flag in generator_flags:
    name_value = flag.split('=', 1)
    if len(name_value) == 2 and name_value[0] == 'output_dir':
      output_dir = name_value[1]

  root = os.path.join(chrome_root, output_dir)
  debug_path = os.path.join(root, 'Debug')
  release_path = os.path.join(root, 'Release')

  def is_release_15s_newer(test_path):
    try:
      debug_mtime = os.path.getmtime(os.path.join(debug_path, test_path))
    except os.error:
      debug_mtime = 0
    try:
      rel_mtime = os.path.getmtime(os.path.join(release_path, test_path))
    except os.error:
      rel_mtime = 0
    return rel_mtime - debug_mtime >= 15

  if is_release_15s_newer('build.ninja') or is_release_15s_newer('protoc'):
    return release_path
  return debug_path


def GetClangCommandFromNinjaForFilename(chrome_root, filename):
  """Returns the command line to build |filename|.

  Asks ninja how it would build the source file. If the specified file is a
  header, tries to find its companion source file first.

  Args:
    chrome_root: (String) Path to src/.
    filename: (String) Path to source file being edited.

  Returns:
    (List of Strings) Command line arguments for clang.
  """
  if not chrome_root:
    return []

  # Generally, everyone benefits from including Chromium's src/, because all of
  # Chromium's includes are relative to that.
  chrome_flags = ['-I' + os.path.join(chrome_root)]

  # Version of Clang used to compile Chromium can be newer then version of
  # libclang that YCM uses for completion. So it's possible that YCM's libclang
  # doesn't know about some used warning options, which causes compilation
  # warnings (and errors, because of '-Werror');
  chrome_flags.append('-Wno-unknown-warning-option')

  # Default file to get a reasonable approximation of the flags for a Blink
  # file.
  blink_root = os.path.join(chrome_root, 'third_party', 'WebKit')
  default_blink_file = os.path.join(blink_root, 'Source', 'core', 'Init.cpp')

  # Header files can't be built. Instead, try to match a header file to its
  # corresponding source file.
  if filename.endswith('.h'):
    # Add config.h to Blink headers, which won't have it by default.
    if filename.startswith(blink_root):
      chrome_flags.append('-include')
      chrome_flags.append(os.path.join(blink_root, 'Source', 'config.h'))

    alternates = ['.cc', '.cpp']
    for alt_extension in alternates:
      alt_name = filename[:-2] + alt_extension
      if os.path.exists(alt_name):
        filename = alt_name
        break
    else:
      if filename.startswith(blink_root):
        # If this is a Blink file, we can at least try to get a reasonable
        # approximation.
        filename = default_blink_file
      else:
        # If this is a standalone .h file with no source, the best we can do is
        # try to use the default flags.
        return chrome_flags

  out_dir = GetNinjaOutputDirectory(chrome_root)

  # Ninja needs the path to the source file relative to the output build
  # directory.
  rel_filename = os.path.relpath(os.path.realpath(filename),
                                 os.path.realpath(out_dir))

  # Ask ninja how it would build our source file.
  p = subprocess.Popen(['ninja', '-v', '-C', out_dir, '-t',
                        'commands', rel_filename + '^'],
                       stdout=subprocess.PIPE)
  stdout, stderr = p.communicate()
  if p.returncode:
    return chrome_flags

  # Ninja might execute several commands to build something. We want the last
  # clang command.
  clang_line = None
  for line in reversed(stdout.split('\n')):
    if 'clang' in line:
      clang_line = line
      break
  else:
    return chrome_flags

  # Parse flags that are important for YCM's purposes.
  for flag in clang_line.split(' '):
    if flag.startswith('-I'):
      # Relative paths need to be resolved, because they're relative to the
      # output dir, not the source.
      if flag[2] == '/':
        chrome_flags.append(flag)
      else:
        abs_path = os.path.normpath(os.path.join(out_dir, flag[2:]))
        chrome_flags.append('-I' + abs_path)
    elif flag.startswith('-std'):
      chrome_flags.append(flag)
    elif flag.startswith('-') and flag[1] in 'DWFfmO':
      if flag == '-Wno-deprecated-register' or flag == '-Wno-header-guard':
        # These flags causes libclang (3.3) to crash. Remove it until things
        # are fixed.
        continue
      chrome_flags.append(flag)

  return chrome_flags


def FlagsForFile(filename):
  """This is the main entry point for YCM. Its interface is fixed.

  Args:
    filename: (String) Path to source file being edited.

  Returns:
    (Dictionary)
      'flags': (List of Strings) Command line flags.
      'do_cache': (Boolean) True if the result should be cached.
  """
  chrome_root = FindChromeSrcFromFilename(filename)
  chrome_flags = GetClangCommandFromNinjaForFilename(chrome_root,
                                                     filename)
  final_flags = flags + chrome_flags

  return {
    'flags': final_flags,
    'do_cache': True
  }
