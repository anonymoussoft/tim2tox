// Dart Compat Layer - Main Entry Point
// This file serves as the main entry point for the modularized dart_compat layer.
// All implementations have been split into separate module files for better maintainability.
//
// Module Structure:
// - dart_compat_internal.h: Shared declarations and forward declarations
// - dart_compat_utils.cpp: Helper functions and global variables
// - dart_compat_listeners.cpp: Listener implementations and callback registration
// - dart_compat_callbacks.cpp: Callback class implementations
// - dart_compat_sdk.cpp: SDK initialization and authentication functions
// - dart_compat_message.cpp: Message-related functions
// - dart_compat_friendship.cpp: Friendship-related functions
// - dart_compat_conversation.cpp: Conversation-related functions
// - dart_compat_group.cpp: Group-related functions
// - dart_compat_user.cpp: User-related functions
// - dart_compat_signaling.cpp: Signaling-related functions
// - dart_compat_community.cpp: Community-related functions
// - dart_compat_other.cpp: Other miscellaneous functions
//
// All function implementations are in their respective module files.
// This file only includes the internal header to ensure proper linking.

#include "dart_compat_internal.h"

// All implementations are in separate module files:
// - See individual module files for function implementations
// - All functions are exported via extern "C" in their respective modules
// - The linker will resolve all symbols from the module files
