/**
 *  naken_asm assembler.
 *  Author: Michael Kohn
 *   Email: mike@mikekohn.net
 *     Web: http://www.mikekohn.net/
 * License: GPL
 *
 * Copyright 2010-2015 by Michael Kohn
 *
 */

#include "table/tms1000.h"

struct _table_tms1000 table_tms1000[] =
{
  { "knez", 0x09, 0x0e },
  { "tka", -1, 0x08 },
  { "setr", 0x0d, 0x0d },
  { "rstr", 0x0c, 0x0c },
  { "tdo", 0x0a, 0x0a },
  { "clo", 0x0b, 0x0b },
  { "tam", 0x03, 0x27 },
  { "tmy", 0x22, 0x22 },
  { "tma", 0x21, 0x21 },
  { "xma", 0x2e, 0x03 },
  { "tamiy", 0x20, -1 },
  { "tamiyc", -1, 0x25 },
  { "tamdyn", -1, 0x24 },
  { "tamza", 0x04, 0x26 },
  { "amaac", 0x25, 0x06 },
  { "saman", 0x27, 0x3c },
  { "imac", 0x28, 0x3e },
  { "dman", 0x2a, 0x07 },
  { "alem", 0x29, 0x01 },
  { "mnea", -1, 0x00 },
  { "mnez", 0x26, 0x3f },
  //{ "sbit", -1, -1 },
  //{ "rbit", -1, -1 },
  //{ "tbit1", -1, -1 },
  //{ "tcy", -1, -1 },
  //{ "tcmiy", -1, -1 },
  //{ "ldx", -1, -1 },
  //{ "ldp", -1, -1 },
  //{ "alec", -1, -1 },
  //{ "ynec", -1, -1 },
  { "a2aac", -1, 0x78 },
  { "a3aac", -1, 0x74 },
  { "a4aac", -1, 0x7c },
  { "a5aac", -1, 0x72 },
  { "a6aac", 0x06, 0x7a },
  { "a7aac", -1, 0x76 },
  { "a8aac", 0x01, 0x7e },
  { "a9aac", -1, 0x71 },
  { "a10aac", 0x05, 0x79 },
  { "a11aac", -1, 0x75 },
  { "a12aac", -1, 0x7d },
  { "a13aac", -1, 0x73 },
  { "a14aac", -1, 0x7b },
  { "retn", 0x0f, 0x0f },
  //{ "br", -1, -1 },
  //{ "call", -1, -1 },
  { "tay", 0x24, 0x20 },
  { "tya", 0x23, 0x23 },
  { "ynea", 0x02, 0x02 },
  { "cla", 0x2f, 0x7f },
  { "ia", 0x0e, -1 },
  { "iac", -1, 0x70 },
  { "dan", 0x07, 0x77 },
  { "iyc", 0x2b, 0x05 },
  { "dyn", 0x2c, 0x04 },
  { "cpaiz", 0x2d, 0x3d },
  { "comx", 0x00, -1 },
  { "comx", -1, 0x09 },
  { "comc", -1, 0x0b },
  { NULL, 0, 0 }
};

