/// Abstract event bus interface
/// 
/// This allows the framework to work with different event bus implementations
/// without being tied to a specific package.
abstract class EventBus {
  /// Subscribe to events of type T on a topic
  Stream<T> on<T>(String topic);
  
  /// Emit an event of type T on a topic
  void emit<T>(String topic, T event);
}

