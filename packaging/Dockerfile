# SPDX-FileCopyrightText: 2026 ZhuyinFlow contributors
# SPDX-License-Identifier: MulanPSL-2.0
ARG SWIFT_IMAGE=swift:6.4-noble
FROM ${SWIFT_IMAGE}
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential cmake pkg-config libfcitx5core-dev libfcitx5config-dev \
    libfcitx5utils-dev fcitx5-modules-dev fcitx5 nlohmann-json3-dev \
    dpkg-dev patchelf desktop-file-utils xz-utils ca-certificates \
    && rm -rf /var/lib/apt/lists/*
ENV HOME=/tmp
