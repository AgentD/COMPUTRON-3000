#include "asm.h"



typedef struct label_t
{
    char* name;
    unsigned long position;
    int type;

    struct label_t* next;
    struct label_t* prev;
}
LABEL;



static LABEL* known_labels;
static LABEL* unknown_labels;
static LABEL* resolved_labels;


#define FREE_NODE( node ) free( (node)->name ); free( node )
#define ADD_TO_LIST( list, node ) (node)->prev=NULL; (node)->next=list;\
                                  if( list ) { (list)->prev=node; } (list)=(node)


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

LABEL* create_label( const char* name, unsigned long value, int type )
{
    LABEL* l;
    unsigned int length = label_length( name );

    /* allocate space for a new lable */
    l = malloc( sizeof(LABEL) );

    if( !l )
        return NULL;

    /* allocate space for the label name */
    l->name = malloc( length + 1 );

    if( !l->name )
    {
        free( l );
        return NULL;
    }

    /* copy values */
    strncpy( l->name, name, length );
    l->name[ length ] = '\0';

    l->position = value;
    l->type     = type;

    return l;
}

void free_list( LABEL* list )
{
    LABEL *i, *old;

    for( i=list; i; )
    {
        old = i;
        i = i->next;
        FREE_NODE( old );
    }
}

LABEL* find_in_list( LABEL* list, const char* name )
{
    LABEL* i;

    for( i=list; i; i=i->next )
    {
        if( !label_cmp( i->name, name ) )
            return i;
    }

    return NULL;
}

/****************************************************************************/

void add_label( const char* name, unsigned long value, int type,
                FILE* output )
{
    LABEL *new_label, *i, *old;

    SKIP_SPACE( name );

    /* try to find references to the label */
    if( !(type & LABEL_TYPE_DEFINE) )
    {
        for( i=unknown_labels; (i=find_in_list( i, name )); )
        {
            fseek( output, i->position, SEEK_SET );
            write_label( i->position, value, i->type, output );

            /* move to resolved list and advance */
            if( i==unknown_labels ) unknown_labels = i->next;
            if( i->next           ) i->next->prev  = i->prev;
            if( i->prev           ) i->prev->next  = i->next;

            old = i;
            i = i->next;

            if( old->type & LABEL_NEED_DIFF )
            {
                FREE_NODE( old );
            }
            else
            {
                ADD_TO_LIST( resolved_labels, old );
            }
        }

        fseek( output, 0, SEEK_END );
    }

    /* create a new label */
    new_label = create_label( name, value, type );

    if( !new_label )
        return;             /* FIXME: handle error */

    ADD_TO_LIST( known_labels, new_label );
}

void require_label( const char* name, FILE* output, int type )
{
    LABEL *new_label, *i;
    unsigned long position = ftell( output );

    SKIP_SPACE( name );

    /* try to find the label */
    if( (i = find_in_list( known_labels, name )) )
    {
        write_label( position, i->position, type, output );

        if( type & LABEL_NEED_DIFF )
            return;
    }

    /* create a new label */
    new_label = create_label( name, position, type );

    if( !new_label )
        return;             /* FIXME: handle error */

    /* add the label to the correct list */
    if( i )
    {
        ADD_TO_LIST( resolved_labels, new_label );
    }
    else
    {
        ADD_TO_LIST( unknown_labels, new_label );

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

    SKIP_SPACE( name );

    if( !(i = find_in_list( known_labels, name )) ||
        !(i->type & LABEL_TYPE_DEFINE) )
        return 0;

    *value = i->position;
    return 1;
}

void reset_labels( void )
{
    free_list( unknown_labels );
    free_list( known_labels );
    free_list( resolved_labels );

    resolved_labels = NULL;
    known_labels    = NULL;
    unknown_labels  = NULL;
}

unsigned int write_unresolved_smbols( FILE* out )
{
    LABEL* i;
    unsigned int count = 0;

    for( i=unknown_labels; i; i=i->next, ++count )
    {
        fputc(  i->position     & 0xFF, out );
        fputc( (i->position>>8) & 0xFF, out );
        fputc(  i->type         & 0xFF, out );
        fputs(  i->name,                out );
        fputc( '\0',                    out );
    }

    return count;
}

unsigned int write_export_symbols( FILE* out )
{
    LABEL* i;
    unsigned int count = 0;

    for( i=known_labels; i; i=i->next )
    {
        if( i->type & LABEL_FLAG_EXPORT )
        {
            fputc(  i->position     & 0xFF, out );
            fputc( (i->position>>8) & 0xFF, out );
            fputs(  i->name,                out );
            fputc( '\0',                    out );
            ++count;
        }
    }

    return count;
}

unsigned int write_relocation_table( FILE* out )
{
    LABEL *i, *j;
    unsigned int count = 0;

    for( i=resolved_labels; i; i=i->next, ++count )
    {
        j = find_in_list( known_labels, i->name );

        fputc(  i->position     & 0xFF, out );
        fputc( (i->position>>8) & 0xFF, out );
        fputc(  j->position     & 0xFF, out );
        fputc( (j->position>>8) & 0xFF, out );
        fputc(  i->type         & 0xFF, out );
    }

    return count;
}

