/// Mock CommonUtils for testing
/// 
/// This file provides a way to mock CommonUtils.init() to avoid path_provider dependency
/// in test environment. It should be imported before any SDK initialization.

import 'dart:io';
import 'package:path/path.dart' as path;

/// Initialize CommonUtils with test directories
/// This bypasses path_provider plugin calls
Future<void> initTestCommonUtils() async {
  final testDataDir = path.join(Directory.systemTemp.path, 'tim2tox_tests');
  final testAppDir = Directory(path.join(testDataDir, 'app'));
  final testCacheDir = Directory(path.join(testDataDir, 'cache'));
  
  // Create directories
  await testAppDir.create(recursive: true);
  await testCacheDir.create(recursive: true);
  
  // Use reflection or direct access to set private fields
  // Since we can't directly access private fields, we'll need to call init with mock functions
  // But init() will still call path_provider, so we need a different approach
  
  // Alternative: Set the directories directly if possible
  // This requires modifying CommonUtils or using a test-specific version
}
