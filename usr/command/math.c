/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2022 Sanpe <sanpeqf@gmail.com>
 */

#include <kshell.h>
#include <initcall.h>
#include <ctype.h>
#include <string.h>
#include <kmalloc.h>

static void usage(void)
{
    kshell_printf("usage: math [option] \"grammar\" ...\n");
    kshell_printf("\t-h  display this message\n");
}

static inline int is_value(int c, bool env)
{
    return isxdigit(c) || isalpha(c) || (!env && c == '-');
}

static int get_value(char *end, char *varname)
{
    char *result, *walk = varname;
    char tmp = 0;

    for (;;) {
        if (!is_value(*walk, true) || walk == end) {
            tmp = *walk;
            *walk = '\0';
            break;
        }
        walk++;
    }

    result = kshell_getenv(varname);
    if (tmp)
        *walk = tmp;

    return strtoi(result ?: varname);
}

static void set_value(char *end, char *varname, int value, bool hex)
{
    char *walk;
    char tmp, str[32];

    for (tmp = 0, walk = varname; walk <= end; ++walk) {
        if (!isalpha(*walk) || walk == end) {
            tmp = *walk;
            *walk = '\0';
            break;
        }
    }

    if (kshell_getenv(varname)) {
        itoa(value, str, hex ? 16 : 10);
        kshell_setenv(varname, str, true);
    }

    if (tmp)
        *walk = tmp;
}

static char *prev_number(char *start, char *end, char *curr, int *number, char **region)
{
    bool alnum = false;
    char *varname;
    int result;

    for (result = 0; curr >= start; --curr) {
        if (alnum && (!is_value(*curr, false))) {
            result = get_value(end, varname);
            break;
        } else if (!alnum) {
            if (is_value(*curr, false)) {
                alnum = true;
                varname = curr;
            } else if (!isspace(*curr))
                return NULL;
        }
    }

    if (alnum && curr < start)
        result = get_value(end, start);

    if (region)
        *region = curr;

    if (number)
        *number = result;

    return alnum ? curr : NULL;
}

static char *next_number(char *end, char *curr, int *number, char **region)
{
    bool alnum = false;
    int result;

    for (result = 0; curr < end; ++curr) {
        if (alnum && (!is_value(*curr, false)))
            break;
        else if (!alnum) {
            if (is_value(*curr, false)) {
                alnum = true;
                if (region)
                    *region = curr;
                result = get_value(end, curr);
            } else if (!isspace(*curr))
                return NULL;
        }
    }

    if (number)
        *number = result;

    return alnum ? curr : NULL;
}

static state string_extension(char **start, char **end, char **curr, size_t size, size_t ext)
{
    char *nblock, *ncurr;
    size_t nsize;

    if (size >= ext) {
        memset(*curr, ' ', size);
        return -ENOERR;
    }

    nsize = (*end - *start) + (ext - size);
    nblock = kmalloc(nsize, GFP_KERNEL);
    if (!nblock)
        return -ENOMEM;

    ncurr = nblock + (*curr - *start);
    memcpy(nblock, *start, *curr - *start);
    memcpy(ncurr + ext, *curr + size, (*end - *curr) - size);

    if (ksize(start))
        kfree(start);

    *start = nblock;
    *end = nblock + nsize;
    *curr = ncurr;

    return -ENOERR;
}

static int math_parser(char *start, char *end, bool hex)
{
    int plen, result;
    char *curr;

    /* Principal priority */
    for (curr = start; (curr = strchr(curr, '(')); ++curr) {
        char *walk = curr + 1;
        unsigned int stack;

        for (stack = 1; *walk; ++walk) {
            if (*walk == '(')
                ++stack;
            else if (*walk == ')') {
                if (!--stack)
                    break;
            }
        }

        if (stack)
            return 0;

        result = math_parser(curr + 1, walk, hex);
        plen = snprintf(0, 0, "%d", result);
        string_extension(&start, &end, &curr, walk - curr + 1, plen + 1);
        snprintf(curr, plen + 1, "%d", result);
        curr[plen] = ' ';
    }

    /* Secondary priority */
    for (curr = start; curr < end; ++curr) {
        char *left = NULL, *right = NULL;
        size_t size, ext;

        if (!strncmp(curr, "++", 2) && (left = prev_number(start, end, curr - 1, &result, &right))) {
            /* variable++ */
            left++;
            set_value(end, left, result + 1, hex);
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = (curr + 2) - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = '\a';
            curr = left + max(size, ext) - 1;

        } else if (!strncmp(curr, "--", 2) && (left = prev_number(start, end, curr - 1, &result, &right))) {
            /* variable-- */
            left++;
            set_value(end, left, result - 1, hex);
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = (curr + 2) - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = '\a';
            curr = left + max(size, ext) - 1;

        } else if (!strncmp(curr, "++", 2) && (right = next_number(end, curr + 2, &result, &left))) {
            /* ++variable */
            set_value(end, left, result + 1, hex);
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &curr, (size = right - curr), (ext = plen + 1));
            snprintf(curr, plen + 1, "%d", result + 1);
            curr[plen] = '\a';
            curr += max(size, ext) - 1;

        } else if (!strncmp(curr, "--", 2) && (right = next_number(end, curr + 2, &result, &left))) {
            /* --variable */
            set_value(end, left, result - 1, hex);
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &curr, (size = right - curr), (ext = plen + 1));
            snprintf(curr, plen + 1, "%d", result - 1);
            curr[plen] = '\a';
            curr += max(size, ext) - 1;

        } else if (*curr == '+' && (right = next_number(end, curr + 1, &result, NULL))) {
            /* +variable */
            if (prev_number(start, end, curr - 1, NULL, NULL))
                continue;
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &curr, (size = right - curr), (ext = plen + 1));
            snprintf(curr, plen + 1, "%d", result);
            curr[plen] = '\a';
            curr += max(size, ext) - 1;

        } else if (*curr == '-' && (right = next_number(end, curr + 1, &result, NULL))) {
            /* -variable */
            if (prev_number(start, end, curr - 1, NULL, NULL))
                continue;
            plen = snprintf(0, 0, "%d", -result);
            string_extension(&start, &end, &curr, (size = right - curr), (ext = plen + 1));
            snprintf(curr, plen + 1, "%d", -result);
            curr[plen] = '\a';
            curr += max(size, ext) - 1;

        } else if (*curr == '~' && (right = next_number(end, curr + 1, &result, NULL))) {
            /* ~variable */
            plen = snprintf(0, 0, "%d", ~result);
            string_extension(&start, &end, &curr, (size = right - curr), (ext = plen + 1));
            snprintf(curr, plen + 1, "%d", ~result);
            curr[plen] = '\a';
            curr += max(size, ext) - 1;
        }
    }

    if (strnstr(start, "++", end - start) ||
        strnstr(start, "--", end - start) ||
        strnchr(start, end - start, '~'))
        return 0;

    for (curr = start; curr < end; ++curr) {
        if (*curr == '\a')
            *curr = ' ';
    }

    /* Tertiary priority */
    for (curr = start; curr < end; ++curr) {
        char *left = NULL, *right = NULL;
        size_t size, ext;
        int rresult;

        if (*curr == '*' && (left = prev_number(start, end, curr - 1, &result, NULL))
                         && (right = next_number(end, curr + 1, &rresult, NULL))) {
            /* variable * variable */
            left++;
            result *= rresult;
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;

        } else if (*curr == '/' && (left = prev_number(start, end, curr - 1, &result, NULL))
                                && (right = next_number(end, curr + 1, &rresult, NULL))) {
            /* variable / variable */
            if (!rresult)
                return 0;
            left++;
            result /= rresult;
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;

        } else if (*curr == '%' && (left = prev_number(start, end, curr - 1, &result, NULL))
                                && (right = next_number(end, curr + 1, &rresult, NULL))) {
            /* variable % variable */
            if (!rresult)
                return 0;
            left++;
            result %= rresult;
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;
        }
    }

    /* Quartus priority */
    for (curr = start; curr < end; ++curr) {
        char *left = NULL, *right = NULL;
        size_t size, ext;
        int rresult;

        if (*curr == '+' && (left = prev_number(start, end, curr - 1, &result, NULL))
                         && (right = next_number(end, curr + 1, &rresult, NULL))) {
            /* variable + variable */
            left++;
            result += rresult;
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;

        } else if (*curr == '-' && (left = prev_number(start, end, curr - 1, &result, NULL))
                                && (right = next_number(end, curr + 1, &rresult, NULL))) {
            /* variable - variable */
            left++;
            result -= rresult;
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;
        }
    }

    /* Fifth priority */
    for (curr = start; curr < end; ++curr) {
        char *left = NULL, *right = NULL;
        size_t size, ext;
        int rresult;

        if (!strncmp(curr, "<<", 2) && (left = prev_number(start, end, curr - 1, &result, NULL))
                                    && (right = next_number(end, curr + 2, &rresult, NULL))) {
            /* variable << variable */
            left++;
            result <<= rresult;
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;

        } else if (!strncmp(curr, ">>", 2) && (left = prev_number(start, end, curr - 1, &result, NULL))
                                           && (right = next_number(end, curr + 2, &rresult, NULL))) {
            /* variable >> variable */
            left++;
            result >>= rresult;
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;
        }
    }

    /* Sixth priority */
    for (curr = start; curr < end; ++curr) {
        char *left = NULL, *right = NULL;
        size_t size, ext;
        int rresult;

        if (!strncmp(curr, ">=", 2) && (left = prev_number(start, end, curr - 1, &result, NULL))
                                    && (right = next_number(end, curr + 2, &rresult, NULL))) {
            /* variable >= variable */
            left++;
            result = result >= rresult;
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;

        } else if (!strncmp(curr, "<=", 2) && (left = prev_number(start, end, curr - 1, &result, NULL))
                                           && (right = next_number(end, curr + 2, &rresult, NULL))) {
            /* variable <= variable */
            left++;
            result = result <= rresult;
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;

        } else if (*curr == '>' && (left = prev_number(start, end, curr - 1, &result, NULL))
                                && (right = next_number(end, curr + 1, &rresult, NULL))) {
            /* variable < variable */
            left++;
            result = result > rresult;
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;

        } else if (*curr == '<' && (left = prev_number(start, end, curr - 1, &result, NULL))
                                && (right = next_number(end, curr + 1, &rresult, NULL))) {
            /* variable < variable */
            left++;
            result = result < rresult;
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;
        }
    }

    /* Seventh priority */
    for (curr = start; curr < end; ++curr) {
        char *left = NULL, *right = NULL;
        size_t size, ext;
        int rresult;

        if (!strncmp(curr, "==", 2) && (left = prev_number(start, end, curr - 1, &result, NULL))
                                    && (right = next_number(end, curr + 2, &rresult, NULL))) {
            /* variable == variable */
            left++;
            result = result == rresult;
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;

        } else if (!strncmp(curr, "!=", 2) && (left = prev_number(start, end, curr - 1, &result, NULL))
                                           && (right = next_number(end, curr + 2, &rresult, NULL))) {
            /* variable != variable */
            left++;
            result = result != rresult;
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;
        }
    }

    /* Eighth priority */
    for (curr = start; curr < end; ++curr) {
        char *left = NULL, *right = NULL;
        size_t size, ext;
        int rresult;

        if (*curr == '&' && (left = prev_number(start, end, curr - 1, &result, NULL))
                         && (right = next_number(end, curr + 1, &rresult, NULL))) {
            /* variable & variable */
            left++;
            result &= rresult;
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;
        }
    }

    /* Ninth priority */
    for (curr = start; curr < end; ++curr) {
        char *left = NULL, *right = NULL;
        size_t size, ext;
        int rresult;

        if (*curr == '^' && (left = prev_number(start, end, curr - 1, &result, NULL))
                         && (right = next_number(end, curr + 1, &rresult, NULL))) {
            /* variable < variable */
            left++;
            result ^= rresult;
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;
        }
    }

    /* Tenth priority */
    for (curr = start; curr < end; ++curr) {
        char *left = NULL, *right = NULL;
        size_t size, ext;
        int rresult;

        if (*curr == '|' && (left = prev_number(start, end, curr - 1, &result, NULL))
                         && (right = next_number(end, curr + 1, &rresult, NULL))) {
            /* variable | variable */
            left++;
            result |= rresult;
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;
        }
    }

    /* Eleventh priority */
    for (curr = start; curr < end; ++curr) {
        char *left = NULL, *right = NULL;
        size_t size, ext;
        int rresult;

        if (!strncmp(curr, "&&", 2) && (left = prev_number(start, end, curr - 1, &result, NULL))
                                    && (right = next_number(end, curr + 2, &rresult, NULL))) {
            /* variable < variable */
            left++;
            result = result && rresult;
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;
        }
    }

    /* Twelfth priority */
    for (curr = start; curr < end; ++curr) {
        char *left = NULL, *right = NULL;
        size_t size, ext;
        int rresult;

        if (!strncmp(curr, "||", 2) && (left = prev_number(start, end, curr - 1, &result, NULL))
                                    && (right = next_number(end, curr + 2, &rresult, NULL))) {
            /* variable < variable */
            left++;
            result = result || rresult;
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;
        }
    }

    /* Thirteenth priority */

    /* Fourteenth priority */
    for (curr = start; curr < end; ++curr) {
        char *left = NULL, *right = NULL;
        size_t size, ext;
        int rresult;

        if (!strncmp(curr, "+=", 2) && (left = prev_number(start, end, curr - 1, &result, NULL))
                                    && (right = next_number(end, curr + 2, &rresult, NULL))) {
            /* variable += variable */
            left++;
            result += rresult;
            set_value(end, left, result, hex);
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;

        } else if (!strncmp(curr, "-=", 2) && (left = prev_number(start, end, curr - 1, &result, NULL))
                                           && (right = next_number(end, curr + 2, &rresult, NULL))) {
            /* variable -= variable */
            left++;
            result -= rresult;
            set_value(end, left, result, hex);
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;

        } else if (!strncmp(curr, "*=", 2) && (left = prev_number(start, end, curr - 1, &result, NULL))
                                           && (right = next_number(end, curr + 2, &rresult, NULL))) {
            /* variable *= variable */
            left++;
            result *= rresult;
            set_value(end, left, result, hex);
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;

        } else if (!strncmp(curr, "/=", 2) && (left = prev_number(start, end, curr - 1, &result, NULL))
                                           && (right = next_number(end, curr + 2, &rresult, NULL))) {
            /* variable /= variable */
            if (!rresult)
                return 0;
            left++;
            result /= rresult;
            set_value(end, left, result, hex);
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;

        } else if (!strncmp(curr, "%=", 2) && (left = prev_number(start, end, curr - 1, &result, NULL))
                                           && (right = next_number(end, curr + 2, &rresult, NULL))) {
            /* variable %= variable */
            if (!rresult)
                return 0;
            left++;
            result %= rresult;
            set_value(end, left, result, hex);
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;

        } else if (!strncmp(curr, "&=", 2) && (left = prev_number(start, end, curr - 1, &result, NULL))
                                           && (right = next_number(end, curr + 2, &rresult, NULL))) {
            /* variable &= variable */
            if (!rresult)
                return 0;
            left++;
            result &= rresult;
            set_value(end, left, result, hex);
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;

        } else if (!strncmp(curr, "^=", 2) && (left = prev_number(start, end, curr - 1, &result, NULL))
                                           && (right = next_number(end, curr + 2, &rresult, NULL))) {
            /* variable ^= variable */
            if (!rresult)
                return 0;
            left++;
            result ^= rresult;
            set_value(end, left, result, hex);
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;

        } else if (!strncmp(curr, "|=", 2) && (left = prev_number(start, end, curr - 1, &result, NULL))
                                           && (right = next_number(end, curr + 2, &rresult, NULL))) {
            /* variable |= variable */
            if (!rresult)
                return 0;
            left++;
            result |= rresult;
            set_value(end, left, result, hex);
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;

        } else if (!strncmp(curr, "<<=", 3) && (left = prev_number(start, end, curr - 1, &result, NULL))
                                            && (right = next_number(end, curr + 3, &rresult, NULL))) {
            /* variable <<= variable */
            if (!rresult)
                return 0;
            left++;
            result <<= rresult;
            set_value(end, left, result, hex);
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;

        } else if (!strncmp(curr, ">>=", 3) && (left = prev_number(start, end, curr - 1, &result, NULL))
                                            && (right = next_number(end, curr + 3, &rresult, NULL))) {
            /* variable >>= variable */
            if (!rresult)
                return 0;
            left++;
            result >>= rresult;
            set_value(end, left, result, hex);
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;

        } else if (*curr == '=' && (left = prev_number(start, end, curr - 1, &result, NULL))
                                && (right = next_number(end, curr + 1, &rresult, NULL))) {
            /* variable = variable */
            if (!rresult)
                return 0;
            left++;
            result = rresult;
            set_value(end, left, result, hex);
            plen = snprintf(0, 0, "%d", result);
            string_extension(&start, &end, &left, (size = right - left), (ext = plen + 1));
            snprintf(left, plen + 1, "%d", result);
            left[plen] = ' ';
            curr = left + max(size, ext) - 1;
        }
    }

    if (!next_number(end, start, &result, NULL))
        return 0;

    return result;
}

static state math_main(int argc, char *argv[])
{
    unsigned int count;
    bool xflag = false;
    int result = 0;

    for (count = 1; count < argc; ++count) {
        char *para = argv[count];

        if (para[0] != '-')
            break;

        switch (para[1]) {
            case 'x':
                xflag = true;
                break;

            case 'X':
                xflag = false;
                break;

            case 'h':
                goto usage;

            default:
                goto finish;
        }
    }

    if (argc == count)
        goto usage;

finish:
    for (; count < argc; ++count) {
        unsigned int len = strlen(argv[count]);
        result = math_parser(argv[count], argv[count] + len, xflag);
    }

    return result;

usage:
    usage();
    return -EINVAL;
}

static struct kshell_command math_cmd = {
    .name = "math",
    .desc = "environment variable calculation",
    .exec = math_main,
};

static state math_init(void)
{
    return kshell_register(&math_cmd);
}
kshell_initcall(math_init);
