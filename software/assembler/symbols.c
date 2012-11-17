#include "asm.h"



typedef struct
{
    char* name;
    unsigned long position;
    int type;
}
LABEL;



static unsigned long base_address;
static LABEL*        known_labels;
static LABEL*        unknown_labels;
static unsigned int  num_known_labels;
static unsigned int  num_unknown_labels;
static unsigned int  max_known_labels;
static unsigned int  max_unknown_labels;



void set_base_address( unsigned long address )
{
    base_address = address;
}

void add_label( const char* name, unsigned long value )
{
    LABEL* new_labels;

    if( num_known_labels==max_known_labels )
    {
        new_labels = realloc( known_labels,
                              sizeof(LABEL)*(max_known_labels+10) );

        /* FIXME: handle error */
        if( !new_labels )
            return;

        known_labels = new_labels;
        max_known_labels += 10;
    }

    known_labels[ num_known_labels ].name = malloc( strlen(name) + 1 );

    /* FIXME: handle error */
    if( !known_labels[ num_known_labels ].name )
        return;

    strcpy( known_labels[ num_known_labels ].name, name );
    known_labels[ num_known_labels ].position = value;

    ++num_known_labels;
}

void require_label( const char* name, unsigned long position, int type )
{
    LABEL* new_labels;

    if( num_unknown_labels==max_unknown_labels )
    {
        new_labels = realloc( unknown_labels,
                              sizeof(LABEL)*(max_unknown_labels+10) );

        /* FIXME: handle error */
        if( !new_labels )
            return;

        unknown_labels = new_labels;
        max_unknown_labels += 10;
    }

    unknown_labels[ num_unknown_labels ].name = malloc( strlen(name) + 1 );

    /* FIXME: handle error */
    if( !unknown_labels[ num_unknown_labels ].name )
        return;

    strcpy( unknown_labels[ num_unknown_labels ].name, name );
    unknown_labels[ num_unknown_labels ].position = position;
    unknown_labels[ num_unknown_labels ].type = type;

    ++num_unknown_labels;
}

void post_process_labels( FILE* file )
{
    unsigned long value;
    unsigned int i, j;

    for( i=0; i<num_unknown_labels; ++i )
    {
        for( j=0; j<num_known_labels; ++j )
        {
            if( strcmp( unknown_labels[i].name, known_labels[j].name ) )
                continue;

            fseek( file, unknown_labels[i].position, SEEK_SET );

            value = known_labels[j].position;

            if( unknown_labels[i].type & LABEL_NEED_DIFF )
                value -= unknown_labels[i].position;
            else
                value += base_address;

            switch( unknown_labels[i].type & ~(LABEL_NEED_DIFF) )
            {
            case LABEL_NEED_10:
                fputc( value>>8 & 0xFF, file );
                fputc( value    & 0xFF, file );
                break;
            case LABEL_NEED_01:
                fputc( value    & 0xFF, file );
                fputc( value>>8 & 0xFF, file );
                break;
            case LABEL_NEED_0:
                fputc( value & 0xFF, file );
                break;
            case LABEL_NEED_1:
                fputc( value>>8 & 0xFF, file );
                break;
            }
        }
    }
}

void reset_labels( void )
{
    base_address = 0;
    free( known_labels );
    free( unknown_labels );

    num_known_labels = 0;
    num_unknown_labels = 0;
    max_known_labels = 0;
    max_unknown_labels = 0;
}

