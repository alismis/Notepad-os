# NotepadOS (Debian-based Linux distribution)

A **Debian-based live Linux distribution** with the **LXQt** desktop
environment, built with Debian's official [`live-build`](https://live-team.pages.debian.net/live-manual/)
toolchain. The output is a bootable `.iso` you can write to a USB stick or
run in a virtual machine.

* Base: **Debian 12 (bookworm)**, `amd64`
* Desktop: **LXQt** (lightweight Qt desktop) with the **SDDM** login manager
* Type: **live ISO** (boots straight into a usable desktop, no install required)

## What's in this directory

```
distro/
├── auto/
│   ├── config          # live-build configuration (distro, mirrors, ISO metadata)
│   ├── build           # `lb build` wrapper
│   └── clean           # `lb clean` wrapper
├── config/
│   └── package-lists/
│       ├── desktop.list.chroot   # LXQt + apps
│       └── live.list.chroot      # kernel, live tooling, base utils
├── build-in-docker.sh  # one-command reproducible build inside a Debian container
└── README.md
```

Everything needed to reproduce the distribution from source lives here — this
directory **is** the source code of the distro.

## Building the ISO

### Option A — reproducible build in Docker (recommended)

Works on any host with Docker, regardless of the host distribution:

```bash
cd distro
./build-in-docker.sh
# -> out/NotepadOS-live.iso
```

### Option B — build directly on a Debian host

Requires a Debian (bookworm or newer) machine with `live-build` installed:

```bash
sudo apt-get update
sudo apt-get install -y live-build
cd distro
sudo lb config      # reads auto/config
sudo lb build       # produces live-image-amd64.hybrid.iso
```

## Downloading / getting the source code

* **The distro's build source** is this Git repository — clone it or download a
  ZIP from GitHub:

  ```bash
  git clone https://github.com/alismis/Notepad-os.git
  ```

  or grab the "Download ZIP" button on the repository page.

* **Every upstream package's source** is downloadable from Debian. From a
  running NotepadOS system (or any Debian box) you can fetch the exact sources:

  ```bash
  # enable deb-src, then:
  apt-get source <package-name>
  ```

* **A source ISO** (all source packages bundled) can be produced by adding
  `--source true --source-images iso` to `auto/config` and rebuilding. It is
  disabled by default because it roughly doubles build time and size.

## Running / trying the ISO

Run it in QEMU without touching your machine:

```bash
qemu-system-x86_64 -m 2048 -smp 2 -cdrom out/NotepadOS-live.iso -boot d
```

Or write it to a USB stick (replace `/dev/sdX` with your device):

```bash
sudo dd if=out/NotepadOS-live.iso of=/dev/sdX bs=4M status=progress oflag=sync
```

The live session logs in automatically to the LXQt desktop.
