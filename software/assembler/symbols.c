#include "asm.h"



typedef struct label_t
{
    char* name;
    unsigned int name_length;
    unsigned long position;
    int type;

    struct label_t* next;
}
LABEL;



static unsigned long base_address;
static LABEL*        known_labels;
static LABEL*        unknown_labels;



static unsigned int label_length( const char* name )
{
    unsigned int i = 0;

    while( isalnum( *(name++) ) )
        ++i;

    return i;
}



void set_base_address( unsigned long address )
{
    base_address = address;
}

void add_label( const char* name, unsigned long value, int type )
{
    LABEL* new_label;
    unsigned int length;

    length = label_length( name );

    /* allocate space for a new lable */
    new_label = malloc( sizeof(LABEL) );

    if( !new_label )
        return;             /* FIXME: handle error */

    /* allocate space for the label name */
    new_label->name = malloc( length + 1 );

    if( !new_label->name )
    {
        free( new_label );
        return;             /* FIXME: handle error */
    }

    /* adjust offset if required */
    if( type==LABEL_TYPE_LABEL )
        value += base_address;

    /* copy values */
    strncpy( new_label->name, name, length );
    new_label->name[length] = '\0';

    new_label->position    = value;
    new_label->type        = type;
    new_label->name_length = length;

    /* add the label to the list */
    new_label->next = known_labels;
    known_labels    = new_label;
}

void require_label( const char* name, unsigned long position, int type )
{
    LABEL* new_label;
    unsigned int length;

    length = label_length( name );

    /* allocate space for a new lable */
    new_label = malloc( sizeof(LABEL) );

    if( !new_label )
        return;             /* FIXME: handle error */

    /* allocate space for the label name */
    new_label->name = malloc( length + 1 );

    if( !new_label->name )
    {
        free( new_label );
        return;             /* FIXME: handle error */
    }

    /* copy values */
    strncpy( new_label->name, name, length );
    new_label->name[ length ] = '\0';

    new_label->position = position;
    new_label->type = type;

    /* add the label to the list */
    new_label->next = unknown_labels;
    unknown_labels  = new_label;
}

int get_define( const char* name, unsigned long* value )
{
    LABEL* i;

    for( i=known_labels; i; i=i->next )
    {
        if(  i->type == LABEL_TYPE_DEFINE &&
            !strncmp( i->name, name, i->name_length ) )
        {
            *value = i->position;
            return 1;
        }
    }

    return 0;
}

void post_process_labels( FILE* file )
{
    unsigned long value;
    LABEL* i;
    LABEL* j;

    for( i=unknown_labels; i; i=i->next )
    {
        for( j=known_labels; j; j=j->next )
        {
            /* don't post process defines */
            if( j->type == LABEL_TYPE_DEFINE )
                continue;

            /* check if we got the right one */
            if( strcmp( i->name, j->name ) )
                continue;

            /* insert the value */
            fseek( file, i->position, SEEK_SET );

            value = j->position;

            if( i->type & LABEL_NEED_DIFF )
                value -= i->position;

            switch( i->type & ~(LABEL_NEED_DIFF) )
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
    LABEL* i;
    LABEL* old;

    base_address = 0;

    for( i=unknown_labels; i; )
    {
        old = i;
        i = i->next;
        free( old->name );
        free( old );
    }

    for( i=known_labels; i; )
    {
        old = i;
        i = i->next;
        free( old->name );
        free( old );
    }
}

