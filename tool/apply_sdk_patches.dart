// Apply tencent_cloud_chat_sdk patch series to a vendored SDK directory.
// Run from tim2tox repo root: dart run tool/apply_sdk_patches.dart --sdk-dir=<path>
// Or from any dir: dart run /path/to/tim2tox/tool/apply_sdk_patches.dart --sdk-dir=<path> (tim2tox root = dir containing tool/ and patches/)
import 'dart:convert';
import 'dart:io';

void main(List<String> args) async {
  String? sdkDir;
  for (final a in args) {
    if (a.startsWith('--sdk-dir=')) {
      sdkDir = a.substring('--sdk-dir='.length);
      break;
    }
  }
  if (sdkDir == null || sdkDir.isEmpty) {
    stderr.writeln('Usage: dart run tool/apply_sdk_patches.dart --sdk-dir=<path>');
    exit(1);
  }
  final sdk = Directory(sdkDir);
  if (!sdk.existsSync()) {
    stderr.writeln('apply_sdk_patches: SDK dir not found: $sdkDir');
    exit(1);
  }

  final tim2toxRoot = _findTim2toxRoot();
  final lockFile = File('$tim2toxRoot/tool/tencent_cloud_chat_sdk.lock.json');
  if (!lockFile.existsSync()) {
    stderr.writeln('apply_sdk_patches: lock file not found: $lockFile');
    exit(1);
  }
  final lock = jsonDecode(lockFile.readAsStringSync()) as Map<String, dynamic>;
  final version = lock['version'] as String? ?? '';
  if (version.isEmpty) {
    stderr.writeln('apply_sdk_patches: lock file must have version');
    exit(1);
  }

  final patchesDir = Directory('$tim2toxRoot/patches/tencent_cloud_chat_sdk/$version');
  final seriesFile = File('${patchesDir.path}/series');
  if (!seriesFile.existsSync()) {
    stdout.writeln('No series file at ${patchesDir.path}/series; skipping patches.');
    exit(0);
  }
  final lines = seriesFile.readAsLinesSync().map((s) => s.trim()).where((s) => s.isNotEmpty && !s.startsWith('#')).toList();
  if (lines.isEmpty) {
    exit(0);
  }

  for (final name in lines) {
    final patchFile = File('${patchesDir.path}/$name');
    if (!patchFile.existsSync()) {
      stderr.writeln('apply_sdk_patches: patch file missing: ${patchesDir.path}/$name');
      exit(1);
    }
    stdout.writeln('Applying SDK patch: $name');
    int c = await _run(sdk.path, 'git', ['apply', '--check', '-p1', patchFile.path]);
    if (c != 0) {
      stderr.writeln('apply_sdk_patches: patch would not apply: $name');
      exit(1);
    }
    c = await _run(sdk.path, 'git', ['apply', '-p1', patchFile.path]);
    if (c != 0) {
      stderr.writeln('apply_sdk_patches: failed to apply patch: $name');
      exit(1);
    }
  }
  stdout.writeln('All patches applied.');
}

String _findTim2toxRoot() {
  var dir = Directory.current;
  while (true) {
    if (File('${dir.path}/tool/tencent_cloud_chat_sdk.lock.json').existsSync() && Directory('${dir.path}/patches').existsSync()) {
      return dir.path;
    }
    final parent = dir.parent;
    if (parent.path == dir.path) {
      stderr.writeln('apply_sdk_patches: run from tim2tox repo root or pass cwd so tool/ and patches/ are found');
      exit(1);
    }
    dir = parent;
  }
}

Future<int> _run(String cwd, String executable, List<String> args) async {
  final r = await Process.run(executable, args, workingDirectory: cwd, runInShell: false);
  if (r.stdout.toString().trim().isNotEmpty) stdout.write(r.stdout);
  if (r.stderr.toString().trim().isNotEmpty) stderr.write(r.stderr);
  return r.exitCode;
}
