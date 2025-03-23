/*
rvjit_loong64.h - RVJIT LoongArch 64 Backend
Copyright (C) 2025 gns
                   LekKit <github.com/LekKit>
                   cerg2010cerg2010 <github.com/cerg2010cerg2010>

YOU WOULDN'T WRITE LARVA

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/


#include "rvjit.h"
#include "mem_ops.h"
#include "bit_ops.h"
#include "compiler.h"
#include "rvvm_types.h"
#include "utils.h"
#include <stddef.h>

#ifndef RVJIT_LOONG64_H
#define RVJIT_LOONG64_H

#define LOONG_REG_ZERO 0x0
#define LOONG_REG_RA   0x1
#define LOONG_REG_SP   0x3
#define LOONG_REG_A0   0x4
#define LOONG_REG_R21  0x15

#define LOONG_REG_GSCR LOONG_REG_R21

#ifdef RVJIT_ABI_SYSV
#define VM_PTR_REG LOONG_REG_A0
#endif

#define LOONGF_SIMM_OK(x, b)	((((x) + (1 << (b-1))) >> (b)) == 0)
#define LOONGF_UIMM_OK(x, b)	(((x) >> (b)) == 0)
#define checku11(i)		LOONGF_UIMM_OK(i, 11)
#define checki12(i)		LOONGF_SIMM_OK(i, 12)
#define checki13(i)		LOONGF_SIMM_OK(i, 13)
#define checki16(i)		LOONGF_SIMM_OK(i, 16)
#define checki18(broffset)		(LOONGF_SIMM_OK(broffset, 18) && (((broffset) & 3) == 0))
#define checki20(i)		LOONGF_SIMM_OK(i, 20)
#define checki21(i)		LOONGF_SIMM_OK(i, 21)
#define checki23(broffset)		(LOONGF_SIMM_OK(broffset, 23) && (((broffset) & 3) == 0))
#define checki26(i)		LOONGF_SIMM_OK(i, 26)
#define checki28(joffset)		(LOONGF_SIMM_OK(joffset, 28) && (((joffset) & 3) == 0))
#define checku32(i)		LOONGF_UIMM_OK(i, 32)

/*
** FYI GitHub:loongson-community/loongarch-opcodes
** 龙芯手册脑袋尖尖的，少看
*/

#define LOONGF_D(gpr) (((gpr) & 0x1f))
#define LOONGF_J(gpr) (((gpr) & 0x1f) << 5)
#define LOONGF_K(gpr) (((gpr) & 0x1f) << 10)
#define LOONGF_A(gpr) (((gpr) & 0x1f) << 15)
#define LOONGF_Fd(fpr) (((fpr) & 0x1f))
#define LOONGF_Fj(fpr) (((fpr) & 0x1f) << 5)
#define LOONGF_Fk(fpr) (((fpr) & 0x1f) << 10)
#define LOONGF_Fa(fpr) (((fpr) & 0x1f) << 15)

#define LOONGF_Sd5k16_MC(simm) (((simm & 0xffff) << 10) | ((simm & 0x1f0000) >> 16))
#define LOONGF_Sd5k16_OFFSET(offset) ((((offset) & 0x3fffc) << 8) | (((offset) & 0x7c0000) >> 18))
#define LOONGF_Sd10k16_MC(simm) ((((simm) & 0xffff) << 10) | (((simm) & 0x3ff0000) >> 16))
#define LOONGF_Sd10k16_OFFSET(offset) ((((offset) & 0x3fffc) << 8) | (((offset) & 0xffc0000) >> 18))
#define LOONGF_Sk12(simm) (((simm) & 0xfff) << 10)
#define LOONGF_Sk14(simm) (((simm) & 0x3fff) << 10)
#define LOONGF_Sk16_MC(simm) (((simm) & 0xffff) << 10)
#define LOONGF_Sk16_OFFSET(offset) (((offset) & 0x3fffc) << 8)
#define LOONGF_Sj20(simm) (((simm) & 0xfffff) << 10)
#define LOONGF_Ud5(uimm) (((uimm) & 0x1fU))
#define LOONGF_Uj5(uimm) (((uimm) & 0x1fU) << 5)
#define LOONGF_Uk5(uimm) (((uimm) & 0x1fU) << 10)
#define LOONGF_Uk6(uimm) (((uimm) & 0x3fU) << 10)
#define LOONGF_Uk12(uimm) (((uimm) & 0xfffU) << 10)
#define LOONGF_Uk14(uimm) (((uimm) & 0x3fffU) << 10)
#define LOONGF_Um5(uimm) (((uimm) & 0x1fU) << 16)
#define LOONGF_Um6(uimm) (((uimm) & 0x3fU) << 16)
#define LOONGF_Cd(fcc) (((fcc) & 7))
#define LOONGF_Cj(fcc) (((fcc) & 7) << 5)
#define LOONGF_Ua2(ua2) (((ua2) & 3) << 15)
#define LOONGF_Ua3(ua3) (((ua3) & 7) << 15)
#define LOONGF_Td(scr) (((scr) & 3))
#define LOONGF_Tj(scr) (((scr) & 3) << 5)

/*
** typedef enum LOONGIns {
    // Integer instructions.
    LOONGI_MV = 0x00150000,
    LOONGI_NOP = 0x03400000,
  
    LOONGI_AND = 0x00148000,
    LOONGI_ANDI = 0x03400000,
    LOONGI_OR = 0x00150000,
    LOONGI_ORI = 0x03800000,
    LOONGI_XOR = 0x00158000,
    LOONGI_XORI = 0x03c00000,
    LOONGI_NOR = 0x00140000,
  
    LOONGI_SLT = 0x00120000,
    LOONGI_SLTU = 0x00128000,
    LOONGI_SLTI = 0x02000000,
    LOONGI_SLTUI = 0x02400000,
  
    LOONGI_ADD_W = 0x00100000,
    LOONGI_ADDI_W = 0x02800000,
    LOONGI_SUB_W = 0x00110000,
    LOONGI_MUL_W = 0x001c0000,
    LOONGI_MULH_W = 0x001c8000,
    LOONGI_DIV_W = 0x00200000,
    LOONGI_DIV_WU = 0x00210000,
  
    LOONGI_SLLI_W = 0x00408000,
    LOONGI_SRLI_W = 0x00448000,
    LOONGI_SRAI_W = 0x00488000,
    LOONGI_ROTRI_W = 0x004c8000,
    LOONGI_ROTRI_D = 0x004d0000,
    LOONGI_SLL_W = 0x00170000,
    LOONGI_SRL_W = 0x00178000,
    LOONGI_SRA_W = 0x00180000,
    LOONGI_ROTR_W = 0x001b0000,
    LOONGI_ROTR_D = 0x001b8000,
  
    LOONGI_EXT_W_B = 0x00005c00,
    LOONGI_EXT_W_H = 0x00005800,
    LOONGI_REVB_2H = 0x00003000,
    LOONGI_REVB_4H = 0x00003400,
  
    LOONGI_ALSL_W = 0x00040000,
    LOONGI_ALSL_D = 0x002c0000,
  
    LOONGI_B = 0x50000000,
    LOONGI_BL = 0x54000000,
    LOONGI_JIRL = 0x4c000000,
  
    LOONGI_BEQ = 0x58000000,
    LOONGI_BNE = 0x5c000000,
    LOONGI_BLT = 0x60000000,
    LOONGI_BGE = 0x64000000,
    LOONGI_BGEU = 0x6c000000,
    LOONGI_BLTU = 0x68000000,
    LOONGI_BEQZ = 0x40000000,
    LOONGI_BNEZ = 0x44000000,
    LOONGI_BCEQZ = 0x48000000,
    LOONGI_BCNEZ = 0x48000100,
  
    // Load/store instructions.
    LOONGI_LD_W = 0x28800000,
    LOONGI_LD_D = 0x28c00000,
    LOONGI_ST_W = 0x29800000,
    LOONGI_ST_D = 0x29c00000,
    LOONGI_LD_B = 0x28000000,
    LOONGI_ST_B = 0x29000000,
    LOONGI_LD_H = 0x28400000,
    LOONGI_ST_H = 0x29400000,
    LOONGI_LD_BU = 0x2a000000,
    LOONGI_LD_HU = 0x2a400000,
    LOONGI_LDX_B = 0x38000000,
    LOONGI_LDX_BU = 0x38200000,
    LOONGI_LDX_H = 0x38040000,
    LOONGI_LDX_HU = 0x38240000,
    LOONGI_LDX_D = 0x380c0000,
    LOONGI_STX_D = 0x381c0000,
    LOONGI_LDX_W = 0x38080000,
    LOONGI_STX_W = 0x38180000,
    LOONGI_STX_B = 0x38100000,
    LOONGI_STX_H = 0x38140000,
    LOONGI_FLD_S = 0x2b000000,
    LOONGI_FST_S = 0x2b400000,
    LOONGI_FLD_D = 0x2b800000,
    LOONGI_FST_D = 0x2bc00000,
    LOONGI_FLDX_D = 0x38340000,
    LOONGI_FLDX_S = 0x38300000,
    LOONGI_FSTX_D = 0x383c0000,
    LOONGI_FSTX_S = 0x38380000,
  
    LOONGI_ADD_D = 0x00108000,
    LOONGI_ADDI_D = 0x02c00000,
    LOONGI_ADDU16I_D = 0x10000000,
    LOONGI_LU12I_W = 0x14000000,
    LOONGI_LU32I_D = 0x16000000,
    LOONGI_LU52I_D = 0x3000000,
    LOONGI_SUB_D = 0x00118000,
    LOONGI_DIV_D = 0x00220000,
    LOONGI_DIV_DU = 0x00230000,
    LOONGI_MUL_D = 0x001d8000,
  
    LOONGI_SLLI_D = 0x00410000,
    LOONGI_SRLI_D = 0x00450000,
    LOONGI_SLL_D = 0x00188000,
    LOONGI_SRL_D = 0x00190000,
    LOONGI_SRAI_D = 0x00490000,
    LOONGI_SRA_D = 0x00198000,
    LOONGI_REVH_D = 0x00004400,
  
    // Extract/insert instructions.
    LOONGI_BSTRPICK_D = 0x00c00000,
    LOONGI_BSTRINS_D = 0x00800000,
  
    LOONGI_MASKEQZ = 0x00130000,
    LOONGI_MASKNEZ = 0x00138000,
  
    // FP instructions.
    LOONGI_FRINT_S = 0x011e4400,
    LOONGI_FRINT_D = 0x011e4800,
    LOONGI_FTINTRM_L_D = 0x011a2800,
    LOONGI_FTINTRP_L_D = 0x011a6800,
    LOONGI_FTINTRNE_L_D = 0x011ae800,
  
    LOONGI_FMOV_S = 0x01149400,
    LOONGI_FMOV_D = 0x01149800,
  
    LOONGI_FABS_D = 0x01140800,
    LOONGI_FNEG_D = 0x01141800,
  
    LOONGI_FADD_D = 0x01010000,
    LOONGI_FSUB_D = 0x01030000,
    LOONGI_FMUL_D = 0x01050000,
    LOONGI_FDIV_D = 0x01070000,
    LOONGI_FSQRT_D = 0x01144800,
  
    LOONGI_FMIN_D = 0x010b0000,
    LOONGI_FMAX_D = 0x01090000,
  
    LOONGI_FADD_S = 0x01008000,
    LOONGI_FSUB_S = 0x01028000,
  
    LOONGI_FMADD_S = 0x08100000,
    LOONGI_FMADD_D = 0x08200000,
    LOONGI_FNMADD_D = 0x08a00000,
    LOONGI_FMSUB_S = 0x08500000,
    LOONGI_FMSUB_D = 0x08600000,
    LOONGI_FNMSUB_D = 0x08e00000,
  
    LOONGI_FCVT_D_S = 0x01192400,
    LOONGI_FTINT_W_S = 0x011b0400,
    LOONGI_FCVT_S_D = 0x01191800,
    LOONGI_FTINT_W_D = 0x011b0800,
    LOONGI_FFINT_S_W = 0x011d1000,
    LOONGI_FFINT_D_W = 0x011d2000,
    LOONGI_FFINT_S_L = 0x011d1800,
    LOONGI_FFINT_D_L = 0x011d2800,
  
    LOONGI_FTINTRZ_W_S = 0x011a8400,
    LOONGI_FTINTRZ_W_D = 0x011a8800,
    LOONGI_FTINTRZ_L_S = 0x011aa400,
    LOONGI_FTINTRZ_L_D = 0x011aa800,
    LOONGI_FTINTRM_W_S = 0x011a0400,
    LOONGI_FTINTRM_W_D = 0x011a0800,
  
    LOONGI_MOVFR2GR_S = 0x0114b400,
    LOONGI_MOVGR2FR_W = 0x0114a400,
    LOONGI_MOVGR2FR_D = 0x0114a800,
    LOONGI_MOVFR2GR_D = 0x0114b800,
  
    LOONGI_FCMP_CEQ_D = 0x0c220000,
    LOONGI_FCMP_CLT_S = 0x0c110000,
    LOONGI_FCMP_CLT_D = 0x0c210000,
    LOONGI_FCMP_CLE_D = 0x0c230000,
    LOONGI_FCMP_CULE_D = 0x0c270000,
    LOONGI_FCMP_CULT_D = 0x0c250000,
    LOONGI_FCMP_CNE_D = 0x0c280000,
    LOONGI_FSEL = 0x0d000000,
** } LOONGIns;
** Copied from Loongson LuaJIT, FYI
*/

/* */

static inline size_t rvjit_native_default_hregmask(void)
{
    // a0-a7, t0-t8 are caller-saved
    // a0 is preserved as vmptr
    return rvjit_hreg_mask(5)  |
           rvjit_hreg_mask(6)  |
           rvjit_hreg_mask(7)  |
           rvjit_hreg_mask(8)  |
           rvjit_hreg_mask(9)  |
           rvjit_hreg_mask(10) |
           rvjit_hreg_mask(11) |
           rvjit_hreg_mask(12) |
           rvjit_hreg_mask(13) |
           rvjit_hreg_mask(14) |
           rvjit_hreg_mask(15) |
           rvjit_hreg_mask(16) |
           rvjit_hreg_mask(17) |
           rvjit_hreg_mask(18) |
           rvjit_hreg_mask(19) |
           rvjit_hreg_mask(20) ;
}

/* EMPTY, Pseudo */

typedef enum LoongInsEMPTY {
    LOONGI_NOP = 0x03400000,
    LOONGI_RET = 0x4c000020,
} LoongInsEMPTY;

static inline void rvjit_loong_EMPTY_op(rvjit_block_t* block, LoongInsEMPTY insn)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn);
    rvjit_put_code(block, code, 4);
}

typedef enum LoongInsJPseudo {
    LOONGI_JR = 0x4c000000,
} LoongInsJPseudo;

static inline void rvjit_loong_J_Pseudo_op(rvjit_block_t* block, LoongInsJPseudo insn, regid_t rj)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_J(rj));
    rvjit_put_code(block, code, 4);
}

/* 2R -> [DJ, FdFj, FdJ, DFj, DCj, JUd5, DUj5, CdFj, FdCj, CdJ] */

typedef enum LoongInsDJ {
    LOONGI_CLO_W     = 0x00001000,
    LOONGI_CLZ_W     = 0x00001400,
    LOONGI_CTO_W     = 0x00001800,
    LOONGI_CTZ_W     = 0x00001c00,
    LOONGI_CLO_D     = 0x00002000,
    LOONGI_CLZ_D     = 0x00002400,
    LOONGI_CTO_D     = 0x00002800,
    LOONGI_CTZ_D     = 0x00002c00,
    LOONGI_REVB_2H   = 0x00003000,
    LOONGI_REVB_4H   = 0x00003400,
    LOONGI_REVB_2W   = 0x00003800,
    LOONGI_REVB_D    = 0x00003c00,
    LOONGI_REVH_2W   = 0x00004000,
    LOONGI_REVH_D    = 0x00004400,
    LOONGI_BITREV_4B = 0x00004800,
    LOONGI_BITREV_8B = 0x00004c00,
    LOONGI_BITREV_W  = 0x00005000,
    LOONGI_BITREV_D  = 0x00005400,
    // LOONGI_EXT_W_H   = 0x00005800,
    // LOONGI_EXT_W_B   = 0x00005c00,
    LOONGI_SEXT_H    = 0x00005800,
    LOONGI_SEXT_B    = 0x00005c00,
    /*
    LOONGI_RDTIMEL_W = 0x00006000,
    LOONGI_RDTIMEH_W = 0x00006400,
    LOONGI_RDTIME_D  = 0x00006800,
    LOONGI_CPUCFG_W  = 0x00006c00,
    */

    LOONGI_LLACQ_W = 0x38578000,
    LOONGI_SCREL_W = 0x38578400,
    LOONGI_LLACQ_D = 0x38578800,
    LOONGI_SCREL_D = 0x38578c00,

    /* Pseudo */
    LOONGI_MOVE = 0x00150000,   // addi.d rd, rj, zero
    LOONGI_SEXT_W = 0x02800000, // addi.w rd, rj, zero
    LOONGI_ZEXT_B = 0x0343fc00, // andi rd, rj, 0xff
    LOONGI_ZEXT_H = 0x00cf0000, // bstrpick.d rd, rj, 15, 0
    LOONGI_ZEXT_W = 0x00df0000, // bstrpick.d rd, rj, 31, 0
} LoongInsDJ;

static inline void rvjit_loong_DJ_op(rvjit_block_t* block, LoongInsDJ insn, regid_t rd, regid_t rj)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_J(rj) | LOONGF_D(rd));
    rvjit_put_code(block, code, 4);
}

typedef enum LoongInsFdFj {
    LOONGI_FABS_S    = 0x01140400,
    LOONGI_FABS_D    = 0x01140800,
    LOONGI_FNEG_S    = 0x01141400,
    LOONGI_FNEG_D    = 0x01141800,
    LOONGI_FLOGB_S   = 0x01142400,
    LOONGI_FLOGB_D   = 0x01142800,
    LOONGI_FCLASS_S  = 0x01143400,
    LOONGI_FCLASS_D  = 0x01143800,
    LOONGI_FSQRT_S   = 0x01144400,
    LOONGI_FSQRT_D   = 0x01144800,
    LOONGI_FRECIP_S  = 0x01145400,
    LOONGI_FRECIP_D  = 0x01145800,
    LOONGI_FRSQRT_S  = 0x01146400,
    LOONGI_FRSQRT_D  = 0x01146800,
    LOONGI_FRECIPE_S = 0x01147400,
    LOONGI_FRECIPE_D = 0x01147800,
    LOONGI_FRSQRTE_S = 0x01148400,
    LOONGI_FRSQRTE_D = 0x01148800,
    LOONGI_FMOV_S    = 0x01149400,
    LOONGI_FMOV_D    = 0x01149800,

    LOONGI_FCVT_S_D     = 0x01191800,
    LOONGI_FCVT_D_S     = 0x01192400,
    LOONGI_FTINTRM_W_S  = 0x011a0400,
    LOONGI_FTINTRM_W_D  = 0x011a0800,
    LOONGI_FTINTRM_L_S  = 0x011a2400,
    LOONGI_FTINTRM_L_D  = 0x011a2800,
    LOONGI_FTINTRP_W_S  = 0x011a4400,
    LOONGI_FTINTRP_W_D  = 0x011a4800,
    LOONGI_FTINTRP_L_S  = 0x011a6400,
    LOONGI_FTINTRP_L_D  = 0x011a6800,
    LOONGI_FTINTRZ_W_S  = 0x011a8400,
    LOONGI_FTINTRZ_W_D  = 0x011a8800,
    LOONGI_FTINTRZ_L_S  = 0x011aa400,
    LOONGI_FTINTRZ_L_D  = 0x011aa800,
    LOONGI_FTINTRNE_W_S = 0x011ac400,
    LOONGI_FTINTRNE_W_D = 0x011ac800,
    LOONGI_FTINTRNE_L_S = 0x011ae400,
    LOONGI_FTINTRNE_L_D = 0x011ae800,
    LOONGI_FTINT_W_S    = 0x011b0400,
    LOONGI_FTINT_W_D    = 0x011b0800,
    LOONGI_FTINT_L_S    = 0x011b2400,
    LOONGI_FTINT_L_D    = 0x011b2800,
    LOONGI_FFINT_S_W    = 0x011d1000,
    LOONGI_FFINT_S_L    = 0x011d1800,
    LOONGI_FFINT_D_W    = 0x011d2000,
    LOONGI_FFINT_D_L    = 0x011d2800,
    LOONGI_FRINT_S      = 0x011e4400,
    LOONGI_FRINT_D      = 0x011e4800,
} LoongInsFdFj;

static inline void rvjit_loong_FdFj_op(rvjit_block_t* block, LoongInsFdFj insn, regid_t fd, regid_t fj)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_Fj(fj) | LOONGF_Fd(fd));
    rvjit_put_code(block, code, 4);
}

typedef enum LoongInsFdJ {
    LOONGI_MOVGR2FR_W  = 0x0114a400,
    LOONGI_MOVGR2FR_D  = 0x0114a800,
    LOONGI_MOVGR2FRH_W = 0x0114ac00,
} LoongInsFdJ;

static inline void rvjit_loong_FdJ_op(rvjit_block_t* block, LoongInsFdJ insn, regid_t fd, regid_t rj)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_J(rj) | LOONGF_Fd(fd));
    rvjit_put_code(block, code, 4);
}

typedef enum LoongInsDFj {
    LOONGI_MOVFR2GR_S  = 0x0114b400,
    LOONGI_MOVFR2GR_D  = 0x0114b800,
    LOONGI_MOVFRH2GR_S = 0x0114bc00,
} LoongInsDFj;

static inline void rvjit_loong_DFj_op(rvjit_block_t* block, LoongInsDFj insn, regid_t rd, regid_t fj)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_Fj(fj) | LOONGF_D(rd));
    rvjit_put_code(block, code, 4);
}

typedef enum LoongInsDCj {
    // LOONGI_MOVCF2GR = 0x0114dc00,
    LOONGI_MOVFCCR2GR = 0x0114dc00,
} LoongInsDCj;

static inline void rvjit_loong_DCj_op(rvjit_block_t* block, LoongInsDCj insn, regid_t rd, regid_t fcc)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_Cj(fcc) | LOONGF_D(rd));
    rvjit_put_code(block, code, 4);
}

typedef enum LoongInsJUd5 {
    // LOONGI_MOVGR2FCSR = 0x0114c000,
    LOONGI_FCSRWR = 0x0114c000,
} LoongInsJUd5;

static inline void rvjit_loong_JUd5_op(rvjit_block_t* block, LoongInsJUd5 insn, regid_t fcsr, regid_t rj)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_J(rj) | LOONGF_Ud5(fcsr));
    rvjit_put_code(block, code, 4);
}

typedef enum LoongInsDUj5 {
    // LOONGI_MOVFCSR2GR = 0x0114c800,
    LOONGI_FCSRRD = 0x0114c800,
} LoongInsDUj5;

static inline void rvjit_loong_DUj5_op(rvjit_block_t* block, LoongInsDUj5 insn, regid_t rd, regid_t fcsr)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_Uj5(fcsr) | LOONGF_D(rd));
    rvjit_put_code(block, code, 4);
}

typedef enum LoongInsCdFj {
    // LOONGI_MOVFR2CF = 0x0114d000,
    LOONGI_MOVFR2FCC = 0x0114d000,
} LoongInsCdFj;

static inline void rvjit_loong_CdFj_op(rvjit_block_t* block, LoongInsCdFj insn, regid_t fcc, regid_t fj)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_Fj(fj) | LOONGF_Cd(fcc));
    rvjit_put_code(block, code, 4);
}

typedef enum LoongInsFdCj {
    // LOONGI_MOVCF2FR = 0x0114d400,
    LOONGI_MOVFCC2FR = 0x0114d400,
} LoongInsFdCj;

static inline void rvjit_loong_FdCj_op(rvjit_block_t* block, LoongInsFdCj insn, regid_t fd, regid_t fcc)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_Cj(fcc) | LOONGF_Fd(fd));
    rvjit_put_code(block, code, 4);
}

typedef enum LoongInsCdJ {
    // LOONGI_MOVGR2CF = 0x0114d800,
    LOONGI_MOVGR2FCC = 0x0114d800,
} LoongInsCdJ;

static inline void rvjit_loong_CdJ_op(rvjit_block_t* block, LoongInsCdJ insn, regid_t fcc, regid_t rj)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_J(rj) | LOONGF_Cd(fcc));
    rvjit_put_code(block, code, 4);
}

/* 3R -> [DJK, FdJK, FdFjFk, DJKUa2, DJKUa3] */

typedef enum LoongInsDJK {
    LOONGI_ADD_W     = 0x00100000,
    LOONGI_ADD_D     = 0x00108000,
    LOONGI_SUB_W     = 0x00110000,
    LOONGI_SUB_D     = 0x00118000,
    LOONGI_SLT       = 0x00120000,
    LOONGI_SLTU      = 0x00128000,
    LOONGI_MASKEQZ   = 0x00130000,
    LOONGI_MASKNEZ   = 0x00138000,
    LOONGI_NOR       = 0x00140000,
    LOONGI_AND       = 0x00148000,
    LOONGI_OR        = 0x00150000,
    LOONGI_XOR       = 0x00158000,
    LOONGI_ORN       = 0x00160000,
    LOONGI_ANDN      = 0x00168000,
    LOONGI_SLL_W     = 0x00170000,
    LOONGI_SRL_W     = 0x00178000,
    LOONGI_SRA_W     = 0x00180000,
    LOONGI_SLL_D     = 0x00188000,
    LOONGI_SRL_D     = 0x00190000,
    LOONGI_SRA_D     = 0x00198000,
    LOONGI_ROTR_W    = 0x001b0000,
    LOONGI_ROTR_D    = 0x001b8000,
    LOONGI_MUL_W     = 0x001c0000,
    LOONGI_MULH_W    = 0x001c8000,
    LOONGI_MULH_WU   = 0x001d0000,
    LOONGI_MUL_D     = 0x001d8000,
    LOONGI_MULH_D    = 0x001e0000,
    LOONGI_MULH_DU   = 0x001e8000,
    LOONGI_MULW_D_W  = 0x001f0000,
    LOONGI_MULW_D_WU = 0x001f8000,
    LOONGI_DIV_W     = 0x00200000,
    LOONGI_MOD_W     = 0x00208000,
    LOONGI_DIV_WU    = 0x00210000,
    LOONGI_MOD_WU    = 0x00218000,
    LOONGI_DIV_D     = 0x00220000,
    LOONGI_MOD_D     = 0x00228000,
    LOONGI_DIV_DU    = 0x00230000,
    LOONGI_MOD_DU    = 0x00238000,
    /*
    LOONGI_CRC_W_B_W  = 0x00240000,
    LOONGI_CRC_W_H_W  = 0x00248000,
    LOONGI_CRC_W_W_W  = 0x00250000,
    LOONGI_CRC_W_D_W  = 0x00258000,
    LOONGI_CRCC_W_B_W = 0x00260000,
    LOONGI_CRCC_W_H_W = 0x00268000,
    LOONGI_CRCC_W_W_W = 0x00270000,
    LOONGI_CRCC_W_D_W = 0x00278000,
    */

    LOONGI_LDX_B   = 0x38000000,
    LOONGI_LDX_H   = 0x38040000,
    LOONGI_LDX_W   = 0x38080000,
    LOONGI_LDX_D   = 0x380c0000,
    LOONGI_STX_B   = 0x38100000,
    LOONGI_STX_H   = 0x38140000,
    LOONGI_STX_W   = 0x38180000,
    LOONGI_STX_D   = 0x381c0000,
    LOONGI_LDX_BU  = 0x38200000,
    LOONGI_LDX_HU  = 0x38240000,
    LOONGI_LDX_WU  = 0x38280000,
    // LOONGI_PRELDX = 0x382c0000,

    LOONGI_SC_Q    = 0x38570000,
} LoongInsDJK;

static inline void rvjit_loong_DJK_op(rvjit_block_t* block, LoongInsDJK insn, regid_t rd, regid_t rj, regid_t rk)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_K(rk) | LOONGF_J(rj) | LOONGF_D(rd));
    rvjit_put_code(block, code, 4);
}

typedef enum LoongInsFdJK {
    LOONGI_FLDX_S  = 0x38300000,
    LOONGI_FLDX_D  = 0x38340000,
    LOONGI_FSTX_S  = 0x38380000,
    LOONGI_FSTX_D  = 0x383c0000,
} LoongInsFdJK;

static inline void rvjit_loong_FdJK_op(rvjit_block_t* block, LoongInsFdJK insn, regid_t fd, regid_t rj, regid_t rk)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_K(rk) | LOONGF_J(rj) | LOONGF_D(fd));
    rvjit_put_code(block, code, 4);
}

typedef enum LoongInsFdFjFk {
    LOONGI_FADD_S      = 0x01008000,
    LOONGI_FADD_D      = 0x01010000,
    LOONGI_FSUB_S      = 0x01028000,
    LOONGI_FSUB_D      = 0x01030000,
    LOONGI_FMUL_S      = 0x01048000,
    LOONGI_FMUL_D      = 0x01050000,
    LOONGI_FDIV_S      = 0x01068000,
    LOONGI_FDIV_D      = 0x01070000,
    LOONGI_FMAX_S      = 0x01088000,
    LOONGI_FMAX_D      = 0x01090000,
    LOONGI_FMIN_S      = 0x010a8000,
    LOONGI_FMIN_D      = 0x010b0000,
    LOONGI_FMAXA_S     = 0x010c8000,
    LOONGI_FMAXA_D     = 0x010d0000,
    LOONGI_FMINA_S     = 0x010e8000,
    LOONGI_FMINA_D     = 0x010f0000,
    LOONGI_FSCALEB_S   = 0x01108000,
    LOONGI_FSCALEB_D   = 0x01110000,
    LOONGI_FCOPYSIGN_S = 0x01128000,
    LOONGI_FCOPYSIGN_D = 0x01130000,
} LoongInsFdFjFk;

static inline void rvjit_loong_FdFjFk_op(rvjit_block_t* block, LoongInsFdFjFk insn, regid_t rd, regid_t rj, regid_t rk)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_Fk(rk) | LOONGF_Fj(rj) | LOONGF_Fd(rd));
    rvjit_put_code(block, code, 4);
}

typedef enum LoongInsDJKUa2 {
    // LOONGI_ALSL_W  = 0x00040000,
    // LOONGI_ALSL_WU = 0x00060000,
    LOONGI_SLADD_W    = 0x00040000,
    LOONGI_SLADD_WU   = 0x00060000,
    LOONGI_BYTEPICK_W = 0x00080000,
    // LOONGI_ALSL_D  = 0x002c0000,
    LOONGI_SLADD_D    = 0x002c0000,
} LoongInsDJKUa2;

static inline void rvjit_loong_DJKUa2_op(rvjit_block_t* block, LoongInsDJKUa2 insn, regid_t rd, regid_t rj, regid_t rk, uint8_t sa2)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_Ua2(sa2) | LOONGF_K(rk) | LOONGF_J(rj) | LOONGF_D(rd));
    rvjit_put_code(block, code, 4);
}

typedef enum LoongInsDJKUa3 {
    LOONGI_BYTEPICK_D = 0x000c0000,
} LoongInsDJKUa3;

static inline void rvjit_loong_DJKUa3_op(rvjit_block_t* block, LoongInsDJKUa3 insn, regid_t rd, regid_t rj, regid_t rk, uint8_t sa3)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_Ua3(sa3) | LOONGF_K(rk) | LOONGF_J(rj) | LOONGF_D(rd));
    rvjit_put_code(block, code, 4);
}

typedef enum LoongInsCdFjFk {
    LOONGI_FCMP_COND_S = 0x0c100000,
    LOONGI_FCMP_COND_D = 0x0c200000,
} LoongInsCdFjFk;

/* 4R -> [FdFjFkFa, FdFjFkCa] */

typedef enum LoongInsFdFjFkFa {
    LOONGI_FMADD_S  = 0x08100000,
    LOONGI_FMADD_D  = 0x08200000,
    LOONGI_FMSUB_S  = 0x08500000,
    LOONGI_FMSUB_D  = 0x08600000,
    LOONGI_FNMADD_S = 0x08900000,
    LOONGI_FNMADD_D = 0x08a00000,
    LOONGI_FNMSUB_S = 0x08d00000,
    LOONGI_FNMSUB_D = 0x08e00000,
} LoongInsFdFjFkFa;

static inline void rvjit_loong_FdFjFkFa_op(rvjit_block_t* block, LoongInsFdFjFkFa insn, regid_t rd, regid_t rj, regid_t rk, regid_t ra)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_A(ra) | LOONGF_K(rk) | LOONGF_J(rj) | LOONGF_D(rd) );
    rvjit_put_code(block, code, 4);
}

typedef enum LoongInsFdFjFkCa {
    LOONGI_FSEL = 0x0d000000,
} LoongInsFdFjFkCa;

/* 2Ri8 -> [DJUk8, JUk8] */

// typedef enum LoongInsDJUk8 {
//     LOONGI_LDDIR = 0x6400000;
// } LoongInsDJUk8;

// typedef enum LoongInsJUk8 {
//     LOONGI_LDPTE = 0x06440000;
// }

/* [DJUk5, DJUk6] */

typedef enum LoongInsDJUk5 {
    LOONGI_SLLI_W  = 0x00408000,
    LOONGI_SRLI_W  = 0x00448000,
    LOONGI_SRAI_W  = 0x00488000,
    LOONGI_ROTRI_W = 0x004c8000,

    /* LBT */
    LOONGI_RCRI_W = 0x00508000,
} LoongInsDJUk5;

typedef enum LoongInsDJUk6 {
    LOONGI_SLLI_D  = 0x00410000,
    LOONGI_SRLI_D  = 0x00450000,
    LOONGI_SRAI_D  = 0x00490000,
    LOONGI_ROTRI_D = 0x004d0000,

    /* LBT */
    LOONGI_RCRI_D = 0x00510000,
} LoongInsDJUk6;

static inline void rvjit_loong_DJUk5_op(rvjit_block_t* block, LoongInsDJUk5 insn, regid_t rd, regid_t rj, uint8_t imm)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_Uk5(imm) | LOONGF_J(rj) | LOONGF_D(rd));
    rvjit_put_code(block, code, 4);
}

static inline void rvjit_loong_DJUk6_op(rvjit_block_t* block, LoongInsDJUk6 insn, regid_t rd, regid_t rj, uint8_t imm)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_Uk6(imm) | LOONGF_J(rj) | LOONGF_D(rd));
    rvjit_put_code(block, code, 4);
}

/* 2Ri12 -> [DJSk12, DJUk12, FdJSk12, DJUk5Um5, DJUk6Um6] */

typedef enum LoongInsDJSk12 {
    LOONGI_SLTI    = 0x02000000,
    LOONGI_SLTUI   = 0x02400000,
    LOONGI_ADDI_W  = 0x02800000,
    LOONGI_ADDI_D  = 0x02c00000,
    LOONGI_LU52I_D = 0x03000000,
    
    // LOONGI_CACOP = 0x06000000,

    LOONGI_LD_B  = 0x28000000,
    LOONGI_LD_H  = 0x28400000,
    LOONGI_LD_W  = 0x28800000,
    LOONGI_LD_D  = 0x28c00000,
    LOONGI_ST_B  = 0x29000000,
    LOONGI_ST_H  = 0x29400000,
    LOONGI_ST_W  = 0x29800000,
    LOONGI_ST_D  = 0x29c00000,
    LOONGI_LD_BU = 0x2a000000,
    LOONGI_LD_HU = 0x2a400000,
    LOONGI_LD_WU = 0x2a800000,

    /* LBT */
    LOONGI_LDL_W = 0x2e000000,
    LOONGI_LDR_W = 0x2e400000,
    LOONGI_LDL_D = 0x2e800000,
    LOONGI_LDR_D = 0x2ec00000,
    LOONGI_STL_W = 0x2f000000,
    LOONGI_STR_W = 0x2f400000,
    LOONGI_STL_D = 0x2f800000,
    LOONGI_STR_D = 0x2fc00000,
} LoongInsDJSk12;

typedef enum LoongInsDJUk12 {
    LOONGI_ANDI    = 0x03400000,
    LOONGI_ORI     = 0x03800000,
    LOONGI_XORI    = 0x03c00000,
} LoongInsDJUk12;

typedef enum LoongInsFdJSk12 {
    LOONGI_FLD_S  = 0x2b000000,
    LOONGI_FST_S  = 0x2b400000,
    LOONGI_FLD_D  = 0x2b800000,
    LOONGI_FST_D  = 0x2bc00000,
} LoongInsFdJSk12;

typedef enum LoongInsDJUk5Um5 {
    LOONGI_BSTRINS_W  = 0x00600000,
    LOONGI_BSTRPICK_W = 0x00608000,
} LoongInsDJUk5Um5;

typedef enum LoongInsDJUk6Um6 {
    LOONGI_BSTRINS_D  = 0x00800000,
    LOONGI_BSTRPICK_D = 0x00c00000,
} LoongInsDJUk6Um6;

static inline void rvjit_loong_DJSk12_op(rvjit_block_t* block, LoongInsDJSk12 insn, regid_t rd, regid_t rj, int16_t imm)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_Sk12(imm) | LOONGF_J(rj) | LOONGF_D(rd));
    rvjit_put_code(block, code, 4);
}

static inline void rvjit_loong_DJUk12_op(rvjit_block_t* block, LoongInsDJUk12 insn, regid_t rd, regid_t rj, uint16_t imm)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_Uk12(imm) | LOONGF_J(rj) | LOONGF_D(rd));
    rvjit_put_code(block, code, 4);
}

static inline void rvjit_loong_FdJSk12_op(rvjit_block_t* block, LoongInsFdJSk12 insn, regid_t fd, regid_t rj, int16_t imm)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_Sk12(imm) | LOONGF_J(rj) | LOONGF_Fd(fd));
    rvjit_put_code(block, code, 4);
}

static inline void rvjit_loong_DJUk5Um5_op(rvjit_block_t* block, LoongInsDJUk5Um5 insn, regid_t rd, regid_t rj, uint8_t kimm5, uint8_t mimm5)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_Um5(mimm5) | LOONGF_Uk5(kimm5) | LOONGF_J(rj) | LOONGF_D(rd));
    rvjit_put_code(block, code, 4);
}

static inline void rvjit_loong_DJUk6Um6_op(rvjit_block_t* block, LoongInsDJUk6Um6 insn, regid_t rd, regid_t rj, uint8_t kimm6, uint8_t mimm6)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_Um6(mimm6) | LOONGF_Uk6(kimm6) | LOONGF_J(rj) | LOONGF_D(rd));
    rvjit_put_code(block, code, 4);
}

/* 2Ri14 -> [DJSk14, DJUk14] */

typedef enum LoongInsDJSk14 {
    LOONGI_LL_W    = 0x20000000,
    LOONGI_SC_W    = 0x21000000,
    LOONGI_LL_D    = 0x22000000,
    LOONGI_SC_D    = 0x23000000,

    LOONGI_LDPTR_W = 0x24000000,
    LOONGI_STPTR_W = 0x25000000,
    LOONGI_LDPTR_D = 0x26000000,
    LOONGI_STPTR_D = 0x27000000,
    // LOONGI_LDOX4_W = 0x24000000,
    // LOONGI_STOX4_W = 0x25000000,
    // LOONGI_LDOX4_D = 0x26000000,
    // LOONGI_STOX4_D = 0x27000000,
} LoongInsDJSk14;

// typedef enum LoongInsDJUk14 {
//     LOONGI_CSRRD   = 0x04000000,
//     LOONGI_CSRWR   = 0x04000020,
//     LOONGI_CSRXCHG = 0x040007c0,
// } LoongInsDJUk14;

static inline void rvjit_loong_DJSk14_op(rvjit_block_t* block, LoongInsDJSk14 insn, regid_t rd, regid_t rj, int16_t imm)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_Sk14(imm) | LOONGF_J(rj) | LOONGF_D(rd));
    rvjit_put_code(block, code, 4);
}

/* 2Ri16 -> [DJSk16] */

typedef enum LoongInsDJSk16 {
    LOONGI_ADDU16I_D = 0x10000000,
    
    LOONGI_JIRL = 0x4c000000,

    LOONGI_BEQ  = 0x58000000,
    LOONGI_BNE  = 0x5c000000,
    LOONGI_BLT  = 0x60000000,
    LOONGI_BGE  = 0x64000000,
    LOONGI_BLTU = 0x68000000,
    LOONGI_BGEU = 0x6c000000,
} LoongInsDJSk16;

static inline void rvjit_loong_DJSk16_op_mc(rvjit_block_t* block, LoongInsDJSk16 insn, regid_t rd, regid_t rj, int16_t simm)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_Sk16_MC(simm) | LOONGF_J(rj) | LOONGF_D(rd));
    rvjit_put_code(block, code, 4);
}

static inline void rvjit_loong_DJSk16_op_offset(rvjit_block_t* block, LoongInsDJSk16 insn, regid_t rd, regid_t rj, int32_t offset)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_Sk16_OFFSET(offset) | LOONGF_J(rj) | LOONGF_D(rd));
    rvjit_put_code(block, code, 4);
}

/* 1Ri21 -> [DSj20, JSd5k16, CjSd5k16] */

typedef enum LoongInsDSj20 {
    LOONGI_LU12I_W   = 0x14000000,
    // LOONGI_LU32I_D = 0x16000000,
    LOONGI_CU32I_D   = 0x16000000,
    LOONGI_PCADDU2I  = 0x18000000,
    LOONGI_PCALAU12I = 0x1a000000,
    LOONGI_PCADDU12I = 0x1c000000,
    LOONGI_PCADDU18I = 0x1e000000,
} LoongInsDSj20;

typedef enum LoongInsJSd5k16 {
    LOONGI_BEQZ = 0x40000000,
    LOONGI_BNEZ = 0x44000000,
} LoongInsJSd5k16;

typedef enum LoongInsCjSd5k16 {
    LOONGI_BCEQZ = 0x48000000,
    LOONGI_BCNEZ = 0x48000100,
} LoongInsCjSd5k16;

static inline void rvjit_loong_DSj20_op(rvjit_block_t* block, LoongInsDSj20 insn, regid_t rd, int32_t imm)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_Sj20(imm) | LOONGF_D(rd));
    rvjit_put_code(block, code, 4);
}

static inline void rvjit_loong_JSd5k16_op_mc(rvjit_block_t* block, LoongInsJSd5k16 insn, regid_t rj, ptrdiff_t simm)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_Sd5k16_MC(simm) | LOONGF_J(rj));
    rvjit_put_code(block, code, 4);
}

static inline void rvjit_loong_CjSd5k16_op_mc(rvjit_block_t* block, LoongInsCjSd5k16 insn, regid_t fcc, ptrdiff_t simm)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_Sd5k16_MC(simm) | LOONGF_Cj(fcc));
    rvjit_put_code(block, code, 4);
}

static inline void rvjit_loong_JSd5k16_op_offset(rvjit_block_t* block, LoongInsJSd5k16 insn, regid_t rj, ptrdiff_t offset)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_Sd5k16_OFFSET(offset) | LOONGF_J(rj));
    rvjit_put_code(block, code, 4);
}

static inline void rvjit_loong_CjSd5k16_op_offset(rvjit_block_t* block, LoongInsCjSd5k16 insn, regid_t fcc, ptrdiff_t offset)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_Sd5k16_OFFSET(offset) | LOONGF_Cj(fcc));
    rvjit_put_code(block, code, 4);
}

/* i26 -> [Sd10k16] */

typedef enum LoongInsSd10k16 {
    LOONGI_B  = 0x50000000,
    LOONGI_BL = 0x54000000,
} LoongInsSd10k16;

static inline void rvjit_loong_Sd10k16_op_mc(rvjit_block_t* block, LoongInsSd10k16 insn, ptrdiff_t simm)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_Sd10k16_MC(simm));
    rvjit_put_code(block, code, 4);
}

static inline void rvjit_loong_Sd10k16_op_offset(rvjit_block_t* block, LoongInsSd10k16 insn, ptrdiff_t offset)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_Sd10k16_OFFSET(offset));
    rvjit_put_code(block, code, 4);
}

/* LBT */

typedef enum LoongRegLbt {
    LOONG_REG_SCR0,
    LOONG_REG_SCR1,
    LOONG_REG_SCR2,
    LOONG_REG_SCR3,
} LoongRegLbt;

typedef enum LoongInsTdJ {
    LOONGI_MOVGR2SCR = 0x00000800,
} LoongInsTdJ;

typedef enum LoongInsDTj {
    LOONGI_MOVSCR2GR = 0x00000c00,
} LoongInsDTj;

static inline void rvjit_loong_TdJ_op(rvjit_block_t* block, LoongInsTdJ insn, LoongRegLbt td, regid_t rj)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_J(rj) | LOONGF_Td(td));
    rvjit_put_code(block, code, 4);
}

static inline void rvjit_loong_DTj_op(rvjit_block_t* block, LoongInsDTj insn, regid_t rd, LoongRegLbt tj)
{
    uint8_t code[4];
    write_uint32_le_m(code, insn | LOONGF_Tj(tj) | LOONGF_D(rd));
    rvjit_put_code(block, code, 4);
}

static inline void rvjit_loong_Sd10k16_reloc_mc(LoongInsSd10k16 insn, void* addr, ptrdiff_t simm)
{
    write_uint32_le_m(addr, insn | LOONGF_Sd10k16_MC(simm));
}

static inline void rvjit_loong_Sd10k16_reloc_offset(LoongInsSd10k16 insn, void* addr, ptrdiff_t offset)
{
    write_uint32_le_m(addr, insn | LOONGF_Sd10k16_OFFSET(offset));
}

static inline void rvjit_loong_condbranch_patch_mc(void* addr, ptrdiff_t simm)
{
    uint32_t insn = read_uint32_le_m(addr) & 0xfc0003ff;
    write_uint32_le_m(addr, insn | LOONGF_Sk16_MC(simm));
}

static inline void rvjit_loong_branch_patch_mc(void* addr, ptrdiff_t simm)
{
    uint32_t insn = read_uint32_le_m(addr) & 0xfc000000;
    write_uint32_le_m(addr, insn | LOONGF_Sd10k16_MC(simm));
}

static inline void rvjit_loong_condbranch_patch_offset(void* addr, ptrdiff_t offset)
{
    uint32_t insn = read_uint32_le_m(addr) & 0xfc0003ff;
    write_uint32_le_m(addr, insn | LOONGF_Sk16_OFFSET(offset));
}

/* Patch: Replacing insn with new branch insn */
/* I hate "branch", why not calling it "jump" if we're doing uncond control-flow transfer? */

static inline void rvjit_loong_branch_patch_offset(void* addr, ptrdiff_t offset)
{
    write_uint32_le_m(addr, LOONGI_B | LOONGF_Sd10k16_OFFSET(offset));
}

static inline size_t rvjit_native_abireclaim_hregmask(void)
{
    // We have enough caller-saved registers, no need for push/pop as well
    return 0;
}

static inline void rvjit_native_push(rvjit_block_t* block, regid_t reg)
{
    UNUSED(block);
    UNUSED(reg);
    rvvm_fatal("Unimplemented rvjit_native_push for Loong64 backend");
}

static inline void rvjit_native_pop(rvjit_block_t* block, regid_t reg)
{
    UNUSED(block);
    UNUSED(reg);
    rvvm_fatal("Unimplemented rvjit_native_pop for Loong64 backend");
}

/*
 * Basic functionality
 */
static inline void rvjit_native_zero_reg(rvjit_block_t* block, regid_t reg)
{
    rvjit_loong_DJ_op(block, LOONGI_MOVE, reg, LOONG_REG_ZERO);
}

static inline void rvjit_native_ret(rvjit_block_t* block)
{
    rvjit_loong_EMPTY_op(block, LOONGI_RET);
}

static inline void rvjit_native_setreg32s(rvjit_block_t* block, regid_t reg, int32_t imm) {
    if(checki12(imm)) {
        rvjit_loong_DJSk12_op(block, LOONGI_ADDI_W, reg, LOONG_REG_ZERO, imm);
    } else {
        int32_t hi20 = imm >> 12;
        int32_t lo12 = imm & 0xfff;
        rvjit_loong_DSj20_op(block, LOONGI_LU12I_W, reg, hi20);
        if (lo12)
            rvjit_loong_DJSk12_op(block, LOONGI_ADDI_W, reg, reg, lo12);
    }
}

static inline void rvjit_native_setreg32(rvjit_block_t* block, regid_t reg, uint32_t imm) {
    rvjit_native_setreg32s(block, reg, (int32_t)imm);
}

static inline void rvjit_native_setregw(rvjit_block_t* block, regid_t reg, uint64_t imm) {
    if (checku32(imm)) {
        rvjit_native_setreg32(block, reg, (uint32_t)imm);
    } else {
        int32_t hi32 = imm >> 32;
        int32_t lo32 = imm & 0xffffffff;
        rvjit_loong_DSj20_op(block, LOONGI_LU12I_W, reg, lo32 >> 12);
        rvjit_loong_DJSk12_op(block, LOONGI_ADDI_W, reg, reg, lo32 & 0xfff);
        rvjit_loong_DSj20_op(block, LOONGI_CU32I_D, reg, hi32 & 0xfffff);
        rvjit_loong_DJSk12_op(block, LOONGI_LU52I_D, reg, reg, hi32 >> 20);
    }
}    

static inline branch_t rvjit_native_jmp(rvjit_block_t* block, branch_t handle, bool target)
{
    if (target) {
        // This is a jump label
        if (handle == BRANCH_NEW) {
            // Backward jump: Save label address
            return block->size;
        } else {
            // Forward jump: Patch jump offset
            rvjit_loong_Sd10k16_reloc_offset(LOONGI_B, block->code + handle, block->size - handle);
            return BRANCH_NEW;
        }
    } else {
        // This is a jump entry
        if (handle == BRANCH_NEW) {
            // Forward jump: Emit insn, patch it later
            branch_t tmp = block->size;
            rvjit_loong_Sd10k16_op_offset(block, LOONGI_B, 0);
            return tmp;
        } else {
            // Backward jump: Emit instruction using label address
            rvjit_loong_Sd10k16_op_offset(block, LOONGI_B, handle - block->size);
            return BRANCH_NEW;
        }
    }
}

static branch_t rvjit_loong_condbranch_entry(rvjit_block_t* block, uint32_t insn, regid_t hrs1, regid_t hrs2, branch_t handle)
{
    // RV Branch: rs1 (op) rs2; LA Branch: rj (op) rd
    if (handle == BRANCH_NEW) {
        branch_t tmp = block->size;
        rvjit_loong_DJSk16_op_offset(block, insn, hrs2, hrs1, 0);
        return tmp;
    } else {
        rvjit_loong_DJSk16_op_offset(block, insn, hrs2, hrs1, handle - block->size);
        return BRANCH_NEW;
    }
}

static branch_t rvjit_loong_condbranch_target(rvjit_block_t* block, branch_t handle)
{
    if (handle == BRANCH_NEW) {
        return block->size;
    } else {
        // Patch to jump
        rvjit_loong_branch_patch_offset(block->code + handle, block->size - handle);
        return BRANCH_NEW;
    }
}

static inline branch_t rvjit_loong_condbranch(rvjit_block_t* block, uint32_t insn, regid_t hrs1, regid_t hrs2, branch_t handle, bool target)
{
    if (target) {
        return rvjit_loong_condbranch_target(block, handle);
    } else {
        return rvjit_loong_condbranch_entry(block, insn, hrs1, hrs2, handle);
    }
}

/*
 * Linker routines
 */

// Emit jump instruction (may return false if offset cannot be encoded)
static inline bool rvjit_tail_jmp(rvjit_block_t* block, int32_t offset)
{
    if (checki28(offset)) {
        rvjit_loong_Sd10k16_op_offset(block, LOONGI_B, offset);
    } else {
        regid_t tmp = rvjit_claim_hreg(block);
        // rvjit_loong_DSj20_op(block, LOONGI_PCADDU12I, tmp, (offset + ((offset & 0x800) << 1)) >> 12);
        // rvjit_loong_DJSk16_op_offset(block, LOONGI_JIRL, LOONG_REG_ZERO, tmp, ((offset & 0xfff) ^ 0x800) - 0x800);
        rvjit_loong_DSj20_op(block, LOONGI_PCADDU18I, tmp, (offset + ((offset & 0x20000) << 1)) >> 18);
        rvjit_loong_DJSk16_op_offset(block, LOONGI_JIRL, LOONG_REG_ZERO, tmp, offset & 0x3fffc);
        rvjit_free_hreg(block, tmp);
    }
    return true;
}

// Emit patchable ret instruction
static inline void rvjit_patchable_ret(rvjit_block_t *block)
{
    rvjit_loong_EMPTY_op(block, LOONGI_RET);
}

// Jump if word pointed to by addr is nonzero (may emit nothing if the offset cannot be encoded)
// Used to check interrupts in block linkage
static inline void rvjit_tail_bnez(rvjit_block_t* block, regid_t addr, int32_t offset)
{
    size_t offset_fixup = block->size;
    int32_t off;
    regid_t tmp = rvjit_claim_hreg(block);
    rvjit_loong_DJSk12_op(block, LOONGI_LD_W, tmp, addr, 0);

    off = offset - (block->size - offset_fixup);
    if (checki23(off)) {
        // Offset fits in branch-zero insn
        rvjit_loong_JSd5k16_op_offset(block, LOONGI_BNEZ, tmp, offset);
    } else {
        // Use b for 28-bit offset or pcaddu18i/jirl for 38-bit offset
        branch_t l1 = rvjit_loong_condbranch(block, LOONGI_BEQ, tmp, LOONG_REG_ZERO, BRANCH_NEW, false);
        off = offset - (block->size - offset_fixup);
        if (checki28(off)) {
            rvjit_loong_Sd10k16_op_offset(block, LOONGI_B, offset);
        } else {
            rvjit_loong_DSj20_op(block, LOONGI_PCADDU18I, tmp, (off + ((off & 0x20000) << 1)) >> 18);
            rvjit_loong_DJSk16_op_offset(block, LOONGI_JIRL, LOONG_REG_ZERO, tmp, off & 0x3fffc);
        }
        rvjit_loong_condbranch(block, LOONGI_BEQ, tmp, LOONG_REG_ZERO, l1, true);

    }

    rvjit_free_hreg(block, tmp);
}

// Patch instruction at addr into ret
static inline void rvjit_patch_ret(void* addr)
{
    write_uint32_le_m(addr, LOONGI_RET);
}

// Patch jump instruction at addr (may return false if offset cannot be encoded)
static inline bool rvjit_patch_jmp(void* addr, int32_t offset)
{
    if (checki28(offset)) {
        rvjit_loong_branch_patch_offset(addr, offset);
        return true;
    } else {
        return false;
    }
}

static inline void rvjit_jmp_reg(rvjit_block_t *block, regid_t reg)
{
    rvjit_loong_J_Pseudo_op(block, LOONGI_JR, reg);
}

/*
 * RV32
 */
static inline void rvjit32_native_add(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_ADD_W, hrds, hrs1, hrs2);
}

static inline void rvjit32_native_sub(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_SUB_W, hrds, hrs1, hrs2);
}

static inline void rvjit32_native_or(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_OR, hrds, hrs1, hrs2);
}

static inline void rvjit32_native_and(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_AND, hrds, hrs1, hrs2);
}

static inline void rvjit32_native_xor(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_XOR, hrds, hrs1, hrs2);
}

static inline void rvjit32_native_sra(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_SRA_W, hrds, hrs1, hrs2);
}

static inline void rvjit32_native_srl(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_SRL_W, hrds, hrs1, hrs2);
}

static inline void rvjit32_native_sll(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_SLL_W, hrds, hrs1, hrs2);
}

static inline void rvjit32_native_addi(rvjit_block_t *block, regid_t hrds, regid_t hrs1, int32_t simm)
{
    rvjit_loong_DJSk12_op(block, LOONGI_ADDI_W, hrds, hrs1, simm);
}

static inline void rvjit32_native_ori(rvjit_block_t *block, regid_t hrds, regid_t hrs1, int32_t simm)
{
    if (checku11(simm))
        rvjit_loong_DJUk12_op(block, LOONGI_ORI, hrds, hrs1, simm);
    else {
        rvjit_loong_TdJ_op(block, LOONGI_MOVGR2SCR, LOONG_REG_SCR3, LOONG_REG_GSCR);
        
        rvjit_loong_DJSk12_op(block, LOONGI_ADDI_W, LOONG_REG_GSCR, LOONG_REG_ZERO, simm);
        rvjit_loong_DJK_op(block, LOONGI_OR, hrds, hrs1, LOONG_REG_GSCR);
        
        rvjit_loong_DTj_op(block, LOONGI_MOVSCR2GR, LOONG_REG_GSCR, LOONG_REG_SCR3);
    }
}

static inline void rvjit32_native_andi(rvjit_block_t *block, regid_t hrds, regid_t hrs1, int32_t simm)
{
    if (checku11(simm))
        rvjit_loong_DJUk12_op(block, LOONGI_ANDI, hrds, hrs1, simm);
    else {
        rvjit_loong_TdJ_op(block, LOONGI_MOVGR2SCR, LOONG_REG_SCR3, LOONG_REG_GSCR);
        
        rvjit_loong_DJSk12_op(block, LOONGI_ADDI_W, LOONG_REG_GSCR, LOONG_REG_ZERO, simm);
        rvjit_loong_DJK_op(block, LOONGI_AND, hrds, hrs1, LOONG_REG_GSCR);
        
        rvjit_loong_DTj_op(block, LOONGI_MOVSCR2GR, LOONG_REG_GSCR, LOONG_REG_SCR3);
    }
}

static inline void rvjit32_native_xori(rvjit_block_t *block, regid_t hrds, regid_t hrs1, int32_t simm)
{
    if (checku11(simm))
        rvjit_loong_DJUk12_op(block, LOONGI_XORI, hrds, hrs1, simm);
    else {
        rvjit_loong_TdJ_op(block, LOONGI_MOVGR2SCR, LOONG_REG_SCR3, LOONG_REG_GSCR);
        
        rvjit_loong_DJSk12_op(block, LOONGI_ADDI_W, LOONG_REG_GSCR, LOONG_REG_ZERO, simm);
        rvjit_loong_DJK_op(block, LOONGI_XOR, hrds, hrs1, LOONG_REG_GSCR);
        
        rvjit_loong_DTj_op(block, LOONGI_MOVSCR2GR, LOONG_REG_GSCR, LOONG_REG_SCR3);
    }
}

static inline void rvjit32_native_srai(rvjit_block_t *block, regid_t hrds, regid_t hrs1, uint8_t uimm)
{
    rvjit_loong_DJUk5_op(block, LOONGI_SRAI_W, hrds, hrs1, uimm);
}

static inline void rvjit32_native_srli(rvjit_block_t *block, regid_t hrds, regid_t hrs1, uint8_t uimm)
{
    rvjit_loong_DJUk5_op(block, LOONGI_SRLI_W, hrds, hrs1, uimm);
}

static inline void rvjit32_native_slli(rvjit_block_t *block, regid_t hrds, regid_t hrs1, uint8_t uimm)
{
    rvjit_loong_DJUk5_op(block, LOONGI_SLLI_W, hrds, hrs1, uimm);
}

static inline void rvjit32_native_slti(rvjit_block_t *block, regid_t hrds, regid_t hrs1, int32_t simm)
{
    rvjit_loong_DJSk12_op(block, LOONGI_SLTI, hrds, hrs1, simm);
}

static inline void rvjit32_native_sltiu(rvjit_block_t *block, regid_t hrds, regid_t hrs1, int32_t uimm)
{
    rvjit_loong_DJSk12_op(block, LOONGI_SLTUI, hrds, hrs1, uimm);
}

static inline void rvjit32_native_slt(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_SLT, hrds, hrs1, hrs2);
}

static inline void rvjit32_native_sltu(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_SLTU, hrds, hrs1, hrs2);
}

static inline void rvjit32_native_lb(rvjit_block_t *block, regid_t dest, regid_t base, int32_t offset)
{
    rvjit_loong_DJSk12_op(block, LOONGI_LD_B, dest, base, offset);
}

static inline void rvjit32_native_lbu(rvjit_block_t *block, regid_t dest, regid_t base, int32_t offset)
{
    rvjit_loong_DJSk12_op(block, LOONGI_LD_BU, dest, base, offset);
}

static inline void rvjit32_native_lh(rvjit_block_t *block, regid_t dest, regid_t base, int32_t offset)
{
    rvjit_loong_DJSk12_op(block, LOONGI_LD_H, dest, base, offset);
}

static inline void rvjit32_native_lhu(rvjit_block_t *block, regid_t dest, regid_t base, int32_t offset)
{
    rvjit_loong_DJSk12_op(block, LOONGI_LD_HU, dest, base, offset);
}

static inline void rvjit32_native_lw(rvjit_block_t *block, regid_t dest, regid_t base, int32_t offset)
{
    rvjit_loong_DJSk12_op(block, LOONGI_LD_W, dest, base, offset);
}

static inline void rvjit32_native_sb(rvjit_block_t *block, regid_t src, regid_t base, int32_t offset)
{
    rvjit_loong_DJSk12_op(block, LOONGI_ST_B, src, base, offset);
}

static inline void rvjit32_native_sh(rvjit_block_t *block, regid_t src, regid_t base, int32_t offset)
{
    rvjit_loong_DJSk12_op(block, LOONGI_ST_H, src, base, offset);
}

static inline void rvjit32_native_sw(rvjit_block_t *block, regid_t src, regid_t base, int32_t offset)
{
    rvjit_loong_DJSk12_op(block, LOONGI_ST_W, src, base, offset);
}

static inline branch_t rvjit32_native_beq(rvjit_block_t *block, regid_t hrs1, regid_t hrs2, branch_t handle, bool target)
{
    return rvjit_loong_condbranch(block, LOONGI_BEQ, hrs1, hrs2, handle, target);
}

static inline branch_t rvjit32_native_bne(rvjit_block_t *block, regid_t hrs1, regid_t hrs2, branch_t handle, bool target)
{
    return rvjit_loong_condbranch(block, LOONGI_BNE, hrs1, hrs2, handle, target);
}

static inline branch_t rvjit32_native_beqz(rvjit_block_t *block, regid_t hrs1, branch_t handle, bool target)
{
    return rvjit_loong_condbranch(block, LOONGI_BEQ, hrs1, LOONG_REG_ZERO, handle, target);
}

static inline branch_t rvjit32_native_bnez(rvjit_block_t *block, regid_t hrs1, branch_t handle, bool target)
{
    return rvjit_loong_condbranch(block, LOONGI_BNE, hrs1, LOONG_REG_ZERO, handle, target);
}

static inline branch_t rvjit32_native_blt(rvjit_block_t *block, regid_t hrs1, regid_t hrs2, branch_t handle, bool target)
{
    return rvjit_loong_condbranch(block, LOONGI_BLT, hrs1, hrs2, handle, target);
}

static inline branch_t rvjit32_native_bge(rvjit_block_t *block, regid_t hrs1, regid_t hrs2, branch_t handle, bool target)
{
    return rvjit_loong_condbranch(block, LOONGI_BGE, hrs1, hrs2, handle, target);
}

static inline branch_t rvjit32_native_bltu(rvjit_block_t *block, regid_t hrs1, regid_t hrs2, branch_t handle, bool target)
{
    return rvjit_loong_condbranch(block, LOONGI_BLTU, hrs1, hrs2, handle, target);
}

static inline branch_t rvjit32_native_bgeu(rvjit_block_t *block, regid_t hrs1, regid_t hrs2, branch_t handle, bool target)
{
    return rvjit_loong_condbranch(block, LOONGI_BGEU, hrs1, hrs2, handle, target);
}

static inline void rvjit32_native_mul(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_MUL_W, hrds, hrs1, hrs2);
}

static inline void rvjit32_native_mulh(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_MULH_W, hrds, hrs1, hrs2);
}

static inline void rvjit32_native_mulhu(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_MULH_WU, hrds, hrs1, hrs2);
}

static inline void rvjit32_native_mulhsu(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_TdJ_op(block, LOONGI_MOVGR2SCR, LOONG_REG_SCR3, LOONG_REG_GSCR);

    rvjit_loong_DJUk6_op(block, LOONGI_SLLI_D, LOONG_REG_GSCR, hrs2, 32);
    rvjit_loong_DJUk6_op(block, LOONGI_SRAI_D, LOONG_REG_GSCR, LOONG_REG_GSCR, 32);
    rvjit_loong_DJK_op(block, LOONGI_MUL_D, hrds, hrs1, LOONG_REG_GSCR);
    rvjit_loong_DJUk6_op(block, LOONGI_SRAI_D, hrds, hrds, 32);

    rvjit_loong_DTj_op(block, LOONGI_MOVSCR2GR, LOONG_REG_GSCR, LOONG_REG_SCR3);
}

static inline void rvjit32_native_div(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    if (false) // Flaky on LA1.0!
        rvjit_loong_DJK_op(block, LOONGI_DIV_W, hrds, hrs1, hrs2);
    else {
        rvjit_loong_TdJ_op(block, LOONGI_MOVGR2SCR, LOONG_REG_SCR3, LOONG_REG_GSCR);
        if (hrds != hrs1)
            rvjit_loong_TdJ_op(block, LOONGI_MOVGR2SCR, LOONG_REG_SCR2, hrs1);
        rvjit_loong_DJ_op(block, LOONGI_SEXT_W, hrds, hrs1);
        rvjit_loong_DJ_op(block, LOONGI_SEXT_W, LOONG_REG_GSCR, hrs2);
        rvjit_loong_DJK_op(block, LOONGI_DIV_W, hrds, hrds, LOONG_REG_GSCR);
        /*
        if (hrds == hrs1): hrs1 changes anyway
        if (hrds == hrs2): does not matter
        if (hrs1 == hrs2): does not matter
        if (hrds == hrs1 && hrs1 == hrs2): hrs1/hrs2 changes anyway
        */
        if (hrds != hrs1)
            rvjit_loong_DTj_op(block, LOONGI_MOVSCR2GR, hrs1, LOONG_REG_SCR2);
        rvjit_loong_DTj_op(block, LOONGI_MOVSCR2GR, LOONG_REG_GSCR, LOONG_REG_SCR3);
    }
}

static inline void rvjit32_native_divu(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    if (false) // Flaky on LA1.0!
        rvjit_loong_DJK_op(block, LOONGI_DIV_WU, hrds, hrs1, hrs2);
    else {
        rvjit_loong_TdJ_op(block, LOONGI_MOVGR2SCR, LOONG_REG_SCR3, LOONG_REG_GSCR);
        if (hrds != hrs1)
            rvjit_loong_TdJ_op(block, LOONGI_MOVGR2SCR, LOONG_REG_SCR2, hrs1);
        rvjit_loong_DJ_op(block, LOONGI_ZEXT_W, hrds, hrs1);
        rvjit_loong_DJ_op(block, LOONGI_ZEXT_W, LOONG_REG_GSCR, hrs2);
        rvjit_loong_DJK_op(block, LOONGI_DIV_WU, hrds, hrds, LOONG_REG_GSCR);
        /*
        if (hrds == hrs1): hrs1 changes anyway
        if (hrds == hrs2): does not matter
        if (hrs1 == hrs2): does not matter
        if (hrds == hrs1 && hrs1 == hrs2): hrs1/hrs2 changes anyway
        */
        if (hrds != hrs1)
            rvjit_loong_DTj_op(block, LOONGI_MOVSCR2GR, hrs1, LOONG_REG_SCR2);
        rvjit_loong_DTj_op(block, LOONGI_MOVSCR2GR, LOONG_REG_GSCR, LOONG_REG_SCR3);
    }
}

static inline void rvjit32_native_rem(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    if (false) // Flaky on LA1.0!
        rvjit_loong_DJK_op(block, LOONGI_MOD_W, hrds, hrs1, hrs2);
    else {
        rvjit_loong_TdJ_op(block, LOONGI_MOVGR2SCR, LOONG_REG_SCR3, LOONG_REG_GSCR);
        if (hrds != hrs1)
            rvjit_loong_TdJ_op(block, LOONGI_MOVGR2SCR, LOONG_REG_SCR2, hrs1);
        rvjit_loong_DJ_op(block, LOONGI_SEXT_W, hrds, hrs1);
        rvjit_loong_DJ_op(block, LOONGI_SEXT_W, LOONG_REG_GSCR, hrs2);
        rvjit_loong_DJK_op(block, LOONGI_MOD_W, hrds, hrds, LOONG_REG_GSCR);
        /*
        if (hrds == hrs1): hrs1 changes anyway
        if (hrds == hrs2): does not matter
        if (hrs1 == hrs2): does not matter
        if (hrds == hrs1 && hrs1 == hrs2): hrs1/hrs2 changes anyway
        */
        if (hrds != hrs1)
            rvjit_loong_DTj_op(block, LOONGI_MOVSCR2GR, hrs1, LOONG_REG_SCR2);
        rvjit_loong_DTj_op(block, LOONGI_MOVSCR2GR, LOONG_REG_GSCR, LOONG_REG_SCR3);
    }
}

static inline void rvjit32_native_remu(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    if (false) // Flaky on LA1.0!
        rvjit_loong_DJK_op(block, LOONGI_MOD_WU, hrds, hrs1, hrs2);
    else {
        rvjit_loong_TdJ_op(block, LOONGI_MOVGR2SCR, LOONG_REG_SCR3, LOONG_REG_GSCR);
        if (hrds != hrs1)
            rvjit_loong_TdJ_op(block, LOONGI_MOVGR2SCR, LOONG_REG_SCR2, hrs1);
        rvjit_loong_DJ_op(block, LOONGI_ZEXT_W, hrds, hrs1);
        rvjit_loong_DJ_op(block, LOONGI_ZEXT_W, LOONG_REG_GSCR, hrs2);
        rvjit_loong_DJK_op(block, LOONGI_MOD_WU, hrds, hrds, LOONG_REG_GSCR);
        /*
        if (hrds == hrs1): hrs1 changes anyway
        if (hrs1 == hrs2): does not matter
        if (hrds == hrs1 && hrs1 == hrs2): hrs1/hrs2 changes anyway
        */
        if (hrds != hrs1)
            rvjit_loong_DTj_op(block, LOONGI_MOVSCR2GR, hrs1, LOONG_REG_SCR2);
        rvjit_loong_DTj_op(block, LOONGI_MOVSCR2GR, LOONG_REG_GSCR, LOONG_REG_SCR3);
    }
}

/*
 * RV64
 */
static inline void rvjit64_native_add(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_ADD_D, hrds, hrs1, hrs2);
}

static inline void rvjit64_native_addw(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_ADD_W, hrds, hrs1, hrs2);
}

static inline void rvjit64_native_sub(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_SUB_D, hrds, hrs1, hrs2);
}

static inline void rvjit64_native_subw(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_SUB_W, hrds, hrs1, hrs2);
}

static inline void rvjit64_native_or(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_OR, hrds, hrs1, hrs2);
}

static inline void rvjit64_native_and(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_AND, hrds, hrs1, hrs2);
}

static inline void rvjit64_native_xor(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_XOR, hrds, hrs1, hrs2);
}

static inline void rvjit64_native_sra(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_SRA_D, hrds, hrs1, hrs2);
}

static inline void rvjit64_native_sraw(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_SRA_W, hrds, hrs1, hrs2);
}

static inline void rvjit64_native_srl(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_SRL_D, hrds, hrs1, hrs2);
}

static inline void rvjit64_native_srlw(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_SRL_W, hrds, hrs1, hrs2);
}

static inline void rvjit64_native_sll(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_SLL_D, hrds, hrs1, hrs2);
}

static inline void rvjit64_native_sllw(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_SLL_W, hrds, hrs1, hrs2);
}

static inline void rvjit64_native_addi(rvjit_block_t *block, regid_t hrds, regid_t hrs1, int32_t simm)
{
    rvjit_loong_DJSk12_op(block, LOONGI_ADDI_D, hrds, hrs1, simm);
}

static inline void rvjit64_native_addiw(rvjit_block_t *block, regid_t hrds, regid_t hrs1, int32_t simm)
{
    rvjit_loong_DJSk12_op(block, LOONGI_ADDI_W, hrds, hrs1, simm);
}

static inline void rvjit64_native_ori(rvjit_block_t *block, regid_t hrds, regid_t hrs1, int32_t simm)
{
    if (checku11(simm))
        rvjit_loong_DJUk12_op(block, LOONGI_ORI, hrds, hrs1, simm);
    else {
        rvjit_loong_TdJ_op(block, LOONGI_MOVGR2SCR, LOONG_REG_SCR3, LOONG_REG_GSCR);
        
        rvjit_loong_DJSk12_op(block, LOONGI_ADDI_W, LOONG_REG_GSCR, LOONG_REG_ZERO, simm);
        rvjit_loong_DJK_op(block, LOONGI_OR, hrds, hrs1, LOONG_REG_GSCR);
        
        rvjit_loong_DTj_op(block, LOONGI_MOVSCR2GR, LOONG_REG_GSCR, LOONG_REG_SCR3);
    }
}

static inline void rvjit64_native_andi(rvjit_block_t *block, regid_t hrds, regid_t hrs1, int32_t simm)
{
    if (checku11(simm))
        rvjit_loong_DJUk12_op(block, LOONGI_ANDI, hrds, hrs1, simm);
    else {
        rvjit_loong_TdJ_op(block, LOONGI_MOVGR2SCR, LOONG_REG_SCR3, LOONG_REG_GSCR);

        rvjit_loong_DJSk12_op(block, LOONGI_ADDI_W, LOONG_REG_GSCR, LOONG_REG_ZERO, simm);
        rvjit_loong_DJK_op(block, LOONGI_AND, hrds, hrs1, LOONG_REG_GSCR);

        rvjit_loong_DTj_op(block, LOONGI_MOVSCR2GR, LOONG_REG_GSCR, LOONG_REG_SCR3);
    }
}

static inline void rvjit64_native_xori(rvjit_block_t *block, regid_t hrds, regid_t hrs1, int32_t simm)
{
    if (checku11(simm))
        rvjit_loong_DJUk12_op(block, LOONGI_XORI, hrds, hrs1, simm);
    else {
        rvjit_loong_TdJ_op(block, LOONGI_MOVGR2SCR, LOONG_REG_SCR3, LOONG_REG_GSCR);

        rvjit_loong_DJSk12_op(block, LOONGI_ADDI_W, LOONG_REG_GSCR, LOONG_REG_ZERO, simm);
        rvjit_loong_DJK_op(block, LOONGI_XOR, hrds, hrs1, LOONG_REG_GSCR);

        rvjit_loong_DTj_op(block, LOONGI_MOVSCR2GR, LOONG_REG_GSCR, LOONG_REG_SCR3);
    }
}

static inline void rvjit64_native_srli(rvjit_block_t *block, regid_t hrds, regid_t hrs1, uint8_t uimm)
{
    rvjit_loong_DJUk6_op(block, LOONGI_SRLI_D, hrds, hrs1, uimm);
}

static inline void rvjit64_native_srliw(rvjit_block_t *block, regid_t hrds, regid_t hrs1, uint8_t uimm)
{
    rvjit_loong_DJUk5_op(block, LOONGI_SRLI_W, hrds, hrs1, uimm);
}

static inline void rvjit64_native_srai(rvjit_block_t *block, regid_t hrds, regid_t hrs1, uint8_t uimm)
{
    rvjit_loong_DJUk6_op(block, LOONGI_SRAI_D, hrds, hrs1, uimm);
}

static inline void rvjit64_native_sraiw(rvjit_block_t *block, regid_t hrds, regid_t hrs1, uint8_t uimm)
{
    rvjit_loong_DJUk5_op(block, LOONGI_SRAI_W, hrds, hrs1, uimm);
}

static inline void rvjit64_native_slli(rvjit_block_t *block, regid_t hrds, regid_t hrs1, uint8_t uimm)
{
    rvjit_loong_DJUk6_op(block, LOONGI_SLLI_D, hrds, hrs1, uimm);
}

static inline void rvjit64_native_slliw(rvjit_block_t *block, regid_t hrds, regid_t hrs1, uint8_t uimm)
{
    rvjit_loong_DJUk5_op(block, LOONGI_SLLI_W, hrds, hrs1, uimm);
}

static inline void rvjit64_native_slti(rvjit_block_t *block, regid_t hrds, regid_t hrs1, int32_t simm)
{
    rvjit_loong_DJSk12_op(block, LOONGI_SLTI, hrds, hrs1, simm);
}

static inline void rvjit64_native_sltiu(rvjit_block_t *block, regid_t hrds, regid_t hrs1, int32_t uimm)
{
    rvjit_loong_DJSk12_op(block, LOONGI_SLTUI, hrds, hrs1, uimm);
}

static inline void rvjit64_native_slt(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_SLT, hrds, hrs1, hrs2);
}

static inline void rvjit64_native_sltu(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_SLTU, hrds, hrs1, hrs2);
}

static inline void rvjit64_native_lb(rvjit_block_t *block, regid_t dest, regid_t base, int32_t offset)
{
    rvjit_loong_DJSk12_op(block, LOONGI_LD_B, dest, base, offset);
}

static inline void rvjit64_native_lbu(rvjit_block_t *block, regid_t dest, regid_t base, int32_t offset)
{
    rvjit_loong_DJSk12_op(block, LOONGI_LD_BU, dest, base, offset);
}

static inline void rvjit64_native_lh(rvjit_block_t *block, regid_t dest, regid_t base, int32_t offset)
{
    rvjit_loong_DJSk12_op(block, LOONGI_LD_H, dest, base, offset);
}

static inline void rvjit64_native_lhu(rvjit_block_t *block, regid_t dest, regid_t base, int32_t offset)
{
    rvjit_loong_DJSk12_op(block, LOONGI_LD_HU, dest, base, offset);
}

static inline void rvjit64_native_lw(rvjit_block_t *block, regid_t dest, regid_t base, int32_t offset)
{
    rvjit_loong_DJSk12_op(block, LOONGI_LD_W, dest, base, offset);
}

static inline void rvjit64_native_lwu(rvjit_block_t *block, regid_t dest, regid_t base, int32_t offset)
{
    rvjit_loong_DJSk12_op(block, LOONGI_LD_WU, dest, base, offset);
}

static inline void rvjit64_native_ld(rvjit_block_t *block, regid_t dest, regid_t base, int32_t offset)
{
    rvjit_loong_DJSk12_op(block, LOONGI_LD_D, dest, base, offset);
}

static inline void rvjit64_native_sb(rvjit_block_t *block, regid_t src, regid_t base, int32_t offset)
{
    rvjit_loong_DJSk12_op(block, LOONGI_ST_B, src, base, offset);
}

static inline void rvjit64_native_sh(rvjit_block_t *block, regid_t src, regid_t base, int32_t offset)
{
    rvjit_loong_DJSk12_op(block, LOONGI_ST_H, src, base, offset);
}

static inline void rvjit64_native_sw(rvjit_block_t *block, regid_t src, regid_t base, int32_t offset)
{
    rvjit_loong_DJSk12_op(block, LOONGI_ST_W, src, base, offset);
}

static inline void rvjit64_native_sd(rvjit_block_t *block, regid_t src, regid_t base, int32_t offset)
{
    rvjit_loong_DJSk12_op(block, LOONGI_ST_D, src, base, offset);
}

static inline branch_t rvjit64_native_beq(rvjit_block_t *block, regid_t hrs1, regid_t hrs2, branch_t handle, bool target)
{
    return rvjit_loong_condbranch(block, LOONGI_BEQ, hrs1, hrs2, handle, target);
}

static inline branch_t rvjit64_native_bne(rvjit_block_t *block, regid_t hrs1, regid_t hrs2, branch_t handle, bool target)
{
    return rvjit_loong_condbranch(block, LOONGI_BNE, hrs1, hrs2, handle, target);
}

static inline branch_t rvjit64_native_beqz(rvjit_block_t *block, regid_t hrs1, branch_t handle, bool target)
{
    return rvjit_loong_condbranch(block, LOONGI_BEQ, hrs1, LOONG_REG_ZERO, handle, target);
}

static inline branch_t rvjit64_native_bnez(rvjit_block_t *block, regid_t hrs1, branch_t handle, bool target)
{
    return rvjit_loong_condbranch(block, LOONGI_BNE, hrs1, LOONG_REG_ZERO, handle, target);
}

static inline branch_t rvjit64_native_blt(rvjit_block_t *block, regid_t hrs1, regid_t hrs2, branch_t handle, bool target)
{
    return rvjit_loong_condbranch(block, LOONGI_BLT, hrs1, hrs2, handle, target);
}

static inline branch_t rvjit64_native_bge(rvjit_block_t *block, regid_t hrs1, regid_t hrs2, branch_t handle, bool target)
{
    return rvjit_loong_condbranch(block, LOONGI_BGE, hrs1, hrs2, handle, target);
}

static inline branch_t rvjit64_native_bltu(rvjit_block_t *block, regid_t hrs1, regid_t hrs2, branch_t handle, bool target)
{
    return rvjit_loong_condbranch(block, LOONGI_BLTU, hrs1, hrs2, handle, target);
}

static inline branch_t rvjit64_native_bgeu(rvjit_block_t *block, regid_t hrs1, regid_t hrs2, branch_t handle, bool target)
{
    return rvjit_loong_condbranch(block, LOONGI_BGEU, hrs1, hrs2, handle, target);
}

static inline void rvjit64_native_mul(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_MUL_D, hrds, hrs1, hrs2);
}

static inline void rvjit64_native_mulh(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_MULH_D, hrds, hrs1, hrs2);
}

static inline void rvjit64_native_mulhu(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_MULH_DU, hrds, hrs1, hrs2);
}

static inline void rvjit64_native_mulhsu(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_TdJ_op(block, LOONGI_MOVGR2SCR, LOONG_REG_SCR3, LOONG_REG_GSCR);

    rvjit_loong_DJUk6_op(block, LOONGI_SRAI_D, LOONG_REG_GSCR, hrs1, 63);
    regid_t hrdsu = rvjit_claim_hreg(block);
    rvjit_loong_DJK_op(block, LOONGI_MULH_DU, hrdsu, hrs1, hrs2);
    rvjit_loong_DJK_op(block, LOONGI_MUL_D, LOONG_REG_GSCR, LOONG_REG_GSCR, hrs2);
    rvjit_loong_DJK_op(block, LOONGI_ADD_D, hrds, hrdsu, LOONG_REG_GSCR);
    rvjit_free_hreg(block, hrdsu);

    rvjit_loong_DTj_op(block, LOONGI_MOVSCR2GR, LOONG_REG_GSCR, LOONG_REG_SCR3);
}

static inline void rvjit64_native_div(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_DIV_D, hrds, hrs1, hrs2);
}

static inline void rvjit64_native_divu(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_DIV_DU, hrds, hrs1, hrs2);
}

static inline void rvjit64_native_rem(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_MOD_D, hrds, hrs1, hrs2);
}

static inline void rvjit64_native_remu(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_MOD_DU, hrds, hrs1, hrs2);
}

static inline void rvjit64_native_mulw(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    rvjit_loong_DJK_op(block, LOONGI_MUL_W, hrds, hrs1, hrs2);
}

static inline void rvjit64_native_divw(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    if (false) // Flaky on LA1.0!
        rvjit_loong_DJK_op(block, LOONGI_DIV_W, hrds, hrs1, hrs2);
    else {
        rvjit_loong_TdJ_op(block, LOONGI_MOVGR2SCR, LOONG_REG_SCR3, LOONG_REG_GSCR);
        if (hrds != hrs1)
            rvjit_loong_TdJ_op(block, LOONGI_MOVGR2SCR, LOONG_REG_SCR2, hrs1);
        rvjit_loong_DJ_op(block, LOONGI_SEXT_W, hrds, hrs1);
        rvjit_loong_DJ_op(block, LOONGI_SEXT_W, LOONG_REG_GSCR, hrs2);
        rvjit_loong_DJK_op(block, LOONGI_DIV_W, hrds, hrds, LOONG_REG_GSCR);
        /*
        if (hrds == hrs1): hrs1 changes anyway
        if (hrds == hrs2): does not matter
        if (hrs1 == hrs2): does not matter
        if (hrds == hrs1 && hrs1 == hrs2): hrs1/hrs2 changes anyway
        */
        if (hrds != hrs1)
            rvjit_loong_DTj_op(block, LOONGI_MOVSCR2GR, hrs1, LOONG_REG_SCR2);
        rvjit_loong_DTj_op(block, LOONGI_MOVSCR2GR, LOONG_REG_GSCR, LOONG_REG_SCR3);
    }
}

static inline void rvjit64_native_divuw(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    if (false) // Flaky on LA1.0!
        rvjit_loong_DJK_op(block, LOONGI_DIV_WU, hrds, hrs1, hrs2);
    else {
        rvjit_loong_TdJ_op(block, LOONGI_MOVGR2SCR, LOONG_REG_SCR3, LOONG_REG_GSCR);
        if (hrds != hrs1)
            rvjit_loong_TdJ_op(block, LOONGI_MOVGR2SCR, LOONG_REG_SCR2, hrs1);
        rvjit_loong_DJ_op(block, LOONGI_ZEXT_W, hrds, hrs1);
        rvjit_loong_DJ_op(block, LOONGI_ZEXT_W, LOONG_REG_GSCR, hrs2);
        rvjit_loong_DJK_op(block, LOONGI_DIV_WU, hrds, hrds, LOONG_REG_GSCR);
        /*
        if (hrds == hrs1): hrs1 changes anyway
        if (hrds == hrs2): does not matter
        if (hrs1 == hrs2): does not matter
        if (hrds == hrs1 && hrs1 == hrs2): hrs1/hrs2 changes anyway
        */
        if (hrds != hrs1)
            rvjit_loong_DTj_op(block, LOONGI_MOVSCR2GR, hrs1, LOONG_REG_SCR2);
        rvjit_loong_DTj_op(block, LOONGI_MOVSCR2GR, LOONG_REG_GSCR, LOONG_REG_SCR3);
    }
}

static inline void rvjit64_native_remw(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    if (false) // Flaky on LA1.0!
        rvjit_loong_DJK_op(block, LOONGI_MOD_W, hrds, hrs1, hrs2);
    else {
        rvjit_loong_TdJ_op(block, LOONGI_MOVGR2SCR, LOONG_REG_SCR3, LOONG_REG_GSCR);
        if (hrds != hrs1)
            rvjit_loong_TdJ_op(block, LOONGI_MOVGR2SCR, LOONG_REG_SCR2, hrs1);
        rvjit_loong_DJ_op(block, LOONGI_SEXT_W, hrds, hrs1);
        rvjit_loong_DJ_op(block, LOONGI_SEXT_W, LOONG_REG_GSCR, hrs2);
        rvjit_loong_DJK_op(block, LOONGI_MOD_W, hrds, hrds, LOONG_REG_GSCR);
        /*
        if (hrds == hrs1): hrs1 changes anyway
        if (hrds == hrs2): does not matter
        if (hrs1 == hrs2): does not matter
        if (hrds == hrs1 && hrs1 == hrs2): hrs1/hrs2 changes anyway
        */
        if (hrds != hrs1)
            rvjit_loong_DTj_op(block, LOONGI_MOVSCR2GR, hrs1, LOONG_REG_SCR2);
        rvjit_loong_DTj_op(block, LOONGI_MOVSCR2GR, LOONG_REG_GSCR, LOONG_REG_SCR3);
    }
}

static inline void rvjit64_native_remuw(rvjit_block_t *block, regid_t hrds, regid_t hrs1, regid_t hrs2)
{
    if (false) // Flaky on LA1.0!
        rvjit_loong_DJK_op(block, LOONGI_MOD_WU, hrds, hrs1, hrs2);
    else {
        rvjit_loong_TdJ_op(block, LOONGI_MOVGR2SCR, LOONG_REG_SCR3, LOONG_REG_GSCR);
        if (hrds != hrs1)
            rvjit_loong_TdJ_op(block, LOONGI_MOVGR2SCR, LOONG_REG_SCR2, hrs1);
        rvjit_loong_DJ_op(block, LOONGI_ZEXT_W, hrds, hrs1);
        rvjit_loong_DJ_op(block, LOONGI_ZEXT_W, LOONG_REG_GSCR, hrs2);
        rvjit_loong_DJK_op(block, LOONGI_MOD_WU, hrds, hrds, LOONG_REG_GSCR);
        /*
        if (hrds == hrs1): hrs1 changes anyway
        if (hrs1 == hrs2): does not matter
        if (hrds == hrs1 && hrs1 == hrs2): hrs1/hrs2 changes anyway
        */
        if (hrds != hrs1)
            rvjit_loong_DTj_op(block, LOONGI_MOVSCR2GR, hrs1, LOONG_REG_SCR2);
        rvjit_loong_DTj_op(block, LOONGI_MOVSCR2GR, LOONG_REG_GSCR, LOONG_REG_SCR3);
    }
}

#endif // RVJIT_LOONG64_H