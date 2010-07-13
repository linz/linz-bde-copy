#ifndef _BDE_COPY_GZIP_H
#define _BDE_COPY_GZIP_H

#include "bde_copy_utils.h"


class gzipbuff : public readbuff
{
public:
	gzipbuff( char *name, int zipbuffsize = 0 );
	~gzipbuff();

protected:
	virtual int fill();
private:
	void *gz;
};


class gzip_data_writer : public data_writer
{
public:
	static data_writer *open( char *fname, bool append = false, int zipbuffsize=0 );
	virtual ~gzip_data_writer();
	virtual bool write( const void *buffer, int length );
private:
	gzip_data_writer( void *gz ) : gz(gz){};
	void *gz;
};
#endif
