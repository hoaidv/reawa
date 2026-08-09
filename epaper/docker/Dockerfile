# x86_64 Linux build host for reMarkable SDK cross-compilation.
# On Apple Silicon: docker compose runs with platform linux/amd64.
# Place the SDK installer under ./sdk-installer/ (gitignored) before install-sdk.sh.

FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    cmake \
    curl \
    file \
    git \
    ninja-build \
    openssh-client \
    python3 \
    rsync \
    xxd \
    && rm -rf /var/lib/apt/lists/*

ENV RM_SDK_ROOT=/opt/remarkable-sdk

WORKDIR /workspace

COPY install-sdk.sh /usr/local/bin/install-sdk.sh
RUN chmod +x /usr/local/bin/install-sdk.sh

CMD ["/bin/bash", "-lc", "echo 'Mount SDK .sh to /sdk-installer and run install-sdk.sh'; exec bash"]
