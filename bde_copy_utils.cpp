#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "bde_copy_utils.h"

#if !defined(_WIN32) && !defined(_MSC_VER)
#include <sys/types.h>
#include <unistd.h>

char* get_image_path()
{
    char* path;
    char _link[20];
    char buf[10];
    pid_t pid = getpid();
    sprintf( buf,"%d", pid );
    strcpy( _link, "/proc/" );
    strcat( _link, buf );
#if defined(__linux) || defined(linux)
    strcat( _link, "/exe" );
#endif
#if defined(sun) || defined(__sun)
    strcat( _link, "/path/a.out" );
#endif
#if defined(__bsdi__)
    strcat( _link, "/file" );
#endif
    char proc[512];
    ssize_t len = readlink( _link, proc, 512);
    if ( len != -1 )
    {
      proc[len] = '\0';
      path = new char[strlen( proc )];
      path = strcpy( path, proc );
    }
    return path;
}

#ifndef _strlwr
char* _strlwr ( char* string)
{
    char *cp;
    for ( cp=string; *cp; ++cp )
    {
        if ( 'A' <= *cp && *cp <= 'Z' )
          *cp += 'a' - 'A';
    }
    return(string);
}
#endif

#endif

char *clean_string(char *cp)
{
    char *sp=0;
    char *ep=0;
    for( ; *cp; cp++ )
    { 
        if( isspace(*cp)) *cp=' '; 
        else 
        {
            if( ! sp ) { sp = cp; }
            ep = cp+1;
        }
    }
    if( ep ) *ep = 0;
    return sp ? sp : cp;
}

#if Singleton
unsigned char *readbuff::buffer = 0;
unsigned char *readbuff::cp = 0;
unsigned char *readbuff::ep = 0;
int readbuff::eof = 0;
int readbuff::bufsize = 0;
unsigned long readbuff::nread = 0;
char *readbuff::bname=0;
readbuff *readbuff::instance = 0;
#endif

int readbuff::filebuffsize=1048576;

readbuff::readbuff(char *name, int size)
{
    cp = ep = buffer = new unsigned char[size];
    eof = 0;
    bufsize = size;
	nread = 0;
	bname = strdup(name);
#if Singleton
    if( instance )
    {
        fprintf(stderr,"Invalid constructor to singleton readbuff\n");
        exit(2);
    }
    instance = this;
#endif
}

readbuff::~readbuff()
{
	free(bname);
    delete [] buffer;
#if Singleton
    instance = 0;
#endif
}

int readbuff::refill()
{
    if( eof ) return FEOF;
#if Singleton
    int nfilled = instance->fill();
#else
    int nfilled = fill();
#endif
    if( ! nfilled ) { eof = FEOF; return FEOF; }
    ep = buffer + nfilled;
    cp = buffer;
    nread += nfilled;
    return *cp++;
}

int readbuff::getline(char *buffer, int bufsize)
{
    int ok = FEOF;
    char *b = buffer;
    char *eb = buffer+bufsize-1;
    int c;
    while((c = getc()) != FEOF)
    {
        ok = OK;
        if( c == '\r' ) continue;
        if( c == '\n' ) break;
        *b++ = (char) c;
        if( b >= eb ) break;
    }
    *b++ = 0;
    return ok;
}

int readbuff::endline()
{
    int ok = EOL;

    int c;
    while( (c = getc()) != FEOF)
    {
        if( c == '\n' ) return ok;
        if( ! isspace(c)) ok = BADEOL;
    }
    return FEOF;
}


////////////////////////////////////////////////////////////////////////////////


filebuff::filebuff( char *name ) : readbuff( name, filebuffsize )
{
    fh = fopen(name,"rb");
    if( fh ) setstatus(OK); else setstatus(OPEN_ERROR);
}

filebuff::~filebuff()
{
    if ( fh != NULL )
      fclose(fh);
}

int filebuff::fill()
{
    return fread(buffer,1,bufsize,fh);
}


////////////////////////////////////////////////////////////////////////////////
// Expandable buffer designed for simplicity and speed

bool buffer::keep_escape = false;

buffer::buffer( int bufsize )
{
    init(bufsize);
}

buffer::buffer( char *str )
{
    size = str ? strlen(str)+1 : 32;
    init(size);
    if( str ) setstring( str );
}

void buffer::init( int bufsize )
{
    size = bufsize;
    if(size < 32 ) size = 32;
    sp = new unsigned char [size];
    ep = sp+(size-1); // leave room for null terminator
    cp = sp;
}

void buffer::setstring( char *str )
{
    reset();
    if( str )
    {
        for( ; *str; str++ ) add(*str);
    }
}

char *buffer::parse_char( char *source, unsigned char *value )
{
    while( isspace(*source)) source++;
    if( ! *source ) return 0;

    if( source[0] == '\\' && source[1] == 'x' && isxdigit(source[2]) && isxdigit(source[3]))
    {
        char hex[3];
        int chrval;
        hex[0] = source[2];
        hex[1] = source[3];
        hex[2] = 0;
        sscanf(hex,"%x",&chrval);
        (*value) = (unsigned char) chrval;
        return source+4;
    }

    if( source[0] == '\\' && source[1] )
    {
        unsigned char c = source[1];
        switch( c )
        {
        case 'r': c = '\r'; break;
        case 'n': c = '\n'; break;
        case 't': c = '\t'; break;
        case 's': c = ' '; break;
        }
        (*value) = c;
        return source + 2;
    }

    (*value) = *source;
    return source+1;
}

void buffer ::setchars( char *source )
{
    unsigned char ch;
    reset();
    source = clean_string(source);
    if( _stricmp(source,"none") != 0 )
    {
        while( (source = parse_char(source,&ch)) != NULL ){ add(ch); }
    }
}

buffer::~buffer()
{
    delete [] sp;
}

void buffer::expand()
{
    int nsize = size*4;
    unsigned char *np = new unsigned char[nsize];
    memcpy(np,sp,size);
    delete [] sp;
    sp = np;
    cp = np+(size-1);
    ep = np+(nsize-1);
    size = nsize;
}

int buffer::load( readbuff *buff, unsigned char terminator, unsigned char escape )
{
    int ok = 1;
    reset();
    for ( ; ; )
    {
        int c = buff->getc();
        if( c == terminator ) break;
        if( escape && c == escape ) 
		    {
			     if( keep_escape) add((char) c);
			     c = buff->getc();
		    }
        if( c == readbuff::FEOF ) { ok = 0; break; }
        if( ! add((char) c) ) break;
    }
    return ok;
}

bool buffer::write( data_writer *out )
{
    int length = len();
    if( length == 0 ) return 1;
    return out->write( data(), length);
}


//////////////////////////////////////////////////////////////////////////////////////

field_error_func bde_field::error_func = 0;

bde_field::bde_field(char *name, field_type type)
{
    _name = new char [strlen(name)+1];
    strcpy(_name,name);
    _strlwr(_name);
    _type = type;
    _selected = true;
    start_delim = 0;
    end_delim = 0;
}

bde_field::~bde_field()
{
    delete [] _name;
}

void bde_field::write_error( err_type type, char *message )
{
	if( error_func ) { error_func( type, (char *) name(),message); }
}

void bde_field::set_default_delim( char *start, char *end )
{
    if( ! default_start_delim ) default_start_delim = new buffer(10);
    if( ! default_end_delim ) default_end_delim = new buffer(10);
    default_start_delim->setchars(start);
    default_end_delim->setchars(end);
}

bool bde_field::write_field(data_writer *out)
{
    if( start_delim && ! start_delim->write(out) ) return false;
    if( ! write(out) ) return 0;
    if( end_delim && ! end_delim->write(out) ) return false;
    return true;
}

bool bde_field::write_header( data_writer *out )
{
    if( default_start_delim && ! default_start_delim->write(out) ) return false;
    int nch = strlen(name());
    if( ! out->write(name(),nch) ) return false;
    if( default_end_delim && ! default_end_delim->write(out) ) return false;
    return true;
}

buffer *bde_field::default_start_delim = 0;
buffer *bde_field::default_end_delim = 0;

//////////////////////////////////////////////////////////////////////////////////////

number_field::number_field( char *name ) : bde_field( name, ft_number )
{
    start_delim = fld_start_delim ? fld_start_delim : default_start_delim;
    if( start_delim && start_delim->len() == 0 ) start_delim = 0;
    end_delim = fld_end_delim ? fld_end_delim : default_end_delim;
    if( end_delim && end_delim->len() == 0 ) end_delim = 0;
}

void number_field::set_delim(char *start, char *end)
{
    if( ! fld_start_delim ) fld_start_delim = new buffer(10);
    fld_start_delim->reset();
    if( start ) fld_start_delim->setchars( start );
    if( ! fld_end_delim ) fld_end_delim = new buffer(10);
    fld_end_delim->reset();
    if( end ) fld_end_delim->setchars( end );
}

buffer *number_field::fld_start_delim = 0;
buffer *number_field::fld_end_delim = 0;

//////////////////////////////////////////////////////////////////////////////////////

replace_def::replace_def()
{
	replace = false;
	chars = 0;
	nextchar = 0;
	message = 0;
}

replace_def::~replace_def()
{
	if( chars ) delete chars;
	if( message ) delete [] message;
	if( nextchar ) delete [] nextchar;
}

void replace_def::set_replace( char *input_str, char *output_str, char *error_message )
{
	replace = true;
    unsigned char ch;
	input_str = buffer::parse_char(input_str,&ch);
    if( input_str )
	{
		if( ! nextchar ) nextchar = new replace_def[256];
		nextchar[ch].set_replace( input_str, output_str, error_message );
	}
	else
	{
		if( chars ) { delete chars; chars = 0; }
		chars = new buffer(10);
		if( _stricmp(output_str,"delete") != 0 )
		{
			chars->setchars(output_str);
		}
		if( message ) { delete [] message; message = 0; }
		if( error_message ) 
		{ 
			message = new char[ strlen(error_message) + 1 ];
			strcpy( message, error_message );
		}
	}
}

int replace_def::apply( unsigned char **cp, char **error_message, data_writer *out, bool &applied )
{
	int result = 1;
    (*cp)++;
	unsigned char ch=**cp;
	if( ch && nextchar && nextchar[ch].replaced() )
	{
		nextchar[ch].apply(cp,error_message,out,applied);
	}
	if( chars && ! applied )
	{
		result = chars->write(out);
		applied = true;
		(*error_message) = message;
	}
	if( ! applied ) (*cp)--;
	return result;
}

text_field::text_field( char *name ) : bde_field( name, ft_text )
{
    start_delim = fld_start_delim ? fld_start_delim : default_start_delim;
    if( start_delim && start_delim->len() == 0 ) start_delim = 0;
    end_delim = fld_end_delim ? fld_end_delim : default_end_delim;
    if( end_delim && end_delim->len() == 0 ) end_delim = 0;
}

void text_field::set_delim(char *start, char *end)
{
    if( ! fld_start_delim ) fld_start_delim = new buffer(10);
    fld_start_delim->reset();
    if( start ) fld_start_delim->setchars( start );
    if( ! fld_end_delim ) fld_end_delim = new buffer(10);
    fld_end_delim->reset();
    if( end ) fld_end_delim->setchars( end );
}

buffer *text_field::fld_start_delim = 0;
buffer *text_field::fld_end_delim = 0;
replace_def text_field::replace[256];

int text_field::set_output_char( char *input_chr, char *output_str, char *message )
{
    unsigned char ch;
    input_chr = parse_char(input_chr,&ch);
    if( ! input_chr ) return 0;
	replace[ch].set_replace(input_chr,output_str,message);
    return 1;
}


bool text_field::write_field( data_writer *out )
{
    if( start_delim && ! start_delim->write(out) ) return false;
	  str();  // Ensure null terminator
    unsigned char *sp = data();
    int len = this->len();
    unsigned char *cp;
    for( cp = sp; len ; )
    {
        if( replace[*cp].replaced() )
        {
			char *message = 0;
            if( cp > sp ){ int nc = cp-sp; if( ! out->write(sp,nc) ) return false; }
            sp = cp;
			if( ! replace[*cp].apply( &cp, &message, out ) ) return false;
			if( cp == sp ) 
			{
				cp++;
				len--;
			}
			else
			{
				len -= cp-sp;
				sp = cp;
			}
			if( message )
			{
				write_error(et_invalid_char, message);
			}

        }
		else
		{
			len--;
			cp++;
		}
    }
    if( cp > sp ){ int nc = cp-sp; if( ! out->write(sp,nc) ) return false; }
    if( end_delim && ! end_delim->write(out) ) return false;
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////

date_field::date_field( char *name ) : bde_field( name, ft_date )
{
    start_delim = fld_start_delim ? fld_start_delim : default_start_delim;
    if( start_delim && start_delim->len() == 0 ) start_delim = 0;
    end_delim = fld_end_delim ? fld_end_delim : default_end_delim;
    if( end_delim && end_delim->len() == 0 ) end_delim = 0;
}

void date_field::set_delim(char *start, char *end)
{
    if( ! fld_start_delim ) fld_start_delim = new buffer(10);
    fld_start_delim->reset();
    if( start ) fld_start_delim->setchars( start );
    if( ! fld_end_delim ) fld_end_delim = new buffer(10);
    fld_end_delim->reset();
    if( end ) fld_end_delim->setchars( end );
}

buffer *date_field::fld_start_delim = 0;
buffer *date_field::fld_end_delim = 0;


int date_field::year()
{
	str();
	int nf = 0;
	int inf = 1;
	for( char *s = str(); s; s++ )
	{
		if( isdigit(*s))
		{
			nf+=inf;
			if( nf == 3 ) return atoi(s);
			inf=0;
		}
		else
		{
			inf = 1;
		}
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////

datetime_field::datetime_field( char *name ) : bde_field( name, ft_datetime )
{
    start_delim = fld_start_delim ? fld_start_delim : default_start_delim;
    if( start_delim && start_delim->len() == 0 ) start_delim = 0;
    end_delim = fld_end_delim ? fld_end_delim : default_end_delim;
    if( end_delim && end_delim->len() == 0 ) end_delim = 0;
}


void datetime_field::set_delim(char *start, char *end)
{
    if( ! fld_start_delim ) fld_start_delim = new buffer(10);
    fld_start_delim->reset();
    if( start ) fld_start_delim->setchars( start );
    if( ! fld_end_delim ) fld_end_delim = new buffer(10);
    fld_end_delim->reset();
    if( end ) fld_end_delim->setchars( end );
}

buffer *datetime_field::fld_start_delim = 0;
buffer *datetime_field::fld_end_delim = 0;

int datetime_field::year()
{
	char *s = str();
	if( ! isdigit(*s)) return -1;
    return atoi(str());
}

//////////////////////////////////////////////////////////////////////////////////////

geometry_field::geometry_field( char *name ) : bde_field( name, ft_geometry )
{
    start_delim = fld_start_delim ? fld_start_delim : default_start_delim;
    if( start_delim && start_delim->len() == 0 ) start_delim = 0;
    end_delim = fld_end_delim ? fld_end_delim : default_end_delim;
    if( end_delim && end_delim->len() == 0 ) end_delim = 0;
}

void geometry_field::set_delim(char *start, char *end)
{
    if( ! fld_start_delim ) fld_start_delim = new buffer(10);
    fld_start_delim->reset();
    if( start ) fld_start_delim->setchars( start );
    if( ! fld_end_delim ) fld_end_delim = new buffer(10);
    fld_end_delim->reset();
    if( end ) fld_end_delim->setchars( end );
}


buffer *geometry_field::fld_start_delim = 0;
buffer *geometry_field::fld_end_delim = 0;

double geometry_field::lon_offset = 0.0;
int geometry_field::ilon_offset = 0;
bool geometry_field::int_offset = true;
buffer *geometry_field::wkt_prefix = 0;
bool geometry_field::keep_prefix = true;

void geometry_field::set_lon_offset(double offset)
{
	lon_offset = offset; 
	ilon_offset = (int) offset;
	int_offset = ilon_offset >= 0 && ilon_offset == offset;
}

void geometry_field::set_wkt_prefix(char *prefix)
{
    if( wkt_prefix ){ delete wkt_prefix; wkt_prefix=0; }
    keep_prefix = ! prefix || _stricmp(prefix,"leave") == 0;
    if( ! keep_prefix && _stricmp(prefix,"none") != 0 )
    {
        wkt_prefix = new buffer(prefix);
        if( wkt_prefix->len() == 0 ){ delete wkt_prefix; wkt_prefix = 0; }
    }
}

bool geometry_field::write_field( data_writer *out )
{
    if( start_delim && ! start_delim->write(out) ) return false;
    char *sp = str();
    int len = this->len();

    // Skip leading spaces and digits
    if( ! keep_prefix ) while( isspace(*sp) || isdigit(*sp)) { sp++; len--; }

    // Write the prefix if required
    if( wkt_prefix && ! wkt_prefix->write(out) ) return false;

    // Write the content

    if( lon_offset )
    {
        if( ! write_offset_geom(sp,out) ) return false;
    }
    else
    {
        if( ! out->write(sp,len)) return false;
    }
    if( end_delim && ! end_delim->write(out) ) return false;
    return true;
}

inline char *inttobuf( int i, char *buffer )
{
	*buffer = 0;
	for( ; ; )
	{
		*--buffer = '0'+(i % 10);
		i /= 10;
		if( i == 0 ) break;
	}
	return buffer;
}

int geometry_field::write_offset_geom( char *sp, data_writer *out )
{
    // Process text to extract numeric fields using a state machine */

    char *cp = sp;  // Current pointer
    char *np = 0;   // Start of number pointer
    int ndp = 0; 
    unsigned int state = 0;
	unsigned int action = 0;
    char lonbuf[32];
	char *endbuf = lonbuf+31;

    for( cp = sp; *cp; cp++ )
    {
        /* start_state_machine
        
            # Example of state machine code
            
            state_variable state
            input_variable *cp
            action_variable action
            
            class digit '0123456789'
            class point '.'
            class sign '-'
            class space ' '
            class start '(,'
            
            state none start=>ready
            state ready space start sign=>sign+startfp digit=>number+start
            state sign digit=>number_fp
            state number digit point=>none+endint space=>none+endint
            state number_fp digit point=>number_dp space=>none+endfp
            state number_dp digit=>number_dp+addndp space=>none+endfp
            
            default_state none
            
        start_state_machine_implementation */

        static unsigned char sm_class[256] = {
           0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
           0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
           4,0,0,0,0,0,0,0,5,0,0,0,5,3,2,0,
           1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,
           0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
           0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
           0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
           0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
           0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
           0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
           0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
           0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
           0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
           0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
           0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
           0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

        static const unsigned short sm_trans[6][6]={
           {0,0,0,0,0,1},
           {0,515,0,258,1,1},
           {0,4,0,0,0,0},
           {0,3,768,0,768,0},
           {0,4,5,0,1024,0},
           {0,1285,0,0,1024,0}};

        unsigned short sm_result = sm_trans[state][sm_class[*cp]];
        state = (unsigned char)(sm_result & 0xFF);
        action = (unsigned char)(sm_result >> 8);

        #define sm_state_none 0
        #define sm_state_ready 1
        #define sm_state_sign 2
        #define sm_state_number 3
        #define sm_state_number_fp 4
        #define sm_state_number_dp 5

        #define sm_action_startfp 1
        #define sm_action_start 2
        #define sm_action_endint 3
        #define sm_action_endfp 4
        #define sm_action_addndp 5
        
        /* end_state_machine_implementation
           end_state_machine */

        if( action )
        {
            switch(action)
            {
                case sm_action_start: 
					np = cp; 
					if( int_offset ) break;
					ndp = 0;
					state = sm_state_number_fp;
					break;

				case sm_action_endint: 
                    // Print text up to start of number
					{
                    int len = np - sp;
                    if( len > 0 )
                    {
                        if( ! out->write(sp,len)) return 0;
                    }
                    // Output the offset longitude
                    int lon = atoi(np)+ilon_offset;
					np = inttobuf(lon,endbuf);
                    len = strlen(np);
                    if( ! out->write(np,len) ) return 0;
					}
                    // Reset the start pointer
                    sp = cp;
                    break;

                case sm_action_startfp: 
					np = cp; 
					ndp = 0;
					break;

                case sm_action_addndp: ndp++; break;

                case sm_action_endfp: 
                    // Print text up to start of number
					{
                    int len = np - sp;
                    if( len > 0 )
                    {
                        if( ! out->write(sp,len) ) return 0;
                    }
                    // Output the offset longitude
                    double lon = atof(np)+lon_offset;
                    char lonbuf[32];
                    sprintf(lonbuf,"%.*lf",ndp,lon );
                    len = strlen(lonbuf);
                    if( ! out->write(lonbuf,len) ) return 0;
					}
                    // Reset the start pointer
                    sp = cp;
                    break;
            }
        }
    }
    if( cp > sp )
    {
        int len = cp-sp;
        if( ! out->write(sp,len)) return 0;
    }
    return 1;
}


data_writer *file_data_writer::open( char *fname, bool append )
{
	FILE *f = fopen(fname,append ? "ab" : "wb" );
	if( ! f ) return 0;
	file_data_writer *fdw = new file_data_writer(f);
	if( append && ftell(f) > 0 ) fdw->empty = false;
	return fdw;
}

file_data_writer::~file_data_writer()
{
	fclose(f);
}

bool file_data_writer::write( const void *buffer, int length )
{
	if( length )
	{
		empty = false;
		return fwrite(buffer,1,length,f) == (size_t) length;
	}
	return true;
}
