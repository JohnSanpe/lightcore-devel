/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright(c) 2022 Sanpe <sanpeqf@gmail.com>
 */

#include <math.h>
#include <export.h>

static const int16_t sin16_table[] = {
    0,
    572,   1144,  1715,  2286,  2856,  3425,
    3993,  4560,  5126,  5690,  6252,  6813,
    7371,  7927,  8481,  9032,  9580,  10126,
    10668, 11207, 11743, 12275, 12803, 13328,
    13848, 14364, 14876, 15383, 15886, 16383,
    16876, 17364, 17846, 18323, 18794, 19260,
    19720, 20173, 20621, 21062, 21497, 21925,
    22347, 22762, 23170, 23571, 23964, 24351,
    24730, 25101, 25465, 25821, 26169, 26509,
    26841, 27165, 27481, 27788, 28087, 28377,
    28659, 28932, 29196, 29451, 29697, 29934,
    30162, 30381, 30591, 30791, 30982, 31163,
    31335, 31498, 31650, 31794, 31927, 32051,
    32165, 32269, 32364, 32448, 32523, 32587,
    32642, 32687, 32722, 32747, 32762, 32767,
};

static const int32_t sin32_table[] = {
    0,
    37478757,   74946098,   112390610,  149800887,  187165532,
    224473166,  261712422,  298871958,  335940456,  372906622,
    409759197,  446486956,  483078711,  519523315,  555809667,
    591926714,  627863455,  663608942,  699152288,  734482665,
    769589311,  804461533,  839088709,  873460290,  907565806,
    941394869,  974937174,  1008182504, 1041120731, 1073741823,
    1106035843, 1137992954, 1169603421, 1200857616, 1231746017,
    1262259217, 1292387921, 1322122950, 1351455249, 1380375880,
    1408876036, 1436947035, 1464580326, 1491767491, 1518500249,
    1544770458, 1570570114, 1595891360, 1620726482, 1645067914,
    1668908244, 1692240207, 1715056698, 1737350766, 1759115620,
    1780344630, 1801031330, 1821169418, 1840752761, 1859775393,
    1878231518, 1896115517, 1913421940, 1930145516, 1946281152,
    1961823931, 1976769120, 1991112165, 2004848699, 2017974536,
    2030485679, 2042378316, 2053648825, 2064293773, 2074309916,
    2083694205, 2092443780, 2100555977, 2108028324, 2114858545,
    2121044560, 2126584484, 2131476630, 2135719507, 2139311823,
    2142252485, 2144540595, 2146175458, 2147156575, 2147483647,
};

#define GENERIC_SIN_OPS(name, type, table)      \
type sin##name(type angle)                      \
{                                               \
    angle = angle % 360;                        \
    if (angle < 0)                              \
        angle = 360 + angle;                    \
                                                \
    if (angle < 90)                             \
        return table[angle];                    \
                                                \
    if (angle >= 90 && angle < 180) {           \
        angle = 180 - angle;                    \
        return table[angle];                    \
    }                                           \
                                                \
    if (angle >= 180 && angle < 270) {          \
        angle = angle - 180;                    \
        return -table[angle];                   \
    }                                           \
                                                \
    /* angle >= 270 */                          \
    angle = 360 - angle;                        \
    return -table[angle];                       \
}                                               \
EXPORT_SYMBOL(sin##name);                       \
                                                \
type cos##name(type angle)                      \
{                                               \
    return sin##name(angle + 90);               \
}                                               \
EXPORT_SYMBOL(cos##name)

GENERIC_SIN_OPS(s16, int16_t, sin16_table);
GENERIC_SIN_OPS(s32, int32_t, sin32_table);