#include "asm.h"



#define BC 0x00
#define DE 0x01
#define HL 0x02
#define SP 0x03

#define IX 0x02
#define IY 0x02

#define A 0x07
#define B 0x00
#define C 0x01
#define D 0x02
#define E 0x03
#define H 0x04
#define L 0x05

#define INV 0xFF



static unsigned long reg16( const char* reg )
{
    char a = toupper( reg[0] ), b = toupper( reg[1] );

    switch( a )
    {
    case 'B': return b=='C' ? BC : INV;           /* BC */
    case 'D': return b=='E' ? DE : INV;           /* DE */
    case 'I': return b=='X' || b=='Y' ? HL : INV; /* IX, IY */
    case 'H': return b=='L' ? HL : INV;           /* HL */
    case 'S': return b=='P' ? SP : INV;           /* SP */
    }

    return INV;
}

static unsigned long reg8( const char* reg )
{
    switch( toupper( reg[0] ) )
    {
    case 'A': return A;
    case 'B': return B;
    case 'C': return C;
    case 'D': return D;
    case 'E': return E;
    case 'H': return H;
    case 'L': return L;
    }

    return INV;
}

static void gen_prefix( char test, FILE* out )
{
    if( test=='x' || test=='X' )
        inst( 0xDD, out );
    else if( test=='y' || test=='Y' )
        inst( 0xFD, out );
}

static void alu_compose( const char* src, unsigned long opcode, int shift,
                         FILE* out )
{
    unsigned long reg = reg8( src );

    if( reg < INV )
    {
        inst( opcode | ((reg & 0x07)<<shift), out );
    }
    else if( *src == '(' )
    {
        if( src[1]=='I' || src[1]=='i' )
        {
            gen_prefix( src[2], out );

            if( opcode > 0xFF )
            {
                inst( (opcode>>8) & 0xFF, out );
                diff8( src+3, out );
                inst( (opcode & 0xFF) | (0x06<<shift), out );
            }
            else
            {
                inst( opcode | (0x06<<shift), out );
                diff8( src+3, out );
            }
        }
        else
        {
            inst( opcode | (0x06<<shift), out );
        }
    }
    else
    {
        inst( opcode | (0x46<<shift), out );
        imm8( src, out );
    }
}

static int conditional( const char* cond, unsigned long opcode, FILE* out )
{
    switch( toupper( cond[0] ) )
    {
    case 'N':
        if( cond[1]=='C' || cond[1]=='c' )
            opcode |= 0x02 << 3;
        break;
    case 'Z':
        opcode |= 0x01 << 3;
        break;
    case 'C':
        opcode |= 0x03 << 3;
        break;
    case 'P':
        if( cond[1]=='O' || cond[1]=='o' )
            opcode |= 0x04 << 3;
        else if( cond[1]=='E' || cond[1]=='e' )
            opcode |= 0x05 << 3;
        else
            opcode |= 0x06 << 3;
        break;
    case 'M':
        opcode |= 0x07 << 3;
        break;
    default:
        return 0;
    }

    inst( opcode, out );
    return 1;
}



void assemble_line_z80( unsigned long mnemonic, const char* a0,
                        const char* a1, FILE* out )
{
    unsigned long a, b;

    switch( mnemonic )
    {
    case MK_4CC('L','D',0,0):
        a = reg16( a0 );
        b = reg16( a1 );

        if( a==SP && b==HL )       /* LD SP, <HL|IX|IY> */
        {
            gen_prefix( a1[1], out );
            inst( 0xF9, out );
        }
        else if( a<INV && b==INV )     /* load 16 bit register */
        {
            gen_prefix( a0[1], out );

            if( *a1 == '(' )            /* LD <reg>, (address) */
            {
                ++a1;

                if( a==HL )
                    inst( 0x2A, out );
                else
                    inst( 0xED4B | ((a & 0x07)<<4), out );
            }
            else                        /* LD <reg>, immediate */
                inst( 0x01 | ((a & 0x07)<<4), out );

            imm16( a1, out, 1 );
        }
        else if( b<INV && a==INV )  /* store 16 bit register */
        {
            if( *a0 == '(' ) ++a0;

            gen_prefix( a1[1], out );

            if( b==HL )
                inst( 0x22, out );
            else
                inst( 0xED43 | ((b & 0x07)<<4), out );

            imm16( a0, out, 1 );
        }
        else if( *a1 == '(' )      /* load 8 bit */
        {
            ++a1;

            a = reg8( a0 );

            if( a == A )
            {
                if( a1[0]=='I' || a1[0]=='i' )      /* LD A, (IX|IY+index) */
                {
                    gen_prefix( a1[1], out );
                    inst( 0x7E, out );
                    imm8( a1+2, out );
                }
                else if( a1[0]=='h' || a1[0]=='H' ) /* LD A, (HL) */
                    inst( 0x7E, out );
                else if( a1[0]=='b' || a1[0]=='B' ) /* LD A, (BC) */
                    inst( 0x0A, out );
                else if( a1[0]=='d' || a1[0]=='D' ) /* LD A, (DE) */
                    inst( 0x1A, out );
                else                                /* LD A, (address) */
                {
                    inst( 0x3A, out );
                    imm16( a1, out, 1 );
                }
            }
            else                           /* LD <reg>, (HL|IX+idx|IY+idx) */
            {
                gen_prefix( a1[1], out );
                inst( 0x46 | ((a & 0x07)<<3), out );

                if( a1[0]=='I' || a1[0]=='i' )
                    imm8( a1+2, out );
            }
        }
        else if( *a0 == '(' )       /* store 8 bit */
        {
            ++a0;
            a = reg16( a0 );

            if( a==BC )             /* LD (BC), A */
                inst( 0x02, out );
            else if( a==DE )        /* LD (DE), A */
                inst( 0x12, out );
            else if( a==HL )
            {
                b = reg8( a1 );

                gen_prefix( a0[1], out );

                if( b<INV )        /* LD (HL|IX+idx|IY+idx), <reg> */
                {
                    inst( 0x70 | b, out );

                    if( a0[0]=='i' || a0[0]=='I' )
                        imm8( a0+2, out );
                }
                else                    /* LD (HL|IX+idx|IY+idx), immediate */
                {
                    inst( 0x36, out );

                    if( a0[0]=='i' || a0[0]=='I' )
                        imm8( a0+2, out );

                    imm8( a1, out );
                }
            }
            else                        /* LD (address), A */
            {
                inst( 0x32, out );
                imm16( a0, out, 1 );
            }
        }
        else if( *a1=='I' || *a1=='i' )      /* LD A, I */
            inst( 0xED57, out );
        else if( *a1=='R' || *a1=='r' ) /* LD A, R */
            inst( 0xED5F, out );
        else if( *a0=='I' || *a0=='i' ) /* LD I, A */
            inst( 0xED47, out );
        else if( *a0=='R' || *a0=='r' ) /* LD R, A */
            inst( 0xED4F, out );
        else
        {
            a = reg8( a0 );
            b = reg8( a1 );

            if( a<INV && b==INV )       /* LD <reg>, immediate */
            {
                inst( 0x06 | ((a & 0x07)<<3), out );
                imm8( a1, out );
            }
            else if( a<INV && b<INV )   /* LD <reg>, <reg> */
                inst( 0x40 | ((a & 0x07)<<3) | (b & 0x07), out );
        }
        break;
    case MK_4CC('E','X',0,0):
        if( *a0=='(' )                  /* EX (SP), <HL|IX|IY> */
        {
            gen_prefix( a1[1], out );
            inst( 0xE3, out );
        }
        else if( *a0=='a' || *a0=='A' ) /* EX AF, AF' */
            inst( 0x08, out );
        else                            /* EX DE, HL  */
            inst( 0xEB, out );
        break;
    case MK_4CC('A','D','D',0):
        if( *a0=='A' || *a0=='a' )
        {
            alu_compose( a1, 0x80, 0, out );
        }
        else
        {
            gen_prefix( a0[1], out );
            inst( 0x09 | ((reg16( a1 ) & 0x07)<<4), out );
        }
        break;
    case MK_4CC('A','D','C',0):
        if( *a0=='A' || *a0=='a' )
            alu_compose( a1, 0x88, 0, out );
        else
            inst( 0xED4A | ((reg16( a1 ) & 0x07)<<4), out );
        break;
    case MK_4CC('S','B','C',0):
        if( a0[1]=='L' )
            inst( 0xED42 | ((reg16( a1 ) & 0x07)<<4), out );
        else
            alu_compose( a0, 0x98, 0, out );
        break;

    case MK_4CC('E','X','X',0):   inst( 0xD9,   out ); break;
    case MK_4CC('D','I',0,0):     inst( 0xF3,   out ); break;
    case MK_4CC('E','I',0,0):     inst( 0xFB,   out ); break;
    case MK_4CC('N','O','P',0):   inst( 0x00,   out ); break;
    case MK_4CC('H','L','T',0):   inst( 0x76,   out ); break;
    case MK_4CC('D','A','A',0):   inst( 0x27,   out ); break;
    case MK_4CC('C','P','L',0):   inst( 0x2F,   out ); break;
    case MK_4CC('S','C','F',0):   inst( 0x37,   out ); break;
    case MK_4CC('C','C','F',0):   inst( 0x3F,   out ); break;
    case MK_4CC('N','E','G',0):   inst( 0xED44, out ); break;
    case MK_4CC('R','L','C','A'): inst( 0x07,   out ); break;
    case MK_4CC('R','R','C','A'): inst( 0x07,   out ); break;
    case MK_4CC('R','L','A',0):   inst( 0x17,   out ); break;
    case MK_4CC('R','R','A',0):   inst( 0x1F,   out ); break;
    case MK_4CC('R','L','D',0):   inst( 0xED6F, out ); break;
    case MK_4CC('R','R','D',0):   inst( 0xED67, out ); break;
    case MK_4CC('C','P','I',0):   inst( 0xEDA1, out ); break;
    case MK_4CC('C','P','I','R'): inst( 0xEDB1, out ); break;
    case MK_4CC('C','P','D',0):   inst( 0xEDA9, out ); break;
    case MK_4CC('C','P','D','R'): inst( 0xEDB9, out ); break;
    case MK_4CC('R','E','T','I'): inst( 0xED4D, out ); break;
    case MK_4CC('R','E','T','N'): inst( 0xED45, out ); break;
    case MK_4CC('I','N','I',0):   inst( 0xEDA2, out ); break;
    case MK_4CC('I','N','I','R'): inst( 0xEDB2, out ); break;
    case MK_4CC('I','N','D',0):   inst( 0xEDAA, out ); break;
    case MK_4CC('I','N','D','R'): inst( 0xEDBA, out ); break;
    case MK_4CC('O','U','T','I'): inst( 0xEDA3, out ); break;
    case MK_4CC('O','T','I','R'): inst( 0xEDB3, out ); break;
    case MK_4CC('O','U','T','D'): inst( 0xEDAB, out ); break;
    case MK_4CC('O','T','D','R'): inst( 0xEDBB, out ); break;
    case MK_4CC('L','D','I',0):   inst( 0xEDA0, out ); break;
    case MK_4CC('L','D','I','R'): inst( 0xEDB0, out ); break;
    case MK_4CC('L','D','D',0):   inst( 0xEDA8, out ); break;
    case MK_4CC('L','D','D','R'): inst( 0xEDB8, out ); break;
    case MK_4CC('D','J','N','Z'): inst( 0x10,   out ); diff8(a0, out); break;

    case MK_4CC('S','U','B',0): alu_compose( a0, 0x90,   0, out ); break;
    case MK_4CC('S','L','A',0): alu_compose( a0, 0xCB20, 0, out ); break;
    case MK_4CC('S','R','A',0): alu_compose( a0, 0xCB28, 0, out ); break;
    case MK_4CC('S','R','L',0): alu_compose( a0, 0xCB38, 0, out ); break;
    case MK_4CC('R','L','C',0): alu_compose( a0, 0xCB00, 0, out ); break;
    case MK_4CC('R','L',0,0):   alu_compose( a0, 0xCB10, 0, out ); break;
    case MK_4CC('R','R','C',0): alu_compose( a0, 0xCB08, 0, out ); break;
    case MK_4CC('R','R',0,0):   alu_compose( a0, 0xCB18, 0, out ); break;
    case MK_4CC('A','N','D',0): alu_compose( a0, 0xA0,   0, out ); break;
    case MK_4CC('O','R',0,0):   alu_compose( a0, 0xB0,   0, out ); break;
    case MK_4CC('X','O','R',0): alu_compose( a0, 0xA8,   0, out ); break;
    case MK_4CC('C','P',0,0):   alu_compose( a0, 0xB8,   0, out ); break;
    case MK_4CC('B','I','T',0):
        alu_compose( a1, 0xCB40 | (((*a0 - '0') & 0x07)<<3), 0, out );
        break;
    case MK_4CC('R','E','S',0):
        alu_compose( a1, 0xCB80 | (((*a0 - '0') & 0x07)<<3), 0, out );
        break;
    case MK_4CC('S','E','T',0):
        alu_compose( a1, 0xCBC0 | (((*a0 - '0') & 0x07)<<3), 0, out );
        break;

    case MK_4CC('I','M',0,0):
        inst( 0xED, out );
        if( *a1=='0' )
            inst( 0x46, out );
        else if( *a1=='1' )
            inst( 0x56, out );
        else if( *a1=='2' )
            inst( 0x5E, out );
        break;
    case MK_4CC('I','N','C',0):
        a = reg16( a0 );

        if( a<INV )
        {
            gen_prefix( a0[1], out );
            inst( 0x03 | ((a & 0x07)<<4), out );
        }
        else
            alu_compose( a0, 0x04, 3, out );
        break;
    case MK_4CC('D','E','C',0):
        a = reg16( a0 );

        if( a<INV )
        {
            gen_prefix( a0[1], out );
            inst( 0x0B | ((a & 0x07)<<4), out );
        }
        else
            alu_compose( a0, 0x05, 3, out );
        break;
    case MK_4CC('J','P',0,0):
        if( *a0 == '(' )
        {
            gen_prefix( a0[2], out );
            inst( 0xE9, out );
        }
        else if( conditional( a0, 0xC2, out ) )
        {
            imm16( a1, out, 1 );
        }
        else
        {
            inst( 0xC3, out );
            imm16( a0, out, 1 );
        }
        break;
    case MK_4CC('J','R',0,0):
        if( !conditional( a0, 0x20, out ) )
        {
            inst( 0x18, out );
            diff8( a0, out );
        }
        else
            diff8( a1, out );
        break;
    case MK_4CC('C','A','L','L'):
        if( conditional( a0, 0xC4, out ) )
        {
            imm16( a1, out, 1 );
        }
        else
        {
            inst( 0xCD, out );
            imm16( a0, out, 1 );
        }
        break;        
    case MK_4CC('R','E','T',0):
        if( !conditional( a0, 0xC0, out ) )
            inst( 0xC9, out );
        break;
    case MK_4CC('R','S','T',0):
        switch( a0[0] )
        {
        case '0': inst( 0xC7, out ); break;
        case '1': inst( a0[1]=='0' ? 0xD7 : 0xDF, out ); break;
        case '2': inst( a0[1]=='0' ? 0xE7 : 0xEF, out ); break;
        case '3': inst( a0[1]=='0' ? 0xF7 : 0xFF, out ); break;
        case '8': inst( 0xCF, out ); break;
        }
        break;
    case MK_4CC('P','U','S','H'):
        gen_prefix( a0[1], out );
        inst( 0xC5 | ((reg16( a0 ) & 0x07)<<4), out );
        break;
    case MK_4CC('P','O','P',0):
        gen_prefix( a0[1], out );
        inst( 0xC1 | ((reg16( a0 ) & 0x07)<<4), out );
        break;
    case MK_4CC('I','N',0,0):
        if( *a1=='(' ) ++a1;

        if( reg8( a1 )==C )
            inst( 0xED40 | ((reg8( a0 ) & 0x07)<<3), out );
        else
        {
            inst( 0xDB, out );
            imm8( a1, out );
        }
        break;
    case MK_4CC('O','U','T',0):
        if( *a0=='(' ) ++a0;

        if( reg8( a0 )==C )
            inst( 0xED41 | ((reg8( a1 ) & 0x07)<<3), out );
        else
        {
            inst( 0xD3, out );
            imm8( a0, out );
        }
        break;
    }
}

