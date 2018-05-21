#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

#ifdef __unix
#include <unistd.h>
#elif _WIN32
#include <windows.h>
#endif

#include "vm.h"
#include "blang.h"
#include "util/stringbuf.h"

static void header() {
	const char blang_ascii_art[] = {
	  0x20, 0x20, 0x5f, 0x5f, 0x5f, 0x20, 0x5f, 0x20, 0x20, 0x20, 0x20, 0x20,
	  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x0a,
	  0x20, 0x7c, 0x20, 0x5f, 0x20, 0x29, 0x20, 0x7c, 0x5f, 0x5f, 0x20, 0x5f,
	  0x20, 0x5f, 0x20, 0x5f, 0x20, 0x20, 0x5f, 0x5f, 0x20, 0x5f, 0x20, 0x0a,
	  0x20, 0x7c, 0x20, 0x5f, 0x20, 0x5c, 0x20, 0x2f, 0x20, 0x5f, 0x60, 0x20,
	  0x7c, 0x20, 0x27, 0x20, 0x5c, 0x2f, 0x20, 0x5f, 0x60, 0x20, 0x7c, 0x0a,
	  0x20, 0x7c, 0x5f, 0x5f, 0x5f, 0x2f, 0x5f, 0x5c, 0x5f, 0x5f, 0x2c, 0x5f,
	  0x7c, 0x5f, 0x7c, 0x7c, 0x5f, 0x5c, 0x5f, 0x5f, 0x2c, 0x20, 0x7c, 0x0a,
	  0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	  0x20, 0x20, 0x20, 0x20, 0x20, 0x7c, 0x5f, 0x5f, 0x5f, 0x2f, 0x20, 0x0a,
	  0
	};

	printf(blang_ascii_art);
	printf("Version %s\n", BLANG_VERSION_STR);
}

static int charCount(const char *str, char c) {
	int count = 0;
	size_t len = strlen(str);
	for(size_t i = 0; i < len; i++) {
		if(str[i] == c) {
			count++;
		}
	}
	return count;
}

static void interactiveEval(VM *vm) {
	header();
	rl_bind_key('\t', rl_insert);

	StringBuffer src;
	sbuf_create(&src);
	for(;;) {
		char *line = readline("blang>> ");
		if(line == NULL) {
			printf("\n");
			break;
		}

		if(strlen(line) == 0) {
			free(line);
			continue;
		}

		sbuf_appendstr(&src, line);
		add_history(line);

		int openc = charCount(line, '{');
		int closec = charCount(line, '}');
		int depth = openc - closec;

		free(line);

		if(depth > 0) {
			char *blockLine;
			while((blockLine = readline("....... ")) != NULL) {
				if(strlen(blockLine) == 0) continue;

				sbuf_appendchar(&src, '\n');
				sbuf_appendstr(&src, blockLine);
				add_history(blockLine);

				openc = charCount(blockLine, '{');
				closec = charCount(blockLine, '}');
				depth += openc - closec;

				free(blockLine);

				if(depth <= 0) break;
			}
		}

		evaluate(vm, sbuf_get_backing_buf(&src));
		sbuf_clear(&src);
	}

	sbuf_destroy(&src);
}

static char* readSrcFile(const char *path) {
	FILE *srcFile = fopen(path, "r+");
	if(srcFile == NULL || errno == EISDIR) {
		if(srcFile) fclose(srcFile);
		perror("Error while reading input file");
		return NULL;
	}

	fseek(srcFile, 0, SEEK_END);
	size_t size = ftell(srcFile);
	rewind(srcFile);

	char *src = malloc(size + 1);
	if(src == NULL) {
		fclose(srcFile);
		fprintf(stderr, "Error while reading the file: out of memory.\n");
		return NULL;
	}

	size_t read = fread(src, sizeof(char), size, srcFile);
	if(read < size) {
		free(src);
		fclose(srcFile);
		fprintf(stderr, "Error: couldn't read file.\n");
		return NULL;
	}

	fclose(srcFile);

	src[read] = '\0';
	return src;
}

int main(int argc, const char **argv) {
	VM vm;
	initVM(&vm);

	EvalResult res = VM_EVAL_SUCCSESS;
	if(argc == 1) {
		interactiveEval(&vm);
	} else {
		initCommandLineArgs(argc - 2, argv + 2);

		char *src = readSrcFile(argv[1]);
		if(src == NULL) {
			return VM_GENERIC_ERR;
		}

		char *directory = strrchr(argv[1], '/');
		if(directory != NULL) {
			size_t length = directory - argv[1] + 1;
			char *pwd = malloc(length + 1);
			memcpy(pwd, argv[1], length);
			pwd[length] = '\0';

#ifdef __unix
			if(chdir(pwd))
#elif _WIN32
			if(!SetCurrentDirectory(pwd))
#endif
			{
				fprintf(stderr, "Failed to change cwd.\n");
				exit(1);
			}


			free(pwd);
		}

		res = evaluate(&vm, src);
		free(src);
	}

	freeVM(&vm);
	exit(res);
}