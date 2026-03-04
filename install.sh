#!/bin/bash
# Install ue-toolkit skills into both project .claude/skills/ and ~/.claude/skills/
set -euo pipefail

PLUGIN_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_SKILLS_DIR="${PLUGIN_DIR}/../.claude/skills"
GLOBAL_SKILLS_DIR="$HOME/.claude/skills"
mkdir -p "$PROJECT_SKILLS_DIR" "$GLOBAL_SKILLS_DIR"

ACTIVE_SKILLS="ue-scripter ue-editor ue-coder ue-runner ue-builder ue-agent ue-material-agent ue-gas-agent ue-architect"

# Clean up stale ue-api-* symlinks from both locations
for dir in "$PROJECT_SKILLS_DIR" "$GLOBAL_SKILLS_DIR"; do
  for old_api in "$dir"/ue-api-*; do
    [ -e "$old_api" ] || [ -L "$old_api" ] || continue
    echo "Removing old API skill: $(basename "$old_api") from $dir"
    rm -rf "$old_api"
  done
done

# Symlink active skills into both project and global .claude/skills/
count=0
for name in $ACTIVE_SKILLS; do
  skill_dir="$PLUGIN_DIR/skills/$name"
  if [ -d "$skill_dir" ]; then
    ln -sfn "$skill_dir" "$PROJECT_SKILLS_DIR/$name"
    ln -sfn "$skill_dir" "$GLOBAL_SKILLS_DIR/$name"
    count=$((count + 1))
  else
    echo "Warning: skill dir not found: $name"
  fi
done

echo "Installed $count skills into project and global .claude/skills/"
