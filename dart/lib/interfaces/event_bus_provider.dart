import 'event_bus.dart';

/// Provider interface for event bus access
/// 
/// This allows the SDK Platform to access the event bus without
/// directly depending on client-specific implementations.
abstract class EventBusProvider {
  /// Get the event bus instance
  EventBus get eventBus;
}

