#include "file.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#endif

#if defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))
#include <sys/stat.h>
#endif

#define FIELD_FILE_HANDLE "_handle"
#define FIELD_FILE_CLOSED "_closed"

#define BL_SEEK_SET  0
#define BL_SEEK_CURR 1
#define BL_SEEK_END  2

// static helper functions

static ObjString *readline(BlangVM *vm, FILE *file) {
	size_t len = 0;
	size_t size = 256;
	ObjString *line = allocateString(vm, size);

	char *ret = fgets(line->data, size + 1, file);
	if(ret == NULL) {
		if(feof(file)) {
			reallocateString(vm, line, 0);
			return line;
		} else {
			return NULL;
		}
	}
	len = strlen(line->data);

	char *newLine;
	while((newLine = strchr(line->data, '\n')) == NULL) {
		char buf[256];

		ret = fgets(buf, 256, file);
		if(ret == NULL) {
			if(feof(file)) {
				break;
			} else {
				return NULL;
			}
		}

		size_t bufLen = strlen(buf);
		while(len + bufLen >= size) {
			size_t newSize = size * 2;
			reallocateString(vm, line, newSize);
			size = newSize;
		}

		memcpy(line->data + len, buf, bufLen);
		len += bufLen;
	}

	if(line->length != len)
		reallocateString(vm, line, len);
	
	return line;
}

static int64_t getFileSize(FILE *stream) {
	int64_t fsize = -1;

#ifdef _WIN32
	int fd = _fileno(stream);
	if(fd < 0) {
		return -1;
	}

	HANDLE f = (HANDLE)_get_osfhandle(fd);
	if(f == INVALID_HANDLE_VALUE) {
		return -1;
	}

	DWORD lo = 0;
	DWORD hi = 0;
	lo = GetFileSize(f, &hi);
	fsize = (int64_t) (((uint64_t) hi) << 32) | lo;
#else
	int fd = fileno(stream);
	if(fd < 0) {
		return -1;
	}

	struct stat stat;
	if(fstat(fd, &stat)) {
		return -1;
	}

	fsize = (int64_t) stat.st_size;
#endif

	return fsize;
}

static int blSeek(FILE *file, long offset, int blWhence) {
	int whence = 0;
	switch(blWhence) {
	case BL_SEEK_SET:
		whence = SEEK_SET;
		break;
	case BL_SEEK_CURR:
		whence = SEEK_CUR;
		break;
	case BL_SEEK_END:
		whence = SEEK_END;
		break;
	}
	return fseek(file, offset, whence);
}

// class File {

static bool isClosed(BlangVM *vm, ObjInstance *file) {
	Value closed;
	blGetField(vm, file, FIELD_FILE_CLOSED, &closed);
	return AS_BOOL(closed);
}

NATIVE(bl_File_seek) {
	if(isClosed(vm, BL_THIS)) {
		BL_RAISE_EXCEPTION(vm, "IOException", "closed file");
	}

	Value h;
	if(!blGetField(vm, BL_THIS, FIELD_FILE_HANDLE, &h) || !IS_HANDLE(h)) {
		BL_RETURN_NULL;
	}

	if(!checkInt(vm, args[1], "off") || !checkInt(vm, args[2], "whence")) {
		return true;
	}

	FILE *f = (FILE*) AS_HANDLE(h);

	double offset = AS_NUM(args[1]);
	double whence = AS_NUM(args[2]);

	if(whence != BL_SEEK_SET && whence != BL_SEEK_CURR && whence != BL_SEEK_END) {
		BL_RAISE_EXCEPTION(vm, "InvalidArgException",
			"whence must be SEEK_SET, SEEK_CUR or SEEK_END");
	}

	if(blSeek(f, offset, whence)) {
		BL_RAISE_EXCEPTION(vm, "IOException", strerror(errno));
	}

	BL_RETURN_NULL;
}

NATIVE(bl_File_tell) {
	if(isClosed(vm, BL_THIS)) {
		BL_RAISE_EXCEPTION(vm, "IOException", "closed file");
	}

	Value h;
	if(!blGetField(vm, BL_THIS, FIELD_FILE_HANDLE, &h) || !IS_HANDLE(h)) {
		BL_RETURN_NULL;
	}

	FILE *f = (FILE*) AS_HANDLE(h);

	long off = ftell(f);
	if(off == -1) {
		BL_RAISE_EXCEPTION(vm, "IOException", strerror(errno));
	}

	BL_RETURN_NUM(off);
}

NATIVE(bl_File_rewind) {
	if(isClosed(vm, BL_THIS)) {
		BL_RAISE_EXCEPTION(vm, "IOException", "closed file");
	}

	Value h;
	if(!blGetField(vm, BL_THIS, FIELD_FILE_HANDLE, &h) || !IS_HANDLE(h)) {
		BL_RETURN_NULL;
	}

	FILE *f = (FILE*) AS_HANDLE(h);
	rewind(f);
	BL_RETURN_NULL;
}

NATIVE(bl_File_readAll) {
	if(isClosed(vm, BL_THIS)) {
		BL_RAISE_EXCEPTION(vm, "IOException", "closed file");
	}

	Value h;
	if(!blGetField(vm, BL_THIS, FIELD_FILE_HANDLE, &h) || !IS_HANDLE(h)) {
		BL_RETURN_NULL;
	}

	FILE *f = (FILE*) AS_HANDLE(h);

	long off = ftell(f);
	if(off == -1) {
		BL_RAISE_EXCEPTION(vm, "IOException", strerror(errno));
	}

	int64_t size = getFileSize(f) - off;
	if(size < 0) {
		BL_RETURN_NULL;
	}

	ObjString *data = allocateString(vm, size);

	if(fread(data->data, sizeof(char), size, f) < (size_t) size) {
		BL_RAISE_EXCEPTION(vm, "IOException", "Couldn't read the whole file.");
	}

	BL_RETURN_OBJ(data);
}

NATIVE(bl_File_readLine) {
	if(isClosed(vm, BL_THIS)) {
		BL_RAISE_EXCEPTION(vm, "IOException", "closed file");
	}

	Value h;
	if(!blGetField(vm, BL_THIS, FIELD_FILE_HANDLE, &h) || !IS_HANDLE(h)) {
		BL_RETURN_NULL;
	}

	FILE *f = (FILE*) AS_HANDLE(h);

	ObjString *line = readline(vm, f);
	if(line == NULL) {
		BL_RAISE_EXCEPTION(vm, "IOException", strerror(errno));
	}

	BL_RETURN_OBJ(line);
}

NATIVE(bl_File_close) {
	Value h;
	if(!blGetField(vm, BL_THIS, FIELD_FILE_HANDLE, &h) || !IS_HANDLE(h)) {
		BL_RETURN_NULL;
	}

	blSetField(vm, BL_THIS, FIELD_FILE_HANDLE, NULL_VAL);

	FILE *f = (void*) AS_HANDLE(h);
	if(fclose(f)) {
		BL_RAISE_EXCEPTION(vm, "IOException", strerror(errno));
	}

	blSetField(vm, BL_THIS, FIELD_FILE_HANDLE, NULL_VAL);
	blSetField(vm, BL_THIS, FIELD_FILE_CLOSED, BOOL_VAL(true));

	BL_RETURN_NULL;
}

NATIVE(bl_File_size) {
	if(isClosed(vm, BL_THIS)) {
		BL_RAISE_EXCEPTION(vm, "IOException", "closed file");
	}

	Value h;
	if(!blGetField(vm, BL_THIS, FIELD_FILE_HANDLE, &h) || !IS_HANDLE(h)) {
		BL_RETURN_NUM(-1);
	}

	FILE *stream = (FILE*) AS_HANDLE(h);

	BL_RETURN_NUM(getFileSize(stream));
}

// } class File

// functions

NATIVE(bl_open) {
	char *fname = AS_STRING(args[1])->data;
	char *m = AS_STRING(args[2])->data;

	size_t mlen = strlen(m);
	if(mlen > 3 ||
	  (m[0] != 'r' && m[0] != 'w' && m[0] != 'a') ||
	  (mlen > 1 && (m[1] != 'b' && m[1] != '+')) ||
	  (mlen > 2 && m[2] != 'b'))
	{
		BL_RAISE_EXCEPTION(vm, "InvalidArgException", "invalid mode string \"%s\"", m);
	}

	FILE *f = fopen(AS_STRING(args[1])->data, m);
	if(f == NULL) {
		if(errno == ENOENT) {
			BL_RAISE_EXCEPTION(vm, "FileNotFoundException", "Couldn't find file `%s`.", fname);
		}
		BL_RAISE_EXCEPTION(vm, "IOException", strerror(errno));
	}

	BL_RETURN(HANDLE_VAL(f));
}
