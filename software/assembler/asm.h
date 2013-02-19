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




typedef void (* assemble_line_fun )( unsigned long, const char*,
                                     const char*, FILE* );



void assemble_line_8080( unsigned long mnemonic, const char* a0,
                         const char* a1, FILE* out );

void assemble_line_z80( unsigned long mnemonic, const char* a0,
                        const char* a1, FILE* out );



/**
 * \brief Assemble a file
 *
 * This functions is defined in asm.c
 *
 * \param input  The plain text input assembly file
 * \param output The output binary file
 *
 * \return Non-zero on success, zero if there was an error.
 */
int assemble_file( FILE* input, FILE* output, assemble_line_fun asm_fun );


/**
 * \brief Set the base address of the program currently being assembled
 *
 * This functions is defined in symbols.c
 *
 * This function can be called multiple times, it will alter the addresses
 * of all labels defined afterwards.
 *
 * The function is called within assemble_file if a [ORG xxxx] directive is
 * discovered.
 *
 * \param address The new base address
 */
void set_base_address( unsigned long address );

/**
 * \brief Add a label or definition
 *
 * This functions is defined in symbols.c
 *
 * This function is called within assemble_file if a label definition or a
 * .def directive is discovered.
 *
 * \param name   The name of the label.
 * \param value  The value of the label (e.g. position in the output file or
 *               value of the define).
 * \param type   LABEL_TYPE_LABEL for ordinary labels or LABEL_TYPE_DEFINE for
 *               definitions.
 * \param output The binary output file. All unresolved references to the new
 *               label are resolved.
 */
void add_label( const char* name, unsigned long value, int type,
                FILE* output );

/**
 * \brief Require a label
 *
 * This functions is defined in symbols.c
 *
 * \param name   The name of the required label.
 * \param output The output binary file. The label value is written if found,
 *               or the current position is stored for later insertion and a
 *               fill value is written.
 * \param type   How to insert the label. LABEL_NEED_10 for 16 bit highbyte
 *               first, LABEL_NEED_01 for 16 bit lowbyte first, LABEL_NEED_0
 *               for 8 bit lowbyte, LABEL_NEED_1 for 8 bit highbyte.
 *               The flag LABEL_NEED_DIFF can be used if the difference to
 *               the insertion position is required.
 */
void require_label( const char* name, FILE* output, int type );

/**
 * \brief Get the value of a define
 *
 * \param name  The name of the define
 * \param value Returns the value of the define
 *
 * \return Non-zero if the define could be found, zero if not.
 */
int get_define( const char* name, unsigned long* value );

/**
 * \brief Reset all internal state of the label handling code.
 *
 * This functions is defined in symbols.c
 */
void reset_labels( void );



/**
 * \brief Embedd an 8 bit immediate value into a binary file
 *
 * This functions is defined in asm.c
 *
 * If the given string can not be converted to a numeric value,
 * it is issued as a required label.
 *
 * \param input  The input string to convert into an 8 bit value
 * \param output The output binary file
 */
void imm8( const char* input, FILE* output );

/**
 * \brief Embedd an 8 bit immediate difference value into a binary file
 *
 * This functions is defined in asm.c
 *
 * If the given string can not be converted to a numeric value,
 * it is issued as a required label of type LABEL_NEED_DIFF.
 *
 * \param input  The input string to convert into an 8 bit value
 * \param output The output binary file
 */
void diff8( const char* input, FILE* output );

/**
 * \brief Embedd a 16 bit immediate value into a binary file
 *
 * This functions is defined in asm.c
 *
 * If the given string can not be converted to a numeric value,
 * it is issued as a required label.
 *
 * \param input  The input string to convert into a 16 bit value.
 * \param output The output binary file.
 * \param le     Non-zero for little endian, zero for bit endian.
 */
void imm16( const char* input, FILE* output, int le );

/**
 * \brief Write an instruction into a binary file
 *
 * This functions is defined in asm.c
 *
 * \param opcode The opcode to write. It is automatically written
 *               as a big endian value, i.e. high byte first.
 * \param output The output binary file.
 */
void inst( unsigned long opcode, FILE* output );


/**
 * \brief Read a character literal from an input string
 *
 * \param str   The string to translate. Can contain things like "bla" or
 *              "\b\r\n\033". The first character of the string is translated
 *              into an ASCII character (parsing escape sequences, etc.)
 * \param delta Returns the number of bytes to advance in the string, e.g. for
 *              the escape sequence "\n", there are two bytes to advance to
 *              get to the next character in the string.
 *
 * \return The translated ASCII character
 */
char read_char( const char* str, int* delta );

/**
 * \brief Translate an integer literal to an actual integer
 *
 * \param str The string to read
 * \param out Returns the translated value
 *
 * \return Non-zero on success, zero on error (i.e. the integer literal is
 *         invalid)
 */
int read_num( const char* str, unsigned long* out );

#endif /* ASSEMBLER_COMMON_H */

