FROM nvidia/cuda:12.5.0-devel-ubuntu22.04

ENV DEBIAN_FRONTEND=noninteractive

# ---------------------------------------------------------------------------
# System packages
# ---------------------------------------------------------------------------
RUN apt-get update && apt-get install -y \
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
    libboost-dev \
    libglm-dev \
    ca-certificates \
    linux-tools-generic \
    valgrind \
    && rm -rf /var/lib/apt/lists/*

# ---------------------------------------------------------------------------
# Copy source
# ---------------------------------------------------------------------------
WORKDIR /app
COPY . .

# ---------------------------------------------------------------------------
# Build - headless, Tesla T4 (CC 7.5)
# ---------------------------------------------------------------------------
RUN mkdir -p build && cd build && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CUDA_ARCHITECTURES=75 \
        -DTOOP_HEADLESS=ON \
        -DBOOST_ROOT=/usr \
    && cmake --build . --config Release -j$(nproc)

# ---------------------------------------------------------------------------
# Python venv for bench scripts
# ---------------------------------------------------------------------------
RUN python3 -m venv /app/.venv && \
    /app/.venv/bin/pip install --upgrade pip && \
    /app/.venv/bin/pip install -r Toop_Bench/requirements.txt

ENV PATH="/app/.venv/bin:$PATH"

# ---------------------------------------------------------------------------
# Entry point - headless bench binary
# ---------------------------------------------------------------------------
ENTRYPOINT ["/app/x64/Release/Toop_Bench"]
CMD []