#include <stdio.h>
#include <getopt.h>
#include "common.h"
#include "partial/partial.h"

static char quiet = 0;

void callback(ZipInfo * info, CDFile * file, size_t progress) {
	int percentDone = progress * 100 / file->compressedSize;
	if (!quiet) fprintf(stderr, "Getting: %d%%\033[2K", percentDone);
}

void print_help() {
	fprintf(stderr,
		"Usage: partialzip [-Z] [-opts[modifiers]] fileurl[.zip] [list] [-x xlist] [-d exdir]\n"
		"\n"
		"  Default action is to extract files in list, except those in xlist, to exdir;\n"
		"  fileurl[.zip] may not be a wildcard.  -Z => ZipInfo mode (\"partialzip -Z\" for usage).\n"
		"\n"
		"  -p  extract files to pipe, no messages     -l  list files (short format)\n"
		"  -f  freshen existing files, create none    -t  test compressed archive data\n"
		"  -u  update files, create if necessary      -z  display archive comment only\n"
		"  -v  list verbosely/show version info       -T  timestamp archive to latest\n"
		"  -x  exclude files that follow (in xlist)   -d  extract files into exdir\n"
		"modifiers:\n"
		"  -n  never overwrite existing files         -q  quiet mode (-Q => quieter)\n"
		"  -o  overwrite files WITHOUT prompting      -a  auto-convert any text files\n"
		"  -j  junk paths (do not make directories)   -A  treat ALL files as text\n"
		"  -C  match filenames case-insensitively     -L  make (some) names lowercase\n"
		"  -X  restore UID/GID info                   -V  retain VMS version numbers\n"
		"  -K  keep setuid/setgid/tacky permissions   -M  pipe through \"more\" pager\n"
		"See \"partialzip -hh\" or unzip.txt for more help.  Examples:\n"
		"  partialzip data1 -x joe   => extract all files except joe from zipfile data1.zip\n"
		"  partialzip -p foo | more  => send contents of foo.zip via pipe into program more\n"
		"  partialzip -fo foo ReadMe => quietly replace existing ReadMe if archive file newer\n");
}

int extract_file(ZipInfo * info, const char * filename, unsigned char ** buffer, size_t * size) {
	CDFile * file = PartialZipFindFile(info, filename);
	if (!file) {
		if (!quiet) fprintf(stderr, "Cannot find %s in %s\n", filename, info->url);
		return -1;
	}

	int dataLen = file->size;
	unsigned char * data = PartialZipGetFile(info, file);
	//TODO: check failure of realloc
	data = realloc(data, dataLen + 1);
	data[dataLen] = '\0';

	*buffer = data;
	*size = dataLen;
	return 0;
}

int write_file(const char * filename, unsigned char * data, size_t size) {
	FILE * fp = fopen(filename, "w");
	if (fp == NULL) {
		if (!quiet) fprintf(stderr, "Failed to open file at %s\n", filename);
		return -1;
	}

	int done = fwrite(data, sizeof(unsigned char), size, fp);
	fclose(fp);
	return done != size;
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		print_help();
		return -1;
	}

	char h = 0, l = 0, v = 0, z = 0, t = 0, p = 0, f = 0, u = 0, T = 0, x = 0, d = 0;
	char q = 0, Q = 0, n = 0, o = 0, j = 0, C = 0, L = 0, a = 0, A = 0, X = 0, K = 0, V = 0, M = 0;
	char * exdir = NULL;
	int opt;
	int option_index = 0;
	while ((opt = getopt(argc, argv, "hlvztpfuTxd:qQnojCLaAXKVM")) != -1) {
		switch (opt) {
		case 'h': h = 1; break;
		case 'l': l = 1; break;
		case 'v': v = 1; break;
		case 'z': z = 1; break;
		case 't': t = 1; break;
		case 'p': p = 1; break;
		case 'f': f = 1; break;
		case 'u': u = 1; break;
		case 'T': T = 1; break;
		case 'x': x = 1; break;
		case 'd': d = 1; exdir = optarg; break;
		case 'q': q = 1; break;
		case 'Q': Q = 1; break;
		case 'n': n = 1; break;
		case 'o': o = 1; break;
		case 'j': j = 1; break;
		case 'C': C = 1; break;
		case 'L': L = 1; break;
		case 'a': a = 1; break;
		case 'A': A = 1; break;
		case 'X': X = 1; break;
		case 'K': K = 1; break;
		case 'V': V = 1; break;
		case 'M': M = 1; break;
		}
	}

	quiet = q;

	if (h) {
		print_help();
		return 0;
	}

	// Prepend with "file://" if not already present
	char * url = argv[optind++];
	char fname[strlen(url) + sizeof("file://")];
	if (strstr(url, "://") == NULL) {
		strcpy(fname, "file://");
	}
	strcat(fname, url);

	ZipInfo * info = PartialZipInitWithCallback(fname, callback);
	if (!info) {
		if (!quiet) fprintf(stderr, "Cannot open %s\n", fname);
		return -1;
	}

#if 0
  -l  list files (short format)
  -v  list verbosely/show version info
  -z  display archive comment only
  -t  test compressed archive data
  -p  extract files to pipe, no messages
  -f  freshen existing files, create none
  -u  update files, create if necessary
  -T  timestamp archive to latest
  -x  exclude files that follow (in xlist)
  -d  extract files into exdir
modifiers:
  -q  quiet mode (-Q => quieter)
  -n  never overwrite existing files
  -o  overwrite files WITHOUT prompting
  -j  junk paths (do not make directories)
  -C  match filenames case-insensitively
  -L  make (some) names lowercase
  -a  auto-convert any text files
  -A  treat ALL files as text
  -X  restore UID/GID info
  -K  keep setuid/setgid/tacky permissions
  -V  retain VMS version numbers
  -M  pipe through "more" pager
#endif

	if (l) {
		PartialZipListFiles(info);
		goto cleanup;
	}

	// printf("remaining args: %d\n", argc - optind);
	// for (int i = optind; i < argc; i++) {
	// 	printf("%s\n", argv[i]);
	// }

	if (argc - optind == 0) {
		if (!quiet) fprintf(stdout, "[#] Extracting all files\n");
		goto cleanup;
	} else {
		if (x) {
			if (!quiet) fprintf(stdout, "[#] Extracting all but listed files\n");
		} else {
			// printf("[#] Extracting listed files\n");
			for (int i = optind; i < argc; i++) {
				const char * filename = argv[i];
				unsigned char * buffer = NULL;
				size_t size = 0;
				if (!quiet) fprintf(stdout, "  inflating: %s\n", filename);
				int r = extract_file(info, filename, &buffer, &size);
				if (r == 0) {
					//check if file exists, print and read answer:
					// printf("replace %s? [y]es, [n]o, [A]ll, [N]one, [r]ename: ", filename);
					r = write_file(filename, buffer, size);
					if (r != 0) {
						if (!quiet) fprintf(stderr, "Couldn't extract %s\n", filename);
					}
				}
			}
		}

		goto cleanup;
	}

	if (!quiet) fprintf(stderr, "Not supported yet\n");

	cleanup:
	PartialZipRelease(info);

	return 0;
}
