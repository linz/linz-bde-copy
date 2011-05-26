/***************************************************************************
 $Id$

 Copyright 2011 Crown copyright (c)
 Land Information New Zealand and the New Zealand Government.
 All rights reserved

 This program is released under the terms of the new BSD license. See 
 the LICENSE file for more information.
****************************************************************************/

#ifndef _BDE_COPY_UTILS_H
#define _BDE_COPY_UTILS_H

#include <stdio.h>

#if defined (_WIN32) || defined (_MSC_VER)
#define DIR_SEPARATOR '\\'
#else
#define DIR_SEPARATOR '/'
#endif

#if !defined(_WIN32) && !defined(_MSC_VER)
#ifndef _stricmp
#define _stricmp strcasecmp
#endif

#ifndef stricmp
#define stricmp strcasecmp
#endif

#ifndef _strdup
#define _strdup strdup
#endif

#ifndef _strnicmp
#define _strnicmp strncasecmp
#endif

char* get_image_path();

#ifndef _strlwr
char* _strlwr ( char* __s);
#endif

#endif

enum err_type { et_date_range=0, et_invalid_char, et_column_count, et_file_size, et_count };
enum err_severity { es_ignore, es_warning, es_error, es_fatal };

typedef void (*field_error_func)(err_type type, char *name, char *message);

#if defined(_WIN32)
char *basename (const char *name);
#endif
char *catdir (const char *dir1, const char *dir2);
char *clean_string( char *str );

#define Singleton 1
#if Singleton
#define Static static
#else
#define Static
#endif

class data_writer
{
public:
	virtual ~data_writer(){};
	virtual bool write( const void *buffer, int length ) = 0;
	bool isempty(){ return empty;}
protected:
	data_writer(){ empty = true; }
	bool empty;
};

class file_data_writer : public data_writer
{
public:
	static data_writer *open( char *fname, bool append = false );
	virtual ~file_data_writer();
	virtual bool write( const void *buffer, int length );
private:
	file_data_writer( FILE *f ) : f(f){};
	FILE *f;
};

class readbuff
{
public:
	enum bufStatus { FEOF=-1, OK=0, OPEN_ERROR=-2, EOL=-3, BADEOL=-4 };
	readbuff(char *name, int size=16384);
	virtual ~readbuff();
	Static int ateof(){ if( getc() == FEOF ) return 1;  cp--; return 0; }
	Static int getc(){ return cp < ep ? *cp++ : refill(); }
	Static long loc() { return nread + (ep-cp); }
	Static int status() { return eof; }
	Static int getline(char *buffer,int bufsize);
	Static int endline();
	Static char *name(){ return bname;}
	static int filebuffsize;
protected:
	virtual int fill() = 0;
	void setstatus( int status ){ eof = status; }
	Static unsigned char *buffer;
	Static int bufsize;
private:
	Static unsigned char *cp;
	Static unsigned char *ep;
	Static unsigned long nread;
	Static int eof;
	Static int refill();
	Static char *bname;
#if Singleton
	Static readbuff *instance;
#endif
};


class filebuff : public readbuff
{
public:
	filebuff( char *name );
	~filebuff();
protected:
	virtual int fill();
private:
	FILE *fh;
};


class buffer
{
public:
	buffer(int bufsize=256);
	buffer(char *str );
	~buffer();
	int add( unsigned char c ){ if( cp >= ep ) expand(); *cp++ = c; return 1;}
	int add( char *c ){ while(*c){ add(*c); c++; }; return 1; }
	int len(){ return cp - sp; }
	unsigned char *data(){ return sp; }
	void setstring( char *str );
	void setchars( char *source );
	void reset(){ cp = sp; }
	char *str(){ *cp=0; return (char *) sp; }
	int load( readbuff *buff, unsigned char terminator='|', unsigned char escape='\\' ); 
	bool write( data_writer *out );
	static bool keep_escape;
	static char *parse_char( char *source, unsigned char *value );
protected:
private:
	void init(int size );
	void expand();
	unsigned char *sp;
	unsigned char *ep;
	unsigned char *cp;
	int size;
};


enum field_type{ ft_number=0, ft_text, ft_date, ft_datetime, ft_geometry, ft_count };

class bde_field : public buffer
{
public:
	bde_field( char *name, field_type type );
	virtual ~bde_field();
	const char *name(){ return _name; }
	field_type type(){ return _type; }
	bool selected(){ return _selected; }
	void select( bool value ){ _selected = value; }
	virtual bool write_field( data_writer *out );
	bool write_header( data_writer *out );
	static void set_default_delim( char *start, char *end );
	static void set_error_func( field_error_func func ){ error_func = func; }
protected:
	void write_error( err_type type, char *message );
	static field_error_func error_func;
	static buffer *default_start_delim;
	static buffer *default_end_delim;
	buffer *start_delim;
	buffer *end_delim;
private:
	char *_name;
	field_type _type;
	bool _selected;
};


class number_field : public bde_field
{
public:
	number_field( char *name );
	static void set_delim( char *start, char *end );
private:
	static buffer *fld_start_delim;
	static buffer *fld_end_delim;
};

class replace_def
{
public:
	replace_def();
	~replace_def();
	bool replaced(){ return replace; }
	void set_replace( char *input_str, char *output_str, char *error_message );
	int apply( unsigned char **cp, char **error_message, data_writer *out )
	{
		bool applied = false;
		(*error_message) = 0;
		return apply(cp, error_message, out, applied);
	};
private:
	int apply(unsigned char **cp, char **error_message, data_writer *out, bool &applied);
	bool replace;
	buffer *chars;
	char *message;
	replace_def *nextchar;
};

class text_field : public bde_field
{
public:
	text_field( char *name );
	virtual bool write_field( data_writer *out );
	static void set_delim( char *start, char *end );
	static int set_output_char( char *input_chr, char *output_str, char *error_message );
private:
	static buffer *fld_start_delim;
	static buffer *fld_end_delim;
	static replace_def replace[256];
};

class date_field : public bde_field
{
public:
	date_field( char *name );
	int year();
	static void set_delim( char *start, char *end );
private:
	static buffer *fld_start_delim;
	static buffer *fld_end_delim;
};

class datetime_field : public bde_field
{
public:
	datetime_field( char *name );
	int year();
	static void set_delim( char *start, char *end );
private:
	static buffer *fld_start_delim;
	static buffer *fld_end_delim;
};

class geometry_field : public bde_field
{
public:
	geometry_field( char *name );
	virtual bool write_field( data_writer *out );
	static void set_delim( char *start, char *end );
	static void set_wkt_prefix( char *prefix );
	static void set_lon_offset( double offset );
private:
	static int write_offset_geom( char *sp, data_writer *out );
	static buffer *fld_start_delim;
	static buffer *fld_end_delim;
	static double lon_offset;
	static int ilon_offset;
	static bool int_offset;
	static buffer *wkt_prefix;
	static bool keep_prefix;
};


#endif
