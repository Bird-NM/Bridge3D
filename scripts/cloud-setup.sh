#!/usr/bin/env bash
set -euo pipefail
# This task is isolated. Reuse /workspace/Bridge3D without creating a worktree.
cd /workspace/Bridge3D
mkdir -p /workspace/tooling/apt/lists/partial /workspace/tooling/apt/cache/archives/partial /workspace/tooling/sysroot
if ! test -x /workspace/tooling/python/cmake/data/bin/cmake; then
  python3 -m pip install --target /workspace/tooling/python cmake==4.4.5 ninja==1.13.2
fi
printf '%s\n' 'deb https://deb.debian.org/debian trixie main' > /workspace/tooling/sources.list
apt_options=(-o Dir::Etc::sourcelist=/workspace/tooling/sources.list -o Dir::Etc::sourceparts=- -o Dir::State::lists=/workspace/tooling/apt/lists -o Dir::Cache=/workspace/tooling/apt/cache -o Debug::NoLocking=1)
apt-get "${apt_options[@]}" update
apt-get "${apt_options[@]}" --download-only -y install libgl-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libx11-dev xvfb xauth
for package in /workspace/tooling/apt/cache/archives/*.deb; do
  dpkg-deb -x "$package" /workspace/tooling/sysroot
done
python3 - <<'PY'
from pathlib import Path
root = Path('/workspace/tooling/sysroot/usr/lib/x86_64-linux-gnu')
for library in root.glob('*.so'):
    if library.is_symlink() and not library.exists():
        target = Path('/usr/lib/x86_64-linux-gnu') / library.readlink().name
        if target.exists():
            library.unlink()
            library.symlink_to(target)
PY
cmake_bin=/workspace/tooling/python/cmake/data/bin
"$cmake_bin/cmake" -S . -B build -DCMAKE_PREFIX_PATH=/workspace/tooling/sysroot/usr -DCMAKE_INCLUDE_PATH=/workspace/tooling/sysroot/usr/include -DCMAKE_LIBRARY_PATH=/workspace/tooling/sysroot/usr/lib/x86_64-linux-gnu -DCMAKE_BUILD_TYPE=Release
CPATH=/workspace/tooling/sysroot/usr/include "$cmake_bin/cmake" --build build -j 4
"$cmake_bin/ctest" --test-dir build --output-on-failure
