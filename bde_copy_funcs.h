#ifndef BDE_COPY_FUNCS_H
#define BDE_COPY_FUNCS_H

enum {dt_any=0, dt_file=1, dt_dir=2};

void *dir_open( char *dirname );
char *dir_next( void *dir, int type );
void dir_close( void *dir );
bool file_exists( char *fname );

#endif