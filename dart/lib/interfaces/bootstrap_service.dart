/// Abstract interface for bootstrap node configuration
/// 
/// This allows the framework to work with different bootstrap node
/// storage implementations.
abstract class BootstrapService {
  /// Get the current bootstrap node host
  Future<String?> getBootstrapHost();
  
  /// Get the current bootstrap node port
  Future<int?> getBootstrapPort();
  
  /// Get the current bootstrap node public key
  Future<String?> getBootstrapPublicKey();
  
  /// Set bootstrap node configuration
  Future<void> setBootstrapNode({
    required String host,
    required int port,
    required String publicKey,
  });
}

