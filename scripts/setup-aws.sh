#!/bin/bash
# setup_aws.sh - AWS instance setup for Toop development
# Run once on a fresh Ubuntu 22.04 instance with a Tesla T4 GPU
# Usage: chmod +x setup_aws.sh && ./setup_aws.sh

set -e

# ---------------------------------------------------------------------------
# Pre-flight checks
# ---------------------------------------------------------------------------
echo "[INFO] Checking Ubuntu version..."
. /etc/os-release
if [[ "$ID" != "ubuntu" ]]; then
    echo "[ERROR] This script is intended for Ubuntu only."
    exit 1
fi
echo "[INFO] Running on Ubuntu $VERSION_ID"

# ---------------------------------------------------------------------------
# System packages
# ---------------------------------------------------------------------------
echo "[INFO] Updating system packages..."
sudo apt-get update
sudo apt-get upgrade -y

echo "[INFO] Installing build tools and utilities..."
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    curl \
    vim \
    tmux \
    python3 \
    python3-pip \
    python3-venv \
    ca-certificates

# ---------------------------------------------------------------------------
# Docker (official repo - newer than Ubuntu default)
# ---------------------------------------------------------------------------
echo "[INFO] Installing Docker..."
sudo install -m 0755 -d /etc/apt/keyrings
sudo curl -fsSL https://download.docker.com/linux/ubuntu/gpg \
    -o /etc/apt/keyrings/docker.asc
sudo chmod a+r /etc/apt/keyrings/docker.asc

echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] \
https://download.docker.com/linux/ubuntu \
$(. /etc/os-release && echo "$VERSION_CODENAME") stable" | \
sudo tee /etc/apt/sources.list.d/docker.list > /dev/null

sudo apt-get update
sudo apt-get install -y \
    docker-ce \
    docker-ce-cli \
    containerd.io \
    docker-compose-plugin \
    docker-buildx-plugin

sudo usermod -aG docker $USER

# ---------------------------------------------------------------------------
# NVIDIA Container Toolkit (allows Docker containers to access the GPU)
# ---------------------------------------------------------------------------
echo "[INFO] Installing NVIDIA Container Toolkit..."
curl -fsSL https://nvidia.github.io/libnvidia-container/gpgkey | \
    sudo gpg --dearmor -o /usr/share/keyrings/nvidia-container-toolkit-keyring.gpg

curl -s -L https://nvidia.github.io/libnvidia-container/stable/deb/nvidia-container-toolkit.list | \
    sed 's#deb https://#deb [signed-by=/usr/share/keyrings/nvidia-container-toolkit-keyring.gpg] https://#g' | \
    sudo tee /etc/apt/sources.list.d/nvidia-container-toolkit.list

sudo apt-get update
sudo apt-get install -y nvidia-container-toolkit
sudo nvidia-ctk runtime configure --runtime=docker
sudo systemctl restart docker

# ---------------------------------------------------------------------------
# Python deps for bench scripts (host-side, outside Docker)
# ---------------------------------------------------------------------------
echo "[INFO] Installing Python bench dependencies..."
pip3 install -r Toop_Bench/requirements.txt

# ---------------------------------------------------------------------------
# SSH key for GitHub (hint only)
# ---------------------------------------------------------------------------
echo ""
echo "[INFO] SSH key setup for GitHub:"
echo "  1. Generate a key:  ssh-keygen -t ed25519 -C 'aws-toop'"
echo "  2. Copy public key: cat ~/.ssh/id_ed25519.pub"
echo "  3. Add to GitHub:   Settings -> SSH and GPG keys -> New SSH key"
echo "  4. Test:            ssh -T git@github.com"

# ---------------------------------------------------------------------------
# perf - allow performance counters inside Docker containers
# ---------------------------------------------------------------------------
echo "[INFO] Configuring perf event paranoid level..."
echo 'kernel.perf_event_paranoid=-1' | sudo tee /etc/sysctl.d/99-perf.conf
sudo sysctl -w kernel.perf_event_paranoid=-1

# ---------------------------------------------------------------------------
# Verification
# ---------------------------------------------------------------------------
echo ""
echo "[INFO] Verifying installations..."
echo -n "  Docker:                   "; docker --version
echo -n "  Docker Compose:           "; docker compose version
echo -n "  Python3:                  "; python3 --version
echo -n "  cmake:                    "; cmake --version | head -1
echo -n "  NVIDIA Container Toolkit: "; nvidia-ctk --version

echo ""
echo "[INFO] Setup complete!"
echo "[WARN] Log out and back in for docker group changes to take effect."
echo ""
echo "[INFO] Then build and run Toop:"
echo "  git clone git@github.com:<your-user>/Toop.git"
echo "  cd Toop"
echo "  docker compose build"
echo "  docker compose up toop"
echo ""
echo "[INFO] To run bench scripts inside the container:"
echo "  docker compose run --profile bench bench Toop_Bench/scripts/run_ncu_sweep.py"
echo ""
echo "[INFO] To open a shell inside the container:"
echo "  docker compose run --profile shell shell"