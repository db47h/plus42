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

#include <stdlib.h>
#include <string.h>

#include "core_commands1.h"
#include "core_commands2.h"
#include "core_display.h"
#include "core_globals.h"
#include "core_helpers.h"
#include "core_main.h"
#include "core_math1.h"
#include "core_math2.h"
#include "core_parser.h"
#include "core_sto_rcl.h"
#include "core_variables.h"
#include "shell.h"


/********************************************************/
/* Implementations of HP-42S built-in functions, part 2 */
/********************************************************/

int docmd_sf(arg_struct *arg) {
    if (arg->type == ARGTYPE_STK)
        return ERR_NONEXISTENT;
    int err;
    int4 num;
    err = arg_to_num(arg, &num);
    if (err != ERR_NONE)
        return err;
    if (num >= 100)
        return ERR_NONEXISTENT;
    if (num >= 36 && num <= 80)
        return ERR_RESTRICTED_OPERATION;
    if (virtual_flags[num] == '1')
        return virtual_flag_handler(FLAGOP_SF, num);
    else {
        flags.farray[num] = 1;
        if (num == 30)
            /* This is the stack_lift_disable flag.
             * Since we automatically enable stack lift after every command,
             * unless mode_disable_stack_lift is set, we must intervene for
             * this to actually have the intended effect.
             */
            mode_disable_stack_lift = true;
        else if (num == 25)
            /* This is the error ignore flag.
             * When flag 25 is cleared due to an error occurring, 'lasterr'
             * is set to the error number. This is to make sure we start
             * with a known, sane value.
             */
            lasterr = ERR_NONE;
        return ERR_NONE;
    }
}

int docmd_cf(arg_struct *arg) {
    if (arg->type == ARGTYPE_STK)
        return ERR_NONEXISTENT;
    int err;
    int4 num;
    err = arg_to_num(arg, &num);
    if (err != ERR_NONE)
        return err;
    if (num >= 100)
        return ERR_NONEXISTENT;
    if (num >= 36 && num <= 80)
        return ERR_RESTRICTED_OPERATION;
    if (virtual_flags[num] == '1')
        return virtual_flag_handler(FLAGOP_CF, num);
    else {
        flags.farray[num] = 0;
        return ERR_NONE;
    }
}

int docmd_fs_t(arg_struct *arg) {
    if (arg->type == ARGTYPE_STK)
        return ERR_NONEXISTENT;
    int err;
    int4 num;
    err = arg_to_num(arg, &num);
    if (err != ERR_NONE)
        return err;
    if (num >= 100)
        return ERR_NONEXISTENT;
    if (virtual_flags[num] == '1')
        return virtual_flag_handler(FLAGOP_FS_T, num);
    else
        return flags.farray[num] ? ERR_YES : ERR_NO;
}

int docmd_fc_t(arg_struct *arg) {
    if (arg->type == ARGTYPE_STK)
        return ERR_NONEXISTENT;
    int err;
    int4 num;
    err = arg_to_num(arg, &num);
    if (err != ERR_NONE)
        return err;
    if (num >= 100)
        return ERR_NONEXISTENT;
    if (virtual_flags[num] == '1')
        return virtual_flag_handler(FLAGOP_FC_T, num);
    else
        return flags.farray[num] ? ERR_NO : ERR_YES;
}

int docmd_fsc_t(arg_struct *arg) {
    if (arg->type == ARGTYPE_STK)
        return ERR_NONEXISTENT;
    int err;
    int4 num;
    err = arg_to_num(arg, &num);
    if (err != ERR_NONE)
        return err;
    if (num >= 100)
        return ERR_NONEXISTENT;
    if (num >= 36 && num <= 80)
        return ERR_RESTRICTED_OPERATION;
    if (virtual_flags[num] == '1')
        return virtual_flag_handler(FLAGOP_FSC_T, num);
    else {
        err = flags.farray[num] ? ERR_YES : ERR_NO;
        flags.farray[num] = 0;
        return err;
    }
}

int docmd_fcc_t(arg_struct *arg) {
    if (arg->type == ARGTYPE_STK)
        return ERR_NONEXISTENT;
    int err;
    int4 num;
    err = arg_to_num(arg, &num);
    if (err != ERR_NONE)
        return err;
    if (num >= 100)
        return ERR_NONEXISTENT;
    if (num >= 36 && num <= 80)
        return ERR_RESTRICTED_OPERATION;
    if (virtual_flags[num] == '1')
        return virtual_flag_handler(FLAGOP_FCC_T, num);
    else {
        err = flags.farray[num] ? ERR_NO : ERR_YES;
        flags.farray[num] = 0;
        return err;
    }
}

int docmd_comb(arg_struct *arg) {
    phloat y = ((vartype_real *) stack[sp - 1])->x;
    phloat x = ((vartype_real *) stack[sp])->x;
    phloat r, s, q = 1;
    vartype *v;
    if (x < 0 || x != floor(x) || x == x - 1 || y < 0 || y != floor(y))
        return ERR_INVALID_DATA;
    if (y < x)
        return ERR_INVALID_DATA;
    if (x > y / 2)
        x = y - x;
    #ifdef BCD_MATH
        s = x == 0 ? 1 : pow(10, 1 + floor(log10(x)));
    #else
        s = x == 0 ? 1 : pow(2, 1 + floor(log2(x)));
    #endif
    r = 1 / s;
    while (q <= x) {
        r *= y--;
        if (p_isinf(r)) {
            if (flags.f.range_error_ignore) {
                r = POS_HUGE_PHLOAT;
                break;
            } else
                return ERR_OUT_OF_RANGE;
        }
        r /= q++;
    }
    r *= s;
    if (p_isinf(r)) {
        if (flags.f.range_error_ignore)
            r = POS_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    }
    v = new_real(r);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    return binary_result(v);
}

int docmd_perm(arg_struct *arg) {
    phloat y = ((vartype_real *) stack[sp - 1])->x;
    phloat x = ((vartype_real *) stack[sp])->x;
    phloat r = 1;
    vartype *v;
    if (x < 0 || x != floor(x) || x == x - 1 || y < 0 || y != floor(y))
        return ERR_INVALID_DATA;
    if (y < x)
        return ERR_INVALID_DATA;
    while (x > 0) {
        r *= y--;
        if (p_isinf(r)) {
            if (flags.f.range_error_ignore) {
                r = POS_HUGE_PHLOAT;
                break;
            } else
                return ERR_OUT_OF_RANGE;
        }
        x--;
    }
    v = new_real(r);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    return binary_result(v);
}

static int mappable_fact(phloat x, phloat *y) {
    phloat f = 1;
    if (x < 0 || x != floor(x))
        return ERR_INVALID_DATA;
    while (x > 1) {
        f *= x--;
        if (p_isinf(f)) {
            if (flags.f.range_error_ignore) {
                *y = POS_HUGE_PHLOAT;
                return ERR_NONE;
            } else
                return ERR_OUT_OF_RANGE;
        }
    }
    *y = f;
    return ERR_NONE;
}

int docmd_fact(arg_struct *arg) {
    vartype *v;
    int err = map_unary(stack[sp], &v, mappable_fact, NULL);
    if (err == ERR_NONE)
        unary_result(v);
    return err;
}

static int mappable_gamma(phloat x, phloat *y) {
    if (x == 0 || x < 0 && x == floor(x))
        return ERR_INVALID_DATA;
    *y = tgamma(x);
    int inf = p_isinf(*y);
    if (inf != 0)
        if (flags.f.range_error_ignore)
            *y = inf < 0 ? NEG_HUGE_PHLOAT : POS_HUGE_PHLOAT;
        else
            return ERR_OUT_OF_RANGE;
    return ERR_NONE;
}

int docmd_gamma(arg_struct *arg) {
    vartype *v;
    int err = map_unary(stack[sp], &v, mappable_gamma, NULL);
    if (err == ERR_NONE)
        unary_result(v);
    return err;
}

int docmd_ran(arg_struct *arg) {
    vartype *v = new_real(math_random());
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    return recall_result(v);
}

int docmd_seed(arg_struct *arg) {
    phloat x = ((vartype_real *) stack[sp])->x;
    if (x == 0) {
        int8 s = shell_random_seed();
        if (s < 0)
            s = -s;
        s %= 100000000000000LL;
        s = s * 10 + 1;
        random_number_high = s / 100000000LL;
        random_number_low = s % 100000000LL;
        return ERR_NONE;
    }
    if (x < 0)
        x = -x;
    #ifdef BCD_MATH
        const phloat e12(1000000000000LL);
        if (x >= 1) {
            int exp = to_int(floor(log10(x)));
            Phloat mant = floor(x * pow(Phloat(10), 11 - exp) + 0.5);
            if (mant >= Phloat(1000000000000LL)) {
                mant /= 10;
                exp++;
            }
            x = (mant + ((exp + 1) % 100) / Phloat(100) + Phloat(1, 1000)) / e12;
        } else if (x >= Phloat(1LL, 1000000000000LL)) {
            x = floor(x * e12) / e12 + Phloat(1LL, 1000000000000000LL);
        } else {
            int exp = to_int(floor(log10(x) + 10000));
            if (exp > 9984)
                exp = 9984;
            x = ((exp + 16) % 100) / Phloat(100000000000000LL) + Phloat(1LL, 1000000000000000LL);
        }
        random_number_high = to_int8(x * Phloat(10000000));
        random_number_low = to_int8(fmod(x * Phloat(1000000000000000LL), Phloat(100000000)));
    #else
        if (x >= 1) {
            int exp = (int) floor(log10(x));
            int8 mant = (int8) floor(x * pow(10.0, 11 - exp) + 0.5);
            if (mant >= 1000000000000LL) {
                mant /= 10;
                exp++;
            }
            random_number_high = mant / 100000;
            random_number_low = (mant % 100000) * 1000L + (exp + 1) % 100 * 10 + 1;
        } else if (x >= 1e-12) {
            int8 t = (int8) floor(x * 1e12);
            random_number_high = t / 100000;
            random_number_low = (t % 100000) * 1000L + 1;
        } else {
            int exp = (int) floor(log10(x) + 1000);
            if (exp > 984)
                exp = 984;
            random_number_high = 0;
            random_number_low = (exp + 16) % 100 * 10 + 1;
        }
    #endif
    return ERR_NONE;
}

int docmd_lbl(arg_struct *arg) {
    if (mode_message_lines == 0)
        fly_goose();
    return ERR_NONE;
}

int docmd_rtn(arg_struct *arg) {
    if (!program_running()) {
        clear_all_rtns();
        pc = -1;
        return ERR_NONE;
    }
    int err = pop_func_state(false);
    if (err != ERR_NONE)
        return err;
    return rtn(ERR_NONE);
}

int docmd_input(arg_struct *arg) {
    vartype *v;
    int err;
    if (arg->type == ARGTYPE_IND_NUM
            || arg->type == ARGTYPE_IND_STK
            || arg->type == ARGTYPE_IND_STR) {
        err = resolve_ind_arg(arg);
        if (err != ERR_NONE)
            return err;
    }

    err = generic_rcl(arg, &v, true);
    if (err == ERR_NONEXISTENT || err == ERR_VARIABLE_NOT_WRITABLE) {
        v = new_real(0);
        if (v == NULL)
            return ERR_INSUFFICIENT_MEMORY;
    } else if (err != ERR_NONE)
        return err;

    input_length = 0;
    input_arg = *arg;
    if (arg->type == ARGTYPE_NUM) {
        char2buf(input_name, 11, &input_length, 'R');
        if (arg->val.num < 10)
            char2buf(input_name, 11, &input_length, '0');
        input_length += int2string(arg->val.num, input_name + input_length,
                                            11 - input_length);
    } else if (arg->type == ARGTYPE_STK) {
        string2buf(input_name, 11, &input_length, "ST ", 3);
        char2buf(input_name, 11, &input_length, arg->val.stk);
    } else if (arg->type == ARGTYPE_STR) {
        string2buf(input_name, 11, &input_length, arg->val.text, arg->length);
    } else {
        free_vartype(v);
        return ERR_INVALID_TYPE;
    }

    if (flags.f.printer_enable && flags.f.printer_exists
            && (flags.f.trace_print || flags.f.normal_print)) {
        int size = input_length + 1;
        switch (v->type) {
            case TYPE_STRING: size += ((vartype_string *) v)->length + 2; break;
            case TYPE_EQUATION: size += ((vartype_equation *) v)->data->length + 2; break;
            case TYPE_UNIT: size += ((vartype_unit *) v)->length + 100; break;
            default: size += 100;
        }
        char *buf = (char *) malloc(size);
        if (buf != NULL) {
            int bufptr = 0;
            string2buf(buf, size, &bufptr, input_name, input_length);
            char2buf(buf, size, &bufptr, '?');
            bufptr += vartype2string(v, buf + bufptr, size - bufptr);
            print_lines(buf, bufptr, true);
            free(buf);
        }
    }

    docmd_cld(NULL);
    if (recall_result(v) != ERR_NONE)
        return ERR_INSUFFICIENT_MEMORY;
    else
        return ERR_STOP;
}

int view_helper(arg_struct *arg, bool print) {
    int err;
    if (arg->type == ARGTYPE_IND_NUM
            || arg->type == ARGTYPE_IND_STK
            || arg->type == ARGTYPE_IND_STR) {
        err = resolve_ind_arg(arg);
        if (err != ERR_NONE)
            return err;
    }
    vartype *v;
    err = generic_rcl(arg, &v);
    if (err != ERR_NONE)
        return err;

    char buf[64];
    int bufptr = 0;
    switch (arg->type) {
        case ARGTYPE_NUM: {
            int num = arg->val.num;
            char2buf(buf, 64, &bufptr, 'R');
            if (num < 10)
                char2buf(buf, 64, &bufptr, '0');
            bufptr += int2string(num, buf + bufptr, 64 - bufptr);
            break;
        }
        case ARGTYPE_STK:
            string2buf(buf, 64, &bufptr, "ST ", 3);
            char2buf(buf, 64, &bufptr, arg->val.stk);
            break;
        case ARGTYPE_STR:
            string2buf(buf, 64, &bufptr, arg->val.text, arg->length);
            break;
    }
    char2buf(buf, 64, &bufptr, '=');
    int part2 = bufptr;

    int vlength;
    if (v->type == TYPE_STRING) {
        vartype_string *s = (vartype_string *) v;
        vlength = s->length + 2;
    } else if (v->type == TYPE_EQUATION) {
        vartype_equation *eq = (vartype_equation *) v;
        vlength = eq->data->length + 2;
    } else {
        vlength = 42; // max length of complex number
    }
    char *sbuf = NULL;
    char *pbuf = buf;
    int maxlen = disp_r * disp_c;
    int slen = bufptr + vlength;
    if (slen > maxlen)
        slen = maxlen;
    if (slen > 64) {
        sbuf = (char *) malloc(slen);
        if (sbuf == NULL) {
            slen = 64;
        } else {
            memcpy(sbuf, buf, bufptr);
            pbuf = sbuf;
        }
    }
    bufptr += vartype2string(v, pbuf + bufptr, slen - bufptr);
    draw_long_message(0, pbuf, bufptr);
    free(sbuf);
    bufptr = part2;

    err = ERR_NONE;
    if (print && (flags.f.printer_enable || !program_running())) {
        if (flags.f.printer_exists) {
            set_annunciators(-1, -1, 1, -1, -1, -1);
            sbuf = NULL;
            pbuf = buf;
            slen = bufptr + vlength;
            if (slen > 64) {
                sbuf = (char *) malloc(slen);
                if (sbuf == NULL) {
                    slen = 64;
                } else {
                    memcpy(sbuf, buf, bufptr);
                    pbuf = sbuf;
                }
            }
            bufptr += vartype2string(v, pbuf + bufptr, slen - bufptr);
            print_lines(pbuf, bufptr, true);
            free(sbuf);
            set_annunciators(-1, -1, 0, -1, -1, -1);
        } else
            err = ERR_STOP;
    }
    free_vartype(v);
    return err;
}

int docmd_view(arg_struct *arg) {
    return view_helper(arg, 1);
}

int docmd_aview(arg_struct *arg) {
    alpha_view_helper(reg_alpha, reg_alpha_length);
    if (flags.f.printer_enable || !program_running()) {
        if (flags.f.printer_exists)
            docmd_pra(arg);
        else
            return ERR_STOP;
    }
    return ERR_NONE;
}

int docmd_xeq(arg_struct *arg) {
    if (program_running()) {
        int err = push_rtn_addr(current_prgm, pc);
        if (err != ERR_NONE)
            return err;
        err = docmd_gto(arg);
        if (err != ERR_NONE) {
            pgm_index dummy1;
            int4 dummy2;
            bool dummy3;
            pop_rtn_addr(&dummy1, &dummy2, &dummy3);
        } else
            save_csld();
        return err;
    } else {
        directory *dir = cwd;
        int err = docmd_gto(arg);
        if (err != ERR_NONE)
            return err;
        clear_all_rtns();
        save_csld();
        // GTO with ARGTYPE_LBLINDEX (which can only happen in RUN mode),
        // sets the current directory to the target's containing directory.
        // XEQ shouldn't do this, hence:
        cwd = dir;
        return ERR_RUN;
    }
}

int docmd_prompt(arg_struct *arg) {
    alpha_view_helper(reg_alpha, reg_alpha_length);
    if (flags.f.printer_enable && flags.f.printer_exists
            && (flags.f.trace_print || flags.f.normal_print))
        docmd_pra(arg);
    return ERR_STOP;
}

int docmd_pse(arg_struct *arg) {
    if (program_running()) {
        int saved_command = pending_command;
        pending_command = CMD_NONE;
        redisplay();
        pending_command = saved_command;
        mode_pause = true;
    }
    return ERR_NONE;
}

static int generic_loop_helper(phloat *x, bool isg) {
    phloat t;
    #ifdef BCD_MATH
        phloat i;
    #else
        int8 i;
    #endif
    int4 j, k;
    int s;

    if (*x == (isg ? *x + 1 : *x - 1)) {
        /* Too big to do anything useful with; this is what the real
         * HP-42S does in this case:
         */
        return isg == (*x < 0) ? ERR_YES : ERR_NO;
    }
    /* Break number up as follows: II.JJJKKRRRRR
     * The sign goes with I; everything else is considered positive.
     */
    t = *x;
    if (t < 0) {
        t = -t;
        s = -1;
    } else
        s = 1;

    #ifdef BCD_MATH
        i = floor(t);
        t = (t - i) * 100000;
    #else
        i = to_int8(t);
        t = (t - i) * 100000;
        /* The 0.0000005 is a precaution to prevent the loop increment
         * value from being taken to be 1 lower than what the user intended;
         * this can happen because the decimal fractions used here cannot,
         * in general, be represented exactly in binary, so that what should
         * be 10.00902 may actually end up being approximated as something
         * fractionally lower -- and 10.0090199999999+ would be interpreted
         * as having a loop increment of 1, not the 2 that was intended.
         * By adding 0.0000005 before truncating, we effectively round to
         * 7 decimals, which is all that a real HP-42S would have left after
         * the multiplication by 100000. So, we sacrifice some of the range
         * of an IEEE-754 double, but maintain HP-42S compatibility.
         */
        t = t + 0.0000005;
    #endif
    k = to_int4(t);
    j = k / 100;
    k -= j * 100;
    if (k == 0)
        k = 1;

    /* Update the 'real' loop control value separately from the components
     * we have just separated out. I'm very paranoid about cumulative errors,
     * so I don't rebuild the loop control value from i, j, k, etc.
     * This way is computationally cheaper, anyway.
     */
    if (isg) {
        if (*x < 0 && floor(-(*x)) <= k)
            *x = -(*x) + k - 2 * i;
        else
            *x += k;
    } else {
        if (*x > 0 && *x < k)
            *x = -(*x) - k + 2 * i;
        else
            *x -= k;
    }

    /* Now we do what you would expect ISG/DSE to do... */
    if (isg) {
        if (s == -1)
            i = k - i;
        else
            i = k + i;
        return i > j ? ERR_NO : ERR_YES;
    } else {
        if (s == -1)
            i = -i - k;
        else
            i = i - k;
        return i <= j ? ERR_NO : ERR_YES;
    }
}

static int generic_loop(arg_struct *arg, bool isg) {
    int err;
    if (arg->type == ARGTYPE_IND_NUM
            || arg->type == ARGTYPE_IND_STK
            || arg->type == ARGTYPE_IND_STR) {
        err = resolve_ind_arg(arg);
        if (err != ERR_NONE)
            return err;
    }
    switch (arg->type) {
        case ARGTYPE_NUM: {
            vartype *regs = recall_var("REGS", 4);
            if (regs == NULL)
                return ERR_SIZE_ERROR;
            else if (regs->type == TYPE_REALMATRIX) {
                vartype_realmatrix *rm = (vartype_realmatrix *) regs;
                int4 size = rm->rows * rm->columns;
                int4 index = arg->val.num;
                if (index >= size)
                    return ERR_SIZE_ERROR;
                if (rm->array->is_string[index] != 0)
                    return ERR_ALPHA_DATA_IS_INVALID;
                else {
                    if (!disentangle(regs))
                        return ERR_INSUFFICIENT_MEMORY;
                    return generic_loop_helper(&rm->array->data[index], isg);
                }
            } else if (regs->type == TYPE_COMPLEXMATRIX) {
                return ERR_INVALID_TYPE;
            } else {
                /* This should never happen; STO should prevent
                 * "REGS" from being any other type than a real or
                 * complex matrix.
                 */
                return ERR_INTERNAL_ERROR;
            }
        }
        case ARGTYPE_STK: {
            int idx;
            switch (arg->val.stk) {
                case 'X': idx = 0; break;
                case 'Y': idx = 1; break;
                case 'Z': idx = 2; break;
                case 'T': idx = 3; break;
                case 'L': idx = -1; break;
            }
            vartype *v;
            if (idx == -1) {
                v = lastx;
            } else {
                if (idx > sp)
                    return ERR_STACK_DEPTH_ERROR;
                v = stack[sp - idx];
            }
            if (v->type == TYPE_REAL)
                return generic_loop_helper(&((vartype_real *) v)->x, isg);
            else if (v->type == TYPE_STRING)
                return ERR_ALPHA_DATA_IS_INVALID;
            else
                return ERR_INVALID_TYPE;
        }
        case ARGTYPE_STR: {
            bool writable;
            vartype *v = recall_var(arg->val.text, arg->length, &writable);
            if (v == NULL)
                return ERR_NONEXISTENT;
            else if (!writable)
                return ERR_VARIABLE_NOT_WRITABLE;
            else if (v->type == TYPE_REAL)
                return generic_loop_helper(&((vartype_real *) v)->x, isg);
            else if (v->type == TYPE_STRING)
                return ERR_ALPHA_DATA_IS_INVALID;
            else
                return ERR_INVALID_TYPE;
        }
        default:
            return ERR_INTERNAL_ERROR;
    }
}

int docmd_isg(arg_struct *arg) {
    return generic_loop(arg, true);
}

int docmd_dse(arg_struct *arg) {
    return generic_loop(arg, false);
}

int docmd_aip(arg_struct *arg) {
    char buf[44];
    int size = ip2revstring(((vartype_real *) stack[sp])->x, buf, 44);
    append_alpha_string(buf, size, 1);
    if (flags.f.trace_print && flags.f.printer_exists)
        docmd_pra(NULL);
    return ERR_NONE;
}

int docmd_xtoa(arg_struct *arg) {
    if (stack[sp]->type == TYPE_REAL) {
        phloat x = ((vartype_real *) stack[sp])->x;
        if (x < 0)
            x = -x;
        if (x >= 256)
            return ERR_INVALID_DATA;
        append_alpha_char(to_char(x));
    } else if (stack[sp]->type == TYPE_STRING) {
        vartype_string *s = (vartype_string *) stack[sp];
        append_alpha_string(s->txt(), s->length, 0);
    } else if (stack[sp]->type == TYPE_REALMATRIX) {
        vartype_realmatrix *m = (vartype_realmatrix *) stack[sp];
        int4 size = m->rows * m->columns;
        int4 i;
        char buf[44];
        int buflen = 0;
        for (i = size - 1; i >= 0; i--) {
            if (m->array->is_string[i] != 0) {
                int4 len;
                char *text;
                get_matrix_string(m, i, &text, &len);
                for (int4 j = len - 1; j >= 0; j--) {
                    buf[buflen++] = text[j];
                    if (buflen == 44)
                        goto done;
                }
            } else {
                phloat d = m->array->data[i];
                if (d < 0)
                    d = -d;
                if (d >= 256)
                    buf[buflen++] = (char) 255;
                else
                    buf[buflen++] = to_char(d);
                if (buflen == 44)
                    goto done;
            }
        }
        done:
        append_alpha_string(buf, buflen, 1);
    }
    if (flags.f.trace_print && flags.f.printer_exists)
        docmd_pra(NULL);
    return ERR_NONE;
}

int docmd_agraph(arg_struct *arg) {
    switch (stack[sp]->type) {
        case TYPE_REAL: {
            if (stack[sp - 1]->type == TYPE_REAL) {
                phloat x = ((vartype_real *) stack[sp])->x;
                phloat y = ((vartype_real *) stack[sp - 1])->x;
                draw_pattern(x, y, reg_alpha, reg_alpha_length);
                flush_display();
                mode_message_lines = ALL_LINES;
                return ERR_NONE;
            } else if (stack[sp - 1]->type == TYPE_STRING)
                return ERR_ALPHA_DATA_IS_INVALID;
            else
                return ERR_INVALID_TYPE;
        }
        case TYPE_COMPLEX: {
            phloat x = ((vartype_complex *) stack[sp])->re;
            phloat y = ((vartype_complex *) stack[sp])->im;
            draw_pattern(x, y, reg_alpha, reg_alpha_length);
            flush_display();
            mode_message_lines = ALL_LINES;
            return ERR_NONE;
        }
        case TYPE_COMPLEXMATRIX: {
            vartype_complexmatrix *cm = (vartype_complexmatrix *) stack[sp];
            int4 size = 2 * cm->rows * cm->columns;
            int4 i;
            for (i = 0; i < size; i += 2)
                draw_pattern(cm->array->data[i], cm->array->data[i + 1],
                             reg_alpha, reg_alpha_length);
            flush_display();
            mode_message_lines = ALL_LINES;
            return ERR_NONE;
        }
        case TYPE_STRING:
            return ERR_ALPHA_DATA_IS_INVALID;
        default:
            return ERR_INVALID_TYPE;
    }
}

static void pixel_helper(phloat dx, phloat dy) {
    dx = dx < 0 ? -floor(-dx + 0.5) : floor(dx + 0.5);
    dy = dy < 0 ? -floor(-dy + 0.5) : floor(dy + 0.5);
    int w1 = disp_w + 1;
    int x = dx < -w1 ? -w1 : dx > w1 ? w1 : to_int(dx);
    int h1 = disp_h + 1;
    int y = dy < -h1 ? -h1 : dy > h1 ? h1 : to_int(dy);
    int i;
    int dot = 1;
    if (x < 0) {
        x = -x;
        if (x >= 1 && x <= disp_w)
            for (i = 0; i < disp_h; i++)
                draw_pixel(x - 1, i);
        dot = 0;
    }
    if (y < 0) {
        y = -y;
        if (y >= 1 && y <= disp_h)
            for (i = 0; i < disp_w; i++)
                draw_pixel(i, y - 1);
        dot = 0;
    }
    if (dot && x >= 1 && x <= disp_w && y >= 1 && y <= disp_h)
        draw_pixel(x - 1, y - 1);
}

int docmd_pixel(arg_struct *arg) {
    if (stack[sp]->type == TYPE_REAL) {
        if (stack[sp - 1]->type == TYPE_REAL) {
            pixel_helper(((vartype_real *) stack[sp])->x,
                         ((vartype_real *) stack[sp - 1])->x);
            flush_display();
            mode_message_lines = ALL_LINES;
            return ERR_NONE;
        } else if (stack[sp - 1]->type == TYPE_STRING)
            return ERR_ALPHA_DATA_IS_INVALID;
        else
            return ERR_INVALID_TYPE;
    } else if (stack[sp]->type == TYPE_COMPLEX) {
        pixel_helper(((vartype_complex *) stack[sp])->re,
                     ((vartype_complex *) stack[sp])->im);
        flush_display();
        mode_message_lines = ALL_LINES;
        return ERR_NONE;
    } else if (stack[sp]->type == TYPE_COMPLEXMATRIX) {
        vartype_complexmatrix *m = (vartype_complexmatrix *) stack[sp];
        int4 size = 2 * m->rows * m->columns;
        int4 i;
        for (i = 0; i < size; i += 2)
            pixel_helper(m->array->data[i], m->array->data[i + 1]);
        flush_display();
        mode_message_lines = ALL_LINES;
        return ERR_NONE;
    } else if (stack[sp]->type == TYPE_STRING)
        return ERR_ALPHA_DATA_IS_INVALID;
    else
        return ERR_INVALID_TYPE;
}

int docmd_beep(arg_struct *arg) {
    tone(8);
    tone(5);
    tone(9);
    tone(8);
    return ERR_NONE;
}

int docmd_tone(arg_struct *arg) {
    if (arg->type == ARGTYPE_STK)
        return ERR_INVALID_DATA;
    int err;
    int4 num;
    err = arg_to_num(arg, &num);
    if (err != ERR_NONE)
        return err;
    if (num >= 10)
        return ERR_INVALID_DATA;
    tone(num);
    return ERR_NONE;
}

int docmd_mvar(arg_struct *arg) {
    return ERR_NONE;
}

int docmd_varmenu(arg_struct *arg) {
    if (arg->type == ARGTYPE_IND_NUM
            || arg->type == ARGTYPE_IND_STK
            || arg->type == ARGTYPE_IND_STR) {
        int err = resolve_ind_arg(arg);
        if (err != ERR_NONE)
            return err;
    }
    return start_varmenu_lbl(arg->val.text, arg->length, 0);
}

int docmd_eqnmenu(arg_struct *arg) {
    vartype_equation *eq;
    int err = get_arg_equation(arg, &eq);
    if (err != ERR_NONE)
        return err;
    return start_varmenu_eqn((vartype *) eq, 0);
}

int docmd_getkey(arg_struct *arg) {
    mode_getkey = true;
    mode_getkey1 = false;
    mode_disable_stack_lift = flags.f.stack_lift_disable;
    return ERR_NONE;
}

int docmd_menu(arg_struct *arg) {
    if (!program_running())
        return ERR_RESTRICTED_OPERATION;
    set_menu(MENULEVEL_PLAIN, MENU_PROGRAMMABLE);
    mode_plainmenu_sticky = true;
    return ERR_NONE;
}

/*
 * Note: Cheerfully casting to vartype_real * here, even though the parameter
 * can also be a unit object. This is OK because the unit object has its
 * numerical component at the same offset as the real object.
 */

int docmd_x_eq_0(arg_struct *arg) {
    return ((vartype_real *) stack[sp])->x == 0 ? ERR_YES : ERR_NO;
}

int docmd_x_ne_0(arg_struct *arg) {
    return ((vartype_real *) stack[sp])->x != 0 ? ERR_YES : ERR_NO;
}

int docmd_x_lt_0(arg_struct *arg) {
    return ((vartype_real *) stack[sp])->x < 0 ? ERR_YES : ERR_NO;
}

int docmd_x_gt_0(arg_struct *arg) {
    return ((vartype_real *) stack[sp])->x > 0 ? ERR_YES : ERR_NO;
}

int docmd_x_le_0(arg_struct *arg) {
    return ((vartype_real *) stack[sp])->x <= 0 ? ERR_YES : ERR_NO;
}

int docmd_x_ge_0(arg_struct *arg) {
    return ((vartype_real *) stack[sp])->x >= 0 ? ERR_YES : ERR_NO;
}

int docmd_x_eq_y(arg_struct *arg) {
    return vartype_equals(stack[sp], stack[sp - 1]) ? ERR_YES : ERR_NO;
}

int docmd_x_ne_y(arg_struct *arg) {
    return vartype_equals(stack[sp], stack[sp - 1]) ? ERR_NO : ERR_YES;
}

int docmd_x_lt_y(arg_struct *arg) {
    return generic_comparison(stack[sp], stack[sp - 1], 'L');
}

int docmd_x_gt_y(arg_struct *arg) {
    return generic_comparison(stack[sp], stack[sp - 1], 'G');
}

int docmd_x_le_y(arg_struct *arg) {
    return generic_comparison(stack[sp], stack[sp - 1], 'l');
}

int docmd_x_ge_y(arg_struct *arg) {
    return generic_comparison(stack[sp], stack[sp - 1], 'g');
}

struct sigma_label_spec {
    const char *text;
    int length;
};

static const sigma_label_spec sigma_labels[] = {
    { "\5X=",           3 },
    { "\5X^2=",         5 },
    { "\5Y=",           3 },
    { "\5Y^2=",         5 },
    { "\5XY=",          4 },
    { "N=",             2 },
    { "\5LN(X)=",       7 },
    { "\5LN(X)^2=",     9 },
    { "\5LN(Y)=",       7 },
    { "\5LN(Y)^2=",     9 },
    { "\5LN(X)LN(Y)=", 12 },
    { "\5XLN(Y)=",      8 },
    { "\5YLN(X)=",      8 }
};

int docmd_prsigma(arg_struct *arg) {
    vartype *regs = recall_var("REGS", 4);
    vartype_realmatrix *rm;
    int nr;
    int4 size, max, i;
    char buf[100];
    int bufptr;

    if (regs == NULL)
        return ERR_NONEXISTENT;
    if (regs->type != TYPE_REALMATRIX)
        return ERR_INVALID_TYPE;
    rm = (vartype_realmatrix *) regs;
    nr = flags.f.all_sigma ? 13 : 6;
    size = rm->rows * rm->columns;
    max = mode_sigma_reg + nr;
    if (max > size)
        return ERR_SIZE_ERROR;
    if (!flags.f.printer_enable && program_running())
        return ERR_NONE;
    if (!flags.f.printer_exists)
        return ERR_PRINTING_IS_DISABLED;

    set_annunciators(-1, -1, 1, -1, -1, -1);
    print_text(NULL, 0, true);
    for (i = 0; i < nr; i++) {
        int4 j = i + mode_sigma_reg;
        if (rm->array->is_string[j] != 0) {
            char *text;
            int4 len;
            get_matrix_string(rm, j, &text, &len);
            char *sbuf = (char *) malloc(len + 2);
            if (sbuf == NULL) {
                print_wide(sigma_labels[i].text, sigma_labels[i].length, "<Low Mem>", 9);
            } else {
                sbuf[0] = '"';
                memcpy(sbuf + 1, text, len);
                sbuf[len + 1] = '"';
                print_wide(sigma_labels[i].text, sigma_labels[i].length, sbuf, len + 2);
                free(sbuf);
            }
        } else {
            bufptr = easy_phloat2string(rm->array->data[j], buf, 100, 0);
            print_wide(sigma_labels[i].text, sigma_labels[i].length, buf, bufptr);
        }
    }
    set_annunciators(-1, -1, 0, -1, -1, -1);
    return ERR_NONE;
}

int docmd_prp(arg_struct *arg) {
    pgm_index prgm;
    if (arg->type == ARGTYPE_LBLINDEX) {
        directory *dir = get_dir(arg->target);
        prgm.set(dir->id, dir->labels[arg->val.num].prgm);
    } else if (arg->type == ARGTYPE_STR) {
        if (arg->length == 0)
            prgm = current_prgm;
        else {
            int4 pc;
            if (!find_global_label(arg, &prgm, &pc))
                return ERR_LABEL_NOT_FOUND;
        }
    } else
        return ERR_INVALID_TYPE;
    if (!flags.f.printer_exists)
        return ERR_PRINTING_IS_DISABLED;
    return print_program(prgm, -1, -1, false);
}

static vartype *prv_var;
static int4 prv_index;
static bool prv_prreg;
static int prv_worker(bool interrupted);

int docmd_prv(arg_struct *arg) {
    if (arg->type == ARGTYPE_IND_NUM || arg->type == ARGTYPE_IND_STK
            || arg->type == ARGTYPE_IND_STR) {
        int err = resolve_ind_arg(arg);
        if (err != ERR_NONE)
            return err;
    }
    if (arg->type != ARGTYPE_STR)
        return ERR_INVALID_TYPE;
    else {
        vartype *v = recall_var(arg->val.text, arg->length);
        char lbuf[32], rbuf[100];
        int llen = 0, rlen = 0;
        if (v == NULL)
            return ERR_NONEXISTENT;
        if (!flags.f.printer_enable && program_running())
            return ERR_NONE;
        if (!flags.f.printer_exists)
            return ERR_PRINTING_IS_DISABLED;

        set_annunciators(-1, -1, 1, -1, -1, -1);
        string2buf(lbuf, 8, &llen, arg->val.text, arg->length);
        char2buf(lbuf, 8, &llen, '=');
        if (v->type == TYPE_STRING || v->type == TYPE_EQUATION) {
            const char *text;
            int4 length;
            char d;
            if (v->type == TYPE_STRING) {
                vartype_string *s = (vartype_string *) v;
                text = s->txt();
                length = s->length;
                d = '"';
            } else {
                vartype_equation *eq = (vartype_equation *) v;
                equation_data *eqd = eq->data;
                text = eqd->text;
                length = eqd->length;
                d = '\'';
            }
            char *sbuf = (char *) malloc(length + 2);
            if (sbuf == NULL) {
                set_annunciators(-1, -1, 0, -1, -1, -1);
                return ERR_INSUFFICIENT_MEMORY;
            }
            sbuf[0] = d;
            memcpy(sbuf + 1, text, length);
            sbuf[length + 1] = d;
            print_wide(lbuf, llen, sbuf, length + 2);
            free(sbuf);
        } else {
            rlen = vartype2string(v, rbuf, 100);
            print_wide(lbuf, llen, rbuf, rlen);
        }

        if (v->type == TYPE_REALMATRIX
                || v->type == TYPE_COMPLEXMATRIX
                || v->type == TYPE_LIST
                && ((vartype_list *) v)->size > 0) {
            prv_var = v;
            prv_prreg = false;
            prv_index = 0;
            mode_interruptible = prv_worker;
            mode_stoppable = true;
            return ERR_INTERRUPTIBLE;
        } else {
            set_annunciators(-1, -1, 0, -1, -1, -1);
            return ERR_NONE;
        }
    }
}

static int prv_worker(bool interrupted) {
    char lbuf[32], rbuf[100];
    int llen = 0, rlen = 0;
    int4 i, j, sz;

    if (interrupted) {
        set_annunciators(-1, -1, 0, -1, -1, -1);
        return ERR_STOP;
    }

    if (prv_var->type == TYPE_REALMATRIX) {
        vartype_realmatrix *rm = (vartype_realmatrix *) prv_var;
        sz = rm->rows * rm->columns;
        if (prv_prreg) {
            char2buf(lbuf, 32, &llen, 'R');
            if (prv_index < 10)
                char2buf(lbuf, 32, &llen, '0');
            llen += int2string(prv_index, lbuf + llen, 32 - llen);
            char2buf(lbuf, 32, &llen, '=');
        } else {
            i = prv_index / rm->columns;
            j = prv_index % rm->columns;
            llen = int2string(i + 1, lbuf, 32);
            char2buf(lbuf, 32, &llen, ':');
            llen += int2string(j + 1, lbuf + llen, 32 - llen);
            char2buf(lbuf, 32, &llen, '=');
        }
        if (rm->array->is_string[prv_index] != 0) {
            char *text;
            int4 len;
            get_matrix_string(rm, prv_index, &text, &len);
            char *sbuf = (char *) malloc(len + 2);
            if (sbuf == NULL) {
                print_wide(lbuf, llen, "<Low Mem>", 9);
            } else {
                sbuf[0] = '"';
                memcpy(sbuf + 1, text, len);
                sbuf[len + 1] = '"';
                print_wide(lbuf, llen, sbuf, len + 2);
                free(sbuf);
            }
        } else {
            rlen = easy_phloat2string(rm->array->data[prv_index],
                                        rbuf, 100, 0);
            print_wide(lbuf, llen, rbuf, rlen);
        }
    } else if (prv_var->type == TYPE_COMPLEXMATRIX) {
        vartype_complexmatrix *cm = (vartype_complexmatrix *) prv_var;
        sz = cm->rows * cm->columns;
        if (prv_prreg) {
            char2buf(lbuf, 32, &llen, 'R');
            if (prv_index < 10)
                char2buf(lbuf, 32, &llen, '0');
            llen += int2string(prv_index, lbuf + llen, 32 - llen);
            char2buf(lbuf, 32, &llen, '=');
        } else {
            i = prv_index / cm->columns;
            j = prv_index % cm->columns;
            llen = int2string(i + 1, lbuf, 32);
            char2buf(lbuf, 32, &llen, ':');
            llen += int2string(j + 1, lbuf + llen, 32 - llen);
            char2buf(lbuf, 32, &llen, '=');
        }
        vartype_complex cpx;
        cpx.type = TYPE_COMPLEX;
        cpx.re = cm->array->data[2 * prv_index];
        cpx.im = cm->array->data[2 * prv_index + 1];
        rlen = vartype2string((vartype *) &cpx, rbuf, 100);
        print_wide(lbuf, llen, rbuf, rlen);
    } else /* prv_var->type == TYPE_LIST */ {
        vartype_list *list = (vartype_list *) prv_var;
        i = prv_index;
        sz = list->size;
        llen = int2string(i + 1, lbuf, 32);
        char2buf(lbuf, 32, &llen, '=');
        vartype *v = list->array->data[i];
        if (v->type == TYPE_STRING || v->type == TYPE_EQUATION) {
            char *text;
            int4 len;
            char d;
            if (v->type == TYPE_STRING) {
                vartype_string *s = (vartype_string *) v;
                text = s->txt();
                len = s->length;
                d = '"';
            } else {
                vartype_equation *eq = (vartype_equation *) v;
                text = eq->data->text;
                len = eq->data->length;
                d = eq->data->compatMode ? '`' : '\'';
            }
            char *sbuf = (char *) malloc(len + 2);
            if (sbuf == NULL) {
                print_wide(lbuf, llen, "<Low Mem>", 9);
            } else {
                sbuf[0] = d;
                memcpy(sbuf + 1, text, len);
                sbuf[len + 1] = d;
                print_wide(lbuf, llen, sbuf, len + 2);
                free(sbuf);
            }
        } else {
            rlen = vartype2string(v, rbuf, 100);
            print_wide(lbuf, llen, rbuf, rlen);
        }
    }

    if (++prv_index < sz)
        return ERR_INTERRUPTIBLE;
    else {
        set_annunciators(-1, -1, 0, -1, -1, -1);
        return ERR_NONE;
    }
}

int docmd_prreg(arg_struct *arg) {
    vartype *regs = recall_var("REGS", 4);
    if (regs == NULL)
        return ERR_NONEXISTENT;
    if (!flags.f.printer_enable && program_running())
        return ERR_NONE;
    if (!flags.f.printer_exists)
        return ERR_PRINTING_IS_DISABLED;
    set_annunciators(-1, -1, 1, -1, -1, -1);
    print_text(NULL, 0, true);
    prv_var = regs;
    prv_prreg = true;
    prv_index = 0;
    mode_interruptible = prv_worker;
    mode_stoppable = true;
    return ERR_INTERRUPTIBLE;
}

int docmd_prstk(arg_struct *arg) {
    char buf[100];
    int len;
    // arg == NULL if we're called to do TRACE mode auto-print
    if (arg != NULL && !flags.f.printer_enable && program_running())
        return ERR_NONE;
    if (!flags.f.printer_exists)
        return ERR_PRINTING_IS_DISABLED;
    set_annunciators(-1, -1, 1, -1, -1, -1);
    if (arg != NULL)
        print_text(NULL, 0, true);
    if (flags.f.big_stack) {
        if (sp == -1)
            print_text("<Empty Stack>", 13, true);
        else {
            char nbuf[8];
            int nlen;
            for (int i = 0; i <= sp; i++) {
                nlen = uint2string(sp + 1 - i, nbuf, 7);
                char2buf(nbuf, 8, &nlen, '=');
                if (stack[i]->type == TYPE_STRING) {
                    vartype_string *s = (vartype_string *) stack[i];
                    char *sbuf = (char *) malloc(s->length + 2);
                    if (sbuf == NULL) {
                        print_wide(nbuf, nlen, "<Low Mem>", 9);
                    } else {
                        sbuf[0] = '"';
                        memcpy(sbuf + 1, s->txt(), s->length);
                        sbuf[s->length + 1] = '"';
                        print_wide(nbuf, nlen, sbuf, s->length + 2);
                        free(sbuf);
                    }
                } else {
                    len = vartype2string(stack[i], buf, 100);
                    print_wide(nbuf, nlen, buf, len);
                }
            }
        }
    } else {
        const int index[] = { REG_T, REG_Z, REG_Y, REG_X };
        const char * const title[] = { "T=", "Z=", "Y=", "X=" };
        for (int i = 0; i < 4; i++) {
            if (stack[index[i]]->type == TYPE_STRING) {
                vartype_string *s = (vartype_string *) stack[index[i]];
                char *sbuf = (char *) malloc(s->length + 2);
                if (sbuf == NULL) {
                    print_wide(title[i], 2, "<Low Mem>", 9);
                } else {
                    sbuf[0] = '"';
                    memcpy(sbuf + 1, s->txt(), s->length);
                    sbuf[s->length + 1] = '"';
                    print_wide(title[i], 2, sbuf, s->length + 2);
                    free(sbuf);
                }
            } else {
                len = vartype2string(stack[index[i]], buf, 100);
                print_wide(title[i], 2, buf, len);
            }
        }
    }
    set_annunciators(-1, -1, 0, -1, -1, -1);
    return ERR_NONE;
}

int docmd_pra(arg_struct *arg) {
    // arg == NULL if we're called to do TRACE mode auto-print
    if (arg != NULL && !flags.f.printer_enable && program_running())
        return ERR_NONE;
    else
        return alpha_print_helper(reg_alpha, reg_alpha_length);
}

int docmd_prx(arg_struct *arg) {
    // arg == NULL if we're called to do TRACE mode auto-print
    if (arg != NULL && !flags.f.printer_enable && program_running())
        return ERR_NONE;
    if (!flags.f.printer_exists)
        return ERR_PRINTING_IS_DISABLED;
    else {
        set_annunciators(-1, -1, 1, -1, -1, -1);
        if (stack[sp]->type == TYPE_STRING || stack[sp]->type == TYPE_EQUATION) {
            const char *text;
            int length;
            char d;
            if (stack[sp]->type == TYPE_STRING) {
                vartype_string *s = (vartype_string *) stack[sp];
                text = s->txt();
                length = s->length;
                d = '"';
            } else {
                vartype_equation *eq = (vartype_equation *) stack[sp];
                equation_data *eqd = eq->data;
                text = eqd->text;
                length = eqd->length;
                d = '\'';
            }
            char *lbuf = (char *) malloc(length + 2);
            if (lbuf == NULL) {
                set_annunciators(-1, -1, 0, -1, -1, -1);
                return ERR_INSUFFICIENT_MEMORY;
            }
            lbuf[0] = d;
            memcpy(lbuf + 1, text, length);
            lbuf[length + 1] = d;
            print_right(lbuf, length + 2, "***", 3);
            free(lbuf);
        } else if (stack[sp]->type == TYPE_UNIT) {
            char buf[100];
            int len;
            vartype_unit *u = (vartype_unit *) stack[sp];
            u->type = TYPE_REAL;
            len = vartype2string((vartype *) u, buf, 100);
            u->type = TYPE_UNIT;
            char *text = (char *) malloc(len + 1 + u->length);
            if (text == NULL) {
                set_annunciators(-1, -1, 0, -1, -1, -1);
                return ERR_INSUFFICIENT_MEMORY;
            }
            memcpy(text, buf, len);
            text[len] = '_';
            memcpy(text + len + 1, u->text, u->length);
            bool left = len > (flags.f.double_wide_print ? 12 : 24);
            print_lines(text, len + 1 + u->length, left);
            free(text);
        } else {
            char buf[100];
            int len;
            len = vartype2string(stack[sp], buf, 100);
            if (stack[sp]->type == TYPE_REAL)
                print_right(buf, len, "***", 3);
            else {
                /* Normally we print X right-justified, but if it doesn't fit on
                * one line, we print it left-justified, because having the excess
                * go near the right margin looks weird and confusing.
                */
                bool left = len > (flags.f.double_wide_print ? 12 : 24);
                print_lines(buf, len, left);
            }
        }

        if (arg != NULL && (stack[sp]->type == TYPE_REALMATRIX
                            || stack[sp]->type == TYPE_COMPLEXMATRIX
                            || stack[sp]->type == TYPE_LIST
                            && ((vartype_list *) stack[sp])->size > 0)) {
            prv_var = stack[sp];
            prv_prreg = false;
            prv_index = 0;
            mode_interruptible = prv_worker;
            mode_stoppable = true;
            return ERR_INTERRUPTIBLE;
        } else {
            set_annunciators(-1, -1, 0, -1, -1, -1);
            return ERR_NONE;
        }
    }
}

static int prusr_state;
static int prusr_index;
static int prusr_worker(bool interrupted);

int docmd_prusr(arg_struct *arg) {
    if (!flags.f.printer_enable && program_running())
        return ERR_NONE;
    if (!flags.f.printer_exists)
        return ERR_PRINTING_IS_DISABLED;
    else {
        set_annunciators(-1, -1, 1, -1, -1, -1);
        print_text(NULL, 0, true);
        prusr_state = arg == NULL ? 2 : 0;
        prusr_index = cwd->vars_count - 1;
        mode_interruptible = prusr_worker;
        mode_stoppable = true;
        return ERR_INTERRUPTIBLE;
    }
}

static int prusr_worker(bool interrupted) {
    if (interrupted) {
        set_annunciators(-1, -1, 0, -1, -1, -1);
        return ERR_STOP;
    }

    if (prusr_state != 1) {
        if (prusr_index < 0) {
            if (prusr_state == 2) {
                set_annunciators(-1, -1, 0, -1, -1, -1);
                return ERR_NONE;
            } else {
                if (cwd->vars_count > 0)
                    print_text(NULL, 0, true);
                prusr_state = 1;
                prusr_index = 0;
                goto state1;
            }
        }
        char lbuf[8];
        int llen = 0;
        string2buf(lbuf, 8, &llen, cwd->vars[prusr_index].name,
                                   cwd->vars[prusr_index].length);
        char2buf(lbuf, 8, &llen, '=');
        if (cwd->vars[prusr_index].value->type == TYPE_STRING) {
            vartype_string *s = (vartype_string *) cwd->vars[prusr_index].value;
            char *sbuf = (char *) malloc(s->length + 2);
            if (sbuf == NULL) {
                print_wide(lbuf, llen, "<Low Mem>", 9);
            } else {
                sbuf[0] = '"';
                memcpy(sbuf + 1, s->txt(), s->length);
                sbuf[s->length + 1] = '"';
                print_wide(lbuf, llen, sbuf, s->length + 2);
                free(sbuf);
            }
        } else {
            char rbuf[100];
            int rlen = vartype2string(cwd->vars[prusr_index].value, rbuf, 100);
            print_wide(lbuf, llen, rbuf, rlen);
        }
        prusr_index--;
    } else {
        char buf[13];
        int len;
        state1:
        len = 0;
        if (prusr_index >= cwd->labels_count) {
            set_annunciators(-1, -1, 0, -1, -1, -1);
            return ERR_NONE;
        }
        if (cwd->labels[prusr_index].length == 0) {
            if (prusr_index == cwd->labels_count - 1)
                string2buf(buf, 13, &len, ".END.", 5);
            else
                string2buf(buf, 13, &len, "END", 3);
        } else {
            string2buf(buf, 13, &len, "LBL \"", 5);
            string2buf(buf, 13, &len, cwd->labels[prusr_index].name,
                                      cwd->labels[prusr_index].length);
            char2buf(buf, 13, &len, '"');
        }
        print_text(buf, len, true);
        prusr_index++;
    }
    return ERR_INTERRUPTIBLE;
}

int docmd_list(arg_struct *arg) {
    if (arg->type != ARGTYPE_NUM)
        return ERR_INVALID_TYPE;
    if (arg->val.num == 0)
        return ERR_NONE;
    if (!flags.f.printer_exists)
        return ERR_PRINTING_IS_DISABLED;
    return print_program(current_prgm, pc, arg->val.num, false);
}

int docmd_adv(arg_struct *arg) {
    if (flags.f.printer_exists
            && (flags.f.printer_enable || !program_running())) {
        set_annunciators(-1, -1, 1, -1, -1, -1);
        print_text(NULL, 0, true);
        set_annunciators(-1, -1, 0, -1, -1, -1);
    }
    return ERR_NONE;
}

int docmd_prlcd(arg_struct *arg) {
    if (!flags.f.printer_enable && program_running())
        return ERR_NONE;
    if (!flags.f.printer_exists)
        return ERR_PRINTING_IS_DISABLED;
    else {
        set_annunciators(-1, -1, 1, -1, -1, -1);
        int err = print_display();
        set_annunciators(-1, -1, 0, -1, -1, -1);
        return err;
    }
}

int docmd_delay(arg_struct *arg) {
    phloat x = ((vartype_real *) stack[sp])->x;
    if (x < 0)
        x = -x;
    if (x >= 1.95)
        return ERR_INVALID_DATA;
    else
        /* We don't actually use the delay value... */
        return ERR_NONE;
}

int docmd_pon(arg_struct *arg) {
    flags.f.printer_exists = 1;
    flags.f.printer_enable = 1;
    return ERR_NONE;
}

int docmd_poff(arg_struct *arg) {
    flags.f.printer_exists = 0;
    flags.f.printer_enable = 0;
    return ERR_NONE;
}

int docmd_man(arg_struct *arg) {
    flags.f.trace_print = 0;
    flags.f.normal_print = 0;
    return ERR_NONE;
}

int docmd_norm(arg_struct *arg) {
    flags.f.trace_print = 0;
    flags.f.normal_print = 1;
    return ERR_NONE;
}

int docmd_trace(arg_struct *arg) {
    flags.f.trace_print = 1;
    flags.f.normal_print = 0;
    return ERR_NONE;
}

int docmd_gto(arg_struct *arg) {
    int running = program_running();
    if (!running)
        clear_all_rtns();

    if (arg->type == ARGTYPE_NUM || arg->type == ARGTYPE_STK
                                 || arg->type == ARGTYPE_LCLBL) {
        if (!running || arg->target == -1)
            arg->target = find_local_label(arg);
        if (arg->target == -2)
            return ERR_LABEL_NOT_FOUND;
        else {
            pc = arg->target;
            move_prgm_highlight(1);
            return ERR_NONE;
        }
    }

    if (arg->type == ARGTYPE_STR) {
        pgm_index new_prgm;
        int4 new_pc;
        if (find_global_label(arg, &new_prgm, &new_pc)) {
            current_prgm = new_prgm;
            pc = new_pc;
            move_prgm_highlight(1);
            return ERR_NONE;
        } else
            return ERR_LABEL_NOT_FOUND;
    }

    if (arg->type == ARGTYPE_IND_NUM || arg->type == ARGTYPE_IND_STK
            || arg->type == ARGTYPE_IND_STR) {
        int err = resolve_ind_arg(arg);
        if (err != ERR_NONE)
            return err;
        if (arg->type == ARGTYPE_NUM) {
            int4 target_pc = find_local_label(arg);
            if (target_pc == -2)
                return ERR_LABEL_NOT_FOUND;
            else {
                pc = target_pc;
                move_prgm_highlight(1);
                return ERR_NONE;
            }
        } else {
            pgm_index newprgm;
            int4 newpc;
            if (find_global_label(arg, &newprgm, &newpc)) {
                current_prgm = newprgm;
                pc = newpc;
                move_prgm_highlight(1);
                return ERR_NONE;
            } else
                return ERR_LABEL_NOT_FOUND;
        }
    }

    if (arg->type == ARGTYPE_LBLINDEX) {
        int labelindex = arg->val.num;
        directory *dir = get_dir(arg->target);
        current_prgm.set(dir->id, dir->labels[labelindex].prgm);
        pc = dir->labels[labelindex].pc;
        move_prgm_highlight(1);
        cwd = dir;
        return ERR_NONE;
    }

    return ERR_INTERNAL_ERROR;
}

int docmd_number(arg_struct *arg) {
    if (p_isnan(arg->val_d)) {
        if (memcmp(&arg->val_d, &NAN_1_PHLOAT, sizeof(phloat)) == 0)
            return ERR_NUMBER_TOO_LARGE;
        else if (memcmp(&arg->val_d, &NAN_2_PHLOAT, sizeof(phloat)) == 0)
            return ERR_NUMBER_TOO_SMALL;
        else
            return ERR_INTERNAL_ERROR;
    }
    vartype *new_x = new_real(arg->val_d);
    if (new_x == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    int err = recall_result_silently(new_x);
    if (err == ERR_NONE)
        print_stack_trace();
    return err;
}

int docmd_string(arg_struct *arg) {
    int append = arg->length > 0 && arg->val.text[0] == 127;
    if (append) {
        append_alpha_string(arg->val.text + 1, arg->length - 1, 0);
        if (flags.f.trace_print && flags.f.printer_exists)
            docmd_pra(NULL);
    } else {
        reg_alpha_length = 0;
        append_alpha_string(arg->val.text, arg->length, 0);
    }
    return ERR_NONE;
}

int docmd_gtodot(arg_struct *arg) {
    if (arg->type == ARGTYPE_NUM) {
        clear_all_rtns();
        pc = line2pc(arg->val.num);
        move_prgm_highlight(1);
        return ERR_NONE;
    } else if (arg->type == ARGTYPE_STR) {
        pgm_index new_prgm;
        int4 new_pc;
        if (find_global_label(arg, &new_prgm, &new_pc)) {
            clear_all_rtns();
            current_prgm = new_prgm;
            pc = new_pc;
            move_prgm_highlight(1);
            cwd = get_dir(current_prgm.dir);
            return ERR_NONE;
        } else
            return ERR_LABEL_NOT_FOUND;
    } else if (arg->type == ARGTYPE_LBLINDEX) {
        clear_all_rtns();
        int labelindex = arg->val.num;
        directory *dir = get_dir(arg->target);
        current_prgm.set(dir->id, dir->labels[labelindex].prgm);
        pc = dir->labels[labelindex].pc;
        move_prgm_highlight(1);
        cwd = dir;
        return ERR_NONE;
    } else
        return ERR_INVALID_TYPE;
}

int docmd_gtodotdot(arg_struct *arg) {
    goto_dot_dot(false);
    return ERR_NONE;
}

int docmd_stop(arg_struct *arg) {
    return ERR_STOP;
}

int docmd_newmat(arg_struct *arg) {
    vartype *m;

    int4 row, col;
    if (!dim_to_int4(stack[sp - 1], &row))
        return ERR_DIMENSION_ERROR;
    if (!dim_to_int4(stack[sp], &col))
        return ERR_DIMENSION_ERROR;

    m = new_realmatrix(row + 1, col + 1);
    if (m == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    else
        return binary_result(m);
}

int docmd_rup(arg_struct *arg) {
    if (sp > 0) {
        vartype *temp = stack[0];
        memmove(stack, stack + 1, sp * sizeof(vartype *));
        stack[sp] = temp;
    }
    print_trace();
    return ERR_NONE;
}

int docmd_real_t(arg_struct *arg) {
    return stack[sp]->type == TYPE_REAL ? ERR_YES : ERR_NO;
}

int docmd_cpx_t(arg_struct *arg) {
    return stack[sp]->type == TYPE_COMPLEX ? ERR_YES : ERR_NO;
}

int docmd_str_t(arg_struct *arg) {
    return stack[sp]->type == TYPE_STRING ? ERR_YES : ERR_NO;
}

int docmd_mat_t(arg_struct *arg) {
    return stack[sp]->type == TYPE_REALMATRIX
            || stack[sp]->type == TYPE_COMPLEXMATRIX ? ERR_YES : ERR_NO;
}

int docmd_dim_t(arg_struct *arg) {
    int4 rows, columns;
    if (stack[sp]->type == TYPE_REALMATRIX) {
        rows = ((vartype_realmatrix *) stack[sp])->rows;
        columns = ((vartype_realmatrix *) stack[sp])->columns;
    } else {
        rows = ((vartype_complexmatrix *) stack[sp])->rows;
        columns = ((vartype_complexmatrix *) stack[sp])->columns;
    }
    vartype *new_y = new_real(rows);
    vartype *new_x = new_real(columns);
    if (new_x == NULL || new_y == NULL) {
        free_vartype(new_x);
        free_vartype(new_y);
        return ERR_INSUFFICIENT_MEMORY;
    }
    return unary_two_results(new_x, new_y);
}

static int assign_helper(int num, arg_struct *arg) {
    assign_custom_key(num, arg->val.text, arg->length);
    flags.f.local_label = 0;
    return ERR_NONE;
}

int docmd_asgn01(arg_struct *arg) {
    return assign_helper(1, arg);
}

int docmd_asgn02(arg_struct *arg) {
    return assign_helper(2, arg);
}

int docmd_asgn03(arg_struct *arg) {
    return assign_helper(3, arg);
}

int docmd_asgn04(arg_struct *arg) {
    return assign_helper(4, arg);
}

int docmd_asgn05(arg_struct *arg) {
    return assign_helper(5, arg);
}

int docmd_asgn06(arg_struct *arg) {
    return assign_helper(6, arg);
}

int docmd_asgn07(arg_struct *arg) {
    return assign_helper(7, arg);
}

int docmd_asgn08(arg_struct *arg) {
    return assign_helper(8, arg);
}

int docmd_asgn09(arg_struct *arg) {
    return assign_helper(9, arg);
}

int docmd_asgn10(arg_struct *arg) {
    return assign_helper(10, arg);
}

int docmd_asgn11(arg_struct *arg) {
    return assign_helper(11, arg);
}

int docmd_asgn12(arg_struct *arg) {
    return assign_helper(12, arg);
}

int docmd_asgn13(arg_struct *arg) {
    return assign_helper(13, arg);
}

int docmd_asgn14(arg_struct *arg) {
    return assign_helper(14, arg);
}

int docmd_asgn15(arg_struct *arg) {
    return assign_helper(15, arg);
}

int docmd_asgn16(arg_struct *arg) {
    return assign_helper(16, arg);
}

int docmd_asgn17(arg_struct *arg) {
    return assign_helper(17, arg);
}

int docmd_asgn18(arg_struct *arg) {
    return assign_helper(18, arg);
}

int docmd_on(arg_struct *arg) {
    shell_always_on(1);
    return ERR_NONE;
}

int docmd_off(arg_struct *arg) {
    shell_always_on(0);
#ifdef IPHONE
    if (!off_enabled()) {
        squeak();
        return ERR_STOP;
    }
#endif
    if (program_running() && no_keystrokes_yet)
        return ERR_SUSPICIOUS_OFF;
    set_running(false);
    quitting = true;
    shell_powerdown();
    return ERR_NONE;
}

int docmd_key1g(arg_struct *arg) {
    assign_prgm_key(1, true, arg);
    return ERR_NONE;
}

int docmd_key2g(arg_struct *arg) {
    assign_prgm_key(2, true, arg);
    return ERR_NONE;
}

int docmd_key3g(arg_struct *arg) {
    assign_prgm_key(3, true, arg);
    return ERR_NONE;
}

int docmd_key4g(arg_struct *arg) {
    assign_prgm_key(4, true, arg);
    return ERR_NONE;
}

int docmd_key5g(arg_struct *arg) {
    assign_prgm_key(5, true, arg);
    return ERR_NONE;
}

int docmd_key6g(arg_struct *arg) {
    assign_prgm_key(6, true, arg);
    return ERR_NONE;
}

int docmd_key7g(arg_struct *arg) {
    assign_prgm_key(7, true, arg);
    return ERR_NONE;
}

int docmd_key8g(arg_struct *arg) {
    assign_prgm_key(8, true, arg);
    return ERR_NONE;
}

int docmd_key9g(arg_struct *arg) {
    assign_prgm_key(9, true, arg);
    return ERR_NONE;
}

int docmd_key1x(arg_struct *arg) {
    assign_prgm_key(1, false, arg);
    return ERR_NONE;
}

int docmd_key2x(arg_struct *arg) {
    assign_prgm_key(2, false, arg);
    return ERR_NONE;
}

int docmd_key3x(arg_struct *arg) {
    assign_prgm_key(3, false, arg);
    return ERR_NONE;
}

int docmd_key4x(arg_struct *arg) {
    assign_prgm_key(4, false, arg);
    return ERR_NONE;
}

int docmd_key5x(arg_struct *arg) {
    assign_prgm_key(5, false, arg);
    return ERR_NONE;
}

int docmd_key6x(arg_struct *arg) {
    assign_prgm_key(6, false, arg);
    return ERR_NONE;
}

int docmd_key7x(arg_struct *arg) {
    assign_prgm_key(7, false, arg);
    return ERR_NONE;
}

int docmd_key8x(arg_struct *arg) {
    assign_prgm_key(8, false, arg);
    return ERR_NONE;
}

int docmd_key9x(arg_struct *arg) {
    assign_prgm_key(9, false, arg);
    return ERR_NONE;
}

int docmd_vmsto(arg_struct *arg) {
    /* STO variant that is invoked from a VARMENU */

    /* If overwriting an existing value that has a unit, with a new value
     * without a unit, apply the old unit to the new value. Note that this
     * means that if the user specifically wants to replace a value with a unit
     * with a value without one, they'll have to use the regular STO, or clear
     * the existing variable first.
     */
    if (arg->type != ARGTYPE_STR)
        return ERR_INTERNAL_ERROR;
    vartype *prev_x = NULL;
    if (stack[sp]->type == TYPE_REAL) {
        bool writable;
        vartype *v = recall_var(arg->val.text, arg->length, &writable);
        if (v != NULL && writable && v->type == TYPE_UNIT) {
            v = dup_vartype(v);
            if (v == NULL)
                return ERR_INSUFFICIENT_MEMORY;
            ((vartype_unit *) v)->x = ((vartype_real *) stack[sp])->x;
            prev_x = stack[sp];
            stack[sp] = v;
        }
    }

    int err = docmd_sto(arg);
    if (err == ERR_NONE)
        err = view_helper(arg, 0);
    if (err == ERR_NONE) {
        free_vartype(prev_x);
        mode_varmenu = true;
    } else if (prev_x != NULL) {
        free_vartype(stack[sp]);
        stack[sp] = prev_x;
    }
    return err;
}

int docmd_vmsto2(arg_struct *arg) {
    /* Special-purpose STO variant that is invoked from the Solver's VARMENU.
     * It saves the previous value of the target variable; this feature is used
     * by docmd_vmsolve() to provide the second initial guess (the first is
     * taken from the target variable, that is, the variable named as SOLVE's
     * parameter).
     */
    vartype *v;
    if (arg->type != ARGTYPE_STR)
        return ERR_INVALID_TYPE;
    v = recall_var(arg->val.text, arg->length);
    if (v == NULL || (v->type != TYPE_REAL && v->type != TYPE_UNIT))
        remove_shadow(arg->val.text, arg->length);
    else
        put_shadow(arg->val.text, arg->length, v);

    return docmd_vmsto(arg);
}

int docmd_sigma_reg(arg_struct *arg) {
    if (arg->type == ARGTYPE_IND_NUM
            || arg->type == ARGTYPE_IND_STK
            || arg->type == ARGTYPE_IND_STR) {
        int err = resolve_ind_arg(arg);
        if (err != ERR_NONE)
            return err;
    }
    if (arg->type == ARGTYPE_STR)
        return ERR_ALPHA_DATA_IS_INVALID;
    if (arg->type != ARGTYPE_NUM)
        return ERR_INVALID_DATA;
    mode_sigma_reg = (int4) arg->val.num;
    if (mode_sigma_reg < 0)
        mode_sigma_reg = -mode_sigma_reg;
    return ERR_NONE;
}

int docmd_sigma_reg_t(arg_struct *arg) {
    vartype *v = new_real(mode_sigma_reg);
    if (v == NULL)
        return ERR_INSUFFICIENT_MEMORY;
    return recall_result(v);
}

int docmd_cld(arg_struct *arg) {
    if (mode_message_lines > 0 && mode_goose >= 0)
        mode_goose = -1 - mode_goose;
    clear_message();
    return ERR_NONE;
}
