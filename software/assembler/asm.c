#include "asm.h"



int main( int argc, char** argv )
{
    FILE *output, *input;
    char out_name[ 128 ];
    char* dot;
    int i;
    assemble_line_fun asm_fun = NULL;

    if( argc==1 )
    {
        printf( "Error: no input files\n" );
        return EXIT_FAILURE;
    }

    for( i=1; i<argc; ++i )
    {
        if( !strcmp( argv[i], "-mz80" ) )
        {
            asm_fun = assemble_line_z80;
            continue;
        }

        if( !strcmp( argv[i], "-mi8080" ) )
        {
            asm_fun = assemble_line_8080;
            continue;
        }

        /******** assemble a file ********/
        if( !asm_fun )
        {
            printf( "Error: No architecture specified!\n" );
            return EXIT_FAILURE;
        }

        /* generate filename for the output binary */
        strncpy( out_name, argv[i], sizeof(out_name) );

        dot = strrchr( out_name, '.' );

        if( dot )
            *dot = '\0';

        strcat( out_name, ".o" );

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
        if( !assemble_file( input, output, asm_fun ) )
        {
            fclose( input );
            fclose( output );
            return EXIT_FAILURE;
        }

        /* cleanup */
        fclose( input );
        fclose( output );
    }

    return EXIT_SUCCESS;
}

/***************************************************************************/
int assemble_file( FILE* input, FILE* output, assemble_line_fun asm_fun )
{
    char buffer[ 128 ];
    unsigned int i, j, num_imp, num_reloc, num_exp;
    unsigned long mnemonic, temp, base_address;
    unsigned long imp_start, exp_start, reloc_start;
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

            if( (buffer[ j ]==';' || buffer[ j ]=='#') && !is_quote )
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

        FIND_SPACE( a0 );
        SKIP_SPACE( a0 );

        a1 = a0;

        FIND_SPACE( a1 );
        SKIP_SPACE( a1 );

        if( *a1 == ',' )
        {
            ++a1;
            SKIP_SPACE( a1 );
        }

        /* handle assembler directives */
        switch( mnemonic )
        {
        case MK_4CC('.','O','R','G'):
            read_num( a0, &base_address );
            break;
        case MK_4CC('.','D','B',0):
        {
            int delta=0;

            if( read_num( a0, &temp ) )
                fputc( temp & 0xFF, output );
            else if( a0[0]=='"' )
            {
                for( i=1; a0[i] && a0[i]!='"'; i+=delta )
                    fputc( read_char( a0+i, &delta ), output );
            }
            break;
        }
        case MK_4CC('.','B','L','K'):
        {
            unsigned long times;

            read_num( a0, &times );
            read_num( a1, &temp  );

            temp &= 0xFF;

            for( i=0; i<times; ++i )
                fputc( temp, output );
            break;
        }
        case MK_4CC('.','L','O','C'):
        {
            unsigned long times, position;

            read_num( a0, &times );
            read_num( a1, &temp  );

            temp &= 0xFF;
            position = ftell( output );

            if( times > position )
            {
                times -= position;

                for( i=0; i<times; ++i )
                    fputc( temp, output );
            }
            break;
        }
        case MK_4CC('.','D','E','F'):
            read_num( a1, &temp );
            add_label( a0, temp, LABEL_TYPE_DEFINE, output );
            break;
        case MK_4CC('.','E','X','P'):
            add_label( a0, ftell(output), LABEL_TYPE_LABEL|LABEL_FLAG_EXPORT,
                       output );
            break;
        default:
            if( buffer[j]==':' )
                add_label( buffer, ftell(output), LABEL_TYPE_LABEL, output );
            else
                asm_fun( mnemonic, a0, a1, output );
        }
    }

    /* generate symbol tables and cleanup the symbol lists */
    imp_start   = ftell( output );
    num_imp     = write_unresolved_smbols( output );
    exp_start   = ftell( output );
    num_exp     = write_export_symbols( output );
    reloc_start = ftell( output );
    num_reloc   = write_relocation_table( output );

    reset_labels( );

    /* generate footer */
    fputc(  base_address     & 0xFF, output );
    fputc( (base_address>>8) & 0xFF, output );
    fputc(  imp_start        & 0xFF, output );
    fputc( (imp_start>>8)    & 0xFF, output );
    fputc(  exp_start        & 0xFF, output );
    fputc( (exp_start>>8)    & 0xFF, output );
    fputc(  reloc_start      & 0xFF, output );
    fputc( (reloc_start>>8)  & 0xFF, output );
    fputc(  num_imp          & 0xFF, output );
    fputc( (num_imp>>8)      & 0xFF, output );
    fputc(  num_exp          & 0xFF, output );
    fputc( (num_exp>>8)      & 0xFF, output );
    fputc(  num_reloc        & 0xFF, output );
    fputc( (num_reloc>>8)    & 0xFF, output );

    return 1;
}

/************************* common helper functions *************************/
char read_char( const char* str, int* delta )
{
    int d = 1;
    char out = str[0];
    char* end = NULL;

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
        case 'x':  out = strtol( str+2, &end, 16 ); break;
        default:   out = strtol( str+2, &end,  8 ); break;
        }
    }

    if( delta )
        *delta = end ? (end-str) : d;

    return out;
}

int read_num( const char* str, unsigned long* out )
{
    int need_neg = 0;

    *out = 0;

    /* ASCII character literal */
    if( str[0] == '\'' )
    {
        *out = read_char( str+1, NULL );
        return 1;
    }

    /* skip sign */
    if( str[0]=='-' )
    {
        need_neg = 1;
        ++str;
    }
    else if( str[0]=='+' )
    {
        ++str;
    }

    /* if it starts with a digit, read it */
    if( isdigit( str[0] ) )
    {
        /* binary literal (0b...) */
        if( str[0]=='0' && str[1]=='b' )
            *out = strtol( str+2, NULL, 2 );
        else
            *out = strtol( str, NULL, 0 );

        if( need_neg )
            *out = (~(*out)) + 1;

        return 1;
    }
    else if( str[0]=='$' )  /* alternate hex number indicator */
    {
        *out = strtol( str+1, NULL, 16 );

        if( need_neg )
            *out = (~(*out)) + 1;

        return 1;
    }

    /* last chance. Try if it is a define */
    if( get_define( str, out ) )
    {
        if( need_neg )
            *out = (~(*out)) + 1;

        return 1;
    }

    /* could not obtain value */
    return 0;
}

void imm8( const char* input, FILE* output )
{
    unsigned long temp = 0;

    if( read_num( input, &temp ) )
        fputc( temp & 0xFF, output );
    else
        require_label( input, output, LABEL_NEED_0 );
}

void diff8( const char* input, FILE* output )
{
    unsigned long temp = 0;

    if( read_num( input, &temp ) )
        fputc( temp & 0xFF, output );
    else
        require_label( input, output, LABEL_NEED_DIFF|LABEL_NEED_0 );
}

void imm16( const char* input, FILE* output, int le )
{
    unsigned long temp = 0;

    if( read_num( input, &temp ) )
    {
        if( le )
        {
            fputc(  temp     & 0xFF, output );
            fputc( (temp>>8) & 0xFF, output );
        }
        else
        {
            fputc( (temp>>8) & 0xFF, output );
            fputc(  temp     & 0xFF, output );
        }
    }
    else
    {
        require_label( input, output, le ? LABEL_NEED_01 : LABEL_NEED_10 );
    }
}

void inst( unsigned long opcode, FILE* output )
{
    if( opcode>0xFFFFFF )
        fputc( (opcode>>24) & 0xFF, output );

    if( opcode>0xFFFF )
        fputc( (opcode>>16) & 0xFF, output );

    if( opcode>0xFF )
        fputc( (opcode>>8) & 0xFF, output );

    fputc( opcode & 0xFF, output );
}

