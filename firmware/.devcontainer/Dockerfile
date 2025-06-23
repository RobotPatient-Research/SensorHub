# Use UBI9-minimal as base
FROM registry.access.redhat.com/ubi9-minimal

# Install core build & runtime tools
RUN microdnf update -y && \
    microdnf install -y \
    bash \
    tar \
    git \
    vim \
    shadow-utils \
    xz \
    ninja-build \
    cmake \
    sudo \
    gdb \
    gcc \
    make \
    binutils \
    && microdnf clean all

# Add non-root user for VS Code Dev Container usage
RUN useradd -m vscode && echo "vscode ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/vscode

# Base URL and version prefix
ENV ARM_BASE_URL=https://developer.arm.com/-/media/Files/downloads/gnu/14.2.rel1/binrel
ENV ARM_VERSION=14.2.rel1
ENV ARM_DIR=/opt/arm-toolchain

# Determine ARCH and set URL accordingly
# This works for both x86_64 and arm64 (Apple M1/M2)
RUN ARCH=$(uname -m) && \
    if [ "$ARCH" = "x86_64" ]; then \
      ARM_URL="$ARM_BASE_URL/arm-gnu-toolchain-${ARM_VERSION}-x86_64-arm-none-eabi.tar.xz"; \
    elif [ "$ARCH" = "aarch64" ] || [ "$ARCH" = "arm64" ]; then \
      ARM_URL="$ARM_BASE_URL/arm-gnu-toolchain-${ARM_VERSION}-aarch64-arm-none-eabi.tar.xz"; \
    else \
      echo "Unsupported architecture: $ARCH"; exit 1; \
    fi && \
    echo "Downloading $ARM_URL" && \
    curl -L "$ARM_URL" -o /tmp/arm-toolchain.tar.xz && \
    tar -xJf /tmp/arm-toolchain.tar.xz -C /opt && \
    TOOLCHAIN_DIR=$(tar -tf /tmp/arm-toolchain.tar.xz | head -1 | cut -f1 -d"/") && \
    ln -s "/opt/$TOOLCHAIN_DIR" "$ARM_DIR" && \
    rm /tmp/arm-toolchain.tar.xz



# Add to PATH
ENV PATH="$ARM_DIR/bin:${PATH}"

# Switch to dev user
USER vscode
WORKDIR /workspace

# Default command (for devcontainer)
CMD ["sleep", "infinity"]
