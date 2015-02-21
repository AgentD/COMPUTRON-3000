#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>



#define ROMSIZE 1024



static void usage( void )
{
    puts("romprog <devicefile> -w <romdump>\n"
         "  Write a binary ROM blob to a rom programmer at a device file\n\n"
         "romprog <devicefile> -r <romdump>\n"
         "  Read a binary ROM blob from a rom programmer to a file\n");
}

static int set_interface_attribs( int fd, int speed, int parity )
{
    struct termios tty;

    memset( &tty, 0, sizeof(tty) );

    if( tcgetattr(fd, &tty) != 0 )
        return 0;

    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag &= ~IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 5;

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if( tcsetattr(fd, TCSANOW, &tty) != 0 )
        return 0;

    return 1;
}



int main( int argc, char** argv )
{
    unsigned char data[ ROMSIZE ], indata[ ROMSIZE ];
    int i, fd, dumpfile, writerom = 0;
    struct stat sb;
    char hex[2];

    /* process comman line arguments */
    if( argc != 4 )
    {
        usage( );
        return EXIT_SUCCESS;
    }

    if( !strcmp( argv[2], "-r" ) )
    {
        writerom = 0;
    }
    else if( !strcmp( argv[2], "-w" ) )
    {
        writerom = 1;
    }
    else
    {
        usage( );
        return EXIT_SUCCESS;
    }

    /* open tty */
    fd = open( argv[1], O_RDWR|O_NOCTTY|O_SYNC );

    if( fd < 0 )
    {
        fprintf( stderr, "Error opening %s: %s\n", argv[1], strerror(errno) );
        return EXIT_FAILURE;
    }

    if( !set_interface_attribs( fd, B9600, 0 ) )
    {
        fprintf( stderr, "Error setting baud rate: %s\n", strerror(errno) );
        close( fd );
        return EXIT_FAILURE;
    }

    /* open ROM dump file */
    dumpfile = open( argv[3],
                     writerom ? (O_WRONLY|O_CREAT|O_TRUNC) : O_RDONLY, 0775 );

    if( dumpfile < 0 )
    {
        fprintf( stderr, "Error opening %s: %s", argv[3], strerror(errno) );
        close( fd );
        return EXIT_FAILURE;
    }

    if( writerom && fstat( dumpfile, &sb )!=0 )
    {
        fprintf( stderr, "Failed reading ROM dump size: %s\n",
                 strerror(errno) );
        close( fd );
        close( dumpfile );
        return EXIT_FAILURE;
    }

    /* write data to ROM if requested */
    memset( data, 0, ROMSIZE );
    memset( indata, 0, ROMSIZE );

    if( writerom )
    {
        read( dumpfile, data, sb.st_size<ROMSIZE ? sb.st_size : ROMSIZE );

        puts( "WRITING..." );
        write( fd, "WRITE\n", 6 );

        for( i=0; i<ROMSIZE; ++i )
        {
            hex[0] = (data[i]>>8) & 0x0F;
            hex[1] = data[i] & 0x0F;

            hex[0] = hex[0]<10 ? (hex[0]+'0') : (hex[0]-10+'A');
            hex[1] = hex[1]<10 ? (hex[1]+'0') : (hex[1]-10+'A');

            write( fd, hex, 2 );
        }
    }

    /* read data from ROM */
    puts( "READING..." );
    write( fd, "READ\n", 5 );

    for( i=0; i<ROMSIZE; ++i )
    {
        read( fd, hex, 2 );

        if( hex[0]>='0' && hex[0]<='9' )
            indata[i] |= (hex[0]-'0')<<4;
        else if( hex[0]>='A' && hex[0]<='F' )
            indata[i] |= (hex[0]-'A')<<4;

        if( hex[1]>='0' && hex[1]<='9' )
            indata[i] |= (hex[1]-'0');
        else if( hex[0]>='A' && hex[0]<='F' )
            indata[i] |= (hex[1]-'A');
    }

    /* process data read from ROM */
    if( writerom )
    {
        if( memcmp( indata, data, ROMSIZE )!=0 )
        {
            fputs( "VERIFY FAILED", stderr );
        }
        else
        {
            puts( "VERIFIED" );
        }
    }
    else
    {
        write( dumpfile, indata, ROMSIZE );
    }

    /* cleanup */
    close( fd );
    close( dumpfile );
    return EXIT_SUCCESS;
}

