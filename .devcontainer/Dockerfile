FROM ghcr.io/armmbed/mbed-os-env:master-latest

ARG project="mbed-can-motor-driver-for-spirit"

# [Optional] Uncomment this section to install additional vcpkg ports.
# RUN su vscode -c "${VCPKG_ROOT}/vcpkg install <your-port-name-here>"

# [Optional] Uncomment this section to install additional packages.
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends clang-format \
    && git config --global --add safe.directory /workspaces/${project} \
    && git config --global --add safe.directory /workspaces/${project}/mbed-os \
    && git config --global --add safe.directory /workspaces/${project}/spirit
