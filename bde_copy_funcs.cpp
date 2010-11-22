/*
Copyright (C) 2010 LAND INFORMATION NEW ZEALAND

This program is released under the terms of the license contained
in the file LICENSE.
*/

#include <stdio.h>

#include "bde_copy_funcs.h"


bool file_exists( char *fname )
{
    FILE *f = fopen(fname,"r");
    if( f ) fclose(f);
    return f != 0;
}
