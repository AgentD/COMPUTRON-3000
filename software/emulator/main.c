/*
 * main.c
 * This file is part of COMPUTRON 3000 emulator
 *
 * Copyright (C) 2013 - David Oberhollenzer
 *
 * COMPUTRON 3000 emulator is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * COMPUTRON 3000 emulator is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with COMPUTRON 3000 emulator.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "z80.h"



/* addresses for system bus devices */
#define RS232_DEVICE_ADDRESS 0x0F

#define BANK_SWITCH_REGISTER 0x00



static byte ROM[ 1024 ];
static byte KRAM[ 31744 ];
static byte* RAM = NULL;

static int ramsize = 0;
static int bank_reg = 0xFF;



/******************* memory access *******************/
byte memory_read( int param, ushort address )
{
    int ROMSEL = (address<1024);
    int MREQ_LO = !(address & 0x8000);
    int address_long;
    (void)param;

    if( MREQ_LO )
    {
        return ROMSEL ? ROM[ address ] : KRAM[ address-1024 ];
    }
    else
    {
        address_long = (address & 0x7FFF) | (bank_reg << 15);

        if( RAM && address_long<ramsize )
            return RAM[ address_long ];
    }

    return 0xFF;
}

void memory_write( int param, ushort address, byte data )
{
    int ROMSEL = (address<1024);
    int MREQ_LO = !(address & 0x8000);
    int address_long;
    (void)param;

    if( MREQ_LO )
    {
        if( !ROMSEL )
            KRAM[ address-1024 ] = data;
    }
    else
    {
        address_long = (address & 0x7FFF) | (bank_reg << 15);

        if( RAM && address_long<ramsize )
            RAM[ address_long ] = data;
    }
}

/******************* bus I/O *******************/
byte bus_read( int param, ushort address )
{
    (void)param;

    switch( address & 0xFF )
    {
    case RS232_DEVICE_ADDRESS:
        return getchar( );
    }

    return 0xFF;
}

void bus_write( int param, ushort address, byte data )
{
    (void)param;

    switch( address & 0xFF )
    {
    case BANK_SWITCH_REGISTER:
        bank_reg = data;
        break;
    case RS232_DEVICE_ADDRESS:
        putchar( data );
        break;
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

            fread( ROM, 1, sizeof(ROM), romfile );
            fclose( romfile );
        }
        else if( !strcmp( argv[i], "-r" ) && (i<(argc-1)) )
        {
            ramsize = atoi( argv[ i + 1 ] ) * 32768;
            RAM = malloc( ramsize );
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

    free( RAM );

    return EXIT_SUCCESS;
}

