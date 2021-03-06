/**@file main.c
 * @brief Extensions and driver for the 'pickle' interpreter. The
 * interpreter is a copy and modification of the 'picol' interpreter
 * by antirez. See the 'pickle.h' header for more information.
 * @author Richard James Howe
 * @license BSD */

#include "pickle.h"
#include "block.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>
#include <stdarg.h>

#define LINE_SZ   (1024)      /* super lazy: maximum size of a line */
#define UNUSED(X) ((void)(X))
#define NELEM(X)  (sizeof (X) / sizeof ((X)[0]))

typedef struct {
	char *arg;   /* parsed argument */
	int error,   /* turn error reporting on/off */
	    index,   /* index into argument list */
	    option,  /* parsed option */
	    reset;   /* set to reset */
	char *place; /* internal use: scanner position */
	int  init;   /* internal use: initialized or not */
} pickle_getopt_t;   /* getopt clone; with a few modifications */

typedef struct {
	const char *name;
	pickle_command_func_t func;
	void *data;
} pickle_register_command_t;

typedef struct { int argc; char **argv; } argument_t;

static int use_custom_allocator = 0;
static pickle_t *interp = NULL;
static int signal_variable = 0;

static void *custom_malloc(void *a, size_t length)           { return pool_malloc(a, length); }
static int   custom_free(void *a, void *v)                   { return pool_free(a, v); }
static void *custom_realloc(void *a, void *v, size_t length) { return pool_realloc(a, v, length); }

static pickle_allocator_t block_allocator = {
	.free    = custom_free,
	.realloc = custom_realloc,
	.malloc  = custom_malloc,
	.arena   = NULL
};

static int get_a_line(FILE *input, char **out) {
	assert(input);
	assert(out);
	char *line = NULL, *p = NULL;
	size_t linelen = 0;
	*out = NULL;
	for (char t[LINE_SZ] = { 0 }; fgets(t, sizeof t, input); memset(t, 0, sizeof t)) {
		const size_t tlen = strlen(t);
		linelen += tlen;
		p = realloc(line, linelen + 1);
		if (!p) {
			free(line);
			return PICKLE_ERROR;
		}
		if (linelen <= LINE_SZ)
			memset(p, 0, linelen + 1);
		line = p;
		strcat(line, t);
		if (!(tlen == (sizeof(t) - 1) && t[sizeof(t) - 2] != '\n'))
			break;
	}
	*out = line;
	return PICKLE_OK;
}

static int pickleGetLine(pickle_t *i, FILE *f) {
	assert(i);
	assert(f);
	char *line = NULL;
	int r = get_a_line(f, &line);
	if (r < 0) {
		free(line);
		return pickle_set_result_error(i, "Out Of Memory");
	}
	if (line) {
		r = pickle_set_result_string(i, line);
	} else {
		if (pickle_set_result_string(i, "EOF") != PICKLE_OK)
			return pickle_set_result_error(i, "Out Of Memory");
		r = PICKLE_BREAK;
	}
	free(line);
	return r;
}

static int pickleCommandSystem(pickle_t *i, const int argc, char **argv, void *pd) {
	assert(!pd);
	UNUSED(pd);
	if (argc != 2 && argc != 1)
		return pickle_set_result_error_arity(i, 2, argc, argv);
	const int r = system(argc == 1 ? NULL : argv[1]);
	return pickle_set_result_integer(i, r);
}

static int pickleCommandRandom(pickle_t *i, const int argc, char **argv, void *pd) {
	assert(!pd);
	UNUSED(pd);
	if (argc != 1 && argc != 2)
		return pickle_set_result_error_arity(i, 2, argc, argv);
	if (argc == 2) {
		srand(atol(argv[1]));
		return PICKLE_OK;
	}
	return pickle_set_result_integer(i, rand());
}

static int pickleCommandExit(pickle_t *i, const int argc, char **argv, void *pd) {
	assert(!pd);
	UNUSED(pd);
	if (argc != 2 && argc != 1)
		return pickle_set_result_error_arity(i, 2, argc, argv);
	const char *code = argc == 2 ? argv[1] : "0";
	exit(atoi(code));
	return PICKLE_OK;
}

static int pickleCommandGetEnv(pickle_t *i, const int argc, char **argv, void *pd) {
	assert(!pd);
	UNUSED(pd);
	if (argc != 2)
		return pickle_set_result_error_arity(i, 2, argc, argv);
	const char *env = getenv(argv[1]);
	return pickle_set_result_string(i, env ? env : "");
}

static int pickleCommandClock(pickle_t *i, const int argc, char **argv, void *pd) {
	assert(!pd);
	UNUSED(pd);
	if (argc == 1) {
		const long t = (((double)(clock()) / (double)CLOCKS_PER_SEC) * 1000.0);
		return pickle_set_result_integer(i, t);
	}
	if (argc != 2)
		return pickle_set_result_error_arity(i, 2, argc, argv);
	char buf[LINE_SZ] = { 0 };
	time_t rawtime;
	time(&rawtime);
	struct tm *timeinfo = gmtime(&rawtime);
	strftime(buf, sizeof buf, argv[1], timeinfo);
	return pickle_set_result_string(i, buf);
}

static int pickleCommandRaise(pickle_t *i, const int argc, char **argv, void *pd) {
	assert(!pd);
	UNUSED(pd);
	if (argc != 2)
		return pickle_set_result_error_arity(i, 2, argc, argv);
	return pickle_set_result_integer(i, raise(atoi(argv[1])));
}

static void signal_handler(int sig) {
	signal_variable = sig;
}

static int pickleCommandSignal(pickle_t *i, const int argc, char **argv, void *pd) {
	assert(!pd);
	UNUSED(pd);
	if (argc != 1 && argc != 3)
		return pickle_set_result_error_arity(i, 2, argc, argv);
	if (argc == 1) {
		const int sig = signal_variable;
		signal_variable = 0;
		return pickle_set_result_integer(i, sig);
	}
	int r = PICKLE_ERROR, sig = atoi(argv[1]);
	const char *rq = argv[2];
	if (!strcmp(rq, "ignore"))  { r = SIG_ERR == signal(sig, SIG_IGN) ? r : PICKLE_OK; }
	if (!strcmp(rq, "default")) { r = SIG_ERR == signal(sig, SIG_DFL) ? r : PICKLE_OK; }
	if (!strcmp(rq, "catch"))   { r = SIG_ERR == signal(sig, signal_handler) ? r : PICKLE_OK; }
	if (pickle_set_result_integer(i, r == PICKLE_OK) != PICKLE_OK)
		return PICKLE_ERROR;
	return r;
}

static void memory_tracer(void *file, const char *fmt, ...) {
	assert(file);
	assert(fmt);
	FILE *out = file;
	va_list ap;
	va_start(ap, fmt);
	vfprintf(out, fmt, ap);
	va_end(ap);
	fputc('\n', out);
}

static int pickleCommandHeapUsage(pickle_t *i, int argc, char **argv, void *pd) {
	pool_t *p = pd;
	long info = PICKLE_ERROR;
	const char *rq = NULL;

	if (argc > 3)
		return pickle_set_result_error_arity(i, 3, argc, argv);
	if (argc == 1) {
		info = !!p;
		goto done;
	}

	if (!p)
		return pickle_set_result_string(i, "unknown");
	rq = argv[1];
	if (argc == 2) {
		if      (!strcmp(rq, "freed"))    { info = p->freed; }
		else if (!strcmp(rq, "allocs"))   { info = p->allocs;  }
		else if (!strcmp(rq, "reallocs")) { info = p->relocations;  }
		else if (!strcmp(rq, "active"))   { info = p->active;  }
		else if (!strcmp(rq, "max"))      { info = p->max;    }
		else if (!strcmp(rq, "total"))    { info = p->total;  }
		else if (!strcmp(rq, "blocks"))   { info = p->blocks; }
		else if (!strcmp(rq, "arenas"))   { info = p->count; }
		else if (!strcmp(rq, "tron"))     { p->tracer = memory_tracer; p->tracer_arg = stdout; return PICKLE_OK; }
		else if (!strcmp(rq, "troff"))    { p->tracer = NULL; p->tracer_arg = NULL; return PICKLE_OK; }
		else { /* do nothing */ }
	} else if (argc == 3) {
		const int pool = atoi(argv[2]);
		if ((pool >= 0) && (pool < (int)p->count)) {
			block_arena_t *a = p->arenas[pool];
			if      (!strcmp(rq, "arena-size"))   { info = a->freelist.bits; }
			else if (!strcmp(rq, "arena-block"))  { info = a->blocksz; }
			else if (!strcmp(rq, "arena-active")) { info = a->active; }
			else if (!strcmp(rq, "arena-max"))    { info = a->max; }
			else { /* do nothing */ }
		}
	}
done:
	return pickle_set_result_integer(i, info);
}

static char *slurp(FILE *input) {
	assert(input);
	char *r = NULL;
	long pos = 0;
	if (fseek(input, 0, SEEK_END) < 0)
		goto fail;
	pos = ftell(input);
	if (pos < 0)
		goto fail;
	if (fseek(input, 0, SEEK_SET) < 0)
		goto fail;
	if (!(r = malloc(pos + 1)))
		goto fail;
	if (pos != (long)fread(r, 1, pos, input))
		goto fail;
	r[pos] = '\0'; /* Ensure NUL termination */
	rewind(input);
	return r;
fail:
	free(r);
	rewind(input);
	return NULL;
}

static char *slurp_by_name(const char *name) {
	assert(name);
	FILE *input = fopen(name, "rb");
	if (!input)
		return NULL;
	char *r = slurp(input);
	fclose(input);
	return r;
}

/* Retrieve and process those pickles you filed away for safe keeping */
static int file(pickle_t *i, const char *name, FILE *output, int command) {
	assert(i);
	assert(file);
	assert(output);
	errno = 0;
	char *program = slurp_by_name(name);
	if (!program) {
		if (command)
			return pickle_set_result_error(i, "Failed to open file %s (rb): %s\n", name, strerror(errno));
		fprintf(stderr, "Failed to open file %s (rb): %s\n", name, strerror(errno));
		return PICKLE_ERROR;
	}
	const int retcode = pickle_eval(i, program);
	if (retcode != PICKLE_OK)
		if (!command) {
			const char *s = NULL;
			if (pickle_get_result_string(i, &s) != PICKLE_OK) {
				free(program);
				return PICKLE_ERROR;
			}
			fprintf(output, "%s\n", s);
		}
	free(program);
	return retcode;
}

static int pickleCommandSource(pickle_t *i, const int argc, char **argv, void *pd) {
	assert(i);
	assert(file);
	assert(pd);
	if (argc != 2)
		return pickle_set_result_error_arity(i, 2, argc, argv);
	if (file(i, argv[1], pd, 1) < 0)
		return PICKLE_ERROR;
	return PICKLE_OK;
}

static int pickleCommandFile(pickle_t *i, const int argc, char **argv, void *pd) {
	assert(pd);
	FILE *f = (FILE*)pd;
	if (argc == 1)
		return pickle_set_result_integer(i, ftell(f));
	const char *name = argv[0];
	if (argc == 2) {
		if (!strcmp("-clear", argv[1])) {
			clearerr(f);
			return PICKLE_OK;
		}
		if (!strcmp("-flush", argv[1]))
			return pickle_set_result_integer(i, fflush(f));
		if (!strcmp("-close", argv[1])) {
			if (pickle_rename_command(i, name, "") != PICKLE_OK)
				return pickle_set_result_error(i, "unable to remove command: %s", name);
			return pickle_set_result_integer(i, fclose(f));
		}
		if (!strcmp("-getc", argv[1]))
			return pickle_set_result_integer(i, fgetc(f));
		if (!strcmp("-gets", argv[1]))
			return pickleGetLine(i, f);
		if (!strcmp("-rewind", argv[1]))
			return pickle_set_result_integer(i, fseek(f, 0, SEEK_SET));
		if (!strcmp("-error", argv[1]))
			return pickle_set_result_integer(i, ferror(f));
		if (!strcmp("-eof", argv[1]))
			return pickle_set_result_integer(i, feof(f));
	}
	if (argc == 3) {
		if (!strcmp("-putc", argv[1]))
			return pickle_set_result_integer(i, fputc(argv[2][0], f));
		if (!strcmp("-puts", argv[1]))
			return pickle_set_result_integer(i, fputs(argv[2], f));
	}
	if (argc == 4) {
		if (!strcmp("-seek", argv[1])) {
			int whence = -1;
			if (!strcmp("start", argv[3]))
				whence = SEEK_SET;
			if (!strcmp("current", argv[3]))
				whence = SEEK_CUR;
			if (!strcmp("end", argv[3]))
				whence = SEEK_END;
			if (whence < 0)
				return pickle_set_result(i, "invalid whence %s", argv[3]);
			return pickle_set_result_integer(i, fseek(f, atol(argv[2]), whence));
		}
	}
	return pickle_set_result_error(i, "file: unknown subcommand");
}

static int pickleCommandFOpen(pickle_t *i, const int argc, char **argv, void *pd) {
	assert(i);
	assert(argv);
	UNUSED(pd);
	assert(!pd);
	if (argc != 3)
		return pickle_set_result_error_arity(i, 3, argc, argv);
	errno = 0;
	char buf[LINE_SZ] = { 0 };
	FILE *handle = fopen(argv[1], argv[2]);
	if (!handle)
		return pickle_set_result_error(i, "unable to open %s (mode = %s): %s", argv[1], argv[2], strerror(errno));
	snprintf(buf, sizeof buf, "%p", (void*)handle);
	if (pickle_register_command(i, buf, pickleCommandFile, handle) != PICKLE_OK)
		goto fail;
	return pickle_set_result_string(i, buf);
fail:
	if (handle)
		fclose(handle);
	return pickle_set_result_error(i, "open failed");
}

static int pickleCommandErrno(pickle_t *i, const int argc, char **argv, void *pd) {
	assert(i);
	assert(argv);
	assert(!pd);
	UNUSED(pd);
	if (argc == 1)
		return pickle_set_result_integer(i, errno);
	if (argc == 2) {
		if (!strcmp("-string", argv[1])) {
			const char *s = strerror(errno);
			s = s ? s: "unknown";
			return pickle_set_result_string(i, s);
		}
	}
	if (argc == 3) {
		if (!strcmp("-string", argv[1])) {
			const char *s = strerror(atoi(argv[2]));
			s = s ? s: "unknown";
			return pickle_set_result_string(i, s);
		}
		if (!strcmp("-set", argv[1])) {
			errno = atoi(argv[2]);
			return PICKLE_OK;
		}
	}
	return pickle_set_result_error_arity(i, 2, argc, argv);
}

static int pickleCommandFRename(pickle_t *i, const int argc, char **argv, void *pd) {
	assert(i);
	assert(argv);
	assert(!pd);
	UNUSED(pd);
	if (argc == 3) {
		const char *src = argv[1], *dst = argv[2];
		if (!strcmp(dst, ""))
			return pickle_set_result_integer(i, remove(src));
		return pickle_set_result_integer(i, rename(src, dst));
	}
	return pickle_set_result_error_arity(i, 3, argc, argv);
}

static int register_custom_commands(pickle_t *i, pool_t *p, int prompt) {
	assert(i);
	const pickle_register_command_t commands[] = {
		{ "system",   pickleCommandSystem,    NULL },
		{ "exit",     pickleCommandExit,      NULL },
		{ "quit",     pickleCommandExit,      NULL },
		{ "bye",      pickleCommandExit,      NULL }, /* hold over from Forth */
		{ "getenv",   pickleCommandGetEnv,    NULL },
		{ "random",   pickleCommandRandom,    NULL },
		{ "clock",    pickleCommandClock,     NULL },
		{ "raise",    pickleCommandRaise,     NULL },
		{ "signal",   pickleCommandSignal,    NULL },
		{ "source",   pickleCommandSource,    stdout },
		{ "heap",     pickleCommandHeapUsage, p },
		{ "fopen",    pickleCommandFOpen,     NULL },
		{ "frename",  pickleCommandFRename,   NULL },
		{ "stdin",    pickleCommandFile,      stdin },
		{ "stdout",   pickleCommandFile,      stdout },
		{ "stderr",   pickleCommandFile,      stderr },
		{ "errno",    pickleCommandErrno,     NULL },
	};
	if (pickle_set_var_string(i, "prompt", prompt ? "pickle> " : "") != PICKLE_OK)
		return PICKLE_ERROR;
	for (size_t j = 0; j < sizeof(commands)/sizeof(commands[0]); j++)
		if (pickle_register_command(i, commands[j].name, commands[j].func, commands[j].data) != 0)
			return PICKLE_ERROR;
	return PICKLE_OK;
}

/* An interactive pickle - the things you can do with it! */
static int interactive(pickle_t *i, FILE *input, FILE *output) { /* NB. This could be rewritten as a script now */
	assert(i);
	assert(input);
	assert(output);
	for (;;) {
		char clibuf[LINE_SZ] = { 0 };
		const char *prompt = NULL;
		pickle_get_var_string(i, "prompt", &prompt);
		prompt = prompt ? prompt : ""; /* NB. We could evaluate the prompt for more flexibility. */
		fputs(prompt, output);
		fflush(output);
		if (!fgets(clibuf, sizeof clibuf, input))
			return PICKLE_OK;
		const int retcode = pickle_eval(i, clibuf);
		const char *s = NULL;
		if (pickle_get_result_string(i, &s) != PICKLE_OK)
			return PICKLE_ERROR;
		if (s[0] != '\0')
			fprintf(output, "[%d] %s\n", retcode, s);
	}
	return PICKLE_OK;
}

/* Adapted from: <https://stackoverflow.com/questions/10404448> */
static int pickle_getopt(pickle_getopt_t *opt, const int argc, char *const argv[], const char *fmt) {
	assert(opt);
	assert(fmt);
	assert(argv);
	/* enum { BADARG_E = ':', BADCH_E = '?' }; */
	enum { BADARG_E = PICKLE_ERROR, BADCH_E = PICKLE_ERROR };

	if (!(opt->init)) {
		opt->place = ""; /* option letter processing */
		opt->init  = 1;
		opt->index = 1;
	}

	if (opt->reset || !*opt->place) { /* update scanning pointer */
		opt->reset = 0;
		if (opt->index >= argc || *(opt->place = argv[opt->index]) != '-') {
			opt->place = "";
			return PICKLE_RETURN;
		}
		if (opt->place[1] && *++opt->place == '-') { /* found "--" */
			opt->index++;
			opt->place = "";
			return PICKLE_RETURN;
		}
	}

	const char *oli = NULL; /* option letter list index */
	if ((opt->option = *opt->place++) == ':' || !(oli = strchr(fmt, opt->option))) { /* option letter okay? */
		 /* if the user didn't specify '-' as an option, assume it means -1.  */
		if (opt->option == '-')
			return PICKLE_RETURN;
		if (!*opt->place)
			opt->index++;
		/*if (opt->error && *fmt != ':')
			(void)fprintf(stderr, "illegal option -- %c\n", opt->option);*/
		return BADCH_E;
	}

	if (*++oli != ':') { /* don't need argument */
		opt->arg = NULL;
		if (!*opt->place)
			opt->index++;
	} else {  /* need an argument */
		if (*opt->place) { /* no white space */
			opt->arg = opt->place;
		} else if (argc <= ++opt->index) { /* no arg */
			opt->place = "";
			if (*fmt == ':')
				return BADARG_E;
			/*if (opt->error)
				(void)fprintf(stderr, "option requires an argument -- %c\n", opt->option);*/
			return BADCH_E;
		} else	{ /* white space */
			opt->arg = argv[opt->index];
		}
		opt->place = "";
		opt->index++;
	}
	return opt->option; /* dump back option letter */
}

static int picolTestGetOpt(void) {
	pickle_getopt_t opt = { .init = 0 };
	char *argv[] = {
		"./program",
		"-h",
		"-f",
		"argument-to-f",
		"-c",
		"file",
	};
	const int argc = sizeof(argv) / sizeof(argv[0]);
	char *argument_to_f = NULL;
	int ch = 0, r = 0, result = 0;
	while ((ch = pickle_getopt(&opt, argc, argv, "hf:c")) != PICKLE_RETURN) {
		switch (ch) {
		case 'h': if (result & 1) r = -1; result |= 1; break;
		case 'f': if (result & 2) r = -2; result |= 2; argument_to_f = opt.arg; break;
		case 'c': if (result & 4) r = -4; result |= 4; break;
		default:
			r = -8;
		}
	}
	r += result == 7 ? 0 : -8;
	if (argument_to_f)
		r += !strcmp("argument-to-f", argument_to_f) ? 0 : -16;
	else
		r += -32;
	return r;
}

static int tests(void) {
	typedef int (*test_t)(void);
	static const test_t ts[] = { block_tests, pickle_tests, picolTestGetOpt, NULL };
	int r = 0;
	for (size_t i = 0; ts[i]; i++)
		 if (ts[i]() != 0)
			 r = -1;
	return r;
}

static void help(FILE *output, const char *arg0) {
	assert(output);
	assert(arg0);
	const unsigned long v = pickle_version();
	const unsigned q = (v >> 24) & 0xFFu;
	const unsigned x = (v >> 16) & 0xFFu;
	const unsigned y = (v >>  8) & 0xFFu;
	const unsigned z = (v >>  0) & 0xFFu;

	static const char *msg = "\
Usage:      %s file.tcl...\n\n\
Pickle:     A tiny TCL like language derived/copied from 'picol'\n\
License:    BSD (Antirez for original picol, Richard Howe for modifications)\n\
Repository: https://github.com/howerj/pickle\n\
Version:    %u.%u.%u (%u)\n\n\
Options:\n\
\n\
\t--,\tstop processing command line arguments\n\
\t-h,\tdisplay this help message and exit\n\
\t-t,\trun built in self tests and exit (return code 0 is success)\n\
\t-a,\tuse custom block allocator, for testing purposes\n\
\t-A,\tenable debugging of the custom allocator, implies '-a'\n\
\t-s,\tsuppress prompt printing\n\
\n\
If no arguments are given then input is taken from stdin. Otherwise\n\
they are treated as scripts to execute. Maximum length of an input \n\
line is is %d bytes. There are no configuration files or environment\n\
variables needed by the interpreter. Non zero return codes indicate\n\
failure.\n";
	fprintf(output, msg, arg0, x, y, z, q, LINE_SZ);
}

static void cleanup(void) {
	static int cleaned = 0;
	if (cleaned)
		return;
	cleaned = 1;
	pickle_delete(interp);
	if (use_custom_allocator) {
		use_custom_allocator = 0;
		pool_delete(block_allocator.arena);
	}
}

int main(int argc, char **argv) {
	pickle_getopt_t opt = { .init = 0 };
	int r = 0, prompt_on = 1, memory_debug = 0, ch;

	static const pool_specification_t specs[] = {
		{ 8,   512 }, /* most allocations are quite small */
		{ 16,  256 },
		{ 32,  128 },
		{ 64,   64 },
		{ 128,  32 },
		{ 256,  16 },
		{ 512,   8 }, /* maximum string length is bounded by this */
	};

	if (atexit(cleanup)) {
		fprintf(stderr, "atexit failed\n");
		return -1;
	}

	while ((ch = pickle_getopt(&opt, argc, argv, "hatsA")) != PICKLE_RETURN) {
		switch (ch) {
		case 'A': memory_debug = 1; /* fall through */
		case 'a': use_custom_allocator = 1; break;
		case 's': prompt_on = 0; break;
		case 'h': help(stdout, argv[0]); return 0;
		case 't': return tests();
		default: help(stderr, argv[0]); return -1;
		}
	}

	if (use_custom_allocator) {
		pool_t *p = pool_new(sizeof(specs) / sizeof(specs[0]), &specs[0]);
		if (!(block_allocator.arena = p)) {
			fputs("memory pool allocation failure\n", stderr);
			return EXIT_FAILURE;
		} else {
			if (memory_debug) {
				p->tracer     = memory_tracer;
				p->tracer_arg = stdout;
			}
		}
	}

	if ((r = pickle_new(&interp, use_custom_allocator ? &block_allocator : NULL)) != PICKLE_OK)
		goto end;
	if ((r = register_custom_commands(interp, block_allocator.arena, prompt_on)) < 0)
		goto end;

	static const char *ns[] = {
		"proc puts {x} { stdout -puts $x; stdout -puts \"\n\" }",
		"proc error {x} { stderr -puts $x; stderr -puts \"\n\"; return $x -1 }",
		"proc gets {}  { stdin -gets }",
		"proc putch {c} { stdout -putc $c }",
		"proc getch {} { stdin -getc }",
	};

	for (size_t i = 0; i < NELEM(ns); i++)
		if ((r = pickle_eval(interp, ns[i])) != PICKLE_OK)
			goto end;

	pickle_set_argv(interp, argc, argv);
	if (argc == opt.index) {
		r = interactive(interp, stdin, stdout);
	} else {
		for (int j = opt.index; j < argc; j++) {
			if ((r = file(interp, argv[j], stdout, 0)) != PICKLE_OK)
				break;
		}
	}
end:
	cleanup();
	return r;
}

