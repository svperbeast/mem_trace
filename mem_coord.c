#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <limits.h>
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

static int get_exec_path(char *exec_path, int len)
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

void get_exec_name(char *exec_name)
{
	int pid;

	pid = getpid();
	printf("pid: %d\n", pid);
	return;
}

/*
 * IN@addr: address
 * OUT@file: file name
 * OUT@line: line number
 */
void get_coordinate(unsigned long addr, char *file, char *line)
{
	char cmd[BUFSIZ];
	char lind[BUFSIZ];

	snprintf(cmd, sizeof(cmd),
		"%s %lx -e \n", prog_path, addr);
}
