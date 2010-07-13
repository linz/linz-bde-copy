// Crude windows implementation of directory functions

#include <io.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <malloc.h>

#include "bde_copy_funcs.h"

struct dir_data
{
	intptr_t handle;
	_finddata_t data;
	int status;
};

void *dir_open( char *dirname )
{
	if( ! dirname ) return 0;
	char *fspec = (char *) _alloca(strlen(dirname)+3);
	strcpy(fspec,dirname);
	strcat(fspec,"/*");
	dir_data *d = new dir_data;
	d->handle = _findfirst(fspec, &(d->data));
	if( d->handle == -1 ) { delete d; d = 0; return 0; }
	d->status = 1;
	return d;
}

char *dir_next( void *dir, int type )
{
	dir_data *d = (dir_data *) dir;
	while( 1 )
	{
		if( d->status == 0 )
		{
			int sts = _findnext(d->handle,&(d->data));
			if( sts != 0 ) d->status = 2;
		}
		if( d->status == 2 ) return 0;
		d->status = 0;
		if( strcmp(d->data.name,".") == 0 || strcmp(d->data.name,"..") == 0 ) continue;
		unsigned int isdir = d->data.attrib & _A_SUBDIR;
		if( type == dt_file && isdir ) continue;
		if( type == dt_dir && ! isdir ) continue;
		break;
	}
	return d->data.name;
}

void dir_close( void *dir )
{
	dir_data *d = (dir_data *) dir;
	_findclose(d->handle);
	delete(d);
}

bool file_exists( char *fname )
{
	FILE *f = fopen(fname,"r");
	if( f ) fclose(f);
	return f != 0;
}