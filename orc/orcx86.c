
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>

#include <orc/orcprogram.h>
#include <orc/orcx86.h>
#include <orc/orcdebug.h>
#include <orc/orcutils.h>


/**
 * SECTION:orcx86
 * @title: x86
 * @short_description: code generation for x86
 */

const char *
orc_x86_get_regname(int i)
{
  static const char *x86_regs[] = {
    "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
    "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d" };

  if (i>=ORC_GP_REG_BASE && i<ORC_GP_REG_BASE + 16) return x86_regs[i - ORC_GP_REG_BASE];
  switch (i) {
    case 0:
      return "UNALLOCATED";
    case 1:
      return "direct";
    default:
      return "ERROR";
  }
}

int
orc_x86_get_regnum(int i)
{
  return (i&0xf);
}

const char *
orc_x86_get_regname_8(int i)
{
  static const char *x86_regs[] = { "al", "cl", "dl", "bl",
    "ah", "ch", "dh", "bh" };

  if (i>=ORC_GP_REG_BASE && i<ORC_GP_REG_BASE + 8) return x86_regs[i - ORC_GP_REG_BASE];
  switch (i) {
    case 0:
      return "UNALLOCATED";
    case 1:
      return "direct";
    default:
      return "ERROR";
  }
}

const char *
orc_x86_get_regname_16(int i)
{
  static const char *x86_regs[] = { "ax", "cx", "dx", "bx",
    "sp", "bp", "si", "di" };

  if (i>=ORC_GP_REG_BASE && i<ORC_GP_REG_BASE + 8) return x86_regs[i - ORC_GP_REG_BASE];
  switch (i) {
    case 0:
      return "UNALLOCATED";
    case 1:
      return "direct";
    default:
      return "ERROR";
  }
}

const char *
orc_x86_get_regname_64(int i)
{
  static const char *x86_regs[] = {
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15" };

  if (i>=ORC_GP_REG_BASE && i<ORC_GP_REG_BASE + 16) return x86_regs[i - ORC_GP_REG_BASE];
  switch (i) {
    case 0:
      return "UNALLOCATED";
    case 1:
      return "direct";
    default:
      return "ERROR";
  }
}

const char *
orc_x86_get_regname_ptr(OrcCompiler *compiler, int i)
{
  if (compiler->is_64bit) {
    return orc_x86_get_regname_64 (i);
  } else {
    return orc_x86_get_regname (i);
  }
}

void
orc_x86_emit_push (OrcCompiler *compiler, int size, int reg)
{

  if (size == 1) {
    ORC_COMPILER_ERROR(compiler, "bad size");
  } else if (size == 2) {
    ORC_ASM_CODE(compiler,"  pushw %%%s\n", orc_x86_get_regname_16(reg));
    *compiler->codeptr++ = 0x66;
    *compiler->codeptr++ = 0x50 + orc_x86_get_regnum(reg);
  } else {
    ORC_ASM_CODE(compiler,"  pushl %%%s\n", orc_x86_get_regname(reg));
    *compiler->codeptr++ = 0x50 + orc_x86_get_regnum(reg);
  }
}

void
orc_x86_emit_pop (OrcCompiler *compiler, int size, int reg)
{

  if (size == 1) {
    ORC_COMPILER_ERROR(compiler, "bad size");
  } else if (size == 2) {
    ORC_ASM_CODE(compiler,"  popw %%%s\n", orc_x86_get_regname_16(reg));
    *compiler->codeptr++ = 0x66;
    *compiler->codeptr++ = 0x58 + orc_x86_get_regnum(reg);
  } else {
    ORC_ASM_CODE(compiler,"  popl %%%s\n", orc_x86_get_regname(reg));
    *compiler->codeptr++ = 0x58 + orc_x86_get_regnum(reg);
  }
}

#define X86_MODRM(mod, rm, reg) ((((mod)&3)<<6)|(((rm)&7)<<0)|(((reg)&7)<<3))
#define X86_SIB(ss, ind, reg) ((((ss)&3)<<6)|(((ind)&7)<<3)|((reg)&7))

void
orc_x86_emit_modrm_memoffset (OrcCompiler *compiler, int reg1, int offset, int reg2)
{
  if (offset == 0 && reg2 != compiler->exec_reg) {
    if (reg2 == X86_ESP) {
      *compiler->codeptr++ = X86_MODRM(0, 4, reg1);
      *compiler->codeptr++ = X86_SIB(0, 4, reg2);
    } else {
      *compiler->codeptr++ = X86_MODRM(0, reg2, reg1);
    }
  } else if (offset >= -128 && offset < 128) {
    *compiler->codeptr++ = X86_MODRM(1, reg2, reg1);
    if (reg2 == X86_ESP) {
      *compiler->codeptr++ = X86_SIB(0, 4, reg2);
    }
    *compiler->codeptr++ = (offset & 0xff);
  } else {
    *compiler->codeptr++ = X86_MODRM(2, reg2, reg1);
    if (reg2 == X86_ESP) {
      *compiler->codeptr++ = X86_SIB(0, 4, reg2);
    }
    *compiler->codeptr++ = (offset & 0xff);
    *compiler->codeptr++ = ((offset>>8) & 0xff);
    *compiler->codeptr++ = ((offset>>16) & 0xff);
    *compiler->codeptr++ = ((offset>>24) & 0xff);
  }
}

void
orc_x86_emit_modrm_reg (OrcCompiler *compiler, int reg1, int reg2)
{
  *compiler->codeptr++ = X86_MODRM(3, reg1, reg2);
}

void
orc_x86_emit_rex (OrcCompiler *compiler, int size, int reg1, int reg2, int reg3)
{
  int rex = 0x40;

  if (compiler->is_64bit) {
    if (size >= 8) rex |= 0x08;
    if (reg1 & 8) rex |= 0x4;
    if (reg2 & 8) rex |= 0x2;
    if (reg3 & 8) rex |= 0x1;

    if (rex != 0x40) *compiler->codeptr++ = rex;
  }
}

void
orc_x86_emit_mov_memoffset_reg (OrcCompiler *compiler, int size, int offset,
    int reg1, int reg2)
{

  switch (size) {
    case 1:
      ORC_ASM_CODE(compiler,"  movb %d(%%%s), %%%s\n", offset,
          orc_x86_get_regname_ptr(compiler, reg1),
          orc_x86_get_regname_8(reg2));
      orc_x86_emit_rex(compiler, size, reg2, 0, reg1);
      *compiler->codeptr++ = 0x8a;
      orc_x86_emit_modrm_memoffset (compiler, reg2, offset, reg1);
      return;
    case 2:
      ORC_ASM_CODE(compiler,"  movw %d(%%%s), %%%s\n", offset, orc_x86_get_regname_ptr(compiler, reg1),
          orc_x86_get_regname_16(reg2));
      *compiler->codeptr++ = 0x66;
      break;
    case 4:
      ORC_ASM_CODE(compiler,"  movl %d(%%%s), %%%s\n", offset, orc_x86_get_regname_ptr(compiler, reg1),
          orc_x86_get_regname(reg2));
      break;
    case 8:
      ORC_ASM_CODE(compiler,"  mov %d(%%%s), %%%s\n", offset, orc_x86_get_regname_ptr(compiler, reg1),
          orc_x86_get_regname_64(reg2));
      break;
    default:
      ORC_COMPILER_ERROR(compiler, "bad size");
      break;
  }

  orc_x86_emit_rex(compiler, size, reg2, 0, reg1);
  *compiler->codeptr++ = 0x8b;
  orc_x86_emit_modrm_memoffset (compiler, reg2, offset, reg1);
}

void
orc_x86_emit_mov_reg_memoffset (OrcCompiler *compiler, int size, int reg1, int offset,
    int reg2)
{
  switch (size) {
    case 1:
      ORC_ASM_CODE(compiler,"  movb %%%s, %d(%%%s)\n", orc_x86_get_regname_8(reg1), offset,
          orc_x86_get_regname_ptr(compiler, reg2));
      orc_x86_emit_rex(compiler, size, reg1, 0, reg2);
      *compiler->codeptr++ = 0x88;
      orc_x86_emit_modrm_memoffset (compiler, reg1, offset, reg2);
      return;
    case 2:
      ORC_ASM_CODE(compiler,"  movw %%%s, %d(%%%s)\n", orc_x86_get_regname_16(reg1), offset,
          orc_x86_get_regname_ptr(compiler, reg2));
      *compiler->codeptr++ = 0x66;
      break;
    case 4:
      ORC_ASM_CODE(compiler,"  movl %%%s, %d(%%%s)\n", orc_x86_get_regname(reg1), offset,
          orc_x86_get_regname_ptr(compiler, reg2));
      break;
    case 8:
      ORC_ASM_CODE(compiler,"  mov %%%s, %d(%%%s)\n", orc_x86_get_regname(reg1), offset,
          orc_x86_get_regname_ptr(compiler, reg2));
      break;
    default:
      ORC_COMPILER_ERROR(compiler, "bad size");
      break;
  }

  orc_x86_emit_rex(compiler, size, reg1, 0, reg2);
  *compiler->codeptr++ = 0x89;
  orc_x86_emit_modrm_memoffset (compiler, reg1, offset, reg2);
}

void
orc_x86_emit_mov_imm_reg (OrcCompiler *compiler, int size, int value, int reg1)
{
  if (size == 2) {
    ORC_ASM_CODE(compiler,"  movw $%d, %%%s\n", value, orc_x86_get_regname_16(reg1));
    orc_x86_emit_rex(compiler, size, reg1, 0, 0);
    *compiler->codeptr++ = 0x66;
    *compiler->codeptr++ = 0xb8 + orc_x86_get_regnum(reg1);
    *compiler->codeptr++ = (value & 0xff);
    *compiler->codeptr++ = ((value>>8) & 0xff);
  } else if (size == 4) {
    ORC_ASM_CODE(compiler,"  movl $%d, %%%s\n", value, orc_x86_get_regname(reg1));
    orc_x86_emit_rex(compiler, size, reg1, 0, 0);
    *compiler->codeptr++ = 0xb8 + orc_x86_get_regnum(reg1);
    *compiler->codeptr++ = (value & 0xff);
    *compiler->codeptr++ = ((value>>8) & 0xff);
    *compiler->codeptr++ = ((value>>16) & 0xff);
    *compiler->codeptr++ = ((value>>24) & 0xff);
  } else {
    /* FIXME */
  }

}

void orc_x86_emit_mov_reg_reg (OrcCompiler *compiler, int size, int reg1, int reg2)
{
  if (size == 2) {
    ORC_ASM_CODE(compiler,"  movw %%%s, %%%s\n", orc_x86_get_regname_16(reg1),
        orc_x86_get_regname_16(reg2));
    *compiler->codeptr++ = 0x66;
  } else if (size == 4) {
    ORC_ASM_CODE(compiler,"  movl %%%s, %%%s\n", orc_x86_get_regname(reg1),
        orc_x86_get_regname(reg2));
  } else {
    ORC_ASM_CODE(compiler,"  mov %%%s, %%%s\n", orc_x86_get_regname_64(reg1),
        orc_x86_get_regname_64(reg2));
  }

  orc_x86_emit_rex(compiler, size, reg2, 0, reg1);
  *compiler->codeptr++ = 0x89;
  orc_x86_emit_modrm_reg (compiler, reg2, reg1);
}

void
orc_x86_emit_test_reg_reg (OrcCompiler *compiler, int size, int reg1, int reg2)
{
  if (size == 2) {
    ORC_ASM_CODE(compiler,"  testw %%%s, %%%s\n", orc_x86_get_regname_16(reg1),
        orc_x86_get_regname_16(reg2));
    *compiler->codeptr++ = 0x66;
  } else if (size == 4) {
    ORC_ASM_CODE(compiler,"  testl %%%s, %%%s\n", orc_x86_get_regname(reg1),
        orc_x86_get_regname(reg2));
  } else {
    ORC_ASM_CODE(compiler,"  test %%%s, %%%s\n", orc_x86_get_regname(reg1),
        orc_x86_get_regname(reg2));
  }

  orc_x86_emit_rex(compiler, size, reg2, 0, reg1);
  *compiler->codeptr++ = 0x85;
  orc_x86_emit_modrm_reg (compiler, reg2, reg1);
}

void
orc_x86_emit_sar_imm_reg (OrcCompiler *compiler, int size, int value, int reg)
{
  if (value == 0) return;

  if (size == 2) {
    ORC_ASM_CODE(compiler,"  sarw $%d, %%%s\n", value, orc_x86_get_regname_16(reg));
  } else if (size == 4) {
    ORC_ASM_CODE(compiler,"  sarl $%d, %%%s\n", value, orc_x86_get_regname(reg));
  } else {
    ORC_ASM_CODE(compiler,"  sar $%d, %%%s\n", value, orc_x86_get_regname_64(reg));
  }

  orc_x86_emit_rex(compiler, size, reg, 0, 0);
  if (value == 1) {
    *compiler->codeptr++ = 0xd1;
    orc_x86_emit_modrm_reg (compiler, reg, 7);
  } else {
    *compiler->codeptr++ = 0xc1;
    orc_x86_emit_modrm_reg (compiler, reg, 7);
    *compiler->codeptr++ = value;
  }
}

void
orc_x86_emit_and_imm_memoffset (OrcCompiler *compiler, int size, int value,
    int offset, int reg)
{
  if (size == 2) {
    ORC_ASM_CODE(compiler,"  andw $%d, %d(%%%s)\n", value, offset,
        orc_x86_get_regname_ptr(compiler, reg));
    *compiler->codeptr++ = 0x66;
  } else if (size == 4) {
    ORC_ASM_CODE(compiler,"  andl $%d, %d(%%%s)\n", value, offset,
        orc_x86_get_regname_ptr(compiler, reg));
  } else {
    ORC_ASM_CODE(compiler,"  and $%d, %d(%%%s)\n", value, offset,
        orc_x86_get_regname_ptr(compiler, reg));
  }

  orc_x86_emit_rex(compiler, size, 0, 0, reg);
  if (value >= -128 && value < 128) {
    *compiler->codeptr++ = 0x83;
    /* FIXME */
    orc_x86_emit_modrm_memoffset (compiler, 0, offset, reg);
    *compiler->codeptr++ = (value & 0xff);
  } else {
    *compiler->codeptr++ = 0x81;
    /* FIXME */
    orc_x86_emit_modrm_memoffset (compiler, 0, offset, reg);
    *compiler->codeptr++ = (value & 0xff);
    *compiler->codeptr++ = ((value>>8) & 0xff);
    if (size == 4) {
      *compiler->codeptr++ = ((value>>16) & 0xff);
      *compiler->codeptr++ = ((value>>24) & 0xff);
    }
  }
}

void
orc_x86_emit_and_imm_reg (OrcCompiler *compiler, int size, int value, int reg)
{
  if (size == 2) {
    ORC_ASM_CODE(compiler,"  andw $%d, %%%s\n", value, orc_x86_get_regname_16(reg));
    *compiler->codeptr++ = 0x66;
  } else if (size == 4) {
    ORC_ASM_CODE(compiler,"  andl $%d, %%%s\n", value, orc_x86_get_regname(reg));
  } else {
    ORC_ASM_CODE(compiler,"  and $%d, %%%s\n", value, orc_x86_get_regname_64(reg));
  }

  orc_x86_emit_rex(compiler, size, 0, 0, reg);
  if (value >= -128 && value < 128) {
    *compiler->codeptr++ = 0x83;
    orc_x86_emit_modrm_reg (compiler, reg, 4);
    *compiler->codeptr++ = (value & 0xff);
  } else {
    *compiler->codeptr++ = 0x81;
    orc_x86_emit_modrm_reg (compiler, reg, 4);
    *compiler->codeptr++ = (value & 0xff);
    *compiler->codeptr++ = ((value>>8) & 0xff);
    if (size == 4) {
      *compiler->codeptr++ = ((value>>16) & 0xff);
      *compiler->codeptr++ = ((value>>24) & 0xff);
    }
  }
}

void
orc_x86_emit_add_imm_memoffset (OrcCompiler *compiler, int size, int value,
    int offset, int reg)
{
  if (size == 2) {
    ORC_ASM_CODE(compiler,"  addw $%d, %d(%%%s)\n", value, offset,
        orc_x86_get_regname_ptr(compiler, reg));
    *compiler->codeptr++ = 0x66;
  } else if (size == 4) {
    ORC_ASM_CODE(compiler,"  addl $%d, %d(%%%s)\n", value, offset,
        orc_x86_get_regname_ptr(compiler, reg));
  } else {
    ORC_ASM_CODE(compiler,"  add $%d, %d(%%%s)\n", value, offset,
        orc_x86_get_regname_ptr(compiler, reg));
  }

  orc_x86_emit_rex(compiler, size, 0, 0, reg);
  if (value >= -128 && value < 128) {
    *compiler->codeptr++ = 0x83;
    orc_x86_emit_modrm_memoffset (compiler, 0, offset, reg);
    *compiler->codeptr++ = (value & 0xff);
  } else {
    *compiler->codeptr++ = 0x81;
    orc_x86_emit_modrm_memoffset (compiler, 0, offset, reg);
    *compiler->codeptr++ = (value & 0xff);
    *compiler->codeptr++ = ((value>>8) & 0xff);
    if (size == 4) {
      *compiler->codeptr++ = ((value>>16) & 0xff);
      *compiler->codeptr++ = ((value>>24) & 0xff);
    }
  }
}

void
orc_x86_emit_add_reg_memoffset (OrcCompiler *compiler, int size, int reg1,
    int offset, int reg)
{
  if (size == 2) {
    ORC_ASM_CODE(compiler,"  addw %%%s, %d(%%%s)\n",
        orc_x86_get_regname_ptr(compiler, reg1), offset,
        orc_x86_get_regname_ptr(compiler, reg));
    *compiler->codeptr++ = 0x66;
  } else if (size == 4) {
    ORC_ASM_CODE(compiler,"  addl %%%s, %d(%%%s)\n",
        orc_x86_get_regname_ptr(compiler, reg1), offset,
        orc_x86_get_regname_ptr(compiler, reg));
  } else {
    ORC_ASM_CODE(compiler,"  add %%%s, %d(%%%s)\n",
        orc_x86_get_regname_ptr(compiler, reg1), offset,
        orc_x86_get_regname_ptr(compiler, reg));
  }

  orc_x86_emit_rex(compiler, size, 0, 0, reg);
  *compiler->codeptr++ = 0x01;
  orc_x86_emit_modrm_memoffset (compiler, reg1, offset, reg);
}

void
orc_x86_emit_add_imm_reg (OrcCompiler *compiler, int size, int value, int reg, orc_bool record)
{
  if (!record) {
    if (size == 4 && !compiler->is_64bit) {
      ORC_ASM_CODE(compiler,"  lea %d(%%%s), %%%s\n", value,
          orc_x86_get_regname(reg), orc_x86_get_regname(reg));
      orc_x86_emit_rex(compiler, size, 0, 0, reg);
      *compiler->codeptr++ = 0x8d;
      orc_x86_emit_modrm_memoffset (compiler, reg, value, reg);
      return;
    }
    if (size == 8 && compiler->is_64bit) {
      ORC_ASM_CODE(compiler,"  lea %d(%%%s), %%%s\n", value,
          orc_x86_get_regname_64(reg), orc_x86_get_regname_64(reg));
      orc_x86_emit_rex(compiler, size, reg, 0, reg);
      *compiler->codeptr++ = 0x8d;
      orc_x86_emit_modrm_memoffset (compiler, reg, value, reg);
      return;
    }
  }

  if (size == 2) {
    ORC_ASM_CODE(compiler,"  addw $%d, %%%s\n", value, orc_x86_get_regname_16(reg));
    *compiler->codeptr++ = 0x66;
  } else if (size == 4) {
    ORC_ASM_CODE(compiler,"  addl $%d, %%%s\n", value, orc_x86_get_regname(reg));
  } else {
    ORC_ASM_CODE(compiler,"  add $%d, %%%s\n", value, orc_x86_get_regname_64(reg));
  }

  orc_x86_emit_rex(compiler, size, 0, 0, reg);
  if (value >= -128 && value < 128) {
    *compiler->codeptr++ = 0x83;
    orc_x86_emit_modrm_reg (compiler, reg, 0);
    *compiler->codeptr++ = (value & 0xff);
  } else {
    *compiler->codeptr++ = 0x81;
    orc_x86_emit_modrm_reg (compiler, reg, 0);
    *compiler->codeptr++ = (value & 0xff);
    *compiler->codeptr++ = ((value>>8) & 0xff);
    if (size == 4) {
      *compiler->codeptr++ = ((value>>16) & 0xff);
      *compiler->codeptr++ = ((value>>24) & 0xff);
    }
  }
}

void
orc_x86_emit_add_reg_reg (OrcCompiler *compiler, int size, int reg1, int reg2)
{
  if (size == 2) {
    ORC_ASM_CODE(compiler,"  addw %%%s, %%%s\n", orc_x86_get_regname_16(reg1),
        orc_x86_get_regname_16(reg2));
    *compiler->codeptr++ = 0x66;
  } else if (size == 4) {
    ORC_ASM_CODE(compiler,"  addl %%%s, %%%s\n", orc_x86_get_regname(reg1),
        orc_x86_get_regname(reg2));
  } else {
    ORC_ASM_CODE(compiler,"  add %%%s, %%%s\n", orc_x86_get_regname_64(reg1),
        orc_x86_get_regname_64(reg2));
  }

  orc_x86_emit_rex(compiler, size, reg2, 0, reg1);
  *compiler->codeptr++ = 0x01;
  orc_x86_emit_modrm_reg (compiler, reg2, reg1);
}

void
orc_x86_emit_sub_reg_reg (OrcCompiler *compiler, int size, int reg1, int reg2)
{
  if (size == 2) {
    ORC_ASM_CODE(compiler,"  subw %%%s, %%%s\n", orc_x86_get_regname_16(reg1),
        orc_x86_get_regname_16(reg2));
    *compiler->codeptr++ = 0x66;
  } else if (size == 4) {
    ORC_ASM_CODE(compiler,"  subl %%%s, %%%s\n", orc_x86_get_regname(reg1),
        orc_x86_get_regname(reg2));
  } else {
    ORC_ASM_CODE(compiler,"  sub %%%s, %%%s\n", orc_x86_get_regname_64(reg1),
        orc_x86_get_regname_64(reg2));
  }

  orc_x86_emit_rex(compiler, size, reg2, 0, reg1);
  *compiler->codeptr++ = 0x29;
  orc_x86_emit_modrm_reg (compiler, reg2, reg1);
}

void
orc_x86_emit_imul_memoffset_reg (OrcCompiler *compiler, int size,
    int offset, int reg, int destreg)
{
  ORC_ASM_CODE(compiler,"  imul %d(%%%s), %%%s\n", offset,
      orc_x86_get_regname_ptr(compiler, reg),
      orc_x86_get_regname(destreg));

  orc_x86_emit_rex(compiler, size, 0, 0, reg);
  *compiler->codeptr++ = 0x0f;
  *compiler->codeptr++ = 0xaf;
  orc_x86_emit_modrm_memoffset (compiler, destreg, offset, reg);
}

void
orc_x86_emit_sub_memoffset_reg (OrcCompiler *compiler, int size,
    int offset, int reg, int destreg)
{
  if (size == 2) {
    ORC_ASM_CODE(compiler,"  subw %d(%%%s), %%%s\n", offset,
        orc_x86_get_regname_ptr(compiler, reg),
        orc_x86_get_regname_16(destreg));
    *compiler->codeptr++ = 0x66;
  } else if (size == 4) {
    ORC_ASM_CODE(compiler,"  subl %d(%%%s), %%%s\n", offset,
        orc_x86_get_regname_ptr(compiler, reg),
        orc_x86_get_regname(destreg));
  } else {
    ORC_ASM_CODE(compiler,"  sub %d(%%%s), %%%s\n", offset,
        orc_x86_get_regname_ptr(compiler, reg),
        orc_x86_get_regname_64(destreg));
  }

  orc_x86_emit_rex(compiler, size, 0, 0, reg);
  *compiler->codeptr++ = 0x2b;
  orc_x86_emit_modrm_memoffset (compiler, destreg, offset, reg);
}

void
orc_x86_emit_cmp_reg_memoffset (OrcCompiler *compiler, int size, int reg1,
    int offset, int reg)
{
  if (size == 2) {
    ORC_ASM_CODE(compiler,"  cmpw %%%s, %d(%%%s)\n", orc_x86_get_regname_16(reg1), offset,
        orc_x86_get_regname_ptr(compiler, reg));
    *compiler->codeptr++ = 0x66;
  } else if (size == 4) {
    ORC_ASM_CODE(compiler,"  cmpl %%%s, %d(%%%s)\n", orc_x86_get_regname(reg1), offset,
        orc_x86_get_regname_ptr(compiler, reg));
  } else {
    ORC_ASM_CODE(compiler,"  cmp %%%s, %d(%%%s)\n", orc_x86_get_regname_64(reg1), offset,
        orc_x86_get_regname_ptr(compiler, reg));
  }

  orc_x86_emit_rex(compiler, size, 0, 0, reg);
  *compiler->codeptr++ = 0x39;
  orc_x86_emit_modrm_memoffset (compiler, reg1, offset, reg);
}

void
orc_x86_emit_cmp_imm_memoffset (OrcCompiler *compiler, int size, int value,
    int offset, int reg)
{
  if (size == 2) {
    ORC_ASM_CODE(compiler,"  cmpw $%d, %d(%%%s)\n", value, offset,
        orc_x86_get_regname_ptr(compiler, reg));
    *compiler->codeptr++ = 0x66;
  } else if (size == 4) {
    ORC_ASM_CODE(compiler,"  cmpl $%d, %d(%%%s)\n", value, offset,
        orc_x86_get_regname_ptr(compiler, reg));
  } else {
    ORC_ASM_CODE(compiler,"  cmp $%d, %d(%%%s)\n", value, offset,
        orc_x86_get_regname_ptr(compiler, reg));
  }

  orc_x86_emit_rex(compiler, size, 0, 0, reg);
  if (value >= -128 && value < 128) {
    *compiler->codeptr++ = 0x83;
    orc_x86_emit_modrm_memoffset (compiler, 7, offset, reg);
    *compiler->codeptr++ = (value & 0xff);
  } else {
    *compiler->codeptr++ = 0x81;
    orc_x86_emit_modrm_memoffset (compiler, 7, offset, reg);
    *compiler->codeptr++ = (value & 0xff);
    *compiler->codeptr++ = ((value>>8) & 0xff);
    if (size == 4) {
      *compiler->codeptr++ = ((value>>16) & 0xff);
      *compiler->codeptr++ = ((value>>24) & 0xff);
    }
  }
}

void
orc_x86_emit_test_imm_memoffset (OrcCompiler *compiler, int size, int value,
    int offset, int reg)
{
  if (size == 2) {
    ORC_ASM_CODE(compiler,"  testw $%d, %d(%%%s)\n", value, offset,
        orc_x86_get_regname_ptr(compiler, reg));
    *compiler->codeptr++ = 0x66;
  } else if (size == 4) {
    ORC_ASM_CODE(compiler,"  testl $%d, %d(%%%s)\n", value, offset,
        orc_x86_get_regname_ptr(compiler, reg));
  } else {
    ORC_ASM_CODE(compiler,"  test $%d, %d(%%%s)\n", value, offset,
        orc_x86_get_regname_ptr(compiler, reg));
  }

  orc_x86_emit_rex(compiler, size, 0, 0, reg);

  *compiler->codeptr++ = 0xf7;
  orc_x86_emit_modrm_memoffset (compiler, 0, offset, reg);
  *compiler->codeptr++ = (value & 0xff);
  *compiler->codeptr++ = ((value>>8) & 0xff);
  if (size == 4) {
    *compiler->codeptr++ = ((value>>16) & 0xff);
    *compiler->codeptr++ = ((value>>24) & 0xff);
  }
}

void
orc_x86_emit_dec_memoffset (OrcCompiler *compiler, int size,
    int offset, int reg)
{
  if (size == 2) {
    ORC_ASM_CODE(compiler,"  decw %d(%%%s)\n", offset, orc_x86_get_regname_ptr(compiler, reg));
    *compiler->codeptr++ = 0x66;
  } else if (size == 4) {
    ORC_ASM_CODE(compiler,"  addl $-1, %d(%%%s)\n", offset, orc_x86_get_regname_ptr(compiler, reg));
    orc_x86_emit_rex(compiler, size, 0, 0, reg);
    *compiler->codeptr++ = 0x83;
    orc_x86_emit_modrm_memoffset (compiler, 0, offset, reg);
    *compiler->codeptr++ = 0xff;
    return;
  } else {
    ORC_ASM_CODE(compiler,"  dec %d(%%%s)\n", offset, orc_x86_get_regname_ptr(compiler, reg));
  }

  orc_x86_emit_rex(compiler, size, 0, 0, reg);
  *compiler->codeptr++ = 0xff;
  orc_x86_emit_modrm_memoffset (compiler, 1, offset, reg);
}

void orc_x86_emit_ret (OrcCompiler *compiler)
{
  if (compiler->is_64bit) {
    ORC_ASM_CODE(compiler,"  retq\n");
  } else {
    ORC_ASM_CODE(compiler,"  ret\n");
  }
  *compiler->codeptr++ = 0xc3;
}

void orc_x86_emit_emms (OrcCompiler *compiler)
{
  ORC_ASM_CODE(compiler,"  emms\n");
  *compiler->codeptr++ = 0x0f;
  *compiler->codeptr++ = 0x77;
}

void orc_x86_emit_rep_movs (OrcCompiler *compiler, int size)
{
  switch (size) {
    case 1:
      ORC_ASM_CODE(compiler,"  rep movsb\n");
      *compiler->codeptr++ = 0xf3;
      *compiler->codeptr++ = 0xa4;
      break;
    case 2:
      ORC_ASM_CODE(compiler,"  rep movsw\n");
      *compiler->codeptr++ = 0x66;
      *compiler->codeptr++ = 0xf3;
      *compiler->codeptr++ = 0xa5;
      break;
    case 4:
      ORC_ASM_CODE(compiler,"  rep movsl\n");
      *compiler->codeptr++ = 0xf3;
      *compiler->codeptr++ = 0xa5;
      break;
  }
}

void
x86_add_fixup (OrcCompiler *compiler, unsigned char *ptr, int label, int type)
{
  compiler->fixups[compiler->n_fixups].ptr = ptr;
  compiler->fixups[compiler->n_fixups].label = label;
  compiler->fixups[compiler->n_fixups].type = type;
  compiler->n_fixups++;
}

void
x86_add_label (OrcCompiler *compiler, unsigned char *ptr, int label)
{
  compiler->labels[label] = ptr;
}

void orc_x86_emit_jmp (OrcCompiler *compiler, int label)
{
  ORC_ASM_CODE(compiler,"  jmp %d%c\n", label,
      (compiler->labels[label]!=NULL) ? 'b' : 'f');

  if (compiler->long_jumps) {
    *compiler->codeptr++ = 0xe9;
    x86_add_fixup (compiler, compiler->codeptr, label, 1);
    *compiler->codeptr++ = 0xfc;
    *compiler->codeptr++ = 0xff;
    *compiler->codeptr++ = 0xff;
    *compiler->codeptr++ = 0xff;
  } else {
    *compiler->codeptr++ = 0xeb;
    x86_add_fixup (compiler, compiler->codeptr, label, 0);
    *compiler->codeptr++ = 0xff;
  }
}

void orc_x86_emit_jle (OrcCompiler *compiler, int label)
{
  ORC_ASM_CODE(compiler,"  jle %d%c\n", label,
      (compiler->labels[label]!=NULL) ? 'b' : 'f');

  if (compiler->long_jumps) {
    *compiler->codeptr++ = 0x0f;
    *compiler->codeptr++ = 0x8e;
    x86_add_fixup (compiler, compiler->codeptr, label, 1);
    *compiler->codeptr++ = 0xfc;
    *compiler->codeptr++ = 0xff;
    *compiler->codeptr++ = 0xff;
    *compiler->codeptr++ = 0xff;
  } else {
    *compiler->codeptr++ = 0x7e;
    x86_add_fixup (compiler, compiler->codeptr, label, 0);
    *compiler->codeptr++ = 0xff;
  }
}

void orc_x86_emit_je (OrcCompiler *compiler, int label)
{
  ORC_ASM_CODE(compiler,"  je %d%c\n", label,
      (compiler->labels[label]!=NULL) ? 'b' : 'f');

  if (compiler->long_jumps) {
    *compiler->codeptr++ = 0x0f;
    *compiler->codeptr++ = 0x84;
    x86_add_fixup (compiler, compiler->codeptr, label, 1);
    *compiler->codeptr++ = 0xfc;
    *compiler->codeptr++ = 0xff;
    *compiler->codeptr++ = 0xff;
    *compiler->codeptr++ = 0xff;
  } else {
    *compiler->codeptr++ = 0x74;
    x86_add_fixup (compiler, compiler->codeptr, label, 0);
    *compiler->codeptr++ = 0xff;
  }
}

void orc_x86_emit_jne (OrcCompiler *compiler, int label)
{
  ORC_ASM_CODE(compiler,"  jne %d%c\n", label,
      (compiler->labels[label]!=NULL) ? 'b' : 'f');

  if (compiler->long_jumps) {
    *compiler->codeptr++ = 0x0f;
    *compiler->codeptr++ = 0x85;
    x86_add_fixup (compiler, compiler->codeptr, label, 1);
    *compiler->codeptr++ = 0xfc;
    *compiler->codeptr++ = 0xff;
    *compiler->codeptr++ = 0xff;
    *compiler->codeptr++ = 0xff;
  } else {
    *compiler->codeptr++ = 0x75;
    x86_add_fixup (compiler, compiler->codeptr, label, 0);
    *compiler->codeptr++ = 0xff;
  }
}

void orc_x86_emit_label (OrcCompiler *compiler, int label)
{
  ORC_ASM_CODE(compiler,"%d:\n", label);

  x86_add_label (compiler, compiler->codeptr, label);
}

void
orc_x86_do_fixups (OrcCompiler *compiler)
{
  int i;
  for(i=0;i<compiler->n_fixups;i++){
    if (compiler->fixups[i].type == 0) {
      unsigned char *label = compiler->labels[compiler->fixups[i].label];
      unsigned char *ptr = compiler->fixups[i].ptr;
      int diff;

      diff = ((orc_int8)ptr[0]) + (label - ptr);
      if (diff != (orc_int8)diff) {
        ORC_COMPILER_ERROR(compiler, "short jump too long");
      }

      ptr[0] = diff;
    } else if (compiler->fixups[i].type == 1) {
      unsigned char *label = compiler->labels[compiler->fixups[i].label];
      unsigned char *ptr = compiler->fixups[i].ptr;
      int diff;

      diff = ORC_READ_UINT32_LE (ptr) + (label - ptr);
      ORC_WRITE_UINT32_LE(ptr, diff);
    }
  }
}

void
orc_x86_emit_prologue (OrcCompiler *compiler)
{
  orc_compiler_append_code(compiler,".global %s\n", compiler->program->name);
  orc_compiler_append_code(compiler,".p2align 4\n");
  orc_compiler_append_code(compiler,"%s:\n", compiler->program->name);
  if (compiler->is_64bit) {
    int i;
    for(i=0;i<16;i++){
      if (compiler->used_regs[ORC_GP_REG_BASE+i] &&
          compiler->save_regs[ORC_GP_REG_BASE+i]) {
        orc_x86_emit_push (compiler, 8, ORC_GP_REG_BASE+i);
      }
    }
  } else {
    orc_x86_emit_push (compiler, 4, X86_EBP);
    if (compiler->use_frame_pointer) {
      orc_x86_emit_mov_reg_reg (compiler, 4, X86_ESP, X86_EBP);
    }
    orc_x86_emit_mov_memoffset_reg (compiler, 4, 8, X86_ESP, compiler->exec_reg);
    if (compiler->used_regs[X86_EDI]) {
      orc_x86_emit_push (compiler, 4, X86_EDI);
    }
    if (compiler->used_regs[X86_ESI]) {
      orc_x86_emit_push (compiler, 4, X86_ESI);
    }
    if (compiler->used_regs[X86_EBX]) {
      orc_x86_emit_push (compiler, 4, X86_EBX);
    }
  }

}

void
orc_x86_emit_epilogue (OrcCompiler *compiler)
{
  if (compiler->is_64bit) {
    int i;
    for(i=15;i>=0;i--){
      if (compiler->used_regs[ORC_GP_REG_BASE+i] &&
          compiler->save_regs[ORC_GP_REG_BASE+i]) {
        orc_x86_emit_pop (compiler, 8, ORC_GP_REG_BASE+i);
      }
    }
  } else {
    if (compiler->used_regs[X86_EBX]) {
      orc_x86_emit_pop (compiler, 4, X86_EBX);
    }
    if (compiler->used_regs[X86_ESI]) {
      orc_x86_emit_pop (compiler, 4, X86_ESI);
    }
    if (compiler->used_regs[X86_EDI]) {
      orc_x86_emit_pop (compiler, 4, X86_EDI);
    }
    orc_x86_emit_pop (compiler, 4, X86_EBP);
  }
  orc_x86_emit_ret (compiler);
}

void
orc_x86_emit_align (OrcCompiler *compiler)
{
  int diff;
  int align_shift = 4;

  diff = (compiler->program->code - compiler->codeptr)&((1<<align_shift) - 1);
  while (diff) {
    ORC_ASM_CODE(compiler,"  nop\n");
    *compiler->codeptr++ = 0x90;
    diff--;
  }
}

/* memcpy implementation based on rep movs */

int
orc_x86_assemble_copy_check (OrcCompiler *compiler)
{
  if (compiler->program->n_insns == 1 &&
      compiler->program->is_2d == FALSE &&
      (strcmp (compiler->program->insns[0].opcode->name, "copyb") == 0 ||
      strcmp (compiler->program->insns[0].opcode->name, "copyw") == 0 ||
      strcmp (compiler->program->insns[0].opcode->name, "copyl") == 0)) {
    return TRUE;
  }

  return FALSE;
}

void
orc_x86_assemble_copy (OrcCompiler *compiler)
{
  OrcInstruction *insn;
  int shift = 0;

  insn = compiler->program->insns + 0;

  if (strcmp (insn->opcode->name, "copyw") == 0) {
    shift = 1;
  } else if (strcmp (insn->opcode->name, "copyl") == 0) {
    shift = 2;
  }

  compiler->used_regs[X86_EDI] = TRUE;
  compiler->used_regs[X86_ESI] = TRUE;

  orc_x86_emit_prologue (compiler);

  orc_x86_emit_mov_memoffset_reg (compiler, 4,
      (int)ORC_STRUCT_OFFSET(OrcExecutor,arrays[insn->dest_args[0]]),
      compiler->exec_reg, X86_EDI);
  orc_x86_emit_mov_memoffset_reg (compiler, 4,
      (int)ORC_STRUCT_OFFSET(OrcExecutor,arrays[insn->src_args[0]]),
      compiler->exec_reg, X86_ESI);
  orc_x86_emit_mov_memoffset_reg (compiler, 4,
      (int)ORC_STRUCT_OFFSET(OrcExecutor,n), compiler->exec_reg,
      X86_ECX);

  orc_x86_emit_sar_imm_reg (compiler, 4, 2 - shift, X86_ECX);
  orc_x86_emit_rep_movs (compiler, 4);
  if (shift == 0) {
    orc_x86_emit_mov_memoffset_reg (compiler, 4,
        (int)ORC_STRUCT_OFFSET(OrcExecutor,n), compiler->exec_reg,
        X86_ECX);
    orc_x86_emit_and_imm_reg (compiler, 4, 3, X86_ECX);
    orc_x86_emit_rep_movs (compiler, 1);
  }
  if (shift == 1) {
    orc_x86_emit_mov_memoffset_reg (compiler, 4,
        (int)ORC_STRUCT_OFFSET(OrcExecutor,n), compiler->exec_reg,
        X86_ECX);
    orc_x86_emit_and_imm_reg (compiler, 4, 1, X86_ECX);
    orc_x86_emit_rep_movs (compiler, 2);
  }

  orc_x86_emit_epilogue (compiler);

  orc_x86_do_fixups (compiler);
}

