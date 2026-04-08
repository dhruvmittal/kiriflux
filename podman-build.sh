#!/bin/bash
set -e

# 1. Define the output folder on your host
OUTPUT_DIR="$(pwd)/out"
mkdir -p "$OUTPUT_DIR"

echo "🚀 Starting Nix build inside Podman..."

# 2. Run the Nix container
# We use --rm to clean up the container after the build
# We mount the current dir to /src and /nix to a temporary volume for speed
podman run --rm \
  -e HOST_UID=$(id -u) \
  -e HOST_GID=$(id -g) \
  -v "$(pwd):/src:Z" \
  -w /src \
  docker.io/nixos/nix \
  sh -c "
  # Mark the directory as safe for Git
    git config --global --add safe.directory /src

    # Enable flakes and nix-command inside the container
    mkdir -p ~/.config/nix
    echo 'experimental-features = nix-command flakes' > ~/.config/nix/nix.conf
    
    # Copy result and fix ownership
    OUT_PATH=\$(nix build .#flatpak --print-out-paths)
    cp -RL \$OUT_PATH /src/out/
    chown -R \$HOST_UID:\$HOST_GID /src/out/
  "

echo "✅ Build complete! Your Flatpak is in: $OUTPUT_DIR"
