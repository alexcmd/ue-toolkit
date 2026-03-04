#!/bin/bash
# Remove ue-toolkit symlinks from .claude/skills/
set -euo pipefail

PLUGIN_DIR="$(cd "$(dirname "$0")" && pwd)"
SKILLS_DIR="${PLUGIN_DIR}/../.claude/skills"

count=0
for link in "$SKILLS_DIR"/ue-*; do
  if [ -L "$link" ]; then
    target=$(readlink "$link")
    # Only remove symlinks pointing into ue-toolkit
    if [[ "$target" == *"ue-toolkit/skills/"* ]]; then
      echo "Removing symlink: $(basename "$link")"
      rm "$link"
      count=$((count + 1))
    fi
  fi
done

echo "Removed $count skill symlinks"
