# nv2stats — Build Setup Guide

## Prerequisites

This project depends on two shared libraries:

- **SQLite3**
- **zlib**

Setup differs per platform. Follow the section for your OS.

---

## Linux

### 1. Install dependencies

**Debian / Ubuntu:**
```bash
sudo apt install libsqlite3-dev zlib1g-dev
```

**Fedora:**
```bash
sudo dnf install sqlite-devel zlib-devel
```

### 2. Build

```bash
cmake -S . -B build
cmake --build build
```

The executable will be at `build/nv2stats`.

---

## Windows

### 1. Install vcpkg

If you don't have vcpkg yet:

```bash
git clone https://github.com/microsoft/vcpkg
.\vcpkg\bootstrap-vcpkg.bat
```

### 2. Install dependencies

We use the `x64-windows` triplet to get shared (`.dll`) versions of the libraries:

```bash
.\vcpkg\vcpkg install sqlite3:x64-windows zlib:x64-windows
```

### 3. Build

Pass the vcpkg toolchain file to CMake so it knows where to find the installed libraries:

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=<path_to_vcpkg_repo_root>/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

> **Note:** Replace `<path_to_vcpkg_repo_root>` with the actual path where you cloned vcpkg.

The executable and the required `.dll` files will be placed together in the build output directory automatically.

## Redacted URLs

Source code is missing URLs necessary for tool to work. They are REDACTED as to not be easily available on the web. You can obtain info on this URLs by contacting community on Discord server dedicated for this awesome platformer franchise.