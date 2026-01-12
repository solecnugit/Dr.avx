/**
 * @file rewrite_utils.h
 *
 * @copyright Copyright (c) 2024
 *
 */

/*
 * rewrite_utils.h -- instr rewriting utility functions
 */

#ifndef _REWRITE_UTILS_H_
#define _REWRITE_UTILS_H_

#include "../globals.h"
#include "../fragment.h"
#include "../emit.h"
#include "dr_ir_utils.h"
#include "globals_api.h"
#include "globals_shared.h"
#include "instr.h"
#include "instr_create_shared.h"
#include "instrlist.h"
#include "decode.h"
#include "decode_fast.h"
#include "disassemble.h"
#include "instrument.h"
#include "include/dr_defines.h"
#include "opnd_api.h"
#include "arch_exports.h"

#include <sys/types.h>
#include <stdarg.h>

#define STD_OUTF 1
#define STD_ERRF 2

/**
 * avx512 opcode constants for corresponding rewrite functions arrays to index.
 * NOTE: we use the enum index defined in `/core/ir/x86/opcode_api.h` to fast lookup
 *       rewrite function in the `rewrite_funcs` pointer array. Since some avx2 instr
 *       also encoded in evex prefix, we treat them as psudo avx512 instr in the array
 *       as well.
 */
#define AVX512_FIRST_OP OP_vmovss
#define AVX512_LAST_OP OP_vshufi64x2

#define NUM_AVX512_INSTR_OP ((AVX512_LAST_OP - AVX512_FIRST_OP) + 1)
#define TO_AVX512_RWFUNC_INDEX(opcode) (opcode - AVX512_FIRST_OP)
#define TO_AVX512_RWFUNC_OPCODE(index) (index + AVX512_FIRST_OP)

/** xyzmm register size in hexadecimal */
#define SIZE_OF_XMM 0x10 /* 16bytes(128bits) */
#define SIZE_OF_YMM 0x20 /* 32bytes(256bits) */
#define SIZE_OF_ZMM 0x40 /* 64bytes(512bits) */

#define MCXT_NUM_YMM_SLOTS (MCXT_NUM_SIMD_SLOTS / 2) /* in avx2 only use half */

/** ymm register used bitmap manipulation macros */
#define SET_YMM_USED(dc, ymm_idx) ((dc)->ymm_used_bitmap |= (0x1 << (ymm_idx)))
#define SET_YMM_UNUSED(dc, ymm_idx) ((dc)->ymm_used_bitmap &= ~(0x1 << (ymm_idx)))
#define IS_YMM_USED(dc, index) (((dc)->ymm_used_bitmap & (0x1 << (index))) != 0)

/** high ymm register spilled bitmap manipulation macros */
#define SET_HIGH_YMM_SPILLED(dc, ymm_idx) \
    ((dc)->high_ymm_is_spilled_bitmap |= (0x1 << ((ymm_idx)-16))) // assume ymm_idx >=16 and <= 31
#define IS_HIGH_YMM_SPILLED(dc, ymm_idx) (((dc)->high_ymm_is_spilled_bitmap & (0x1 << ((ymm_idx)-16))) != 0)
#define RESET_ALL_HIGH_YMM(dc) ((dc)->high_ymm_is_spilled_bitmap = 0x0)

/** find first unused ymm register index */
#define FIND_FIRST_UNUSED_YMM(dc)         \
    ({                                    \
        int _ymm_index = -1;              \
        for (int _i = 0; _i < 16; _i++) { \
            if (!IS_YMM_USED(dc, _i)) {   \
                _ymm_index = _i;          \
                break;                    \
            }                             \
        }                                 \
        _ymm_index;                       \
    })

/** find and set first unused ymm register index */
#define FIND_AND_SET_FIRST_UNUSED_YMM(dc) \
    ({                                    \
        int _ymm_index = -1;              \
        for (int _i = 0; _i < 16; _i++) { \
            if (!IS_YMM_USED(dc, _i)) {   \
                SET_YMM_USED(dc, _i);     \
                _ymm_index = _i;          \
                break;                    \
            }                             \
        }                                 \
        _ymm_index;                       \
    })

/** zmm register index and ymm register index mapping */
#define ZMM_REG_NUM 32
#define YMM_REG_NUM 16

/** AVX512 xyzmm register index */
#define ZMM_REG_FIRST DR_REG_ZMM0
#define ZMM_REG_LAST DR_REG_ZMM31
#define YMM_REG_FIRST DR_REG_YMM0
#define YMM_REG_LAST DR_REG_YMM31
#define XMM_REG_FIRST DR_REG_XMM0
#define XMM_REG_LAST DR_REG_XMM31

/** AVX2 xymm register index  */
#define AVX2_YMM_REG_LAST DR_REG_YMM15
#define AVX2_XMM_REG_LAST DR_REG_XMM15

/** AVX2 ymm loop mapping magic numbers and mapping function */
#define AVX2_YMM_UPPER_BOUND 31
#define AVX2_XMM_UPPER_BOUND 31
// f(x) = 31 - x, ymm31 -> ymm0, ymm30 -> ymm1, ... , ymm17->ymm14, ymm16 -> ymm15
#define HIGH_16YMM_TO_LOW_16YMM_MAPPING(idx) (AVX2_YMM_UPPER_BOUND - idx)
#define HIGH_TO_LOW_YMM_REG_ID_NUM(idx) (YMM_REG_FIRST + HIGH_16YMM_TO_LOW_16YMM_MAPPING(idx))
#define HIGH_16XMM_TO_LOW_16XMM_MAPPING(idx) (AVX2_XMM_UPPER_BOUND - idx)
#define HIGH_TO_LOW_XMM_REG_ID_NUM(idx) (XMM_REG_FIRST + HIGH_16XMM_TO_LOW_16XMM_MAPPING(idx))

/** yzmm register DR reg_num and real reg_idx conversion */
#define TO_ZMM_REG_INDEX(reg_id) (reg_id - ZMM_REG_FIRST) // assume DR_REG_ZMM0 <= reg_id <= DR_REG_ZMM31
#define TO_YMM_REG_INDEX(reg_id) (reg_id - YMM_REG_FIRST)
#define TO_XMM_REG_INDEX(reg_id) (reg_id - XMM_REG_FIRST)
#define TO_ZMM_REG_ID_NUM(idx) (idx + ZMM_REG_FIRST)
#define TO_YMM_REG_ID_NUM(idx) (idx + YMM_REG_FIRST)
#define TO_XMM_REG_ID_NUM(idx) (idx + XMM_REG_FIRST)

/** zmm idx to ymm idx, and ymm idx to xmm idx conversion */
#define ZMM_TO_YMM_DISTANCE (DR_REG_ZMM0 - DR_REG_YMM0)
#define YMM_TO_XMM_DISTANCE (DR_REG_YMM0 - DR_REG_XMM0)
#define ZMM_INDEX_TO_YMM_INDEX(reg_id) (reg_id - ZMM_TO_YMM_DISTANCE)
#define YMM_INDEX_TO_XMM_INDEX(reg_id) (reg_id - YMM_TO_XMM_DISTANCE)

/** XYZMM conversion */
#define XMM_TO_YMM(reg_id) ((reg_id - DR_REG_XMM0) + DR_REG_YMM0)
#define XMM_TO_ZMM(reg_id) ((reg_id - DR_REG_XMM0) + DR_REG_ZMM0)
#define YMM_TO_XMM(reg_id) ((reg_id - DR_REG_YMM0) + DR_REG_XMM0)
#define YMM_TO_ZMM(reg_id) ((reg_id - DR_REG_YMM0) + DR_REG_ZMM0)
#define ZMM_TO_XMM(reg_id) ((reg_id - DR_REG_ZMM0) + DR_REG_XMM0)
#define ZMM_TO_YMM(reg_id) ((reg_id - DR_REG_ZMM0) + DR_REG_YMM0)

/** k_ register DR reg_num and real k_idx conversion */
#define TO_K_REG_INDEX(reg_id) (reg_id - DR_REG_K0)
#define TO_K_REG_ID_NUM(idx) (idx + DR_REG_K0)

/** simd reg categorization function */
#define IS_ZMM_REG(reg_id) ((reg_id >= ZMM_REG_FIRST) && (reg_id <= ZMM_REG_LAST))
#define IS_YMM_REG(reg_id) ((reg_id >= YMM_REG_FIRST) && (reg_id <= YMM_REG_LAST))
#define IS_XMM_REG(reg_id) ((reg_id >= XMM_REG_FIRST) && (reg_id <= XMM_REG_LAST))
#define IS_HIGH_YMM_REG(reg_id) ((reg_id >= DR_REG_YMM16) && (reg_id <= DR_REG_YMM31))
#define IS_HIGH_XMM_REG(reg_id) ((reg_id >= DR_REG_XMM16) && (reg_id <= DR_REG_XMM31))

/** mask reg categorization function */
#define IS_MASK_REG(reg_id) ((DR_REG_K0 <= reg_id) && (reg_id <= DR_REG_K7))

/** gpr categorization function */
#define IS_64GPR_REG(reg_id) ((reg_id >= DR_REG_RAX) && (reg_id <= DR_REG_R15))
#define IS_32GPR_REG(reg_id) ((reg_id >= DR_REG_EAX) && (reg_id <= DR_REG_R15D))
#define IS_16GPR_REG(reg_id) ((reg_id >= DR_REG_AX) && (reg_id <= DR_REG_R15W))
#define IS_8GPR_REG(reg_id) ((reg_id >= DR_REG_AL) && (reg_id <= DR_REG_R15L))
#define IS_QWORD_GPR(reg_id) IS_64GPR_REG(reg_id)
#define IS_DWORD_GPR(reg_id) IS_32GPR_REG(reg_id)
#define IS_WORD_GPR(reg_id) IS_16GPR_REG(reg_id)
#define IS_BYTE_GPR(reg_id) IS_8GPR_REG(reg_id)

/** gpr alias conversion function */

/** Convert 64-bit GPR to specified width GPR */
#define GPR64_TO_GPR32(reg_id) (reg_id - DR_REG_RAX + DR_REG_EAX)
#define GPR64_TO_GPR16(reg_id) (reg_id - DR_REG_RAX + DR_REG_AX)
#define GPR_QWORD_TO_DWORD(reg_id) GPR64_TO_GPR32(reg_id)
#define GPR_QWORD_TO_WORD(reg_id) GPR64_TO_GPR16(reg_id)

/** Convert 32-bit GPR to specified width GPR */
#define GPR32_TO_GPR64(reg_id) (reg_id - DR_REG_EAX + DR_REG_RAX)
#define GPR32_TO_GPR16(reg_id) (reg_id - DR_REG_EAX + DR_REG_AX)
#define GPR_DWORD_TO_QWORD(reg_id) GPR32_TO_GPR64(reg_id)
#define GPR_DWORD_TO_WORD(reg_id) GPR32_TO_GPR16(reg_id)

/** Convert 16-bit GPR to specified width GPR */
#define GPR16_TO_GPR64(reg_id) (reg_id - DR_REG_AX + DR_REG_RAX)
#define GPR16_TO_GPR32(reg_id) (reg_id - DR_REG_AX + DR_REG_EAX)
#define GPR_WORD_TO_QWORD(reg_id) GPR16_TO_GPR64(reg_id)
#define GPR_WORD_TO_DWORD(reg_id) GPR16_TO_GPR32(reg_id)

/**
 * Convert to 8-bit low byte register (safer than the previous macros)
 * These handle the special cases for 8-bit registers
 */
#define GPR64_TO_GPR8(reg_id)                              \
    ((reg_id >= DR_REG_RAX && reg_id <= DR_REG_RBX)        \
         ? (reg_id - DR_REG_RAX + DR_REG_AL)               \
         : ((reg_id >= DR_REG_RSP && reg_id <= DR_REG_RDI) \
                ? (reg_id - DR_REG_RSP + DR_REG_SPL)       \
                : ((reg_id >= DR_REG_R8 && reg_id <= DR_REG_R15) ? (reg_id - DR_REG_R8 + DR_REG_R8L) : DR_REG_NULL)))

#define GPR32_TO_GPR8(reg_id)                                                                           \
    ((reg_id >= DR_REG_EAX && reg_id <= DR_REG_EBX)                                                     \
         ? (reg_id - DR_REG_EAX + DR_REG_AL)                                                            \
         : ((reg_id >= DR_REG_ESP && reg_id <= DR_REG_EDI)                                              \
                ? (reg_id - DR_REG_ESP + DR_REG_SPL)                                                    \
                : ((reg_id >= DR_REG_R8D && reg_id <= DR_REG_R15D) ? (reg_id - DR_REG_R8D + DR_REG_R8L) \
                                                                   : DR_REG_NULL)))

#define GPR16_TO_GPR8(reg_id)                                                                           \
    ((reg_id >= DR_REG_AX && reg_id <= DR_REG_BX)                                                       \
         ? (reg_id - DR_REG_AX + DR_REG_AL)                                                             \
         : ((reg_id >= DR_REG_SP && reg_id <= DR_REG_DI)                                                \
                ? (reg_id - DR_REG_SP + DR_REG_SPL)                                                     \
                : ((reg_id >= DR_REG_R8W && reg_id <= DR_REG_R15W) ? (reg_id - DR_REG_R8W + DR_REG_R8L) \
                                                                   : DR_REG_NULL)))

#define GPR_QWORD_TO_BYTE(reg_id) GPR64_TO_GPR8(reg_id)
#define GPR_DWORD_TO_BYTE(reg_id) GPR32_TO_GPR8(reg_id)
#define GPR_WORD_TO_BYTE(reg_id) GPR16_TO_GPR8(reg_id)

/* ======================================== *
 *     zmm, xymm register mapping uitls
 * ======================================== */

typedef struct _dr_ymm_pair_t dr_ymm_pair_t;
struct _dr_ymm_pair_t {
    uint ymm_upper; // upper ymm reg idx
    uint ymm_lower; // lower ymm reg idx
};

/** FIXME: The following policy for mapping is not thread safe, since we manully manipulate the global variable */

/**
 * @brief find empty for zmm register mapping
 * @note No replacement policy yet, if all ymm_bitmap is full, we end the program.
 * @param ymm_pair ymm register pair to be mapped, default NULL when given
 * @return SUCCESS(find empty pair) or NOT_FIND
 */
int
find_and_set_unused_ymm_pair(dcontext_t *dcontext, dr_ymm_pair_t *ymm_pair);

/**
 * @brief find empty ymm register index in 0~15, use reverse linear probe if mapping function result is already occupied
 * @note No replacement policy as well, TODO: add replacement policy when rewrite engiene entry point change
 * @param ymm_reg ymm register to be mapped
 * @return DR_REG_YMM_FIND_INDEX or DR_REG_NULL
 */
reg_id_t
find_and_set_unused_ymm(dcontext_t *dcontext, reg_id_t ymm_reg);

/**
 * @brief Find an unused physical XMM register (0..15) for a logical XMM and
 *        return the mapped physical XMM register id.
 *
 * If the preferred mapping is occupied, performs a reverse linear probe to find
 * a free slot. Returns DR_REG_NULL if no slot is available.
 *
 * @param dcontext Thread context
 * @param xmm_reg Logical XMM register id (DR_REG_XMM0..DR_REG_XMM31)
 * @return reg_id_t Mapped physical XMM register id, or DR_REG_NULL on failure
 */
reg_id_t
find_and_set_unused_xmm(dcontext_t *dcontext, reg_id_t xmm_reg);

/**
 * @brief Create a mapping YMM operand. If the YMM is in 0..15, return it directly;
 *        otherwise map (16..31) down to an available low YMM and return that.
 *
 * Used when lowering AVX-512 semantics to AVX2 where only YMM0..YMM15 exist.
 *
 * @param dcontext Thread context
 * @param ymm_reg Logical YMM register id (DR_REG_YMM0..DR_REG_YMM31)
 * @return opnd_t Operand referencing the mapped physical YMM register
 */
opnd_t
create_mapping_ymm_opnd(dcontext_t *dcontext, reg_id_t ymm_reg);

/**
 * @brief Create a mapping xmm opnd object. If xmm in (0,15) then directly create, otherwise turn (16, 31) into valid
 * empty (0,15) then create corresponding xmm opnd.
 *
 * @param xmm_reg
 * @return opnd_t
 */
opnd_t
create_mapping_xmm_opnd(dcontext_t *dcontext, reg_id_t xmm_reg);

/**
 * @brief Select two unused YMM slots (0..15), mark them as used, and store the
 *        indices into `ymm_pair`.
 *
 * Typically used to map a single ZMM register onto two YMM registers.
 *
 * @param ymm_pair Output pair of selected YMM indices {lower, upper}
 * @return SUCCESS on success, NOT_FIND if insufficient free slots
 */
int
replace_and_spill_ymm_pair(dr_ymm_pair_t *ymm_pair);

/* ======================================== *
 *   rewrite util tls spill related funcs
 * ======================================== */
#define XMM_SPILL_SLOT0 DR_REG_XMM10
#define XMM_SPILL_SLOT1 DR_REG_XMM11
#define XMM_SPILL_SLOT2 DR_REG_XMM12
#define XMM_SPILL_SLOT3 DR_REG_XMM13
#define XMM_SPILL_SLOT4 DR_REG_XMM14
#define XMM_SPILL_SLOT5 DR_REG_XMM15
#define XMM_SPILL_SLOTMAX XMM_SPILL_SLOT5

#define YMM_SPILL_SLOT0 DR_REG_YMM10
#define YMM_SPILL_SLOT1 DR_REG_YMM11
#define YMM_SPILL_SLOT2 DR_REG_YMM12
#define YMM_SPILL_SLOT3 DR_REG_YMM13
#define YMM_SPILL_SLOT4 DR_REG_YMM14
#define YMM_SPILL_SLOT5 DR_REG_YMM15
#define YMM_SPILL_SLOTMAX YMM_SPILL_SLOT5

#define ZMM_SPILL_SLOT0 DR_REG_ZMM10
#define ZMM_SPILL_SLOT1 DR_REG_ZMM11
#define ZMM_SPILL_SLOT2 DR_REG_ZMM12
#define ZMM_SPILL_SLOT3 DR_REG_ZMM13
#define ZMM_SPILL_SLOT4 DR_REG_ZMM14
#define ZMM_SPILL_SLOT5 DR_REG_ZMM15
#define ZMM_SPILL_SLOTMAX ZMM_SPILL_SLOT5
#define NEED_SPILL_XMM(reg) (DR_REG_XMM16 <= reg && reg <= DR_REG_XMM31)
#define NEED_SPILL_YMM(reg) (DR_REG_YMM16 <= reg && reg <= DR_REG_YMM31)
#define NEED_SPILL_ZMM(reg) (DR_REG_ZMM16 <= reg && reg <= DR_REG_ZMM31)

#define MAX_SPILL_AVOIDS 6

typedef struct _spill_reg_pair_t spill_reg_pair_t;
struct _spill_reg_pair_t {
    reg_id_t reg1;
    reg_id_t reg2;
};

/**
 * @brief Return one XMM spill register that is different from `reg`.
 *
 * Scans the predefined XMM spill slots and returns the first candidate not
 * equal to the avoid register.
 *
 * @param reg Register to avoid
 * @return reg_id_t Chosen XMM spill register, or DR_REG_NULL on error
 */
reg_id_t
find_one_available_spill_xmm(reg_id_t reg);

/**
 * @brief Find an available XMM spill register that doesn't conflict with any of the specified registers
 *
 * @param reg_to_avoid1 First register to avoid
 * @param reg_to_avoid2 Second register to avoid (can be DR_REG_NULL if not needed)
 * @param reg_to_avoid3 Third register to avoid (can be DR_REG_NULL if not needed)
 * @return reg_id_t Available spill register
 */
reg_id_t
find_available_spill_xmm_avoiding(reg_id_t reg_to_avoid1, reg_id_t reg_to_avoid2, reg_id_t reg_to_avoid3);

/**
 * @brief Return one YMM spill register that is different from `reg`.
 *
 * Scans the predefined YMM spill slots and returns the first candidate not
 * equal to the avoid register.
 *
 * @param reg Register to avoid
 * @return reg_id_t Chosen YMM spill register, or DR_REG_NULL on error
 */
reg_id_t
find_one_available_spill_ymm(reg_id_t reg);

/**
 * @brief Find an available YMM spill register that doesn't conflict with any of the specified registers
 *
 * @param reg_to_avoid1 First register to avoid
 * @param reg_to_avoid2 Second register to avoid (can be DR_REG_NULL if not needed)
 * @param reg_to_avoid3 Third register to avoid (can be DR_REG_NULL if not needed)
 * @return reg_id_t Available spill register
 */
reg_id_t
find_available_spill_ymm_avoiding(reg_id_t reg_to_avoid1, reg_id_t reg_to_avoid2, reg_id_t reg_to_avoid3);

/**
 * @brief Find an available XMM spill register that doesn't conflict with any of the specified registers
 *
 * @param num_regs Number of registers to avoid
 * @param ... Registers to avoid
 * @return reg_id_t Available spill register
 */
reg_id_t
find_available_spill_xmm_avoiding_variadic(int num_regs, ...);

/**
 * @brief Find an available YMM spill register that doesn't conflict with any of the specified registers
 *
 * @param num_regs Number of registers to avoid
 * @param ... Registers to avoid
 * @return reg_id_t Available spill register
 */
reg_id_t
find_available_spill_ymm_avoiding_variadic(int num_regs, ...);

/**
 * @brief Return one ZMM spill register that is different from `reg`.
 *
 * Scans the predefined ZMM spill slots and returns the first candidate not
 * equal to the avoid register.
 *
 * @param reg Register to avoid
 * @return reg_id_t Chosen ZMM spill register, or DR_REG_NULL on error
 */
reg_id_t
find_one_available_spill_zmm(reg_id_t reg);

/**
 * @brief Find an available ZMM spill register that doesn't conflict with any of the specified registers
 *
 * @param reg_to_avoid1 First register to avoid
 * @param reg_to_avoid2 Second register to avoid (can be DR_REG_NULL if not needed)
 * @param reg_to_avoid3 Third register to avoid (can be DR_REG_NULL if not needed)
 * @return reg_id_t Available spill register
 */
reg_id_t
find_available_spill_zmm_avoiding(reg_id_t reg_to_avoid1, reg_id_t reg_to_avoid2, reg_id_t reg_to_avoid3);

/**
 * @brief Choose two distinct XMM spill registers avoiding `reg1` and `reg2`.
 *
 * @param reg1 First register to avoid
 * @param reg2 Second register to avoid
 * @return spill_reg_pair_t Pair of chosen registers; DR_REG_NULL fields on failure
 */
spill_reg_pair_t
find_two_available_spill_xmms(reg_id_t reg1, reg_id_t reg2);

/**
 * @brief Choose two distinct YMM spill registers avoiding `reg1` and `reg2`.
 *
 * @param reg1 First register to avoid
 * @param reg2 Second register to avoid
 * @return spill_reg_pair_t Pair of chosen registers; DR_REG_NULL fields on failure
 */
spill_reg_pair_t
find_two_available_spill_ymms(reg_id_t reg1, reg_id_t reg2);

/**
 * @brief Choose two distinct ZMM spill registers avoiding `reg1` and `reg2`.
 *
 * @param reg1 First register to avoid
 * @param reg2 Second register to avoid
 * @return spill_reg_pair_t Pair of chosen registers; DR_REG_NULL fields on failure
 */
spill_reg_pair_t
find_two_available_spill_zmms(reg_id_t reg1, reg_id_t reg2);

/**
 * @brief mov %gs:tls_base_ymm_offset -> %rax
 * need an additional instruction before to save %rax to tls_rax_slot
 */
/**
 * @brief Create an instruction to load the TLS base (SIMD area) into %rax.
 *
 * Caller is responsible for saving/restoring %rax if needed prior to insertion.
 *
 * @param dcontext Thread context
 * @return instr_t* Newly created instruction
 */
instr_t *
instr_create_load_tls_base(dcontext_t *dcontext);

/**
 * @brief Save YMM0..YMM15 to TLS spill area.
 *
 * Inserts meta-instructions before `first_avx512_instr_next` to store all YMM registers.
 *
 * @param dcontext Thread context
 * @param ilist Instruction list to insert into
 * @param first_avx512_instr_next Insertion point
 */
void
save_simd_to_tls(dcontext_t *dcontext, instrlist_t *ilist, instr_t *first_avx512_instr_next);

/**
 * @brief Restore YMM0..YMM15 from TLS spill area.
 *
 * Inserts meta-instructions before `last_avx512_instr_next` to reload all YMM registers.
 *
 * @param dcontext Thread context
 * @param ilist Instruction list to insert into
 * @param last_avx512_instr_next Insertion point
 */
void
restore_simd_from_tls(dcontext_t *dcontext, instrlist_t *ilist, instr_t *last_avx512_instr_next);

/**
 * @brief Emit instructions to save only the SIMD registers used by the instruction at `pc` into TLS.
 *
 * @param dcontext Thread context
 * @param pc Application PC of the AVX-512 instruction
 * @return instr_t* The last inserted instruction of the save sequence
 */
instr_t *
save_simd_to_tls_finegrained(dcontext_t *dcontext, app_pc pc);

/**
 * @brief Emit instructions to restore only the SIMD registers used by the instruction at `pc` from TLS.
 *
 * @param dcontext Thread context
 * @param pc Application PC of the AVX-512 instruction
 * @return instr_t* The last inserted instruction of the restore sequence
 */
instr_t *
restore_simd_from_tls_finegrained(dcontext_t *dcontext, app_pc pc);

/**
 * @brief Replace a logical YMM with a mapped physical YMM and spill if required.
 *
 * @param ymm Pointer to the YMM descriptor to be replaced/spilled
 * @return int SUCCESS on success, error code otherwise
 */
int
replace_and_spill_ymm(dr_ymm_t *ymm);

/**
 * @brief Replace a logical XMM with a mapped physical XMM and spill if required.
 *
 * @param xmm Pointer to the XMM descriptor to be replaced/spilled
 * @return int SUCCESS on success, error code otherwise
 */
int
replace_and_spill_xmm(dr_xmm_t *xmm);

typedef struct _dr_zmm_map_ymm_pair_t dr_zmm_map_ymm_pair_t;
struct _dr_zmm_map_ymm_pair_t {
    uint zmm_idx;
    dr_ymm_pair_t ymm_pair;
};

extern dr_zmm_map_ymm_pair_t zmm_to_ymm_pair_mappings[MCXT_NUM_SIMD_SLOTS];

typedef struct _dr_ymm_map_ymm_t dr_ymm_map_ymm_t;
struct _dr_ymm_map_ymm_t {
    uint ymm_idx;
    uint ymm_mapping_idx;
};

extern dr_ymm_map_ymm_t ymm_to_ymm_mappings[MCXT_NUM_SIMD_SLOTS];

/**
 * @brief Initialize the ZMM-to-YMM pair mapping table to empty entries.
 */
void
init_zmm_to_ymm_pair_mapping();

/**
 * @brief Add a mapping from a ZMM index to a YMM pair.
 *
 * @param zmm_idx ZMM register index (0..MCXT_NUM_SIMD_SLOTS-1)
 * @param ymm_pair Pair of YMM indices (lower, upper)
 * @return int SUCCESS on success, error code otherwise
 */
int
add_zmm_to_ymm_pair_mapping(int zmm_idx, dr_ymm_pair_t *ymm_pair);

/**
 * @brief Retrieve the mapped YMM pair for a ZMM index.
 *
 * @param zmm_idx ZMM register index
 * @param ymm_pair Output pair (lower, upper) on success
 * @return int SUCCESS if found, NOT_GET if no mapping
 */
int
get_zmm_to_ymm_pair_mapping(int zmm_idx, dr_ymm_pair_t *ymm_pair);

/**
 * @brief Update the mapped YMM pair for a ZMM index.
 *
 * @param zmm_idx ZMM register index
 * @param ymm_pair New pair to set
 * @return int SUCCESS on success, error code otherwise
 */
int
set_zmm_to_ymm_pair_mapping(int zmm_idx, dr_ymm_pair_t *ymm_pair);

/**
 * @brief Print a ZMM-to-YMM pair mapping entry for debugging.
 *
 * @param zmm_idx ZMM register index to print
 */
void
print_zmm_to_ymm_pair_mapping_to_file(int zmm_idx);

#define NOT_FIND -1
#define NOT_GET -1
#define EMPTY -1

#define NULL_INSTR NULL

/* ======================================== *
 *          spilled gpr selector
 * ======================================== */
#define GPR_SPILL_SLOT0 DR_REG_RAX
#define GPR_SPILL_SLOT1 DR_REG_RCX
#define GPR_SPILL_SLOT2 DR_REG_RDX
#define GPR_SPILL_SLOT3 DR_REG_RBX
#define GPR_SPILL_SLOT4 DR_REG_RSI
#define GPR_SPILL_SLOT5 DR_REG_RDI
#define GPR_SPILL_SLOT6 DR_REG_R8
#define GPR_SPILL_SLOT7 DR_REG_R9
#define GPR_SPILL_SLOT8 DR_REG_R10
#define GPR_SPILL_SLOT9 DR_REG_R11
#define GPR_SPILL_SLOT10 DR_REG_R12
#define GPR_SPILL_SLOT11 DR_REG_R13
#define GPR_SPILL_SLOT12 DR_REG_R14
#define GPR_SPILL_SLOT13 DR_REG_R15
#define GPR_SPILL_SLOTMAX GPR_SPILL_SLOT13
#define GPR_SPILL_SLOT_NUM 14

#define find_spills_avoiding_1(out1, num_avoids, ...) \
    find_available_spill_gprs_avoiding_outptrs(1, (num_avoids), &(out1), __VA_ARGS__)

#define find_spills_avoiding_2(out1, out2, num_avoids, ...) \
    find_available_spill_gprs_avoiding_outptrs(2, (num_avoids), &(out1), &(out2), __VA_ARGS__)

#define find_spills_avoiding_3(out1, out2, out3, num_avoids, ...) \
    find_available_spill_gprs_avoiding_outptrs(3, (num_avoids), &(out1), &(out2), &(out3), __VA_ARGS__)

#define find_spills_avoiding_4(out1, out2, out3, out4, num_avoids, ...) \
    find_available_spill_gprs_avoiding_outptrs(4, (num_avoids), &(out1), &(out2), &(out3), &(out4), __VA_ARGS__)

#define find_spills_avoiding_5(out1, out2, out3, out4, out5, num_avoids, ...)                                \
    find_available_spill_gprs_avoiding_outptrs(5, (num_avoids), &(out1), &(out2), &(out3), &(out4), &(out5), \
                                               __VA_ARGS__)

#define find_spills_avoiding_6(out1, out2, out3, out4, out5, out6, num_avoids, ...)                                   \
    find_available_spill_gprs_avoiding_outptrs(6, (num_avoids), &(out1), &(out2), &(out3), &(out4), &(out5), &(out6), \
                                               __VA_ARGS__)

reg_id_t
find_one_available_spill_gpr_avoiding_variadic(int num_avoids, ...);

int
find_available_spill_gprs_avoiding_outptrs(int needed, int num_avoids, ...);

/* ======================================== *
 *     rewrite util functions signatures
 * ======================================== */

#define print_file_multiple_zmm_mappings(...)                              \
    do {                                                                   \
        reg_id_t regs[] = { __VA_ARGS__ };                                 \
        for (size_t i = 0; i < (sizeof(regs) / sizeof(regs[0])); i++) {    \
            print_file_zmm_to_ymm_pair_mapping(TO_ZMM_REG_INDEX(regs[i])); \
        }                                                                  \
    } while (0)

#ifdef DEBUG
/**
 * @brief Debug print of one ZMM-to-YMM mapping entry.
 *
 * @param zmm_idx ZMM index
 */
void
print_file_zmm_to_ymm_pair_mapping(int zmm_idx);

/**
 * @brief Debug print of one high->low YMM mapping entry.
 *
 * @param ymm_idx Logical YMM index (typically 16..31)
 */
void
print_file_ymm_to_ymm_mapping(int ymm_idx);

/**
 * @brief Print a formatted summary for a rewritten instruction and its operands.
 */
void
print_rewrite_info(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start, const char *instr_name,
                   bool has_mask, bool has_src1, bool has_src2, bool has_dst);

/**
 * @brief Print one instruction and optionally its ZMM register mapping info.
 */
void
print_rewrite_one_instr(dcontext_t *dcontext, instr_t *instr1, bool has_zmm_reg, reg_id_t *src_reg, reg_id_t *dst_reg);

/**
 * @brief Print two instructions and optionally their ZMM mapping info.
 */
void
print_rewrite_two_instr(dcontext_t *dcontext, instr_t *instr1, instr_t *instr2, bool has_zmm_reg, reg_id_t *src_reg,
                        reg_id_t *dst_reg);

/**
 * @brief Print three instructions for debugging.
 */
void
print_rewrite_three_instr(dcontext_t *dcontext, instr_t *instr1, instr_t *instr2, instr_t *instr3);

/**
 * @brief Print an arbitrary sequence of instructions for debugging.
 */
void
print_rewrite_variadic_instr(dcontext_t *dcontext, int num_instr, ...);

/**
 * @brief Dump the per-thread saved SIMD and opmask slots from mcontext for debugging.
 */
void
print_zmm_in_dcontext(dcontext_t *dcontext);

/**
 * @brief Dump the per-thread TLS-stored AVX-512 related areas for debugging.
 */
void
print_tls_avx512_regs(dcontext_t *dcontext);

#endif /* DEBUG */

/* ======================================== *
 *    instrlist handling funcs
 * ======================================== */

/**
 * @brief Append a variable number of instructions to an instruction list.
 *
 * @param dcontext Thread context
 * @param ilist Instruction list to append to
 * @param num_instr Number of instructions following in the varargs
 * @param ... instr_t* pointers
 */
void
instrlist_append_variadic_instr(dcontext_t *dcontext, instrlist_t *ilist, int num_instr, ...);

/**
 * @brief Link instruction `instr` to `next` by updating next/prev pointers.
 */
static inline void
instr_concat_next(instr_t *instr, instr_t *next)
{
    instr->next = next;
    next->prev = instr;
}

/**
 * @brief Concatenate a sequence of instructions via their next/prev links.
 *
 * @param ilist Unused parameter (kept for symmetry)
 * @param num_instr Number of instructions in the sequence
 * @param ... instr_t* pointers in order
 */
void
instrlist_concat_next_instr(instrlist_t *ilist, int num_instr, ...);

/* ======================================== *
 *    complicated instr handling funcs
 * ======================================== */

/**
 * @brief Check whether an AVX-512 instruction uses a zeroing mask (.Z).
 *
 * @param instr Instruction to inspect
 * @return true if zeroing mask is set; false otherwise
 */
bool
is_avx512_zero_mask(instr_t *instr);

/* marco template for rewrite function */

#define FIXED_ALLOC_BOTH_SRC(_s1, _s2, _dst) \
    do {                                     \
        (_s1) = YMM_SPILL_SLOT0;             \
        (_s2) = YMM_SPILL_SLOT1;             \
    } while (0)

#define FIXED_ALLOC_DST_ONLY(_d, _src1, _src2) \
    do {                                       \
        (_d) = YMM_SPILL_SLOT0;                \
    } while (0)

#define FIXED_ALLOC_SRC1_DST(_s1, _d, _src2) \
    do {                                     \
        (_s1) = YMM_SPILL_SLOT0;             \
        (_d) = YMM_SPILL_SLOT1;              \
    } while (0)

#define FIXED_ALLOC_SRC2_DST(_s2, _d, _src1) \
    do {                                     \
        (_s2) = YMM_SPILL_SLOT0;             \
        (_d) = YMM_SPILL_SLOT1;              \
    } while (0)

#define DYN_ALLOC_BOTH_SRC(_s1, _s2, _dst)                                           \
    do {                                                                             \
        (_s1) = find_available_spill_ymm_avoiding((_dst), DR_REG_NULL, DR_REG_NULL); \
        (_s2) = find_available_spill_ymm_avoiding((_dst), (_s1), DR_REG_NULL);       \
    } while (0)

#define DYN_ALLOC_DST_ONLY(_d, _src1, _src2)                                     \
    do {                                                                         \
        (_d) = find_available_spill_ymm_avoiding((_src1), (_src2), DR_REG_NULL); \
    } while (0)

#define DYN_ALLOC_SRC1_DST(_s1, _d, _src2)                                            \
    do {                                                                              \
        (_s1) = find_available_spill_ymm_avoiding((_src2), DR_REG_NULL, DR_REG_NULL); \
        (_d) = find_available_spill_ymm_avoiding((_src2), (_s1), DR_REG_NULL);        \
    } while (0)

#define DYN_ALLOC_SRC2_DST(_s2, _d, _src1)                                            \
    do {                                                                              \
        (_s2) = find_available_spill_ymm_avoiding((_src1), DR_REG_NULL, DR_REG_NULL); \
        (_d) = find_available_spill_ymm_avoiding((_src1), (_s2), DR_REG_NULL);        \
    } while (0)

#define DEFINE_YMM_YMM_BINOP_GEN_FIXED(OP)                                                              \
    DEFINE_YMM_YMM_BINOP_GEN_IMPL(OP, FIXED_ALLOC_BOTH_SRC, FIXED_ALLOC_DST_ONLY, FIXED_ALLOC_SRC1_DST, \
                                  FIXED_ALLOC_SRC2_DST)

#define DEFINE_YMM_YMM_BINOP_GEN_DYNAMIC(OP) \
    DEFINE_YMM_YMM_BINOP_GEN_IMPL(OP, DYN_ALLOC_BOTH_SRC, DYN_ALLOC_DST_ONLY, DYN_ALLOC_SRC1_DST, DYN_ALLOC_SRC2_DST)

#define DEFINE_YMM_YMM_BINOP_GEN_IMPL(OP, ALLOC_BOTH_SRC, ALLOC_DST_ONLY, ALLOC_SRC1_DST, ALLOC_SRC2_DST)              \
    instr_t *OP##_ymm_and_ymm_gen(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, reg_id_t src1_reg,         \
                                  reg_id_t src2_reg, reg_id_t dst_reg, reg_id_t mask_reg)                              \
    {                                                                                                                  \
        (void)mask_reg;                                                                                                \
        instrlist_remove(ilist, instr);                                                                                \
        instr_destroy(dcontext, instr);                                                                                \
                                                                                                                       \
        const uint src1_need_spill_flag = NEED_SPILL_YMM(src1_reg) ? 1 : 0;                                            \
        const uint src2_need_spill_flag = NEED_SPILL_YMM(src2_reg) ? 2 : 0;                                            \
        const uint dst_need_spill_flag = NEED_SPILL_YMM(dst_reg) ? 4 : 0;                                              \
        const uint need_spill_flag = src1_need_spill_flag | src2_need_spill_flag | dst_need_spill_flag;                \
                                                                                                                       \
        switch (need_spill_flag) {                                                                                     \
        case 0: { /* no spill */                                                                                       \
            opnd_t op_src1 = opnd_create_reg(src1_reg);                                                                \
            opnd_t op_src2 = opnd_create_reg(src2_reg);                                                                \
            opnd_t op_dst = opnd_create_reg(dst_reg);                                                                  \
            instr_t *i1 = INSTR_CREATE_##OP(dcontext, op_dst, op_src1, op_src2);                                       \
                                                                                                                       \
            return i1;                                                                                                 \
        } break;                                                                                                       \
                                                                                                                       \
        case 1: { /* src1 need spill */                                                                                \
            reg_id_t spill_src1_reg = DR_REG_NULL;                                                                     \
            if (src2_reg == dst_reg)                                                                                   \
                spill_src1_reg = find_one_available_spill_ymm(src2_reg);                                               \
            else                                                                                                       \
                spill_src1_reg = find_available_spill_ymm_avoiding(src2_reg, dst_reg, DR_REG_NULL);                    \
            opnd_t op_spill_src1 = opnd_create_reg(spill_src1_reg);                                                    \
            opnd_t op_src2 = opnd_create_reg(src2_reg);                                                                \
            opnd_t op_dst = opnd_create_reg(dst_reg);                                                                  \
            instr_t *i1 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_src1_reg,                                             \
                                                 TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_src1_reg)), OPSZ_32);         \
            instr_t *i2 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src1_reg,                                        \
                                                      TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(src1_reg)), OPSZ_32);          \
            instr_t *i3 = INSTR_CREATE_##OP(dcontext, op_dst, op_spill_src1, op_src2);                                 \
            instr_t *i4 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src1_reg,                                        \
                                                      TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_src1_reg)), OPSZ_32);    \
            instrlist_concat_next_instr(ilist, 4, i1, i2, i3, i4);                                                     \
            return i1;                                                                                                 \
        } break;                                                                                                       \
                                                                                                                       \
        case 2: { /* src2 need spill */                                                                                \
            reg_id_t spill_src2_reg = DR_REG_NULL;                                                                     \
            if (src1_reg == dst_reg)                                                                                   \
                spill_src2_reg = find_one_available_spill_ymm(src2_reg);                                               \
            else                                                                                                       \
                spill_src2_reg = find_available_spill_ymm_avoiding(src1_reg, dst_reg, DR_REG_NULL);                    \
            opnd_t op_spill_src2 = opnd_create_reg(spill_src2_reg);                                                    \
            opnd_t op_src1 = opnd_create_reg(src1_reg);                                                                \
            opnd_t op_dst = opnd_create_reg(dst_reg);                                                                  \
            instr_t *i1 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_src2_reg,                                             \
                                                 TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_src2_reg)), OPSZ_32);         \
            instr_t *i2 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src2_reg,                                        \
                                                      TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(src2_reg)), OPSZ_32);          \
            instr_t *i3 = INSTR_CREATE_##OP(dcontext, op_dst, op_src1, op_spill_src2);                                 \
            instr_t *i4 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src2_reg,                                        \
                                                      TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_src2_reg)), OPSZ_32);    \
            instrlist_concat_next_instr(ilist, 4, i1, i2, i3, i4);                                                     \
            return i1;                                                                                                 \
        } break;                                                                                                       \
                                                                                                                       \
        case 3: { /* both src need spill */                                                                            \
            reg_id_t spill_src1_reg, spill_src2_reg;                                                                   \
            ALLOC_BOTH_SRC(spill_src1_reg, spill_src2_reg, dst_reg);                                                   \
            opnd_t op_spill_src1 = opnd_create_reg(spill_src1_reg);                                                    \
            opnd_t op_spill_src2 = opnd_create_reg(spill_src2_reg);                                                    \
            opnd_t op_dst = opnd_create_reg(dst_reg);                                                                  \
            instr_t *i1 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_src1_reg,                                             \
                                                 TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_src1_reg)), OPSZ_32);         \
            instr_t *i2 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_src2_reg,                                             \
                                                 TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_src2_reg)), OPSZ_32);         \
            instr_t *i3 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src1_reg,                                        \
                                                      TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(src1_reg)), OPSZ_32);          \
            instr_t *i4 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src2_reg,                                        \
                                                      TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(src2_reg)), OPSZ_32);          \
            instr_t *i5 = INSTR_CREATE_##OP(dcontext, op_dst, op_spill_src1, op_spill_src2);                           \
            instr_t *i6 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src1_reg,                                        \
                                                      TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_src1_reg)), OPSZ_32);    \
            instr_t *i7 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src2_reg,                                        \
                                                      TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_src2_reg)), OPSZ_32);    \
            instrlist_concat_next_instr(ilist, 7, i1, i2, i3, i4, i5, i6, i7);                                         \
            return i1;                                                                                                 \
        } break;                                                                                                       \
                                                                                                                       \
        case 4: { /* dst need spill */                                                                                 \
            reg_id_t spill_dst_reg;                                                                                    \
            ALLOC_DST_ONLY(spill_dst_reg, src1_reg, src2_reg);                                                         \
            opnd_t op_src1 = opnd_create_reg(src1_reg);                                                                \
            opnd_t op_src2 = opnd_create_reg(src2_reg);                                                                \
            opnd_t op_spill_dst = opnd_create_reg(spill_dst_reg);                                                      \
            instr_t *i1 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_dst_reg,                                              \
                                                 TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_dst_reg)), OPSZ_32);          \
            instr_t *i2 = INSTR_CREATE_##OP(dcontext, op_spill_dst, op_src1, op_src2);                                 \
            instr_t *i3 =                                                                                              \
                SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_dst_reg, TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(dst_reg)), OPSZ_32); \
            instr_t *i4 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_dst_reg,                                         \
                                                      TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_dst_reg)), OPSZ_32);     \
            instrlist_concat_next_instr(ilist, 4, i1, i2, i3, i4);                                                     \
            return i1;                                                                                                 \
        } break;                                                                                                       \
                                                                                                                       \
        case 5: { /* src1 and dst need spill */                                                                        \
            reg_id_t spill_src1_reg, spill_dst_reg;                                                                    \
            ALLOC_SRC1_DST(spill_src1_reg, spill_dst_reg, src2_reg);                                                   \
            opnd_t op_spill_src1 = opnd_create_reg(spill_src1_reg);                                                    \
            opnd_t op_src2 = opnd_create_reg(src2_reg);                                                                \
            opnd_t op_spill_dst = opnd_create_reg(spill_dst_reg);                                                      \
            instr_t *i1 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_src1_reg,                                             \
                                                 TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_src1_reg)), OPSZ_32);         \
            instr_t *i2 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_dst_reg,                                              \
                                                 TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_dst_reg)), OPSZ_32);          \
            instr_t *i3 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src1_reg,                                        \
                                                      TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(src1_reg)), OPSZ_32);          \
            instr_t *i4 = INSTR_CREATE_##OP(dcontext, op_spill_dst, op_spill_src1, op_src2);                           \
            instr_t *i5 =                                                                                              \
                SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_dst_reg, TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(dst_reg)), OPSZ_32); \
            instr_t *i6 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src1_reg,                                        \
                                                      TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_src1_reg)), OPSZ_32);    \
            instr_t *i7 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_dst_reg,                                         \
                                                      TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_dst_reg)), OPSZ_32);     \
            instrlist_concat_next_instr(ilist, 7, i1, i2, i3, i4, i5, i6, i7);                                         \
            return i1;                                                                                                 \
        } break;                                                                                                       \
                                                                                                                       \
        case 6: { /* src2 and dst need spill */                                                                        \
            reg_id_t spill_src2_reg, spill_dst_reg;                                                                    \
            ALLOC_SRC2_DST(spill_src2_reg, spill_dst_reg, src1_reg);                                                   \
            opnd_t op_src1 = opnd_create_reg(src1_reg);                                                                \
            opnd_t op_spill_src2 = opnd_create_reg(spill_src2_reg);                                                    \
            opnd_t op_spill_dst = opnd_create_reg(spill_dst_reg);                                                      \
            instr_t *i1 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_src2_reg,                                             \
                                                 TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_src2_reg)), OPSZ_32);         \
            instr_t *i2 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_dst_reg,                                              \
                                                 TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_dst_reg)), OPSZ_32);          \
            instr_t *i3 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src2_reg,                                        \
                                                      TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(src2_reg)), OPSZ_32);          \
            instr_t *i4 = INSTR_CREATE_##OP(dcontext, op_spill_dst, op_src1, op_spill_src2);                           \
            instr_t *i5 =                                                                                              \
                SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_dst_reg, TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(dst_reg)), OPSZ_32); \
            instr_t *i6 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src2_reg,                                        \
                                                      TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_src2_reg)), OPSZ_32);    \
            instr_t *i7 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_dst_reg,                                         \
                                                      TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_dst_reg)), OPSZ_32);     \
            instrlist_concat_next_instr(ilist, 7, i1, i2, i3, i4, i5, i6, i7);                                         \
            return i1;                                                                                                 \
        } break;                                                                                                       \
                                                                                                                       \
        case 7: { /* all need spill */                                                                                 \
            if (src1_reg == dst_reg && src2_reg == dst_reg) {                                                          \
                reg_id_t spill_reg = YMM_SPILL_SLOT0;                                                                  \
                opnd_t op_spill = opnd_create_reg(spill_reg);                                                          \
                instr_t *i1 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_reg,                                              \
                                                     TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_reg)), OPSZ_32);          \
                instr_t *i2 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_reg,                                         \
                                                          TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(src1_reg)), OPSZ_32);      \
                instr_t *i3 = INSTR_CREATE_##OP(dcontext, op_spill, op_spill, op_spill);                               \
                instr_t *i4 =                                                                                          \
                    SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_reg, TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(dst_reg)), OPSZ_32); \
                instr_t *i5 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_reg,                                         \
                                                          TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_reg)), OPSZ_32);     \
                instrlist_concat_next_instr(ilist, 5, i1, i2, i3, i4, i5);                                             \
                return i1;                                                                                             \
            } else if (src1_reg == dst_reg) {                                                                          \
                reg_id_t spill_src1_reg = YMM_SPILL_SLOT0;                                                             \
                reg_id_t spill_src2_reg = YMM_SPILL_SLOT1;                                                             \
                reg_id_t spill_dst_reg = spill_src1_reg;                                                               \
                opnd_t op_spill_src1 = opnd_create_reg(spill_src1_reg);                                                \
                opnd_t op_spill_src2 = opnd_create_reg(spill_src2_reg);                                                \
                opnd_t op_spill_dst = opnd_create_reg(spill_dst_reg);                                                  \
                instr_t *i1 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_src1_reg,                                         \
                                                     TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_src1_reg)), OPSZ_32);     \
                instr_t *i2 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_src2_reg,                                         \
                                                     TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_src2_reg)), OPSZ_32);     \
                instr_t *i3 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src1_reg,                                    \
                                                          TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(src1_reg)), OPSZ_32);      \
                instr_t *i4 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src2_reg,                                    \
                                                          TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(src2_reg)), OPSZ_32);      \
                instr_t *i5 = INSTR_CREATE_##OP(dcontext, op_spill_dst, op_spill_src1, op_spill_src2);                 \
                instr_t *i6 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_dst_reg,                                          \
                                                     TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(dst_reg)), OPSZ_32);            \
                instr_t *i7 = RESTORE_SIMD_FROM_SIZED_TLS(                                                             \
                    dcontext, spill_src1_reg, TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_src1_reg)), OPSZ_32);            \
                instr_t *i8 = RESTORE_SIMD_FROM_SIZED_TLS(                                                             \
                    dcontext, spill_src2_reg, TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_src2_reg)), OPSZ_32);            \
                instrlist_concat_next_instr(ilist, 8, i1, i2, i3, i4, i5, i6, i7, i8);                                 \
                return i1;                                                                                             \
            } else if (src2_reg == dst_reg) {                                                                          \
                reg_id_t spill_src1_reg = YMM_SPILL_SLOT0;                                                             \
                reg_id_t spill_src2_reg = YMM_SPILL_SLOT1;                                                             \
                reg_id_t spill_dst_reg = spill_src2_reg;                                                               \
                opnd_t op_spill_src1 = opnd_create_reg(spill_src1_reg);                                                \
                opnd_t op_spill_src2 = opnd_create_reg(spill_src2_reg);                                                \
                opnd_t op_spill_dst = opnd_create_reg(spill_dst_reg);                                                  \
                instr_t *i1 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_src1_reg,                                         \
                                                     TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_src1_reg)), OPSZ_32);     \
                instr_t *i2 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_src2_reg,                                         \
                                                     TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_src2_reg)), OPSZ_32);     \
                instr_t *i3 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src1_reg,                                    \
                                                          TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(src1_reg)), OPSZ_32);      \
                instr_t *i4 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src2_reg,                                    \
                                                          TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(src2_reg)), OPSZ_32);      \
                instr_t *i5 = INSTR_CREATE_##OP(dcontext, op_spill_dst, op_spill_src1, op_spill_src2);                 \
                instr_t *i6 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_dst_reg,                                          \
                                                     TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(dst_reg)), OPSZ_32);            \
                instr_t *i7 = RESTORE_SIMD_FROM_SIZED_TLS(                                                             \
                    dcontext, spill_src1_reg, TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_src1_reg)), OPSZ_32);            \
                instr_t *i8 = RESTORE_SIMD_FROM_SIZED_TLS(                                                             \
                    dcontext, spill_src2_reg, TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_src2_reg)), OPSZ_32);            \
                instrlist_concat_next_instr(ilist, 8, i1, i2, i3, i4, i5, i6, i7, i8);                                 \
                return i1;                                                                                             \
            } else {                                                                                                   \
                reg_id_t spill_src1_reg = YMM_SPILL_SLOT0;                                                             \
                reg_id_t spill_src2_reg = YMM_SPILL_SLOT1;                                                             \
                reg_id_t spill_dst_reg = spill_src1_reg;                                                               \
                opnd_t op_spill_src1 = opnd_create_reg(spill_src1_reg);                                                \
                opnd_t op_spill_src2 = opnd_create_reg(spill_src2_reg);                                                \
                opnd_t op_spill_dst = opnd_create_reg(spill_dst_reg);                                                  \
                instr_t *i1 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_src1_reg,                                         \
                                                     TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_src1_reg)), OPSZ_32);     \
                instr_t *i2 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_src2_reg,                                         \
                                                     TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_src2_reg)), OPSZ_32);     \
                instr_t *i3 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src1_reg,                                    \
                                                          TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(src1_reg)), OPSZ_32);      \
                instr_t *i4 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src2_reg,                                    \
                                                          TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(src2_reg)), OPSZ_32);      \
                instr_t *i5 = INSTR_CREATE_##OP(dcontext, op_spill_dst, op_spill_src1, op_spill_src2);                 \
                instr_t *i6 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_dst_reg,                                          \
                                                     TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(dst_reg)), OPSZ_32);            \
                instr_t *i7 = RESTORE_SIMD_FROM_SIZED_TLS(                                                             \
                    dcontext, spill_src1_reg, TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_src1_reg)), OPSZ_32);            \
                instr_t *i8 = RESTORE_SIMD_FROM_SIZED_TLS(                                                             \
                    dcontext, spill_src2_reg, TLS_ZMM_idx_SLOT(TO_YMM_REG_INDEX(spill_src2_reg)), OPSZ_32);            \
                instrlist_concat_next_instr(ilist, 8, i1, i2, i3, i4, i5, i6, i7, i8);                                 \
                return i1;                                                                                             \
            }                                                                                                          \
        } break;                                                                                                       \
                                                                                                                       \
        default: {                                                                                                     \
            REWRITE_INFO(STD_OUTF, #OP "_ymm_and_ymm pattern not support\n");                                          \
        } break;                                                                                                       \
        }                                                                                                              \
        return NULL_INSTR;                                                                                             \
    }

#define DEFINE_XMM_XMM_BINOP_GEN_FIXED(OP)                                                              \
    DEFINE_XMM_XMM_BINOP_GEN_IMPL(OP, FIXED_ALLOC_BOTH_SRC, FIXED_ALLOC_DST_ONLY, FIXED_ALLOC_SRC1_DST, \
                                  FIXED_ALLOC_SRC2_DST)

#define DEFINE_XMM_XMM_BINOP_GEN_DYNAMIC(OP) \
    DEFINE_XMM_XMM_BINOP_GEN_IMPL(OP, DYN_ALLOC_BOTH_SRC, DYN_ALLOC_DST_ONLY, DYN_ALLOC_SRC1_DST, DYN_ALLOC_SRC2_DST)

#define DEFINE_XMM_XMM_BINOP_GEN_IMPL(OP, ALLOC_BOTH_SRC, ALLOC_DST_ONLY, ALLOC_SRC1_DST, ALLOC_SRC2_DST)              \
    instr_t *OP##_xmm_and_xmm_gen(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, reg_id_t src1_reg,         \
                                  reg_id_t src2_reg, reg_id_t dst_reg, reg_id_t mask_reg)                              \
    {                                                                                                                  \
        (void)mask_reg;                                                                                                \
        instrlist_remove(ilist, instr);                                                                                \
        instr_destroy(dcontext, instr);                                                                                \
                                                                                                                       \
        const uint src1_need_spill_flag = NEED_SPILL_XMM(src1_reg) ? 1 : 0;                                            \
        const uint src2_need_spill_flag = NEED_SPILL_XMM(src2_reg) ? 2 : 0;                                            \
        const uint dst_need_spill_flag = NEED_SPILL_XMM(dst_reg) ? 4 : 0;                                              \
        const uint need_spill_flag = src1_need_spill_flag | src2_need_spill_flag | dst_need_spill_flag;                \
                                                                                                                       \
        switch (need_spill_flag) {                                                                                     \
        case 0: { /* no spill */                                                                                       \
            opnd_t op_src1 = opnd_create_reg(src1_reg);                                                                \
            opnd_t op_src2 = opnd_create_reg(src2_reg);                                                                \
            opnd_t op_dst = opnd_create_reg(dst_reg);                                                                  \
            instr_t *i1 = INSTR_CREATE_##OP(dcontext, op_dst, op_src1, op_src2);                                       \
                                                                                                                       \
            return i1;                                                                                                 \
        } break;                                                                                                       \
                                                                                                                       \
        case 1: { /* src1 need spill */                                                                                \
            reg_id_t spill_src1_reg = DR_REG_NULL;                                                                     \
            if (src2_reg == dst_reg)                                                                                   \
                spill_src1_reg = find_one_available_spill_xmm(src2_reg);                                               \
            else                                                                                                       \
                spill_src1_reg = find_available_spill_xmm_avoiding(src2_reg, dst_reg, DR_REG_NULL);                    \
            opnd_t op_spill_src1 = opnd_create_reg(spill_src1_reg);                                                    \
            opnd_t op_src2 = opnd_create_reg(src2_reg);                                                                \
            opnd_t op_dst = opnd_create_reg(dst_reg);                                                                  \
            instr_t *i1 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_src1_reg,                                             \
                                                 TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_src1_reg)), OPSZ_16);         \
            instr_t *i2 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src1_reg,                                        \
                                                      TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(src1_reg)), OPSZ_16);          \
            instr_t *i3 = INSTR_CREATE_##OP(dcontext, op_dst, op_spill_src1, op_src2);                                 \
            instr_t *i4 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src1_reg,                                        \
                                                      TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_src1_reg)), OPSZ_16);    \
            instrlist_concat_next_instr(ilist, 4, i1, i2, i3, i4);                                                     \
            return i1;                                                                                                 \
        } break;                                                                                                       \
                                                                                                                       \
        case 2: { /* src2 need spill */                                                                                \
            reg_id_t spill_src2_reg = DR_REG_NULL;                                                                     \
            if (src1_reg == dst_reg)                                                                                   \
                spill_src2_reg = find_one_available_spill_xmm(src2_reg);                                               \
            else                                                                                                       \
                spill_src2_reg = find_available_spill_xmm_avoiding(src1_reg, dst_reg, DR_REG_NULL);                    \
            opnd_t op_spill_src2 = opnd_create_reg(spill_src2_reg);                                                    \
            opnd_t op_src1 = opnd_create_reg(src1_reg);                                                                \
            opnd_t op_dst = opnd_create_reg(dst_reg);                                                                  \
            instr_t *i1 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_src2_reg,                                             \
                                                 TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_src2_reg)), OPSZ_16);         \
            instr_t *i2 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src2_reg,                                        \
                                                      TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(src2_reg)), OPSZ_16);          \
            instr_t *i3 = INSTR_CREATE_##OP(dcontext, op_dst, op_src1, op_spill_src2);                                 \
            instr_t *i4 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src2_reg,                                        \
                                                      TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_src2_reg)), OPSZ_16);    \
            instrlist_concat_next_instr(ilist, 4, i1, i2, i3, i4);                                                     \
            return i1;                                                                                                 \
        } break;                                                                                                       \
                                                                                                                       \
        case 3: { /* both src need spill */                                                                            \
            reg_id_t spill_src1_reg, spill_src2_reg;                                                                   \
            ALLOC_BOTH_SRC_XMM(spill_src1_reg, spill_src2_reg, dst_reg);                                                   \
            opnd_t op_spill_src1 = opnd_create_reg(spill_src1_reg);                                                    \
            opnd_t op_spill_src2 = opnd_create_reg(spill_src2_reg);                                                    \
            opnd_t op_dst = opnd_create_reg(dst_reg);                                                                  \
            instr_t *i1 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_src1_reg,                                             \
                                                 TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_src1_reg)), OPSZ_16);         \
            instr_t *i2 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_src2_reg,                                             \
                                                 TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_src2_reg)), OPSZ_16);         \
            instr_t *i3 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src1_reg,                                        \
                                                      TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(src1_reg)), OPSZ_16);          \
            instr_t *i4 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src2_reg,                                        \
                                                      TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(src2_reg)), OPSZ_16);          \
            instr_t *i5 = INSTR_CREATE_##OP(dcontext, op_dst, op_spill_src1, op_spill_src2);                           \
            instr_t *i6 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src1_reg,                                        \
                                                      TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_src1_reg)), OPSZ_16);    \
            instr_t *i7 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src2_reg,                                        \
                                                      TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_src2_reg)), OPSZ_16);    \
            instrlist_concat_next_instr(ilist, 7, i1, i2, i3, i4, i5, i6, i7);                                         \
            return i1;                                                                                                 \
        } break;                                                                                                       \
                                                                                                                       \
        case 4: { /* dst need spill */                                                                                 \
            reg_id_t spill_dst_reg;                                                                                    \
            ALLOC_DST_ONLY(spill_dst_reg, src1_reg, src2_reg);                                                         \
            opnd_t op_src1 = opnd_create_reg(src1_reg);                                                                \
            opnd_t op_src2 = opnd_create_reg(src2_reg);                                                                \
            opnd_t op_spill_dst = opnd_create_reg(spill_dst_reg);                                                      \
            instr_t *i1 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_dst_reg,                                              \
                                                 TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_dst_reg)), OPSZ_16);          \
            instr_t *i2 = INSTR_CREATE_##OP(dcontext, op_spill_dst, op_src1, op_src2);                                 \
            instr_t *i3 =                                                                                              \
                SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_dst_reg, TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(dst_reg)), OPSZ_16); \
            instr_t *i4 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_dst_reg,                                         \
                                                      TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_dst_reg)), OPSZ_16);     \
            instrlist_concat_next_instr(ilist, 4, i1, i2, i3, i4);                                                     \
            return i1;                                                                                                 \
        } break;                                                                                                       \
                                                                                                                       \
        case 5: { /* src1 and dst need spill */                                                                        \
            reg_id_t spill_src1_reg, spill_dst_reg;                                                                    \
            ALLOC_SRC1_DST(spill_src1_reg, spill_dst_reg, src2_reg);                                                   \
            opnd_t op_spill_src1 = opnd_create_reg(spill_src1_reg);                                                    \
            opnd_t op_src2 = opnd_create_reg(src2_reg);                                                                \
            opnd_t op_spill_dst = opnd_create_reg(spill_dst_reg);                                                      \
            instr_t *i1 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_src1_reg,                                             \
                                                 TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_src1_reg)), OPSZ_16);         \
            instr_t *i2 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_dst_reg,                                              \
                                                 TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_dst_reg)), OPSZ_16);          \
            instr_t *i3 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src1_reg,                                        \
                                                      TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(src1_reg)), OPSZ_16);          \
            instr_t *i4 = INSTR_CREATE_##OP(dcontext, op_spill_dst, op_spill_src1, op_src2);                           \
            instr_t *i5 =                                                                                              \
                SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_dst_reg, TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(dst_reg)), OPSZ_16); \
            instr_t *i6 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src1_reg,                                        \
                                                      TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_src1_reg)), OPSZ_16);    \
            instr_t *i7 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_dst_reg,                                         \
                                                      TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_dst_reg)), OPSZ_16);     \
            instrlist_concat_next_instr(ilist, 7, i1, i2, i3, i4, i5, i6, i7);                                         \
            return i1;                                                                                                 \
        } break;                                                                                                       \
                                                                                                                       \
        case 6: { /* src2 and dst need spill */                                                                        \
            reg_id_t spill_src2_reg, spill_dst_reg;                                                                    \
            ALLOC_SRC2_DST(spill_src2_reg, spill_dst_reg, src1_reg);                                                   \
            opnd_t op_src1 = opnd_create_reg(src1_reg);                                                                \
            opnd_t op_spill_src2 = opnd_create_reg(spill_src2_reg);                                                    \
            opnd_t op_spill_dst = opnd_create_reg(spill_dst_reg);                                                      \
            instr_t *i1 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_src2_reg,                                             \
                                                 TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_src2_reg)), OPSZ_16);         \
            instr_t *i2 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_dst_reg,                                              \
                                                 TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_dst_reg)), OPSZ_16);          \
            instr_t *i3 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src2_reg,                                        \
                                                      TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(src2_reg)), OPSZ_16);          \
            instr_t *i4 = INSTR_CREATE_##OP(dcontext, op_spill_dst, op_src1, op_spill_src2);                           \
            instr_t *i5 =                                                                                              \
                SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_dst_reg, TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(dst_reg)), OPSZ_16); \
            instr_t *i6 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src2_reg,                                        \
                                                      TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_src2_reg)), OPSZ_16);    \
            instr_t *i7 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_dst_reg,                                         \
                                                      TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_dst_reg)), OPSZ_16);     \
            instrlist_concat_next_instr(ilist, 7, i1, i2, i3, i4, i5, i6, i7);                                         \
            return i1;                                                                                                 \
        } break;                                                                                                       \
                                                                                                                       \
        case 7: { /* all need spill */                                                                                 \
            if (src1_reg == dst_reg && src2_reg == dst_reg) {                                                          \
                reg_id_t spill_reg = XMM_SPILL_SLOT0;                                                                  \
                opnd_t op_spill = opnd_create_reg(spill_reg);                                                          \
                instr_t *i1 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_reg,                                              \
                                                     TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_reg)), OPSZ_16);          \
                instr_t *i2 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_reg,                                         \
                                                          TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(src1_reg)), OPSZ_16);      \
                instr_t *i3 = INSTR_CREATE_##OP(dcontext, op_spill, op_spill, op_spill);                               \
                instr_t *i4 =                                                                                          \
                    SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_reg, TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(dst_reg)), OPSZ_16); \
                instr_t *i5 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_reg,                                         \
                                                          TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_reg)), OPSZ_16);     \
                instrlist_concat_next_instr(ilist, 5, i1, i2, i3, i4, i5);                                             \
                return i1;                                                                                             \
            } else if (src1_reg == dst_reg) {                                                                          \
                reg_id_t spill_src1_reg = XMM_SPILL_SLOT0;                                                             \
                reg_id_t spill_src2_reg = XMM_SPILL_SLOT1;                                                             \
                reg_id_t spill_dst_reg = spill_src1_reg;                                                               \
                opnd_t op_spill_src1 = opnd_create_reg(spill_src1_reg);                                                \
                opnd_t op_spill_src2 = opnd_create_reg(spill_src2_reg);                                                \
                opnd_t op_spill_dst = opnd_create_reg(spill_dst_reg);                                                  \
                instr_t *i1 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_src1_reg,                                         \
                                                     TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_src1_reg)), OPSZ_16);     \
                instr_t *i2 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_src2_reg,                                         \
                                                     TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_src2_reg)), OPSZ_16);     \
                instr_t *i3 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src1_reg,                                    \
                                                          TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(src1_reg)), OPSZ_16);      \
                instr_t *i4 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src2_reg,                                    \
                                                          TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(src2_reg)), OPSZ_16);      \
                instr_t *i5 = INSTR_CREATE_##OP(dcontext, op_spill_dst, op_spill_src1, op_spill_src2);                 \
                instr_t *i6 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_dst_reg,                                          \
                                                     TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(dst_reg)), OPSZ_16);            \
                instr_t *i7 = RESTORE_SIMD_FROM_SIZED_TLS(                                                             \
                    dcontext, spill_src1_reg, TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_src1_reg)), OPSZ_16);            \
                instr_t *i8 = RESTORE_SIMD_FROM_SIZED_TLS(                                                             \
                    dcontext, spill_src2_reg, TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_src2_reg)), OPSZ_16);            \
                instrlist_concat_next_instr(ilist, 8, i1, i2, i3, i4, i5, i6, i7, i8);                                 \
                return i1;                                                                                             \
            } else if (src2_reg == dst_reg) {                                                                          \
                reg_id_t spill_src1_reg = XMM_SPILL_SLOT0;                                                             \
                reg_id_t spill_src2_reg = XMM_SPILL_SLOT1;                                                             \
                reg_id_t spill_dst_reg = spill_src2_reg;                                                               \
                opnd_t op_spill_src1 = opnd_create_reg(spill_src1_reg);                                                \
                opnd_t op_spill_src2 = opnd_create_reg(spill_src2_reg);                                                \
                opnd_t op_spill_dst = opnd_create_reg(spill_dst_reg);                                                  \
                instr_t *i1 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_src1_reg,                                         \
                                                     TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_src1_reg)), OPSZ_16);     \
                instr_t *i2 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_src2_reg,                                         \
                                                     TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_src2_reg)), OPSZ_16);     \
                instr_t *i3 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src1_reg,                                    \
                                                          TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(src1_reg)), OPSZ_16);      \
                instr_t *i4 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src2_reg,                                    \
                                                          TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(src2_reg)), OPSZ_16);      \
                instr_t *i5 = INSTR_CREATE_##OP(dcontext, op_spill_dst, op_spill_src1, op_spill_src2);                 \
                instr_t *i6 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_dst_reg,                                          \
                                                     TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(dst_reg)), OPSZ_16);            \
                instr_t *i7 = RESTORE_SIMD_FROM_SIZED_TLS(                                                             \
                    dcontext, spill_src1_reg, TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_src1_reg)), OPSZ_16);            \
                instr_t *i8 = RESTORE_SIMD_FROM_SIZED_TLS(                                                             \
                    dcontext, spill_src2_reg, TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_src2_reg)), OPSZ_16);            \
                instrlist_concat_next_instr(ilist, 8, i1, i2, i3, i4, i5, i6, i7, i8);                                 \
                return i1;                                                                                             \
            } else {                                                                                                   \
                reg_id_t spill_src1_reg = XMM_SPILL_SLOT0;                                                             \
                reg_id_t spill_src2_reg = XMM_SPILL_SLOT1;                                                             \
                reg_id_t spill_dst_reg = spill_src1_reg;                                                               \
                opnd_t op_spill_src1 = opnd_create_reg(spill_src1_reg);                                                \
                opnd_t op_spill_src2 = opnd_create_reg(spill_src2_reg);                                                \
                opnd_t op_spill_dst = opnd_create_reg(spill_dst_reg);                                                  \
                instr_t *i1 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_src1_reg,                                         \
                                                     TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_src1_reg)), OPSZ_16);     \
                instr_t *i2 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_src2_reg,                                         \
                                                     TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_src2_reg)), OPSZ_16);     \
                instr_t *i3 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src1_reg,                                    \
                                                          TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(src1_reg)), OPSZ_16);      \
                instr_t *i4 = RESTORE_SIMD_FROM_SIZED_TLS(dcontext, spill_src2_reg,                                    \
                                                          TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(src2_reg)), OPSZ_16);      \
                instr_t *i5 = INSTR_CREATE_##OP(dcontext, op_spill_dst, op_spill_src1, op_spill_src2);                 \
                instr_t *i6 = SAVE_SIMD_TO_SIZED_TLS(dcontext, spill_dst_reg,                                          \
                                                     TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(dst_reg)), OPSZ_16);            \
                instr_t *i7 = RESTORE_SIMD_FROM_SIZED_TLS(                                                             \
                    dcontext, spill_src1_reg, TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_src1_reg)), OPSZ_16);            \
                instr_t *i8 = RESTORE_SIMD_FROM_SIZED_TLS(                                                             \
                    dcontext, spill_src2_reg, TLS_ZMM_idx_SLOT(TO_XMM_REG_INDEX(spill_src2_reg)), OPSZ_16);            \
                instrlist_concat_next_instr(ilist, 8, i1, i2, i3, i4, i5, i6, i7, i8);                                 \
                return i1;                                                                                             \
            }                                                                                                          \
        } break;                                                                                                       \
                                                                                                                       \
        default: {                                                                                                     \
            REWRITE_INFO(STD_OUTF, #OP "_xmm_and_xmm pattern not support\n");                                          \
        } break;                                                                                                       \
        }                                                                                                              \
        return NULL_INSTR;                                                                                             \
    }

#endif /* _REWRITE_UTILS_H_ */
