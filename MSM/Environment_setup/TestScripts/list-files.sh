#!/bin/sh
# list_sftp_files.sh - List files from an SFTP server and save to JSON
# Requires: sftp, jq (optional but recommended for proper JSON formatting)

# =============================
# Config
# =============================
SERVER="ap-southeast-1.sftpcloud.io"   # SFTP server address
USER="ofp-msm"                         # Username
PASS="root123456"                      # Password
REMOTE_DIR="/ota-test"                 # Remote directory to list
OUTPUT_FILE="/etc/available_files.json"  # Local JSON output file

# =============================
# Get file list from SFTP
# =============================
echo "Fetching file list from $SERVER:$REMOTE_DIR..."

# Create a temporary file for sftp batch commands
TMP_BATCH=$(mktemp)
echo "ls -1 $REMOTE_DIR" > "$TMP_BATCH"

# Use sftp with password authentication through sshpass (if password login)
FILE_LIST=$(sshpass -p "$PASS" sftp -oBatchMode=no -oStrictHostKeyChecking=no "$USER@$SERVER" < "$TMP_BATCH" 2>/dev/null)
rm -f "$TMP_BATCH"

if [ -z "$FILE_LIST" ]; then
    echo "No files found or unable to connect."
    exit 1
fi

# =============================
# Convert to JSON
# =============================
# Clean output:
#  - remove "sftp>" prompts
#  - strip directories
#  - remove .zip extension
#  - build JSON under key "versions"
JSON_ARRAY=$(printf '%s\n' "$FILE_LIST" \
    | grep -v '^sftp>' \
    | sed 's#.*/##' \
    | sed 's/\.zip$//' \
    | awk 'BEGIN{print "{\"versions\": ["}{print "  \""$0"\","}END{print "]}"}' \
    | sed '$s/,$//')

# Save to file
echo "$JSON_ARRAY" > "$OUTPUT_FILE"

echo "File list saved to $OUTPUT_FILE"
