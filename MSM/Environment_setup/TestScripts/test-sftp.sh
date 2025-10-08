#!/bin/sh
# test_sftp.sh - Check latest OTA file on remote SFTP server

SERVER="ap-southeast-1.sftpcloud.io"
USER="ofp-msm"
PASS="root123456"
REMOTE_DIR="ota-test"

echo "$(date): Checking for new OTA..."

# Run SFTP command non-interactively using sshpass
SFTP_OUTPUT=$(sshpass -p "$PASS" sftp -oBatchMode=no -oStrictHostKeyChecking=no -b - "$USER@$SERVER" 2>/dev/null <<EOF
ls -1 $REMOTE_DIR
EOF
)

# Print what we got
echo "SFTP OUTPUT:"
echo "$SFTP_OUTPUT"

# Extract only filenames (remove directory prefix)
FILES=$(echo "$SFTP_OUTPUT" | sed 's|.*/||' | grep '^MSM_.*\.zip$')

# Find latest file (sort alphabetically and pick last)
LATEST_FILE=$(echo "$FILES" | sort -V | tail -n 1)

echo "LATEST FILE: $LATEST_FILE"

# Optional: automatically download the latest OTA
# sshpass -p "$PASS" sftp "$USER@$SERVER:$REMOTE_DIR/$LATEST_FILE" .

exit 0
