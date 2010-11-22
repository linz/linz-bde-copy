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
        if( zipbuffsize == 0 ) zipbuffsize = filebuffsize;
        gzbuffer(gz,zipbuffsize);
    }
    else 
    {
        setstatus(OPEN_ERROR);
    }
}

gzipbuff::~gzipbuff()
{
    if( gz ) gzclose(gz);
}

int gzipbuff::fill()
{
    if( ! gz ) return 0;
    int nch = gzread(gz, buffer,bufsize);
    return nch < 0 ? 0 : nch;
}


data_writer *gzip_data_writer::open( char *fname, bool append, int zipbuffsize )
{
    if( append && file_exists(fname)) return 0;
    void *gz = gzopen(fname,"wb" );
    if( ! gz ) return 0;
    if( zipbuffsize == 0 ) zipbuffsize = readbuff::filebuffsize;
    gzbuffer(gz,zipbuffsize);
    return new gzip_data_writer(gz);
}

gzip_data_writer::~gzip_data_writer()
{
    gzclose(gz);
}

bool gzip_data_writer::write( const void *buffer, int length )
{
    if( length )
    {
        empty = false;
        return gzwrite(gz,buffer,length) == length;
    }
    return true;
}
