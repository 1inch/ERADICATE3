# Installation

ERADICATE3 is a C++11 program that offloads its work to a GPU via OpenCL. To build and run it you need three things:

1. A C++ compiler and `make`
2. OpenCL headers and an import library (build time)
3. A GPU driver with OpenCL support (run time — NVIDIA, AMD and Intel drivers all include this)

Instructions for a fresh Windows machine are below, followed by shorter sections for Linux and macOS.

---

## Windows (fresh machine, via MSYS2)

Windows ships with no compiler or `make`. The easiest way to get a complete GNU toolchain is [MSYS2](https://www.msys2.org/).

### 1. Install your GPU driver

Install the normal driver for your GPU (GeForce driver, AMD Adrenalin, or Intel Arc/Graphics driver). All of them install the OpenCL runtime (`C:\Windows\System32\OpenCL.dll`). If the driver is already installed, skip this step.

### 2. Install MSYS2

1. Download the installer from https://www.msys2.org/ and run it (default location `C:\msys64` is fine).
2. When the install finishes, open the **"MSYS2 UCRT64"** shell from the Start menu. (MSYS2 installs several shell flavors — use UCRT64 for everything below.)

### 3. Install the toolchain and OpenCL development files

In the UCRT64 shell:

```bash
pacman -Syu
```

If it asks you to close the terminal, close it, reopen **MSYS2 UCRT64**, and run `pacman -Syu` once more. Then install the packages:

```bash
pacman -S --needed make git \
    mingw-w64-ucrt-x86_64-gcc \
    mingw-w64-ucrt-x86_64-opencl-headers \
    mingw-w64-ucrt-x86_64-opencl-icd
```

This gives you `g++`, `make`, `git`, the `CL/cl.h` headers, and `libOpenCL` to link against.

### 4. Clone and build

```bash
git clone <repository-url> ERADICATE3
cd ERADICATE3
make
```

(If you already have the sources on disk, e.g. under `C:\git\ERADICATE3`, just `cd /c/git/ERADICATE3` instead of cloning.)

This produces `ERADICATE2.x64.exe` in the project directory.

### 5. Run

Run it **from the project directory** — the program loads `keccak.cl` and `eradicate2.cl` from the current working directory at startup:

```bash
./ERADICATE2.x64.exe --benchmark
./ERADICATE2.x64.exe -A 0x00000000000000000000000000000000deadbeef --leading 0
```

The binary is statically linked against the GCC runtime, so you can also run it from a regular `cmd`/PowerShell window — just make sure the two `.cl` files are next to it and it's your current directory.

Run `./ERADICATE2.x64.exe --help` for all options (modes, device selection, work sizes).

### Troubleshooting (Windows)

- **`CL/cl.h: No such file or directory`** — the OpenCL packages are missing or you are in the wrong shell. Re-run the `pacman -S` command above and make sure you build from the **UCRT64** shell, not the plain "MSYS2 MSYS" one.
- **`cannot find -lOpenCL`** — the import library is missing: install it with `pacman -S mingw-w64-ucrt-x86_64-opencl-icd` (it provides `/ucrt64/lib/libOpenCL.dll.a`). Also make sure the Makefile does not link with plain `-static` — MSYS2 has no static `libOpenCL.a`, so OpenCL must be linked dynamically.
- **Program prints `Devices:` and exits immediately** — no OpenCL-capable GPU was found. Install/update your GPU vendor driver (step 1). Remote-desktop sessions and VMs often expose no GPU.
- **`error: failed to open input file` / kernel compile errors at startup** — you are not running from the directory containing `keccak.cl` and `eradicate2.cl`.

---

## Linux (Debian/Ubuntu)

```bash
sudo apt update
sudo apt install build-essential ocl-icd-opencl-dev opencl-headers
```

Also install your GPU vendor's OpenCL runtime if it isn't already present (e.g. the NVIDIA proprietary driver, or `intel-opencl-icd` / ROCm for Intel/AMD). Then:

```bash
make
./ERADICATE2.x64 --benchmark
```

## macOS

OpenCL ships with the system; you only need the Xcode command-line tools:

```bash
xcode-select --install
make
./ERADICATE2.x64 --benchmark
```

---

## Usage examples

```bash
# Benchmark, no scoring
./ERADICATE2.x64.exe --benchmark

# Leading zeros
./ERADICATE2.x64.exe -A 0x00000000000000000000000000000000deadbeef --leading 0

# Most zero characters anywhere in the address
./ERADICATE2.x64.exe -A 0x00000000000000000000000000000000deadbeef --zeros
```

See `README.md` or `--help` for the full list of scoring modes and tuning options (`-w`, `-W`, `-S`).
