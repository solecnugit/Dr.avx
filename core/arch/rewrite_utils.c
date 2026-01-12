/**
 * @file rewrite_utils.c
 *
 * @copyright Copyright (c) 2024
 *
 */

/*
 * rewrite_utils.c -- instr rewriting utility functions implementation
 */

#include "rewrite_utils.h"
#include "dr_ir_utils.h"
#include "globals_api.h"
#include "instr_api.h"
#include "instr_create_api.h"
#include "instrlist_api.h"
#include "opcode_api.h"
#include "opnd.h"
#include "opnd_api.h"
#include <sys/types.h>
#include <stdarg.h>

/* ======================================== *
 *  rewrite util functions implementation
 * ======================================== */

int
find_and_set_unused_ymm_pair(dcontext_t *dcontext, dr_ymm_pair_t *ymm_pair)
{
    uint found_num = 0;
    for (uint ymm_idx = 0; ymm_idx < YMM_REG_NUM; ymm_idx++) {
        if (!IS_YMM_USED(dcontext, ymm_idx)) {
            SET_YMM_USED(dcontext, ymm_idx);
            if (found_num == 0) {
                ymm_pair->ymm_lower = ymm_idx;
            } else {
                ymm_pair->ymm_upper = ymm_idx;
            }
            found_num++;
            if (found_num == 2) {
                return SUCCESS;
            }
        }
    }
    REWRITE_ERROR(STD_OUTF, "no empty ymm pair in 0~15 is available");
    return NOT_FIND;
}

reg_id_t
find_and_set_unused_ymm(dcontext_t *dcontext, reg_id_t ymm_reg)
{
    int ymm_idx = TO_YMM_REG_INDEX(ymm_reg);
    int mapping_ymm_idx = HIGH_TO_LOW_YMM_REG_ID_NUM(ymm_idx);
    if (IS_YMM_USED(dcontext, mapping_ymm_idx)) {
        for (uint idx = mapping_ymm_idx; idx >= 0; idx--) {
            if (!IS_YMM_USED(dcontext, idx)) {
                mapping_ymm_idx = idx;
                // add mapping relation to global mapping table
                ymm_to_ymm_mappings[ymm_idx].ymm_mapping_idx = idx;
                return TO_YMM_REG_ID_NUM(mapping_ymm_idx);
            }
        }
        // goes here means no empty ymm in 0~15 is available so use register replacement policy
        // TODO: add replacement policy here
        REWRITE_ERROR(STD_OUTF, "no empty ymm in 0~15 is available");
        return DR_REG_NULL;
    }
    return TO_YMM_REG_ID_NUM(mapping_ymm_idx);
}

reg_id_t
find_and_set_unused_xmm(dcontext_t *dcontext, reg_id_t xmm_reg)
{
    int xmm_idx = TO_XMM_REG_INDEX(xmm_reg);
    int mapping_xmm_idx = HIGH_TO_LOW_XMM_REG_ID_NUM(xmm_idx);
    // still use ymm used or not checking, because YMM and XMM are the same
    if (IS_YMM_USED(dcontext, mapping_xmm_idx)) {
        for (uint idx = mapping_xmm_idx; idx >= 0; idx--) {
            if (!IS_YMM_USED(dcontext, idx)) {
                mapping_xmm_idx = idx;
                return TO_XMM_REG_ID_NUM(mapping_xmm_idx);
            }
        }
        REWRITE_ERROR(STD_OUTF, "no empty xmm in 0~15 is available");
        return DR_REG_NULL;
    }
    return TO_XMM_REG_ID_NUM(mapping_xmm_idx);
}

opnd_t /* FIXME: need to modify the mapping mechnism */
create_mapping_ymm_opnd(dcontext_t *dcontext, reg_id_t ymm_reg)
{
    // if (IS_HIGH_YMM_REG(ymm_reg)) {
    //     reg_id_t mapping_ymm_reg = find_and_set_unused_ymm(dcontext, ymm_reg);
    //     return opnd_create_reg(mapping_ymm_reg);
    // }
    // if ymm_reg in (0, 15) directly create
    return opnd_create_reg(ymm_reg);
}

opnd_t /* FIXME: need to modify the mapping mechnism */
create_mapping_xmm_opnd(dcontext_t *dcontext, reg_id_t xmm_reg)
{
    // if (IS_HIGH_XMM_REG(xmm_reg)) {
    //     reg_id_t mapping_xmm_reg = find_and_set_unused_xmm(dcontext, xmm_reg);
    //     return opnd_create_reg(mapping_xmm_reg);
    // }
    return opnd_create_reg(xmm_reg);
}

/* ======================================== *
 *   rewrite util tls spill related funcs
 * ======================================== */

reg_id_t
find_one_available_spill_xmm(reg_id_t reg)
{
    /*
     * Scan the predefined XMM spill slots and pick the first slot not equal
     * to the register we must avoid. Returns DR_REG_NULL only if the table is
     * inconsistent.
     */
    for (reg_id_t idx_reg = XMM_SPILL_SLOT0; idx_reg <= XMM_SPILL_SLOTMAX; idx_reg++) {
        if (reg != idx_reg)
            return idx_reg;
    }
    REWRITE_ERROR(STD_OUTF, "find_one_available_spill_xmm functionality wrong");
    return DR_REG_NULL;
}

reg_id_t
find_available_spill_xmm_avoiding(reg_id_t reg_to_avoid1, reg_id_t reg_to_avoid2, reg_id_t reg_to_avoid3)
{
    /* Try all spill slots until we find one that avoids all specified registers. */
    // Try all spill slots until we find one that doesn't conflict with any of the registers to avoid
    for (reg_id_t candidate = XMM_SPILL_SLOT0; candidate <= XMM_SPILL_SLOTMAX; candidate++) {
        if (candidate != reg_to_avoid1 && (reg_to_avoid2 == DR_REG_NULL || candidate != reg_to_avoid2) &&
            (reg_to_avoid3 == DR_REG_NULL || candidate != reg_to_avoid3)) {
            return candidate;
        }
    }

    // If we couldn't find a non-conflicting register (should be impossible with 6 slots and max 3 conflicts)
    REWRITE_ERROR(STD_OUTF, "find_available_spill_xmm_avoiding couldn't find a non-conflicting register");
    return DR_REG_NULL;
}

reg_id_t
find_available_spill_xmm_avoiding_variadic(int num_regs, ...)
{
    va_list args;
    va_start(args, num_regs);

    for (reg_id_t candidate = XMM_SPILL_SLOT0; candidate <= XMM_SPILL_SLOTMAX; candidate++) {
        bool is_conflict = false;
        va_list args_copy;
        va_copy(args_copy, args);

        for (int i = 0; i < num_regs; i++) {
            reg_id_t reg_to_avoid = (reg_id_t)va_arg(args_copy, int);
            if (reg_to_avoid != DR_REG_NULL && candidate == reg_to_avoid) {
                is_conflict = true;
                break;
            }
        }

        va_end(args_copy);
        if (!is_conflict) {
            va_end(args);
            return candidate;
        }
    }

    va_end(args);
    REWRITE_ERROR(STD_OUTF, "find_available_spill_xmm_avoiding_variadic couldn't find a non-conflicting register");
    return DR_REG_NULL;
}

reg_id_t
find_one_available_spill_ymm(reg_id_t reg)
{
    /* Scan the predefined YMM spill slots and pick the first slot not equal to the avoid register. */
    for (reg_id_t idx_reg = YMM_SPILL_SLOT0; idx_reg <= YMM_SPILL_SLOTMAX; idx_reg++) {
        if (reg != idx_reg)
            return idx_reg;
    }
    REWRITE_ERROR(STD_OUTF, "find_one_available_spill_ymm functionality wrong");
    return DR_REG_NULL;
}

reg_id_t
find_available_spill_ymm_avoiding(reg_id_t reg_to_avoid1, reg_id_t reg_to_avoid2, reg_id_t reg_to_avoid3)
{
    /* Try all spill slots until we find one that avoids all specified registers. */
    // Try all spill slots until we find one that doesn't conflict with any of the registers to avoid
    for (reg_id_t candidate = YMM_SPILL_SLOT0; candidate <= YMM_SPILL_SLOTMAX; candidate++) {
        if (candidate != reg_to_avoid1 && (reg_to_avoid2 == DR_REG_NULL || candidate != reg_to_avoid2) &&
            (reg_to_avoid3 == DR_REG_NULL || candidate != reg_to_avoid3)) {
            return candidate;
        }
    }

    // If we couldn't find a non-conflicting register (should be impossible with 6 slots and max 3 conflicts)
    REWRITE_ERROR(STD_OUTF, "find_available_spill_ymm_avoiding couldn't find a non-conflicting register");
    return DR_REG_NULL;
}

reg_id_t
find_available_spill_ymm_avoiding_variadic(int num_regs, ...)
{
    va_list args;
    va_start(args, num_regs);

    for (reg_id_t candidate = YMM_SPILL_SLOT0; candidate <= YMM_SPILL_SLOTMAX; candidate++) {
        bool is_conflict = false;
        va_list args_copy;
        va_copy(args_copy, args);

        for (int i = 0; i < num_regs; i++) {
            reg_id_t reg_to_avoid = (reg_id_t)va_arg(args_copy, int);
            if (reg_to_avoid != DR_REG_NULL && candidate == reg_to_avoid) {
                is_conflict = true;
                break;
            }
        }

        va_end(args_copy);
        if (!is_conflict) {
            va_end(args);
            return candidate;
        }
    }

    va_end(args);
    REWRITE_ERROR(STD_OUTF, "find_available_spill_ymm_avoiding_variadic couldn't find a non-conflicting register");
    return DR_REG_NULL;
}

reg_id_t
find_one_available_spill_zmm(reg_id_t reg)
{
    /* Scan the predefined ZMM spill slots and pick the first slot not equal to the avoid register. */
    for (reg_id_t idx_reg = ZMM_SPILL_SLOT0; idx_reg <= ZMM_SPILL_SLOTMAX; idx_reg++) {
        if (reg != idx_reg)
            return idx_reg;
    }
    REWRITE_ERROR(STD_OUTF, "find_one_available_spill_zmm functionality wrong");
    return DR_REG_NULL;
}

reg_id_t
find_available_spill_zmm_avoiding(reg_id_t reg_to_avoid1, reg_id_t reg_to_avoid2, reg_id_t reg_to_avoid3)
{
    /* Try all spill slots until we find one that avoids all specified registers. */
    // Try all spill slots until we find one that doesn't conflict with any of the registers to avoid
    for (reg_id_t candidate = ZMM_SPILL_SLOT0; candidate <= ZMM_SPILL_SLOTMAX; candidate++) {
        if (candidate != reg_to_avoid1 && (reg_to_avoid2 == DR_REG_NULL || candidate != reg_to_avoid2) &&
            (reg_to_avoid3 == DR_REG_NULL || candidate != reg_to_avoid3)) {
            return candidate;
        }
    }

    // If we couldn't find a non-conflicting register (should be impossible with 6 slots and max 3 conflicts)
    REWRITE_ERROR(STD_OUTF, "find_available_spill_zmm_avoiding couldn't find a non-conflicting register");
    return DR_REG_NULL;
}

spill_reg_pair_t
find_two_available_spill_xmms(reg_id_t reg1, reg_id_t reg2)
{
    spill_reg_pair_t spill_pair;
    // Start from first spill slot and find two available slots
    for (reg_id_t idx_reg = XMM_SPILL_SLOT0; idx_reg <= XMM_SPILL_SLOTMAX; idx_reg++) {
        // Skip if current slot matches either input reg
        if (idx_reg == reg1 || idx_reg == reg2)
            continue;

        spill_pair.reg1 = idx_reg;
        // Find second available slot starting from next position
        for (reg_id_t idx_reg2 = idx_reg + 1; idx_reg2 <= XMM_SPILL_SLOTMAX; idx_reg2++) {
            if (idx_reg2 != reg1 && idx_reg2 != reg2) {
                spill_pair.reg2 = idx_reg2;
                return spill_pair;
            }
        }
    }

    // Should never reach here since we have 6 spill slots and at most 2 are occupied
    REWRITE_ERROR(STD_OUTF, "find_two_available_spill_xmms functionality wrong");
    spill_pair.reg1 = DR_REG_NULL;
    spill_pair.reg2 = DR_REG_NULL;
    return spill_pair;
}

spill_reg_pair_t
find_two_available_spill_ymms(reg_id_t reg1, reg_id_t reg2)
{
    spill_reg_pair_t spill_pair;
    // Start from first spill slot and find two available slots
    for (reg_id_t idx_reg = YMM_SPILL_SLOT0; idx_reg <= YMM_SPILL_SLOTMAX; idx_reg++) {
        // Skip if current slot matches either input reg
        if (idx_reg == reg1 || idx_reg == reg2)
            continue;

        spill_pair.reg1 = idx_reg;
        // Find second available slot starting from next position
        for (reg_id_t idx_reg2 = idx_reg + 1; idx_reg2 <= YMM_SPILL_SLOTMAX; idx_reg2++) {
            if (idx_reg2 != reg1 && idx_reg2 != reg2) {
                spill_pair.reg2 = idx_reg2;
                return spill_pair;
            }
        }
    }

    // Should never reach here since we have 6 spill slots and at most 2 are occupied
    REWRITE_ERROR(STD_OUTF, "find_two_available_ymms functionality wrong");
    spill_pair.reg1 = DR_REG_NULL;
    spill_pair.reg2 = DR_REG_NULL;
    return spill_pair;
}

spill_reg_pair_t
find_two_available_spill_zmms(reg_id_t reg1, reg_id_t reg2)
{
    spill_reg_pair_t spill_pair;
    // Start from first spill slot and find two available slots
    for (reg_id_t idx_reg = ZMM_SPILL_SLOT0; idx_reg <= ZMM_SPILL_SLOTMAX; idx_reg++) {
        // Skip if current slot matches either input reg
        if (idx_reg == reg1 || idx_reg == reg2)
            continue;

        spill_pair.reg1 = idx_reg;
        // Find second available slot starting from next position
        for (reg_id_t idx_reg2 = idx_reg + 1; idx_reg2 <= ZMM_SPILL_SLOTMAX; idx_reg2++) {
            if (idx_reg2 != reg1 && idx_reg2 != reg2) {
                spill_pair.reg2 = idx_reg2;
                return spill_pair;
            }
        }
    }

    // Should never reach here since we have 6 spill slots and at most 2 are occupied
    REWRITE_ERROR(STD_OUTF, "find_two_available_spill_zmms functionality wrong");
    spill_pair.reg1 = DR_REG_NULL;
    spill_pair.reg2 = DR_REG_NULL;
    return spill_pair;
}

instr_t *
instr_create_load_tls_base(dcontext_t *dcontext)
{
    // mov %gs:tls_base_ymm_offset -> target_reg (target reg is TLS_RAX_SLOT)
    instr_t *new_instr2 = INSTR_CREATE_mov_ld(dcontext, opnd_create_reg(DR_REG_RAX),
                                              opnd_create_tls_slot(os_tls_offset(TLS_ZMM_idx_SLOT(0))));
    return new_instr2;
}

void
save_simd_to_tls(dcontext_t *dcontext, instrlist_t *ilist, instr_t *first_avx512_instr_next)
{
    // save ymm0-ymm15 to tls
    for (uint i = 0; i < YMM_REG_NUM; i++) {
        instrlist_meta_preinsert(ilist, first_avx512_instr_next,
                                 SAVE_SIMD_TO_SIZED_TLS(dcontext, DR_REG_YMM0 + i, TLS_ZMM_idx_SLOT(i), OPSZ_32));
    }
}

void
restore_simd_from_tls(dcontext_t *dcontext, instrlist_t *ilist, instr_t *last_avx512_instr_next)
{
    // restore ymm0-ymm15 from tls
    for (uint i = 0; i < YMM_REG_NUM; i++) {
        instrlist_meta_preinsert(ilist, last_avx512_instr_next,
                                 RESTORE_SIMD_FROM_SIZED_TLS(dcontext, DR_REG_YMM0 + i, TLS_ZMM_idx_SLOT(i), OPSZ_32));
    }
}

/* ======================================== *
 *   rewrite util zmm to ymm pair mapping
 * ======================================== */

void
init_zmm_to_ymm_pair_mapping()
{
    for (uint zmm_idx = 0; zmm_idx < MCXT_NUM_SIMD_SLOTS; zmm_idx++) {
        zmm_to_ymm_pair_mappings[zmm_idx].zmm_idx = EMPTY;
        zmm_to_ymm_pair_mappings[zmm_idx].ymm_pair.ymm_lower = EMPTY;
        zmm_to_ymm_pair_mappings[zmm_idx].ymm_pair.ymm_upper = EMPTY;
    }
}

int
add_zmm_to_ymm_pair_mapping(int zmm_idx, dr_ymm_pair_t *ymm_pair)
{
    if (zmm_idx < 0 || zmm_idx >= MCXT_NUM_SIMD_SLOTS) {
        REWRITE_ERROR(STD_ERRF, "zmm_idx{%d} in zmm_to_ymm_pair_mapping out of bounds\n", zmm_idx);
    }
    zmm_to_ymm_pair_mappings[zmm_idx].zmm_idx = zmm_idx;
    zmm_to_ymm_pair_mappings[zmm_idx].ymm_pair.ymm_lower = ymm_pair->ymm_lower;
    zmm_to_ymm_pair_mappings[zmm_idx].ymm_pair.ymm_upper = ymm_pair->ymm_upper;
    return SUCCESS;
}

int
get_zmm_to_ymm_pair_mapping(int zmm_idx, dr_ymm_pair_t *ymm_pair)
{
    if (zmm_idx < 0 || zmm_idx >= MCXT_NUM_SIMD_SLOTS) {
        REWRITE_ERROR(STD_ERRF, "zmm_idx{%d} in zmm_to_ymm_pair_mapping out of bounds\n", zmm_idx);
    }
    uint ymm_lower_idx = zmm_to_ymm_pair_mappings[zmm_idx].ymm_pair.ymm_lower;
    uint ymm_upper_idx = zmm_to_ymm_pair_mappings[zmm_idx].ymm_pair.ymm_upper;
    if (ymm_lower_idx == EMPTY || ymm_upper_idx == EMPTY) {
        return NOT_GET;
    }
    ymm_pair->ymm_lower = ymm_lower_idx;
    ymm_pair->ymm_upper = ymm_upper_idx;
    return SUCCESS;
}

/* ======================================== *
 *     GPR spill register selectors (x86-64)
 * ======================================== */

static const reg_id_t GPR_SPILL_SLOTS[GPR_SPILL_SLOT_NUM] = {
    GPR_SPILL_SLOT0,  GPR_SPILL_SLOT1,  GPR_SPILL_SLOT2,  GPR_SPILL_SLOT3,
    GPR_SPILL_SLOT4,  GPR_SPILL_SLOT5,  GPR_SPILL_SLOT6,  GPR_SPILL_SLOT7,
    GPR_SPILL_SLOT8,  GPR_SPILL_SLOT9,  GPR_SPILL_SLOT10, GPR_SPILL_SLOT11,
    GPR_SPILL_SLOT12, GPR_SPILL_SLOT13
};

static inline int reg_to_bit(reg_id_t reg) {
    // Assume register IDs are sequential starting from RAX
    if (reg >= DR_REG_RAX && reg <= DR_REG_R15) {
        return reg - DR_REG_RAX;
    }
    return -1; // Invalid register
}



int
find_available_spill_gprs_avoiding_outptrs(int needed, int num_avoids, ...)
{
    // Fast path: early exit for invalid requests
    if (needed <= 0)
        return 0;
    
    if (needed > GPR_SPILL_SLOT_NUM) {
        REWRITE_ERROR(STD_OUTF, "requested %d GPR spill regs, but only %d available", needed, GPR_SPILL_SLOT_NUM);
        return 0;
    }

    // Use bitmask for O(1) avoidance checking instead of O(n) linear search
    uint64 avoid_mask = 0;
    reg_id_t *out_ptrs[GPR_SPILL_SLOT_NUM];
    int out_ptr_count = 0;

    va_list args;
    va_start(args, num_avoids);

    // Collect output pointers (unrolled for common cases)
    switch (needed) {
    case 1:
        out_ptrs[0] = va_arg(args, reg_id_t *);
        out_ptr_count = (out_ptrs[0] != NULL) ? 1 : 0;
        break;
    case 2:
        out_ptrs[0] = va_arg(args, reg_id_t *);
        out_ptrs[1] = va_arg(args, reg_id_t *);
        out_ptr_count = 0;
        if (out_ptrs[0] != NULL) out_ptr_count++;
        if (out_ptrs[1] != NULL) out_ptr_count++;
        break;
    case 3:
        out_ptrs[0] = va_arg(args, reg_id_t *);
        out_ptrs[1] = va_arg(args, reg_id_t *);
        out_ptrs[2] = va_arg(args, reg_id_t *);
        out_ptr_count = 0;
        if (out_ptrs[0] != NULL) out_ptr_count++;
        if (out_ptrs[1] != NULL) out_ptr_count++;
        if (out_ptrs[2] != NULL) out_ptr_count++;
        break;
    default:
        // General case for larger numbers
        for (int i = 0; i < needed; i++) {
            reg_id_t *p = va_arg(args, reg_id_t *);
            if (p != NULL) {
                out_ptrs[out_ptr_count++] = p;
            }
        }
        break;
    }

    // Build avoidance bitmask - convert registers to bit positions
    const int clamp_avoids = (num_avoids < MAX_SPILL_AVOIDS) ? num_avoids : MAX_SPILL_AVOIDS;
    for (int i = 0; i < clamp_avoids; i++) {
        reg_id_t reg_to_avoid = (reg_id_t)va_arg(args, int);
        if (reg_to_avoid != DR_REG_NULL) {
            int bit_pos = reg_to_bit(reg_to_avoid);
            if (bit_pos >= 0 && bit_pos < 64) {
                avoid_mask |= (1ULL << bit_pos);
            }
        }
    }
    va_end(args);

    // Fast path: if no avoidance needed, just assign first N registers
    if (avoid_mask == 0) {
        const int assign_count = (needed < out_ptr_count) ? needed : out_ptr_count;
        for (int i = 0; i < assign_count; i++) {
            *out_ptrs[i] = GPR_SPILL_SLOTS[i];
        }
        return needed;
    }

    // Main selection loop - optimized with bitmask lookup
    int selected_count = 0;
    for (int i = 0; i < GPR_SPILL_SLOT_NUM && selected_count < needed; i++) {
        const reg_id_t candidate = GPR_SPILL_SLOTS[i];
        const int bit_pos = reg_to_bit(candidate);
        
        // O(1) avoidance check using bitmask
        if (bit_pos >= 0 && (avoid_mask & (1ULL << bit_pos)) == 0) {
            // Register is available - assign if we have output pointer
            if (selected_count < out_ptr_count) {
                *out_ptrs[selected_count] = candidate;
            }
            selected_count++;
        }
    }

    if (selected_count < needed) {
        REWRITE_DEBUG(STD_OUTF, "requested %d GPR spill regs, only found %d", needed, selected_count);
    }
    
    return selected_count;
}

reg_id_t
find_one_available_spill_gpr_avoiding_variadic(int num_avoids, ...)
{
    reg_id_t result = DR_REG_NULL;

    /* Materialize avoids to reuse the multi-select logic cleanly. */
    reg_id_t out_regs[1] = { DR_REG_NULL };

    reg_id_t avoid_buffer[MAX_SPILL_AVOIDS];
    int avoid_count = 0;

    va_list args;
    va_start(args, num_avoids);
    for (int i = 0; i < num_avoids && avoid_count < (int)MAX_SPILL_AVOIDS; i++) {
        reg_id_t reg_to_avoid = (reg_id_t)va_arg(args, int);
        if (reg_to_avoid != DR_REG_NULL)
            avoid_buffer[avoid_count++] = reg_to_avoid;
    }
    va_end(args);

    /* Now call the core routine using the collected avoids. */
    /* We need a variadic call signature; reuse the same order. */
    switch (avoid_count) {
    case 0:
        if (find_available_spill_gprs_avoiding_outptrs(1, 0, &out_regs[0]) == 1)
            result = out_regs[0];
        break;
    case 1:
        if (find_available_spill_gprs_avoiding_outptrs(1, 1, &out_regs[0], avoid_buffer[0]) == 1)
            result = out_regs[0];
        break;
    case 2:
        if (find_available_spill_gprs_avoiding_outptrs(1, 2, &out_regs[0], avoid_buffer[0], avoid_buffer[1]) == 1)
            result = out_regs[0];
        break;
    case 3:
        if (find_available_spill_gprs_avoiding_outptrs(1, 3, &out_regs[0], avoid_buffer[0], avoid_buffer[1], avoid_buffer[2]) == 1)
            result = out_regs[0];
        break;
    case 4:
        if (find_available_spill_gprs_avoiding_outptrs(1, 4, &out_regs[0], avoid_buffer[0], avoid_buffer[1], avoid_buffer[2], avoid_buffer[3]) == 1)
            result = out_regs[0];
        break;
    default:
        /* Clamp to MAX_SPILL_AVOIDS to maintain a small variadic surface. */
        if (find_available_spill_gprs_avoiding_outptrs(1, MAX_SPILL_AVOIDS,
                                                        &out_regs[0],
                                                        avoid_buffer[0], avoid_buffer[1], avoid_buffer[2],
                                                        avoid_buffer[3], avoid_buffer[4]) == 1)
            result = out_regs[0];
        break;
    }

    if (result == DR_REG_NULL) {
        REWRITE_ERROR(STD_OUTF, "find_one_available_spill_gpr_avoiding_variadic could not find a register");
    }
    return result;
}

/* ======================================== *
 *      Debug type build function
 * ======================================== */

#ifdef DEBUG

void
print_file_zmm_to_ymm_pair_mapping(int zmm_idx)
{
    // if (zmm_idx < 0 || zmm_idx >= MCXT_NUM_SIMD_SLOTS) {
    //     print_file(STD_ERRF, "[ERROR]: zmm_idx{%d} in zmm_to_ymm_pair_mapping out of bounds\n", zmm_idx);
    // }
    ASSERT(zmm_idx >= 0 && zmm_idx < MCXT_NUM_SIMD_SLOTS);
    REWRITE_INFO(STD_OUTF, "zmm%d~(ymm%d, ymm%d)\n", zmm_idx, zmm_to_ymm_pair_mappings[zmm_idx].ymm_pair.ymm_lower,
                 zmm_to_ymm_pair_mappings[zmm_idx].ymm_pair.ymm_upper);
}

#    define print_file_multiple_zmm_mappings(...)                              \
        do {                                                                   \
            reg_id_t regs[] = { __VA_ARGS__ };                                 \
            for (size_t i = 0; i < (sizeof(regs) / sizeof(regs[0])); i++) {    \
                print_file_zmm_to_ymm_pair_mapping(TO_ZMM_REG_INDEX(regs[i])); \
            }                                                                  \
        } while (0)

void
print_file_ymm_to_ymm_mapping(int ymm_idx)
{
    ASSERT(ymm_idx >= 16 && ymm_idx < MCXT_NUM_SIMD_SLOTS);
    REWRITE_INFO(STD_OUTF, "ymm%d~ymm%d\n", ymm_idx, ymm_to_ymm_mappings[ymm_idx].ymm_mapping_idx);
}

void
print_rewrite_info(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start, const char *instr_name,
                   bool has_mask, bool has_src1, bool has_src2, bool has_dst)
{
    REWRITE_INFO(STD_OUTF, "==== Rewriting %s at %p ====", instr_name, instr_start);
    instr_disassemble(dcontext, instr, STD_OUTF);
    NEWLINE(STD_OUTF);
    if (has_mask) {
        opnd_t mask_opnd = instr_get_src(instr, 0);
        dr_print_opnd(dcontext, STD_OUTF, mask_opnd, "  mask:");
        if (is_avx512_zero_mask(instr)) {
            print_file(STD_OUTF, "  mask is zero: .Z\n");
        }
    }
    if (has_src1) {
        opnd_t src1_opnd = instr_get_src(instr, 1);
        dr_print_opnd(dcontext, STD_OUTF, src1_opnd, "  src1:");
    }
    if (has_src2) {
        opnd_t src2_opnd = instr_get_src(instr, 2);
        dr_print_opnd(dcontext, STD_OUTF, src2_opnd, "  src2:");
    }
    if (has_dst) {
        opnd_t dst_opnd = instr_get_dst(instr, 0);
        dr_print_opnd(dcontext, STD_OUTF, dst_opnd, "  dst:");
    }
}

void
print_rewrite_one_instr(dcontext_t *dcontext, instr_t *instr1, bool has_zmm_reg, reg_id_t *src_reg, reg_id_t *dst_reg)
{
    instr_disassemble(dcontext, instr1, STD_OUTF);
    NEWLINE(STD_OUTF);
    if (has_zmm_reg) {
        if (src_reg)
            print_file_zmm_to_ymm_pair_mapping(TO_ZMM_REG_INDEX(*src_reg));
        if (dst_reg)
            print_file_zmm_to_ymm_pair_mapping(TO_ZMM_REG_INDEX(*dst_reg));
    }
}

void
print_rewrite_two_instr(dcontext_t *dcontext, instr_t *instr1, instr_t *instr2, bool has_zmm_reg, reg_id_t *src_reg,
                        reg_id_t *dst_reg)
{
    instr_disassemble(dcontext, instr1, STD_OUTF);
    NEWLINE(STD_OUTF);
    instr_disassemble(dcontext, instr2, STD_OUTF);
    NEWLINE(STD_OUTF);
    if (has_zmm_reg) {
        if (src_reg)
            print_file_zmm_to_ymm_pair_mapping(TO_ZMM_REG_INDEX(*src_reg));
        if (dst_reg)
            print_file_zmm_to_ymm_pair_mapping(TO_ZMM_REG_INDEX(*dst_reg));
    }
}

void
print_rewrite_three_instr(dcontext_t *dcontext, instr_t *instr1, instr_t *instr2, instr_t *instr3)
{
    instr_disassemble(dcontext, instr1, STD_OUTF);
    NEWLINE(STD_OUTF);
    instr_disassemble(dcontext, instr2, STD_OUTF);
    NEWLINE(STD_OUTF);
    instr_disassemble(dcontext, instr3, STD_OUTF);
    NEWLINE(STD_OUTF);
}

void
print_rewrite_variadic_instr(dcontext_t *dcontext, int count, ...)
{
    va_list args;
    va_start(args, count);

    REWRITE_DEBUG(STD_OUTF, "==== INSTRUCTION SEQUENCE ====");

    for (int i = 0; i < count; i++) {
        instr_t *instr = va_arg(args, instr_t *);
        instr_disassemble(dcontext, instr, STD_OUTF);
        NEWLINE(STD_OUTF);
    }

    REWRITE_DEBUG(STD_OUTF, "==============================");
    NEWLINE(STD_OUTF);
    va_end(args);
}

void
print_zmm_in_dcontext(dcontext_t *dcontext)
{
    priv_mcontext_t *mc = get_mcontext(dcontext);
    // Header
    REWRITE_INFO(STD_OUTF, "-----------------------------");
    // Print SIMD slots (zmm registers)
    for (int zmm_idx = 0; zmm_idx < MCXT_NUM_SIMD_SLOTS; zmm_idx++) {
        REWRITE_INFO(STD_OUTF, "zmm%d: %p", zmm_idx, mc->simd[zmm_idx]);
        // Alternate between newline and tab
        REWRITE_INFO(STD_OUTF, (zmm_idx % 2 == 1) ? "\n" : "\t");
    }
    // Ensure ending with a newline for better formatting
    NEWLINE(STD_OUTF);
    // Print opmask slots (k registers)
    for (int k_idx = 0; k_idx < MCXT_NUM_OPMASK_SLOTS; k_idx++) {
        REWRITE_INFO(STD_OUTF, "k%d: %p\t", k_idx, mc->opmask[k_idx]);
        // Add a newline every 4 items for better readability
        if ((k_idx + 1) % 4 == 0) {
            REWRITE_INFO(STD_OUTF, "\n");
        }
    }
    // Ensure ending with a newline for better formatting
    NEWLINE(STD_OUTF);
    // Footer
    REWRITE_INFO(STD_OUTF, "-----------------------------");
}

void
print_tls_avx512_regs(dcontext_t *dcontext)
{
    spill_state_t *state = &dcontext->local_state->spill_space;
    // Header
    REWRITE_INFO(STD_OUTF, "-----------------------------");
    // Print SIMD slots (zmm registers)
    for (int zmm_idx = 0; zmm_idx < MCXT_NUM_SIMD_SLOTS; zmm_idx++) {
        REWRITE_INFO(STD_OUTF, "zmm%d: %p", zmm_idx, state->zmm_regs[zmm_idx]);
        // Alternate between newline and tab
        REWRITE_INFO(STD_OUTF, (zmm_idx % 2 == 1) ? "\n" : "\t");
    }
    // Print opmask slots (k registers)
    for (int k_idx = 0; k_idx < MCXT_NUM_OPMASK_SLOTS; k_idx++) {
        REWRITE_INFO(STD_OUTF, "k%d: %p\t", k_idx, state->k_regs[k_idx]);
        // Add a newline every 4 items for better readability
        if ((k_idx + 1) % 4 == 0) {
            REWRITE_INFO(STD_OUTF, "\n");
        }
    }
    // Ensure ending with a newline for better formatting
    NEWLINE(STD_OUTF);
    // Footer
    REWRITE_INFO(STD_OUTF, "-----------------------------");
}

#endif /* DEBUG */

/* ======================================== *
 *    mask register simulation util funcs
 * ======================================== */

instr_t *
rewrite_save_rax_reg(dcontext_t *dcontext, instrlist_t *ilist)
{
    ushort offs =
        os_tls_offset(TLS_REG0_SLOT); // hard code it to the TLS_REGO_SLOT, which corresponds to rax in x86,x64
    instr_t *save_rax_instr = INSTR_CREATE_mov_ld(dcontext, opnd_create_tls_slot(offs), opnd_create_reg(DR_REG_RAX));
    instr_set_meta(save_rax_instr);
    instrlist_append(ilist, save_rax_instr);
    return save_rax_instr;
}

instr_t *
rewrite_restore_rax_reg(dcontext_t *dcontext)
{
    ushort offs = os_tls_offset(TLS_REG0_SLOT);
    instr_t *restore_rax_instr = INSTR_CREATE_mov_st(dcontext, opnd_create_reg(DR_REG_RAX), opnd_create_tls_slot(offs));
    instr_set_meta(restore_rax_instr);
    return restore_rax_instr;
}

void
instrlist_append_variadic_instr(dcontext_t *dcontext, instrlist_t *ilist, int num_instr, ...)
{
    va_list args;
    va_start(args, num_instr);

    for (int i = 0; i < num_instr; i++) {
        instr_t *instr = va_arg(args, instr_t *);
        instrlist_append(ilist, instr);
    }

    va_end(args);
}

void /* TODO: why use frist argument `instrlist_t`? it's not used, FIXME: should removed */
instrlist_concat_next_instr(instrlist_t *ilist, int num_instr, ...)
{
    va_list args;
    va_start(args, num_instr);

    if (num_instr < 2) {
        va_end(args);
        return;
    }

    instr_t *prev_instr = va_arg(args, instr_t *);
    for (int i = 1; i < num_instr; i++) {
        instr_t *next_instr = va_arg(args, instr_t *);
        instr_concat_next(prev_instr, next_instr);
        prev_instr = next_instr;
    }

    va_end(args);
}

bool
is_avx512_zero_mask(instr_t *instr)
{
    // check if the mask is zero
    uint prefixes = instr_get_prefixes(instr);
    return TEST(0x000800000, prefixes);
}

