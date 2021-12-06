/***************************************************************************


 Copyright 2011 Crown copyright (c)
 Land Information New Zealand and the New Zealand Government.
 All rights reserved

 This program is released under the terms of the new BSD license. See 
 the LICENSE file for more information.
****************************************************************************/

#include <stdio.h>

#include "bde_copy_funcs.h"


bool file_exists( char *fname )
{
    FILE *f = fopen(fname,"r");
    if( f ) fclose(f);
    return f != 0;
}
