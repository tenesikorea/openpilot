#!/usr/bin/bash

# acquire git hash from remote
cd /data/openpilot
CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
LOCAL_HASH=$(git rev-parse HEAD)
git fetch
REMOTE_HASH=$(git rev-parse --verify origin/$CURRENT_BRANCH)
echo -n "$REMOTE_HASH" > /data/params/d/GitCommitRemote