# ERADICATE3

ERADICATE3 is a GPU-accelerated (OpenCL) vanity address generator for **CREATE3** deployments. It brute-forces salts until the resulting CREATE3 address matches the pattern you want.

It is a fork of [ERADICATE2](https://github.com/johguse/ERADICATE2). Where ERADICATE2 searched CREATE2 addresses, ERADICATE3 mines the salt used by a CREATE3 factory, so the generated address depends only on the **factory (deployer) address**, the factory's **proxy bytecode hash**, and the **salt** — not on the init code of the contract you eventually deploy.

Two deployment schemes are supported:

- **Pure CREATE3 (default)** — for factories that use your 32-byte salt verbatim, e.g. a [`Create3Deployer`](https://etherscan.io/address/0xaa710bd40c633Ab46d30Fc6baF6885143f3a6Dd7) with `deploy(bytes32 salt, bytes code)`. The full salt is mined and printed, ready to pass to `deploy(salt, code)` / `addressOf(salt)`.
- **1inch Address NFT mode (`--nft`)** — for the [1inch Address NFT](https://etherscan.io/address/0x1ADD4E55ecEffd795B01d22203D280c93A2F1dc3) deployer, which derives the salt as `magic | (keccak256(account) & LOW_128_BIT_MASK)`: the high 16 bytes are a free-to-choose `magic` and the low 16 bytes are bound to the account as front-running protection. The 16-byte magic is mined and printed; pass it to `mint(magic)` or `mintFor(magic, account)` to claim the address.

## Building

Requires an OpenCL SDK/runtime for your GPU and a C++11 compiler — see [INSTALL.md](INSTALL.md) for platform-specific instructions.

```
make
```

This produces the executable `ERADICATE3.x64` (`ERADICATE3.x64.exe` on Windows). The OpenCL kernel files (`keccak.cl`, `eradicate3.cl`) are read at runtime, so run the binary from the repository directory.

## Usage

```
usage: ./ERADICATE3.x64 [OPTIONS]

  Deployment scheme:
    -N, --nft               1inch Address NFT mode. Mines the bytes16 magic for
                            mint(magic)/mintFor(magic, account); the deployer
                            derives the salt as magic ++ keccak256(account)[16..31].

  Input:
    -D, --deployer-address  CREATE3 factory address. Required in default mode.
                            [default in NFT mode = 1ADD4E55ecEffd795B01d22203D280c93A2F1dc3]
    -B, --bytecode-hash     keccak256 of the factory's CREATE2 proxy child
                            bytecode.
                            [default = 21c35dbe1b344a2488cf3321d6ce542f8e9f305544ff09e4993a62319a497c1f]
    -A, --caller-address    NFT mode only (required there): account the vanity
                            address is minted for. Rejected in default mode.

    Init code never affects CREATE3 addresses, so -I/--init-code and
    -i/--init-code-file are rejected in both modes.

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

### Pure CREATE3 (default)

Mine an address with as many leading zeros as possible for a plain CREATE3 factory:

```
./ERADICATE3.x64 -D 0xaa710bd40c633Ab46d30Fc6baF6885143f3a6Dd7 --leading 0
```

Each match prints the full 32-byte salt:

```
  Time:    12s Score: 10 Salt: 0x37e1...c04b Address: 0x00000...
```

Use that salt directly with the factory: verify with `addressOf(salt)` and deploy with `deploy(salt, initCode)`. The address does not depend on the init code or on who sends the transaction (though factories like `Create3Deployer` may restrict who is allowed to call `deploy`).

### 1inch Address NFT mode

Mine a magic bound to your account (deployer defaults to the 1inch Address NFT contract):

```
./ERADICATE3.x64 --nft -A 0x00000000000000000000000000000000deadbeef --zeros
```

Each match prints the 16-byte magic:

```
  Time:    12s Score: 10 Magic: 0x37e1...c04b Address: 0x00000...
```

Pass it to the deployer's `mint(magic)` / `mintFor(magic, account)`. The full CREATE3 salt is that magic in the high 16 bytes plus the low 16 bytes of `keccak256(account)`, so the address only reproduces for the same `-A` account. Anyone can submit the mint transaction — the address is bound to the account, not to the transaction sender.

### Custom factory

Any CREATE3 factory that uses the standard proxy bytecode works with the default `-B`; override it for a non-standard proxy:

```
./ERADICATE3.x64 \
  -D 0xYourFactoryAddress \
  -B 0xYourProxyChildBytecodeHash \
  --matching dead
```

## Credits

Fork of ERADICATE2 by Johan Gustafsson. Maintained at [1inch/ERADICATE3](https://github.com/1inch/ERADICATE3).
