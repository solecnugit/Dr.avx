/**
 * @file rewrite.h
 * @author yue tang (ytang@stu.ecnu.edu.cn)
 *
 * @copyright Copyright (c) 2024
 */

/*
 * rewrite.h -- instr rewriting utilities and helper functions
 */

#ifndef _REWRITE_H_
#define _REWRITE_H_ #include "globals_api.h"

#include "rewrite_utils.h"

/* ======================================== *
 * rewrite functions signature
 * ======================================== */

instr_t * /* 0 */
rw_func_empty(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* invalid */
rw_func_invalid(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* invalid */
rw_func_invalid(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 1 */
rw_func_vmovss(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 2 */
rw_func_vmovsd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 3 */
rw_func_vmovups(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 4 */
rw_func_vmovupd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 5 */
rw_func_vmovlps(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 6 */
rw_func_vmovsldup(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 7 */
rw_func_vmovlpd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 8 */
rw_func_vmovddup(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 9 */
rw_func_vunpcklps(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 10 */
rw_func_vunpcklpd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 11 */
rw_func_vunpckhps(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 12 */
rw_func_vunpckhpd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 13 */
rw_func_vmovhps(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 14 */
rw_func_vmovshdup(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 15 */
rw_func_vmovhpd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 16 */
rw_func_vmovaps(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 17 */
rw_func_vmovapd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 18 */
rw_func_vcvtsi2ss(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 19 */
rw_func_vcvtsi2sd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 20 */
rw_func_vmovntps(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 21 */
rw_func_vmovntpd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 22 */
rw_func_vcvttss2si(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 23 */
rw_func_vcvttsd2si(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 24 */
rw_func_vcvtss2si(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 25 */
rw_func_vcvtsd2si(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 26 */
rw_func_vucomiss(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 27 */
rw_func_vucomisd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 28 */
rw_func_vcomiss(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 29 */
rw_func_vcomisd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 30 */
rw_func_vmovmskps(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 31 */
rw_func_vmovmskpd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 32 */
rw_func_vsqrtps(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 33 */
rw_func_vsqrtss(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 34 */
rw_func_vsqrtpd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 35 */
rw_func_vsqrtsd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 81 */
rw_func_vpackuswb(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 100 */
rw_func_vmovq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 111 */
rw_func_vpsrlq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 112 */
rw_func_vpaddq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 125 */
rw_func_vpsrad(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 141 */
rw_func_vpsllw(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 142 */
rw_func_vpslld(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 143 */
rw_func_vpsllq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 148 */
rw_func_vpsubb(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 149 */
rw_func_vpsubw(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 150 */
rw_func_vpsubd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 150 */
rw_func_vpsubq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 153 */
rw_func_vpaddw(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 154 */
rw_func_vpaddd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 155 */
rw_func_vpsrldq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 189 */
rw_func_vpmovsxwd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 191 */
rw_func_vpmovsxdq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 196 */
rw_func_vpmovzxbw(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 197 */
rw_func_vpmovzxbd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 198 */
rw_func_vpmovzxbq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 199 */
rw_func_vpmovzxwd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 200 */
rw_func_vpmovzxwq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 201 */
rw_func_vpmovzxdq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 211 */
rw_func_vpmulld(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 440 */
rw_func_vpgatherdd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 441 */
rw_func_vpgatherdq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 442 */
rw_func_vpgatherqd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 443 */
rw_func_vpgatherqq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 464 ~ 467 vpbroadcast{b,w,d,q} template function */
rw_func_vpbroadcast_(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start,
                     const char *instr_name);

instr_t * /* 464 */
rw_func_vpbroadcastb(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 465 */
rw_func_vpbroadcastw(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 466 */
rw_func_vpbroadcastd(dcontext_t *dcontext, instrlist_t *ilitst, instr_t *instr, app_pc instr_start);

instr_t * /* 467 */
rw_func_vpbroadcastq(dcontext_t *dcontext, instrlist_t *ilitst, instr_t *instr, app_pc instr_start);

/*==========================================
 * k regs manipulation instructions start
 *=========================================*/

instr_t * /* 472 */
rw_func_kmovw(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 473 */
rw_func_kmovb(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 474 */
rw_func_kmovq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 475 */
rw_func_kmovd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 476 */
rw_func_kandw(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 477 */
rw_func_kandb(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 478 */
rw_func_kandq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 479 */
rw_func_kandd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 480 */
rw_func_kandnw(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 481 */
rw_func_kandnb(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 482 */
rw_func_kandnq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 483 */
rw_func_kandnd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 484 */
rw_func_kunpckbw(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 485 */
rw_func_kunpckwd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 486 */
rw_func_kunpckdq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 487 */
rw_func_knotw(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 488 */
rw_func_knotb(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 489 */
rw_func_knotq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 490 */
rw_func_knotd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 491 */
rw_func_korw(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 492 */
rw_func_korb(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 493 */
rw_func_korq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 494 */
rw_func_kord(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 495 */
rw_func_kxnorw(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 496 */
rw_func_kxnorb(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 497 */
rw_func_kxnorq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 498 */
rw_func_kxnord(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 499 */
rw_func_kxorw(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 500 */
rw_func_kxorb(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 501 */
rw_func_kxorq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 502 */
rw_func_kxord(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 503 */
rw_func_kaddw(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 504 */
rw_func_kaddb(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 505 */
rw_func_kaddq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 506 */
rw_func_kaddd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 507 */
rw_func_kortestw(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 508 */
rw_func_kortestb(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 509 */
rw_func_kortestq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 510 */
rw_func_kortestd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 511 */
rw_func_kshiftlw(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 512 */
rw_func_kshiftlb(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 513 */
rw_func_kshiftlq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 514 */
rw_func_kshiftld(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 515 */
rw_func_kshiftrw(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 516 */
rw_func_kshiftrb(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 517 */
rw_func_kshiftrq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 518 */
rw_func_kshiftrd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 519 */
rw_func_ktestw(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 520 */
rw_func_ktestb(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 521 */
rw_func_ktestq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 522 */
rw_func_ktestd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

/*==========================================
 * k regs manipulation instructions end
 *=========================================*/

instr_t * /* 555 */
rw_func_vcvttsd2usi(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 556 */
rw_func_vcvttss2usi(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 561 */
rw_func_vcvtusi2sd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 562 */
rw_func_vcvtusi2ss(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 563 */
rw_func_vextractf64x2(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 572 */
rw_func_vextracti32x4(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 574 */
rw_func_vextracti64x2(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 607 */
rw_func_vinserti64x4(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 608 ~ 609 vmovdqa{32,64} template function */
rw_func_vmovdqa_(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start, const char *instr_name);

instr_t * /* 608 */
rw_func_vmovdqa32(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 609 */
rw_func_vmovdqa64(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 610 */
rw_func_vmovdqu16(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 611 */
rw_func_vmovdqu32(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 612 */
rw_func_vmovdqu64(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 613 */
rw_func_vmovdqu8(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 615 */
rw_func_vpandd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 618 */
rw_func_vpandq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 626 */
rw_func_vpcmpd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 627 */
rw_func_vpcmpq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 629 */
rw_func_vpcmpud(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 632 */
rw_func_vpcmpw(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 642 */
rw_func_vpermi2q(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 643 */
rw_func_vpermi2w(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 645 */
rw_func_vpermt2d(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 647 */
rw_func_vpermt2ps(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 648 */
rw_func_vpermt2q(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 649 */
rw_func_vpermt2w(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 653 */
rw_func_vpextr_(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 689 */
rw_func_vpmullq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 691 */
rw_func_vporq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 693 */
rw_func_vprolq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 697 */
rw_func_vprorq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 700 */
rw_func_vpscatterdd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 701 */
rw_func_vpscatterdq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 702 */
rw_func_vpscatterqd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 703 */
rw_func_vpscatterqq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 719 */
rw_func_vpxord(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 720 */
rw_func_vpxorq(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 738 */
rw_func_vrndscaleps(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 740 */
rw_func_vrndscaless(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 767 */
rw_func_vshufi32x4(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

instr_t * /* 779 */
rw_func_vrndscalesd(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

/**
 * @brief binary rewriting driver
 */
instr_t *
exec_rewrite_avx512_instr(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr, app_pc instr_start);

void
exec_rewrite_avx512_bb(dcontext_t *dcontext, instrlist_t *ilist);
/**
 * @typedef instr_rewrite_func_t
 * @brief Typedef for instruction rewrite function pointers.
 */
typedef instr_t *(instr_rewrite_func_t)(dcontext_t *, instrlist_t *, instr_t *, app_pc);

/**
 * @var rewrite_funcs
 * @brief Array of function pointers to instruction rewrite functions.
 *
 * This array maps opcode identifiers to their corresponding instruction rewrite
 * functions. Each entry in the array is a pointer to a function that handles the
 * rewriting of a specific AVX512 instruction. The functions are of type
 * instr_rewrite_func_t, which is a function pointer type taking parameters: a pointer to
 * the dynamic context (`dcontext_t`), a pointer to the instruction list (`instrlist_t`),
 * a pointer to the instruction (`instr_t`), and the start address of the instruction in
 * the application code (`app_pc`).
 *
 * The array indices correspond to opcode identifiers defined for AVX512 instructions.
 * For opcodes that do not have a specific rewrite function, the `rw_func_empty` function
 * is used as a placeholder. This function performs no operation and is intended for
 * unimplemented or unnecessary rewrites.
 *
 * @note This array is part of the binary rewriting driver which is not fully implemented.
 *
 * Usage Example:
 *   instr_rewrite_func_t *rewrite_function = rewrite_funcs[opcode];`
 *   rewrite_function(dcontext, ilist, instr, instr_start);
 *
 * @see instr_rewrite_func_t, rw_func_empty, exec_rewrite_avx512_instr
 */
extern instr_rewrite_func_t *rewrite_funcs[];

#endif /* _REWRITE_H_ */
