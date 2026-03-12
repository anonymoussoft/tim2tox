/// Mock path_provider for testing
/// 
/// This provides mock implementations of path_provider functions
/// to avoid plugin dependency in test environment

import 'dart:io';
import 'package:path/path.dart' as path;

/// Mock getApplicationDocumentsDirectory
Future<Directory> getApplicationDocumentsDirectory() async {
  final testDir = path.join(Directory.systemTemp.path, 'tim2tox_tests', 'app_documents');
  final dir = Directory(testDir);
  if (!await dir.exists()) {
    await dir.create(recursive: true);
  }
  return dir;
}

/// Mock getApplicationSupportDirectory
Future<Directory> getApplicationSupportDirectory() async {
  final testDir = path.join(Directory.systemTemp.path, 'tim2tox_tests', 'app_support');
  final dir = Directory(testDir);
  if (!await dir.exists()) {
    await dir.create(recursive: true);
  }
  return dir;
}

/// Mock getApplicationCacheDirectory
Future<Directory> getApplicationCacheDirectory() async {
  final testDir = path.join(Directory.systemTemp.path, 'tim2tox_tests', 'app_cache');
  final dir = Directory(testDir);
  if (!await dir.exists()) {
    await dir.create(recursive: true);
  }
  return dir;
}

/// Mock getTemporaryDirectory
Future<Directory> getTemporaryDirectory() async {
  final testDir = path.join(Directory.systemTemp.path, 'tim2tox_tests', 'temp');
  final dir = Directory(testDir);
  if (!await dir.exists()) {
    await dir.create(recursive: true);
  }
  return dir;
}
