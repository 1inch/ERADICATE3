#ifndef HPP_HELP
#define HPP_HELP

#include <string>

const std::string g_strHelp = R"(
usage: ./ERADICATE3.x64 [OPTIONS]

  By default ERADICATE3 mines full 32-byte salts for a plain CREATE3 factory
  that uses the salt verbatim, e.g. Create3Deployer.deploy(salt, code). The
  resulting address depends only on the factory address and the salt.

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
    --leading-doubles       Score on hashes leading with hexadecimal pairs

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
    -s, --skip <index>      Skip device given by index.

  Tweaking:
    -w, --work <size>       Set OpenCL local work size. [default = 128]
    -W, --work-max <size>   Set OpenCL maximum work size. [default = -S value]
    -S, --size <size>       Set number of salts tried per loop.
                            [default = 16777216]

  Examples:
    ./ERADICATE3.x64 -D 0xaa710bd40c633Ab46d30Fc6baF6885143f3a6Dd7 --leading 0
    ./ERADICATE3.x64 --nft -A 0x00000000000000000000000000000000deadbeef --zeros

  About:
    ERADICATE3 is a vanity address generator for CREATE3 deployments that
    utilizes computing power from GPUs using OpenCL. It is a fork of
    ERADICATE2 by Johan Gustafsson.
)";

#endif /* HPP_HELP */
