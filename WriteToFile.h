#pragma once
#pragma once
#include <cstdio>
#include <string>

#ifdef _MSC_VER
// Windows MSVC
#define SAFE_FOPEN(fp, filename, mode) fopen_s(&fp, filename, mode)
#else
// Linux/macOS/GCC/Clang
#define SAFE_FOPEN(fp, filename, mode) ((fp) = fopen(filename, mode))
#endif

class CFileHelper {
public:
	static FILE* OpenFileForAppend(const std::string& filename) {
		FILE* fp = nullptr;
		if (SAFE_FOPEN(fp, filename.c_str(), "a") != 0) {
			return nullptr;
		}
		return fp;
	}

	static void WriteToFile(FILE* fp, const std::string& msg) {
		if (fp) {
			fprintf(fp, "%s\n", msg.c_str());
			fflush(fp);
		}
	}

	static void CloseFile(FILE* fp) {
		if (fp) fclose(fp);
	}
};
