/// Abstract interface for preferences storage
/// 
/// This allows the framework to work with different preference storage
/// implementations without being tied to a specific package.
abstract class PreferencesService {
  /// Get a string value by key
  Future<String?> getString(String key);
  
  /// Set a string value by key
  Future<void> setString(String key, String value);
  
  /// Get a boolean value by key
  Future<bool?> getBool(String key);
  
  /// Set a boolean value by key
  Future<void> setBool(String key, bool value);
  
  /// Get an integer value by key
  Future<int?> getInt(String key);
  
  /// Set an integer value by key
  Future<void> setInt(String key, int value);
  
  /// Get a list of strings by key
  Future<List<String>?> getStringList(String key);
  
  /// Set a list of strings by key
  Future<void> setStringList(String key, List<String> value);
  
  /// Remove a value by key
  Future<void> remove(String key);
  
  /// Clear all preferences
  Future<void> clear();
  
  // Helper methods for common operations
  
  /// Get a set of strings by key (convenience method)
  Future<Set<String>> getStringSet(String key) async {
    final list = await getStringList(key);
    return list?.toSet() ?? <String>{};
  }
  
  /// Set a set of strings by key (convenience method)
  Future<void> setStringSet(String key, Set<String> value) async {
    await setStringList(key, value.toList());
  }
}

