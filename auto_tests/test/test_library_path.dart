/// Test library path configuration
/// 
/// This file provides a way to configure the native library path for tests
/// It should be imported before any SDK initialization in test files

import 'dart:io';
import 'package:path/path.dart' as path;

/// Get the absolute path to libtim2tox_ffi.dylib for testing
String? getTestLibraryPath() {
  if (!Platform.isMacOS) {
    return null;
  }
  
  // Try multiple possible locations
  final possiblePaths = [
    // From project root
    path.join(
      path.dirname(path.dirname(path.dirname(Platform.resolvedExecutable))),
      'tim2tox',
      'build',
      'ffi',
      'libtim2tox_ffi.dylib',
    ),
    // Absolute path
    '/Users/bin.gao/chat-uikit/tim2tox/build/ffi/libtim2tox_ffi.dylib',
    // Relative to test directory
    path.join(
      path.dirname(Platform.script.toFilePath()),
      '..',
      '..',
      '..',
      'tim2tox',
      'build',
      'ffi',
      'libtim2tox_ffi.dylib',
    ),
  ];
  
  for (final libPath in possiblePaths) {
    final file = File(libPath);
    if (file.existsSync()) {
      return libPath;
    }
  }
  
  return null;
}
