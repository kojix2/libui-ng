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

HELPER="${RUNNER_TEMP:-/tmp}/libui-get-window-id"
if [[ ! -x "$HELPER" ]]; then
  swiftc "${GITHUB_ACTION_PATH}/window-id.swift" -o "$HELPER"
fi
WINDOW_ID=$("$HELPER" "$APP_PID")

screencapture -x -t png -l "$WINDOW_ID" "$OUTPUT_FILE"
if [[ ! -s "$OUTPUT_FILE" ]]; then
  echo "Screenshot was not created: $OUTPUT_FILE" >&2
  exit 1
fi
