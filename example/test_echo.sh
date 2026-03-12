#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$ROOT_DIR/build"
SERVER_BIN="$BUILD_DIR/echo_bot_server"
CLIENT_BIN="$BUILD_DIR/echo_bot_client"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${YELLOW}==> Building examples...${NC}"
mkdir -p "$BUILD_DIR"
(cd "$BUILD_DIR" && cmake .. -DCMAKE_BUILD_TYPE=Release >/dev/null && make -j$(sysctl -n hw.ncpu) >/dev/null)

if [[ ! -x "$SERVER_BIN" || ! -x "$CLIENT_BIN" ]]; then
  echo -e "${RED}Build failed: binaries not found${NC}"
  exit 1
fi

# Start server
SERVER_LOG="$BUILD_DIR/server.log"
CLIENT_LOG="$BUILD_DIR/client.log"
: > "$SERVER_LOG"
: > "$CLIENT_LOG"

echo -e "${YELLOW}==> Starting echo bot server...${NC}"
# Ensure DYLD_LIBRARY_PATH is defined under 'set -u'
export DYLD_LIBRARY_PATH="${DYLD_LIBRARY_PATH:-}"
export DYLD_LIBRARY_PATH="$BUILD_DIR/toxcore_build:$DYLD_LIBRARY_PATH"
"$SERVER_BIN" > "$SERVER_LOG" 2>&1 &
SERVER_PID=$!
trap 'kill $SERVER_PID >/dev/null 2>&1 || true' EXIT

# Wait for server to print Tox ID
TOX_ID=""
echo -e "${YELLOW}==> Waiting for server ID...${NC}"
for i in {1..60}; do
  if grep -Eq "^(Tox ID|User ID): " "$SERVER_LOG"; then
    TOX_ID=$(grep -E "^(Tox ID|User ID): " "$SERVER_LOG" | tail -n1 | awk -F': ' '{print $2}' | tr -d '\r\n ')
    break
  fi
  sleep 1
done

if [[ -z "$TOX_ID" ]]; then
  echo -e "${RED}Failed to obtain server Tox ID from logs${NC}"
  echo "Server log:" && tail -n +1 "$SERVER_LOG"
  exit 1
fi

echo -e "${GREEN}Server ID: $TOX_ID${NC}"

# Prepare client input script
TEST_MSG="hello_from_automated_test_$(date +%s)"
CLIENT_INPUT="$BUILD_DIR/client_input.txt"
cat > "$CLIENT_INPUT" <<EOF
status
add $TOX_ID
status
$TEST_MSG
status
quit
EOF

echo -e "${YELLOW}==> Running echo bot client (non-interactive)...${NC}"
"$CLIENT_BIN" --auto "$TOX_ID" "$TEST_MSG" > "$CLIENT_LOG" 2>&1 || true

# Verify echo either in client or server logs
if (grep -q "Echo received from friend" "$CLIENT_LOG" || grep -q "Message echoed back to friend" "$SERVER_LOG") \
   && (grep -q "$TEST_MSG" "$CLIENT_LOG" || grep -q "$TEST_MSG" "$SERVER_LOG"); then
  echo -e "${GREEN}Echo verification passed.${NC}"
  RESULT=0
else
  echo -e "${RED}Echo verification failed.${NC}"
  echo -e "${YELLOW}--- Server log ---${NC}" && tail -n +1 "$SERVER_LOG"
  echo -e "${YELLOW}--- Client log ---${NC}" && tail -n +1 "$CLIENT_LOG"
  RESULT=1
fi

kill $SERVER_PID >/dev/null 2>&1 || true
trap - EXIT
exit $RESULT
