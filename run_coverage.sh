#!/usr/bin/env bash
set -e

# Build with coverage flags
rm -rf build-coverage
cmake -B build-coverage -G Ninja -DNIX_BUILD=ON \
    -DCMAKE_CXX_FLAGS='--coverage -fprofile-update=atomic' \
    -DCMAKE_EXE_LINKER_FLAGS='--coverage' \
    -DCMAKE_SHARED_LINKER_FLAGS='--coverage'
cmake --build build-coverage

# Run tests
cd build-coverage
ctest --output-on-failure
cd ..

# Generate coverage report excluding generated files
nix run nixpkgs#gcovr -- -r . --txt \
    -e '.*/autogen/.*' \
    -e '.*/build.*/.*' \
    -e '.*qrc_.*' \
    -e '.*moc_.*' \
    -e '.*kiriflux\.h$'
