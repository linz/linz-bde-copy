/***************************************************************************


 Copyright 2011 Crown copyright (c)
 Land Information New Zealand and the New Zealand Government.
 All rights reserved

 This program is released under the terms of the new BSD license. See 
 the LICENSE file for more information.
****************************************************************************/

#include "bde_copy_gzip.h"
#include "bde_copy_funcs.h"
#include "zlib.h"

gzipbuff::gzipbuff( char *name, int zipbuffsize ) : readbuff( name, filebuffsize )
{
    gzFile p = gzopen(name,"rb");
    gz = (void *)p;
    if( gz ) 
    {
        setstatus(OK);
#if ZLIB_VERNUM >= 0x1235
        if( zipbuffsize == 0 ) zipbuffsize = filebuffsize;
        gzbuffer(p,zipbuffsize);
#endif
    }
    else 
    {
        setstatus(OPEN_ERROR);
    }
}

gzipbuff::~gzipbuff()
{
    if( gz ) gzclose(static_cast<gzFile>(gz));
}

int gzipbuff::fill()
{
    if( ! gz ) return 0;
    int nch = gzread(static_cast<gzFile>(gz), buffer,bufsize);
    return nch < 0 ? 0 : nch;
}


data_writer *gzip_data_writer::open( char *fname, bool append, int zipbuffsize )
{
    if( append && file_exists(fname)) return 0;
    void *gz = gzopen(fname,"wb" );
    if( ! gz ) return 0;
#if ZLIB_VERNUM >= 0x1235
    if( zipbuffsize == 0 ) zipbuffsize = readbuff::filebuffsize;
    gzbuffer(static_cast<gzFile>(gz),zipbuffsize);
#endif
    return new gzip_data_writer(gz);
}

gzip_data_writer::~gzip_data_writer()
{
    gzclose(static_cast<gzFile>(gz));
}

bool gzip_data_writer::write( const void *buffer, int length )
{
    if( length )
    {
        empty = false;
        return gzwrite(static_cast<gzFile>(gz),buffer,length) == length;
    }
    return true;
}
