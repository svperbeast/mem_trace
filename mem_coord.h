#ifndef COORD_INCLUDED
#define COORD_INCLUDED

extern void check_addr2line(void);
extern int get_exec_path(char *exec_path, int len);

/*
 * IN@addr: address
 * IN@exec: executable path
 * OUT@func: function
 * OUT@coord: coordnate (file:line)
 */
extern void get_coordinate(unsigned long addr, const char *exec, 
				char **func, char **coord);

#endif /* !COORD_INCLUDED */
