#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int func(void)
{
	char *p = strdup("leak");
	return 0;
}

int main(int argc, char **argv)
{
	int i;
	printf("test start\n");
#if 0
	void *p;
	for (i = 0; i < 10000; i++) {
		p = malloc(32);
		free(p);
	}
	return 0;
#endif
	func();
	printf("test done\n");
	return 0;
}
