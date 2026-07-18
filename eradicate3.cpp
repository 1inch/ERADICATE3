#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <random>
#include <map>
#include <set>

#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include "hexadecimal.hpp"
#include "Dispatcher.hpp"
#include "ArgParser.hpp"
#include "ModeFactory.hpp"
#include "types.hpp"
#include "help.hpp"
#include "sha3.hpp"

std::string readFile(const char * const szFilename)
{
	std::ifstream in(szFilename, std::ios::in | std::ios::binary);
	std::ostringstream contents;
	contents << in.rdbuf();
	return contents.str();
}

std::vector<cl_device_id> getAllDevices(cl_device_type deviceType = CL_DEVICE_TYPE_GPU)
{
    std::vector<cl_device_id> vDevices;

    cl_uint platformIdCount = 0;
    cl_int err = clGetPlatformIDs(0, nullptr, &platformIdCount);
    if (err != CL_SUCCESS || platformIdCount == 0) return vDevices;

    std::vector<cl_platform_id> platformIds(platformIdCount);
    err = clGetPlatformIDs(platformIdCount, platformIds.data(), nullptr);
    if (err != CL_SUCCESS) return vDevices;

    for (auto p : platformIds) {
        cl_uint countDevice = 0;

        err = clGetDeviceIDs(p, deviceType, 0, nullptr, &countDevice);
        if (err == CL_DEVICE_NOT_FOUND || countDevice == 0) {
            continue; // there's no such type of GPU on this platform
        }
        if (err != CL_SUCCESS) {
            continue;
        }

        std::vector<cl_device_id> deviceIds(countDevice);
        err = clGetDeviceIDs(p, deviceType, countDevice, deviceIds.data(), nullptr);
        if (err != CL_SUCCESS) {
            continue;
        }

        vDevices.insert(vDevices.end(), deviceIds.begin(), deviceIds.end());
    }

    return vDevices;
}

template <typename T, typename U, typename V, typename W>
T clGetWrapper(U function, V param, W param2) {
	T t;
	function(param, param2, sizeof(t), &t, NULL);
	return t;
}

template <typename U, typename V, typename W>
std::string clGetWrapperString(U function, V param, W param2) {
	size_t len;
	function(param, param2, 0, NULL, &len);
	char * const szString = new char[len];
	function(param, param2, len, szString, NULL);
	std::string r(szString);
	delete[] szString;
	return r;
}

template <typename T, typename U, typename V, typename W>
std::vector<T> clGetWrapperVector(U function, V param, W param2) {
	size_t len;
	function(param, param2, 0, NULL, &len);
	len /= sizeof(T);
	std::vector<T> v;
	if (len > 0) {
		T * pArray = new T[len];
		function(param, param2, len * sizeof(T), pArray, NULL);
		for (size_t i = 0; i < len; ++i) {
			v.push_back(pArray[i]);
		}
		delete[] pArray;
	}
	return v;
}

std::vector<std::string> getBinaries(cl_program & clProgram) {
	std::vector<std::string> vReturn;
	auto vSizes = clGetWrapperVector<size_t>(clGetProgramInfo, clProgram, CL_PROGRAM_BINARY_SIZES);
	if (!vSizes.empty()) {
		unsigned char * * pBuffers = new unsigned char *[vSizes.size()];
		for (size_t i = 0; i < vSizes.size(); ++i) {
			pBuffers[i] = new unsigned char[vSizes[i]];
		}

		clGetProgramInfo(clProgram, CL_PROGRAM_BINARIES, vSizes.size() * sizeof(unsigned char *), pBuffers, NULL);
		for (size_t i = 0; i < vSizes.size(); ++i) {
			std::string strData(reinterpret_cast<char *>(pBuffers[i]), vSizes[i]);
			vReturn.push_back(strData);
			delete[] pBuffers[i];
		}

		delete[] pBuffers;
	}

	return vReturn;
}

template <typename T> bool printResult(const T & t, const cl_int & err) {
	std::cout << ((t == NULL) ? lexical_cast::write(err) : "OK") << std::endl;
	return t == NULL;
}

bool printResult(const cl_int err) {
	std::cout << ((err != CL_SUCCESS) ? lexical_cast::write(err) : "OK") << std::endl;
	return err != CL_SUCCESS;
}

std::string keccakDigest(const std::string data) {
	char digest[32];
	sha3(data.c_str(), data.size(), digest, 32);
	return std::string(digest, 32);
}

// Builds the constant CREATE2 pre-image of the CREATE3 proxy:
// 0xff ++ deployer(20) ++ salt(32) ++ keccak256(proxyBytecode)(32), plus keccak padding.
// All arguments are binary strings. The kernel later varies salt words per thread/round.
std::string makePreprocessorInitHashExpression(const std::string & strDeployerBinary, const std::string & strSaltBinary, const std::string & strBytecodeHashBinary) {
	ethhash h = { {0} };

	h.b[0] = 0xff;
	for (int i = 0; i < 20; ++i) {
		h.b[i + 1] = strDeployerBinary[i];
	}

	for (int i = 0; i < 32; ++i) {
		h.b[i + 21] = strSaltBinary[i];
	}

	for (int i = 0; i < 32; ++i) {
		h.b[i + 53] = strBytecodeHashBinary[i];
	}

	h.b[85] ^= 0x01;

	std::ostringstream oss;
	oss << std::hex;
	for (int i = 0; i < 25; ++i) {
		oss << "0x" << h.q[i];
		if (i + 1 != 25) {
			oss << ",";
		}
	}

	return oss.str();
}

int main(int argc, char * * argv) {
	try {
		ArgParser argp(argc, argv);
		bool bHelp = false;
		bool bModeNft = false;
		bool bModeBenchmark = false;
		bool bModeZeroBytes = false;
		bool bModeZeros = false;
		bool bModeLetters = false;
		bool bModeNumbers = false;
		std::string strModeLeading;
		std::string strModeMatching;
		std::string strModeTrailing;
		bool bModeLeadingRange = false;
		bool bModeRange = false;
		bool bModeMirror = false;
		bool bModeDoubles = false;
		int rangeMin = 0;
		int rangeMax = 0;
		std::vector<size_t> vDeviceSkipIndex;
		size_t worksizeLocal = 128;
		size_t worksizeMax = 0; // Will be automatically determined later if not overriden by user
		size_t size = 16777216;
		std::string strAddress;
		std::string strBytecodeHash = "21c35dbe1b344a2488cf3321d6ce542f8e9f305544ff09e4993a62319a497c1f";
		std::string strDeployerAddress;
		std::string strInitCode;
		std::string strInitCodeFile;

		argp.addSwitch('h', "help", bHelp);
		argp.addSwitch('N', "nft", bModeNft);
		argp.addSwitch('0', "benchmark", bModeBenchmark);
		argp.addSwitch('z', "zero-bytes", bModeZeroBytes);
		argp.addSwitch('1', "zeros", bModeZeros);
		argp.addSwitch('2', "letters", bModeLetters);
		argp.addSwitch('3', "numbers", bModeNumbers);
		argp.addSwitch('4', "leading", strModeLeading);
		argp.addSwitch('5', "matching", strModeMatching);
		argp.addSwitch('6', "leading-range", bModeLeadingRange);
		argp.addSwitch('7', "range", bModeRange);
		argp.addSwitch('8', "mirror", bModeMirror);
		argp.addSwitch('9', "leading-doubles", bModeDoubles);
		argp.addSwitch('t', "trailing", strModeTrailing);
		argp.addSwitch('m', "min", rangeMin);
		argp.addSwitch('M', "max", rangeMax);
		argp.addMultiSwitch('s', "skip", vDeviceSkipIndex);
		argp.addSwitch('w', "work", worksizeLocal);
		argp.addSwitch('W', "work-max", worksizeMax);
		argp.addSwitch('S', "size", size);
		argp.addSwitch('A', "caller-address", strAddress);
		argp.addSwitch('B', "bytecode-hash", strBytecodeHash); // create2 PROXY_CHILD_BYTECODE hash
		argp.addSwitch('D', "deployer-address", strDeployerAddress); // create3 deployer address
		argp.addSwitch('I', "init-code", strInitCode);
		argp.addSwitch('i', "init-code-file", strInitCodeFile);

		if (!argp.parse()) {
			std::cout << "error: bad arguments, try again :<" << std::endl;
			return 1;
		}

		const bool bScoringSelected = bModeBenchmark || bModeZeroBytes || bModeZeros || bModeLetters
			|| bModeNumbers || bModeLeadingRange || bModeRange || bModeMirror || bModeDoubles
			|| !strModeLeading.empty() || !strModeTrailing.empty() || !strModeMatching.empty();

		if (bHelp || !bScoringSelected) {
			std::cout << g_strHelp << std::endl;
			return 0;
		}

		// CREATE3 addresses never depend on the deployed contract's init code.
		if (!strInitCode.empty() || !strInitCodeFile.empty()) {
			std::cout << "error: init code does not affect CREATE3 addresses, -I/--init-code and -i/--init-code-file are not supported" << std::endl;
			return 1;
		}

		if (bModeNft) {
			if (strAddress.empty()) {
				std::cout << "error: NFT mode requires -A/--caller-address, the account the vanity address is minted for" << std::endl;
				return 1;
			}
			if (strDeployerAddress.empty()) {
				strDeployerAddress = "1ADD4E55ecEffd795B01d22203D280c93A2F1dc3"; // 1inch Address NFT
			}
		} else {
			if (!strAddress.empty()) {
				std::cout << "error: -A/--caller-address is only used in NFT mode (-N/--nft); pure CREATE3 salts are not bound to a caller" << std::endl;
				return 1;
			}
			if (strDeployerAddress.empty()) {
				if (bModeBenchmark) {
					// Benchmark measures throughput only; the factory address is irrelevant.
					strDeployerAddress = "0000000000000000000000000000000000000000";
				} else {
					std::cout << "error: pure CREATE3 mode requires -D/--deployer-address, the factory whose address the resulting contract depends on" << std::endl;
					return 1;
				}
			}
		}

		const std::string strDeployerBinary = parseHexadecimalBytes(strDeployerAddress);
		const std::string strBytecodeHashBinary = parseHexadecimalBytes(strBytecodeHash);
		if (strDeployerBinary.size() != 20) {
			std::cout << "error: -D/--deployer-address must be a 20-byte hexadecimal address" << std::endl;
			return 1;
		}
		if (strBytecodeHashBinary.size() != 32) {
			std::cout << "error: -B/--bytecode-hash must be a 32-byte hexadecimal hash" << std::endl;
			return 1;
		}

		// Base salt for mining. The kernel varies 12 of the first 16 bytes per
		// device/thread/round; the remaining bytes stay as generated here.
		std::random_device rd;
		std::mt19937_64 eng(rd());
		std::uniform_int_distribution<unsigned int> distr; // C++ requires integer type: "C2338	note : char, signed char, unsigned char, int8_t, and uint8_t are not allowed"
		std::string strSaltBinary(32, '\0');
		for (int i = 0; i < 32; ++i) {
			strSaltBinary[i] = static_cast<char>(distr(eng));
		}

		if (bModeNft) {
			// 1inch Address NFT scheme: salt = magic(16) ++ keccak256(account)[16..31],
			// the lower half binds the address to the account minted for.
			const std::string strAddressDigest = keccakDigest(parseHexadecimalBytes(strAddress));
			for (int i = 0; i < 16; ++i) {
				strSaltBinary[16 + i] = strAddressDigest[16 + i];
			}
		}

		const std::string strPreprocessorInitStructure = makePreprocessorInitHashExpression(strDeployerBinary, strSaltBinary, strBytecodeHashBinary);

		mode mode = ModeFactory::benchmark();
		if (bModeBenchmark) {
			mode = ModeFactory::benchmark();
		} else if (bModeZeroBytes) {
			mode = ModeFactory::zerobytes();
		}  else if (bModeZeros) {
			mode = ModeFactory::zeros();
		} else if (bModeLetters) {
			mode = ModeFactory::letters();
		} else if (bModeNumbers) {
			mode = ModeFactory::numbers();
		} else if (!strModeLeading.empty()) {
			mode = ModeFactory::leading(strModeLeading.front());
		} else if (!strModeTrailing.empty()) {
			mode = ModeFactory::trailing(strModeTrailing);
		} else if (!strModeMatching.empty()) {
			mode = ModeFactory::matching(strModeMatching);
		} else if (bModeLeadingRange) {
			mode = ModeFactory::leadingRange(rangeMin, rangeMax);
		} else if (bModeRange) {
			mode = ModeFactory::range(rangeMin, rangeMax);
		} else if(bModeMirror) {
			mode = ModeFactory::mirror();
		} else if (bModeDoubles) {
			mode = ModeFactory::doubles();
		} else {
			std::cout << g_strHelp << std::endl;
			return 0;
		}

		std::vector<cl_device_id> vFoundDevices = getAllDevices();
		std::vector<cl_device_id> vDevices;
		std::map<cl_device_id, size_t> mDeviceIndex;

		std::vector<std::string> vDeviceBinary;
		std::vector<size_t> vDeviceBinarySize;
		cl_int errorCode;

		std::cout << "Devices:" << std::endl;
		for (size_t i = 0; i < vFoundDevices.size(); ++i) {
			// Ignore devices in skip index
			if (std::find(vDeviceSkipIndex.begin(), vDeviceSkipIndex.end(), i) != vDeviceSkipIndex.end()) {
				continue;
			}

			cl_device_id & deviceId = vFoundDevices[i];

			const auto strName = clGetWrapperString(clGetDeviceInfo, deviceId, CL_DEVICE_NAME);
			const auto computeUnits = clGetWrapper<cl_uint>(clGetDeviceInfo, deviceId, CL_DEVICE_MAX_COMPUTE_UNITS);
			const auto globalMemSize = clGetWrapper<cl_ulong>(clGetDeviceInfo, deviceId, CL_DEVICE_GLOBAL_MEM_SIZE);

			std::cout << "  GPU" << i << ": " << strName << ", " << globalMemSize << " bytes available, " << computeUnits << " compute units" << std::endl;
			vDevices.push_back(vFoundDevices[i]);
			mDeviceIndex[vFoundDevices[i]] = i;
		}

		if (vDevices.empty()) {
			return 1;
		}

		std::cout << std::endl;
		std::cout << "Initializing OpenCL..." << std::endl;
		std::cout << "  Creating context..." << std::flush;
		auto clContext = clCreateContext( NULL, vDevices.size(), vDevices.data(), NULL, NULL, &errorCode);
		if (printResult(clContext, errorCode)) {
			return 1;
		}

		cl_program clProgram;
		if (vDeviceBinary.size() == vDevices.size()) {
			// Create program from binaries
			std::cout << "  Loading kernel from binary..." << std::flush;
			const unsigned char * * pKernels = new const unsigned char *[vDevices.size()];
			for (size_t i = 0; i < vDeviceBinary.size(); ++i) {
				pKernels[i] = reinterpret_cast<const unsigned char *>(vDeviceBinary[i].data());
			}

			cl_int * pStatus = new cl_int[vDevices.size()];

			clProgram = clCreateProgramWithBinary(clContext, vDevices.size(), vDevices.data(), vDeviceBinarySize.data(), pKernels, pStatus, &errorCode);
			if(printResult(clProgram, errorCode)) {
				return 1;
			}
		} else {
			// Create a program from the kernel source
			std::cout << "  Compiling kernel..." << std::flush;
			const std::string strKeccak = readFile("keccak.cl");
			const std::string strVanity = readFile("eradicate3.cl");
			const char * szKernels[] = { strKeccak.c_str(), strVanity.c_str() };

			clProgram = clCreateProgramWithSource(clContext, sizeof(szKernels) / sizeof(char *), szKernels, NULL, &errorCode);
			if (printResult(clProgram, errorCode)) {
				return 1;
			}
		}

		// Build the program
		std::cout << "  Building program..." << std::flush;

		const std::string strBuildOptions = "-D ERADICATE3_MAX_SCORE=" + lexical_cast::write(ERADICATE3_MAX_SCORE) + " -D ERADICATE3_INITHASH=" + strPreprocessorInitStructure;
		if (printResult(clBuildProgram(clProgram, vDevices.size(), vDevices.data(), strBuildOptions.c_str(), NULL, NULL))) {
#ifdef ERADICATE3_DEBUG
			std::cout << std::endl;
			std::cout << "build log:" << std::endl;

			size_t sizeLog;
			clGetProgramBuildInfo(clProgram, vDevices[0], CL_PROGRAM_BUILD_LOG, 0, NULL, &sizeLog);
			char * const szLog = new char[sizeLog];
			clGetProgramBuildInfo(clProgram, vDevices[0], CL_PROGRAM_BUILD_LOG, sizeLog, szLog, NULL);

			std::cout << szLog << std::endl;
			delete[] szLog;
#endif
			return 1;
		}

		std::cout << std::endl;

		Dispatcher d(clContext, clProgram, worksizeMax == 0 ? size : worksizeMax, size, !bModeNft);
		for (auto & i : vDevices) {
			d.addDevice(i, worksizeLocal, mDeviceIndex[i]);
		}

		d.run(mode);
		clReleaseContext(clContext);
		return 0;
	} catch (std::runtime_error & e) {
		std::cout << "std::runtime_error - " << e.what() << std::endl;
	} catch (...) {
		std::cout << "unknown exception occured" << std::endl;
	}

	return 1;
}
