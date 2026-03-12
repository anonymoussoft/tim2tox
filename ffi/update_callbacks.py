#!/usr/bin/env python3
"""
Script to update all GetCallbackUserData and BuildGlobalCallbackJson calls
to include instance_id parameter.
"""

import re
import sys

def update_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()
    
    original_content = content
    
    # Pattern 1: GetCallbackUserData("...") -> GetCallbackUserData(instance_id, "...")
    # But we need to add instance_id = GetCurrentInstanceId(); before the first use in each function
    # This is complex, so we'll do it in two passes
    
    # First, find all functions that use GetCallbackUserData
    # We'll add instance_id at the start of each callback function
    
    # Pattern for callback functions in Listener classes
    callback_pattern = r'(void\s+On\w+\([^)]*\)\s+override\s*\{)'
    
    def add_instance_id_to_callback(match):
        func_start = match.group(0)
        # Check if instance_id is already added
        if 'int64_t instance_id = GetCurrentInstanceId();' in content[match.start():match.start()+500]:
            return func_start
        return func_start + '\n        // Get instance_id for this listener\n        int64_t instance_id = GetCurrentInstanceId();'
    
    # Add instance_id to callback functions
    content = re.sub(callback_pattern, add_instance_id_to_callback, content)
    
    # Pattern 2: GetCallbackUserData("...") -> GetCallbackUserData(instance_id, "...")
    content = re.sub(
        r'GetCallbackUserData\("([^"]+)"\)',
        r'GetCallbackUserData(instance_id, "\1")',
        content
    )
    
    # Pattern 3: BuildGlobalCallbackJson(..., user_data) -> BuildGlobalCallbackJson(..., user_data, instance_id)
    # Match BuildGlobalCallbackJson calls that don't already have instance_id
    content = re.sub(
        r'BuildGlobalCallbackJson\(([^,]+,\s*[^,]+,\s*[^,)]+)\)',
        lambda m: m.group(0) if 'instance_id' in m.group(0) else m.group(0).rstrip(')') + ', instance_id)',
        content
    )
    
    # Pattern 4: StoreCallbackUserData("...", user_data) -> StoreCallbackUserData(instance_id, "...", user_data)
    # But we need instance_id before this call
    def update_store_callback(match):
        callback_name = match.group(1)
        user_data = match.group(2)
        # Check if instance_id is already defined in the function
        # For now, we'll add it before StoreCallbackUserData if not present
        return f'int64_t instance_id = GetCurrentInstanceId();\n        StoreCallbackUserData(instance_id, "{callback_name}", {user_data})'
    
    # Find StoreCallbackUserData calls in extern "C" functions
    # We need to be more careful here - only update if instance_id is not already there
    store_pattern = r'StoreCallbackUserData\("([^"]+)",\s*([^)]+)\)'
    
    def update_store_with_check(match):
        full_match = match.group(0)
        # Check if this line already has instance_id defined before it
        # Look backwards in the content to see if instance_id is defined
        start_pos = match.start()
        # Find the start of the function
        func_start = content.rfind('void ', max(0, start_pos - 200), start_pos)
        if func_start == -1:
            func_start = max(0, start_pos - 100)
        
        func_content = content[func_start:start_pos]
        if 'int64_t instance_id' in func_content or 'instance_id = GetCurrentInstanceId()' in func_content:
            # Already has instance_id, just update the call
            return f'StoreCallbackUserData(instance_id, "{match.group(1)}", {match.group(2)})'
        else:
            # Need to add instance_id
            return f'int64_t instance_id = GetCurrentInstanceId();\n        StoreCallbackUserData(instance_id, "{match.group(1)}", {match.group(2)})'
    
    content = re.sub(store_pattern, update_store_with_check, content)
    
    if content != original_content:
        with open(filepath, 'w') as f:
            f.write(content)
        print(f"Updated {filepath}")
        return True
    else:
        print(f"No changes needed in {filepath}")
        return False

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python3 update_callbacks.py <file>")
        sys.exit(1)
    
    update_file(sys.argv[1])
