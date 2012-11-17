#ifndef ASSEMBLER_COMMON_H
#define ASSEMBLER_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>



#define MK_4CC( a, b, c, d ) (((a)<<24) | ((b)<<16) | ((c)<<8) | (d))

#define LABEL_NEED_10   0x00
#define LABEL_NEED_01   0x01
#define LABEL_NEED_0    0x02
#define LABEL_NEED_1    0x03
#define LABEL_NEED_DIFF 0x10

#define LABEL_TYPE_LABEL  0x00
#define LABEL_TYPE_DEFINE 0x01



int assemble_file( FILE* input, FILE* output );



void set_base_address( unsigned long address );

void add_label( const char* name, unsigned long value, int type );

void require_label( const char* name, unsigned long position, int type );

void post_process_labels( FILE* file );

void reset_labels( void );



char read_char( const char* str, int* delta );

int read_num( const char* str, unsigned long* out );

void assemble_line_8080( unsigned long mnemonic, const char* a0,
                         const char* a1, FILE* out );

#endif /* ASSEMBLER_COMMON_H */

