#include "asm.h"



unsigned long reg8( const char* reg )
{
    switch( toupper( reg[0] ) )
    {
    case 'A': return 0x07;
    case 'B': return 0x00;
    case 'C': return 0x01;
    case 'D': return 0x02;
    case 'E': return 0x03;
    case 'H': return 0x04;
    case 'L': return 0x05;
    case 'M': return 0x06;
    }

    return 0;
}

unsigned long reg16( const char* reg )
{
    switch( toupper( reg[0] ) )
    {
    case 'B': return 0x00;  /* BC */
    case 'D': return 0x01;  /* DE */
    case 'H': return 0x02;  /* HL */
    case 'P':               /* PSW */
    case 'S': return 0x03;  /* SP */
    }

    return 0;
}

void imm8( const char* input, FILE* output )
{
    unsigned long temp = 0;

    read_num( input, &temp );

    fputc( temp & 0xFF, output );
}

void imm16( const char* input, FILE* output )
{
    unsigned long temp = 0;

    if( !read_num( input, &temp ) )
        require_label( input, ftell( output ), LABEL_NEED_01 );

    fputc(  temp     & 0xFF, output );
    fputc( (temp>>8) & 0xFF, output );
}

void inst( unsigned long opcode, FILE* output )
{
    fputc( opcode & 0xFF, output );
}



void assemble_line_8080( unsigned long mnemonic, const char* a0,
                         const char* a1, FILE* out )
{
    switch( mnemonic )
    {
    case MK_4CC('M','O','V',0):   inst(0x40|reg8(a0)<<3 |reg8(a1),out); break;
    case MK_4CC('M','V','I',0):   inst(0x06|reg8(a0)<<3,out);
                                  imm8( a1, out );                      break;
    case MK_4CC('L','X','I',0):   inst(0x01|reg16(a0)<<4,out);
                                  imm16( a1, out );                     break;
    case MK_4CC('L','D','A','X'): inst(toupper(*a0)=='D'?0x1A:0x0A,out);break;
    case MK_4CC('S','T','A','X'): inst(toupper(*a0)=='D'?0x12:0x02,out);break;
    case MK_4CC('L','D','A',0):   inst(0x3A,out); imm16( a0,out );      break;
    case MK_4CC('S','T','A',0):   inst(0x32,out); imm16( a0,out );      break;
    case MK_4CC('L','H','L','D'): inst(0x2A,out); imm16( a0,out );      break;
    case MK_4CC('S','H','L','D'): inst(0x22,out); imm16( a0,out );      break;
    case MK_4CC('S','P','H','L'): inst(0xF9,out);                       break;
    case MK_4CC('X','C','H','G'): inst(0xEB,out);                       break;
    case MK_4CC('X','T','H','L'): inst(0xE3,out);                       break;
    case MK_4CC('D','I',0,0):     inst(0xF3,out);                       break;
    case MK_4CC('E','I',0,0):     inst(0xFB,out);                       break;
    case MK_4CC('N','O','P',0):   inst(0x00,out);                       break;
    case MK_4CC('H','L','T',0):   inst(0x76,out);                       break;
    case MK_4CC('A','D','D',0):   inst(0x80 | reg8( a0 ),out);          break;
    case MK_4CC('A','D','C',0):   inst(0x81 | reg8( a0 ),out);          break;
    case MK_4CC('S','U','B',0):   inst(0x90 | reg8( a0 ),out);          break;
    case MK_4CC('S','B','B',0):   inst(0x91 | reg8( a0 ),out);          break;
    case MK_4CC('A','D','I',0):   inst(0xC6,out); imm8( a0,out );       break;
    case MK_4CC('A','C','I',0):   inst(0xCE,out); imm8( a0,out );       break;
    case MK_4CC('S','U','I',0):   inst(0xD6,out); imm8( a0,out );       break;
    case MK_4CC('S','B','I',0):   inst(0xDE,out); imm8( a0,out );       break;
    case MK_4CC('I','N','R',0):   inst(0x04 | reg8( a0 )<<3,out);       break;
    case MK_4CC('D','C','R',0):   inst(0x05 | reg8( a0 )<<3,out);       break;
    case MK_4CC('D','A','D',0):   inst(0x09 | reg16(a0)<<4, out);
                                  imm16( a1, out );                     break;
    case MK_4CC('I','N','X',0):   inst(0x03 | reg16( a0 )<<4,out);      break;
    case MK_4CC('D','C','X',0):   inst(0x0B | reg16( a0 )<<4,out);      break;
    case MK_4CC('D','A','A',0):   inst(0x27,out);                       break;
    case MK_4CC('C','M','A',0):   inst(0x2F,out);                       break;
    case MK_4CC('S','T','C',0):   inst(0x37,out);                       break;
    case MK_4CC('C','M','C',0):   inst(0x3F,out);                       break;
    case MK_4CC('R','L','C',0):   inst(0x07,out);                       break;
    case MK_4CC('R','R','C',0):   inst(0x0F,out);                       break;
    case MK_4CC('R','A','L',0):   inst(0x17,out);                       break;
    case MK_4CC('R','A','R',0):   inst(0x1F,out);                       break;
    case MK_4CC('A','N','A',0):   inst(0xA0 | reg8( a0 ),out);          break;
    case MK_4CC('X','R','A',0):   inst(0xA8 | reg8( a0 ),out);          break;
    case MK_4CC('O','R','A',0):   inst(0xB0 | reg8( a0 ),out);          break;
    case MK_4CC('C','M','P',0):   inst(0xB8 | reg8( a0 ),out);          break;
    case MK_4CC('A','N','I',0):   inst(0xE6,out); imm8( a0,out );       break;
    case MK_4CC('X','R','I',0):   inst(0xEE,out); imm8( a0,out );       break;
    case MK_4CC('O','R','I',0):   inst(0xF6,out); imm8( a0,out );       break;
    case MK_4CC('C','P','I',0):   inst(0xFE,out); imm8( a0,out );       break;
    case MK_4CC('J','M','P',0):   inst(0xC3,out); imm16( a0,out );      break;
    case MK_4CC('J','N','Z',0):   inst(0xC2,out); imm16( a0,out );      break;
    case MK_4CC('J','Z',0,0):     inst(0xCA,out); imm16( a0,out );      break;
    case MK_4CC('J','N','C',0):   inst(0xD2,out); imm16( a0,out );      break;
    case MK_4CC('J','C',0,0):     inst(0xDA,out); imm16( a0,out );      break;
    case MK_4CC('J','P','O',0):   inst(0xE2,out); imm16( a0,out );      break;
    case MK_4CC('J','P','E',0):   inst(0xEA,out); imm16( a0,out );      break;
    case MK_4CC('J','P',0,0):     inst(0xF2,out); imm16( a0,out );      break;
    case MK_4CC('J','M',0,0):     inst(0xFA,out); imm16( a0,out );      break;
    case MK_4CC('P','C','H','L'): inst(0xE9,out);                       break;
    case MK_4CC('C','A','L','L'): inst(0xCD,out); imm16( a0,out );      break;
    case MK_4CC('C','N','Z',0):   inst(0xC4,out); imm16( a0,out );      break;
    case MK_4CC('C','Z',0,0):     inst(0xCC,out); imm16( a0,out );      break;
    case MK_4CC('C','N','C',0):   inst(0xD4,out); imm16( a0,out );      break;
    case MK_4CC('C','C',0,0):     inst(0xDC,out); imm16( a0,out );      break;
    case MK_4CC('C','P','O',0):   inst(0xE4,out); imm16( a0,out );      break;
    case MK_4CC('C','P','E',0):   inst(0xEC,out); imm16( a0,out );      break;
    case MK_4CC('C','P',0,0):     inst(0xF4,out); imm16( a0,out );      break;
    case MK_4CC('C','M',0,0):     inst(0xFC,out); imm16( a0,out );      break;
    case MK_4CC('R','E','T',0):   inst(0xC9,out);                       break;
    case MK_4CC('R','N','Z',0):   inst(0xC0,out);                       break;
    case MK_4CC('R','Z',0,0):     inst(0xC8,out);                       break;
    case MK_4CC('R','N','C',0):   inst(0xD0,out);                       break;
    case MK_4CC('R','C',0,0):     inst(0xD8,out);                       break;
    case MK_4CC('R','P','O',0):   inst(0xE0,out);                       break;
    case MK_4CC('R','P','E',0):   inst(0xE8,out);                       break;
    case MK_4CC('R','P',0,0):     inst(0xF0,out);                       break;
    case MK_4CC('R','M',0,0):     inst(0xF8,out);                       break;
    case MK_4CC('I','N',0,0):     inst(0xDB,out); imm8( a0,out );       break;
    case MK_4CC('O','U','T',0):   inst(0xD3,out); imm8( a0,out );       break;
    case MK_4CC('P','U','S','H'): inst(0xC5 | reg16( a0 )<<4,out);      break;
    case MK_4CC('P','O','P',0):   inst(0xC1 | reg16( a0 )<<4,out);      break;
    case MK_4CC('R','S','T',0):
        inst(0xC7 | (((*a0) - '0')>>1)<<4 | (((*a0) - '0') % 2),out);
        break;
    }
}

