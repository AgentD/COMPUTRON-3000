#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "z80.h"



static unsigned int rom_bytes = 0;
static unsigned int ram_bytes = 0;

static unsigned char* rom = NULL;
static unsigned char* ram = NULL;



/* addresses for system bus devices */
#define RS232_DEVICE_ADDRESS 0x0F



/******************* memory access *******************/
byte memory_read( int param, ushort address )
{
    byte v = 0x00;
    unsigned int a = address & 0x7FFF;

    if( address & 0x1000 )
    {
        a += (((unsigned int)param) * 0xFFFF);

        if( a < ram_bytes )
            v = ram[ a ];
    }
    else if( a < rom_bytes )
    {
        v = rom[ address ];
    }

    return v;
}

void memory_write( int param, ushort address, byte data )
{
    unsigned int a = address & 0x7FFF;

    a += (((unsigned int)param) * 0xFFFF);

    if( (address & 0x1000) && (a < ram_bytes) )
    {
        ram[ a ] = data;
    }
}

/******************* bus I/O *******************/
byte bus_read( int param, ushort address )
{
    (void)param; (void)address;
    return 0;
}

void bus_write( int param, ushort address, byte data )
{
    (void)param;

    address &= 0x00FF;

    if( address == RS232_DEVICE_ADDRESS )
    {
        putchar( data );
    }
}



int main( int argc, char** argv )
{
    int i;
    FILE* romfile;
    Z80Context cpu;

    /* if no arguments, print usage info */
    if( argc == 1 )
    {
        printf( "COMPUTRON-3000 emulator\n\n" );
        printf( "usage: emu [-b bootromfile] [-r #rambanks]\n\n" );
        return EXIT_SUCCESS;
    }

    /* parse command line arguments */
    for( i=1; i<argc; ++i )
    {
        if( !strcmp( argv[i], "-b" ) && (i<(argc-1)) )
        {
            romfile = fopen( argv[ i + 1 ], "rb" );

            fseek( romfile, 0, SEEK_END );
            rom_bytes = ftell( romfile );
            fseek( romfile, 0, SEEK_SET );

            rom = malloc( rom_bytes );

            if( rom )
            {
                fread( rom, 1, rom_bytes, romfile );
            }
            else
            {
                rom_bytes = 0;
            }

            fclose( romfile );
        }
        else if( !strcmp( argv[i], "-r" ) && (i<(argc-1)) )
        {
            ram_bytes = atoi( argv[ i + 1 ] ) * 32768;

            if( ram_bytes )
            {
                ram = malloc( ram_bytes );

                if( !ram )
                {
                    printf( "failed to allocate ram memory" );
                    return EXIT_FAILURE;
                }
            }
        }
    }

    /* check if we have everything we need */
    if( !rom || !rom_bytes )
    {
        printf( "No boot rom supplied!\n" );
        free( ram );
        free( rom );
        return EXIT_FAILURE;
    }

    /* initialise the system */
    cpu.memRead  = memory_read;
    cpu.memWrite = memory_write;
    cpu.memParam = 0;

    cpu.ioRead  = bus_read;
    cpu.ioWrite = bus_write;
    cpu.ioParam = 0;

    Z80RESET( &cpu );

    printf( "\033c" );      /* reset VT100 compatible terminal (emulator) */

    /* main loop */
    while( 1 )
    {
        char buffer[ 10 ];

        /* if the next instruction is a HALT instruction, stop execution */
        Z80Debug( &cpu, NULL, buffer );

        if( !strcmp( buffer, "HALT" ) )
        {
            printf( "\n"
                    "****************************************\n"
                    "*        COMPUTRON-3000 EMULATOR       *\n"
                    "*                                      *\n"
                    "*              CPU HALTED              *\n"
                    "****************************************\n" );
            break;
        }

        /* execute next instruction */
        Z80Execute( &cpu );
    }

    /* clean up */
    free( ram );
    free( rom );

    return EXIT_SUCCESS;
}

