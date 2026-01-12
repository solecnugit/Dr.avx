/**
 * @file rewrite_analysis.c
 * @author yue tang (ytang@stu.ecnu.edu.cn)
 *
 * @copyright Copyright (c) 2024
 *
 */

/* FIXME: perfermonce overhead unacceptable, don't use anymore */

#include "rewrite_analysis.h"
#include "opnd_api.h"

#define SET_MASK_LIVE(reg) (k_mask_liveness_bitmap |= (1 << ((reg) - DR_REG_K0)))
#define CLEAR_MASK_LIVE(reg) (k_mask_liveness_bitmap &= ~(1 << ((reg) - DR_REG_K0)))
#define IS_MASK_LIVE(reg) (k_mask_liveness_bitmap & (1 << ((reg) - DR_REG_K0)))
#define GET_MASK_LIVE(reg) ((k_mask_liveness_bitmap >> ((reg) - DR_REG_K0)) & 1)
#define MASK_LIVENESS_ALL_MASKS(reg) (k_mask_liveness_bitmap = 0xff)

#define SET_XMM_LIVE(reg) (xyzmm_liveness_bitmap |= (1 << ((reg) - DR_REG_XMM0)))
#define SET_YMM_LIVE(reg) (xyzmm_liveness_bitmap |= (1 << ((reg) - DR_REG_YMM0)))
#define SET_ZMM_LIVE(reg) (xyzmm_liveness_bitmap |= (1 << ((reg) - DR_REG_ZMM0)))
#define CLEAR_XMM_LIVE(reg) (xyzmm_liveness_bitmap &= ~(1 << ((reg) - DR_REG_XMM0)))
#define CLEAR_YMM_LIVE(reg) (xyzmm_liveness_bitmap &= ~(1 << ((reg) - DR_REG_YMM0)))
#define CLEAR_ZMM_LIVE(reg) (xyzmm_liveness_bitmap &= ~(1 << ((reg) - DR_REG_ZMM0)))
#define IS_XMM_LIVE(reg) (xyzmm_liveness_bitmap & (1 << ((reg) - DR_REG_XMM0)))
#define IS_YMM_LIVE(reg) (xyzmm_liveness_bitmap & (1 << ((reg) - DR_REG_YMM0)))
#define IS_ZMM_LIVE(reg) (xyzmm_liveness_bitmap & (1 << ((reg) - DR_REG_ZMM0)))
#define GET_XMM_LIVE(reg) ((xyzmm_liveness_bitmap >> ((reg) - DR_REG_XMM0)) & 1)
#define GET_YMM_LIVE(reg) ((xyzmm_liveness_bitmap >> ((reg) - DR_REG_YMM0)) & 1)
#define GET_ZMM_LIVE(reg) ((xyzmm_liveness_bitmap >> ((reg) - DR_REG_ZMM0)) & 1)

bool
instr_check_and_set_xmm_used(instr_t *instr)
{
    int idx;
    opnd_t opnd;
    reg_t reg;
    for (idx = 0; idx < instr_num_srcs(instr); idx++) {
        opnd = instr_get_src(instr, idx);
        if (opnd_is_reg(opnd)) {
            reg = opnd_get_reg(opnd);
            if (reg_is_strictly_xmm(reg)) {
                SET_XMM_LIVE(reg);
                return SET_XMM;
            }
        }
    }
    for (idx = 0; idx < instr_num_dsts(instr); idx++) {
        opnd = instr_get_dst(instr, idx);
        if (opnd_is_reg(opnd)) {
            reg = opnd_get_reg(opnd);
            if (reg_is_strictly_xmm(reg)) {
                SET_XMM_LIVE(reg);
                return SET_XMM;
            }
        }
    }
    return NO_XMM;
}

bool
instr_check_and_set_ymm_used(instr_t *instr)
{
    return NO_YMM;
}

bool
instr_check_and_set_zmm_used(instr_t *instr)
{
    return NO_ZMM;
}

bool
instr_may_read_write_opmask_register(instr_t *instr) // manully add same feat in each rewrite func is more effecient
{
    return false;
}
