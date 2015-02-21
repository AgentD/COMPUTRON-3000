#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <stdint.h>



#define UART_BAUD_RATE 9600UL
#define ROMSIZE 1024



static int uart_init( void )
{
    uint16_t ubrr = (uint16_t)(F_CPU/(16*UART_BAUD_RATE) - 1);
    int ret = 0;

    DDRD &= ~(1<<PD0);                  /* make RXD input */
    DDRD |= (1<<PD1);                   /* make TXD output */

    UBRRH = (unsigned char)(ubrr>>8);             /* set UART baudrate */
    UBRRL = (unsigned char) ubrr;

    UCSRB = (1<<RXEN) | (1<<TXEN);                /* enable UART RX & TX */
    UCSRC = (1<<URSEL) | (1<<UCSZ1) | (1<<UCSZ0); /* 8 bit data, 1 stop bit */

    /* flush receive buffers */
    do
    {
        ret = UDR;
    }
    while( UCSRA & (1<<RXC) );

    return ret;
}

static void uart_putc( char c )
{
    while( !(UCSRA & (1<<UDRE)) ); /* wait for UART to get ready */
    UDR = c;                       /* send */
}

static char uart_getc( void )
{
    while( !(UCSRA & (1<<RXC)) );  /* wait for UART to receive something */
    return UDR;                    /* return received byte */
}

/****************************************************************************/

/*
    REG0:
        PC4 -> ROM-A12 -> Z80_A8
        PC3 -> ROM-A6  -> Z80_A6
        PC2 -> ROM-A4  -> Z80_A4
        PC1 -> ROM-A2  -> Z80_A2
        PC0 -> ROM-A0  -> Z80_A0

    REG1:
        PC4 -> ROM-A14 -> Z80_A9
        PC3 -> ROM-A7  -> Z80_A7
        PC2 -> ROM-A5  -> Z80_A5
        PC1 -> ROM-A3  -> Z80_A3
        PC0 -> ROM-A1  -> Z80_A1

    PB5 -> REG0_CLK
    PB4 -> REG1_CLK
 */
#define ADDRMASK ((1<<PC0)|(1<<PC1)|(1<<PC2)|(1<<PC3)|(1<<PC4))

static void set_address( unsigned short address )
{
    DDRC |= ADDRMASK;                       /* make address lines output */
    PORTC &= ~ADDRMASK;                     /* clear address lines*/

    PORTC |= (address & (1<< 0)) ? (1<<PC0) : 0; /* set first half address */
    PORTC |= (address & (1<< 2)) ? (1<<PC1) : 0;
    PORTC |= (address & (1<< 4)) ? (1<<PC2) : 0;
    PORTC |= (address & (1<< 6)) ? (1<<PC3) : 0;
    PORTC |= (address & (1<<12)) ? (1<<PC4) : 0;

    PORTB &= ~(1<<PB5);                     /* pull REG0_CLK low */
    _delay_us( 50 );                        /* wait a bit */
    PORTB |= (1<<PB5);                      /* pull REG0_CLK high */
    _delay_us( 50 );                        /* wait a bit */

    PORTC &= ~ADDRMASK;                     /* clear address lines */
    PORTC |= (address & (1<<1)) ? (1<<PC0) : 0;  /* set second half */
    PORTC |= (address & (1<<3)) ? (1<<PC1) : 0;
    PORTC |= (address & (1<<5)) ? (1<<PC2) : 0;
    PORTC |= (address & (1<<7)) ? (1<<PC3) : 0;
    PORTC |= (address & (1<<9)) ? (1<<PC4) : 0;

    PORTB &= ~(1<<PB4);                     /* pull REG1_CLK low */
    _delay_us( 50 );                        /* wait a bit */
    PORTB |= (1<<PB4);                      /* pull REG1_CLK high */
    _delay_us( 50 );                        /* wait a bit */

    DDRC &= ~ADDRMASK;                      /* make address lines input */
}

/*
    PC0 -> ROM-D0 -> Z80_D5
    PC1 -> ROM-D7 -> Z80_D4
    PC2 -> ROM-D1 -> Z80_D6
    PC3 -> ROM-D6 -> Z80_D3
    PC4 -> ROM-D2 -> Z80_D2
    PC5 -> ROM-D5 -> Z80_D7
    PB1 -> ROM-D3 -> Z80_D1
    PB2 -> ROM-D4 -> Z80_D0

    PB0 -> OE
    PD7 -> WR
    PB3 -> CS
 */
#define DATA_MASK_C ((1<<PC0)|(1<<PC1)|(1<<PC2)|(1<<PC3)|(1<<PC4)|(1<<PC5))
#define DATA_MASK_B ((1<<PB1)|(1<<PB2))

#define MAX_WRITE_TIME_MS 11

static void write_byte( unsigned char byte )
{
    DDRB |= DATA_MASK_B;                    /* make data lines outputs */
    DDRC |= DATA_MASK_C;

    PORTB &= ~DATA_MASK_B;                  /* clear data lines */
    PORTC &= ~DATA_MASK_C;

    PORTB |= (byte & 0x01) ? (1<<PB2) : 0;  /* set data byte */
    PORTB |= (byte & 0x02) ? (1<<PB1) : 0;
    PORTC |= (byte & 0x04) ? (1<<PC4) : 0;
    PORTC |= (byte & 0x08) ? (1<<PC3) : 0;
    PORTC |= (byte & 0x10) ? (1<<PC1) : 0;
    PORTC |= (byte & 0x20) ? (1<<PC0) : 0;
    PORTC |= (byte & 0x40) ? (1<<PC2) : 0;
    PORTC |= (byte & 0x80) ? (1<<PC5) : 0;

    PORTD &= ~(1<<PD7);                     /* enable write */
    PORTB &= ~(1<<PB3);                     /* enable chip */
    _delay_ms( MAX_WRITE_TIME_MS );
    PORTD |= (1<<PD7);                      /* disable write */
    PORTB |= (1<<PB3);                      /* disable chip */

    DDRC &= ~DATA_MASK_C;                  /* make data lines inputs */
    DDRB &= ~DATA_MASK_B;
}

static unsigned char read_byte( void )
{
    unsigned char byte = 0;

    PORTB &= ~((1<<PB0)|(1<<PB3));          /* enable output and chip */
    _delay_us( 10 );

    byte |= (PINB & (1<<PB2)) ? 0x01 : 0;   /* read data byte */
    byte |= (PINB & (1<<PB1)) ? 0x02 : 0;
    byte |= (PINC & (1<<PC4)) ? 0x04 : 0;
    byte |= (PINC & (1<<PC3)) ? 0x08 : 0;
    byte |= (PINC & (1<<PC1)) ? 0x10 : 0;
    byte |= (PINC & (1<<PC0)) ? 0x20 : 0;
    byte |= (PINC & (1<<PC2)) ? 0x40 : 0;
    byte |= (PINC & (1<<PC5)) ? 0x80 : 0;

    PORTB |= ((1<<PB0)|(1<<PB3));           /* disable output and chip */

    return byte;
}

static void rom_init( void )
{
    DDRC &= ~(ADDRMASK|DATA_MASK_C);    /* make address/data lines inputs */
    DDRB &= ~DATA_MASK_B;

    PORTB |= (1<<PB0)|(1<<PB3);         /* disable output and chip */
    PORTD |= (1<<PD7);                  /* disable write */

    DDRB |= (1<<PB0)|(1<<PB3);          /* make control lines outputs */
    DDRD |= (1<<PD7);
}

/****************************************************************************/

static unsigned char read_hex_byte( void )
{
    char c, out = 0;

    c = uart_getc( );
    c = (c>='0' && c<='9') ? (c-'0') : ((c>='A' && c<='F') ? (c-'A'+10) : 0);
    out = c<<4;

    c = uart_getc( );
    c = (c>='0' && c<='9') ? (c-'0') : ((c>='A' && c<='F') ? (c-'A'+10) : 0);
    out |= c;

    return out;
}

static void write_hex_byte( unsigned char c )
{
    unsigned char nibble;

    nibble = (c>>4) & 0x0F;
    uart_putc( (nibble<10) ? (nibble+'0') : (nibble-10+'A') );

    nibble = c & 0x0F;
    uart_putc( (nibble<10) ? (nibble+'0') : (nibble-10+'A') );
}


int main( void )
{
    unsigned char data[ ROMSIZE ];
    char c, command[ 8 ];
    int i = 0;

    rom_init( );
    uart_init( );

    while( 1 )
    {
        /* receive command byte */
        c = uart_getc( );
        command[ i++ ] = c;
        i = i>=sizeof(command) ? 0 : i;

        /* process command */
        if( c == '\n' )
        {
            if( !strncmp( command, "WRITE", 5 ) )
            {
                /* read & decode hexdump from UART */
                for( i=0; i<ROMSIZE; ++i )
                    data[ i ] = read_hex_byte( );

                /* write to ROM */
                for( i=0; i<ROMSIZE; ++i )
                {
                    set_address( i );
                    write_byte( data[i] );
                }
            }
            else if( !strncmp( command, "READ", 4 ) )
            {
                /* read from ROM */
                for( i=0; i<ROMSIZE; ++i )
                {
                    set_address( i );
                    data[i] = read_byte( );
                }

                /* write hexdump to UART */
                for( i=0; i<ROMSIZE; ++i )
                    write_hex_byte( data[ i ] );
            }

            i = 0;
        }
    }

    return 0;
}

