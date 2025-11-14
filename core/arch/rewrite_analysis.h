/**
 * @file rewrite_analysis.h
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "rewrite_utils.h"

/**
 * @brief check xymm, mask regsiters liveness during each bb
 */
void
examine_avx512_reg_liveness(dcontext_t *dcontext, instrlist_t *ilist);

void
examine_xymm_livness(dcontext_t *dcontext, instrlist_t *ilist);

void
examine_mask_liveness(dcontext_t *dcontext, instrlist_t *ilist);


/** 8bits for k0~7 mask registers liveness records in current PC, 0 dead, 1 live. */
extern byte k_mask_liveness_bitmap; //FIXME: not thread safe
/** 32bits for x/ymm0~15/zmm0~31 registers liveness records in current PC, 0 dead, 1 live. */
extern uint xyzmm_liveness_bitmap; //FIXME: not thread safe

/* scan this on each bb is costly (TODO: check how costly it is) */

#define NO_XMM false
#define NO_YMM false
#define NO_ZMM false
#define SET_XMM true
#define SET_YMM true
#define SET_ZMM true

/**
 * @brief If current instr has xmm reg uesd, update xymm_liveness status 
 * 
 * @param instr 
 * @return true, if current instr has xmm reg uesd. 
 * @return false 
 */
bool
instr_check_and_set_xmm_used(instr_t* instr);

bool
instr_check_and_set_ymm_used(instr_t* instr);

bool
instr_check_and_set_zmm_used(instr_t* instr);

bool
instr_may_read_write_opmask_register(instr_t* instr); // manully add same feat in each rewrite func is more effecient
