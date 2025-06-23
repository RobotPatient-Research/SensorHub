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

# Install the ARM toolchain manually
# Example: ARM GNU Toolchain 12.3 rel1
ENV ARM_URL=https://developer.arm.com/-/media/Files/downloads/gnu/14.2.rel1/binrel/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi.tar.xz
ENV ARM_DIR=/opt/arm-toolchain

RUN curl -L $ARM_URL -o /tmp/arm-toolchain.tar.xz && \
    tar -xJf /tmp/arm-toolchain.tar.xz -C /opt && \
    ln -s "/opt/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi" "$ARM_DIR" && \
    rm /tmp/arm-toolchain.tar.xz


# Add to PATH
ENV PATH="$ARM_DIR/bin:${PATH}"

# Switch to dev user
USER vscode
WORKDIR /workspace

# Default command (for devcontainer)
CMD ["sleep", "infinity"]
