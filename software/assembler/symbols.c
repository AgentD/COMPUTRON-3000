#include "asm.h"



typedef struct label_t
{
    char* name;
    unsigned long position;
    int type;

    struct label_t* next;
}
LABEL;



static unsigned long base_address;
static LABEL*        known_labels;
static LABEL*        unknown_labels;
static LABEL*        resolved_labels;



static unsigned int label_length( const char* name )
{
    unsigned int i = 0;

    while( isalnum( *name ) || *name=='_' )
    {
        ++i;
        ++name;
    }

    return i;
}

static int label_cmp( const char* a, const char* b )
{
    unsigned int alen = label_length( a );
    unsigned int blen = label_length( b );

    if( alen < blen ) return -1;
    if( alen > blen ) return  1;

    return strncmp( a, b, alen );
}

void write_label( unsigned long position, unsigned long value,
                  int type, FILE* out )
{
    if( type & LABEL_NEED_DIFF )
    {
        type &= ~LABEL_NEED_DIFF;
        value -= position;

        if( type==LABEL_NEED_10 || type==LABEL_NEED_01 )
            value -= 2;
        else
            value -= 1;
    }

    switch( type & 0x0F )
    {
    case LABEL_NEED_10: fputc( (value>>8) & 0xFF, out );
    case LABEL_NEED_0:  fputc(  value     & 0xFF, out ); break;
    case LABEL_NEED_01: fputc(  value     & 0xFF, out );
    case LABEL_NEED_1:  fputc( (value>>8) & 0xFF, out ); break;
    }
}

/****************************************************************************/

void set_base_address( unsigned long address )
{
    base_address = address;
}

void add_label( const char* name, unsigned long value, int type,
                FILE* output )
{
    LABEL *new_label, *i, *prev, *old;
    unsigned int length;

    while( isspace( *name ) )
        ++name;

    /* try to find references to the label */
    if( !(type & LABEL_TYPE_DEFINE) )
    {
        value += base_address;  /* adjust offset */

        for( i=unknown_labels; i; )
        {
            if( !label_cmp( i->name, name ) )
            {
                fseek( output, i->position, SEEK_SET );
                write_label( i->position, value, i->type, output );

                /* move to resolved list and advance */
                if( i==unknown_labels )
                    unknown_labels = unknown_labels->next;
                else
                    prev->next = prev->next->next;

                old = i;
                i = i->next;

                if( old->type & LABEL_NEED_DIFF )
                {
                    free( old->name );
                    free( old );
                }
                else
                {
                    old->next = resolved_labels;
                    resolved_labels = old;
                }
            }
            else
            {
                /* advance */
                prev = i;
                i = i->next;
            }
        }

        fseek( output, 0, SEEK_END );
    }

    /* allocate space for a new lable */
    new_label = malloc( sizeof(LABEL) );

    if( !new_label )
        return;             /* FIXME: handle error */

    /* allocate space for the label name */
    length = label_length( name );

    new_label->name = malloc( length + 1 );

    if( !new_label->name )
    {
        free( new_label );
        return;             /* FIXME: handle error */
    }

    /* copy values */
    strncpy( new_label->name, name, length );
    new_label->name[length] = '\0';

    new_label->position    = value;
    new_label->type        = type;

    /* add the label to the list */
    new_label->next = known_labels;
    known_labels    = new_label;
}

void require_label( const char* name, FILE* output, int type )
{
    LABEL *new_label, *i;
    unsigned int length, resolved = 0;
    unsigned long position = ftell( output );

    while( isspace( *name ) )
        ++name;

    /* try to find the label */
    for( i=known_labels; i; i=i->next )
    {
        if( !(i->type & LABEL_TYPE_DEFINE) && !label_cmp( i->name, name ) )
        {
            write_label( position, i->position, type, output );

            if( type & LABEL_NEED_DIFF )
                return;

            resolved = 1;
            break;
        }
    }

    /* allocate space for a new lable */
    new_label = malloc( sizeof(LABEL) );

    if( !new_label )
        return;             /* FIXME: handle error */

    /* allocate space for the label name */
    length = label_length( name );

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
    new_label->type     = type;

    /* add the label to the correct list */
    if( resolved )
    {
        new_label->next = resolved_labels;
        resolved_labels = new_label;
    }
    else
    {
        new_label->next = unknown_labels;
        unknown_labels  = new_label;

        /* write fill data */
        type &= (~LABEL_NEED_DIFF) & 0x0F;

        if( type==LABEL_NEED_10 || type==LABEL_NEED_01 )
            fputc( 0x00, output );

        fputc( 0x00, output );
    }
}

int get_define( const char* name, unsigned long* value )
{
    LABEL* i;

    while( isspace( *name ) )
        ++name;

    for( i=known_labels; i; i=i->next )
    {
        if( (i->type & LABEL_TYPE_DEFINE) && !label_cmp( i->name, name ) )
        {
            *value = i->position;
            return 1;
        }
    }

    return 0;
}

void reset_labels( void )
{
    LABEL* i;
    LABEL* old;

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

    for( i=resolved_labels; i; )
    {
        old = i;
        i = i->next;
        free( old->name );
        free( old );
    }

    resolved_labels = NULL;
    known_labels    = NULL;
    unknown_labels  = NULL;
    base_address    = 0;
}

