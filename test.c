#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int func(void)
{
	char *p = strdup("fucked up");
	return 0;
}

int main(int argc, char **argv)
{
	int i;
#if 0
	void *p;
	for (i = 0; i < 10; i++) {
		p = malloc(32);
		free(p);
	}
	return 0;
#endif
	func();
	return 0;
}
