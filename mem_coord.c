#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <limits.h>
#include <errno.h>
#ifdef HP_UX
#include <dl.h>
#endif

#include "mem_coord.h"

static int i_have_addr2line = 0;
static char prog_path[PATH_MAX];

static void print_not_found(void)
{
	fprintf(stderr, "Could not find addr2line.\n"
			"Check http://www.gnu.org/software/binutils/\n"
			"Coordinate(file, line)  will not printed.\n");
	return;
}

static void get_coord_line(char *line, char **coord)
{
	char *p = line;
	while (*line) {
		if (*line == '/')
			p = line;
		line++;
	}
	p++;

	*coord = malloc(strlen(p) + 1);
	if (*coord == NULL) {
		fprintf(stderr, "malloc() failed. %d %s\n",
			errno, strerror(errno));
		exit(1);
	}
	strcpy(*coord, p);
	return;
}

static void get_func_line(char *line, char **func)
{
	*func = malloc(strlen(line) + 1);
	if (*func == NULL) {
		fprintf(stderr, "malloc() failed. %d %s\n",
			errno, strerror(errno));
		exit(1);
	}
	strcpy(*func, line);
	return;
}

int get_exec_path(char *exec_path, int len)
{
	int ret;
#ifdef LINUX
	ret = readlink("/proc/self/exe", exec_path, len);
	if (ret < 0) {
		fprintf(stderr, "readlink() failed. %d %s\n",
			errno, strerror(errno));
		return -1;
	}
	exec_path[len - 1] = '\0';
	return 0;
#elif SUNOS
	const char *cmdp;
	char cwd[BUFSIZ], *cwdp;
	char *homep = getenv("HOME");

	cmdp = getexecname();
	if (cmdp != NULL) {
		if (homep != NULL) {
			if (strstr(cmdp, homep) == NULL) {
				cwdp = getcwd(cwd, sizeof(cwd));
				if (cwdp == NULL) {
					fprintf(stderr,
						"getcwd() failed. %d %s\n",
						errno, strerror(errno));
					return -1;
				}
				cwd[sizeof(cwd) - 1] = '\0';
				snprintf(exec_path, len,
					"%s/%s", cwd, cmdp);
			} else {
				strncpy(exec_path, cmdp, len);
				exec_path[len - 1] = '\0';
			}
		} else {
			fprintf(stderr, "warning: couldn't get $HOME\n");
			snprintf(exec_path, len,
				"./%s\n", cmdp);
		}
	} else {
		fprintf(stderr, "getexecname() failed. %d %s\n",
			errno, strerror(errno));
		return -1;
	}
	return 0;
#elif HP_UX
	struct shl_descriptor desc;

	ret = shl_get_r(0, &desc);
	if (ret < 0) {
		fprintf(stderr, "shl_get_r() failed. %d %s\n",
			errno, strerror(errno));
		return -1;
	}
	strncpy(exec_path, desc.filename, len);
	exec_path[len - 1] = '\0';
	return 0;
#else
	return -1;
#endif
}

void check_addr2line(void)
{
	static const char *cmd = "which addr2line";
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
			fprintf(stderr, "> %s ..ok\n", prog_path);
		} else {
			print_not_found();
		}
	}
	pclose(pp);
	return;
}

/*
 * IN@addr: address
 * IN@exec: executable path
 * OUT@func: function
 * OUT@coord: coordnate (file:line)
 */
#define DEFAULT_LEN 8 
void get_coordinate(unsigned long addr, const char *exec, 
			char **func, char **coord)
{
	char cmd[BUFSIZ];
	char line[BUFSIZ];
	FILE *pp;
	int len;

	*func = NULL;
	*coord = NULL;

	if (i_have_addr2line) {
		snprintf(cmd, sizeof(cmd),
			"%s %#lx -f -e %s", prog_path, addr, exec);
		pp = popen(cmd, "r");
		if (pp != NULL) {
			while (fgets(line, sizeof(line), pp)) {
				line[strlen(line) - 1] = '\0';
				if ((strstr(line, "addr2line") != NULL) ||
					(strstr(line, "??") != NULL)) {
					pclose(pp);
					goto dontknow;
				}
				if (strstr(line, ":") != NULL) {
					get_coord_line(line, coord);
				} else {
					get_func_line(line, func);
				}
			}
			pclose(pp);
			return;
		} else {
			goto dontknow;
		}
	} else {
dontknow:
		*func = malloc(DEFAULT_LEN);
		if (*func != NULL) {
			strcpy(*func, "??");
		} else {
			fprintf(stderr, "malloc() failed. %d %s\n",
				errno, strerror(errno));
			exit(1);
		}
		*coord = malloc(DEFAULT_LEN);
		if (*coord != NULL) {
			strcpy(*coord, "??:0");
		} else {
			fprintf(stderr, "malloc() failed. %d %s\n",
				errno, strerror(errno));
			exit(1);
		}
		return;
	}
}
