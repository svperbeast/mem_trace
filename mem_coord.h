#ifndef COORD_INCLUDED
#define COORD_INCLUDED

void check_addr2line(void);

/*
 * IN@addr: address
 * OUT@file: file name
 * OUT@line: line number
 */
void get_coordinate(unsigned long addr, char *file, char *line);

#endif /* !COORD_INCLUDED */
