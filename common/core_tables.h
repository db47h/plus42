/*****************************************************************************
 * Plus42 -- an enhanced HP-42S calculator simulator
 * Copyright (C) 2004-2025  Thomas Okken
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see http://www.gnu.org/licenses/.
 *****************************************************************************/

#ifndef CORE_TABLES_H
#define CORE_TABLES_H 1


#include "free42.h"
#include "core_phloat.h"


/************/
/* Commands */
/************/

/* These are indices into cmd_array,
 * declared below (except the negative ones!)
 */

#define CMD_NONE         -1
#define CMD_CANCELLED    -2
#define CMD_LINGER1      -3
#define CMD_LINGER2      -4
#define CMD_CLX           0
#define CMD_ENTER         1
#define CMD_SWAP          2
#define CMD_RDN           3
#define CMD_CHS           4
#define CMD_DIV           5
#define CMD_MUL           6
#define CMD_SUB           7
#define CMD_ADD           8
#define CMD_LASTX         9
#define CMD_SILENT_OFF   10
#define CMD_SILENT_ON    11
#define CMD_SIN          12
#define CMD_COS          13
#define CMD_TAN          14
#define CMD_ASIN         15
#define CMD_ACOS         16
#define CMD_ATAN         17
#define CMD_LOG          18
#define CMD_10_POW_X     19
#define CMD_LN           20
#define CMD_E_POW_X      21
#define CMD_SQRT         22
#define CMD_SQUARE       23
#define CMD_INV          24
#define CMD_Y_POW_X      25
#define CMD_PERCENT      26
#define CMD_PI           27
#define CMD_COMPLEX      28
#define CMD_STO          29
#define CMD_STO_DIV      30
#define CMD_STO_MUL      31
#define CMD_STO_SUB      32
#define CMD_STO_ADD      33
#define CMD_RCL          34
#define CMD_RCL_DIV      35
#define CMD_RCL_MUL      36
#define CMD_RCL_SUB      37
#define CMD_RCL_ADD      38
#define CMD_FIX          39
#define CMD_SCI          40
#define CMD_ENG          41
#define CMD_ALL          42
#define CMD_NULL         43
#define CMD_ASTO         44
#define CMD_ARCL         45
#define CMD_CLA          46
#define CMD_DEG          47
#define CMD_RAD          48
#define CMD_GRAD         49
#define CMD_RECT         50
#define CMD_POLAR        51
#define CMD_SIZE         52
#define CMD_QUIET        53
#define CMD_CPXRES       54
#define CMD_REALRES      55
#define CMD_KEYASN       56
#define CMD_LCLBL        57
#define CMD_RDXDOT       58
#define CMD_RDXCOMMA     59
#define CMD_CLSIGMA      60
#define CMD_CLP          61
#define CMD_CLV          62
#define CMD_CLST         63
#define CMD_CLRG         64
#define CMD_DEL          65
#define CMD_CLKEYS       66
#define CMD_CLLCD        67
#define CMD_CLMENU       68
#define CMD_CLALLa       69
#define CMD_TO_DEG       70
#define CMD_TO_RAD       71
#define CMD_TO_HR        72
#define CMD_TO_HMS       73
#define CMD_TO_REC       74
#define CMD_TO_POL       75
#define CMD_IP           76
#define CMD_FP           77
#define CMD_RND          78
#define CMD_ABS          79
#define CMD_SIGN         80
#define CMD_MOD          81
#define CMD_SF           82
#define CMD_CF           83
#define CMD_FS_T         84
#define CMD_FC_T         85
#define CMD_FSC_T        86
#define CMD_FCC_T        87
#define CMD_COMB         88
#define CMD_PERM         89
#define CMD_FACT         90
#define CMD_GAMMA        91
#define CMD_RAN          92
#define CMD_SEED         93
#define CMD_LBL          94
#define CMD_RTN          95
#define CMD_INPUT        96
#define CMD_VIEW         97
#define CMD_AVIEW        98
#define CMD_XEQ          99
#define CMD_PROMPT      100
#define CMD_PSE         101
#define CMD_ISG         102
#define CMD_DSE         103
#define CMD_AIP         104
#define CMD_XTOA        105
#define CMD_AGRAPH      106
#define CMD_PIXEL       107
#define CMD_BEEP        108
#define CMD_TONE        109
#define CMD_MVAR        110
#define CMD_VARMENU     111
#define CMD_GETKEY      112
#define CMD_MENU        113
#define CMD_KEYG        114
#define CMD_KEYX        115
#define CMD_X_EQ_0      116
#define CMD_X_NE_0      117
#define CMD_X_LT_0      118
#define CMD_X_GT_0      119
#define CMD_X_LE_0      120
#define CMD_X_GE_0      121
#define CMD_X_EQ_Y      122
#define CMD_X_NE_Y      123
#define CMD_X_LT_Y      124
#define CMD_X_GT_Y      125
#define CMD_X_LE_Y      126
#define CMD_X_GE_Y      127
#define CMD_PRSIGMA     128
#define CMD_PRP         129
#define CMD_PRV         130
#define CMD_PRSTK       131
#define CMD_PRA         132
#define CMD_PRX         133
#define CMD_PRUSR       134
#define CMD_LIST        135
#define CMD_ADV         136
#define CMD_PRLCD       137
#define CMD_DELAY       138
#define CMD_PON         139
#define CMD_POFF        140
#define CMD_MAN         141
#define CMD_NORM        142
#define CMD_TRACE       143
#define CMD_SIGMAADD    144
#define CMD_SIGMASUB    145
#define CMD_GTO         146
#define CMD_END         147
#define CMD_NUMBER      148
#define CMD_STRING      149
#define CMD_RUN         150
#define CMD_SST         151
#define CMD_GTODOT      152
#define CMD_GTODOTDOT   153
#define CMD_STOP        154
#define CMD_NEWMAT      155
#define CMD_RUP         156
#define CMD_REAL_T      157
#define CMD_CPX_T       158
#define CMD_STR_T       159
#define CMD_MAT_T       160
#define CMD_DIM_T       161
#define CMD_ASSIGNa     162
#define CMD_ASSIGNb     163
#define CMD_ASGN01      164
#define CMD_ASGN02      165
#define CMD_ASGN03      166
#define CMD_ASGN04      167
#define CMD_ASGN05      168
#define CMD_ASGN06      169
#define CMD_ASGN07      170
#define CMD_ASGN08      171
#define CMD_ASGN09      172
#define CMD_ASGN10      173
#define CMD_ASGN11      174
#define CMD_ASGN12      175
#define CMD_ASGN13      176
#define CMD_ASGN14      177
#define CMD_ASGN15      178
#define CMD_ASGN16      179
#define CMD_ASGN17      180
#define CMD_ASGN18      181
#define CMD_ON          182
#define CMD_OFF         183
#define CMD_KEY1G       184
#define CMD_KEY2G       185
#define CMD_KEY3G       186
#define CMD_KEY4G       187
#define CMD_KEY5G       188
#define CMD_KEY6G       189
#define CMD_KEY7G       190
#define CMD_KEY8G       191
#define CMD_KEY9G       192
#define CMD_KEY1X       193
#define CMD_KEY2X       194
#define CMD_KEY3X       195
#define CMD_KEY4X       196
#define CMD_KEY5X       197
#define CMD_KEY6X       198
#define CMD_KEY7X       199
#define CMD_KEY8X       200
#define CMD_KEY9X       201
#define CMD_VMEXEC      202
#define CMD_VMSTO       203
#define CMD_SIGMAREG    204
#define CMD_SIGMAREG_T  205
#define CMD_CLD         206
#define CMD_ACOSH       207
#define CMD_ALENG       208
#define CMD_ALLSIGMA    209
#define CMD_AND         210
#define CMD_AOFF        211
#define CMD_AON         212
#define CMD_AROT        213
#define CMD_ASHF        214
#define CMD_ASINH       215
#define CMD_ATANH       216
#define CMD_ATOX        217
#define CMD_BASEADD     218
#define CMD_BASESUB     219
#define CMD_BASEMUL     220
#define CMD_BASEDIV     221
#define CMD_BASECHS     222
#define CMD_BEST        223
#define CMD_BINM        224
#define CMD_BIT_T       225
#define CMD_BST         226
#define CMD_CORR        227
#define CMD_COSH        228
#define CMD_CROSS       229
#define CMD_CUSTOM      230
#define CMD_DECM        231
#define CMD_DELR        232
#define CMD_DET         233
#define CMD_DIM         234
#define CMD_DOT         235
#define CMD_EDIT        236
#define CMD_EDITN       237
#define CMD_EXITALL     238
#define CMD_EXPF        239
#define CMD_E_POW_X_1   240
#define CMD_FCSTX       241
#define CMD_FCSTY       242
#define CMD_FNRM        243
#define CMD_GETM        244
#define CMD_GROW        245
#define CMD_HEXM        246
#define CMD_HMSADD      247
#define CMD_HMSSUB      248
#define CMD_I_ADD       249
#define CMD_I_SUB       250
#define CMD_INDEX       251
#define CMD_INSR        252
#define CMD_INTEG       253
#define CMD_INVRT       254
#define CMD_J_ADD       255
#define CMD_J_SUB       256
#define CMD_LINF        257
#define CMD_LINSIGMA    258
#define CMD_LN_1_X      259
#define CMD_LOGF        260
#define CMD_MEAN        261
#define CMD_NOT         262
#define CMD_OCTM        263
#define CMD_OLD         264
#define CMD_OR          265
#define CMD_PGMSLV      266
#define CMD_PGMINT      267
#define CMD_POSA        268
#define CMD_PUTM        269
#define CMD_PWRF        270
#define CMD_RCLEL       271
#define CMD_RCLIJ       272
#define CMD_RNRM        273
#define CMD_ROTXY       274
#define CMD_RSUM        275
#define CMD_SWAP_R      276
#define CMD_SDEV        277
#define CMD_SINH        278
#define CMD_SLOPE       279
#define CMD_SOLVE       280
#define CMD_STOEL       281
#define CMD_STOIJ       282
#define CMD_SUM         283
#define CMD_TANH        284
#define CMD_TRANS       285
#define CMD_UVEC        286
#define CMD_WMEAN       287
#define CMD_WRAP        288
#define CMD_X_SWAP      289
#define CMD_XOR         290
#define CMD_YINT        291
#define CMD_TO_DEC      292
#define CMD_TO_OCT      293
#define CMD_LEFT        294
#define CMD_UP          295
#define CMD_DOWN        296
#define CMD_RIGHT       297
#define CMD_PERCENT_CH  298
#define CMD_SIMQ        299
#define CMD_MATA        300
#define CMD_MATB        301
#define CMD_MATX        302
#define CMD_GOTOROW     303
#define CMD_GOTOCOLUMN  304
#define CMD_A_THRU_F    305
#define CMD_CLALLb      306
#define CMD_PGMSLVi     307
#define CMD_PGMINTi     308
#define CMD_VMSTO2      309
#define CMD_VMSOLVE     310
#define CMD_MAX         311
#define CMD_MIN         312
#define CMD_FIND        313
#define CMD_XROM        314
/* iPhone hardware support */
#define CMD_ACCEL       315
#define CMD_LOCAT       316
#define CMD_HEADING     317
/* HP-41 Time Module & CX Time */
#define CMD_ADATE       318
#define CMD_ATIME       319
#define CMD_ATIME24     320
#define CMD_CLK12       321
#define CMD_CLK24       322
#define CMD_DATE        323
#define CMD_DATE_PLUS   324
#define CMD_DDAYS       325
#define CMD_DMY         326
#define CMD_DOW         327
#define CMD_MDY         328
#define CMD_TIME        329
/* Intel Decimal Floating-Point Math Library: self-test */
#define CMD_FPTEST      330
/* Programming */
#define CMD_LSTO        331
#define CMD_SST_UP      332
#define CMD_SST_RT      333
#define CMD_YMD         334
#define CMD_GETKEY1     335
#define CMD_GETKEYA     336
#define CMD_LASTO       337
#define CMD_LCLV        338
/* BASE enhancements */
#define CMD_SL          339
#define CMD_SR          340
#define CMD_ASR         341
#define CMD_SLN         342
#define CMD_SRN         343
#define CMD_ASRN        344
#define CMD_LJ          345
#define CMD_RJ          346
#define CMD_RL          347
#define CMD_RR          348
#define CMD_RLC         349
#define CMD_RRC         350
#define CMD_RLN         351
#define CMD_RRN         352
#define CMD_RLCN        353
#define CMD_RRCN        354
#define CMD_SB          355
#define CMD_CB          356
#define CMD_B_T         357
#define CMD_NUM_B       358
#define CMD_MASKL       359
#define CMD_MASKR       360
#define CMD_SC          361
#define CMD_CC          362
#define CMD_C_T         363
#define CMD_N_TO_BS     364
#define CMD_N_TO_BD     365
#define CMD_N_TO_BQ     366
#define CMD_N_TO_DS     367
#define CMD_N_TO_DD     368
#define CMD_N_TO_DQ     369
#define CMD_BS_TO_N     370
#define CMD_BD_TO_N     371
#define CMD_BQ_TO_N     372
#define CMD_DS_TO_N     373
#define CMD_DD_TO_N     374
#define CMD_DQ_TO_N     375
#define CMD_WSIZE       376
#define CMD_WSIZE_T     377
#define CMD_BSIGNED     378
#define CMD_BWRAP       379
#define CMD_BRESET      380
#define CMD_DECINT      381
#define CMD_BINSEP      382
#define CMD_OCTSEP      383
#define CMD_DECSEP      384
#define CMD_HEXSEP      385
/* Useful X-Fcn functions missing from the 42S */
#define CMD_ANUM        386
#define CMD_X_SWAP_F    387
#define CMD_RCLFLAG     388
#define CMD_STOFLAG     389
/* User-defined functions */
#define CMD_FUNC        390
#define CMD_ERRMSG      391
#define CMD_ERRNO       392
#define CMD_RTNYES      393
#define CMD_RTNNO       394
#define CMD_RTNERR      395
#define CMD_STRACE      396
/* Big Stack */
#define CMD_4STK        397
#define CMD_L4STK       398
#define CMD_NSTK        399
#define CMD_LNSTK       400
#define CMD_DEPTH       401
#define CMD_DROP        402
#define CMD_DROPN       403
#define CMD_DUP         404
#define CMD_DUPN        405
#define CMD_PICK        406
#define CMD_UNPICK      407
#define CMD_RDNN        408
#define CMD_RUPN        409
/* Miscellaneous */
#define CMD_NOP         410
#define CMD_FMA         411
#define CMD_PGMMENU     412
#define CMD_PMEXEC      413
#define CMD_PGMVAR      414
#define CMD_VARMNU1     415
#define CMD_X2LINE      416
#define CMD_A2LINE      417
#define CMD_A2PLINE     418
#define CMD_RCOMPLX     419
#define CMD_PCOMPLX     420
#define CMD_CAPS        421
#define CMD_MIXED       422
#define CMD_STATIC      423
#define CMD_DYNAMIC     424
#define CMD_SKIP        425
#define CMD_CPXMAT_T    426
#define CMD_TYPE_T      427
#define CMD_A_THRU_F_2  428
#define CMD_DROP_CANCL  429
#define CMD_PRREG       430
#define CMD_CSLD_T      431
#define CMD_C_LN_1_X    432
#define CMD_C_E_POW_X_1 433
#define CMD_GETMI       434
#define CMD_PUTMI       435
#define CMD_GETLI       436
#define CMD_PUTLI       437
#define CMD_LOCK        438
#define CMD_UNLOCK      439
#define CMD_IDENT       440
/* String & List Functions */
#define CMD_XSTR        441
#define CMD_XASTO       442
#define CMD_LXASTO      443
#define CMD_XVIEW       444
#define CMD_APPEND      445
#define CMD_EXTEND      446
#define CMD_SUBSTR      447
#define CMD_LENGTH      448
#define CMD_HEAD        449
#define CMD_REV         450
#define CMD_POS         451
#define CMD_S_TO_N      452
#define CMD_N_TO_S      453
#define CMD_NN_TO_S     454
#define CMD_C_TO_N      455
#define CMD_N_TO_C      456
#define CMD_LIST_T      457
#define CMD_NEWLIST     458
#define CMD_TO_LIST     459
#define CMD_FROM_LIST   460
/* Generalized Comparisons */
#define CMD_X_EQ_NN     461
#define CMD_X_NE_NN     462
#define CMD_X_LT_NN     463
#define CMD_X_GT_NN     464
#define CMD_X_LE_NN     465
#define CMD_X_GE_NN     466
#define CMD_0_EQ_NN     467
#define CMD_0_NE_NN     468
#define CMD_0_LT_NN     469
#define CMD_0_GT_NN     470
#define CMD_0_LE_NN     471
#define CMD_0_GE_NN     472
/* Statitics Sums */
#define CMD_SN          473
#define CMD_SX          474
#define CMD_SX2         475
#define CMD_SY          476
#define CMD_SY2         477
#define CMD_SXY         478
#define CMD_SLNX        479
#define CMD_SLNX2       480
#define CMD_SLNY        481
#define CMD_SLNY2       482
#define CMD_SLNXLNY     483
#define CMD_SXLNY       484
#define CMD_SYLNX       485
/* Equations */
#define CMD_PARSE       486
#define CMD_UNPARSE     487
#define CMD_EVAL        488
#define CMD_EQN_T       489
#define CMD_STD         490
#define CMD_COMP        491
#define CMD_GTOL        492
#define CMD_XEQL        493
#define CMD_GSTO        494
#define CMD_GRCL        495
#define CMD_SVAR        496
#define CMD_GETITEM     497
#define CMD_GEN_EQ      498
#define CMD_GEN_NE      499
#define CMD_GEN_LT      500
#define CMD_GEN_GT      501
#define CMD_GEN_LE      502
#define CMD_GEN_GE      503
#define CMD_GEN_AND     504
#define CMD_GEN_OR      505
#define CMD_GEN_XOR     506
#define CMD_GEN_NOT     507
#define CMD_IF_T        508
#define CMD_TRUNC       509
#define CMD_DDAYSC      510
#define CMD_GETEQN      511
#define CMD_TO_PAR      512
#define CMD_FSTART      513
#define CMD_FSTACK      514
#define CMD_PUTITEM     515
#define CMD_EVALN       516
#define CMD_EVALNi      517
#define CMD_EQNSLV      518
#define CMD_EQNINT      519
#define CMD_EQNSLVi     520
#define CMD_EQNINTi     521
#define CMD_EQNVAR      522
#define CMD_EQNMENU     523
#define CMD_EQNMNU1     524
#define CMD_SPPV        525
#define CMD_SPFV        526
#define CMD_USPV        527
#define CMD_USFV        528
#define CMD_GEN_N       529
#define CMD_GEN_I       530
#define CMD_GEN_PV      531
#define CMD_GEN_PMT     532
#define CMD_GEN_FV      533
#define CMD_RAISE       534
#define CMD_DIRECT      535
#define CMD_NUMERIC     536
#define CMD_EMBED       537
#define CMD_NEWEQN      538
#define CMD_EDITEQN     539
/* Units */
#define CMD_CONVERT     540
#define CMD_UBASE       541
#define CMD_UVAL        542
#define CMD_UFACT       543
#define CMD_TO_UNIT     544
#define CMD_FROM_UNIT   545
#define CMD_N_PLUS_U    546
#define CMD_UNIT_T      547
/* Directories */
#define CMD_CRDIR       548
#define CMD_PGDIR       549
#define CMD_RENAME      550
#define CMD_CHDIR       551
#define CMD_UPDIR       552
#define CMD_HOME        553
#define CMD_PATH        554
#define CMD_REFMOVE     555
#define CMD_REFCOPY     556
#define CMD_REFFIND     557
#define CMD_PRALL       558
/* Big Screen */
#define CMD_WIDTH       559
#define CMD_HEIGHT      560
#define CMD_HEADER      561
#define CMD_ROW_PLUS    562
#define CMD_ROW_MINUS   563
#define CMD_COL_PLUS    564
#define CMD_COL_MINUS   565
#define CMD_GETDS       566
#define CMD_SETDS       567
#define CMD_1LINE       568
#define CMD_NLINE       569
#define CMD_LTOP        570
#define CMD_ATOP        571
#define CMD_HFLAGS      572
#define CMD_HPOLAR      573
#define CMD_STK         574
/* Menus */
#define CMD_DIRS        575
#define CMD_DIR_FCN     576
#define CMD_UNITS       577
#define CMD_UNIT_FCN    578
#define CMD_PLOT_M      579
#define CMD_TVM         580
#define CMD_EQN         581
#define CMD_EQN_FCN     582
#define CMD_N           583
#define CMD_I_PCT_YR    584
#define CMD_PV          585
#define CMD_PMT         586
#define CMD_FV          587
#define CMD_P_PER_YR    588
#define CMD_TBEGIN      589
#define CMD_TEND        590
#define CMD_TCLEAR      591
#define CMD_TRESET      592
#define CMD_AMORT       593
#define CMD_TNUM_P      594
#define CMD_TINT        595
#define CMD_TPRIN       596
#define CMD_TBAL        597
#define CMD_TNEXT       598
#define CMD_TFIRST      599
#define CMD_TLAST       600
#define CMD_TINCR       601
#define CMD_TGO         602
/* Graphing */
#define CMD_PGMPLOT     603
#define CMD_EQNPLOT     604
#define CMD_PARAM       605
#define CMD_XAXIS       606
#define CMD_YAXIS       607
#define CMD_CONST       608
#define CMD_VIEW_P      609
#define CMD_XMIN        610
#define CMD_XMAX        611
#define CMD_YMIN        612
#define CMD_YMAX        613
#define CMD_SCAN        614
#define CMD_PLOT        615
#define CMD_LINE        616
#define CMD_LIFE        617

#define CMD_SENTINEL    618


/* command_spec.argtype */

#define ARG_NONE      0 /* No argument */
#define ARG_VAR       1 /* Variable: num, stk, var, ind */
#define ARG_REAL      2 /* Variable: num, stk, var, ind; real scalars only */
#define ARG_NAMED     3 /* Variable: var, ind */
#define ARG_NUM9      4 /* num (0..9), ind */
#define ARG_NUM11     5 /* num (0..11), ind */
#define ARG_NUM99     6 /* num (0..99), ind */
#define ARG_COUNT     7 /* numeric-only (SIMQ, DEL, SIZE, LIST) */
#define ARG_FUNC      8 /* numeric-only, [0-4][0-4] (FUNC) */
#define ARG_LBL       9 /* Label: num, lclbl, global, ind */
#define ARG_CKEY     10 /* Key in custom menu */
#define ARG_MKEY     11 /* Key in programmable menu (KEYG/KEYX) */
#define ARG_PRGM     12 /* Alpha label (CATSECT_PGM) */
#define ARG_RVAR     13 /* Variable (real only) (MVAR, INTEG, SOLVE) */
#define ARG_MAT      14 /* Variable (matrix only) (EDITN, INDEX) */
#define ARG_M_STK    15 /* Matrix variable or stack (GETMI, PUTMI) */
#define ARG_L_STK    16 /* List variable or stack (GETLI, PUTLI) */
#define ARG_EQN      17 /* Variable (equation only) (EVAL) */
#define ARG_DIR      18 /* Directory name (CRDIR, PGDIR, CHDIR) */
#define ARG_XSTR     19 /* Long string (XSTR) */
#define ARG_OTHER    20 /* Weirdos */


/* command_spec.flags */

#define FLAG_PRGM_ONLY 1  /* Only allowed in program mode (LBL, DEL, ...) */
#define FLAG_IMMED     2  /* Executes in program mode (DEL, GTO.nnn, ...) */
#define FLAG_HIDDEN    4  /* Cannot be activated using XEQ "NAME" (SIMQ, ...) */
#define FLAG_NO_PRGM   8  /* Cannot be programmed (SIMQ, MATA, ...) */
#define FLAG_NO_SHOW  16  /* Do not show after keytimeout1 */
#define FLAG_SPECIAL  32  /* hp42s_code flags 0x01 */
#define FLAG_ILLEGAL  64  /* hp42s_code flags 0x02 */


/* Builtin cmd arg types */

/* These represent data types used when a command
 * is executing. Compare the ARG_ macros; those
 * represent the data types that a command is *capable*
 * of accepting, and are used during command entry.
 */

#define ARGTYPE_NONE      0
#define ARGTYPE_NUM       1
#define ARGTYPE_NEG_NUM   2 /* Used internally */
#define ARGTYPE_STK       3
#define ARGTYPE_STR       4
#define ARGTYPE_IND_NUM   5
#define ARGTYPE_IND_STK   6
#define ARGTYPE_IND_STR   7
#define ARGTYPE_LCLBL     9
#define ARGTYPE_DOUBLE   10
#define ARGTYPE_LBLINDEX 11
#define ARGTYPE_XSTR     12
#define ARGTYPE_EQN      13


struct arg_struct {
    unsigned char type;
    unsigned short length;
    int4 target;
    union {
        int4 num;
        char text[15];
        char stk;
        int cmd; /* For backward compatibility only! */
        char lclbl;
        const char *xstr;
    } val;
    // This used to be a member of the 'val' union, but once I changed it
    // from 'double' to 'phloat', that was no longer possible.
    phloat val_d;
};


struct command_spec {
    int (*handler)(arg_struct *arg);
    const char *name;
    unsigned char flags;
    unsigned char scode;
    unsigned char code1;
    unsigned char code2;
    unsigned char name_length;
    unsigned char argtype;
    signed char argcount;
    unsigned char rttypes;
};

extern const command_spec cmd_array[];

int handle(int cmd, arg_struct *arg);


#endif
