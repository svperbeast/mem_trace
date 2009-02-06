#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mem_coord.h"

static int i_have_addr2line = 0;
static char prog_path[256];

static void print_not_found(void)
{
	fprintf(stderr, "Could not find addr2line.\n"
			"Check http://www.gnu.org/software/binutils/\n"
			"Coordinate(file, line)  will not printed.\n");
	return;
}

void check_addr2line(void)
{
	static const char *cmd = "which addr2line_bad";
	FILE *pp;
	char line[BUFSIZ];

	prog_path[0] = '\0';

	pp = popen(cmd, "r");
	if (pp == NULL) {
		print_not_found();
	} else {
		while (fgets(line, sizeof(line), pp)) {
			line[strlen(line) - 1] = '\0';
			if (strstr(line, "addr2line")) {
				strncpy(prog_path, line, sizeof(prog_path));
				prog_path[sizeof(prog_path) - 1] = '\0';
			}
		}
		if (prog_path[0] != '\0') {
			i_have_addr2line = 1;
			fprintf(stderr, "%s ..ok\n", prog_path);
		} else {
			print_not_found();
		}
	}
	return;
}

/*
 * IN@addr: address
 * OUT@file: file name
 * OUT@line: line number
 */
void get_coordinate(unsigned long addr, char *file, char *line)
{
}
