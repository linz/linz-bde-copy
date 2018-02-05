/***************************************************************************


 Copyright 2011 Crown copyright (c)
 Land Information New Zealand and the New Zealand Government.
 All rights reserved

 This program is released under the terms of the new BSD license. See
 the LICENSE file for more information.
****************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#if !defined(_WIN32) && !defined(_MSC_VER)
#include <unistd.h>
#endif
#include <new>
#if !defined(__MACH__)
#include <malloc.h>
#endif
#include "bde_copy_revision.h"
#include "bde_copy_config.h"
#include "bde_copy_utils.h"
#include "bde_copy_gzip.h"
#include "bde_copy_funcs.h"

#if defined(_WIN32) && defined(_MSC_VER)
#include "support/win32/dirent.h"
#include <windows.h>
#define PATH_MAX MAX_PATH
#define lstat _stat
#define stat _stat
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#else
#include <dirent.h>
#endif

#define MAXFILES 16

//////////////////////////////////////////////////////////////////


err_severity error_severity[et_count] = { es_debug, es_warning, es_error, es_fatal };

typedef void check_func_type( bde_field &field );

bde_field **field = 0;
bde_field **outfield = 0;
check_func_type **checkfunc = 0;

int nfields = 0;
int noutfields = 0;
int maxfields = 64;
readbuff *input = 0;

buffer repository ("\\\\bde_server\\bde_data");
buffer crs_ext(".crs.gz");
buffer gzip_ext(".gz");
buffer metadata_ext("");
bool use_infile_metadata=false;

const char *level="0";
const char *dataset = 0;

buffer fileheader("");
buffer fieldseparator("|");
buffer lineterminator("\n");

buffer configuration("");

unsigned long size = 0;
char *start = 0;
char *end = 0;
char *tablename = 0;

char *outfile = 0;
char *metafile = 0;
const char *cfgfile = "";
const char *reqfields = 0;
const char *specfields = 0;
const char *whereclause = 0;

char *infile[MAXFILES];
int ninfile = 0;
char *addfile[MAXFILES];
int naddfile = 0;

data_writer *out = 0;
FILE *meta = 0;

int n_errors = 0;
int n_rec = 0;
int n_recout = 0;
int n_recskip = 0;
int max_errors = 0;
int cmd_maxerrors = -1;
bool append = false;
bool header_only = false;
bool col_headers = false;
bool nometa = false;
bool use_archive = false;
bool use_gzip = false;
// output_stdout is true if the option '-' is used as output_file
bool output_stdout = false;

int min_year=0;
buffer err_date("01/01/1800");
buffer err_datetime("1800-01-01 00:00:00");

const char *default_cfg_ext = ".cfg";
int gzipbuffsize = 0;

struct field_override_def
{
    char *file;
    char *fields;
    field_override_def *next;
};

field_override_def *field_overrides = 0;

struct filter_def
{
    bde_field *field;
    char *value;
    bool negate;
    filter_def *next;
};

filter_def *filters = 0;

void close_files()
{
    if( input ) { delete input; input = 0; }
    if( metafile && meta ) { fclose(meta); meta = 0; }
    if( out ) { delete(out); out = 0; }
}

char *copy_string( const char *s )
{
  char *c = new char[strlen(s)+1];
  strcpy(c,s);
  return c;
}

void message_base( err_severity severity, bool showloc, const char *fmt, va_list fmtargs )
{
    if( severity == es_ignore ) return;
#ifndef DEBUG
    if( severity == es_debug ) return;
#endif

    fprintf(meta,"\n");
    if( severity == es_fatal || severity==es_error ) fprintf(meta,"Error: ");
    if( severity == es_warning ) fprintf(meta,"Warning: ");
    if( severity == es_debug ) fprintf(meta,"Debug: ");

    vfprintf(meta,fmt,fmtargs);

    if( showloc && input )
    {
        fprintf(meta,"At record %d of %s\n",n_rec,input->name());
        for( int i = 0; i < nfields; i++ )
        {
            fprintf(meta,"%s|",field[i]->str());
        }
        fprintf(meta,"\n");
    }

    if( severity == es_fatal )
    {
        fprintf(meta,"\nResultStatus: fail\n");

        close_files();
        if( outfile ) { remove(outfile); }

        exit(1);
    }
}


void message( err_severity severity, const char *fmt, ... )
{
    va_list fmtargs;
    va_start(fmtargs,fmt);
    message_base(severity, false,fmt, fmtargs );
    va_end(fmtargs);
}

void allocation_error( )
{
    message(es_fatal,"Cannot allocate sufficient memory\n");
}

void data_error( err_type et, const char *fmt, ... )
{
    err_severity es = error_severity[et];
    bool showloc = et != et_file_size;
    va_list fmtargs;
    va_start(fmtargs,fmt);
    message_base(es, showloc, fmt, fmtargs );
    va_end(fmtargs);

    if( es == es_error )
    {
        n_errors++;
        if( max_errors && n_errors > max_errors)
        {
            message(es_fatal,"Maximum number of errors %d exceeded in %s\n",
                max_errors,input ? input->name() : "??");
        }
    }
}

void init_fields()
{
    if( maxfields < 32 ) maxfields = 32;
    field = new bde_field* [maxfields];
    outfield = new bde_field* [maxfields];
    checkfunc = new check_func_type*[maxfields];
    nfields = 0;
}

void finish()
{
    delete [] field;
}

int add_field(char *name,char *type)
{
    if( nfields >= maxfields )
    {
        message(es_fatal,"Too many columns in CRS file\n");
    }
    bde_field *f = 0;
    if( _stricmp(type,"st_geometry") == 0 ){ f = new geometry_field(name); }
    else if (_stricmp(type,"date") == 0 ){ f = new date_field(name); }
    else if (_stricmp(type,"datetime") == 0 ){ f = new datetime_field(name); }
    else if (_stricmp(type,"decimal") == 0 ){ f = new number_field(name); }
    else if (_stricmp(type,"serial") == 0 ){ f = new number_field(name); }
    else if (_stricmp(type,"integer") == 0 ){ f = new number_field(name); }
    else { f = new text_field(name); }
    field[nfields] = f;
    outfield[noutfields] = f;
    checkfunc[noutfields] = 0;
    nfields++;
    noutfields++;
    return 1;
}

int check_field(char *name,char *type, int nfield)
{
    if( nfields >= maxfields )
    {
        message(es_fatal,"Too many columns in CRS file\n");
    }
    bde_field *f = 0;
    if( _stricmp(type,"st_geometry") == 0 ){ f = new geometry_field(name); }
    else if (_stricmp(type,"date") == 0 ){ f = new date_field(name); }
    else if (_stricmp(type,"datetime") == 0 ){ f = new datetime_field(name); }
    else if (_stricmp(type,"decimal") == 0 ){ f = new number_field(name); }
    else if (_stricmp(type,"serial") == 0 ){ f = new number_field(name); }
    else if (_stricmp(type,"integer") == 0 ){ f = new number_field(name); }
    else { f = new text_field(name); }
    bde_field *check=field[nfield];
    int match=1;
    if( _stricmp(check->name(),f->name()) != 0 ) match=0;
    if( check->type() != f->type() ) match=0;
    delete f;
    return match;
}

void select_fields(const char *fields)
{
    noutfields = 0;
    char *flds = copy_string(fields);
    for( char *name = strtok(flds,":"); name; name = strtok(NULL,":"))
    {
        if( noutfields > maxfields )
        {
            message(es_fatal,"Too many output field specified\n");
        }
        outfield[noutfields] = 0;
        bool found = false;
        for( int i = 0; i < nfields; i++ )
        {
            bde_field *f = field[i];
            if( _stricmp(f->name(),name) == 0 )
            {
                outfield[noutfields] = f;
                checkfunc[noutfields] = 0;
                found = true;
                break;
            }
        }
        if( ! found )
        {
            outfield[noutfields] = new bde_field(name,ft_number);
            message(es_error,
                "Selected field %s will be omitted - not in the CRS file\n",
                name );
        }
        noutfields++;
    }
    delete [] flds;
}


void parse_whereclause(const char *wherecls)
{
    char *whereclause = copy_string(wherecls);
    for( char *name = strtok(whereclause,":"); name; name = strtok(NULL,":"))
    {
        char save;
        char *saveptr;
        char *value;
        for( value = name; *value && *value != '=' && *value != '!'; value++ ){}
        saveptr = value;
        save = *value;
        if( save == '!' ) value++;
        if( *value != '=' || value == name )
        {
            message(es_error,"Invalid conditional expression %s specified\n",name);
            continue;
        }
        value++;
        bde_field *ftest = 0;
        *saveptr = 0;
        for( int i = 0; i < nfields; i++ )
        {
            if( _stricmp(field[i]->name(),name) == 0 )
            {
                ftest = field[i];
                break;
            }
        }
        *saveptr = save;
        if( ! ftest )
        {
            message(es_error,"Conditional expression %s refers to invalid field\n",name);
            continue;
        }
        filter_def *filter = new filter_def();
        filter->field = ftest;
        filter->value = _strdup(value);
        filter->negate = save == '!';
        filter->next = 0;
        if( ! filters )
        {
            filters = filter;
        }
        else
        {
            for( filter_def *f = filters; f; f=f->next )
            {
                if( ! f->next ){ f->next = filter; break; }
            }
        }
    }
    delete [] whereclause;
}


void check_date_field( bde_field &df )
{
    int year = ((date_field &)df).year();
    if( year < min_year )
    {
        data_error(et_date_range,"Date value of column %s is outside valid range\n", df.name() );
        df.setstring(err_date.str());
    }
}

void check_datetime_field( bde_field &df )
{
    int year = ((datetime_field &)df).year();
    if( year < min_year )
    {
        data_error(et_date_range,"Date value of column %s is outside valid range\n", df.name() );
        df.setstring(err_datetime.str());
    }
}
void init_checkfuncs()
{
    if( min_year == 0 ) return;
    for( int i = 0; i < noutfields; i++ )
    {
        checkfunc[i] = 0;
        switch( outfield[i]->type())
        {
        case ft_date: checkfunc[i] = check_date_field; break;
        case ft_datetime: checkfunc[i] = check_datetime_field; break;
    default : break;
        }
    }
}

void set_fields( const char *fields )
{
    nfields = 0;
    noutfields = 0;
    char *flds = copy_string(fields);
    for( char *name = strtok(flds,": "); name; name=strtok(NULL,": "))
    {
        char *type = name + strlen(name);
        char *p = strchr(name,'=');
        if( p != NULL )
        {
            *p = 0;
            type = p+1;
        }
        add_field(name,type);
    }
    delete [] flds;
}

bool apply_field_overrides( bool addfields )
{
    if( ! field_overrides ) return false;
    // Extract the filename component of the input file path
    char *f = input->name();
    char *n = 0;
    while( *f ){ if( *f == '/' || *f == '\\' ) n = f+1; f++;}
    if( ! n ) n = input->name();
    n = strdup(n);
    n = strtok(n,".");
    bool result = false;
    for( field_override_def *fo = field_overrides; fo; fo=fo->next )
    {
        if( _stricmp(n,fo->file) == 0 )
        {
            if( addfields ) set_fields( fo->fields );
            result = true;
        }
    }
    free(n);
    return result;
}

int read_header( bool external_header )
{
    bool readcols = true;
    if( specfields )
    {
        set_fields(specfields);
        readcols = false;
    }
    else if( apply_field_overrides(true) )
    {
        readcols = false;
    }

    char inrec[1024];
    char *line;
    while( input->getline(inrec,1024) == readbuff::OK )
    {
        line = clean_string(inrec);
        if( _strnicmp(line,"START ",6) == 0 )
        {
            start = _strdup(clean_string(line+6));
        }
        else if( _strnicmp(line,"END ",4)== 0 )
        {
            end = _strdup(clean_string(line+4));
        }
        else if( _strnicmp(line,"TABLE ",6) == 0 )
        {
            tablename = _strdup(strtok(line+6," "));
        }
        else if( _strnicmp(line,"COLUMN ",7) == 0 && readcols )
        {
            char *name = strtok(line+7," ");
            char *type = strtok(NULL," ");
            if( name && type ) add_field(name,type);
        }
        else if(_strnicmp(line,"SIZE ",5) == 0 )
        {
            sscanf(line+5,"%ld",&size);
        }
        else if(_strnicmp(line,"{CRS-DATA}",10) == 0 )
        {
            break;
        }
    }

    return (external_header || input->status() == readbuff::OK)
           && tablename && start && end && size != 0;
}

void skip_header()
{
    bool readcols = true;
    if( specfields || apply_field_overrides(false)) readcols = true;

    char inrec[1024];
    char *line;
    int nfield = 0;
    while( input->getline(inrec,1024) == readbuff::OK )
    {
        line = clean_string(inrec);
        if(_strnicmp(line,"{CRS-DATA}",10) == 0 )
        {
            break;
        }
        if( _strnicmp(line,"COLUMN ",7) == 0 && readcols )
        {
            char *name = strtok(line+7," ");
            char *type = strtok(NULL," ");
            if( name && type && nfield < nfields && ! check_field(name,type,nfield) )
            {
                message(es_fatal,"Inconsistent column names %s and %s or types in %s\n",
                    name,field[nfield]->name(),input->name());

            }
            nfield++;
        }
        else if(_strnicmp(line,"SIZE ",5) == 0 )
        {
            sscanf(line+5,"%ld",&size);
        }
    }
    if( readcols && nfield != nfields )
    {
        message(es_fatal,"Inconsistent number of columns %d and %d in %s\n",
            nfields, nfield, input->name());

    }
}


int read_data()
{
  int result=0;
  int ok = 1;
  for(; ;)
  {
    n_rec++;
    for( int i = 0; i < nfields; i++ )
    {
      bde_field &f = *(field[i]);
      f.reset();
      if( ! f.load(input))
      {
        n_rec--;
        if( i == 0 && f.len() == 0 )
        {
          ok = 0;
        }
        break;
      }
    }
    if (ok==0)
    {
      break;
    }
    if( input->endline() == readbuff::EOL )
    {
      result = 1;
      break;
    }
    data_error(et_column_count,"Incorrect number of field\n");
  }
  return result;
}



int write_column_headers( data_writer *out )
{
    int first = 1;
    for( int i = 0; i < noutfields; i++ )
    {
        bde_field &f = (*outfield[i]);
        if( first )
        {
            first = 0;
        }
        else
        {
            if( ! fieldseparator.write(out) ) return 0;
        }

        if( ! f.write_header(out) ) return 0;
    }
    if( ! lineterminator.write(out) ) return 0;
    return 1;
}

void write_field_error( err_type type, char *name, char *message )
{
    data_error( type, "%s in field %s\n",message,name);
}

int write_data( data_writer *out )
{
    int first = 1;
    for( int i = 0; i < noutfields; i++ )
    {
        bde_field &f = (*outfield[i]);
        if( first )
        {
            first = 0;
        }
        else
        {
            if( ! fieldseparator.write(out) ) return 0;
        }

        if( f.len() == 0 ) continue;
        if( checkfunc[i]) checkfunc[i](f);

        if( ! f.write_field(out) ) return 0;
    }
    if( ! lineterminator.write(out) ) return 0;
    return 1;
}

int copy_data( data_writer *out )
{
    n_rec = 0;
    while( read_data() )
    {
        if( filters )
        {
            bool ok = true;
            for( filter_def *f = filters; f; f=f->next )
            {
                ok = (strcmp( f->value, f->field->str()) == 0) ^ f->negate;
                if( ! ok ) break;
            }
            if( ! ok )
            {
                n_recskip++;
                continue;
            }
        }
        if( ! write_data(out) )
        {
            message(es_fatal,"Failed to write data to %s\n",outfile);
            return 0;
        }
        else
        {
            n_recout++;
        }
    }
    return 1;
}

void close_file()
{
    if( input )
    {
        delete input;
        input = 0;
    }
}

bool file_is_gzip( char *file )
{
    return strlen(file) > (size_t)gzip_ext.len() &&
          _stricmp(file+strlen(file)-gzip_ext.len(),gzip_ext.str()) == 0;
}

bool open_file( char *file )
{
    if( input ) close_file();
    if( file_is_gzip( file ) )
    {
        input = new gzipbuff(file,gzipbuffsize);
    }
    else
    {
        input = new filebuff(file);
    }
    if( input->status() != readbuff::OK )
    {
        delete input;
        input = 0;
        message(es_fatal,"Cannot open file %s\n",file);
    }
    return input ? true : false;
}

int copy_prefix_files()
{
    if( ! naddfile ) return 1;
    int ok = 1;

    for( int i = 0; i < naddfile; i++ )
    {
        open_file(addfile[i]);
        if( input )
        {
            if( ! copy_data(out) ) ok = 0;
        }
        close_file();
    }
    return ok;
}

void check_bde_size()
{
    if( ( (unsigned long)input->loc() ) != size )
    {
        data_error(et_file_size,"Size of file %s (%lu) doesn't match size in header (%lu)\n",
            input->name(),input->loc(), size );
    }
}

/// Return true if the given directory entry is a directory
//
/// Follows symlinks
///
/// @param dirpath path of the directory whose entry belongs
/// @param ent dirent structure pointer (used to speedup lookups,
///            when stat is known, depending on filesystem type )
///
/// @return true if the dir entry is a directory, false otherwise
///
static bool is_directory(struct dirent *ent, const char *dirpath)
{
  if( ent->d_type == DT_DIR ) return true;
  if( ent->d_type != DT_UNKNOWN ) return false;
  char path[PATH_MAX];
  sprintf(path, "%s/%s", dirpath, ent->d_name);
  struct stat buf;
  if ( -1 == lstat(path, &buf) ) {
    message(es_warning,"Cannot stat file %s\n", path);
    return false;
  }
  return S_ISDIR(buf.st_mode);
}

/// Return true if the given directory entry is a regular file
//
/// Follows symlinks
///
/// @param dirpath path of the directory whose entry belongs
/// @param ent dirent structure pointer (used to speedup lookups,
///            when stat is known, depending on filesystem type )
///
/// @return true if the dir entry is a file, false otherwise
///
static bool is_file(struct dirent *ent, const char *dirpath)
{
  if( ent->d_type == DT_REG ) return true;
  if( ent->d_type != DT_UNKNOWN ) return false;
  struct stat buf;
  char path[PATH_MAX];
  sprintf(path, "%s/%s", dirpath, ent->d_name);
  if ( -1 == lstat(path, &buf) ) {
    message(es_warning,"Cannot stat file %s\n", path);
    return false;
  }
  return S_ISREG(buf.st_mode);
}

bool valid_dataset( const char *dirname )
{
    if( strlen(dirname) != 14 ) return false;
    if( strncmp(dirname,"20",2) != 0 ) return false;
    for( const char *c = dirname+2; *c; c++) { if( ! isdigit(*c) ) return false; }
    return true;
}

int cmp_filename( const void *fp1, const void *fp2 )
{
    return _stricmp( * (char **) fp1, * (char **) fp2 );
}

char *get_input_file( char *name)
{
    // If the supplied file name includes directory components then leave unaltered
    if( strchr(name,'.') || strchr(name,'\\') || strchr(name,'/')) return name;

    char *filename = new char[ repository.len()+strlen(name)+crs_ext.len()+64];
    strcpy(filename,repository.str());
    strcat(filename,"/level_");
    strcat(filename,level);

    // If a dataset is supplied, check it is valid

    const char *fdataset = 0;
    if( dataset)
    {
        strcat(filename,"/");
        strcat(filename,dataset);
        fdataset = dataset;
    }
    else
    {
        // Find the last dataset
        char *dirname = strdup(filename);
        DIR *dir = opendir(filename);

        if( dir == NULL )
        {
            message(es_fatal,"Invalid bde repository %s\n",filename);
            free(dirname);
            return 0;
        }
        strcat(filename,"/");
        char *end = filename + strlen(filename);
        struct dirent *ent;
        message(es_debug,"Reading dir %s", filename);
        while ( ( ent = readdir( dir ) ) != NULL )
        {
            message(es_debug,"Checking entry %s", ent->d_name);
            if( ! is_directory(ent, dirname) ) {
              message(es_debug,"Entry %s is not a directory (%d instead of %d)", ent->d_name, ent->d_type, DT_DIR);
              continue;
            }
            if( strcmp(ent->d_name,".") == 0 || strcmp(ent->d_name,"..") == 0 )
            {
              message(es_debug,"Entry %s is virtual directory", ent->d_name);
              continue;
            }
            char *d = strdup(ent->d_name);
            if( ! valid_dataset(d) ) {
              message(es_debug,"%s is not a valid dataset", d);
              continue;
            }
            if( *end && strcmp(d,end) < 0 ) {
              message(es_debug,"Something weird happened");
              continue;
            }
            strcpy(end,d);
            fdataset = end;
            free(d);
        }
        free(dirname);
        closedir(dir);
        if( ! *end )
        {
            message(es_fatal,"Could not find a dataset in %s\n",filename);
            return 0;
        }
    }
    strcat(filename,"/");
    strcat(filename,name);
    strcat(filename,crs_ext.str());

    if( use_archive )
    {
        char *archive = new char[repository.len()+20];
        strcpy(archive,repository.str());
        strcat(archive,"/level_");
        strcat(archive,level);
        strcat(archive,"_archive");
        DIR *dir = opendir(archive);
        if( dir != NULL )
        {
                struct dirent *ent;
            while ( ( ent = readdir( dir ) ) != NULL )
            {
                if( ! is_file(ent, archive) ) continue;
                // Check that the file name matches the file being copied
                char *f1 = strdup(ent->d_name);
                char *fname = strtok(f1,".");
                if( _stricmp(fname,name) != 0 ) continue;

                // Check that the file applies to this dataset, not only to those
                // after this date.
                bool ok = true;
                while( char *fdate = strtok(NULL,"."))
                {
                    if( valid_dataset(fdate) && strncmp(fdate,fdataset,14) > 0 ) ok = false;
                }
                free(f1);
                if( ! ok ) continue;
                if( naddfile >= MAXFILES )
                {
                    message(es_fatal,"Too many archive files for program - increase MAXFILES\n");
                    continue;
                }
                char *archive_file = new char[strlen(archive)+strlen(ent->d_name)+2];
                strcpy(archive_file, archive);
                strcat(archive_file,"/");
                strcat(archive_file,ent->d_name);
                addfile[naddfile] = archive_file;
                naddfile++;
            }
            if( naddfile > 1 )
            {
                qsort(addfile,naddfile,sizeof(char *),cmp_filename);
            }
        }
        closedir(dir);
    }


    return filename;
}

readbuff *open_bde_file(char *filename, bool readheader)
{
    bool external_header = false;
    char *header_file = new char[strlen(filename)+strlen(metadata_ext.str())+1];
    strcpy(header_file,filename);

    if( metadata_ext.len() > 0 )
    {

        if( file_is_gzip(filename))
        {
            header_file[strlen(header_file)-gzip_ext.len()] = 0;
        }
        strcat(header_file,metadata_ext.str());

        if( file_exists( header_file ) )
        {
            open_file( header_file );

            if( input->status() == readbuff::OK )
            {
                external_header = true;
            }
        }
        if( ! external_header && ! use_infile_metadata )
        {
            message(es_fatal,"Cannot open metadata file %s\n",header_file);
        }
    }

    if( ! external_header )
    {
        open_file( filename );
        if( input->status() != readbuff::OK )
        {
            message(es_fatal,"Cannot open file %s\n",filename);
        }
    }

    if( readheader && ! read_header( external_header ) )
    {
        message(es_fatal,"Cannot read CRS header in %s\n", header_file);
    }

    if( external_header )
    {
        open_file( filename );
        if( input->status() != readbuff::OK )
        {
            message(es_fatal,"Cannot open file %s\n",filename);
        }
    }

    if( ! readheader && ! external_header ) skip_header();

    delete [] header_file;

    return input;
}

/* Parse a non-blank character from a string, either as a single
   character or a \xHH string, and return a pointer to the next
   character in the string.  Return NULL if no character found */

char *parse_char( char *source, unsigned char *value )
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
        (*value) = source[1];
        return source + 2;
    }

    (*value) = *source;
    return source+1;
}

char *get_data_path( const char *exefile, const char *ext )
{
    char *path = 0;
    int path_count = 3;
    char **search_path = new char*[path_count];
    search_path[0] = copy_string(exefile);

    const char *base_name = basename(exefile);
    char *env_path = getenv("BDECOPY_DATADIR");
    if (env_path != 0)
    {
        search_path[1] = catdir(env_path, base_name);
    }
    else
    {
        search_path[1] = 0;
    }
    search_path[2] = catdir(DATADIR, base_name);

    int i = 0;
    char* temp_base_path = 0;
    size_t ext_len = strlen(ext);
    while (i < path_count && temp_base_path == 0)
    {
        if (search_path[i])
        {
            size_t path_len = strlen(search_path[i]);
            char *temp_file = new char[path_len + ext_len + 1];
            strcpy(temp_file, search_path[i]);
            strcat(temp_file, ext);
            if ( file_exists(temp_file) )
            {
                temp_base_path = search_path[i];
            }
            delete[] temp_file;
        }
        i++;
    }

    if ( temp_base_path )
    {
        path = new char[strlen(temp_base_path) + 1];
        strcpy(path, temp_base_path);
    }

    for (int j=0; j < path_count; j++)
    {
        if( search_path[j] )
            delete[] search_path[j];
    }
    delete[] search_path;

    return path;
}

bool read_configuration_file( char *configfile, bool isdefault )
{

    FILE *cfg = fopen(configfile,"r");
    if( ! cfg )
    {
        if( ! isdefault )
        {
            message(es_fatal,"Cannot open configuration file %s: %s",
                    configfile, strerror(errno));
        }
        return false;
    }

    configuration.add("ConfigFile: ");
    configuration.add(configfile);
    configuration.add("\n");

    char buff[1024];
    bool cfgok = true;
    int lineno = 0;

    while( fgets(buff,1024,cfg) )
    {
        lineno++;
        char *cmd = clean_string(buff);
        if( ! cmd[0] || cmd[0] == '#' ) continue;

        cmd = strtok(cmd," ");
        char *value = strtok(NULL,"");
        if( value ) value = clean_string(value);

        _strlwr(cmd);

        bool cmdok = false;
        if( strcmp(cmd,"field_separator") == 0 && value)
        {

            unsigned char ch;
            fieldseparator.reset();
            while( (value = parse_char(value,&ch)) != NULL ){ fieldseparator.add(ch); }
            cmdok = true;
        }
        else if( strcmp(cmd,"file_prefix") == 0 )
        {
            unsigned char ch;
            fileheader.reset();
            while( (value = parse_char(value,&ch)) != NULL ){ fileheader.add(ch); }
            cmdok = true;
        }
        else if( strcmp(cmd,"line_terminator") == 0 )
        {
            unsigned char ch;
            lineterminator.reset();
            while( (value = parse_char(value,&ch)) != NULL ){ lineterminator.add(ch); }
            cmdok = true;
        }
        else if( strcmp(cmd,"field_delimiter") == 0)
        {
            char *s1 = strtok(value," ");
            char *s2 = 0;
            char *s3 = 0;
            if( s1 )
            {
                cmdok = true;
                s2 = strtok(NULL," ");
                if( s2 ) s3 = strtok(NULL," ");
                if( s3 )
                {
                    if( _stricmp(s1,"number")==0){ number_field::set_delim(s2,s3); } else
                    if( _stricmp(s1,"text")==0){ text_field::set_delim(s2,s3); } else
                    if( _stricmp(s1,"date")==0){ date_field::set_delim(s2,s3); } else
                    if( _stricmp(s1,"datetime")==0){ datetime_field::set_delim(s2,s3); } else
                    if( _stricmp(s1,"geometry")==0){ geometry_field::set_delim(s2,s3); } else
                    cmdok = false;
                }
                else if( s2 ) { bde_field::set_default_delim(s1,s2); }
                else { bde_field::set_default_delim(s1,s1); }
            }
        }
        if( strcmp(cmd,"column_header") == 0 && value)
        {
            char *s1 = strtok(value," ");
            if( s1 && _stricmp(s1,"yes") == 0 ) { col_headers = true; cmdok = true; }
            else if( s1 && _stricmp(s1,"no") == 0 ) { col_headers = false, cmdok = true; }
        }
        else if( strcmp(cmd,"error_type") == 0 )
        {
            char *type=0;
            char *severity=0;
            err_type et;
            err_severity es;
            cmdok = true;
            type = strtok(value," ");
            if( type ) severity = strtok(NULL," ");
            if( ! severity ) cmdok = false;
            if( cmdok )
            {
                if( _stricmp( type, "date_range" ) == 0 ) et = et_date_range;
                else if( _stricmp( type, "invalid_char" ) == 0 ) et = et_invalid_char;
                else if( _stricmp( type, "column_count" ) == 0 ) et = et_column_count;
                else if( _stricmp( type, "file_size") == 0 ) et = et_file_size;
                else cmdok = false;

                if( _stricmp( severity, "ignore" ) == 0 ) es = es_ignore;
                else if( _stricmp( severity, "warning" ) == 0 ) es = es_warning;
                else if( _stricmp( severity, "error") == 0 ) es = es_error;
                else if( _stricmp( severity, "fatal") == 0 ) es = es_fatal;
                else cmdok = false;
            }
            if( cmdok )
            {
                error_severity[et] = es;
            }
        }
        else if( strcmp(cmd,"utf8_encoding") == 0 )
        {
            cmdok=true;
            char *s1 = strtok(value," ");
            if( s1 && _stricmp(s1,"enforced") == 0 ) { text_field::detect_utf8 = true; text_field::expect_utf8 = true; }
            else if( s1 && _stricmp(s1,"invalid") == 0 ) { text_field::detect_utf8 = true; text_field::expect_utf8 = false; }
            else if( s1 && _stricmp(s1,"ignored") == 0 ) { text_field::detect_utf8 = false; text_field::expect_utf8 = false; }
            else cmdok = false;
        }
        else if( strcmp(cmd, "utf8_replace_invalid") == 0 )
        {
            char *repstr = strtok(value," ");
            if( repstr )
            {
                char *message = strtok(NULL,"");
                text_field::replace_utf8_invalid.set_replace(0, repstr, message );
                cmdok = true;
            }
        }
        else if( strcmp(cmd, "utf8_replace_unmapped") == 0 )
        {
            char *repstr = strtok(value," ");
            if( repstr )
            {
                char *message = strtok(NULL,"");
                text_field::replace_utf8_unmapped.set_replace(0, repstr, message );
                cmdok = true;
            }
        }
        else if( strcmp(cmd,"replace") == 0 )
        {
            char *chrstr = strtok(value," ");
            if( chrstr )
            {
                char *repstr = strtok(NULL," ");
                char *message = strtok(NULL,"");
                if( text_field::set_output_char( chrstr, repstr, message )) cmdok = true;
            }
        }
        else if( strcmp(cmd,"minimum_year") == 0 && value)
        {
            if( sscanf(value,"%d",&min_year) == 1 ) cmdok = true;
        }
        else if( strcmp(cmd,"invalid_date_string") == 0 && value)
        {
            err_date.setstring(value);
            cmdok = true;
        }
        else if( strcmp(cmd,"invalid_datetime_string") == 0 && value)
        {
            err_datetime.setstring(value);
            cmdok = true;
        }
        else if( strcmp(cmd,"wkt_prefix") == 0)
        {
            geometry_field::set_wkt_prefix(value);
            cmdok = true;
        }
        else if( strcmp(cmd,"longitude_offset") == 0 && value)
        {
            double lon_offset;
            if( sscanf(value,"%lf",&lon_offset) == 1 )
            {
                geometry_field::set_lon_offset(lon_offset);
                cmdok = true;
            }
        }
        else if( strcmp(cmd,"max_errors") == 0 && value )
        {
            if( sscanf(value,"%d",&max_errors) == 1 ) cmdok = true;
        }
        else if( strcmp(cmd,"file_buffer_size") == 0 && value )
        {
            int bufsize = 0;
            if( sscanf(value,"%d",&bufsize) == 1 && bufsize >= 4096)
            {
                cmdok = true;
                readbuff::filebuffsize = bufsize;
            }
        }
        else if( strcmp(cmd,"gzip_buffer_size") == 0 && value )
        {
            int bufsize = 0;
            if( sscanf(value,"%d",&bufsize) == 1 && bufsize >= 4096)
            {
                cmdok = true;
                gzipbuffsize = bufsize;
            }
        }
        else if( strcmp(cmd,"max_fields") == 0 && value )
        {
            if( sscanf(value,"%d",&maxfields) == 1 && maxfields > 0)
            {
                cmdok = true;
            }
        }
        else if( strcmp(cmd,"field_override") == 0 && value )
        {
            char *file = strtok(value," ");
            char *fields = strtok(NULL,"");
            if( fields ) {while(isspace(*fields)) fields++;}
            if( file && fields )
            {
                field_override_def *fo = new field_override_def;
                fo->file = strdup(file);
                fo->fields = strdup(fields);
                fo->next = field_overrides;
                field_overrides = fo;
                cmdok = true;
            }
        }
        else if( strcmp(cmd,"keep_escapes")== 0 )
        {
            bool keep = true;
            cmdok = true;
            if( value )
            {
                if( _stricmp(value,"no") == 0 ) keep = false;
                else if(_stricmp(value,"yes") != 0 ) cmdok = false;
            }
            buffer::keep_escape = keep;
        }
        else if( strcmp(cmd,"metadata_extension") == 0 && value )
        {
            char *ext = strtok(value," ");
            char *optional = strtok(NULL," ");

            if(_stricmp(ext,"none") == 0 )
            {
                metadata_ext.setstring("");
                use_infile_metadata = true;
                cmdok = true;
            }
            else
            {
                metadata_ext.setstring(ext);
                if( ! optional )
                {
                    use_infile_metadata = false;
                    cmdok = true;
                }
                else if( _stricmp(optional,"optional") == 0 )
                {
                    use_infile_metadata = true;
                    cmdok = true;
                }
            }
        }
        else if( strcmp(cmd,"use_infile_metadata") == 0 && value )
        {
            char *s1 = strtok(value," ");
            if( s1 && _stricmp(s1,"yes") == 0 ) { use_infile_metadata = true; cmdok = true; }
            else if( s1 && _stricmp(s1,"no") == 0 ) { use_infile_metadata = false, cmdok = true; }
        }
        else if( strcmp(cmd,"bde_repository") == 0 && value )
        {
            repository.setstring(value);
            cmdok = true;
        }
        if( ! cmdok )
        {
            message(es_error,
                "Invalid or incomplete config option %s\nAt line %d of %s\n",
                cmd,lineno,configfile );
            cfgok = false;
        }

    }

    fclose(cfg);
    return cfgok;
}

bool read_configuration_part( char *exefile, char *cfgext )
{
    char *configfile = 0;

    if( cfgext && (strchr(cfgext,'.') || strchr(cfgext,'/') || strchr(cfgext,'\\')) )
    {
        configfile = copy_string(cfgext);
    }
    else
    {
        int len = strlen(exefile);
        len += strlen(default_cfg_ext);
        if( cfgext ) len += strlen(cfgext);
        len+=2;
        configfile = new char[len];
        strcpy(configfile,exefile);
        strcat(configfile,default_cfg_ext);
        if( cfgext ) { strcat(configfile,"."); strcat(configfile,cfgext); }
    }
    bool result = read_configuration_file(configfile, cfgext == 0);
    delete [] configfile;
    return result;
}

bool read_configuration( char *exefile )
{
    bool ok = true;
    char *base_config_path = get_data_path(exefile, default_cfg_ext);

    if (base_config_path)
    {
        read_configuration_part(base_config_path, 0);
        if( cfgfile && *cfgfile )
        {
            char *cfgf = copy_string(cfgfile);
            char *c = cfgf;
            while( *c )
            {
                char *e = c;
                while( *e && *e != '+') e++;
                char es = *e;
                *e = 0;
                if( ! read_configuration_part(base_config_path,c) ) ok = false;
                *e = es;
                c = e;
                if( *c ) c++;
            }
            delete [] cfgf;
        }
        delete[] base_config_path;
    }
    else
    {
        message(es_warning, "Could not find base configuration, "
                      "try setting the BDECOPY_DATADIR env variable "
                      "to a directory containing a file with "
                      "the same base name as the executable and "
                      "the %s extension",
                      default_cfg_ext);
        ok = false;
    }
    return ok;
}

void print_metadata()
{
    for( int i = 0; i < ninfile; i++ )
    {
        fprintf(meta,"InputFile: %s\n",infile[i]);
    }
    for( int i = 0; i < naddfile; i++ )
    {
        fprintf(meta,"AdditionalDataFile: %s\n",addfile[i]);
    }
    fputs(configuration.str(), meta );
    fprintf(meta,"OutputFile: %s\n",outfile);

    fprintf(meta,"TableName: %s\n",tablename);
    fprintf(meta,"InputFields: ");
    for( int i = 0; i < nfields; i++ )
    {
        if( i ) { fprintf(meta,"|"); }
        fprintf(meta,"%s",field[i]->name());
    }
    fprintf(meta,"\n");
    fprintf(meta,"OutputFields: ");
    for( int i = 0; i < noutfields; i++ )
    {
        if( i ) { fprintf(meta,"|"); }
        fprintf(meta,"%s",outfield[i]->name());
    }
    fprintf(meta,"\n");
    if( filters )
    {
        fprintf(meta,"Filters:");
        bool first = true;
        for( filter_def *f = filters; f; f=f->next)
        {
            if( ! first ) fprintf(meta," AND");
            first = false;
            fprintf(meta," %s %s \"%s\"",f->field->name(),
                f->negate ? "!=" : "=", f->value );
        }
        fprintf(meta,"\n");
    }

    fprintf(meta,"BdeStartTime: %s\n",start ? start : "");
    fprintf(meta,"BdeEndTime: %s\n",end ? end : "");
    fprintf(meta,"BdeSize: %lu\n",size);
}

void help( char *exefile );

int split_file_names( const char *names, char **list, int *count )
{
    *count = 0;
    char *nms = copy_string(names);
    for( char *n = strtok(nms,"+"); n; n = strtok(NULL,"+"))
    {
        if( *count >= MAXFILES )
        {
            printf("Too many files specified in command - max is %d\n",
                MAXFILES);
            return 0;
        }
        list[*count] = copy_string(n);
        (*count)++;
    }
    delete [] nms;
    return 1;
}

bool read_args( char *image, int argc, char *argv[] )
{
    const char **nxtarg = 0;
    const char *maxerrstr = 0;
    char *infiles = 0;
    const char *addfiles = 0;
    bool argsok = true;

    if( argc < 2 ) return false;

    for( int iarg = 1; iarg < argc; iarg++ )
    {
        char *arg = argv[iarg];
        if( nxtarg ){ *nxtarg = arg; nxtarg = 0; continue; }

        // if the argument only has the character '-' and the next
        // expected input is the outfile, will set output to stdout.
        if( arg[0] == '-' && ! arg[1] && infiles && ! outfile )
        {
            // set output to stdout
            output_stdout = true;
            outfile = arg;
        }
        else if( arg[0] == '-' )
        {
            switch( arg[1] )
            {
            case 'a':
            case 'A':  append= true; break;

            case 'n':
            case 'N':  nometa = true; break;

            case 'c':
            case 'C':  nxtarg = &cfgfile; break;

            case 'f':
            case 'F': nxtarg = &specfields; break;

            case 'w':
            case 'W': nxtarg = &whereclause; break;

            case 'o':
            case 'O': nxtarg = &reqfields; break;

            case 'e':
            case 'E': nxtarg = &maxerrstr; break;

            case 'd':
            case 'D': nxtarg = &dataset; break;

            case 'l':
            case 'L': nxtarg = &level; break;

            case 'p':
            case 'P':  nxtarg = &addfiles; break;

            case 'x':
            case 'X': use_archive = true; break;

            case 'z':
            case 'Z': use_gzip = true; break;

            case 'h':
            case 'H': header_only = true; break;

            case '?':
                      help(image); break;

            default:
                fprintf(stderr,"Invalid option %s\n",arg);
                argsok = false;
            }

            if( nxtarg && arg[2] )
            {
                char *value = arg+2;
                if( *value == ':' ) value++;
                (*nxtarg) = value;
                nxtarg = 0;
            }
        }
        else if( ! infiles )
        {
            infiles = arg;
        }
        else if( ! outfile && ! output_stdout)
        {
            outfile = arg;
        }
        else if( ! metafile )
        {
            metafile = arg;
        }
        else
        {
            fprintf(stderr,"Invalid extra argument: %s\n",arg);
            argsok = false;
        }
    }

    if( maxerrstr && sscanf(maxerrstr,"%d",&cmd_maxerrors) != 1 )
    {
        fprintf(stderr,"Invalid -e (maximum error count) option %s\n",maxerrstr);
        argsok = false;
    }

    if( nxtarg || (! outfile && ! output_stdout))
    {
        fprintf(stderr,"Required arguments not supplied\n");
        argsok = false;
    }

    if( infiles && !split_file_names(infiles,infile,&ninfile)) argsok = 0;
    if( ninfile == 0 )
    {
        fprintf(stderr,"No input files specified\n");
        argsok = false;
    }

    if( addfiles && !split_file_names(addfiles,addfile,&naddfile)) argsok = 0;

    if( strcmp(level,"0") != 0 && strcmp(level,"5") != 0 )
    {
        fprintf(stderr,"Invalid level argument \"%s\"- must be 0 or 5\n",level);
        argsok = false;
    }

    if( dataset && ! valid_dataset(dataset) )
    {
        fprintf(stderr,"Invalid dataset argument \"%s\" - must by YYYYMMDDHHMMSS\n",dataset);
        argsok = false;
    }
    
    if( use_gzip && output_stdout )
    {
        fprintf(stderr,"Cannot use -z option with output to standard output\n");
        argsok = false;
    }

    if( output_stdout && ! metafile )
    {
        fprintf(stderr,"Must specfiy log_file when output to standard output\n");
        argsok = false;
    }
    return argsok;
}


void syntax()
{
    fprintf(stderr,"bde_copy: Extracts data from BDE files\n");
    fprintf(stderr,"Version: %s (%s)\n\n",VERSION,__DATE__);
    fprintf(stderr,
        "Syntax: [options] input_file output_file [log_file]\n\n"
        "input_file is a BDE crs download file, typically gzip compressed\n"
        "output_file is the generated data file\n"
        "log_file holds information about the conversion - default is standard output\n"
        "\n"
        "To output the data to standard output use '-' as a substitute for output_file\n"
        "If output is going to standard output, a log_file must be provided\n"
        "\n"
        "Options:\n"
        "  -c xxx   Use xxx as an additional configuration file - this is read\n"
        "           *in addition* to the bde_copy.cfg file found in the package\n"
        "           data directory (or BDECOPY_DATADIR env variable if given)\n"
        "  -f xxx   Overrides the field defined in the BDE header.  xxx is formatted\n"
        "           as name1=type1:name2=type2: ... \n"
        "  -w xxx   Where condition to filter fields. xxx is formatted as\n"
        "           name1=value1:name2=value2.  The operator can be = or !=.  This is\n"
        "           interpreted as \"where name1 = value1 and name2 = value2 ....\"\n"
        "  -o xxx   Specifies which field are required in the output file.  Field names\n"
        "           are specified as name1:name2:name3...\n"
        "  -e ##    Specifies the maximum number of errors permitted before the translation\n"
        "           (0 = unlimited number of errors)\n"
        "  -l #     Defines the BDE level (0 or 5, default 0)\n"
        "  -d ###   Defines the BDE dataset (YYYYMMDDhhmmss - default most recent)\n"
        "  -a       Append data to existing file if already created\n"
        "  -p ###   Add data (minus header) from files ### ('+' separated) to the\n"
        "           extract\n"
        "  -x       Search for additional data in archive folder\n"
        "  -z       gzip compress the output file\n"
        "  -n       Don't print metadata in output\n"
        "  -h       Specifies that only the header will be translated.\n"
        "  -?       More detailed help\n"
        );
    exit(2);
}

void help( char *exefile )
{
    const char *help_ext = ".help";
    char *help_path = get_data_path (exefile, help_ext);
    if( help_path )
    {
        char *fname = new char[strlen(help_path)+8];
        strcpy(fname,help_path);
        strcat(fname,help_ext);
        FILE *f = fopen(fname,"r");
        if( f )
        {
            printf("bde_copy: Extracts data from BDE files\n");
            printf("Version: %s (%s)\n",VERSION,__DATE__);
            printf("Source version: %s\n\n",REVISION);
            char buf[256];
            while(fgets(buf,256,f)){ fputs(buf,stdout); }
            fclose(f);
            exit(2);
        }
    }
    syntax();
}
////////////////////////////////////////////////////////////////////////////////////
// Main program


int main( int argc, char *argv[] )
{
#if defined(_WIN32) && defined(_MSC_VER)
    char *image = strdup(_pgmptr);
#else
    char *image = get_image_path();
#endif

    // Trim ".exe" from the file name if it is present
    {
        int l = strlen(image);
        if( l >= 4 && _stricmp(image+l-4,".exe") == 0 )
        {
            *(image+l-4)=0;
        }
    }

    meta = stderr;
    std::set_new_handler(allocation_error);

    if( ! read_args(image,argc,argv) ) syntax();

    if( ! read_configuration(image)) {
      message(es_fatal, "Failed reading configuration");
      return 2;
    }

    if( cmd_maxerrors >= 0 ) max_errors = cmd_maxerrors;

    init_fields();

    if( metafile )
    {
        meta = fopen(metafile,"wb");
        if( ! meta )
        {
            meta = stderr;
            char *mf = metafile;
            metafile = 0;
            message(es_fatal,"Cannot open metadata file %s: %s\n",
                    mf, strerror(errno));
            return 2;
        }
    }
    else if( ! output_stdout )
    {
        /* for backward compatibility we default to stdout */
        meta = stdout;
    }

    for( int i = 0; i < ninfile; i++ )
    {
        infile[i] = get_input_file(infile[i]);
    }

    input = open_bde_file(infile[0],true);

    if( reqfields ) { select_fields(reqfields); }

    if( whereclause ){ parse_whereclause(whereclause); }

    if( nfields == 0 )
    {
        message(es_fatal,"No columns specified file header of %s\n",infile[0]);
    }

    if( ! nometa ) print_metadata();
    if( header_only )
    {
        close_files();
        return 0;
    }

    if( use_gzip )
    {
        out = gzip_data_writer::open(outfile,append,gzipbuffsize); 
    }
    else if ( output_stdout )
    {
        out = file_data_writer::open_stdout();
    }
    else
    {
        out = file_data_writer::open(outfile,append); 
    }
    if( ! out )
    {
        message(es_fatal,"Cannot open output file %s\n",outfile);
    }

    init_checkfuncs();
    bde_field::set_error_func( write_field_error );

    // Only write file headers if this is a new (empty) file
    if( out->isempty() )
    {
        fileheader.write(out);
        if( col_headers )
        {
            if( write_column_headers(out) != 1 )
            {
                message(es_fatal,"Failed to write data to %s\n",outfile);
                return 1;
            }
        }
    }

    // Copy any prefix dump files files

    if( naddfile )
    {
        close_file();
        if( ! copy_prefix_files()) return 1;
        open_bde_file(infile[0],false);
    }

    // Copy the data

    for( int i = 0; i < ninfile; i++ )
    {
        if( i ) open_bde_file(infile[i],false);
        if( ! copy_data(out) ) return 1;
        check_bde_size();
        close_file();
    }


    fprintf(meta,"\nInputCount: %d\n",n_rec);
    fprintf(meta,"OutputCount: %d\n",n_recout);
    if( n_recskip ) fprintf(meta,"RejectedCount: %d\n",n_recskip);
    fprintf(meta,"ErrorCount: %d\n",n_errors);
    fprintf(meta,"ResultStatus: success\n");

    close_files();

    return 0;
}
