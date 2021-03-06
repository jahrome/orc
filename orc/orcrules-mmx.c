
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>

#include <orc/orcprogram.h>
#include <orc/orcdebug.h>
#include <orc/orcmmx.h>

#define MMX 1
#define SIZE 65536

/* sse rules */

static void
mmx_rule_loadpX (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  OrcVariable *src = compiler->vars + insn->src_args[0];
  OrcVariable *dest = compiler->vars + insn->dest_args[0];
  int reg;

  if (src->vartype == ORC_VAR_TYPE_PARAM) {
    reg = dest->alloc;

    orc_x86_emit_mov_memoffset_mmx (compiler, 4,
        (int)ORC_STRUCT_OFFSET(OrcExecutor, params[insn->src_args[0]]),
        compiler->exec_reg, reg, FALSE);
    if (src->size == 1) {
      orc_mmx_emit_punpcklbw (compiler, reg, reg);
    }
#ifndef MMX
    if (src->size <= 2) {
      orc_mmx_emit_pshuflw (compiler, 0, reg, reg);
    }
    orc_mmx_emit_pshufd (compiler, 0, reg, reg);
#else
    if (src->size <= 2) {
      orc_mmx_emit_pshufw (compiler, ORC_MMX_SHUF(0,0,0,0), reg, reg);
    } else {
      orc_mmx_emit_pshufw (compiler, ORC_MMX_SHUF(1,0,1,0), reg, reg);
    }
#endif
  } else if (src->vartype == ORC_VAR_TYPE_CONST) {
    int value = src->value;

    reg = dest->alloc;

    if (src->size == 1) {
      value &= 0xff;
      value |= (value<<8);
      value |= (value<<16);
    }
    if (src->size == 2) {
      value &= 0xffff;
      value |= (value<<16);
    }

    if (value == 0) {
      orc_mmx_emit_pxor(compiler, reg, reg);
    } else {
      orc_x86_emit_mov_imm_reg (compiler, 4, value, compiler->gp_tmpreg);
      orc_x86_emit_mov_reg_mmx (compiler, compiler->gp_tmpreg, reg);
#ifndef MMX
      orc_mmx_emit_pshufd (compiler, 0, reg, reg);
#else
      orc_mmx_emit_pshufw (compiler, ORC_MMX_SHUF(1,0,1,0), reg, reg);
#endif
    }
  }
}

static void
mmx_rule_loadX (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  OrcVariable *src = compiler->vars + insn->src_args[0];
  OrcVariable *dest = compiler->vars + insn->dest_args[0];
  int ptr_reg;
  int offset = 0;

  offset = compiler->offset * src->size;
  if (src->ptr_register == 0) {
    int i = insn->src_args[0];
    orc_x86_emit_mov_memoffset_reg (compiler, compiler->is_64bit ? 8 : 4,
        (int)ORC_STRUCT_OFFSET(OrcExecutor, arrays[i]),
        compiler->exec_reg, compiler->gp_tmpreg);
    ptr_reg = compiler->gp_tmpreg;
  } else {
    ptr_reg = src->ptr_register;
  } 
  switch (src->size << compiler->loop_shift) {
    case 1:
      orc_x86_emit_mov_memoffset_reg (compiler, 1, offset, ptr_reg,
          compiler->gp_tmpreg);
      orc_x86_emit_mov_reg_mmx (compiler, compiler->gp_tmpreg, dest->alloc);
      break;
    case 2:
      orc_x86_emit_mov_memoffset_reg (compiler, 2, offset, ptr_reg,
          compiler->gp_tmpreg);
      orc_x86_emit_mov_reg_mmx (compiler, compiler->gp_tmpreg, dest->alloc);
      break;
    case 4:
      orc_x86_emit_mov_memoffset_mmx (compiler, 4, offset, ptr_reg,
          dest->alloc, src->is_aligned);
      break;
    case 8:
      orc_x86_emit_mov_memoffset_mmx (compiler, 8, offset, ptr_reg,
          dest->alloc, src->is_aligned);
      break;
    case 16:
      orc_x86_emit_mov_memoffset_mmx (compiler, 16, offset, ptr_reg,
          dest->alloc, src->is_aligned);
      break;
    default:
      ORC_COMPILER_ERROR(compiler,"bad load size %d",
          src->size << compiler->loop_shift);
      break;
  }
}

static void
mmx_rule_storeX (OrcCompiler *compiler, void *user, OrcInstruction *insn)
{
  OrcVariable *src = compiler->vars + insn->src_args[0];
  OrcVariable *dest = compiler->vars + insn->dest_args[0];
  int offset;
  int ptr_reg;

  offset = compiler->offset * dest->size;
  if (dest->ptr_register == 0) {
    orc_x86_emit_mov_memoffset_reg (compiler, compiler->is_64bit ? 8 : 4,
        dest->ptr_offset, compiler->exec_reg, compiler->gp_tmpreg);
    ptr_reg = compiler->gp_tmpreg; 
  } else {
    ptr_reg = dest->ptr_register;
  } 
  switch (dest->size << compiler->loop_shift) {
    case 1:
      /* FIXME we might be using ecx twice here */
      if (ptr_reg == compiler->gp_tmpreg) {
        ORC_COMPILER_ERROR(compiler,"unimplemented");
      }
      orc_x86_emit_mov_mmx_reg (compiler, src->alloc, compiler->gp_tmpreg);
      orc_x86_emit_mov_reg_memoffset (compiler, 1, compiler->gp_tmpreg, offset, ptr_reg);
      break;
    case 2:
      /* FIXME we might be using ecx twice here */
      if (ptr_reg == compiler->gp_tmpreg) {
        ORC_COMPILER_ERROR(compiler,"unimplemented");
      } 
      orc_x86_emit_mov_mmx_reg (compiler, src->alloc, compiler->gp_tmpreg);
      orc_x86_emit_mov_reg_memoffset (compiler, 2, compiler->gp_tmpreg, offset, ptr_reg);
      break;
    case 4:
      orc_x86_emit_mov_mmx_memoffset (compiler, 4, src->alloc, offset, ptr_reg,
          dest->is_aligned, dest->is_uncached);
      break;
    case 8:
      orc_x86_emit_mov_mmx_memoffset (compiler, 8, src->alloc, offset, ptr_reg,
          dest->is_aligned, dest->is_uncached);
      break;
    case 16:
      orc_x86_emit_mov_mmx_memoffset (compiler, 16, src->alloc, offset, ptr_reg,
          dest->is_aligned, dest->is_uncached);
      break;
    default:
      ORC_COMPILER_ERROR(compiler,"bad size");
      break;
  }
}

static void
mmx_rule_copyx (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->vars[insn->src_args[0]].alloc == p->vars[insn->dest_args[0]].alloc) {
    return;
  }

  orc_mmx_emit_movq (p,
      p->vars[insn->src_args[0]].alloc,
      p->vars[insn->dest_args[0]].alloc);
}

#define UNARY(opcode,insn_name,code) \
static void \
mmx_rule_ ## opcode (OrcCompiler *p, void *user, OrcInstruction *insn) \
{ \
  orc_mmx_emit_660f (p, insn_name, code, \
      p->vars[insn->src_args[0]].alloc, \
      p->vars[insn->dest_args[0]].alloc); \
}

#define BINARY(opcode,insn_name,code) \
static void \
mmx_rule_ ## opcode (OrcCompiler *p, void *user, OrcInstruction *insn) \
{ \
  orc_mmx_emit_660f (p, insn_name, code, \
      p->vars[insn->src_args[1]].alloc, \
      p->vars[insn->dest_args[0]].alloc); \
}


UNARY(absb,"pabsb",0x381c)
BINARY(addb,"paddb",0xfc)
BINARY(addssb,"paddsb",0xec)
BINARY(addusb,"paddusb",0xdc)
BINARY(andb,"pand",0xdb)
BINARY(andnb,"pandn",0xdf)
BINARY(avgub,"pavgb",0xe0)
BINARY(cmpeqb,"pcmpeqb",0x74)
BINARY(cmpgtsb,"pcmpgtb",0x64)
BINARY(maxsb,"pmaxsb",0x383c)
BINARY(maxub,"pmaxub",0xde)
BINARY(minsb,"pminsb",0x3838)
BINARY(minub,"pminub",0xda)
//BINARY(mullb,"pmullb",0xd5)
//BINARY(mulhsb,"pmulhb",0xe5)
//BINARY(mulhub,"pmulhub",0xe4)
BINARY(orb,"por",0xeb)
//UNARY(signb,"psignb",0x3808)
BINARY(subb,"psubb",0xf8)
BINARY(subssb,"psubsb",0xe8)
BINARY(subusb,"psubusb",0xd8)
BINARY(xorb,"pxor",0xef)

UNARY(absw,"pabsw",0x381d)
BINARY(addw,"paddw",0xfd)
BINARY(addssw,"paddsw",0xed)
BINARY(addusw,"paddusw",0xdd)
BINARY(andw,"pand",0xdb)
BINARY(andnw,"pandn",0xdf)
BINARY(avguw,"pavgw",0xe3)
BINARY(cmpeqw,"pcmpeqw",0x75)
BINARY(cmpgtsw,"pcmpgtw",0x65)
BINARY(maxsw,"pmaxsw",0xee)
BINARY(maxuw,"pmaxuw",0x383e)
BINARY(minsw,"pminsw",0xea)
BINARY(minuw,"pminuw",0x383a)
BINARY(mullw,"pmullw",0xd5)
BINARY(mulhsw,"pmulhw",0xe5)
BINARY(mulhuw,"pmulhuw",0xe4)
BINARY(orw,"por",0xeb)
//UNARY(signw,"psignw",0x3809)
BINARY(subw,"psubw",0xf9)
BINARY(subssw,"psubsw",0xe9)
BINARY(subusw,"psubusw",0xd9)
BINARY(xorw,"pxor",0xef)

UNARY(absl,"pabsd",0x381e)
BINARY(addl,"paddd",0xfe)
//BINARY(addssl,"paddsd",0xed)
//BINARY(addusl,"paddusd",0xdd)
BINARY(andl,"pand",0xdb)
BINARY(andnl,"pandn",0xdf)
//BINARY(avgul,"pavgd",0xe3)
BINARY(cmpeql,"pcmpeqd",0x76)
BINARY(cmpgtsl,"pcmpgtd",0x66)
BINARY(maxsl,"pmaxsd",0x383d)
BINARY(maxul,"pmaxud",0x383f)
BINARY(minsl,"pminsd",0x3839)
BINARY(minul,"pminud",0x383b)
BINARY(mulll,"pmulld",0x3840)
//BINARY(mulhsl,"pmulhd",0xe5)
//BINARY(mulhul,"pmulhud",0xe4)
BINARY(orl,"por",0xeb)
//UNARY(signl,"psignd",0x380a)
BINARY(subl,"psubd",0xfa)
//BINARY(subssl,"psubsd",0xe9)
//BINARY(subusl,"psubusd",0xd9)
BINARY(xorl,"pxor",0xef)


static void
mmx_rule_accw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;

  orc_mmx_emit_paddw (p, src, dest);
}

static void
mmx_rule_accl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;

#ifndef MMX
  if (p->loop_shift == 0) {
    orc_mmx_emit_pslldq (p, 12, src);
  }
#endif
  orc_mmx_emit_paddd (p, src, dest);
}

static void
mmx_rule_accsadubl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src1 = p->vars[insn->src_args[0]].alloc;
  int src2 = p->vars[insn->src_args[1]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp = p->tmpreg;
#ifndef MMX
  int tmp2 = X86_MM7;

  if (p->loop_shift == 0) {
    orc_mmx_emit_movq (p, src1, tmp);
    orc_mmx_emit_pslldq (p, 15, tmp);
    orc_mmx_emit_movq (p, src2, tmp2);
    orc_mmx_emit_pslldq (p, 15, tmp2);
    orc_mmx_emit_psadbw (p, tmp2, tmp);
  } else if (p->loop_shift == 1) {
    orc_mmx_emit_movq (p, src1, tmp);
    orc_mmx_emit_pslldq (p, 14, tmp);
    orc_mmx_emit_movq (p, src2, tmp2);
    orc_mmx_emit_pslldq (p, 14, tmp2);
    orc_mmx_emit_psadbw (p, tmp2, tmp);
  } else if (p->loop_shift == 2) {
    orc_mmx_emit_movq (p, src1, tmp);
    orc_mmx_emit_psadbw (p, src2, tmp);
    orc_mmx_emit_pslldq (p, 12, tmp);
  } else if (p->loop_shift == 3) {
    orc_mmx_emit_movq (p, src1, tmp);
    orc_mmx_emit_psadbw (p, src2, tmp);
    orc_mmx_emit_pslldq (p, 8, tmp);
  } else {
    orc_mmx_emit_movq (p, src1, tmp);
    orc_mmx_emit_psadbw (p, src2, tmp);
  }
#else
  orc_mmx_emit_movq (p, src1, tmp);
  orc_mmx_emit_psadbw (p, src2, tmp);
#endif
  orc_mmx_emit_paddd (p, tmp, dest);
}

static void
mmx_rule_signX_ssse3 (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  const char * names[] = { "psignb", "psignw", "psignd" };
  int codes[] = { 0x3808, 0x3809, 0x380a };
  int type = ORC_PTR_TO_INT(user);
  int tmpc;

  if (src == dest) {
    tmpc = orc_compiler_get_constant (p, 1<<type, 1);
    orc_mmx_emit_660f (p, names[type], codes[type], src, tmpc);
    orc_mmx_emit_movq (p, tmpc, dest);
  } else {
    tmpc = orc_compiler_get_constant (p, 1<<type, 1);
    orc_mmx_emit_movq (p, tmpc, dest);
    orc_mmx_emit_660f (p, names[type], codes[type], src, dest);
  }
}

static void
mmx_rule_signw_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp = p->tmpreg;

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  tmp = orc_compiler_get_constant (p, 2, 0x0001);
  orc_mmx_emit_pminsw (p, tmp, dest);

  tmp = orc_compiler_get_constant (p, 2, 0xffff);
  orc_mmx_emit_pmaxsw (p, tmp, dest);
}

static void
mmx_rule_absb_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp = p->tmpreg;

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  orc_mmx_emit_pxor (p, tmp, tmp);
  orc_mmx_emit_pcmpgtb (p, src, tmp);
  orc_mmx_emit_pxor (p, tmp, dest);
  orc_mmx_emit_psubb (p, tmp, dest);
}

static void
mmx_rule_absw_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp = p->tmpreg;

  if (src == dest) {
    orc_mmx_emit_movq (p, src, tmp);
  } else {
    orc_mmx_emit_movq (p, src, tmp);
    orc_mmx_emit_movq (p, tmp, dest);
  }

  orc_mmx_emit_psraw (p, 15, tmp);
  orc_mmx_emit_pxor (p, tmp, dest);
  orc_mmx_emit_psubw (p, tmp, dest);

}

static void
mmx_rule_absl_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp = p->tmpreg;

  if (src == dest) {
    orc_mmx_emit_movq (p, src, tmp);
  } else {
    orc_mmx_emit_movq (p, src, tmp);
    orc_mmx_emit_movq (p, tmp, dest);
  }

  orc_mmx_emit_psrad (p, 31, tmp);
  orc_mmx_emit_pxor (p, tmp, dest);
  orc_mmx_emit_psubd (p, tmp, dest);

}

static void
mmx_rule_shift (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int type = ORC_PTR_TO_INT(user);
  int imm_code1[] = { 0x71, 0x71, 0x71, 0x72, 0x72, 0x72 };
  int imm_code2[] = { 6, 2, 4, 6, 2, 4 };
  int reg_code[] = { 0xf1, 0xd1, 0xe1, 0xf2, 0xd2, 0xe2 };
  const char *code[] = { "psllw", "psrlw", "psraw", "pslld", "psrld", "psrad" };

  if (p->vars[insn->src_args[1]].vartype == ORC_VAR_TYPE_CONST) {
    orc_mmx_emit_shiftimm (p, code[type], imm_code1[type], imm_code2[type],
        p->vars[insn->src_args[1]].value,
        p->vars[insn->dest_args[0]].alloc);
  } else if (p->vars[insn->src_args[1]].vartype == ORC_VAR_TYPE_PARAM) {
    /* FIXME this is a gross hack to reload the register with a
     * 64-bit version of the parameter. */
    orc_x86_emit_mov_memoffset_mmx (p, 4,
        (int)ORC_STRUCT_OFFSET(OrcExecutor, params[insn->src_args[1]]),
        p->exec_reg, p->tmpreg, FALSE);

    orc_mmx_emit_660f (p, code[type], reg_code[type],
        p->tmpreg,
        p->vars[insn->dest_args[0]].alloc);
  } else {
    ORC_COMPILER_ERROR(p,"rule only works with constants or params");
    p->result = ORC_COMPILE_RESULT_UNKNOWN_COMPILE;
  }
}

static void
mmx_rule_convsbw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;

  orc_mmx_emit_punpcklbw (p, src, dest);
  orc_mmx_emit_psraw (p, 8, dest);
}

static void
mmx_rule_convubw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp = p->tmpreg;

  /* FIXME need a zero register */
  if (0) {
    orc_mmx_emit_punpcklbw (p, src, dest);
    orc_mmx_emit_psrlw (p, 8, dest);
  } else {
    if (src != dest) {
      orc_mmx_emit_movq (p, src, dest);
    }
    orc_mmx_emit_pxor(p, tmp, tmp);
    orc_mmx_emit_punpcklbw (p, tmp, dest);
  }
}

static void
mmx_rule_convssswb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;

  orc_mmx_emit_packsswb (p, src, dest);
}

static void
mmx_rule_convsuswb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;

  orc_mmx_emit_packuswb (p, src, dest);
}

static void
mmx_rule_convwb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;

  /* FIXME slow */

  if (dest != src) {
    orc_mmx_emit_movq (p, src, dest);
  }

  orc_mmx_emit_psllw (p, 8, dest);
  orc_mmx_emit_psrlw (p, 8, dest);
  orc_mmx_emit_packuswb (p, dest, dest);
}

static void
mmx_rule_convswl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;

  orc_mmx_emit_punpcklwd (p, src, dest);
  orc_mmx_emit_psrad (p, 16, dest);
}

static void
mmx_rule_convuwl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp = p->tmpreg;

  /* FIXME need a zero register */
  if (0) {
    orc_mmx_emit_punpcklwd (p, src, dest);
    orc_mmx_emit_psrld (p, 16, dest);
  } else {
    if (src != dest) {
      orc_mmx_emit_movq (p, src, dest);
    }
    orc_mmx_emit_pxor(p, tmp, tmp);
    orc_mmx_emit_punpcklwd (p, tmp, dest);
  }
}

static void
mmx_rule_convlw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;

  /* FIXME slow */

  if (dest != src) {
    orc_mmx_emit_movq (p, src, dest);
  }

  orc_mmx_emit_pslld (p, 16, dest);
  orc_mmx_emit_psrad (p, 16, dest);
  orc_mmx_emit_packssdw (p, dest, dest);
}

static void
mmx_rule_convssslw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;

  orc_mmx_emit_packssdw (p, src, dest);
}

static void
mmx_rule_convsuslw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;

  orc_mmx_emit_packusdw (p, src, dest);
}

static void
mmx_rule_mulsbw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[1]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp = p->tmpreg;

  orc_mmx_emit_punpcklbw (p, src, tmp);
  orc_mmx_emit_psraw (p, 8, tmp);
  orc_mmx_emit_punpcklbw (p, dest, dest);
  orc_mmx_emit_psraw (p, 8, dest);
  orc_mmx_emit_pmullw (p, tmp, dest);
}

static void
mmx_rule_mulubw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[1]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp = p->tmpreg;

  orc_mmx_emit_punpcklbw (p, src, tmp);
  orc_mmx_emit_psrlw (p, 8, tmp);
  orc_mmx_emit_punpcklbw (p, dest, dest);
  orc_mmx_emit_psrlw (p, 8, dest);
  orc_mmx_emit_pmullw (p, tmp, dest);
}

static void
mmx_rule_muluwl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[1]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp = p->tmpreg;

  orc_mmx_emit_punpcklwd (p, src, tmp);
  orc_mmx_emit_psrld (p, 16, tmp);
  orc_mmx_emit_punpcklwd (p, dest, dest);
  orc_mmx_emit_psrld (p, 16, dest);
  orc_mmx_emit_pmulld (p, tmp, dest);
}

static void
mmx_rule_mulswl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[1]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp = p->tmpreg;

  orc_mmx_emit_movq (p, dest, tmp);
  orc_mmx_emit_pmulhw (p, src, tmp);
  orc_mmx_emit_pmullw (p, src, dest);
  orc_mmx_emit_punpcklwd (p, tmp, dest);
}

static void
mmx_rule_select0lw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;

  /* FIXME slow */
  /* same as convlw */

  if (dest != src) {
    orc_mmx_emit_movq (p, src, dest);
  }

  orc_mmx_emit_pslld (p, 16, dest);
  orc_mmx_emit_psrad (p, 16, dest);
  orc_mmx_emit_packssdw (p, dest, dest);
}

static void
mmx_rule_select1lw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;

  /* FIXME slow */

  if (dest != src) {
    orc_mmx_emit_movq (p, src, dest);
  }

  orc_mmx_emit_psrad (p, 16, dest);
  orc_mmx_emit_packssdw (p, dest, dest);
}

static void
mmx_rule_select0wb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;

  /* FIXME slow */
  /* same as convwb */

  if (dest != src) {
    orc_mmx_emit_movq (p, src, dest);
  }

  orc_mmx_emit_psllw (p, 8, dest);
  orc_mmx_emit_psraw (p, 8, dest);
  orc_mmx_emit_packsswb (p, dest, dest);
}

static void
mmx_rule_select1wb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;

  /* FIXME slow */

  if (dest != src) {
    orc_mmx_emit_movq (p, src, dest);
  }

  orc_mmx_emit_psraw (p, 8, dest);
  orc_mmx_emit_packsswb (p, dest, dest);
}

static void
mmx_rule_splitlw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest1 = p->vars[insn->dest_args[0]].alloc;
  int dest2 = p->vars[insn->dest_args[1]].alloc;

  /* FIXME slow */

  if (dest1 != src) {
    orc_mmx_emit_movq (p, src, dest1);
  }
  orc_mmx_emit_psrad (p, 16, dest1);
  orc_mmx_emit_packssdw (p, dest1, dest1);

  if (dest2 != src) {
    orc_mmx_emit_movq (p, src, dest2);
  }
  orc_mmx_emit_pslld (p, 16, dest2);
  orc_mmx_emit_psrad (p, 16, dest2);
  orc_mmx_emit_packssdw (p, dest2, dest2);

}

static void
mmx_rule_splitwb (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest1 = p->vars[insn->dest_args[0]].alloc;
  int dest2 = p->vars[insn->dest_args[1]].alloc;

  /* FIXME slow */

  if (dest1 != src) {
    orc_mmx_emit_movq (p, src, dest1);
  }

  orc_mmx_emit_psraw (p, 8, dest1);
  orc_mmx_emit_packsswb (p, dest1, dest1);

  if (dest2 != src) {
    orc_mmx_emit_movq (p, src, dest2);
  }

  orc_mmx_emit_psllw (p, 8, dest2);
  orc_mmx_emit_psraw (p, 8, dest2);
  orc_mmx_emit_packsswb (p, dest2, dest2);
}

static void
mmx_rule_mergebw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[1]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;

  orc_mmx_emit_punpcklbw (p, src, dest);
}

static void
mmx_rule_mergewl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[1]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;

  orc_mmx_emit_punpcklwd (p, src, dest);
}

static void
mmx_rule_swapw (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp = p->tmpreg;

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }
  orc_mmx_emit_movq (p, src, tmp);
  orc_mmx_emit_psllw (p, 8, tmp);
  orc_mmx_emit_psrlw (p, 8, dest);
  orc_mmx_emit_por (p, tmp, dest);
}

static void
mmx_rule_swapl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp = p->tmpreg;

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }
  orc_mmx_emit_movq (p, src, tmp);
  orc_mmx_emit_pslld (p, 16, tmp);
  orc_mmx_emit_psrld (p, 16, dest);
  orc_mmx_emit_por (p, tmp, dest);
  orc_mmx_emit_movq (p, dest, tmp);
  orc_mmx_emit_psllw (p, 8, tmp);
  orc_mmx_emit_psrlw (p, 8, dest);
  orc_mmx_emit_por (p, tmp, dest);
}

#define LOAD_MASK_IS_SLOW
#ifndef LOAD_MASK_IS_SLOW
static void
mmx_emit_load_mask (OrcCompiler *p, unsigned int mask1, unsigned int mask2)
{
  int tmp = p->tmpreg;
  int gptmp = p->gp_tmpreg;
  int tmp2 = X86_MM7;

  orc_x86_emit_mov_imm_reg (p, 4, mask1, gptmp);
  orc_x86_emit_mov_reg_mmx (p, gptmp, tmp);
  orc_mmx_emit_pshufd (p, 0, tmp, tmp);
  orc_x86_emit_mov_imm_reg (p, 4, mask2, gptmp);
  orc_x86_emit_mov_reg_mmx (p, gptmp, tmp2);
  orc_mmx_emit_punpcklbw (p, tmp2, tmp2);
  orc_mmx_emit_punpcklwd (p, tmp2, tmp2);
  orc_mmx_emit_paddb (p, tmp2, tmp);
}

static void
mmx_rule_swapw_ssse3 (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp = p->tmpreg;

  mmx_emit_load_mask (p, 0x02030001, 0x0c080400);

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }
  orc_mmx_emit_pshufb (p, tmp, dest);
}

static void
mmx_rule_swapl_ssse3 (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp = p->tmpreg;

  /* FIXME slow */

  mmx_emit_load_mask (p, 0x00010203, 0x0c080400);

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }
  orc_mmx_emit_pshufb (p, tmp, dest);
}

static void
mmx_rule_select0lw_ssse3 (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp = p->tmpreg;

  /* FIXME slow */

  mmx_emit_load_mask (p, 0x05040100, 0x08000800);

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  orc_mmx_emit_pshufb (p, tmp, dest);
}

static void
mmx_rule_select1lw_ssse3 (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp = p->tmpreg;

  /* FIXME slow */

  mmx_emit_load_mask (p, 0x07060302, 0x08000800);

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  orc_mmx_emit_pshufb (p, tmp, dest);
}

static void
mmx_rule_select0wb_ssse3 (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp = p->tmpreg;

  /* FIXME slow */

  mmx_emit_load_mask (p, 0x06040200, 0x08000800);

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  orc_mmx_emit_pshufb (p, tmp, dest);
}

static void
mmx_rule_select1wb_ssse3 (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[0]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp = p->tmpreg;

  /* FIXME slow */

  mmx_emit_load_mask (p, 0x07050301, 0x08000800);

  if (src != dest) {
    orc_mmx_emit_movq (p, src, dest);
  }

  orc_mmx_emit_pshufb (p, tmp, dest);
}
#endif

/* slow rules */

static void
mmx_rule_maxuw_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[1]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp = p->tmpreg;

  tmp = orc_compiler_get_constant (p, 2, 0x8000);
  orc_mmx_emit_pxor(p, tmp, src);
  orc_mmx_emit_pxor(p, tmp, dest);
  orc_mmx_emit_pmaxsw (p, src, dest);
  orc_mmx_emit_pxor(p, tmp, src);
  orc_mmx_emit_pxor(p, tmp, dest);
}

static void
mmx_rule_minuw_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[1]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp;

  tmp = orc_compiler_get_constant (p, 2, 0x8000);

  orc_mmx_emit_pxor(p, tmp, src);
  orc_mmx_emit_pxor(p, tmp, dest);
  orc_mmx_emit_pminsw (p, src, dest);
  orc_mmx_emit_pxor(p, tmp, src);
  orc_mmx_emit_pxor(p, tmp, dest);
}

static void
mmx_rule_avgsb_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[1]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp;

  tmp = orc_compiler_get_constant (p, 1, 0x80);

  orc_mmx_emit_pxor(p, tmp, src);
  orc_mmx_emit_pxor(p, tmp, dest);
  orc_mmx_emit_pavgb (p, src, dest);
  orc_mmx_emit_pxor(p, tmp, src);
  orc_mmx_emit_pxor(p, tmp, dest);
}

static void
mmx_rule_avgsw_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[1]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp;

  tmp = orc_compiler_get_constant (p, 2, 0x8000);

  orc_mmx_emit_pxor(p, tmp, src);
  orc_mmx_emit_pxor(p, tmp, dest);
  orc_mmx_emit_pavgw (p, src, dest);
  orc_mmx_emit_pxor(p, tmp, src);
  orc_mmx_emit_pxor(p, tmp, dest);
}

static void
mmx_rule_maxsb_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[1]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp = p->tmpreg;

  orc_mmx_emit_movq (p, dest, tmp);
  orc_mmx_emit_pcmpgtb (p, src, tmp);
  orc_mmx_emit_pand (p, tmp, dest);
  orc_mmx_emit_pandn (p, src, tmp);
  orc_mmx_emit_por (p, tmp, dest);
}

static void
mmx_rule_minsb_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[1]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp = p->tmpreg;

  orc_mmx_emit_movq (p, src, tmp);
  orc_mmx_emit_pcmpgtb (p, dest, tmp);
  orc_mmx_emit_pand (p, tmp, dest);
  orc_mmx_emit_pandn (p, src, tmp);
  orc_mmx_emit_por (p, tmp, dest);
}

static void
mmx_rule_maxsl_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[1]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp = p->tmpreg;

  orc_mmx_emit_movq (p, dest, tmp);
  orc_mmx_emit_pcmpgtd (p, src, tmp);
  orc_mmx_emit_pand (p, tmp, dest);
  orc_mmx_emit_pandn (p, src, tmp);
  orc_mmx_emit_por (p, tmp, dest);
}

static void
mmx_rule_minsl_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[1]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp = p->tmpreg;

  orc_mmx_emit_movq (p, src, tmp);
  orc_mmx_emit_pcmpgtd (p, dest, tmp);
  orc_mmx_emit_pand (p, tmp, dest);
  orc_mmx_emit_pandn (p, src, tmp);
  orc_mmx_emit_por (p, tmp, dest);
}

static void
mmx_rule_maxul_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[1]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp;

  tmp = orc_compiler_get_constant (p, 4, 0x80000000);
  orc_mmx_emit_pxor(p, tmp, src);
  orc_mmx_emit_pxor(p, tmp, dest);

  orc_mmx_emit_movq (p, dest, tmp);
  orc_mmx_emit_pcmpgtd (p, src, tmp);
  orc_mmx_emit_pand (p, tmp, dest);
  orc_mmx_emit_pandn (p, src, tmp);
  orc_mmx_emit_por (p, tmp, dest);

  tmp = orc_compiler_get_constant (p, 4, 0x80000000);
  orc_mmx_emit_pxor(p, tmp, src);
  orc_mmx_emit_pxor(p, tmp, dest);
}

static void
mmx_rule_minul_slow (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[1]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp;

  tmp = orc_compiler_get_constant (p, 4, 0x80000000);
  orc_mmx_emit_pxor(p, tmp, src);
  orc_mmx_emit_pxor(p, tmp, dest);

  orc_mmx_emit_movq (p, src, tmp);
  orc_mmx_emit_pcmpgtd (p, dest, tmp);
  orc_mmx_emit_pand (p, tmp, dest);
  orc_mmx_emit_pandn (p, src, tmp);
  orc_mmx_emit_por (p, tmp, dest);

  tmp = orc_compiler_get_constant (p, 4, 0x80000000);
  orc_mmx_emit_pxor(p, tmp, src);
  orc_mmx_emit_pxor(p, tmp, dest);
}

static void
mmx_rule_avgsl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[1]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp = p->tmpreg;

  /* (a+b+1) >> 1 = (a|b) - ((a^b)>>1) */

  orc_mmx_emit_movq (p, dest, tmp);
  orc_mmx_emit_pxor(p, src, tmp);
  orc_mmx_emit_psrad(p, 1, tmp);

  orc_mmx_emit_por(p, src, dest);
  orc_mmx_emit_psubd(p, tmp, dest);
}

static void
mmx_rule_avgul (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  int src = p->vars[insn->src_args[1]].alloc;
  int dest = p->vars[insn->dest_args[0]].alloc;
  int tmp = p->tmpreg;

  /* (a+b+1) >> 1 = (a|b) - ((a^b)>>1) */

  orc_mmx_emit_movq (p, dest, tmp);
  orc_mmx_emit_pxor(p, src, tmp);
  orc_mmx_emit_psrld(p, 1, tmp);

  orc_mmx_emit_por(p, src, dest);
  orc_mmx_emit_psubd(p, tmp, dest);
}

#ifndef MMX
/* float ops */

#define UNARY_F(opcode,insn_name,code) \
static void \
mmx_rule_ ## opcode (OrcCompiler *p, void *user, OrcInstruction *insn) \
{ \
  orc_mmx_emit_0f (p, insn_name, code, \
      p->vars[insn->src_args[0]].alloc, \
      p->vars[insn->dest_args[0]].alloc); \
}

#define BINARY_F(opcode,insn_name,code) \
static void \
mmx_rule_ ## opcode (OrcCompiler *p, void *user, OrcInstruction *insn) \
{ \
  orc_mmx_emit_0f (p, insn_name, code, \
      p->vars[insn->src_args[1]].alloc, \
      p->vars[insn->dest_args[0]].alloc); \
}

BINARY_F(addf, "addps", 0x58)
BINARY_F(subf, "subps", 0x5c)
BINARY_F(mulf, "mulps", 0x59)
BINARY_F(divf, "divps", 0x5e)
UNARY_F(sqrtf, "sqrtps", 0x51)

static void
mmx_rule_minf (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->target_flags & ORC_TARGET_FAST_NAN) {
    orc_mmx_emit_0f (p, "minps", 0x5d,
        p->vars[insn->src_args[1]].alloc,
        p->vars[insn->dest_args[0]].alloc);
  } else {
    orc_mmx_emit_movq (p,
        p->vars[insn->src_args[1]].alloc,
        p->tmpreg);
    orc_mmx_emit_0f (p, "minps", 0x5d,
        p->vars[insn->dest_args[0]].alloc,
        p->tmpreg);
    orc_mmx_emit_0f (p, "minps", 0x5d,
        p->vars[insn->src_args[1]].alloc,
        p->vars[insn->dest_args[0]].alloc);
    orc_mmx_emit_por (p,
        p->tmpreg,
        p->vars[insn->dest_args[0]].alloc);
  }
}

static void
mmx_rule_maxf (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  if (p->target_flags & ORC_TARGET_FAST_NAN) {
    orc_mmx_emit_0f (p, "maxps", 0x5f,
        p->vars[insn->src_args[1]].alloc,
        p->vars[insn->dest_args[0]].alloc);
  } else {
    orc_mmx_emit_movq (p,
        p->vars[insn->src_args[1]].alloc,
        p->tmpreg);
    orc_mmx_emit_0f (p, "maxps", 0x5f,
        p->vars[insn->dest_args[0]].alloc,
        p->tmpreg);
    orc_mmx_emit_0f (p, "maxps", 0x5f,
        p->vars[insn->src_args[1]].alloc,
        p->vars[insn->dest_args[0]].alloc);
    orc_mmx_emit_por (p,
        p->tmpreg,
        p->vars[insn->dest_args[0]].alloc);
  }
}

static void
mmx_rule_cmpeqf (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  orc_mmx_emit_0f (p, "cmpeqps", 0xc2,
      p->vars[insn->src_args[1]].alloc,
      p->vars[insn->dest_args[0]].alloc);
  *p->codeptr++ = 0x00;
}

static void
mmx_rule_cmpltf (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  orc_mmx_emit_0f (p, "cmpltps", 0xc2,
      p->vars[insn->src_args[1]].alloc,
      p->vars[insn->dest_args[0]].alloc);
  *p->codeptr++ = 0x01;
}

static void
mmx_rule_cmplef (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  orc_mmx_emit_0f (p, "cmpleps", 0xc2,
      p->vars[insn->src_args[1]].alloc,
      p->vars[insn->dest_args[0]].alloc);
  *p->codeptr++ = 0x02;
}

static void
mmx_rule_convfl (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  orc_mmx_emit_f30f (p, "cvttps2dq", 0x5b,
      p->vars[insn->src_args[0]].alloc,
      p->vars[insn->dest_args[0]].alloc);
}

static void
mmx_rule_convlf (OrcCompiler *p, void *user, OrcInstruction *insn)
{
  orc_mmx_emit_0f (p, "cvtdq2ps", 0x5b,
      p->vars[insn->src_args[0]].alloc,
      p->vars[insn->dest_args[0]].alloc);
}
#endif

void
orc_compiler_mmx_register_rules (OrcTarget *target)
{
  OrcRuleSet *rule_set;

#define REG(x) \
  orc_rule_register (rule_set, #x , mmx_rule_ ## x, NULL)

  /* SSE 2 */
#ifndef MMX
  rule_set = orc_rule_set_new (orc_opcode_set_get("sys"), target,
      ORC_TARGET_MMX_MMXEXT);
#else
  rule_set = orc_rule_set_new (orc_opcode_set_get("sys"), target,
      ORC_TARGET_MMX_MMX);
#endif

  orc_rule_register (rule_set, "loadb", mmx_rule_loadX, NULL);
  orc_rule_register (rule_set, "loadw", mmx_rule_loadX, NULL);
  orc_rule_register (rule_set, "loadl", mmx_rule_loadX, NULL);
  orc_rule_register (rule_set, "loadpb", mmx_rule_loadpX, NULL);
  orc_rule_register (rule_set, "loadpw", mmx_rule_loadpX, NULL);
  orc_rule_register (rule_set, "loadpl", mmx_rule_loadpX, NULL);

  orc_rule_register (rule_set, "storeb", mmx_rule_storeX, NULL);
  orc_rule_register (rule_set, "storew", mmx_rule_storeX, NULL);
  orc_rule_register (rule_set, "storel", mmx_rule_storeX, NULL);

  REG(addb);
  REG(addssb);
  REG(addusb);
  REG(andb);
  REG(andnb);
  REG(avgub);
  REG(cmpeqb);
  REG(cmpgtsb);
  REG(maxub);
  REG(minub);
  REG(orb);
  REG(subb);
  REG(subssb);
  REG(subusb);
  REG(xorb);

  REG(addw);
  REG(addssw);
  REG(addusw);
  REG(andw);
  REG(andnw);
  REG(avguw);
  REG(cmpeqw);
  REG(cmpgtsw);
  REG(maxsw);
  REG(minsw);
  REG(mullw);
  REG(mulhsw);
  REG(mulhuw);
  REG(orw);
  REG(subw);
  REG(subssw);
  REG(subusw);
  REG(xorw);

  REG(addl);
  REG(andl);
  REG(andnl);
  REG(cmpeql);
  REG(cmpgtsl);
  REG(orl);
  REG(subl);
  REG(xorl);

  REG(select0lw);
  REG(select1lw);
  REG(select0wb);
  REG(select1wb);
  REG(mergebw);
  REG(mergewl);

  orc_rule_register (rule_set, "copyb", mmx_rule_copyx, NULL);
  orc_rule_register (rule_set, "copyw", mmx_rule_copyx, NULL);
  orc_rule_register (rule_set, "copyl", mmx_rule_copyx, NULL);

  orc_rule_register (rule_set, "shlw", mmx_rule_shift, (void *)0);
  orc_rule_register (rule_set, "shruw", mmx_rule_shift, (void *)1);
  orc_rule_register (rule_set, "shrsw", mmx_rule_shift, (void *)2);
  orc_rule_register (rule_set, "shll", mmx_rule_shift, (void *)3);
  orc_rule_register (rule_set, "shrul", mmx_rule_shift, (void *)4);
  orc_rule_register (rule_set, "shrsl", mmx_rule_shift, (void *)5);

  orc_rule_register (rule_set, "convsbw", mmx_rule_convsbw, NULL);
  orc_rule_register (rule_set, "convubw", mmx_rule_convubw, NULL);
  orc_rule_register (rule_set, "convssswb", mmx_rule_convssswb, NULL);
  orc_rule_register (rule_set, "convsuswb", mmx_rule_convsuswb, NULL);
  orc_rule_register (rule_set, "convwb", mmx_rule_convwb, NULL);

  orc_rule_register (rule_set, "convswl", mmx_rule_convswl, NULL);
  orc_rule_register (rule_set, "convuwl", mmx_rule_convuwl, NULL);
  orc_rule_register (rule_set, "convssslw", mmx_rule_convssslw, NULL);

  orc_rule_register (rule_set, "mulsbw", mmx_rule_mulsbw, NULL);
  orc_rule_register (rule_set, "mulubw", mmx_rule_mulubw, NULL);
  orc_rule_register (rule_set, "mulswl", mmx_rule_mulswl, NULL);

  orc_rule_register (rule_set, "accw", mmx_rule_accw, NULL);
  orc_rule_register (rule_set, "accl", mmx_rule_accl, NULL);
  orc_rule_register (rule_set, "accsadubl", mmx_rule_accsadubl, NULL);

#ifndef MMX
  orc_rule_register (rule_set, "addf", mmx_rule_addf, NULL);
  orc_rule_register (rule_set, "subf", mmx_rule_subf, NULL);
  orc_rule_register (rule_set, "mulf", mmx_rule_mulf, NULL);
  orc_rule_register (rule_set, "divf", mmx_rule_divf, NULL);
  orc_rule_register (rule_set, "minf", mmx_rule_minf, NULL);
  orc_rule_register (rule_set, "maxf", mmx_rule_maxf, NULL);
  orc_rule_register (rule_set, "sqrtf", mmx_rule_sqrtf, NULL);
  orc_rule_register (rule_set, "cmpeqf", mmx_rule_cmpeqf, NULL);
  orc_rule_register (rule_set, "cmpltf", mmx_rule_cmpltf, NULL);
  orc_rule_register (rule_set, "cmplef", mmx_rule_cmplef, NULL);
  orc_rule_register (rule_set, "convfl", mmx_rule_convfl, NULL);
  orc_rule_register (rule_set, "convlf", mmx_rule_convlf, NULL);
#endif

  /* slow rules */
  orc_rule_register (rule_set, "maxuw", mmx_rule_maxuw_slow, NULL);
  orc_rule_register (rule_set, "minuw", mmx_rule_minuw_slow, NULL);
  orc_rule_register (rule_set, "avgsb", mmx_rule_avgsb_slow, NULL);
  orc_rule_register (rule_set, "avgsw", mmx_rule_avgsw_slow, NULL);
  orc_rule_register (rule_set, "maxsb", mmx_rule_maxsb_slow, NULL);
  orc_rule_register (rule_set, "minsb", mmx_rule_minsb_slow, NULL);
  orc_rule_register (rule_set, "maxsl", mmx_rule_maxsl_slow, NULL);
  orc_rule_register (rule_set, "minsl", mmx_rule_minsl_slow, NULL);
  orc_rule_register (rule_set, "maxul", mmx_rule_maxul_slow, NULL);
  orc_rule_register (rule_set, "minul", mmx_rule_minul_slow, NULL);
  orc_rule_register (rule_set, "convlw", mmx_rule_convlw, NULL);
  orc_rule_register (rule_set, "signw", mmx_rule_signw_slow, NULL);
  orc_rule_register (rule_set, "absb", mmx_rule_absb_slow, NULL);
  orc_rule_register (rule_set, "absw", mmx_rule_absw_slow, NULL);
  orc_rule_register (rule_set, "absl", mmx_rule_absl_slow, NULL);
  orc_rule_register (rule_set, "swapw", mmx_rule_swapw, NULL);
  orc_rule_register (rule_set, "swapl", mmx_rule_swapl, NULL);
  orc_rule_register (rule_set, "splitlw", mmx_rule_splitlw, NULL);
  orc_rule_register (rule_set, "splitwb", mmx_rule_splitwb, NULL);
  orc_rule_register (rule_set, "avgsl", mmx_rule_avgsl, NULL);
  orc_rule_register (rule_set, "avgul", mmx_rule_avgul, NULL);

  /* SSE 3 -- no rules */

  /* SSSE 3 */
  rule_set = orc_rule_set_new (orc_opcode_set_get("sys"), target,
      ORC_TARGET_MMX_SSSE3);

  orc_rule_register (rule_set, "signb", mmx_rule_signX_ssse3, (void *)0);
  orc_rule_register (rule_set, "signw", mmx_rule_signX_ssse3, (void *)1);
  orc_rule_register (rule_set, "signl", mmx_rule_signX_ssse3, (void *)2);
  REG(absb);
  REG(absw);
  REG(absl);
#ifndef LOAD_MASK_IS_SLOW
  orc_rule_register (rule_set, "swapw", mmx_rule_swapw_ssse3, NULL);
  orc_rule_register (rule_set, "swapl", mmx_rule_swapl_ssse3, NULL);
  orc_rule_register (rule_set, "select0lw", mmx_rule_select0lw_ssse3, NULL);
  orc_rule_register (rule_set, "select1lw", mmx_rule_select1lw_ssse3, NULL);
  orc_rule_register (rule_set, "select0wb", mmx_rule_select0wb_ssse3, NULL);
  orc_rule_register (rule_set, "select1wb", mmx_rule_select1wb_ssse3, NULL);
#endif

  /* SSE 4.1 */
  rule_set = orc_rule_set_new (orc_opcode_set_get("sys"), target,
      ORC_TARGET_MMX_SSE4_1);

  REG(maxsb);
  REG(minsb);
  REG(maxuw);
  REG(minuw);
  REG(maxsl);
  REG(maxul);
  REG(minsl);
  REG(minul);
  REG(mulll);
  orc_rule_register (rule_set, "convsuslw", mmx_rule_convsuslw, NULL);
  orc_rule_register (rule_set, "muluwl", mmx_rule_muluwl, NULL);

  /* SSE 4.2 -- no rules */

  /* SSE 4a -- no rules */
}

