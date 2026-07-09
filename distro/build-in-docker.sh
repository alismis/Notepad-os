#!/usr/bin/env bash
#
# Build the NotepadOS live ISO inside a Debian bookworm container.
#
# This keeps the build reproducible and independent of the host
# distribution. The resulting ISO is written to ./out/NotepadOS-live.iso
#
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
OUT="${HERE}/out"
mkdir -p "${OUT}"

docker run --rm --privileged \
	-v "${HERE}:/config:ro" \
	-v "${OUT}:/out" \
	debian:bookworm \
	/bin/bash -euo pipefail -c '
		export DEBIAN_FRONTEND=noninteractive
		apt-get update
		apt-get install -y live-build ca-certificates
		mkdir -p /build
		cp -a /config/auto /build/auto
		cp -a /config/config /build/config
		cd /build
		lb config
		lb build
		cp -v live-image-*.iso /out/NotepadOS-live.iso
	'
echo "ISO built at: ${OUT}/NotepadOS-live.iso"
