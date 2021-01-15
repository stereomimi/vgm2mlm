#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <cstring>

#if __cplusplus < 201703L // If the version of C++ is less than 17
    #include <experimental/filesystem>
    namespace fs = std::experimental::filesystem;
#else
    #include <filesystem>
    namespace fs = std::filesystem;
#endif

extern "C" {
    #include "vgm2mlm.h"
}

#define PROM_SIZE 524288

const char* HELP_STRING = "usage: vgm2mlm vgm_file out_directory\n";

void eprintf(const char *fmt, ...) 
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void fatal_printf(const char *fmt, ...) 
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(1);
}

// returns pointer to buffer
char* file2buffer(const char* filepath, size_t* size)
{
	FILE* file = NULL;
	char* buffer = NULL;
	int no_size_given = size == NULL;

	if (no_size_given)
		size = (size_t*)malloc(sizeof(size_t));

	file = fopen(filepath, "rb");

	if (file == NULL)
	{
		perror("[file2buffer]");
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	*size = ftell(file);
	rewind(file);
	buffer = (char*)malloc(*size);
	
	if (fread(buffer, 1, *size, file) != *size)
		fatal_printf("Failed to read '%s'\n", filepath);
	fclose(file);

	if (no_size_given)
		free(size);

	return buffer;
}

int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		fatal_printf(HELP_STRING);
		return 1;
	}
    
    fs::create_directories(argv[2]);
	int status = vgm2mlm_df_intf(argv[1], argv[2]);

	if (status != VGM2MLM_STSUCCESS) // (status != 0) also works
		fatal_printf("%s\n", VGM2MLM_STATUS_MESSAGES[status]);

	return 0;
}
