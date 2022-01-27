#!/usr/bin/env bash

set -o errexit -o noclobber -o nounset -o pipefail
shopt -s failglob inherit_errexit

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

mkdir --parents "${project_root}/build"
cd build

cmake -DCMAKE_INSTALL_PREFIX=$(mktemp --directory) ..
make
make check
make install

cd "$project_root"
dpkg_dir="$(mktemp --directory)"
touch "${dpkg_dir}/status"
dpkg-buildpackage --admindir="$dpkg_dir" --build=binary --unsigned-changes --unsigned-source
