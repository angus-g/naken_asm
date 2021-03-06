/**
 *  naken_asm assembler.
 *  Author: Michael Kohn
 *   Email: mike@mikekohn.net
 *     Web: http://www.mikekohn.net/
 * License: GPL
 *
 * Copyright 2010-2016 by Michael Kohn
 *
 * 65xx file by Joe Davisson
 *
 */

#ifndef _DISASM_65XX_H
#define _DISASM_65XX_H

#include "common/assembler.h"

int get_cycle_count_65xx(unsigned short int opcode);
int disasm_65xx(struct _memory *memory, uint32_t address, char *instruction, int *cycles_min, int *cycles_max);
void list_output_65xx(struct _asm_context *asm_context, uint32_t start, uint32_t end);
void disasm_range_65xx(struct _memory *memory, uint32_t flags, uint32_t start, uint32_t end);

#endif

