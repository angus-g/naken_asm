/**
 *  naken_asm assembler.
 *  Author: Michael Kohn
 *   Email: mike@mikekohn.net
 *     Web: http://www.mikekohn.net/
 * License: GPL
 *
 * Copyright 2010-2016 by Michael Kohn
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "asm/common.h"
#include "asm/mips.h"
#include "common/assembler.h"
#include "common/tokens.h"
#include "common/eval_expression.h"
#include "table/mips.h"

enum
{
  OPERAND_TREG,
  OPERAND_FREG,
  OPERAND_IMMEDIATE,
  OPERAND_IMMEDIATE_RS,
};

struct _operand
{
  int value;
  int type;
  int reg2;
};

static int get_number(char *s)
{
  int n = 0;

  while(*s != 0)
  {
    if (*s < '0' || *s > '9') return -1;
    n = (n * 10) + (*s - '0');
    s++;
  }

  return n;
}

static int get_register_mips(char *token, char letter)
{
  int num;

  if (token[0] != '$')
  {
    if (letter != 'f' && strcasecmp(token, "zero") == 0) return 0;
    return -1;
  }
  if (token[1] == 0) { return -1; }

  num = get_number(token + 2);
  if (letter == 'f')
  {
    if (num >= 0 && num <= 31 && token[1] == letter)
    {
      return num; 
    }

    return -1;
  }

  if (token[1] >= '0' && token[1] <= '9')
  {
    num = get_number(token + 1);
    if (num >= 0 && num <= 31) return num;
    return -1;
  }

  if (token[1] == 'v' && num >= 0 && num <= 1) { return 2 + num; }
  if (token[1] == 'a' && num >= 0 && num <= 3) { return 4 + num; }
  if (token[1] == 't' && num >= 0 && num <= 7) { return 8 + num; }
  if (token[1] == 's' && num >= 0 && num <= 7) { return 16 + num; }
  if (token[1] == 't' && num >= 8 && num <= 9) { return 24 + (num - 8); }
  if (token[1] == 'k' && num >= 0 && num <= 1) { return 26 + num; }
  if (token[1] == 's' && num == 8) { return 30; }

  if (strcasecmp(token, "$at") == 0) { return 1; }
  if (strcasecmp(token, "$gp") == 0) { return 28; }
  if (strcasecmp(token, "$sp") == 0) { return 29; }
  if (strcasecmp(token, "$s8") == 0) { return 30; }
  if (strcasecmp(token, "$fp") == 0) { return 30; }
  if (strcasecmp(token, "$ra") == 0) { return 31; }

  return -1;
}

static int add_offset(struct _asm_context *asm_context, int address, uint32_t *opcode)
{
  int32_t offset = address - (asm_context->address + 4);

  if (offset < -(1 << 17) || offset > (1 << 17) - 1)
  {
    print_error_range("Offset", -(1 << 17), (1 << 17) - 1, asm_context); 
    return -1;
  }

  if ((offset & 0x3) != 0)
  {
    print_error_align(asm_context, 4);
    return -1;
  }

  *opcode |= (offset >> 2) & 0xffff;

  return 0;
}

static uint32_t find_opcode(const char *instr_case)
{
  int n = 0;

  while(mips_i_table[n].instr != NULL)
  {
    if (strcmp(instr_case, mips_i_table[n].instr) == 0)
    {
      return mips_i_table[n].function << 26;
    }

    n++;
  }

  return 0xffffffff;
}

static int check_type(struct _asm_context *asm_context, char *instr, int user_type, int table_type, int value)
{
  if (table_type == MIPS_OP_RD ||
      table_type == MIPS_OP_RS ||
      table_type == MIPS_OP_RT)
  {
    if (user_type != OPERAND_TREG)
    {
      print_error_illegal_operands(instr, asm_context);
      return -1;
    }
  }
    else
  if (table_type == MIPS_OP_LABEL)
  {
    if (user_type != OPERAND_IMMEDIATE)
    {
      print_error_illegal_operands(instr, asm_context);
      return -1;
    }
  }
    else
  if (table_type == MIPS_OP_IMMEDIATE)
  {
    if (user_type != OPERAND_IMMEDIATE)
    {
      print_error_illegal_operands(instr, asm_context);
      return -1;
    }

    if (value < 0 || value > 0xffff)
    {
      print_error_range("Constant", 0, 0xffff, asm_context);
      return -1;
    }
  }
    else
  if (table_type == MIPS_OP_IMMEDIATE_SIGNED)
  {
    if (user_type != OPERAND_IMMEDIATE)
    {
      print_error_illegal_operands(instr, asm_context);
      return -1;
    }

    if (value < -32768 || value > 32767)
    {
      print_error_range("Constant", -32768, 32767, asm_context);
      return -1;
    }
  }
    else
  if (table_type == MIPS_OP_SA)
  {
    if (user_type != OPERAND_IMMEDIATE)
    {
      print_error_illegal_operands(instr, asm_context);
      return -1;
    }

    if (value < 0 || value > 31)
    {
      print_error_range("Constant", 0, 31, asm_context);
      return -1;
    }
  }
    else
  if (table_type == MIPS_OP_HINT)
  {
    if (user_type != OPERAND_IMMEDIATE)
    {
      print_error_illegal_operands(instr, asm_context);
      return -1;
    }

    if (value < 0 || value > 31)
    {
      print_error_range("Constant", 0, 31, asm_context);
      return -1;
    }
  }
    else
  if (table_type == MIPS_OP_IMMEDIATE_RS)
  {
    if (user_type != OPERAND_IMMEDIATE_RS)
    {
      print_error_illegal_operands(instr, asm_context);
      return -1;
    }

    if (value < -32768 || value > 32767)
    {
      print_error_range("Constant", -32768, 32767, asm_context);
      return -1;
    }
  }

  return 0;
}

static int check_for_pseudo_instruction(struct _asm_context *asm_context, struct _operand *operands, int *operand_count, char *instr_case)
{
  // Check pseudo-instructions
  if (strcmp(instr_case, "move") == 0 &&
      *operand_count == 2 &&
      operands[0].type == OPERAND_TREG &&
      operands[1].type == OPERAND_TREG)
  {
    strcpy(instr_case, "addu");
    operands[2].value = 0;
    operands[2].type = OPERAND_TREG;;
    *operand_count = 3;
  }
    else
  if ((strcmp(instr_case, "li") == 0 || strcmp(instr_case, "la") == 0) &&
      operands[0].type == OPERAND_TREG &&
      operands[1].type == OPERAND_IMMEDIATE &&
      *operand_count == 2)
  {
    uint32_t opcode;
    opcode = find_opcode("lui");
    add_bin32(asm_context, opcode | (operands[0].value << 16) | ((operands[1].value >> 16) & 0xffff), IS_OPCODE);

    opcode = find_opcode("ori");
    add_bin32(asm_context, opcode | (operands[0].value << 21) |(operands[0].value << 16) | (operands[1].value & 0xffff), IS_OPCODE);

    return 8;
  }
    else
  if (strcmp(instr_case, "nop") == 0 && *operand_count == 0)
  {
    strcpy(instr_case, "sll");
    *operand_count = 3;
    operands[2].type = OPERAND_IMMEDIATE;
  }
    else
  if (strcmp(instr_case, "not") == 0 && *operand_count == 2)
  {
    strcpy(instr_case, "nor");
    *operand_count = 3;
    operands[2].type = OPERAND_TREG;
  }
    else
  if (strcmp(instr_case, "negu") == 0 &&
      *operand_count == 2 &&
      operands[0].type == OPERAND_TREG &&
      operands[1].type == OPERAND_TREG)
  {
    // negu rd, rs = subu rd, 0, rs
    strcpy(instr_case, "subu");
    memcpy(&operands[2], &operands[1], sizeof(struct _operand));
    operands[1].value = 0;
    operands[1].type = OPERAND_TREG;
    *operand_count = 3;
  }
    else
  if (strcmp(instr_case, "b") == 0 &&
      *operand_count == 1 &&
      operands[0].type == OPERAND_IMMEDIATE)
  {
    strcpy(instr_case, "beq");
    memcpy(&operands[2], &operands[0], sizeof(struct _operand));
    operands[0].value = 0;
    operands[0].type = OPERAND_TREG;
    operands[1].value = 0;
    operands[1].type = OPERAND_TREG;
    *operand_count = 3;
  }
    else
  if (strcmp(instr_case, "beqz") == 0 &&
      *operand_count == 2 &&
      operands[0].type == OPERAND_TREG &&
      operands[1].type == OPERAND_IMMEDIATE)
  {
    strcpy(instr_case, "beq");
    memcpy(&operands[2], &operands[1], sizeof(struct _operand));
    operands[1].value = 0;
    operands[1].type = OPERAND_TREG;
    *operand_count = 3;
  }
    else
  if (strcmp(instr_case, "bnez") == 0 &&
      *operand_count == 2 &&
      operands[0].type == OPERAND_TREG &&
      operands[1].type == OPERAND_IMMEDIATE)
  {
    strcpy(instr_case, "bne");
    memcpy(&operands[2], &operands[1], sizeof(struct _operand));
    operands[1].value = 0;
    operands[1].type = OPERAND_TREG;
    *operand_count = 3;
  }
#if 0
    else
  if (strcmp(instr_case, "bal") == 0 &&
      *operand_count == 1 &&
      operands[0].type == OPERAND_IMMEDIATE)
  {
    strcpy(instr_case, "bgezal");
    memcpy(&operands[1], &operands[0], sizeof(struct _operand));
    operands[0].value = 0;
    operands[0].type = OPERAND_TREG;
    *operand_count = 2;
  }
#endif

  return 4;
}

int parse_instruction_mips(struct _asm_context *asm_context, char *instr)
{
  struct _operand operands[4];
  int operand_count = 0;
  char token[TOKENLEN];
  int token_type;
  char instr_case[TOKENLEN];
  int paren_flag;
  int num,n,r;
  uint32_t opcode;
  int opcode_size = 4;
  int found = 0;
#if 0
  int n,cond,s=0;
  int opcode=0;
#endif

  lower_copy(instr_case, instr);
  memset(operands, 0, sizeof(operands));

//printf("%s %s\n", instr_case, instr);

  while(1)
  {
    token_type = tokens_get(asm_context, token, TOKENLEN);
    if (token_type == TOKEN_EOL) { break; }
    //printf("token=%s token_type=%d\n", token, token_type);

    if (operand_count == 0 && IS_TOKEN(token,'.'))
    {
      strcat(instr_case, ".");
      strcat(instr, ".");
      token_type = tokens_get(asm_context, token, TOKENLEN);
      strcat(instr, token);
      n = 0;
      while(token[n] != 0) { token[n]=tolower(token[n]); n++; }
      strcat(instr_case, token);
      continue;
    }

    do
    {
      paren_flag = 0;

      if (IS_TOKEN(token,'('))
      {
        token_type = tokens_get(asm_context, token, TOKENLEN);
        paren_flag = 1;
      }

      num = get_register_mips(token, 't');

      if (num != -1)
      {
        operands[operand_count].value = num;
        operands[operand_count].type = OPERAND_TREG;
        if (paren_flag == 0) { break; }
      }
        else
      if (paren_flag == 0)
      {
        num = get_register_mips(token, 'f');
        if (num != -1)
        {
          operands[operand_count].value = num;
          operands[operand_count].type = OPERAND_FREG;
          break;
        }
      }
        else
      {
        paren_flag = 0;

        tokens_push(asm_context, token, token_type);
        //tokens_push(asm_context, "(", TOKEN_SYMBOL);

        if (asm_context->pass == 1)
        {
          eat_operand(asm_context);
        }
          else
        {
          if (eval_expression(asm_context, &num) != 0)
          {
            print_error_unexp(token, asm_context);
            return -1;
          }

          operands[operand_count].type = OPERAND_IMMEDIATE;
          operands[operand_count].value = num;

          token_type = tokens_get(asm_context, token, TOKENLEN);
          if (IS_NOT_TOKEN(token,')'))
          {
            print_error_unexp(token, asm_context);
            return -1;
          }
        }

        break;
      }

      if (paren_flag == 1)
      {
        token_type = tokens_get(asm_context, token, TOKENLEN);
        if (IS_NOT_TOKEN(token,')'))
        {
          print_error_unexp(token, asm_context);
          return -1;
        }

        operands[operand_count].reg2 = operands[operand_count].value;
        operands[operand_count].value = 0;
        operands[operand_count].type = OPERAND_IMMEDIATE_RS;;

        break;
      }

      operands[operand_count].type = OPERAND_IMMEDIATE;

      if (asm_context->pass == 1)
      {
        eat_operand(asm_context);
        break;
      }

      tokens_push(asm_context, token, token_type);
      if (eval_expression(asm_context, &num) != 0)
      {
        print_error_unexp(token, asm_context);
        return -1;
      }

      operands[operand_count].value = num;

      token_type = tokens_get(asm_context, token, TOKENLEN);
      if (IS_TOKEN(token,'('))
      {
        token_type = tokens_get(asm_context, token, TOKENLEN);
        num = get_register_mips(token, 't');
        if (num == -1)
        {
          print_error_unexp(token, asm_context);
          return -1;
        }

        operands[operand_count].reg2 = num;
        operands[operand_count].type = OPERAND_IMMEDIATE_RS;;

        token_type = tokens_get(asm_context, token, TOKENLEN);

        if (IS_NOT_TOKEN(token,')'))
        {
          print_error_unexp(token, asm_context);
          return -1;
        }
      }
        else
      {
        tokens_push(asm_context, token, token_type);
      }

    } while(0);

    operand_count++;

    token_type = tokens_get(asm_context, token, TOKENLEN);
    if (token_type == TOKEN_EOL) { break; }

    if (IS_NOT_TOKEN(token,','))
    {
      print_error_unexp(token, asm_context);
      return -1;
    }

    if (operand_count == 4)
    {
      print_error_unexp(token, asm_context);
      return -1;
    }
  }

  n = check_for_pseudo_instruction(asm_context, operands, &operand_count, instr_case);

  if (n != 4)
  {
    return opcode_size;
  }

  if (asm_context->pass == 1)
  {
    add_bin32(asm_context, 0, IS_OPCODE);
    return opcode_size;
  }

  // R-Type Instruction [ op 6, rs 5, rt 5, rd 5, sa 5, function 6 ]
  n = 0;
  while(mips_r_table[n].instr != NULL)
  {
    if (strcmp(instr_case, mips_r_table[n].instr) == 0)
    {
      char shift_table[] = { 0, 11, 21, 16, 6 };
      if (mips_r_table[n].operand_count != operand_count)
      {
        //print_error_illegal_operands(instr, asm_context);
        //return -1;
        found = 1;
        n++;
        continue;
      }

      opcode = mips_r_table[n].function;

      for (r = 0; r < operand_count; r++)
      {
        if (check_type(asm_context, instr, operands[r].type, mips_r_table[n].operand[r], operands[r].value))
        {
          return -1;
        }

        if (mips_r_table[n].operand[r] == MIPS_OP_SA)
        {
          if (operands[r].type != OPERAND_IMMEDIATE)
          {
            printf("Error: '%s' expects registers at %s:%d\n", instr, asm_context->filename, asm_context->line);
            return -1;
          }
        }
          else
        {
          if (operands[r].type != OPERAND_TREG)
          {
            printf("Error: '%s' expects registers at %s:%d\n", instr, asm_context->filename, asm_context->line);
            return -1;
          }
        }

        opcode |= operands[r].value << shift_table[(int)mips_r_table[n].operand[r]];
      }

      add_bin32(asm_context, opcode, IS_OPCODE);
      return opcode_size;
    }
    n++;
  }

  // J-Type Instruction [ op 6, target 26 ]
  if (strcmp(instr_case, "j") == 0 || strcmp(instr_case, "jal") == 0)
  {
    // FIXME - what to do with this
    //unsigned int upper = (address + 4) & 0xf0000000;
    if (operand_count != 1)
    {
      print_error_illegal_operands(instr, asm_context);
      return -1;
    }

    if (operands[0].type != OPERAND_IMMEDIATE)
    {
      printf("Error: Expecting address for '%s' at %s:%d\n", instr, asm_context->filename, asm_context->line);
      return -1;
    }

    if (instr_case[2] == 'l')
    {
      opcode = 3 << 26;
    }
      else
    {
      opcode = 2 << 26;
    }

    add_bin32(asm_context, opcode | operands[0].value >> 2, IS_OPCODE);

    return opcode_size;
  }

  // Coprocessor Instruction [ op 6, format 5, ft 5, fs 5, fd 5, funct 6 ]
  n = 0;
  while(mips_cop_table[n].instr != NULL)
  {
    if (strcmp(instr_case, mips_cop_table[n].instr) == 0)
    {
      char shift_table[] = { 0, 5, 11, 16 };
      if (mips_cop_table[n].operand_count != operand_count)
      {
        print_error_illegal_operands(instr, asm_context);
        return -1;
      }

      opcode = (0x11 << 26) | (mips_cop_table[n].format << 21) | mips_cop_table[n].function;

      for (r = 0; r < operand_count; r++)
      {
        if (operands[r].type != OPERAND_FREG)
        {
          printf("Error: '%s' expects registers at %s:%d\n", instr, asm_context->filename, asm_context->line);
          return -1;
        }
        opcode |= operands[r].value << shift_table[(int)mips_cop_table[n].operand[r]];
      }

      add_bin32(asm_context, opcode, IS_OPCODE);
      return opcode_size;
    }
    n++;
  }

  // I-Type?  [ op 6, rs 5, rt 5, imm 16 ]
  n = 0;
  while(mips_i_table[n].instr != NULL)
  {
    if (strcmp(instr_case, mips_i_table[n].instr) == 0)
    {
      char shift_table[] = { 0, 0, 21, 16 };
      if (mips_i_table[n].operand_count != operand_count)
      {
        print_error_opcount(instr, asm_context);
        return -1;
      }

      opcode = mips_i_table[n].function << 26;

      for (r = 0; r < mips_i_table[n].operand_count; r++)
      {
        if (check_type(asm_context, instr, operands[r].type, mips_i_table[n].operand[r], operands[r].value))
        {
          return -1;
        }

        if ((mips_i_table[n].operand[r] == MIPS_OP_RT ||
             mips_i_table[n].operand[r] == MIPS_OP_RS) &&
            operands[r].type == OPERAND_TREG)
        {
          opcode |= operands[r].value << shift_table[(int)mips_i_table[n].operand[r]];
        }
          else
        if (mips_i_table[n].operand[r] == MIPS_OP_LABEL)
        {
          int32_t offset = operands[r].value - (asm_context->address + 4);

          if (offset < -(1 << 17) ||
              offset > (1 << 17) - 1)
          {
            print_error_range("Offset", -(1 << 17), (1 << 17) - 1, asm_context); 
            return -1;
          }

          if ((offset & 0x3) != 0)
          {
            print_error_align(asm_context, 4);
            return -1;
          }

          opcode |= (offset >> 2) & 0xffff;
        }
          else
        if (mips_i_table[n].operand[r] == MIPS_OP_IMMEDIATE ||
            mips_i_table[n].operand[r] == MIPS_OP_IMMEDIATE_SIGNED)
        {
          opcode |= operands[r].value & 0xffff;
        }
          else
        if (mips_i_table[n].operand[r] == MIPS_OP_IMMEDIATE_RS)
        {
          opcode |= operands[r].value & 0xffff;
          opcode |= operands[r].reg2 << 21;
        }
          else
        if (mips_i_table[n].operand[r] == MIPS_OP_HINT)
        {
          opcode |= operands[r].value << 16;
        }
#if 0
          else
        if (mips_i_table[n].operand[r] == MIPS_OP_RT_IS_0)
        {
          // Derr
          print_error_internal(asm_context, __FILE__, __LINE__);
        }
          else
        if (mips_i_table[n].operand[r] == MIPS_OP_RT_IS_1)
        {
          opcode |= 1 << 16;
        }
#endif
          else
        {
          print_error_illegal_operands(instr, asm_context);
          return -1;
        }

#if 0
        if (mips_i_table[n].operand[r] != MIPS_OP_IMMEDIATE &&
            mips_i_table[n].operand[r] != MIPS_OP_IMMEDIATE_SIGNED)
        {
          opcode |= operands[r].value << shift_table[(int)mips_i_table[n].operand[r]];
        }
#endif
      }

      add_bin32(asm_context, opcode, IS_OPCODE);

      return opcode_size;
    }
    n++;
  }

  n = 0;
  while(mips_branch_table[n].instr != NULL)
  {
    if (strcmp(instr_case, mips_branch_table[n].instr) == 0)
    {
      if (mips_branch_table[n].op_rt == -1)
      {
        if (operand_count != 3)
        {
          print_error_opcount(instr, asm_context);
          return -1;
        }

        if (operands[0].type != OPERAND_TREG ||
            operands[1].type != OPERAND_TREG ||
            operands[2].type != OPERAND_IMMEDIATE)
        {
          print_error_illegal_operands(instr, asm_context);
          return -1;
        }

        opcode = (mips_branch_table[n].opcode << 26) |
                 (operands[0].value << 21) |
                 (operands[1].value << 16);

        if (add_offset(asm_context, operands[2].value, &opcode) == -1)
        {
          return -1;
        }

        add_bin32(asm_context, opcode, IS_OPCODE);

        return opcode_size;
      }
        else
      {
        if (operand_count != 2)
        {
          print_error_opcount(instr, asm_context);
          return -1;
        }

        if (operands[0].type != OPERAND_TREG ||
            operands[1].type != OPERAND_IMMEDIATE)
        {
          print_error_illegal_operands(instr, asm_context);
          return -1;
        }

        opcode = (mips_branch_table[n].opcode << 26) |
                 (operands[0].value << 21) |
                 (mips_branch_table[n].op_rt << 16);

        if (add_offset(asm_context, operands[1].value, &opcode) == -1)
        {
          return -1;
        }

        add_bin32(asm_context, opcode, IS_OPCODE);

        return opcode_size;
      }
    }
    n++;
  }

  // Special2 / Special3 type
  n = 0;
  while(mips_special_table[n].instr != NULL)
  {
    if (strcmp(instr_case, mips_special_table[n].instr) == 0)
    {
      if (mips_special_table[n].operand_count != operand_count)
      {
        print_error_illegal_operands(instr, asm_context);
        return -1;
      }

      int shift;

      opcode = (mips_special_table[n].format << 26) |
                mips_special_table[n].function;

      if (mips_special_table[n].type == SPECIAL_TYPE_REGS)
      {
        opcode |= mips_special_table[n].operation << 6;
        shift = 21;
      }
        else
      if (mips_special_table[n].type == SPECIAL_TYPE_SA)
      {
        opcode |= mips_special_table[n].operation << 21;
        shift = 16;
      }
        else
      if (mips_special_table[n].type == SPECIAL_TYPE_BITS)
      {
        shift = 21;
      }
        else
      if (mips_special_table[n].type == SPECIAL_TYPE_BITS2)
      {
        shift = 21;
      }
        else
      {
        print_error_internal(asm_context, __FILE__, __LINE__);
        return -1;
      }

      for (r = 0; r < 4; r++)
      {
        int operand_index = mips_special_table[n].operand[r];

        if (operand_index != -1)
        {
          if (r < 2 || mips_special_table[n].type == SPECIAL_TYPE_REGS)
          {
            if (operands[operand_index].type != OPERAND_TREG)
            {
              printf("Error: '%s' expects registers at %s:%d\n", instr, asm_context->filename, asm_context->line);
              return -1;
            }
          }
            else
          {
            // SPECIAL_TYPE_SA and SPECIAL_TYPE_BITS
            if (operands[operand_index].type != OPERAND_IMMEDIATE)
            {
              printf("Error: '%s' expects immediate %s:%d\n", instr, asm_context->filename, asm_context->line);
              return -1;
            }

            if (operand_index == 3 &&
                mips_special_table[n].type == SPECIAL_TYPE_BITS)
            {
              if (operands[operand_index].value < 1 ||
                  operands[operand_index].value > 32)
              {
                print_error_range("Constant", 1, 32, asm_context);
                return -1;
              }

              operands[operand_index].value--;
            }
              else
            if (operand_index == 3 &&
                mips_special_table[n].type == SPECIAL_TYPE_BITS2)
            {
              if (operands[operand_index].value < 1 ||
                  operands[operand_index].value > 32)
              {
                print_error_range("size", 1, 32, asm_context); 
                return -1;
              }

              operands[operand_index].value += operands[2].value - 1;

              if (operands[operand_index].value < 1 ||
                  operands[operand_index].value > 32)
              {
                print_error_range("pos+size", 1, 32, asm_context); 
                return -1;
              }
            }
              else
            {
              if (operands[r].value < 0 || operands[r].value > 31)
              {
                print_error_range("Constant", 0, 31, asm_context);
                return -1;
              }
            }
          }

          opcode |= operands[operand_index].value << shift;
        }

        shift -= 5;
      }

      // FIXME - Is this always true?
      //opcode |= operands[0].value << shift_table[3];

      add_bin32(asm_context, opcode, IS_OPCODE);
      return opcode_size;
    }
    n++;
  }

  n = 0;
  while(mips_no_operands[n].instr != NULL)
  {
    if (strcmp(instr_case, mips_no_operands[n].instr) == 0)
    {
      if (operand_count != 0)
      {
        print_error_illegal_operands(instr, asm_context);
        return -1;
      }

      add_bin32(asm_context, mips_no_operands[n].opcode, IS_OPCODE);
      return opcode_size;
    }

    n++;
  }

  n = 0;
  while(mips_other[n].instr != NULL)
  {
    if (strcmp(instr_case, mips_other[n].instr) == 0)
    {
      found = 1;

      if (operand_count != mips_other[n].operand_count)
      {
        n++;
        continue;
      }

      opcode = mips_other[n].opcode;

      for (r = 0; r < mips_other[n].operand_count; r++)
      {
        switch(mips_other[n].operand[r])
        {
          case MIPS_OP_RS:
            if (operands[r].type != OPERAND_TREG)
            {
              print_error_illegal_operands(instr, asm_context);
              return -1;
            }

            opcode |= operands[r].value << 21;

            break;
          case MIPS_OP_RT:
            if (operands[r].type != OPERAND_TREG)
            {
              print_error_illegal_operands(instr, asm_context);
              return -1;
            }

            opcode |= operands[r].value << 16;

            break;
          case MIPS_OP_RD:
            if (operands[r].type != OPERAND_TREG)
            {
              print_error_illegal_operands(instr, asm_context);
              return -1;
            }

            opcode |= operands[r].value << 11;

            break;
          case MIPS_OP_SA:
            if (operands[r].type != OPERAND_IMMEDIATE)
            {
              print_error_illegal_operands(instr, asm_context);
              return -1;
            }

            if (operands[r].value < 0 || operands[r].value > 31)
            {
              print_error_range("Constant", 0, 31, asm_context);
              return -1;
            }

            opcode |= operands[r].value << 6;

            break;
          case MIPS_OP_IMMEDIATE_SIGNED:
            if (operands[r].type != OPERAND_IMMEDIATE)
            {
              print_error_illegal_operands(instr, asm_context);
              return -1;
            }

            if (operands[r].value < -32768 || operands[r].value > 32767)
            {
              print_error_range("Constant", -32768, 32767, asm_context);
              return -1;
            }

            opcode |= operands[r].value & 0xffff;

            break;
          case MIPS_OP_LABEL:
            if (operands[r].type != OPERAND_IMMEDIATE)
            {
              print_error_illegal_operands(instr, asm_context);
              return -1;
            }

            if (add_offset(asm_context, operands[r].value, &opcode) == -1)
            {
              return -1;
            }

            break;
          case MIPS_OP_PREG:
            if (operands[r].type != OPERAND_IMMEDIATE)
            {
              print_error_illegal_operands(instr, asm_context);
              return -1;
            }

            if (operands[r].value < 0 || operands[r].value > 1)
            {
              print_error_range("Constant", 0, 1, asm_context);
              return -1;
            }

            opcode |= operands[r].value << 1;
            break;
          default:
            print_error_illegal_operands(instr, asm_context);
            return -1;
        }
      }

      add_bin32(asm_context, opcode, IS_OPCODE);
      return opcode_size;
    }

    n++;
  }

  if (found == 1)
  {
    print_error_illegal_operands(instr, asm_context);
  }
    else
  {
    print_error_unknown_instr(instr, asm_context);
  }

  return -1;
}


