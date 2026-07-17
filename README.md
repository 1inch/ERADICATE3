# ERADICATE3

ERADICATE3 is a GPU-accelerated (OpenCL) vanity address generator for **CREATE3**
deployments. It brute-forces salts until the resulting CREATE3 address matches the
pattern you want.

It is a fork of [ERADICATE2](https://github.com/johguse/ERADICATE2). Where ERADICATE2
searched CREATE2 addresses, ERADICATE3 mines the salt used by a CREATE3 deployer, so
the generated address depends on the **deployer address**, the deployer's **proxy
bytecode hash**, and the **caller address** — not on the init code of the contract you
eventually deploy.

## Building

Requires an OpenCL SDK/runtime for your GPU and a C++11 compiler.

```
make
```

This produces the executable `ERADICATE2.x64` (`ERADICATE2.x64.exe` on Windows).
The OpenCL kernel files (`keccak.cl`, `eradicate2.cl`) are read at runtime, so run the
binary from the repository directory.

## Usage

```
usage: ./ERADICATE2.x64 [OPTIONS]

  Input:
    -A, --caller-address    Address that calls the CREATE3 deployer. Used when
                            deriving the salt, so it must match the address you
                            will deploy from.
    -D, --deployer-address  CREATE3 deployer (factory) address.
                            [default = 1ADD4E55ecEffd795B01d22203D280c93A2F1dc3]
    -B, --bytecode-hash     keccak256 of the deployer's CREATE2 proxy child
                            bytecode.
                            [default = 21c35dbe1b344a2488cf3321d6ce542f8e9f305544ff09e4993a62319a497c1f]
    -I, --init-code         Init code (hex).
    -i, --init-code-file    Read init code from this file.

    The init code should be expressed as a hexadecimal string having the
    prefix 0x both when expressed on the command line with -I and in the
    file pointed to by -i if used. Any whitespace will be trimmed. If no
    init code is specified it defaults to an empty string.

  Basic modes:
    --benchmark             Run without any scoring, a benchmark.
    --zeros                 Score on zeros anywhere in hash.
    --zero-bytes            Score on zero bytes anywhere in hash.
    --letters               Score on letters anywhere in hash.
    --numbers               Score on numbers anywhere in hash.
    --mirror                Score on mirroring from center.
    --leading-doubles       Score on hashes leading with hexadecimal pairs.

  Modes with arguments:
    --leading <single hex>  Score on hashes leading with given hex character.
    --trailing <single hex> Score on hashes trailing with given hex character.
    --matching <hex string> Score on hashes matching given hex string.

  Advanced modes:
    --leading-range         Scores on hashes leading with characters within
                            given range.
    --range                 Scores on hashes having characters within given
                            range anywhere.

  Range:
    -m, --min <0-15>        Set range minimum (inclusive), 0 is '0' 15 is 'f'.
    -M, --max <0-15>        Set range maximum (inclusive), 0 is '0' 15 is 'f'.

  Device control:
    -s, --skip <index>      Skip device given by index. May be repeated.

  Tweaking:
    -w, --work <size>       Set OpenCL local work size. [default = 128]
    -W, --work-max <size>   Set OpenCL maximum work size. [default = -S value]
    -S, --size <size>       Set number of salts tried per loop.
                            [default = 16777216]
```

## Examples

Mine an address with as many leading zeros as possible for a given caller, using the
default 1inch deployer and proxy bytecode hash:

```
./ERADICATE2.x64 -A 0x00000000000000000000000000000000deadbeef --leading 0
```

Score on zeros anywhere in the address:

```
./ERADICATE2.x64 -A 0x00000000000000000000000000000000deadbeef --zeros
```

Use a custom deployer and proxy bytecode hash:

```
./ERADICATE2.x64 \
  -A 0x00000000000000000000000000000000deadbeef \
  -D 0xYourDeployerAddress \
  -B 0xYourProxyChildBytecodeHash \
  --matching dead
```

## Notes

- The salt that produces a match is what you feed to your CREATE3 deployer. Because
  the salt is derived from `--caller-address`, you must deploy from that same caller
  for the address to reproduce.
- `--deployer-address` and `--bytecode-hash` default to the 1inch deployer setup;
  override them if you use a different CREATE3 factory.

## Credits

Fork of ERADICATE2 by Johan Gustafsson. Maintained at
[1inch/ERADICATE3](https://github.com/1inch/ERADICATE3).
