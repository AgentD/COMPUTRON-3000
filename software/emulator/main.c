#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "z80.h"



static unsigned char rom[  1024 ];
static unsigned char ram[ 31744 ];



/* addresses for system bus devices */
#define RS232_DEVICE_ADDRESS 0x0F



/******************* memory access *******************/
byte memory_read( int param, ushort address )
{
    (void)param;

    /* A15 is high -> extended RAM */
    if( address & 0x8000 )
    {
        return 0;
    }

    /* A15 is low, address < 1024 -> ROM */
    if( address < 1024 )
        return rom[ address ];

    /* A15 is low, address >= 1024 -> RAM */
    return ram[ address-1024 ];
}

void memory_write( int param, ushort address, byte data )
{
    (void)param;

    /* A15 is high -> extended RAM */
    if( address & 0x8000 )
    {
        return;
    }

    /* A15 is low, address < 1024 -> ROM */
    if( address < 1024 )
        rom[ address ] = data;

    /* A15 is low, address >= 1024 -> RAM */
    ram[ address-1024 ] = data;
}

/******************* bus I/O *******************/
byte bus_read( int param, ushort address )
{
    (void)param;

    address &= 0x00FF;

    if( address == RS232_DEVICE_ADDRESS )
    {
        return getchar( );
    }

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
    Z80Context cpu;
    int i;

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
            FILE* romfile = fopen( argv[ i + 1 ], "rb" );

            if( !romfile )
            {
                printf( "Could not open '%s'!\n", argv[ i + 1 ] );
                return EXIT_FAILURE;
            }

            fread( rom, 1, sizeof(rom), romfile );
            fclose( romfile );
        }
        else if( !strcmp( argv[i], "-r" ) && (i<(argc-1)) )
        {
            /*ram_bytes = atoi( argv[ i + 1 ] ) * 32768;*/
        }
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

    return EXIT_SUCCESS;
}

