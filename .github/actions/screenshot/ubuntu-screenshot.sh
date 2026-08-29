#!/usr/bin/env bash
set -euo pipefail

APP_PATH="${1:?application path is required}"
OUTPUT_FILE="${2:?output filename is required}"

if [[ ! -x "$APP_PATH" ]]; then
  echo "Application is not executable: $APP_PATH" >&2
  exit 1
fi

"$APP_PATH" &
APP_PID=$!

cleanup() {
  kill "$APP_PID" 2>/dev/null || true
  wait "$APP_PID" 2>/dev/null || true
}
trap cleanup EXIT

WINDOW_ID=""
for attempt in $(seq 1 30); do
  if ! kill -0 "$APP_PID" 2>/dev/null; then
    echo "Application exited before its window appeared: $APP_PATH" >&2
    exit 1
  fi

  WINDOW_ID=$(wmctrl -lp 2>/dev/null | awk -v pid="$APP_PID" '$3 == pid { print $1; exit }' || true)
  if [[ -n "$WINDOW_ID" ]]; then
    break
  fi
  sleep 0.5
done

if [[ -z "$WINDOW_ID" ]]; then
  echo "Could not find a window for PID $APP_PID" >&2
  exit 1
fi

wmctrl -i -a "$WINDOW_ID" || true
sleep 1
import -frame -window "$WINDOW_ID" "$OUTPUT_FILE"

if [[ ! -s "$OUTPUT_FILE" ]]; then
  echo "Screenshot was not created: $OUTPUT_FILE" >&2
  exit 1
fi

identify "$OUTPUT_FILE" || true
