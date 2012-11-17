#include "asm.h"



int main( int argc, char** argv )
{
    FILE *output, *input;
    char out_name[ 128 ];
    char* dot;
    int i;

    if( argc==1 )
    {
        printf( "Error: no input files\n" );
        return EXIT_FAILURE;
    }

    reset_labels( );

    for( i=1; i<argc; ++i )
    {
        /* generate filename for the output binary */
        strncpy( out_name, argv[i], sizeof(out_name) );

        dot = strrchr( out_name, '.' );

        if( dot )
            *dot = '\0';

        strcat( out_name, ".bin" );

        /* open input and output files */
        input = fopen( argv[i], "r" );

        if( !input )
        {
            printf( "Error: Failed to open input file '%s'\n", argv[i] );
            return EXIT_FAILURE;
        }

        output = fopen( out_name, "wb" );

        if( !output )
        {
            printf( "Error: Failed to open output file '%s'\n", out_name );
            fclose( input );
            return EXIT_FAILURE;
        }

        /* do the assembly */
        if( !assemble_file( input, output ) )
        {
            fclose( input );
            fclose( output );
            return EXIT_FAILURE;
        }

        /* cleanup */
        fclose( input );
        fclose( output );
        reset_labels( );
    }

    return EXIT_SUCCESS;
}

/***************************************************************************/
int assemble_file( FILE* input, FILE* output )
{
    char buffer[ 128 ];
    unsigned int i, j;
    unsigned long mnemonic, temp;
    char *a0, *a1;
    int is_quote = 0;

    while( !feof( input ) )
    {
        fgets( buffer, sizeof(buffer), input );

        /* skip preceeding whitespace */
        for( i=0; isspace( buffer[i] ); ++i );

        /* find end of line or commentary start */
        for( j=i; buffer[j] && buffer[j]!='\n'; ++j )
        {
            if( buffer[ j ] == '\'' )
                is_quote = ~is_quote;

            if( buffer[ j ]==';' && !is_quote )
                break;
        }

        /* skip trailing whitespaces */
        if( i!=j ) --j;
        while( isspace( buffer[ j ] ) && i!=j ) --j;

        /* conitinue if the line is empty */
        if( i>=j )
            continue;

        buffer[ j+1 ] = '\0';   /* safety first */

        /* compose mnemonic */
        mnemonic  = toupper( buffer[i++] )<<24;
        mnemonic |= toupper( buffer[i++] )<<16;

        if( i<=j && !isspace( buffer[i] ) )
            mnemonic |= toupper( buffer[i++] )<<8;

        if( i<=j && !isspace( buffer[i] ) )
            mnemonic |= toupper( buffer[i++] );

        /* get arguments */
        a0 = buffer + i;

        while( !isspace( *a0 ) && (*a0) ) ++a0;
        while(  isspace( *a0 ) && (*a0) ) ++a0;

        a1 = a0;

        while( !isspace( *a1 ) && (*a1)!=',' && (*a1) ) ++a1;
        while(  isspace( *a1 )               && (*a1) ) ++a1;

        if( *a1 == ',' )
        {
            ++a1;
            while( isspace( *a1 ) && (*a1) ) ++a1;
        }

        /* handle assembler directives */
        if( mnemonic == MK_4CC('O','R','G',0) )
        {
            read_num( a0, &temp );

            set_base_address( temp );
        }
        else if( mnemonic == MK_4CC('D','B',0,0) )
        {
            int delta=0;

            if( read_num( a0, &temp ) )
                fputc( temp & 0xFF, output );
            else if( a0[0]=='"' )
            {
                for( i=1; a0[i] && a0[i]!='"'; i+=delta )
                    fputc( read_char( a0+i, &delta ), output );
            }
        }
        else if( buffer[j]==':' )
        {
            buffer[j] = '\0';
            add_label( buffer, ftell( output ) );
        }
        else
        {
            /* assemble line */
            assemble_line_8080( mnemonic, a0, a1, output );
        }
    }

    post_process_labels( output );

    return 1;
}

/************************* common helper functions *************************/
char read_char( const char* str, int* delta )
{
    int d = 1;
    char out = str[0];

    if( str[0] == '\\' )        /* escape sequences */
    {
        d = 2;

        switch( str[1] )
        {
        case 'a':  out = '\a'; break;
        case 'b':  out = '\b'; break;
        case 'f':  out = '\f'; break;
        case 'n':  out = '\n'; break;
        case 'r':  out = '\r'; break;
        case 't':  out = '\t'; break;
        case 'v':  out = '\v'; break;
        case '\'': out = '\''; break;
        case '\\': out = '\\'; break;
        case 'x':  d = 4; out = strtol( str+2, NULL, 16 ); break;
        default:   d = 5; out = strtol( str+2, NULL,  8 ); break;
        }
    }

    if( delta )
        *delta = d;

    return out;
}

int read_num( const char* str, unsigned long* out )
{
    *out = 0;

    /* ASCII character literal */
    if( str[0] == '\'' )
    {
        *out = read_char( str+1, NULL );
        return 1;
    }

    /* integer literals ALWAYS start with a digit */
    if( !isdigit( str[0] ) )
        return 0;

    /* binary literal (0b...) */
    if( str[0]=='0' && str[1]=='b' )
        *out = strtol( str+2, NULL, 2 );
    else
        *out = strtol( str, NULL, 0 );

    return 1;
}

