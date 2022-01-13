#ifndef FILE_HANDLING_TEST_H
#define FILE_HANDLING_TEST_H

#include "config.h"
#include "stdlib.h"
#include "sntptest.h"

#include <string.h>
#include <unistd.h>


enum DirectoryType {
	INPUT_DIR = 0,
	OUTPUT_DIR = 1
};

#define SRCDIR_DEF "/tmp/ntp-4.2.8p8/nto-x86-o/sntp/../../sntp/tests/data/";

extern	const char * 	CreatePath(const char* filename,
				   enum DirectoryType argument);
extern	void		DestroyPath(const char* pathname);
extern	int		GetFileSize(FILE *file);
extern	bool		CompareFileContent(FILE* expected, FILE* actual);
extern	void		ClearFile(const char * filename) ;

#endif // FILE_HANDLING_TEST_H

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/tags/7.0.0/GA/lib/io-pkt/dist/ntp/nto-x86-o/sntp/tests/fileHandlingTest.h $ $Rev: 806186 $")
#endif
