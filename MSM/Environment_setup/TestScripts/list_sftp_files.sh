#!/bin/sh
# list_sftp_files.sh - List files from an SFTP server using config.json
# No jq required — JSON parsed with grep/sed/awk
# Requires: sftp, sshpass

CONFIG_FILE="/MSM/potential-pancake/MSM/bin/config/sftp_settings.json"
DEFAULT_OUTPUT="/etc/ota_versions.json"

# =============================
# Read config values
# =============================
if [ ! -f "$CONFIG_FILE" ]; then
    echo "Error: $CONFIG_FILE not found!"
    exit 1
fi

get_json_value() {
    key="$1"
    grep "\"$key\"" "$CONFIG_FILE" | sed -E 's/.*: *"([^"]+)".*/\1/'
}

SERVER=$(get_json_value "ip")
USER=$(get_json_value "username")
PASS=$(get_json_value "password")
REMOTE_DIR=$(get_json_value "remote_dir")
OUTPUT_FILE=$(get_json_value "output_file")

# Default fallback for output file
if [ -z "$OUTPUT_FILE" ]; then
    OUTPUT_FILE="$DEFAULT_OUTPUT"
fi

if [ -z "$SERVER" ] || [ -z "$USER" ] || [ -z "$PASS" ]; then
    echo "Error: Missing required fields (ip, username, or password) in $CONFIG_FILE"
    exit 1
fi

# =============================
# Fetch file list
# =============================
echo "Fetching file list from $SERVER:$REMOTE_DIR..."

TMP_BATCH=$(mktemp)
if [ -n "$REMOTE_DIR" ]; then
    echo "ls -1 $REMOTE_DIR" > "$TMP_BATCH"
else
    echo "ls -1" > "$TMP_BATCH"
fi

FILE_LIST=$(sshpass -p "$PASS" sftp -oBatchMode=no -oStrictHostKeyChecking=no "$USER@$SERVER" < "$TMP_BATCH" 2>/dev/null)
rm -f "$TMP_BATCH"

# Clean output (remove junk, .zip, and empty lines)
FILE_LIST=$(echo "$FILE_LIST" \
    | grep -v '^sftp>' \
    | grep -v '^Connected' \
    | grep -v '^sftp:' \
    | sed 's#.*/##' \
    | sed 's/\.zip$//' \
    | grep -v '^$')

if [ -z "$FILE_LIST" ]; then
    echo "No files found or unable to connect."
    exit 1
fi

# =============================
# Build JSON manually
# =============================
JSON="{\"versions\": ["
first=1
for file in $FILE_LIST; do
    if [ $first -eq 1 ]; then
        JSON="$JSON\"$file\""
        first=0
    else
        JSON="$JSON, \"$file\""
    fi
done
JSON="$JSON]}"

# =============================
# Save to file
# =============================
echo "$JSON" > "$OUTPUT_FILE"
echo "Available versions saved to $OUTPUT_FILE"