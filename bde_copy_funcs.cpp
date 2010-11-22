// Crude windows implementation of directory functions

#include <stdio.h>

#include "bde_copy_funcs.h"


bool file_exists( char *fname )
{
	FILE *f = fopen(fname,"r");
	if( f ) fclose(f);
	return f != 0;
}
