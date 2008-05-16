
#include <stdio.h>
#include <string.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static int usage (void);

static int
command_version (int argc, const char *argv[])
{
	printf  ("low 0.0.1\n");
	return 0;
}

static int
command_help (int argc, const char *argv[])
{
	usage ();
	return 0;
}

static int
NOT_IMPLEMENTED (int argc, const char *argv[])
{
	printf ("This function is not yet implemented\n");

	return 1;
}

typedef struct _SubCommand {
	char *name;
	char *summary;
	int (*func) (int argc, const char *argv[]);
} SubCommand;

const SubCommand commands[] = {
	{ "clean", "Remove cached data", NOT_IMPLEMENTED },
	{ "info", "Display package details", NOT_IMPLEMENTED },
	{ "version", "Display version information", command_version },
	{ "help", "Display a helpful usage message", command_help }
};

static int
usage (void)
{
	int i;

	printf ("low: a yum-like package manager\n");
	for (i = 0; i < ARRAY_SIZE (commands); i++) {
		printf ("  %-20s%s\n", commands[i].name, commands[i].summary);
	}

	return 1;
}

int
main (int argc, const char *argv[])
{
	int i;

	if (argc < 2) {
		return usage ();
	}

	for (i = 0; i < ARRAY_SIZE (commands); i++) {
		if (!strcmp (argv[1], commands[i].name)) {
			return commands[i].func (argc, argv);
		}
	}
	printf ("Unknown command: %s\n", argv[1]);

	return usage ();
}
