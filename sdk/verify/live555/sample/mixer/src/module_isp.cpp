/*
 * module_isp.cpp- Sigmastar
 *
 * Copyright (C) 2018 Sigmastar Technology Corp.
 *
 * Author: XXXX <XXXX@sigmastar.com.cn>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include "module_common.h"
#include "module_config.h"
#include "mi_common.h"
#include "mi_common_datatype.h"
#include "mi_divp_datatype.h"
#include "mi_divp.h"
#include "mi_vif_datatype.h"
#include "mi_vif.h"
#include "mi_sensor.h"
#include "isp_cus3a_if.h"
#include "mid_vpe.h"

#if TARGET_CHIP_I5
#include "mi_isp_pretzel.h"
#include "mi_isp_pretzel_datatype.h"
#elif TARGET_CHIP_I6
#include <poll.h>
#include "mi_isp.h"
#include "mi_isp_datatype.h"
#elif TARGET_CHIP_I6E
#include <poll.h>
#include "mi_isp.h"
#include "mi_isp_datatype.h"
#elif TARGET_CHIP_I6B0
#include <poll.h>
#include "mi_isp.h"
#include "mi_isp_datatype.h"

#endif

#include "module_cus3a.h"

//bool b_openCUS3A_misc = false;
static MI_U8 g_MirrorFlipLevel = (MI_U8)LEVEL0;
extern BOOL g_bCusAEenable;
//static MI_S32 g_fifo_data[256];

struct _crosstalk
{
#if TARGET_CHIP_I5
    MI_BOOL enV2;
    MI_U16 u16ThresholdHigh;
    MI_U16 u16ThresholdLow;
    MI_U16 u16ThresholdV2;
    MI_U16 u8StrengthHigh;
    MI_U16 u8StrengthLow;
    MI_U16 u8StrengthV2;
#elif TARGET_CHIP_I6
    MI_U16 u16ThresholdOffsetV2;
    MI_U16 u16ThresholdV2;
    MI_U16 u8StrengthV2;
#elif TARGET_CHIP_I6E
    MI_U8 u8Strength;
    MI_U8 u8Threshold;
    MI_U16 u16Offset;
#elif TARGET_CHIP_I6B0
    MI_U8 u8Strength;
    MI_U8 u8Threshold;
    MI_U16 u16Offset;
#endif


};

struct _TestAWB
{
    MI_ISP_OP_TYPE_e eOpType;
    MI_S32 u16Bgain;
    MI_S32 u16Gbgain;
    MI_S32 u16Grgain;
    MI_S32 u16Rgain;
};

struct _blacklevel
{
    MI_U16 r;
    MI_U16 gb;
    MI_U16 gr;
    MI_U16 b;
};

struct _deflog
{
    MI_BOOL enable;

    #if (!TARGET_CHIP_I6E && !TARGET_CHIP_I6B0)
    MI_U16 strength;
    #endif
};

struct _AeAttr
{
    MI_U8 index;
    union
    {
        MI_U8 mode;
        MI_U32 luma;
    }y;
};

struct _NR3DAttr
{
    MI_U8 u8TfStr;
    MI_U8 u8TfStrEx;
    MI_U32 u8TfLut;
};

struct _AWBAttr
{
#if TARGET_CHIP_I5
    MI_U16 AreaScale;
#endif

    MI_U16 u16WhiteBgain;
    MI_U16 u16WhiteRgain;
    MI_U8 u8AreaSize;
};

struct _nrDespike
{
    MI_U16 DiffThdCornerCross;
    MI_U8 BlendRatio;
};

struct _nrLuma
{
#if TARGET_CHIP_I5
    MI_BOOL bEnLscReference;
    MI_U8 u8FilterLevel;
    MI_U8 u8BlendRatio;
#elif TARGET_CHIP_I6
    MI_U8 index;
#elif TARGET_CHIP_I6E
    MI_U8 index;
#elif TARGET_CHIP_I6B0
    MI_U8 index;
#endif
};

struct _hsv
{
    MI_S16 s16HueLut;
    MI_U16 u16SatLut;
};

struct _rgbir
{
    MI_BOOL bRemovelEn;
    MI_U16 Ratio_r;
    MI_U16 Ratio_g;
    MI_U16 Ratio_b;
};

struct _wdr
{
    MI_U8 u8BrightLimit;
    MI_U8 u8DarkLimit;
    MI_U16 u8Strength;
};

struct _loadiqbin
{
    const char *filepath;
    MI_U32 key;
};

struct _loadcali
{
    MI_U8 channel;
    MI_U8 caliItem;
    const char * filepath;
};

struct _ircut
{
    MI_BOOL isday = 0;
    MI_U8 boardtype = 0;
};
struct _cus3a_attr
{
    MI_BOOL mAeEnable;
    MI_BOOL mAwbEnable;
    MI_BOOL mAfEnable;
};

struct _cus3a
{
    MI_U8 cus3a_option;
    struct _cus3a_attr tCus3aAttr;
};

struct _ae_state
{
    MI_BOOL AeState;
};

struct _get_ae_state
{
    MI_U8 channel;
};

MI_U16 RGBGamma_L[3][256] = {
    {
        0,  20,  40,  61,  82, 102, 122, 140, 158, 175, 191, 206, 220, 234, 247, 261,
        273, 286, 298, 309, 321, 331, 342, 352, 362, 371, 380, 389, 397, 405, 413, 421,
        429, 436, 444, 451, 459, 466, 473, 479, 486, 493, 499, 505, 511, 517, 523, 529,
        535, 540, 545, 551, 556, 561, 566, 571, 576, 580, 585, 590, 595, 599, 604, 608,
        613, 617, 621, 625, 629, 633, 636, 640, 644, 647, 651, 654, 657, 661, 664, 667,
        670, 674, 677, 680, 683, 686, 689, 692, 695, 698, 701, 704, 708, 711, 714, 717,
        720, 723, 726, 729, 732, 735, 738, 741, 744, 746, 749, 752, 755, 758, 761, 763,
        766, 769, 772, 775, 777, 780, 783, 786, 789, 791, 794, 797, 799, 802, 804, 807,
        809, 812, 814, 816, 818, 821, 823, 825, 827, 829, 831, 833, 835, 837, 839, 841,
        843, 845, 847, 849, 851, 853, 855, 857, 859, 861, 863, 865, 867, 869, 870, 872,
        874, 876, 878, 879, 881, 883, 885, 886, 888, 890, 891, 893, 894, 896, 898, 899,
        901, 903, 905, 906, 908, 910, 911, 913, 915, 916, 918, 920, 922, 923, 925, 927,
        929, 930, 932, 934, 936, 938, 939, 941, 943, 945, 947, 949, 950, 952, 954, 955,
        957, 959, 960, 962, 963, 965, 966, 968, 969, 971, 972, 974, 975, 977, 978, 979,
        981, 982, 984, 985, 986, 988, 989, 991, 992, 993, 995, 996, 997, 999,1000,1001,
        1003,1004,1005,1007,1008,1009,1010,1012,1013,1014,1016,1017,1018,1019,1021,1023
    },
    {
        0,  20,  40,  61,  82, 102, 122, 140, 158, 175, 191, 206, 220, 234, 247, 261,
        273, 286, 298, 309, 321, 331, 342, 352, 362, 371, 380, 389, 397, 405, 413, 421,
        429, 436, 444, 451, 459, 466, 473, 479, 486, 493, 499, 505, 511, 517, 523, 529,
        535, 540, 545, 551, 556, 561, 566, 571, 576, 580, 585, 590, 595, 599, 604, 608,
        613, 617, 621, 625, 629, 633, 636, 640, 644, 647, 651, 654, 657, 661, 664, 667,
        670, 674, 677, 680, 683, 686, 689, 692, 695, 698, 701, 704, 708, 711, 714, 717,
        720, 723, 726, 729, 732, 735, 738, 741, 744, 746, 749, 752, 755, 758, 761, 763,
        766, 769, 772, 775, 777, 780, 783, 786, 789, 791, 794, 797, 799, 802, 804, 807,
        809, 812, 814, 816, 818, 821, 823, 825, 827, 829, 831, 833, 835, 837, 839, 841,
        843, 845, 847, 849, 851, 853, 855, 857, 859, 861, 863, 865, 867, 869, 870, 872,
        874, 876, 878, 879, 881, 883, 885, 886, 888, 890, 891, 893, 894, 896, 898, 899,
        901, 903, 905, 906, 908, 910, 911, 913, 915, 916, 918, 920, 922, 923, 925, 927,
        929, 930, 932, 934, 936, 938, 939, 941, 943, 945, 947, 949, 950, 952, 954, 955,
        957, 959, 960, 962, 963, 965, 966, 968, 969, 971, 972, 974, 975, 977, 978, 979,
        981, 982, 984, 985, 986, 988, 989, 991, 992, 993, 995, 996, 997, 999,1000,1001,
        1003,1004,1005,1007,1008,1009,1010,1012,1013,1014,1016,1017,1018,1019,1021,1023
    },
    {
        0,  20,  40,  61,  82, 102, 122, 140, 158, 175, 191, 206, 220, 234, 247, 261,
        273, 286, 298, 309, 321, 331, 342, 352, 362, 371, 380, 389, 397, 405, 413, 421,
        429, 436, 444, 451, 459, 466, 473, 479, 486, 493, 499, 505, 511, 517, 523, 529,
        535, 540, 545, 551, 556, 561, 566, 571, 576, 580, 585, 590, 595, 599, 604, 608,
        613, 617, 621, 625, 629, 633, 636, 640, 644, 647, 651, 654, 657, 661, 664, 667,
        670, 674, 677, 680, 683, 686, 689, 692, 695, 698, 701, 704, 708, 711, 714, 717,
        720, 723, 726, 729, 732, 735, 738, 741, 744, 746, 749, 752, 755, 758, 761, 763,
        766, 769, 772, 775, 777, 780, 783, 786, 789, 791, 794, 797, 799, 802, 804, 807,
        809, 812, 814, 816, 818, 821, 823, 825, 827, 829, 831, 833, 835, 837, 839, 841,
        843, 845, 847, 849, 851, 853, 855, 857, 859, 861, 863, 865, 867, 869, 870, 872,
        874, 876, 878, 879, 881, 883, 885, 886, 888, 890, 891, 893, 894, 896, 898, 899,
        901, 903, 905, 906, 908, 910, 911, 913, 915, 916, 918, 920, 922, 923, 925, 927,
        929, 930, 932, 934, 936, 938, 939, 941, 943, 945, 947, 949, 950, 952, 954, 955,
        957, 959, 960, 962, 963, 965, 966, 968, 969, 971, 972, 974, 975, 977, 978, 979,
        981, 982, 984, 985, 986, 988, 989, 991, 992, 993, 995, 996, 997, 999,1000,1001,
        1003,1004,1005,1007,1008,1009,1010,1012,1013,1014,1016,1017,1018,1019,1021,1023
    }
};

MI_U16 RGBGamma_N[3][256] = {
    {
        0,   8,  14,  24,  31,  40,  51,  59,  71,  83,  94, 104, 117, 126, 139, 155,
        169, 182, 195, 212, 225, 238, 254, 266, 281, 290, 301, 314, 325, 336, 346, 356,
        365, 374, 384, 393, 400, 410, 415, 424, 434, 441, 449, 457, 465, 471, 479, 487,
        494, 500, 508, 514, 520, 528, 533, 540, 546, 553, 559, 566, 571, 578, 583, 588,
        593, 598, 605, 609, 615, 619, 624, 629, 633, 638, 642, 647, 650, 654, 658, 661,
        665, 669, 672, 675, 679, 682, 686, 690, 693, 696, 700, 703, 706, 709, 712, 715,
        718, 721, 724, 728, 730, 733, 736, 740, 743, 745, 748, 751, 753, 756, 759, 762,
        765, 768, 770, 772, 775, 777, 781, 784, 786, 789, 792, 794, 797, 799, 802, 804,
        807, 809, 811, 814, 816, 818, 821, 824, 826, 828, 831, 833, 835, 837, 839, 842,
        844, 846, 848, 850, 852, 854, 856, 858, 859, 861, 863, 865, 866, 869, 871, 873,
        875, 876, 878, 880, 882, 883, 885, 886, 888, 890, 892, 893, 895, 897, 898, 900,
        901, 904, 905, 906, 908, 910, 911, 914, 916, 917, 919, 920, 922, 924, 926, 927,
        928, 930, 932, 933, 935, 936, 938, 939, 941, 943, 944, 946, 947, 949, 950, 952,
        954, 955, 958, 959, 961, 962, 964, 965, 967, 968, 969, 971, 972, 974, 976, 977,
        979, 980, 982, 983, 985, 986, 987, 989, 990, 992, 993, 995, 996, 997, 999,1002,
        1003,1004,1006,1007,1008,1010,1011,1012,1014,1015,1017,1018,1019,1021,1022,1023
    },
    {
        0,   8,  14,  24,  31,  40,  51,  59,  71,  83,  94, 104, 117, 126, 139, 155,
        169, 182, 195, 212, 225, 238, 254, 266, 281, 290, 301, 314, 325, 336, 346, 356,
        365, 374, 384, 393, 400, 410, 415, 424, 434, 441, 449, 457, 465, 471, 479, 487,
        494, 500, 508, 514, 520, 528, 533, 540, 546, 553, 559, 566, 571, 578, 583, 588,
        593, 598, 605, 609, 615, 619, 624, 629, 633, 638, 642, 647, 650, 654, 658, 661,
        665, 669, 672, 675, 679, 682, 686, 690, 693, 696, 700, 703, 706, 709, 712, 715,
        718, 721, 724, 728, 730, 733, 736, 740, 743, 745, 748, 751, 753, 756, 759, 762,
        765, 768, 770, 772, 775, 777, 781, 784, 786, 789, 792, 794, 797, 799, 802, 804,
        807, 809, 811, 814, 816, 818, 821, 824, 826, 828, 831, 833, 835, 837, 839, 842,
        844, 846, 848, 850, 852, 854, 856, 858, 859, 861, 863, 865, 866, 869, 871, 873,
        875, 876, 878, 880, 882, 883, 885, 886, 888, 890, 892, 893, 895, 897, 898, 900,
        901, 904, 905, 906, 908, 910, 911, 914, 916, 917, 919, 920, 922, 924, 926, 927,
        928, 930, 932, 933, 935, 936, 938, 939, 941, 943, 944, 946, 947, 949, 950, 952,
        954, 955, 958, 959, 961, 962, 964, 965, 967, 968, 969, 971, 972, 974, 976, 977,
        979, 980, 982, 983, 985, 986, 987, 989, 990, 992, 993, 995, 996, 997, 999,1002,
        1003,1004,1006,1007,1008,1010,1011,1012,1014,1015,1017,1018,1019,1021,1022,1023
    },
    {
        0,   8,  14,  24,  31,  40,  51,  59,  71,  83,  94, 104, 117, 126, 139, 155,
        169, 182, 195, 212, 225, 238, 254, 266, 281, 290, 301, 314, 325, 336, 346, 356,
        365, 374, 384, 393, 400, 410, 415, 424, 434, 441, 449, 457, 465, 471, 479, 487,
        494, 500, 508, 514, 520, 528, 533, 540, 546, 553, 559, 566, 571, 578, 583, 588,
        593, 598, 605, 609, 615, 619, 624, 629, 633, 638, 642, 647, 650, 654, 658, 661,
        665, 669, 672, 675, 679, 682, 686, 690, 693, 696, 700, 703, 706, 709, 712, 715,
        718, 721, 724, 728, 730, 733, 736, 740, 743, 745, 748, 751, 753, 756, 759, 762,
        765, 768, 770, 772, 775, 777, 781, 784, 786, 789, 792, 794, 797, 799, 802, 804,
        807, 809, 811, 814, 816, 818, 821, 824, 826, 828, 831, 833, 835, 837, 839, 842,
        844, 846, 848, 850, 852, 854, 856, 858, 859, 861, 863, 865, 866, 869, 871, 873,
        875, 876, 878, 880, 882, 883, 885, 886, 888, 890, 892, 893, 895, 897, 898, 900,
        901, 904, 905, 906, 908, 910, 911, 914, 916, 917, 919, 920, 922, 924, 926, 927,
        928, 930, 932, 933, 935, 936, 938, 939, 941, 943, 944, 946, 947, 949, 950, 952,
        954, 955, 958, 959, 961, 962, 964, 965, 967, 968, 969, 971, 972, 974, 976, 977,
        979, 980, 982, 983, 985, 986, 987, 989, 990, 992, 993, 995, 996, 997, 999,1002,
        1003,1004,1006,1007,1008,1010,1011,1012,1014,1015,1017,1018,1019,1021,1022,1023
    }
};

MI_U16 RGBGamma_H[3][256] = {
    {
        0,   7,  14,  22,  31,  39,  49,  58,  68,  78,  88,  98, 109, 119, 130, 141,
        152, 163, 174, 185, 196, 207, 218, 228, 238, 248, 258, 268, 278, 287, 297, 306,
        315, 324, 333, 342, 350, 359, 367, 375, 383, 391, 399, 407, 414, 422, 429, 437,
        444, 451, 458, 465, 472, 478, 485, 491, 498, 504, 510, 516, 522, 527, 533, 539,
        544, 550, 556, 561, 566, 572, 577, 582, 587, 592, 597, 602, 607, 612, 617, 621,
        626, 631, 635, 640, 643, 647, 652, 656, 660, 664, 668, 672, 676, 680, 684, 688,
        692, 696, 700, 704, 708, 712, 716, 720, 724, 728, 732, 735, 739, 743, 747, 751,
        754, 759, 764, 768, 772, 775, 779, 783, 787, 790, 794, 797, 800, 804, 807, 810,
        813, 816, 819, 822, 825, 828, 831, 834, 836, 839, 842, 844, 847, 848, 851, 853,
        855, 857, 860, 862, 864, 867, 869, 872, 874, 876, 879, 881, 884, 886, 889, 891,
        894, 897, 899, 902, 904, 907, 909, 912, 914, 917, 919, 921, 923, 926, 928, 930,
        932, 934, 936, 938, 940, 942, 944, 945, 947, 949, 951, 953, 954, 956, 958, 959,
        961, 963, 964, 966, 968, 969, 971, 972, 974, 975, 977, 978, 979, 981, 982, 984,
        985, 986, 987, 989, 990, 991, 992, 993, 995, 996, 997, 998, 999,1000,1001,1002,
        1003,1004,1005,1005,1006,1007,1008,1009,1009,1010,1011,1012,1012,1013,1013,1014,
        1015,1015,1016,1016,1016,1017,1017,1018,1018,1018,1019,1019,1019,1019,1019,1019
    },
    {
        0,   7,  14,  22,  31,  39,  49,  58,  68,  78,  88,  98, 109, 119, 130, 141,
        152, 163, 174, 185, 196, 207, 218, 228, 238, 248, 258, 268, 278, 287, 297, 306,
        315, 324, 333, 342, 350, 359, 367, 375, 383, 391, 399, 407, 414, 422, 429, 437,
        444, 451, 458, 465, 472, 478, 485, 491, 498, 504, 510, 516, 522, 527, 533, 539,
        544, 550, 556, 561, 566, 572, 577, 582, 587, 592, 597, 602, 607, 612, 617, 621,
        626, 631, 635, 640, 643, 647, 652, 656, 660, 664, 668, 672, 676, 680, 684, 688,
        692, 696, 700, 704, 708, 712, 716, 720, 724, 728, 732, 735, 739, 743, 747, 751,
        754, 759, 764, 768, 772, 775, 779, 783, 787, 790, 794, 797, 800, 804, 807, 810,
        813, 816, 819, 822, 825, 828, 831, 834, 836, 839, 842, 844, 847, 848, 851, 853,
        855, 857, 860, 862, 864, 867, 869, 872, 874, 876, 879, 881, 884, 886, 889, 891,
        894, 897, 899, 902, 904, 907, 909, 912, 914, 917, 919, 921, 923, 926, 928, 930,
        932, 934, 936, 938, 940, 942, 944, 945, 947, 949, 951, 953, 954, 956, 958, 959,
        961, 963, 964, 966, 968, 969, 971, 972, 974, 975, 977, 978, 979, 981, 982, 984,
        985, 986, 987, 989, 990, 991, 992, 993, 995, 996, 997, 998, 999,1000,1001,1002,
        1003,1004,1005,1005,1006,1007,1008,1009,1009,1010,1011,1012,1012,1013,1013,1014,
        1015,1015,1016,1016,1016,1017,1017,1018,1018,1018,1019,1019,1019,1019,1019,1019
    },
    {
        0,   7,  14,  22,  31,  39,  49,  58,  68,  78,  88,  98, 109, 119, 130, 141,
        152, 163, 174, 185, 196, 207, 218, 228, 238, 248, 258, 268, 278, 287, 297, 306,
        315, 324, 333, 342, 350, 359, 367, 375, 383, 391, 399, 407, 414, 422, 429, 437,
        444, 451, 458, 465, 472, 478, 485, 491, 498, 504, 510, 516, 522, 527, 533, 539,
        544, 550, 556, 561, 566, 572, 577, 582, 587, 592, 597, 602, 607, 612, 617, 621,
        626, 631, 635, 640, 643, 647, 652, 656, 660, 664, 668, 672, 676, 680, 684, 688,
        692, 696, 700, 704, 708, 712, 716, 720, 724, 728, 732, 735, 739, 743, 747, 751,
        754, 759, 764, 768, 772, 775, 779, 783, 787, 790, 794, 797, 800, 804, 807, 810,
        813, 816, 819, 822, 825, 828, 831, 834, 836, 839, 842, 844, 847, 848, 851, 853,
        855, 857, 860, 862, 864, 867, 869, 872, 874, 876, 879, 881, 884, 886, 889, 891,
        894, 897, 899, 902, 904, 907, 909, 912, 914, 917, 919, 921, 923, 926, 928, 930,
        932, 934, 936, 938, 940, 942, 944, 945, 947, 949, 951, 953, 954, 956, 958, 959,
        961, 963, 964, 966, 968, 969, 971, 972, 974, 975, 977, 978, 979, 981, 982, 984,
        985, 986, 987, 989, 990, 991, 992, 993, 995, 996, 997, 998, 999,1000,1001,1002,
        1003,1004,1005,1005,1006,1007,1008,1009,1009,1010,1011,1012,1012,1013,1013,1014,
        1015,1015,1016,1016,1016,1017,1017,1018,1018,1018,1019,1019,1019,1019,1019,1019
    }
};


MI_U16 YUVGamma_L[3][256] = {
    {
        0,  4,   8,   12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56, 60,
        64,  68,  72,  76,  80,  84,  88,  92,  96, 100, 104, 108, 112, 116, 120, 124,
        128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 173, 177, 181, 185, 189,
        193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 233, 237, 241, 245, 249, 253,
        257, 261, 265, 269, 273, 277, 281, 285, 289, 293, 297, 301, 305, 309, 313, 317,
        321, 325, 329, 333, 337, 341, 345, 349, 353, 357, 361, 365, 369, 373, 377, 381,
        385, 389, 393, 397, 401, 405, 409, 413, 417, 421, 425, 429, 433, 437, 441, 445,
        449, 453, 457, 461, 465, 469, 473, 477, 481, 485, 489, 493, 497, 501, 505, 509,
        514, 518, 522, 526, 530, 534, 538, 542, 546, 550, 554, 558, 562, 566, 570, 574,
        578, 582, 586, 590, 594, 598, 602, 606, 610, 614, 618, 622, 626, 630, 634, 638,
        642, 646, 650, 654, 658, 662, 666, 670, 674, 678, 682, 686, 690, 694, 698, 702,
        706, 710, 714, 718, 722, 726, 730, 734, 738, 742, 746, 750, 754, 758, 762, 766,
        770, 774, 778, 782, 786, 790, 794, 798, 802, 806, 810, 814, 818, 822, 826, 830,
        834, 838, 842, 846, 850, 855, 859, 863, 867, 871, 875, 879, 883, 887, 891, 895,
        899, 903, 907, 911, 915, 919, 923, 927, 931, 935, 939, 943, 947, 951, 955, 959,
        963, 967, 971, 975, 979, 983, 987, 991, 995, 999,1003,1007,1011,1015,1019,1023
    },
    {
        0,  4,  8,    12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56,  60,
        64,  68,  72,  76,  80,  84,  88,  92,  96, 100, 104, 108, 112, 116, 120, 124,
        128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 172, 176, 180, 184, 188,
        192, 196, 200, 204, 208, 212, 216, 220, 224, 228, 232, 236, 240, 244, 248, 252,
        256, 260, 264, 268, 272, 276, 280, 284, 288, 292, 296, 300, 304, 308, 312, 316,
        320, 324, 328, 332, 336, 340, 344, 348, 352, 356, 360, 364, 368, 372, 376, 380,
        384, 388, 392, 396, 400, 404, 408, 412, 416, 420, 424, 428, 432, 436, 440, 444,
        448, 452, 456, 460, 464, 468, 472, 476, 480, 484, 488, 492, 496, 500, 504, 511
    },
    {
        0,  4,  8,    12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56,  60,
        64,  68,  72,  76,  80,  84,  88,  92,  96, 100, 104, 108, 112, 116, 120, 124,
        128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 172, 176, 180, 184, 188,
        192, 196, 200, 204, 208, 212, 216, 220, 224, 228, 232, 236, 240, 244, 248, 252,
        256, 260, 264, 268, 272, 276, 280, 284, 288, 292, 296, 300, 304, 308, 312, 316,
        320, 324, 328, 332, 336, 340, 344, 348, 352, 356, 360, 364, 368, 372, 376, 380,
        384, 388, 392, 396, 400, 404, 408, 412, 416, 420, 424, 428, 432, 436, 440, 444,
        448, 452, 456, 460, 464, 468, 472, 476, 480, 484, 488, 492, 496, 500, 504, 511
    },
};

MI_U16 YUVGamma_N[3][256] = {
    {
        0,  11,  22,  33,  42,  51,  60,  68,  76,  83,  90,  96, 102, 108, 113, 118,
        123, 126, 130, 133, 136, 140, 143, 146, 150, 153, 156, 159, 162, 165, 168, 171,
        174, 176, 179, 182, 184, 187, 190, 192, 195, 197, 200, 202, 205, 207, 210, 212,
        215, 217, 219, 222, 224, 227, 229, 232, 234, 237, 240, 242, 245, 248, 251, 253,
        257, 260, 263, 266, 269, 273, 276, 280, 284, 288, 292, 296, 301, 306, 310, 315,
        321, 325, 329, 333, 337, 341, 345, 349, 353, 357, 361, 365, 369, 373, 377, 381,
        385, 389, 393, 397, 401, 405, 409, 413, 417, 421, 425, 429, 433, 437, 441, 445,
        449, 453, 457, 461, 465, 469, 473, 477, 481, 485, 489, 493, 497, 501, 505, 509,
        514, 518, 522, 526, 530, 534, 538, 542, 546, 550, 554, 558, 562, 566, 570, 574,
        578, 582, 586, 590, 594, 598, 602, 606, 610, 614, 618, 622, 626, 630, 634, 638,
        642, 646, 650, 654, 658, 662, 666, 670, 674, 678, 682, 686, 690, 694, 698, 702,
        706, 710, 714, 718, 722, 726, 730, 734, 738, 742, 746, 750, 754, 758, 762, 766,
        770, 774, 778, 782, 786, 790, 794, 798, 802, 806, 810, 814, 818, 822, 826, 830,
        834, 838, 842, 846, 850, 855, 859, 863, 867, 871, 875, 879, 883, 887, 891, 895,
        899, 903, 907, 911, 915, 919, 923, 927, 931, 935, 939, 943, 947, 951, 955, 959,
        963, 967, 971, 975, 979, 983, 987, 991, 995, 999,1003,1007,1011,1015,1019,1023
    },
    {
        0,   4,   8,  12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56,  60,
        64,  68,  72,  76,  80,  84,  88,  92,  96, 100, 104, 108, 112, 116, 120, 124,
        128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 172, 176, 180, 184, 188,
        192, 196, 200, 204, 208, 212, 216, 220, 224, 228, 232, 236, 240, 244, 248, 252,
        256, 260, 264, 268, 272, 276, 280, 284, 288, 292, 296, 300, 304, 308, 312, 316,
        320, 324, 328, 332, 336, 340, 344, 348, 352, 356, 360, 364, 368, 372, 376, 380,
        384, 388, 392, 396, 400, 404, 408, 412, 416, 420, 424, 428, 432, 436, 440, 444,
        448, 452, 456, 460, 464, 468, 472, 476, 480, 484, 488, 492, 496, 500, 504, 511
    },
    {
        0,   4,   8,  12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56,  60,
        64,  68,  72,  76,  80,  84,  88,  92,  96, 100, 104, 108, 112, 116, 120, 124,
        128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 172, 176, 180, 184, 188,
        192, 196, 200, 204, 208, 212, 216, 220, 224, 228, 232, 236, 240, 244, 248, 252,
        256, 260, 264, 268, 272, 276, 280, 284, 288, 292, 296, 300, 304, 308, 312, 316,
        320, 324, 328, 332, 336, 340, 344, 348, 352, 356, 360, 364, 368, 372, 376, 380,
        384, 388, 392, 396, 400, 404, 408, 412, 416, 420, 424, 428, 432, 436, 440, 444,
        448, 452, 456, 460, 464, 468, 472, 476, 480, 484, 488, 492, 496, 500, 504, 511
    }
};

MI_U16 YUVGamma_H[3][256] = {
    {
        0,  14,  28,  42,  55,  67,  79,  90, 100, 111, 120, 130, 139, 147, 155, 163,
        171, 178, 184, 191, 197, 203, 209, 214, 219, 224, 229, 234, 238, 243, 247, 251,
        254, 258, 262, 265, 269, 272, 275, 278, 281, 284, 287, 290, 293, 296, 298, 301,
        304, 306, 308, 311, 313, 315, 318, 320, 322, 324, 326, 328, 329, 331, 333, 334,
        336, 337, 339, 341, 343, 345, 347, 349, 351, 353, 355, 356, 358, 360, 362, 364,
        365, 367, 369, 371, 373, 375, 377, 379, 380, 382, 384, 387, 389, 391, 393, 395,
        397, 400, 402, 405, 408, 410, 413, 416, 419, 422, 426, 429, 433, 436, 440, 444,
        449, 452, 456, 459, 463, 467, 471, 475, 479, 483, 487, 491, 495, 500, 504, 509,
        513, 518, 522, 526, 530, 534, 538, 542, 546, 550, 554, 558, 562, 566, 570, 574,
        578, 582, 586, 590, 594, 598, 602, 606, 610, 614, 618, 622, 626, 630, 634, 638,
        642, 646, 650, 654, 658, 662, 666, 670, 674, 678, 682, 686, 690, 694, 698, 702,
        706, 710, 714, 718, 722, 726, 730, 734, 738, 742, 746, 750, 754, 758, 762, 766,
        770, 774, 778, 782, 786, 790, 794, 798, 802, 806, 810, 814, 818, 822, 826, 830,
        834, 838, 842, 846, 850, 855, 859, 863, 867, 871, 875, 879, 883, 887, 891, 895,
        899, 903, 907, 911, 915, 919, 923, 927, 931, 935, 939, 943, 947, 951, 955, 959,
        963, 967, 971, 975, 979, 983, 987, 991, 995, 999,1003,1007,1011,1015,1019, 1023
    },
    {
        0,  4,    8,  12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56,  60,
        64,  68,  72,  76,  80,  84,  88,  92,  96, 100, 104, 108, 112, 116, 120, 124,
        128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 172, 176, 180, 184, 188,
        192, 196, 200, 204, 208, 212, 216, 220, 224, 228, 232, 236, 240, 244, 248, 252,
        256, 260, 264, 268, 272, 276, 280, 284, 288, 292, 296, 300, 304, 308, 312, 316,
        320, 324, 328, 332, 336, 340, 344, 348, 352, 356, 360, 364, 368, 372, 376, 380,
        384, 388, 392, 396, 400, 404, 408, 412, 416, 420, 424, 428, 432, 436, 440, 444,
        448, 452, 456, 460, 464, 468, 472, 476, 480, 484, 488, 492, 496, 500, 504, 511
    },
    {
        0,   4,   8,  12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56, 60,
        64,  68,  72,  76,  80,  84,  88,  92,  96, 100, 104, 108, 112, 116, 120, 124,
        128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 172, 176, 180, 184, 188,
        192, 196, 200, 204, 208, 212, 216, 220, 224, 228, 232, 236, 240, 244, 248, 252,
        256, 260, 264, 268, 272, 276, 280, 284, 288, 292, 296, 300, 304, 308, 312, 316,
        320, 324, 328, 332, 336, 340, 344, 348, 352, 356, 360, 364, 368, 372, 376, 380,
        384, 388, 392, 396, 400, 404, 408, 412, 416, 420, 424, 428, 432, 436, 440, 444,
        448, 452, 456, 460, 464, 468, 472, 476, 480, 484, 488, 492, 496, 500, 504, 511
    }
};

MI_U8 GetMirrorFlipLevel()
{
    return g_MirrorFlipLevel;
}

int mod_isp_wait_ready_timeout(int time_ms)
{
    int delayCnt = time_ms;
    MI_ISP_IQ_PARAM_INIT_INFO_TYPE_t status;

    while(delayCnt)
    {
        MI_ISP_IQ_GetParaInitStatus(0, &status);
        if(1 == status.stParaAPI.bFlag){
            break;
        }
        delayCnt--;
        usleep(1000);
    }

    if(delayCnt <= 0){
        printf("wait isp ready timeout\n");
        return -1;
    }

    return 0;
}

void mod_isp_ColorToGray(int iEnable)
{
    //set color to gray ===================================
    static int iOriBak = 0;
    static MI_ISP_IQ_SATURATION_TYPE_t sat_ori;
    MI_ISP_IQ_SATURATION_TYPE_t sat_gray;
    static MI_ISP_AWB_ATTR_TYPE_t awb_ori;
    MI_ISP_AWB_ATTR_TYPE_t awb_gray;
    int channel = 0; //set to target channe, default is 0

    if(iEnable)
    {
        if(iOriBak == 0)
        {
            MI_ISP_IQ_GetSaturation(channel, &sat_ori);
            MI_ISP_AWB_GetAttr(channel, &awb_ori);
            iOriBak = 1;
        }
        memcpy(&sat_gray, &sat_ori, sizeof(MI_ISP_IQ_SATURATION_TYPE_t));
        memcpy(&awb_gray, &awb_ori, sizeof(MI_ISP_AWB_ATTR_TYPE_t));

        //set saturation to 0
        sat_gray.bEnable = SS_TRUE;
        sat_gray.enOpType = SS_OP_TYP_MANUAL;
        sat_gray.stManual.stParaAPI.u8SatAllStr = 0;
        MI_ISP_IQ_SetSaturation(channel, &sat_gray);

        //set awb gain to 1x
        awb_gray.eState = SS_ISP_STATE_NORMAL;
        awb_gray.eOpType = SS_OP_TYP_MANUAL;
        awb_gray.stManualParaAPI.u16Rgain = 1024;
        awb_gray.stManualParaAPI.u16Grgain = 1024;
        awb_gray.stManualParaAPI.u16Gbgain = 1024;
        awb_gray.stManualParaAPI.u16Bgain = 1024;
        MI_ISP_AWB_SetAttr(channel, &awb_gray);
    }
    else
    {
        //set color back =====================================
        if(iOriBak == 1)
        {
            MI_ISP_AWB_SetAttr(channel, &awb_ori);
            MI_ISP_IQ_SetSaturation(channel, &sat_ori);
            iOriBak = 0;
        }
    }
}

void nanosleep_ms(long millisecond)
{
    int j;
    struct timespec tTimeSpec;

    tTimeSpec.tv_sec = 0;
    tTimeSpec.tv_nsec = millisecond*1000*1000;

    for (j = 0; j < 10; j++)
    {
        if(nanosleep(&tTimeSpec, NULL) == -1)
        {
            continue;
        }
        break;
    }
}

int mod_isp_gpio_req(int gpioNum)
{
    int fd = -1;
    char buf[4] = {0,};

    sprintf(buf,"%d",gpioNum);
    fd = open("/sys/class/gpio/export",O_WRONLY);
    if (fd == -1)
    {
        printf( "fail to export\n");
        return -1;
    }

    write(fd,buf,sizeof(buf));
    close(fd);
    return 0;
}

int mod_isp_gpio_unexp(int gpioNum)
{
    int fd = -1;
    char buf[4] = {0,};

    sprintf(buf,"%d",gpioNum);
    fd = open("/sys/class/gpio/unexport",O_WRONLY);
    if (fd == -1)
    {
        printf("no export the gpio!\n");
        return -1;
    }
    write(fd,buf,sizeof(buf));
    close(fd);

    return 0;
}

int mod_isp_gpio_set_dir(int gpioNum, const char* direct)
{
    int fd = -1;
    char path[128] = {0,};

    sprintf(path,"/sys/class/gpio/gpio%d/direction",gpioNum);
    fd = open(path,O_WRONLY);
    if (fd == -1)
    {
        printf("fail write direction\n");
        return -1;
    }

    write(fd,direct,strlen(direct));
    close(fd);
    return 0;
}


int mod_isp_gpio_get_val(int gpioNum)
{
    FILE* file = NULL;
    char buf[10];
    char path[32];
    int value = 0;
	int iReadLen = 0;
    memset(buf,0,sizeof(buf));
    memset(path,0,sizeof(path));
    sprintf(path,"/sys/class/gpio/gpio%d/value",gpioNum);

    file = fopen(path,"rb");
    if (file == NULL)
    {
        printf( "fail to open %s!\n", path);
        return -1;
    }

    fseek(file, 0, SEEK_END);

    int len = ftell(file) > (long int)sizeof(buf) ? sizeof(buf) : ftell(file);

    rewind(file);
    if(len>0)
    {
        iReadLen = fread(buf,1,len-1,file);
		if(iReadLen  < len-1)
			MIXER_WARN("read isp goio, read len : %d, should read len: %d\n", iReadLen, len-1);

        buf[len-1] = '\0';
    }
    fclose(file);
    file = NULL;

    value = mixerStr2Int((char*)buf);
    printf("get gpio%d=%d\n",gpioNum,value);
    return value;
}

int mod_isp_gpio_set_val(int gpioNum,int value)
{
    FILE* fd = NULL;
    char buf[2] = {'0'};
    char path[128] = {0,};

    sprintf(&buf[0],"%c",(char)value);
    snprintf(path,sizeof(path),"/sys/class/gpio/gpio%d/value",gpioNum);

    fd = fopen(path,"wb");
    if (fd == NULL)
    {
        printf( "fail to open %s!\n", path);
        return -1;
    }

    fwrite(&buf[0],1,1,fd);
    printf("gpio%d_set %c\n",gpioNum,buf[0]);

    fclose(fd);
    return 0;
}


int mod_isp_ircut_set(int boardtype, int isDay)
{
    static int gpioexport = 0;
    int gpio1 = 145;
    int gpio2 = 146;

    if(boardtype == 0)  // 326D
    {
        gpio1 = 130;
        gpio2 = 131;
    }else{              // 328Q
        gpio1 = 145;
        gpio2 = 146;
    }

    if(gpioexport == 0)
    {
        mod_isp_gpio_req(gpio1);
        mod_isp_gpio_req(gpio2);
        mod_isp_gpio_set_dir(gpio1, "out");
        mod_isp_gpio_set_dir(gpio2, "out");
        gpioexport = 1;
    }

    if(isDay)
    {
        mod_isp_gpio_set_val(gpio1, 1);
        mod_isp_gpio_set_val(gpio2, 0);
        nanosleep_ms(100);
        mod_isp_gpio_set_val(gpio1, 0);
        mod_isp_gpio_set_val(gpio2, 0);
    }else{
        mod_isp_gpio_set_val(gpio1, 0);
        mod_isp_gpio_set_val(gpio2, 1);
        nanosleep_ms(100);
        mod_isp_gpio_set_val(gpio1, 0);
        mod_isp_gpio_set_val(gpio2, 0);
    }
    return 0;
}

#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
unsigned int poll_isp(int fd,int timeout)
{
    struct pollfd pfd;
    int ret = 0;
    unsigned int flag = 0;
	unsigned int readSize = 0;

    pfd.fd = fd;
    pfd.events = POLLIN|POLLRDNORM;

    ret = poll(&pfd,1,timeout);
    if(ret>0)
    {
        readSize = read(fd,(void*)&flag,sizeof(flag));
		if(readSize < sizeof(flag))
		{
			MIXER_WARN("read isp poll failed!, ret : %d\n", readSize);
		}
    }
    return flag;
}
#endif

static void IspTestColorToGray(MI_BOOL enable)
{
    MI_ISP_IQ_COLORTOGRAY_TYPE_t Colortogray;
    MI_ISP_BOOL_e nValue = (MI_ISP_BOOL_e)!!enable;

    MI_ISP_IQ_GetColorToGray(0, &Colortogray);
    Colortogray.bEnable = nValue;
    MI_ISP_IQ_SetColorToGray(0, &Colortogray);
}

static void IspTestSensorMirroFlip(MI_U32 nValue)
{
    MI_SYS_Rotate_e eRot;
    MI_SNR_PAD_ID_e eSnrPadId = E_MI_SNR_PAD_ID_0;
    struct mixer_rotation_t mirrorFlipState;

    memset(&mirrorFlipState,0,sizeof(mirrorFlipState));

    if(MI_SUCCESS != (MI_VPE_GetChannelRotation(0, &eRot)))
    {
        MIXER_ERR("can not get vpe rot state\n");
        return;
    }

    if(0 > GetMixerRotState(&mirrorFlipState, eRot, nValue))
    {
        MIXER_ERR("can not get mixer_rot_state\n");
        return;
    }

    if(MI_SUCCESS != (MI_SNR_SetOrien(eSnrPadId, mirrorFlipState.bMirror, mirrorFlipState.bFlip)))
    {
        MIXER_ERR("vif MirrorType:%d(Mirror=%d, Flip=%d)\n", nValue, mirrorFlipState.bMirror, mirrorFlipState.bFlip);
        return;
    }
    g_MirrorFlipLevel = nValue;
    printf("vif MirrorType:%d(Mirror=%d, Flip=%d eRot=%d)\n", nValue, mirrorFlipState.bMirror, mirrorFlipState.bFlip,eRot);
}



static void IspTestAWB(const struct _TestAWB *pb)
{
    if(NULL == pb)
    {
        MIXER_ERR("pb is null. err\n");
        return;
    }

    MI_ISP_AWB_ATTR_TYPE_t awbAttr;

    MI_ISP_AWB_GetAttr(0, &awbAttr);
    awbAttr.eOpType = (MI_ISP_OP_TYPE_e)pb->eOpType;
    awbAttr.stManualParaAPI.u16Bgain = (MI_U16)pb->u16Bgain;
    awbAttr.stManualParaAPI.u16Gbgain = (MI_U16)pb->u16Gbgain;
    awbAttr.stManualParaAPI.u16Grgain = (MI_U16)pb->u16Grgain;
    awbAttr.stManualParaAPI.u16Rgain = (MI_U16)pb->u16Rgain;

    MI_ISP_AWB_SetAttr(0, &awbAttr);

    printf("mode:%d\n", awbAttr.eOpType);
    printf("Bgain:%hu\n", awbAttr.stManualParaAPI.u16Bgain);
    printf("Gbgain:%hu\n", awbAttr.stManualParaAPI.u16Gbgain);
    printf("Grgain:%hu\n", awbAttr.stManualParaAPI.u16Grgain);
    printf("Rgain:%hu\n", awbAttr.stManualParaAPI.u16Rgain);
}

static  void IspTestAeStrategy(MI_U32 nValue)
{
    MI_ISP_AE_STRATEGY_TYPE_t sSetAeStrategy;
    if(nValue != 0 && nValue != 1 && nValue != 2)
    {
        nValue = 2;
    }
    MI_ISP_AE_GetStrategy(0, &sSetAeStrategy);
    if(nValue==0)
    {
        printf("AE_SetStrategy eAEStrategyMode:BrightPriority\n");
        sSetAeStrategy.eAEStrategyMode = (MI_ISP_AE_STRATEGY_TYPE_e)nValue;
        sSetAeStrategy.stLowerOffset.u16NumOfPoints = 3;
        sSetAeStrategy.stLowerOffset.u32X[0] = -65536;
        sSetAeStrategy.stLowerOffset.u32X[1] = 0;
        sSetAeStrategy.stLowerOffset.u32X[2] = 81920;
        sSetAeStrategy.stLowerOffset.u32Y[0] = 200;
        sSetAeStrategy.stLowerOffset.u32Y[1] = 200;
        sSetAeStrategy.stLowerOffset.u32Y[2] = 200;
        sSetAeStrategy.stUpperOffset.u16NumOfPoints = 3;
        sSetAeStrategy.stUpperOffset.u32X[0] = -65536;
        sSetAeStrategy.stUpperOffset.u32X[1] = 0;
        sSetAeStrategy.stUpperOffset.u32X[2] = 81920;
        sSetAeStrategy.stUpperOffset.u32Y[0] = 0;
        sSetAeStrategy.stUpperOffset.u32Y[1] = 0;
        sSetAeStrategy.stUpperOffset.u32Y[2] = 0;
        sSetAeStrategy.u32AutoSensitivity=0;
        sSetAeStrategy.u32Weighting=1024;
        sSetAeStrategy.u32AutoStrength=0;
        sSetAeStrategy.u32BrightToneSensitivity=1024;
        sSetAeStrategy.u32BrightToneStrength=1024;
        sSetAeStrategy.u32DarkToneSensitivity=0;
        sSetAeStrategy.u32DarkToneStrength=0;
    }
    else if(nValue==1)
    {
        printf("AE_SetStrategy eAEStrategyMode:DarkPriority\n");
        sSetAeStrategy.eAEStrategyMode = (MI_ISP_AE_STRATEGY_TYPE_e)nValue;
        sSetAeStrategy.stLowerOffset.u16NumOfPoints = 3;
        sSetAeStrategy.stLowerOffset.u32X[0] = -65536;
        sSetAeStrategy.stLowerOffset.u32X[1] = 0;
        sSetAeStrategy.stLowerOffset.u32X[2] = 81920;
        sSetAeStrategy.stLowerOffset.u32Y[0] = 0;
        sSetAeStrategy.stLowerOffset.u32Y[1] = 0;
        sSetAeStrategy.stLowerOffset.u32Y[2] = 0;
        sSetAeStrategy.stUpperOffset.u16NumOfPoints = 3;
        sSetAeStrategy.stUpperOffset.u32X[0] = -65536;
        sSetAeStrategy.stUpperOffset.u32X[1] = 0;
        sSetAeStrategy.stUpperOffset.u32X[2] = 81920;
        sSetAeStrategy.stUpperOffset.u32Y[0] = 200;
        sSetAeStrategy.stUpperOffset.u32Y[1] = 200;
        sSetAeStrategy.stUpperOffset.u32Y[2] = 200;
        sSetAeStrategy.u32AutoSensitivity=0;
        sSetAeStrategy.u32Weighting=1024;
        sSetAeStrategy.u32AutoStrength=0;
        sSetAeStrategy.u32BrightToneSensitivity=0;
        sSetAeStrategy.u32BrightToneStrength=0;
        sSetAeStrategy.u32DarkToneSensitivity=1024;
        sSetAeStrategy.u32DarkToneStrength=1024;
    }
    else
    {
        printf("AE_SetStrategy eAEStrategyMode:Auto mode\n");
        sSetAeStrategy.eAEStrategyMode = (MI_ISP_AE_STRATEGY_TYPE_e)nValue;
        sSetAeStrategy.stLowerOffset.u16NumOfPoints = 3;
        sSetAeStrategy.stLowerOffset.u32X[0] = -65536;
        sSetAeStrategy.stLowerOffset.u32X[1] = 0;
        sSetAeStrategy.stLowerOffset.u32X[2] = 81920;
        sSetAeStrategy.stLowerOffset.u32Y[0] = 0;
        sSetAeStrategy.stLowerOffset.u32Y[1] = 0;
        sSetAeStrategy.stLowerOffset.u32Y[2] = 0;
        sSetAeStrategy.stUpperOffset.u16NumOfPoints = 3;
        sSetAeStrategy.stUpperOffset.u32X[0] = -65536;
        sSetAeStrategy.stUpperOffset.u32X[1] = 0;
        sSetAeStrategy.stUpperOffset.u32X[2] = 81920;
        sSetAeStrategy.stUpperOffset.u32Y[0] = 0;
        sSetAeStrategy.stUpperOffset.u32Y[1] = 0;
        sSetAeStrategy.stUpperOffset.u32Y[2] = 0;
        sSetAeStrategy.u32AutoSensitivity=1024;
        sSetAeStrategy.u32Weighting=1024;
        sSetAeStrategy.u32AutoStrength=1024;
        sSetAeStrategy.u32BrightToneSensitivity=0;
        sSetAeStrategy.u32BrightToneStrength=0;
        sSetAeStrategy.u32DarkToneSensitivity=0;
        sSetAeStrategy.u32DarkToneStrength=0;
    }

    MI_ISP_AE_SetStrategy(0, &sSetAeStrategy);
    printf("MI_ISP_SetExposure %d\n", nValue);

}

static void IspTestContrast(MI_U32 nValue)
{
    MI_ISP_IQ_CONTRAST_TYPE_t Contrast;
    MI_ISP_IQ_GetContrast(0, &Contrast);

    Contrast.bEnable = (MI_ISP_BOOL_e)TRUE;

    Contrast.enOpType = SS_OP_TYP_MANUAL;
    Contrast.stManual.stParaAPI.u32Lev = (MI_U32)nValue;
    MI_ISP_IQ_SetContrast(0, &Contrast);
    MIXER_DBG("Contrast:%d\n", nValue);
}

static void IspTestBrightness(MI_U32 nValue)
{
    MI_ISP_IQ_BRIGHTNESS_TYPE_t Brightness;
    MI_ISP_IQ_GetBrightness(0, &Brightness);

    Brightness.bEnable = (MI_ISP_BOOL_e)TRUE;

    Brightness.enOpType = SS_OP_TYP_MANUAL;
    Brightness.stManual.stParaAPI.u32Lev = (MI_U32)nValue;
    MI_ISP_IQ_SetBrightness(0, &Brightness);
    printf("Brightness:%d\n", nValue);
}

static void IspTestGama(MI_U32 nMode, MI_U32 nValue)
{
    MI_U32 j = 0x0;

    if(1 == nMode)    //RGBGamma
    {
        MI_U16 *p_RGBGAMA_R = NULL;
        MI_U16 *p_RGBGAMA_G = NULL;
        MI_U16 *p_RGBGAMA_B = NULL;
        MI_ISP_IQ_RGBGAMMA_TYPE_t* Gamma = new MI_ISP_IQ_RGBGAMMA_TYPE_t;

        MI_ISP_IQ_GetRGBGamma(0, Gamma);

        Gamma->bEnable = (MI_ISP_BOOL_e)TRUE;
        Gamma->enOpType = SS_OP_TYP_MANUAL;

        //nValue;  1 Low contrast, 2 Normal, 3 High contrast

        switch(nValue)
        {
            case 1:
                p_RGBGAMA_R = RGBGamma_L[0];
                p_RGBGAMA_G = RGBGamma_L[1];
                p_RGBGAMA_B = RGBGamma_L[2];
                break;

            case 2:
                p_RGBGAMA_R = RGBGamma_N[0];
                p_RGBGAMA_G = RGBGamma_N[1];
                p_RGBGAMA_B = RGBGamma_N[2];
                break;

            case 3:
                p_RGBGAMA_R = RGBGamma_H[0];
                p_RGBGAMA_G = RGBGamma_H[1];
                p_RGBGAMA_B = RGBGamma_H[2];
                break;

            default:
                printf("%s:%d Input wrong RGBGamma index:%d, only support 1, 2, or 3.\n", __func__, __LINE__, nValue);
                delete Gamma;
		        Gamma = NULL;
                return;
        }

        //printf("%s:%d p_RGBGAMA_R=%p, p_RGBGAMA_G=%p, p_RGBGAMA_B=%p\n", __func__, __LINE__, p_RGBGAMA_R, p_RGBGAMA_G, p_RGBGAMA_B);

        for(j = 0; j < 256; j++)
        {
            Gamma->stManual.stParaAPI.u16LutB[j] = *(p_RGBGAMA_B + j);
            Gamma->stManual.stParaAPI.u16LutG[j] = *(p_RGBGAMA_G + j);
            Gamma->stManual.stParaAPI.u16LutR[j] = *(p_RGBGAMA_R + j);
            //printf("%s:%d p_RGBGAMA_R+%d=%d, p_RGBGAMA_G+%d=%d, p_RGBGAMA_B+%d=%d\n", __func__, __LINE__,j,*(p_RGBGAMA_R + j),j,*(p_RGBGAMA_G + j),j,*(p_RGBGAMA_B + j));
        }

        MI_ISP_IQ_SetRGBGamma(0, Gamma);

        for(j = 0; j < 256; j++)
        {
            printf("Gamma R:[%hu] Gamma G:[%hu] Gamma b:[%hu]\n", Gamma->stManual.stParaAPI.u16LutR[j],
                    Gamma->stManual.stParaAPI.u16LutG[j],
                    Gamma->stManual.stParaAPI.u16LutB[j]);
        }
		delete Gamma;
		Gamma = NULL;
    }
    else if(2 == nMode)
    {
        MI_U16 *p_YUVGAMA_Y = NULL;
        MI_U16 *p_YUVGAMA_U = NULL;
        MI_U16 *p_YUVGAMA_V = NULL;
        MI_ISP_IQ_YUVGAMMA_TYPE_t* yuvgamma = new MI_ISP_IQ_YUVGAMMA_TYPE_t;

        MI_ISP_IQ_GetYUVGamma(0, yuvgamma);

        yuvgamma->bEnable = (MI_ISP_BOOL_e)TRUE;
        yuvgamma->enOpType = SS_OP_TYP_MANUAL;

        //nValue;  // 1 Low contrast, 2 Normal, 3 High contrast
        switch(nValue)
        {
            case 1:
                p_YUVGAMA_Y = YUVGamma_L[0];
                p_YUVGAMA_U = YUVGamma_L[1];
                p_YUVGAMA_V = YUVGamma_L[2];
                break;

            case 2:
                p_YUVGAMA_Y = YUVGamma_N[0];
                p_YUVGAMA_U = YUVGamma_N[1];
                p_YUVGAMA_V = YUVGamma_N[2];
                break;

            case 3:
                p_YUVGAMA_Y = YUVGamma_H[0];
                p_YUVGAMA_U = YUVGamma_H[1];
                p_YUVGAMA_V = YUVGamma_H[2];
                break;

            default:
                printf("%s:%d Input wrong YUVGamma index:%d, only support 1, 2, or 3.\n", __func__, __LINE__, nValue);
                delete yuvgamma;
		        yuvgamma = NULL;
                return;
        }

        //printf("%s:%d p_YUVGAMA_Y=%p, p_YUVGAMA_U=%p, p_YUVGAMA_V=%p\n", __func__, __LINE__, p_YUVGAMA_Y, p_YUVGAMA_U, p_YUVGAMA_V);

        for(j = 0; j < 128; j++)
        {
            yuvgamma->stManual.stParaAPI.u16LutY[j] = *(p_YUVGAMA_Y + j);
            yuvgamma->stManual.stParaAPI.u16LutV[j] = *(p_YUVGAMA_V + j);
            yuvgamma->stManual.stParaAPI.u16LutU[j] = *(p_YUVGAMA_U + j);
            printf("Gamma Y:[%hu] Gamma V:[%hu] Gamma U:[%hu]\n", yuvgamma->stManual.stParaAPI.u16LutY[j],
                    yuvgamma->stManual.stParaAPI.u16LutV[j],
                    yuvgamma->stManual.stParaAPI.u16LutU[j]);
        }

        for(j = 128; j < 256; j++)
        {
            yuvgamma->stManual.stParaAPI.u16LutY[j] = *(p_YUVGAMA_Y + j);
            printf("Gamma Y:[%hu]\n", yuvgamma->stManual.stParaAPI.u16LutY[j]);
        }

        MI_ISP_IQ_SetYUVGamma(0, yuvgamma);
		delete yuvgamma;
		yuvgamma = NULL;
    }
}

static void IspTestSatruation(MI_U32 nValue)
{
    MI_ISP_IQ_SATURATION_TYPE_t Saturation;

    MI_ISP_IQ_GetSaturation(0, &Saturation);

    if(nValue > 127)
    {
        MIXER_WARN("IQ_SetSaturation u8SatAllStr [0,127], %d\n", nValue);
        nValue = 127;
    }

    Saturation.stManual.stParaAPI.u8SatAllStr = (MI_U8)nValue;//0~127

    Saturation.bEnable = (MI_ISP_BOOL_e)TRUE;
    Saturation.enOpType = SS_OP_TYP_MANUAL;
    MI_ISP_IQ_SetSaturation(0, &Saturation);

    printf("Saturation:%u\n", Saturation.stManual.stParaAPI.u8SatAllStr);
}

static void IspTestLightNess(MI_U32 nValue)
{
    MI_ISP_IQ_LIGHTNESS_TYPE_t Lightness;

    MI_ISP_IQ_GetLightness(0, &Lightness);

    if(nValue > 100)
    {
        MIXER_WARN("IQ_LightNess value:[0,100], %d\n", nValue);
        nValue = 100;
    }

    Lightness.bEnable = (MI_ISP_BOOL_e)TRUE;
    Lightness.enOpType = SS_OP_TYP_MANUAL;
    Lightness.stManual.stParaAPI.u32Lev = (MI_U32)nValue;
    MI_ISP_IQ_SetLightness(0, &Lightness);
}


static void IspTestSharpNess(MI_U32 nUDValue, MI_U32 nDValue)
{
    MI_ISP_IQ_SHARPNESS_TYPE_t Sharpnessv1;
    MI_ISP_IQ_GetSharpness(0, &Sharpnessv1);

    Sharpnessv1.bEnable = (MI_ISP_BOOL_e)TRUE;
    #if TARGET_CHIP_I6
    Sharpnessv1.stManual.stParaAPI.bDirEn = (MI_ISP_BOOL_e)TRUE;
    #endif
    Sharpnessv1.enOpType = SS_OP_TYP_MANUAL;

    if(nUDValue > 1023)
    {
        MIXER_WARN("UD out of rang, [0, 1023], %d\n", nUDValue);
        nUDValue = 1023;
    }
    if(nDValue > 1023)
    {
        MIXER_WARN("U out of rang, [0, 1023], %d\n", nDValue);
        nDValue = 1023;
    }

#if TARGET_CHIP_I6 || TARGET_CHIP_I5
    Sharpnessv1.stManual.stParaAPI.u16SharpnessUD = nUDValue;
    Sharpnessv1.stManual.stParaAPI.u16SharpnessD = nDValue;
#else
    Sharpnessv1.stManual.stParaAPI.u16SharpnessUD[0] = nUDValue;
    Sharpnessv1.stManual.stParaAPI.u16SharpnessUD[1] = nUDValue;
    Sharpnessv1.stManual.stParaAPI.u16SharpnessD[0] = nDValue;
    Sharpnessv1.stManual.stParaAPI.u16SharpnessD[1] = nDValue;
#endif

    MI_ISP_IQ_SetSharpness(0, &Sharpnessv1);
}

static  void IspTestFlicker(MI_U32 nValue)
{
    MI_ISP_AE_FLICKER_TYPE_e Flicker;

    MI_ISP_AE_GetFlicker(0, &Flicker);
    printf("AE_SetFlicker Flicker [0,2]\n");
    if(nValue > 2)
    {
        MIXER_WARN("value out of rang, [0, 2], %d\n", nValue);
        nValue = 2;
    }

    Flicker = (MI_ISP_AE_FLICKER_TYPE_e)nValue;
    if(Flicker < SS_AE_FLICKER_TYPE_DISABLE || Flicker > SS_AE_FLICKER_TYPE_MAX)
    {
        Flicker = SS_AE_FLICKER_TYPE_DISABLE;
    }
    MI_ISP_AE_SetFlicker(0, &Flicker);
}

static void IspTestCCM(MI_U16 *nCCM)
{
    if(NULL == nCCM)
    {
        MIXER_ERR("err, nCCM is null\n");
        return;
    }

    int count = 0;
    MI_ISP_IQ_RGBMATRIX_TYPE_t RGBMatrix;

    MI_ISP_IQ_GetRGBMatrix(0, &RGBMatrix);

    RGBMatrix.bEnable = (MI_ISP_BOOL_e)TRUE;
    RGBMatrix.enOpType = SS_OP_TYP_MANUAL;

#if TARGET_CHIP_I6
    for(count = 0; count < 9; count++)
    {
        RGBMatrix.stManual.u16CCM[count] = nCCM[count];
        if(RGBMatrix.stManual.u16CCM[count]  > 8191)
        {
            MIXER_WARN("ccm value out of rang, [0, 8191], %d\n", RGBMatrix.stManual.u16CCM[count] );
            RGBMatrix.stManual.u16CCM[count] = 8191;
        }
    }
#else
    for(count = 0; count < 9; count++)
    {
        if(nCCM[count]  > 8191)
        {
            MIXER_WARN("ccm value out of rang, [0, 8191], %d\n", nCCM[count] );
            nCCM[count] = 8191;
        }

        //     input: 0 1 2     3 4 5    6 7 8
        //mapping: 0 1 2 3  4 5 6 7  8 9 10 11
        if (count >= 3 && count <=5)
            RGBMatrix.stManual.u16CCM[count+1] = nCCM[count];
        else if (count >= 6 && count <=8)
            RGBMatrix.stManual.u16CCM[count+2] = nCCM[count];
        else
            RGBMatrix.stManual.u16CCM[count] = nCCM[count];
    }

#endif

    MI_ISP_IQ_SetRGBMatrix(0, &RGBMatrix);
}

static void IspTestFalseColor(MI_U16 nSMid, MI_U16 nUMid)
{
    MI_ISP_IQ_FALSECOLOR_TYPE_t FalseColorV1;

    MI_ISP_IQ_GetFalseColor(0, &FalseColorV1);
    FalseColorV1.enOpType = SS_OP_TYP_MANUAL;
    FalseColorV1.bEnable = (MI_ISP_BOOL_e)TRUE;

#if TARGET_CHIP_I5
    FalseColorV1.stManual.stParaAPI.u16FreqThrd = 140;
    FalseColorV1.stManual.stParaAPI.u16EdgeScoreThrd = 31;
#elif TARGET_CHIP_I6
    FalseColorV1.stManual.stParaAPI.u8FreqThrd = 140;
    FalseColorV1.stManual.stParaAPI.u8EdgeScoreThrd = (MI_U8)31;
#elif TARGET_CHIP_I6E
    FalseColorV1.stManual.stParaAPI.u8FreqThrd = 140;
    FalseColorV1.stManual.stParaAPI.u8EdgeScoreThrd = (MI_U8)31;
#elif TARGET_CHIP_I6B0
    FalseColorV1.stManual.stParaAPI.u8FreqThrd = 140;
    FalseColorV1.stManual.stParaAPI.u8EdgeScoreThrd = (MI_U8)31;
#endif

    if(nSMid > 11)
    {
        MIXER_WARN("nSMid value out of range, [0, 11], %d\n", nSMid);
        nSMid = 11;
    }

    FalseColorV1.stManual.stParaAPI.u8StrengthMid = (MI_U8)nSMid;
    FalseColorV1.stManual.stParaAPI.u8StrengthMin = (MI_U8)nSMid;

    if(nUMid > 2047)
    {
        MIXER_WARN("u16ChromaThrdOfStrengthMid value out of range, [0, 2047], %d\n", nUMid);
        nUMid = 2047;
    }

#if TARGET_CHIP_I5
    FalseColorV1.stManual.stParaAPI.u16ChromaThrdOfStrengthMax = nUMid;
    FalseColorV1.stManual.stParaAPI.u16ChromaThrdOfStrengthMid = nUMid;
    FalseColorV1.stManual.stParaAPI.u16ChromaThrdOfStrengthMin = nUMid;
#elif TARGET_CHIP_I6
    FalseColorV1.stManual.stParaAPI.u8ChromaThrdOfStrengthMax = nUMid;
    FalseColorV1.stManual.stParaAPI.u8ChromaThrdOfStrengthMid = nUMid;
    FalseColorV1.stManual.stParaAPI.u8ChromaThrdOfStrengthMin = nUMid;
#endif
    MI_ISP_IQ_SetFalseColor(0, &FalseColorV1);
    printf("u8StrengthMid = %d\n", FalseColorV1.stManual.stParaAPI.u8StrengthMid);
#if TARGET_CHIP_I5
    printf("u16ChromaThrdOfStrengthMid = %d\n", FalseColorV1.stManual.stParaAPI.u16ChromaThrdOfStrengthMid);
#elif TARGET_CHIP_I6
    printf("u16ChromaThrdOfStrengthMid = %d\n", FalseColorV1.stManual.stParaAPI.u8ChromaThrdOfStrengthMid);
#endif
}

static void IspTestCrossTalk(const struct _crosstalk   *tCrosstalk)
{
    if(NULL == tCrosstalk)
    {
        MIXER_ERR("err, tCrosstalk is null\n");
        return;
    }
    MI_ISP_IQ_CROSSTALK_TYPE_t Crosstalk;

    MI_ISP_IQ_GetCrossTalk(0, &Crosstalk);

    Crosstalk.bEnable = (MI_ISP_BOOL_e)TRUE;
    Crosstalk.enOpType = SS_OP_TYP_MANUAL;
#if TARGET_CHIP_I5
    Crosstalk.enV2 = (MI_ISP_BOOL_e)tCrosstalk->enV2;

    Crosstalk.stManual.stParaAPI.u16ThresholdHigh = tCrosstalk->u16ThresholdHigh;
    if(Crosstalk.stManual.stParaAPI.u16ThresholdHigh > 255)
    {
        MIXER_WARN("u16ThresholdHigh value out of range, [0, 255], %d\n", Crosstalk.stManual.stParaAPI.u16ThresholdHigh);
        Crosstalk.stManual.stParaAPI.u16ThresholdHigh = 255;
    }


    Crosstalk.stManual.stParaAPI.u16ThresholdLow = tCrosstalk->u16ThresholdLow;
    if(Crosstalk.stManual.stParaAPI.u16ThresholdLow > 255)
    {
        MIXER_WARN("u16ThresholdLow value out of range, [0, 255], %d\n", Crosstalk.stManual.stParaAPI.u16ThresholdLow);
        Crosstalk.stManual.stParaAPI.u16ThresholdLow = 255;
    }


    Crosstalk.stManual.stParaAPI.u16ThresholdV2 = tCrosstalk->u16ThresholdV2;
    if(Crosstalk.stManual.stParaAPI.u16ThresholdV2 > 255)
    {
        MIXER_WARN("u16ThresholdV2 value out of range, [0, 255], %d\n", Crosstalk.stManual.stParaAPI.u16ThresholdV2);
        Crosstalk.stManual.stParaAPI.u16ThresholdV2 = 255;
    }

    Crosstalk.stManual.stParaAPI.u8StrengthHigh = tCrosstalk->u8StrengthHigh;
    if(Crosstalk.stManual.stParaAPI.u8StrengthHigh > 7)
    {
        MIXER_WARN("u8StrengthHigh value out of range, [0, 7], %d\n", Crosstalk.stManual.stParaAPI.u8StrengthHigh);
        Crosstalk.stManual.stParaAPI.u8StrengthHigh = 255;
    }

    Crosstalk.stManual.stParaAPI.u8StrengthLow = tCrosstalk->u8StrengthLow;
    if(Crosstalk.stManual.stParaAPI.u8StrengthLow > 7)
    {
        MIXER_WARN("u8StrengthHigh value out of range, [0, 7], %d\n", Crosstalk.stManual.stParaAPI.u8StrengthLow);
        Crosstalk.stManual.stParaAPI.u8StrengthLow = 255;
    }
    Crosstalk.stManual.stParaAPI.u8StrengthV2 = tCrosstalk->u8StrengthV2;
    if(Crosstalk.stManual.stParaAPI.u8StrengthV2 > 31)
    {
        MIXER_WARN("u16ThresholdV2 value out of range, [0, 31], %d\n", tCrosstalk->u8StrengthV2);
        Crosstalk.stManual.stParaAPI.u8StrengthV2 = 31;
    }
#elif TARGET_CHIP_I6
    Crosstalk.stManual.stParaAPI.u16ThresholdOffsetV2 = (MI_U16)tCrosstalk->u16ThresholdOffsetV2;
    if(Crosstalk.stManual.stParaAPI.u16ThresholdOffsetV2  > 4095)
    {
        MIXER_WARN("u16ThresholdOffsetV2 value out of range, [0, 4095], %d\n", Crosstalk.stManual.stParaAPI.u16ThresholdOffsetV2);
        Crosstalk.stManual.stParaAPI.u16ThresholdOffsetV2 = 4095;
    }

    Crosstalk.stManual.stParaAPI.u16ThresholdV2 = tCrosstalk->u16ThresholdV2;
    if(Crosstalk.stManual.stParaAPI.u16ThresholdV2 > 255)
    {
        MIXER_WARN("u16ThresholdV2 value out of range, [0, 255], %d\n", tCrosstalk->u16ThresholdV2);
        Crosstalk.stManual.stParaAPI.u16ThresholdV2 = 255;
    }
    Crosstalk.stManual.stParaAPI.u8StrengthV2 = tCrosstalk->u8StrengthV2;
    if(Crosstalk.stManual.stParaAPI.u8StrengthV2 > 31)
    {
        MIXER_WARN("u16ThresholdV2 value out of range, [0, 31], %d\n", tCrosstalk->u8StrengthV2);
        Crosstalk.stManual.stParaAPI.u8StrengthV2 = 31;
    }
#elif TARGET_CHIP_I6E

    Crosstalk.stManual.stParaAPI.u16Offset = (MI_U16)tCrosstalk->u16Offset;
    if(Crosstalk.stManual.stParaAPI.u16Offset > 4095)
        Crosstalk.stManual.stParaAPI.u16Offset = 4095;

    Crosstalk.stManual.stParaAPI.u8Threshold = tCrosstalk->u8Threshold;
    if(Crosstalk.stManual.stParaAPI.u8Threshold > 255)
    {
        MIXER_WARN("u16ThresholdV2 value out of range, [0, 255], %d\n", tCrosstalk->u8Threshold);
        Crosstalk.stManual.stParaAPI.u8Threshold = 255;
    }
    Crosstalk.stManual.stParaAPI.u8Strength = tCrosstalk->u8Strength;
    if(Crosstalk.stManual.stParaAPI.u8Strength > 31)
    {
        MIXER_WARN("u16ThresholdV2 value out of range, [0, 31], %d\n", tCrosstalk->u8Strength);
        Crosstalk.stManual.stParaAPI.u8Strength = 31;
    }
#elif TARGET_CHIP_I6B0

    Crosstalk.stManual.stParaAPI.u16Offset = (MI_U16)tCrosstalk->u16Offset;
    if(Crosstalk.stManual.stParaAPI.u16Offset > 4095)
        Crosstalk.stManual.stParaAPI.u16Offset = 4095;

    Crosstalk.stManual.stParaAPI.u8Threshold = tCrosstalk->u8Threshold;
    if(Crosstalk.stManual.stParaAPI.u8Threshold > 255)
    {
        MIXER_WARN("u16ThresholdV2 value out of range, [0, 255], %d\n", tCrosstalk->u8Threshold);
        Crosstalk.stManual.stParaAPI.u8Threshold = 255;
    }
    Crosstalk.stManual.stParaAPI.u8Strength = tCrosstalk->u8Strength;
    if(Crosstalk.stManual.stParaAPI.u8Strength > 31)
    {
        MIXER_WARN("u16ThresholdV2 value out of range, [0, 31], %d\n", tCrosstalk->u8Strength);
        Crosstalk.stManual.stParaAPI.u8Strength = 31;
    }
#endif

    MI_ISP_IQ_SetCrossTalk(0, &Crosstalk);
}

static void IspTestDP(MI_BOOL bHotPixEn, MI_BOOL u16HotPixCompSlpoe, MI_BOOL bDarkPixEn, MI_BOOL u16DarkPixCompSlpoe)
{
    MI_ISP_IQ_DYNAMIC_DP_TYPE_t DefectPixel;

    MI_ISP_IQ_GetDynamicDP(0, &DefectPixel);

    DefectPixel.stManual.stParaAPI.bHotPixEn = (MI_ISP_BOOL_e)bHotPixEn;

    DefectPixel.stManual.stParaAPI.u16HotPixCompSlpoe = u16HotPixCompSlpoe;

    DefectPixel.stManual.stParaAPI.bDarkPixEn = (MI_ISP_BOOL_e)bDarkPixEn;

    DefectPixel.stManual.stParaAPI.u16DarkPixCompSlpoe = (BOOL)u16DarkPixCompSlpoe;

    DefectPixel.bEnable = (MI_ISP_BOOL_e)TRUE;

    DefectPixel.enOpType = SS_OP_TYP_MANUAL;
    MI_ISP_IQ_SetDynamicDP(0, &DefectPixel);
}

static void IspTestBlackLevel(const struct _blacklevel *tbl)
{
    if(NULL == tbl)
    {
        MIXER_ERR("err, tCrosstalk is null\n");
        return;
    }

    MI_ISP_IQ_OBC_TYPE_t BlackLevel;

    MI_ISP_IQ_GetOBC(0, &BlackLevel);

    BlackLevel.bEnable = (MI_ISP_BOOL_e)TRUE;
    BlackLevel.enOpType = SS_OP_TYP_MANUAL;

    BlackLevel.stManual.stParaAPI.u16ValB = tbl->b;
    if(BlackLevel.stManual.stParaAPI.u16ValB > 255)
    {
        MIXER_WARN("ValB out of range. [0, 255]. %d\n",BlackLevel.stManual.stParaAPI.u16ValB );
        BlackLevel.stManual.stParaAPI.u16ValB = 255;
    }

    BlackLevel.stManual.stParaAPI.u16ValGb = tbl->gb;
    if(BlackLevel.stManual.stParaAPI.u16ValGb > 255)
    {
        MIXER_WARN("ValGB out of range. [0, 255]. %d\n", BlackLevel.stManual.stParaAPI.u16ValGb);
        BlackLevel.stManual.stParaAPI.u16ValGb = 255;
    }

    BlackLevel.stManual.stParaAPI.u16ValGr = tbl->gr;
    if(BlackLevel.stManual.stParaAPI.u16ValGr > 255)
    {
        MIXER_WARN("ValGB out of range. [0, 255]. %d\n", BlackLevel.stManual.stParaAPI.u16ValGr);
        BlackLevel.stManual.stParaAPI.u16ValGr = 255;
    }

    BlackLevel.stManual.stParaAPI.u16ValR = tbl->r;
    if(BlackLevel.stManual.stParaAPI.u16ValR > 255)
    {
        MIXER_WARN("ValGB out of range. [0, 255]. %d\n", BlackLevel.stManual.stParaAPI.u16ValR);
        BlackLevel.stManual.stParaAPI.u16ValR = 255;
    }

    MI_ISP_IQ_SetOBC(0, &BlackLevel);
}
#if TARGET_CHIP_I5
static void IspTestDefog(const struct _deflog *tdef)
{
#if !TARGET_OS_DUAL
    if(NULL == tdef)
    {
        MIXER_ERR("err, tdef is null\n");
        return;
    }

    MI_ISP_IQ_DEFOG_TYPE_t Defog;
    MI_ISP_IQ_GetDefog(0, &Defog);
    Defog.bEnable = (MI_ISP_BOOL_e)!!tdef->enable;

    #if TARGET_CHIP_I5
    Defog.enOpType  = SS_OP_TYP_MANUAL;
    Defog.u8Strength = tdef->strength;
    if(Defog.u8Strength  > 100)
    {
        MIXER_WARN("IQ_SetDefog u8Strength [0,100]. %d\n", Defog.u8Strength);
        Defog.u8Strength = 100;
    }
    #endif

    MI_ISP_IQ_SetDefog(0, &Defog);
#endif
}
#endif
static void IspTestAeAttr(const struct _AeAttr *tAeAttr)
{
    if(NULL == tAeAttr)
    {
        MIXER_ERR("err, tAeAttr is null\n");
        return;
    }

    int i = 0;
    MI_ISP_AE_WIN_WEIGHT_TYPE_t sWinWeight;
    MI_ISP_AE_EXPO_LIMIT_TYPE_t sExpoLimit;
    MI_ISP_AE_INTP_LUT_TYPE_t AEtarget;
    MI_U32 Weight_average[1024] =
    {
        253,253,253,253,254,254,254,254,254,254,254,254,253,253,253,253,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        253,253,253,254,254,254,254,254,254,254,254,254,254,253,253,253,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        253,253,254,254,254,254,254,254,254,254,254,254,254,254,253,253,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        253,254,254,254,254,254,254,254,254,254,254,254,254,254,254,253,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        254,254,254,254,254,254,255,255,255,255,254,254,254,254,254,254,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        254,254,254,254,254,255,255,255,255,255,255,254,254,254,254,254,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        254,254,254,254,255,255,255,255,255,255,255,255,254,254,254,254,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        254,254,254,254,255,255,255,255,255,255,255,255,254,254,254,254,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        254,254,254,254,255,255,255,255,255,255,255,255,254,254,254,254,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        254,254,254,254,255,255,255,255,255,255,255,255,254,254,254,254,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        254,254,254,254,254,255,255,255,255,255,255,254,254,254,254,254,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        254,254,254,254,254,254,255,255,255,255,254,254,254,254,254,254,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        253,254,254,254,254,254,254,254,254,254,254,254,254,254,254,253,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        253,253,254,254,254,254,254,254,254,254,254,254,254,254,253,253,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        253,253,253,254,254,254,254,254,254,254,254,254,254,253,253,253,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        253,253,253,253,254,254,254,254,254,254,254,254,253,253,253,253,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    };
    MI_U32 Weight_center[1024] =
    {
        60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        60,60,60,60,60,254,255,255,255,255,60,60,60,60,60,60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        60,60,60,60,60,255,255,255,255,255,60,60,60,60,60,60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        60,60,60,60,60,255,255,255,255,255,60,60,60,60,60,60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        60,60,60,60,60,255,255,255,255,255,60,60,60,60,60,60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        60,60,60,60,60,255,255,255,255,255,60,60,60,60,60,60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        60,60,60,60,60,255,255,255,255,255,60,60,60,60,60,60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        60,60,60,60,60,255,255,255,255,255,60,60,60,60,60,60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    };
    MI_U32 Weight_spot[1024] =
    {
        0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,0 ,0 ,0 ,0 ,1 ,1 ,1 ,1 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,0 ,0 ,1 ,1 ,2 ,3 ,3 ,2 ,1 ,1 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,0 ,1 ,2 ,4 ,7 ,9 ,9 ,7 ,4 ,2 ,1 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,0 ,1 ,4 ,9 ,16 ,23 ,23 ,16 ,9 ,4 ,1 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,1 ,2 ,7 ,16 ,34 ,57 ,57 ,34 ,16 ,7 ,2 ,1 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,1 ,3 ,9 ,23 ,57 ,131 ,131 ,57 ,23 ,9 ,3 ,1 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,1 ,3 ,9 ,23 ,57 ,131 ,131 ,57 ,23 ,9 ,3 ,1 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,1 ,2 ,7 ,16 ,34 ,57 ,57 ,34 ,16 ,7 ,2 ,1 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,0 ,1 ,4 ,9 ,16 ,23 ,23 ,16 ,9 ,4 ,1 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,0 ,1 ,2 ,4 ,7 ,9 ,9 ,7 ,4 ,2 ,1 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,0 ,0 ,1 ,1 ,2 ,3 ,3 ,2 ,1 ,1 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,0 ,0 ,0 ,0 ,1 ,1 ,1 ,1 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
        0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0,
    };

    // 1 weight table, 2 exposure limit, 3 target luma,5 default ;
    switch(tAeAttr->index)
    {
        case 1:
            // 1 average table, 2 center table, 3 spot tale;

            if(1 == tAeAttr->y.mode)
            {
                MI_ISP_AE_GetWinWgt(0, &sWinWeight);
                sWinWeight.eTypeID = SS_AE_WEIGHT_AVERAGE;
                for(i = 0; i < 1024; i++)
                {
                    sWinWeight.stParaAPI.u8AverageTbl[i] = Weight_average[i];
                }
                MI_ISP_AE_SetWinWgt(0, &sWinWeight);
            }
            else if(2 == tAeAttr->y.mode)
            {
                MI_ISP_AE_GetWinWgt(0, &sWinWeight);
                sWinWeight.eTypeID = SS_AE_WEIGHT_CENTER;
                for(i = 0; i < 1024; i++)
                {
                    sWinWeight.stParaAPI.u8AverageTbl[i] = Weight_center[i];
                }
                MI_ISP_AE_SetWinWgt(0, &sWinWeight);
            }
            else if(3 == tAeAttr->y.mode)
            {
                MI_ISP_AE_GetWinWgt(0, &sWinWeight);
                sWinWeight.eTypeID = SS_AE_WEIGHT_SPOT;
                for(i = 0; i < 1024; i++)
                {
                    sWinWeight.stParaAPI.u8AverageTbl[i] = Weight_spot[i];
                }
                MI_ISP_AE_SetWinWgt(0, &sWinWeight);
            }
            else
            {
                MIXER_WARN("mode out of range. [1, 3]. %d\n", tAeAttr->y.mode);
            }
            break;

        case 2:

            MI_ISP_AE_GetExposureLimit(0, &sExpoLimit);
            sExpoLimit.u32MinISPGain = 1024;
            sExpoLimit.u32MinSensorGain = 1024;
            sExpoLimit.u32MinShutterUS = 150;
            sExpoLimit.u32MaxShutterUS = 4000;
            sExpoLimit.u32MaxSensorGain = 1024;
            sExpoLimit.u32MaxISPGain = 1024;
            sExpoLimit.u32MinFNx10 =21;
            sExpoLimit.u32MaxFNx10 =21;
            MI_ISP_AE_SetExposureLimit(0, &sExpoLimit);
            break;
        case 3:
            //if((MI_U32)tAeAttr->y.luma==0)
            //{
            // printf("Please set target_luma:");
            // scanf("%d", &((MI_U32)tAeAttr->y.luma));
            //}
            MI_ISP_AE_GetExposureLimit(0, &sExpoLimit);
            sExpoLimit.u32MinISPGain = 1024;
            sExpoLimit.u32MinSensorGain = 1024;
            sExpoLimit.u32MinShutterUS = 150;
            sExpoLimit.u32MaxShutterUS = 40000;
            sExpoLimit.u32MaxSensorGain = 1024*1024;
            sExpoLimit.u32MaxISPGain = 1024*8;
            sExpoLimit.u32MinFNx10 =21;
            sExpoLimit.u32MaxFNx10 =21;
            MI_ISP_AE_SetExposureLimit(0, &sExpoLimit);

            MI_ISP_AE_GetTarget(0, &AEtarget);
            AEtarget.u16NumOfPoints=16;
            for(i=0;i<16;i++)
            {
                AEtarget.u32X[i]=(MI_U32)0;
                AEtarget.u32Y[i]=(MI_U32)tAeAttr->y.luma;
            }

            MI_ISP_AE_SetTarget(0,&AEtarget);
            break;

        default:
            MI_ISP_AE_GetWinWgt(0, &sWinWeight);
            sWinWeight.eTypeID = SS_AE_WEIGHT_AVERAGE;
            for(i = 0; i < 1024; i++)
            {
                sWinWeight.stParaAPI.u8AverageTbl[i] = Weight_average[i];
            }
            MI_ISP_AE_SetWinWgt(0, &sWinWeight);
            MI_ISP_AE_GetExposureLimit(0, &sExpoLimit);
            sExpoLimit.u32MinISPGain = 1024;
            sExpoLimit.u32MinSensorGain = 1024;
            sExpoLimit.u32MinShutterUS = 150;
            sExpoLimit.u32MaxShutterUS = 40000;
            sExpoLimit.u32MaxSensorGain = 1024*1024;
            sExpoLimit.u32MaxISPGain = 1024*8;
            sExpoLimit.u32MinFNx10 =21;
            sExpoLimit.u32MaxFNx10 =21;
            MI_ISP_AE_SetExposureLimit(0, &sExpoLimit);
            MI_ISP_AE_GetTarget(0, &AEtarget);
            AEtarget.u16NumOfPoints=16;
            for(i=0;i<16;i++)
            {
                AEtarget.u32X[i]=(MI_U32)0;
                AEtarget.u32Y[i]=(MI_U32)470;
            }

            MI_ISP_AE_SetTarget(0,&AEtarget);
            break;
    }
}

static void IspTestAE_EXPO_INFO(void)
{
    MI_ISP_AE_EXPO_INFO_TYPE_t ExpInfo;

    MI_ISP_AE_QueryExposureInfo(0, &ExpInfo);
    printf("u32LVx10 = %u\n", ExpInfo.u32LVx10);
    printf("s32BV = %u\n", ExpInfo.s32BV);
}

static void IspTest3DNRAttr(const struct _NR3DAttr *t3DnrAttr)
{
    if(NULL == t3DnrAttr)
    {
        MIXER_ERR("err, t3DnrAttr is null\n");
        return;
    }

    int i;
    MI_ISP_IQ_NR3D_TYPE_t nNR3D;

    MI_ISP_IQ_GetNR3D(0, &nNR3D);

    nNR3D.bEnable = (MI_ISP_BOOL_e)TRUE;
    nNR3D.enOpType = SS_OP_TYP_MANUAL;

    nNR3D.stManual.stParaAPI.u8TfStr = (MI_U8)t3DnrAttr->u8TfStr;
    if(nNR3D.stManual.stParaAPI.u8TfStr > 64)
    {
        MIXER_WARN("u8TfStr out of range. [0, 64]. %d\n", nNR3D.stManual.stParaAPI.u8TfStr);
        nNR3D.stManual.stParaAPI.u8TfStr = 64;
    }

    nNR3D.stManual.stParaAPI.u8TfStrEx = (MI_U8)t3DnrAttr->u8TfStrEx;
    if(nNR3D.stManual.stParaAPI.u8TfStrEx > 64)
    {
        MIXER_WARN("u8TfStrEx out of range. [0, 64]. %d\n", nNR3D.stManual.stParaAPI.u8TfStrEx );
        nNR3D.stManual.stParaAPI.u8TfStrEx  = 64;
    }

    for(i=0;i<16;i++)
    {
        #if (TARGET_CHIP_I5 || TARGET_CHIP_I6)
        nNR3D.stManual.stParaAPI.u8TfLut[i] = (MI_U8)t3DnrAttr->u8TfLut;
        if(nNR3D.stManual.stParaAPI.u8TfLut[i] > 63)
        {
            MIXER_WARN("u8TfStrEx out of range. [0, 63]. %d\n", nNR3D.stManual.stParaAPI.u8TfLut[i]);
            nNR3D.stManual.stParaAPI.u8TfLut[i] = 63;
        }
        #else
        nNR3D.stManual.stParaAPI.u16TfLut[i] = (MI_U16)t3DnrAttr->u8TfLut;
        if(nNR3D.stManual.stParaAPI.u16TfLut[i] > 63)
        {
            MIXER_WARN("u8TfStrEx out of range. [0, 63]. %d\n", nNR3D.stManual.stParaAPI.u16TfLut[i]);
            nNR3D.stManual.stParaAPI.u16TfLut[i] = 63;
        }
        #endif
    }
    MI_ISP_IQ_SetNR3D(0, &nNR3D);
}

static void IspTestAwbAttr(const struct _AWBAttr *tAwbAttr)
{
    if(NULL == tAwbAttr)
    {
        MIXER_ERR("err, tAwbAttr is null\n");
        return;
    }

    MI_ISP_AWB_ATTR_TYPE_t WBAttr;
    MI_ISP_AWB_ATTR_EX_TYPE_t WBAttrEX;

    MI_ISP_AWB_GetAttr(0, &WBAttr);
    WBAttr.eOpType = SS_OP_TYP_AUTO;
    WBAttr.stAutoParaAPI.eAlgType = (MI_ISP_AWB_ALG_TYPE_e)1;    //AWB_ALG_ADVANCE;
    MI_ISP_AWB_SetAttr(0, &WBAttr);

    MI_ISP_AWB_GetAttrEx(0, &WBAttrEX);
    WBAttrEX.bExtraLightEn = (MI_ISP_BOOL_e)TRUE;
    WBAttrEX.stLightInfo[0].bExclude = MI_ISP_BOOL_e(TRUE);

#if TARGET_CHIP_I5
#if !TARGET_OS_DUAL
    WBAttrEX.u8AreaScale = tAwbAttr->AreaScale;
    if(WBAttrEX.u8AreaScale > 16)
    {
        MIXER_WARN("AWB_SetAttrEx AreaScale [0,16]. %d\n", WBAttrEX.u8AreaScale);
        WBAttrEX.u8AreaScale = 16;
    }
#endif
#endif

    WBAttrEX.stLightInfo[0].u16WhiteBgain = tAwbAttr->u16WhiteBgain;
    if(WBAttrEX.stLightInfo[0].u16WhiteBgain > 4095)
    {
        MIXER_WARN("AWB_SetAttrEx u16WhiteBgain [256, 4095]. %d\n", WBAttrEX.stLightInfo[0].u16WhiteBgain);
        WBAttrEX.stLightInfo[0].u16WhiteBgain = 4095;
    }
    else if(WBAttrEX.stLightInfo[0].u16WhiteBgain < 256)
    {
        MIXER_WARN("AWB_SetAttrEx u16WhiteBgain [256,4095]. %d\n", WBAttrEX.stLightInfo[0].u16WhiteBgain);
        WBAttrEX.stLightInfo[0].u16WhiteBgain = 4095;
    }

    WBAttrEX.stLightInfo[0].u16WhiteRgain = tAwbAttr->u16WhiteRgain;
    if(WBAttrEX.stLightInfo[0].u16WhiteRgain > 4095)
    {
        MIXER_WARN("AWB_SetAttrEx u16WhiteRgain [256,4095]. %d\n", WBAttrEX.stLightInfo[0].u16WhiteRgain);
        WBAttrEX.stLightInfo[0].u16WhiteRgain = 4095;
    }
    else if(WBAttrEX.stLightInfo[0].u16WhiteRgain < 256)
    {
        MIXER_WARN("AWB_SetAttrEx u16WhiteRgain [256,4095]. %d\n", WBAttrEX.stLightInfo[0].u16WhiteRgain);
        WBAttrEX.stLightInfo[0].u16WhiteRgain = 4095;
    }

    WBAttrEX.stLightInfo[0].u8AreaSize = tAwbAttr->u8AreaSize;
    if(WBAttrEX.stLightInfo[0].u8AreaSize > 32)
    {
        MIXER_WARN("AWB_SetAttrEx u8AreaSize [1,32]. %d\n", WBAttrEX.stLightInfo[0].u8AreaSize);
        WBAttrEX.stLightInfo[0].u8AreaSize = 32;
    }
    else if(WBAttrEX.stLightInfo[0].u8AreaSize < 1)
    {
        MIXER_WARN("AWB_SetAttrEx u8AreaSize [1,32]. %d\n", WBAttrEX.stLightInfo[0].u8AreaSize);
        WBAttrEX.stLightInfo[0].u8AreaSize = 1;
    }

    MI_ISP_AWB_SetAttrEx(0, &WBAttrEX);

#if TARGET_CHIP_I5
#if !TARGET_OS_DUAL
    printf("AreaScale = %d\n", WBAttrEX.u8AreaScale);
#endif
#endif

    printf("LightInfo[0].u2WhiteBgain = %hu\n", WBAttrEX.stLightInfo[0].u16WhiteBgain);
    printf("LightInfo[0].u2WhiteRgain = %hu\n", WBAttrEX.stLightInfo[0].u16WhiteRgain);
    printf("LightInfo[0].uAreaSize = %d\n", WBAttrEX.stLightInfo[0].u8AreaSize);
}

static void IspTestAWB_QUERY_INFO(void)
{
    MI_ISP_AWB_QUERY_INFO_TYPE_t WBInfo;

    MI_ISP_AWB_QueryInfo(0, &WBInfo);
    printf("Bgain:%hu\n", WBInfo.u16Bgain);
    printf("ColorTemp:%hu\n", WBInfo.u16ColorTemp);
    printf("Gbgain:%hu\n", WBInfo.u16Gbgain);
    printf("Grgain:%hu\n", WBInfo.u16Grgain);
    printf("Rgain:%hu\n", WBInfo.u16Rgain);
}

static void IspTestAfInfo(void)
{
    CusAFStats_t AFInfo;
#if TARGET_CHIP_I5|| TARGET_CHIP_I6E || TARGET_CHIP_I6B0 || TARGET_CHIP_I6
    int i4WinID=0;
    // MI_ISP_CUS3A_GetAFWindow(0, &AFWin);
    CusAFWin_t AFWin[16] =
    {
        {   0,
            .stParaAPI = { 0, 0, 256, 256},
        },
        {   1,
            .stParaAPI = { 257,   0, 512, 256},
        },
        {   2,
            .stParaAPI = { 513,   0,768, 256},
        },
        {   3,
            .stParaAPI = { 769, 0, 1023, 256},
        },
        {   4,
            .stParaAPI = { 0, 256, 256, 512},
        },
        {   5,
            .stParaAPI = {257, 257,512, 512},
        },
        {   6,
            .stParaAPI = { 513, 257, 768,512},
        },
        {   7,
            .stParaAPI = { 769, 257, 1023,512},
        },
        {   8,
            .stParaAPI = { 0, 513, 256,768},
        },
        {   9,
            .stParaAPI = {257,513, 512,768},
        },
        {   10,
            .stParaAPI = {513,513, 768,768},
        },
        {   11,
            .stParaAPI = {769,513, 1023,768},
        },
        {   12,
            .stParaAPI = {0,769, 256,1023},
        },
        {   13,
            .stParaAPI = {257,769, 512,1023},
        },
        {   14,
            .stParaAPI = {513,769, 768,1023},
        },
        {   15,
            .stParaAPI = {769,769, 1023,1023},
        },
    };


    MI_ISP_CUS3A_SetAFWindow(0, AFWin);
    usleep(1000);

    MI_ISP_CUS3A_GetAFWindow(0, AFWin);
    for(i4WinID=0;i4WinID<16;i4WinID++)
    {
        printf("AFWin.start_x=%d AFWin.start_y=%d AFWin.end_x=%d AFWin.end_y=%d\n", AFWin[i4WinID].stParaAPI.u32StartX, AFWin[i4WinID].stParaAPI.u32StartY, AFWin[i4WinID].stParaAPI.u32EndX, AFWin[i4WinID].stParaAPI.u32EndY);
    }
    memset(&AFInfo, 0, sizeof(CusAFStats_t));
    MI_ISP_CUS3A_GetAFStats(0, &AFInfo);

    for(i4WinID=0;i4WinID<80;i4WinID=i4WinID+5)
    {
        printf("[AF] win0 iir1:0x%02x%02x%02x%02x%02x\n", AFInfo.stParaAPI[0].iir_1[i4WinID+4],AFInfo.stParaAPI[0].iir_1[i4WinID+3],AFInfo.stParaAPI[0].iir_1[i4WinID+2],AFInfo.stParaAPI[0].iir_1[i4WinID+1],AFInfo.stParaAPI[0].iir_1[i4WinID]);
    }
/*#elif TARGET_CHIP_I6
    CusAFWin_t AFWin;

    memset(&AFWin, 0, sizeof(CusAFWin_t));
    MI_ISP_CUS3A_SetAFWindow(0, &AFWin);
    printf("AFWin.start_x=%d AFWin.start_y=%d AFWin.end_x=%d AFWin.end_y=%d", AFWin.stParaAPI.u32StartX, AFWin.stParaAPI.u32StartY, AFWin.stParaAPI.u32EndX, AFWin.stParaAPI.u32EndY);

    memset(&AFWin, 0, sizeof(CusAFWin_t));
    MI_ISP_CUS3A_GetAFWindow(0, &AFWin);
    printf("AFWin.start_x=%d AFWin.start_y=%d AFWin.end_x=%d AFWin.end_y=%d", AFWin.stParaAPI.u32StartX, AFWin.stParaAPI.u32StartY, AFWin.stParaAPI.u32EndX, AFWin.stParaAPI.u32EndY);

    memset(&AFInfo, 0, sizeof(CusAFStats_t));
    MI_ISP_CUS3A_GetAFStats(0, &AFInfo);*/
#endif
}

static void IspTestNRDESPIKE(const struct _nrDespike *tDesp)
{
    if(NULL == tDesp)
    {
        MIXER_ERR("err, tDesp is null\n");
        return;
    }
    MI_ISP_IQ_NRDESPIKE_TYPE_t spike;

    MI_ISP_IQ_GetNRDeSpike(0, &spike);

    spike.bEnable = (MI_ISP_BOOL_e)TRUE;
    spike.enOpType = SS_OP_TYP_MANUAL;
    #if (TARGET_CHIP_I5 || TARGET_CHIP_I6)
    spike.stManual.stParaAPI.u16DiffThdCornerCross = (MI_U16)tDesp->DiffThdCornerCross;
    if(spike.stManual.stParaAPI.u16DiffThdCornerCross > 255)
    {
        MIXER_WARN("DiffThdCornerCross out of range. [0, 15]. %d\n", spike.stManual.stParaAPI.u16DiffThdCornerCross);
        spike.stManual.stParaAPI.u16DiffThdCornerCross = 255;
    }
    #else
    spike.stManual.stParaAPI.u8DiffThdCornerCross = (MI_U16)tDesp->DiffThdCornerCross;
    if(spike.stManual.stParaAPI.u8DiffThdCornerCross > 255)
    {
        MIXER_WARN("DiffThdCornerCross out of range. [0, 255]. %d\n", spike.stManual.stParaAPI.u8DiffThdCornerCross);
        spike.stManual.stParaAPI.u8DiffThdCornerCross = 255;
    }
    #endif

    spike.stManual.stParaAPI.u8BlendRatio = (MI_U8)tDesp->BlendRatio;
    if(spike.stManual.stParaAPI.u8BlendRatio > 15)
    {
        MIXER_WARN("BlendRatio out of range. [0, 15]. %d\n", spike.stManual.stParaAPI.u8BlendRatio);
        spike.stManual.stParaAPI.u8BlendRatio = 15;
    }
    //spike.stManual.stParaAPI.u8BlendRatio = (MI_U8)tDesp->BlendRatio;
    MI_ISP_IQ_SetNRDeSpike(0, &spike);
}

static void IspTestNrLuma(const struct _nrLuma *tNrLuma)
{
    if(NULL == tNrLuma)
    {
        MIXER_ERR("err, tNrLuma is null\n");
        return;
    }

    MI_ISP_IQ_NRLUMA_TYPE_t nrluma;

    MI_ISP_IQ_GetNRLuma(0, &nrluma);
    nrluma.bEnable = (MI_ISP_BOOL_e)TRUE;
    nrluma.enOpType = SS_OP_TYP_MANUAL;

#if TARGET_CHIP_I5
    nrluma.stManual.stParaAPI.bEnLscReference = (MI_ISP_BOOL_e)tNrLuma->bEnLscReference;

    nrluma.stManual.stParaAPI.u8FilterLevel = (MI_U8)tNrLuma->u8FilterLevel;
    if(nrluma.stManual.stParaAPI.u8FilterLevel > 5)
    {
        MIXER_WARN("IQ_SetNRLuma u8FilterLevel [0,5]. %d\n", nrluma.stManual.stParaAPI.u8FilterLevel);
        nrluma.stManual.stParaAPI.u8FilterLevel  = 5;
    }

    nrluma.stManual.stParaAPI.u8BlendRatio = (MI_U8)tNrLuma->u8BlendRatio;
    if(nrluma.stManual.stParaAPI.u8BlendRatio > 63)
    {
        MIXER_WARN("IQ_SetNRLuma u8BlendRatio [0,63]. %d\n", nrluma.stManual.stParaAPI.u8BlendRatio );
        nrluma.stManual.stParaAPI.u8BlendRatio = 63;
    }

    MI_ISP_IQ_SetNRLuma(0, &nrluma);
    printf("bEnLscReference:%d ", nrluma.stManual.stParaAPI.bEnLscReference);
    printf("u8FilterLevel:%d ", nrluma.stManual.stParaAPI.u8FilterLevel);
    printf("u8BlendRatio:%d\n", nrluma.stManual.stParaAPI.u8BlendRatio);
#elif TARGET_CHIP_I6
    MI_U16 u16manual_LumaStrByY_L[10]={0,0,0,0,0,0,0,0,0,0};
    MI_U16 u16manual_LumaStrByY_N[10]={260,262,262,264,265,280,285,300,320,335};
    MI_U16 u16manual_LumaStrByY_H[10]={510,510,510,510,510,510,510,510,510,510};
    MI_U16 j = 0x0;

    nrluma.stManual.stParaAPI.bLumaAdjEn =(MI_ISP_BOOL_e)TRUE;
    // NRluma parameters: 1 Low strength, 2 Normal, 3 High strength;
    switch(tNrLuma->index )
    {
        case 1:
            for(j=0;j<10;j++)
            {nrluma.stManual.stParaAPI.u16LumaStrByY[j]=u16manual_LumaStrByY_L[j];}
            break;

        case 2:
            for(j=0;j<10;j++)
            {nrluma.stManual.stParaAPI.u16LumaStrByY[j]=u16manual_LumaStrByY_N[j];}
            break;

        case 3:
            for(j=0;j<10;j++)
            {nrluma.stManual.stParaAPI.u16LumaStrByY[j]=u16manual_LumaStrByY_H[j];}
            break;

        default:
            printf("%s:%d Input wrong NRluma index:%d, only support 1, 2, or 3.\n", __func__, __LINE__, tNrLuma->index);
            break;
    }
#elif TARGET_CHIP_I6B0
    nrluma.bEnable = SS_TRUE;
    nrluma.enOpType = SS_OP_TYP_MANUAL;
    nrluma.stManual.stParaAPI.u8Strength = 31;
    nrluma.stManual.stParaAPI.u8GMaskSel = 1;
    switch(tNrLuma->index)
    {
        case 1:
            nrluma.stManual.stParaAPI.u16SpfBlendLut[0]=0;
            nrluma.stManual.stParaAPI.u16SpfBlendLut[1] =0;
            break;

        case 2:
            nrluma.stManual.stParaAPI.u16SpfBlendLut[0]=128;
            nrluma.stManual.stParaAPI.u16SpfBlendLut[1] =128;
            break;

        case 3:
            nrluma.stManual.stParaAPI.u16SpfBlendLut[0]=256;
            nrluma.stManual.stParaAPI.u16SpfBlendLut[1] =256;
            break;

        default:
            printf("%s:%d Input wrong NRluma index:%d, only support 1, 2, or 3.\n", __func__, __LINE__, tNrLuma->index);
            break;
    }
#endif
    MI_ISP_IQ_SetNRLuma(0, &nrluma);
#if TARGET_CHIP_I6B0
    MI_ISP_IQ_GetNRLuma(0, &nrluma);

    MIXER_DBG("Now :  nrluma.bEnable =%d,enOpType=%d,u8Strength=%d,u8GMaskSel=%d,u16SpfBlendLut[0]=%d,u16SpfBlendLut[1]=%d\n",\
        nrluma.bEnable,nrluma.enOpType,nrluma.stManual.stParaAPI.u8Strength,nrluma.stManual.stParaAPI.u8GMaskSel,\
        nrluma.stManual.stParaAPI.u16SpfBlendLut[0],nrluma.stManual.stParaAPI.u16SpfBlendLut[1]);
#endif

}

static void IspTestHSV(const struct _hsv *tHsv)
{
    if(NULL == tHsv)
    {
        MIXER_ERR("err, tHsv is null\n");
        return;
    }

    int j = 0x0;
    MI_ISP_IQ_HSV_TYPE_t HSV;

    MI_ISP_IQ_GetHSV(0, &HSV);

    HSV.bEnable = (MI_ISP_BOOL_e)TRUE;
    HSV.enOpType = SS_OP_TYP_MANUAL;
    HSV.stManual.stParaAPI.s16HueLut[0] = (MI_S16)tHsv->s16HueLut;
    if(HSV.stManual.stParaAPI.s16HueLut[0]  < -64)
    {
        MIXER_WARN("IQ_SetHSV s16HueLut [-64,64]. %d\n", HSV.stManual.stParaAPI.s16HueLut[0] );
        HSV.stManual.stParaAPI.s16HueLut[0]  = -64;
    }
    else if(HSV.stManual.stParaAPI.s16HueLut[0]  > 64)
    {
        MIXER_WARN("IQ_SetHSV s16HueLut [-64,64]. %d\n", HSV.stManual.stParaAPI.s16HueLut[0] );
        HSV.stManual.stParaAPI.s16HueLut[0]  = 64;
    }


    HSV.stManual.stParaAPI.u16SatLut[0] = (MI_U16)tHsv->u16SatLut;
    if(HSV.stManual.stParaAPI.u16SatLut[0] > 255)
    {
        MIXER_WARN("IQ_SetHSV s16HueLut [0, 255]. %d\n", HSV.stManual.stParaAPI.u16SatLut[0]);
        HSV.stManual.stParaAPI.u16SatLut[0] = 255;
    }

    for(j = 0; j < 24; j++)
    {
        HSV.stManual.stParaAPI.s16HueLut[j] = HSV.stManual.stParaAPI.s16HueLut[0];
        HSV.stManual.stParaAPI.u16SatLut[j] = HSV.stManual.stParaAPI.u16SatLut[0];
    }
    MI_ISP_IQ_SetHSV(0, &HSV);
    printf("s16HueLut:%d", (MI_S16)HSV.stManual.stParaAPI.s16HueLut[0]);
    printf("u16SatLut:%u\n", HSV.stManual.stParaAPI.u16SatLut[0]);
}

static void IspTestRGBIR(struct _rgbir *tRgbIr)
{
    MI_ISP_IQ_RGBIR_TYPE_t RGBIR;
    MI_U16 j = 0x0;

    MI_ISP_IQ_GetRGBIR(0, &RGBIR);

    RGBIR.bEnable = (MI_ISP_BOOL_e)TRUE;
    RGBIR.enOpType = SS_OP_TYP_MANUAL;

    RGBIR.stManual.stParaAPI.bRemovelEn = (MI_ISP_BOOL_e)tRgbIr->bRemovelEn;

    RGBIR.stManual.stParaAPI.u16Ratio_R[0] = (MI_U16)tRgbIr->Ratio_r;
    for(j=0;j<6;j++)
        RGBIR.stManual.stParaAPI.u16Ratio_R[j] = RGBIR.stManual.stParaAPI.u16Ratio_R[0];

    RGBIR.stManual.stParaAPI.u16Ratio_G[0] = (MI_U16)tRgbIr->Ratio_g;
    for(j=0;j<6;j++)
        RGBIR.stManual.stParaAPI.u16Ratio_G[j] = RGBIR.stManual.stParaAPI.u16Ratio_G[0];

    RGBIR.stManual.stParaAPI.u16Ratio_B[0] = (MI_U16)tRgbIr->Ratio_b;
    for(j=0;j<6;j++)
        RGBIR.stManual.stParaAPI.u16Ratio_B[j] = RGBIR.stManual.stParaAPI.u16Ratio_B[0];

    MI_ISP_IQ_SetRGBIR(0, &RGBIR);

    printf("Ratio_R:%d ", RGBIR.stManual.stParaAPI.u16Ratio_R[0]);
    printf("Ratio_G:%d ", RGBIR.stManual.stParaAPI.u16Ratio_G[0]);
    printf("Ratio_B:%d\n", RGBIR.stManual.stParaAPI.u16Ratio_B[0]);
}

static void IspTestWdr(const struct _wdr *tWdr)
{
    if(NULL == tWdr)
    {
        MIXER_ERR("err, tWdr is null\n");
        return;
    }

    MI_ISP_IQ_WDR_TYPE_t Wdr;

    MI_ISP_IQ_GetWDR(0, &Wdr);

    Wdr.bEnable = (MI_ISP_BOOL_e)TRUE;
    Wdr.enOpType = SS_OP_TYP_MANUAL;
    Wdr.stManual.stParaAPI.u8BrightLimit  = (MI_U8)tWdr->u8BrightLimit;
    Wdr.stManual.stParaAPI.u8DarkLimit = (MI_U8)tWdr->u8DarkLimit;
    Wdr.stManual.stParaAPI.u8Strength = (MI_U16)tWdr->u8Strength;
    MI_ISP_IQ_SetWDR(0, &Wdr);
    printf("u8BrightLimit:%d ", Wdr.stManual.stParaAPI.u8BrightLimit);
    printf("u8DarkLimit:%d ", Wdr.stManual.stParaAPI.u8DarkLimit);
    printf("u8Strength:%d ", Wdr.stManual.stParaAPI.u8Strength);
}

static void IspTestLoadbin(struct _loadiqbin *tIqbin)
{
    if(NULL == tIqbin)
    {
        MIXER_ERR("err, tIqbin is null\n");
        return;
    }
    MIXER_DBG("MI_ISP_API_CmdLoadBinFile filepath = %s, key = %d\n", tIqbin->filepath, tIqbin->key);
    MI_ISP_API_CmdLoadBinFile(0,  (char *)tIqbin->filepath,   tIqbin->key);
}

static void IspTestLoadcaliData(struct _loadcali *tcaliData)
{
    if(NULL == tcaliData || NULL == tcaliData->filepath)
    {
        MIXER_ERR("err, tIqbin is null\n");
        return;
    }

    if(tcaliData->channel > 3)
    {
        MIXER_WARN("calidata channel. %d\n", tcaliData->channel);
        tcaliData->channel = 0;
    }

    if(tcaliData->caliItem >= SS_CALI_ITEM_MAX)
    {
        MIXER_WARN("caliItem(0 : AWB, 1 : OBC, 2 : SDC, 3 : ALSC, 4 : LSC). %d\n", tcaliData->caliItem);
        //tcaliData->caliItem = 0;
    }

    MI_ISP_API_CmdLoadCaliData(tcaliData->channel, (MI_ISP_CALI_ITEM_e)tcaliData->caliItem, (char *)tcaliData->filepath);
    printf("channel=%d,caliItem=%d,filePath=%s\n", tcaliData->channel, tcaliData->caliItem, tcaliData->filepath);
}

static void IspTestIrcut(struct _ircut *tIrcut)
{
    if(NULL == tIrcut)
    {
        MIXER_ERR("err, tIrcut is null\n");
        return;
    }

    mod_isp_ircut_set(tIrcut->boardtype, tIrcut->isday);
    printf("boardtype=%d, mode=%d\n", tIrcut->boardtype, tIrcut->isday);
}

static void IspTestCus3a(struct _cus3a *tCus3a)
{
    if(NULL == tCus3a)
    {
        MIXER_ERR("err, tCus3a is null\n");
        return;
    }

    switch(tCus3a->cus3a_option)
    {
        case 0:
            if(g_bCusAEenable)
            {
                isp_process_cmd(CMD_ISP_CLOSE_CUS3A, NULL, 0);
                printf("close 3A done...\n");
            }
            else
            {
                printf("misc_cus3a has close !!!\n");
            }
            break;

        case 1:
            if(!g_bCusAEenable)
            {
                MI_S32 bCus3a[3] = {0};

                bCus3a[0] = (MI_BOOL)!!tCus3a->tCus3aAttr.mAeEnable;
                bCus3a[1] = (MI_BOOL)!!tCus3a->tCus3aAttr.mAwbEnable;
                bCus3a[2] = (MI_BOOL)!!tCus3a->tCus3aAttr.mAfEnable;
                isp_process_cmd(CMD_ISP_OPEN_CUS3A, (MI_S8*)bCus3a, sizeof(bCus3a));
            }
            else
            {
                printf("misc_cus3a has open,must close first\n");
            }
            break;

        case 2:
            if(!g_bCusAEenable) //MI_CUS3A cannot open while misc_CUS3A opening
            {
                isp_process_cmd(CMD_ISP_TEST_CUS3A_API, NULL, 0);
            }
            else
            {
                printf("please close cus3A first...\n");
            }
            break;
        case 3:
            if(!g_bCusAEenable) //MI_CUS3A cannot open while misc_CUS3A opening
            {
                isp_process_cmd(CMD_ISP_CUS3A_INTERFACE_UT, NULL, 0);
            }
            else
            {
                printf("please close cus3A first...\n");
            }
            break;
        default :
            MIXER_WARN("cus3a param err. %d\n", tCus3a->cus3a_option);
            break;
    }
}

#if TARGET_CHIP_I6 || TARGET_CHIP_I6B0
static void IspTestAeSetState(const struct _ae_state * tAe)
{
    if(NULL == tAe)
    {
        MIXER_ERR("err, tAe is null\n");
        return;
    }

    MI_ISP_SM_STATE_TYPE_e AE_state;


    if(tAe->AeState ==0)
        AE_state =  SS_ISP_STATE_NORMAL;
    else
        AE_state =  SS_ISP_STATE_PAUSE;
    MI_ISP_AE_SetState(0,&AE_state);
}

/*static void IspTestAeGetState(const struct _get_ae_state * tgAe)
  {
  if(NULL == tgAe)
  {
  MIXER_ERR("err, tgAe is null\n");
  return;
  }

  MI_ISP_AE_HW_STATISTICS_t stStats;
  unsigned int channel = 0, indexX = 0, indexY = 0;

  channel = tgAe->channel;

  if(channel > 3)
  {
  MIXER_WARN("ae channel out of range. %d\n", channel);
  channel = 0;
  }
  if(0 != MI_ISP_AE_GetAeHwAvgStats(channel, &stStats))
  {
  printf("GetAeHwAvgStats error\n");
  }
  else
  {
  printf("Show AeHwAvgStats:\n");
  for(indexY = 0; indexY < stStats.nBlkY; ++indexY)
  {
  for(indexX = 0; indexX < stStats.nBlkX; ++indexX)
  {
  printf("%d ", stStats.nAvg[indexY * stStats.nBlkX + indexX].uAvgY);
  }
  printf("\n");
  }
  }
  }*/
#endif

#if MOTOR_TEST
extern int af_init_done;
#endif

void setIspParam(MI_S8 *buf[], MI_U32 ParamLength)
{
    int nValue=0x0, nMode = 0x0;
    int chosen;
    MI_U16 offset = 0x0;
    //    MI_ISP_IQ_COLORTOGRAY_TYPE_t Colortogray;
    MI_U16 j;
    //    int tmp;

    if(mod_isp_wait_ready_timeout(10*1000) < 0)
        return;


    if(NULL == buf ||ParamLength <= offset)
    {
        MIXER_ERR("wrong param\n");
        return;
    }

    for(j = 0x0; j < ParamLength; j++)
    {
        if(NULL == buf[j])
        {
            MIXER_ERR("wrong param\n");
            return;
        }
    }

    chosen = atoi((char *)buf[offset]);
    offset += 1;

    switch(chosen)
    {
        case 1:
            {
                if(ParamLength <= offset)
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                nValue = atoi((char *)buf[offset]);
                offset += 1;
                IspTestColorToGray(nValue);
                break;
            }
        case 3:
            {
                if(ParamLength <= offset)
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                nValue = atoi((char *)buf[offset]);
                offset += 1;

                if(nValue < 0 || nValue > 3)
                {
                    MIXER_WARN("mirror/flip mode:[0,3]. %d, clear it\n", nValue);
                    nValue = 0x0;
                }

                IspTestSensorMirroFlip(nValue);

                break;
            }
        case 6://whitebalance
            {
                struct _TestAWB  TestAwb;
                MI_S32 tnValue=0x0;
                memset(&TestAwb, 0x0, sizeof(TestAwb));

                if(ParamLength <= (MI_U32)(offset+4))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }

                for(j = 0x0; j < ParamLength; j++)
                {
                    if(NULL == buf[j])
                    {
                        MIXER_ERR("wrong param\n");
                        return;
                    }
                    if(0x0 == j)
                    {
                        TestAwb.eOpType =  (MI_ISP_OP_TYPE_e)!!atoi((char *)buf[offset]);
                    }
                    else
                    {
                        tnValue = mixerStr2Int((char *)buf[offset]);
                        if(tnValue < 0 || tnValue > 0x2000)
                        {
                            MIXER_ERR("wrong param\n");
                            return;
                        }
                        if(1 == j)
                            TestAwb.u16Bgain = tnValue;
                        else if(2 == j)
                            TestAwb.u16Gbgain = tnValue;
                        else if(3 == j)
                            TestAwb.u16Grgain = tnValue;
                        else if(4 == j)
                        {
                            TestAwb.u16Rgain = tnValue;
                            break;
                        }
                    }
                    offset += 1;
                }

                IspTestAWB(&TestAwb);
                break;
            }
        case 7:
            {
                if(ParamLength <= offset)
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }

                nValue = atoi((char *)buf[offset]);
                offset += 1;
                IspTestAeStrategy(nValue);

                break;
            }
        case 8:
            {
                if(ParamLength <= offset)
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                nValue = atoi((char *)buf[offset]);
                offset += 1;

                IspTestContrast(nValue);

                break;
            }
        case 9:
            {
                if(ParamLength <= offset)
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                nValue = atoi((char *)buf[offset]);
                offset += 1;
                IspTestBrightness(nValue);
                break;
            }

        case 10:
            {
                if(ParamLength <= (MI_U32)(offset+1))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                nMode = atoi((char *)buf[offset]);
                offset += 1;

                nValue = atoi((char *)buf[offset]);
                offset += 1;

                IspTestGama(nMode, nValue);
                break;
            }

        case 11:
            {
                if(ParamLength <= offset)
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                nValue = atoi((char *)buf[offset]);

                IspTestSatruation(nValue);

                break;
            }
        case 12:
            {
                if(ParamLength <= offset)
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                nValue = atoi((char *)buf[offset]);

                IspTestLightNess(nValue);

                break;
            }

        case 14:
            {
                MI_U32 nUDValue=0x0, nUValue = 0x0;

                if(ParamLength <=  (MI_U32)(offset+1))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                nUDValue = atoi((char *)buf[offset]);
                offset += 1;
                nUValue = atoi((char *)buf[offset]);

                IspTestSharpNess(nUDValue, nUValue);
                break;
            }

        case 15:
            {
                if(ParamLength <= offset)
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                nValue = atoi((char *)buf[offset]);
                IspTestFlicker(nValue);

                break;
            }

        case 17://CCM
            {
                MI_U16 nCCM[9] ={0x0};
                if(ParamLength <=  (MI_U32)(offset+8))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                for(j=0x0; j<9; j++)
                {
                    nCCM[j] = (MI_U16)atoi((char *)buf[offset]);
                    offset += 1;
                }
                IspTestCCM(nCCM);
                break;
            }

        case 18://antifalsecolor
            {
                MI_U32 SMid=0x0, UMid = 0x0;
                if(ParamLength <=  (MI_U32)(offset+1))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                SMid = atoi((char *)buf[offset]);
                offset += 1;
                UMid = atoi((char *)buf[offset]);

                IspTestFalseColor(SMid, UMid);

                break;
            }
        case 19://crosstalk
            {
                struct _crosstalk tmp;
#if TARGET_CHIP_I5
                if(ParamLength <=  (MI_U32)(offset+6))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                tmp.enV2 = atoi((char *)buf[offset]);
                offset += 1;

                tmp.u16ThresholdHigh = atoi((char *)buf[offset]);
                offset += 1;

                tmp.u16ThresholdLow = atoi((char *)buf[offset]);
                offset += 1;

                tmp.u16ThresholdV2 = atoi((char *)buf[offset]);
                offset += 1;

                tmp.u8StrengthHigh = atoi((char *)buf[offset]);
                offset += 1;

                tmp.u8StrengthLow = atoi((char *)buf[offset]);
                offset += 1;

                tmp.u8StrengthV2 = atoi((char *)buf[offset]);
                offset += 1;
#elif TARGET_CHIP_I6
                if(ParamLength <=  (MI_U32)(offset+2))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                tmp.u16ThresholdOffsetV2 = atoi((char *)buf[offset]);
                offset += 1;

                tmp.u16ThresholdV2 = atoi((char *)buf[offset]);
                offset += 1;

                tmp.u8StrengthV2 = atoi((char *)buf[offset]);
                offset += 1;
#elif TARGET_CHIP_I6E

                if(ParamLength <=  (MI_U32)(offset+2))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                tmp.u16Offset = atoi((char *)buf[offset]);
                offset += 1;

                tmp.u8Threshold = atoi((char *)buf[offset]);
                offset += 1;

                tmp.u8Strength = atoi((char *)buf[offset]);
                offset += 1;
#elif TARGET_CHIP_I6B0
                if(ParamLength <=  (MI_U32)(offset+2))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                tmp.u16Offset = atoi((char *)buf[offset]);
                offset += 1;

                tmp.u8Threshold = atoi((char *)buf[offset]);
                offset += 1;

                tmp.u8Strength = atoi((char *)buf[offset]);
                offset += 1;
#endif
                IspTestCrossTalk(&tmp);
                break;
            }

        case 20://DP
            {
                MI_BOOL bHotPixEn, u16HotPixCompSlpoe, bDarkPixEn, u16DarkPixCompSlpoe;
                if(ParamLength <=  (MI_U32)(offset+3))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                bHotPixEn = (MI_BOOL)atoi((char *)buf[offset]);
                offset += 1;

                u16HotPixCompSlpoe = (MI_BOOL)atoi((char *)buf[offset]);
                offset += 1;

                bDarkPixEn = (MI_BOOL)atoi((char *)buf[offset]);
                offset += 1;

                u16DarkPixCompSlpoe = (MI_BOOL)atoi((char *)buf[offset]);
                offset += 1;
                IspTestDP( bHotPixEn, u16HotPixCompSlpoe, bDarkPixEn, u16DarkPixCompSlpoe);

                break;
            }

        case 21://blackLevel
            {
                struct _blacklevel tbl;
                if(ParamLength <=  (MI_U32)(offset+3))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                tbl.b = (MI_BOOL)atoi((char *)buf[offset]);
                offset += 1;

                tbl.gb = (MI_BOOL)atoi((char *)buf[offset]);
                offset += 1;

                tbl.gr = (MI_BOOL)atoi((char *)buf[offset]);
                offset += 1;

                tbl.r = (MI_BOOL)atoi((char *)buf[offset]);
                offset += 1;
                IspTestBlackLevel(&tbl);
                break;
            }
        case 22://defog
            {
#if TARGET_CHIP_I5
                struct _deflog tDeflog;

                if(ParamLength <=  (MI_U32)(offset+1))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                tDeflog.enable = (MI_BOOL)atoi((char *)buf[offset]);
                offset += 1;
            //    #if !TARGET_CHIP_I6E
                tDeflog.strength = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;
            //    #endif
                IspTestDefog(&tDeflog);
#elif TARGET_CHIP_I6E || TARGET_CHIP_I6 || TARGET_CHIP_I6B0
               MIXER_ERR("i6/i6e don't support defog\n");
#endif
                break;
            }

        case 23://ae attr
            {
                struct _AeAttr tmp;
                memset(&tmp, 0x0, sizeof(tmp));
                if(ParamLength >  (MI_U32)(offset+3))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                if(NULL != buf[offset])
                    tmp.index = (MI_BOOL)atoi((char *)buf[offset]);
                else
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                offset += 1;
                if(NULL != buf[offset])
                {
                    tmp.y.luma = (MI_U32)atoi((char *)buf[offset]);
                    if(tmp.y.luma>2000)
                    {
                    tmp.y.luma=2000;
                    MIXER_ERR("luma range 0-2000,so if luma > 2000,luma=2000\n");
                    }
                    //MIXER_WARN("tmp.y.luma %d\n", tmp.y.luma);
                }
                offset += 1;
                if(NULL != buf[offset])
                {
                    tmp.y.mode = (MI_U32)atoi((char *)buf[offset]);
                    if(tmp.y.mode>3)
                    {
                    tmp.y.mode=3;
                    MIXER_ERR("mode range 1-3,so if mode > 3,mode=3;if mode < 1,mode=1\n");
                    }
                   if(tmp.y.mode<1)
                    {
                    tmp.y.mode=1;
                    MIXER_ERR("mode range 1-3,so if mode > 3,mode=3;if mode < 1,mode=1\n");
                    }
                    //MIXER_WARN("tmp.y.luma %d\n", tmp.y.luma);
                }
                IspTestAeAttr(&tmp);
                break;
            }

        case 25:
            {
                IspTestAE_EXPO_INFO();
                break;
            }


        case 27:
            {
                struct _NR3DAttr _3DNRattr;
                if(ParamLength <=  (MI_U32)(offset+2))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                _3DNRattr.u8TfStr = (MI_BOOL)atoi((char *)buf[offset]);
                offset += 1;

                _3DNRattr.u8TfStrEx = (MI_U32)atoi((char *)buf[offset]);
                offset += 1;

                _3DNRattr.u8TfLut = (MI_U32)atoi((char *)buf[offset]);
                offset += 1;

                IspTest3DNRAttr(&_3DNRattr);
                break;
            }

        case 28:
            {
                struct _AWBAttr tmp;
#if TARGET_CHIP_I5
                if(ParamLength <=  (MI_U32)(offset+3))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }

                tmp.AreaScale = (MI_BOOL)atoi((char *)buf[offset]);
                offset += 1;
#elif TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
                if(ParamLength <=  (MI_U32)(offset+2))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
#endif

                tmp.u16WhiteBgain = (MI_U32)atoi((char *)buf[offset]);
                offset += 1;

                tmp.u16WhiteRgain = (MI_U32)atoi((char *)buf[offset]);
                offset += 1;

                tmp.u8AreaSize = (MI_U32)atoi((char *)buf[offset]);
                offset += 1;

                IspTestAwbAttr(&tmp);

                break;
            }

        case 30:
            {
                IspTestAWB_QUERY_INFO();
                break;
            }

        case 32:
            {
                IspTestAfInfo();
                break;
            }

        case 33:
            {
                struct _nrDespike tmp;
                if(ParamLength <=  (MI_U32)(offset+1))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }

                tmp.DiffThdCornerCross = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;

                tmp.BlendRatio = (MI_U8)atoi((char *)buf[offset]);
                offset += 1;

                IspTestNRDESPIKE(&tmp);
                break;
            }

        case 34:
            {
                struct _nrLuma tmp;

#if TARGET_CHIP_I5
                if(ParamLength <=  (MI_U32)(offset+2))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }

                tmp.bEnLscReference = (MI_BOOL)atoi((char *)buf[offset]);
                offset += 1;

                tmp.u8FilterLevel = (MI_U8)atoi((char *)buf[offset]);
                offset += 1;

                tmp.u8BlendRatio = (MI_U8)atoi((char *)buf[offset]);
                offset += 1;


#elif TARGET_CHIP_I6
                if(ParamLength <= (offset))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }

                tmp.index = (MI_U8)atoi((char *)buf[offset]);
                offset += 1;

#elif TARGET_CHIP_I6E
                if(ParamLength <= (offset))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }

                tmp.index = (MI_U8)atoi((char *)buf[offset]);
                offset += 1;
#elif TARGET_CHIP_I6B0
                if(ParamLength <= (offset))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }

                tmp.index = (MI_U8)atoi((char *)buf[offset]);
                offset += 1;

#endif
                IspTestNrLuma(&tmp);
                break;
            }

        case 35:
            {
                struct _hsv tmp;

                if(ParamLength <=  (MI_U32)(offset+1))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }

                tmp.s16HueLut = (MI_S16)atoi((char *)buf[offset]);
                offset += 1;

                tmp.u16SatLut = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;

                IspTestHSV(&tmp);

                break;
            }

        case 36:
            {
                struct _rgbir tmp;

                if(ParamLength <=  (MI_U32)(offset+3))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }

                tmp.bRemovelEn = (MI_BOOL)atoi((char *)buf[offset]);
                offset += 1;

                tmp.Ratio_r = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;

                tmp.Ratio_g = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;

                tmp.Ratio_b = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;

                IspTestRGBIR(&tmp);

                break;
            }

        case 37:
            {
                struct _wdr tmp;

                if(ParamLength <=  (MI_U32)(offset+2))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }

                tmp.u8BrightLimit = (MI_BOOL)atoi((char *)buf[offset]);
                offset += 1;

                tmp.u8DarkLimit = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;

                tmp.u8Strength = (MI_U16)atoi((char *)buf[offset]);
                offset += 1;

                IspTestWdr(&tmp);

                break;
            }
        case 39:
            {
                struct _loadiqbin tmp;

                if(ParamLength <=  (MI_U32)(offset+1))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }

                tmp.filepath = (const char *)buf[offset];
                offset += 1;

                tmp.key = (MI_U32)atoi((char *)buf[offset]);
                offset += 1;


                IspTestLoadbin(&tmp);
                break;
            }
        case 42:
            {
                struct _loadcali tmp;

                if(ParamLength <=  (MI_U32)(offset+2))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }

                tmp.channel = (MI_U8)atoi((char *)buf[offset]);
                offset += 1;

                tmp.caliItem = (MI_U8)atoi((char *)buf[offset]);
                offset += 1;

                tmp.filepath = (const char *)buf[offset];
                offset += 1;
                IspTestLoadcaliData(&tmp);

                break;
            }
        case 43:
            {
                struct _ircut tIrcut;

                if(ParamLength <=  (MI_U32)(offset+1))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }

                tIrcut.boardtype = (MI_U8)atoi((char *)buf[offset]);
                offset += 1;

                tIrcut.isday = (MI_BOOL)atoi((char *)buf[offset]);
                offset += 1;

                IspTestIrcut(&tIrcut);
                break;
            }

        case 44:
            {
                struct _cus3a tCus3a;

                if(ParamLength <= (offset))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                tCus3a.cus3a_option = (MI_U8)atoi((char *)buf[offset]);
                offset += 1;

                if(1 == tCus3a.cus3a_option)
                {
                    if(ParamLength <=  (MI_U32)(offset+2))
                    {
                        MIXER_ERR("wrong param\n");
                        return;
                    }
                    tCus3a.tCus3aAttr.mAeEnable = (MI_BOOL)atoi((char *)buf[offset]);
                    offset += 1;
                    tCus3a.tCus3aAttr.mAwbEnable = (MI_BOOL)atoi((char *)buf[offset]);
                    offset += 1;
                    tCus3a.tCus3aAttr.mAfEnable = (MI_BOOL)atoi((char *)buf[offset]);
                    offset += 1;
                }

                IspTestCus3a(&tCus3a);
                break;
            }
#if TARGET_CHIP_I6 || TARGET_CHIP_I6B0
        case 45:
            {
                struct _ae_state tAeState;
                if(ParamLength <= (offset))
                {
                    MIXER_ERR("wrong param\n");
                    return;
                }
                tAeState.AeState= (MI_BOOL)!!atoi((char *)buf[offset]);
                offset += 1;

                IspTestAeSetState(&tAeState);
                break;
            }
#endif
        case 46: /*For CUS3A hacking only*/
            {
                int bEn = (MI_U8)atoi((char *)buf[offset]);
                if(bEn)
                {
                    printf("Enable userspace 3a\n");
                    MI_ISP_EnableUserspace3A(0,NULL);
                }
                else
                {
                    Cus3AEnable_t En = {0,0,0};
                    printf("Disable userspace 3a\n");
                    MI_ISP_DisableUserspace3A(0);
                    MI_ISP_CUS3A_Enable(0,&En);
                }
                break;
            }
        case 55: /*For CUS3A AF Motor Control*/
            {
            #if MOTOR_TEST
                af_init_done = (MI_U8)atoi((char *)buf[offset]);
            #endif
                break;
            }
        default:
            {
                MIXER_ERR("don't support this command\n");
            }
            break;
    }

}

unsigned int utimeGetTime(void)
{
    struct timespec tTime;
    clock_gettime(CLOCK_MONOTONIC, &tTime);
    return tTime.tv_sec * 1000000 + tTime.tv_nsec / 1000;
}

int isp_process_cmd(MixerCmdId id, MI_S8 *param, MI_U32 paramLen)
{
    MI_ISP_IQ_PARAM_INIT_INFO_TYPE_t stIspFlage;
    static int ready = 1;
    MI_U32 cpLen;

    while(ready)
    {
        MI_ISP_IQ_GetParaInitStatus(0, &stIspFlage);
        if(TRUE == (stIspFlage.stParaAPI.bFlag & 0xff))
        {
            ready = 0;
            break;
        }

        usleep(1000 * 10);
    }

    switch(id)
    {
        case CMD_ISP_OPEN:
            break;
        case CMD_ISP_SET_CONTRAST:
        case CMD_ISP_SET_BRIGHTNESS:
        case CMD_ISP_SET_SATURATION:
        case CMD_ISP_SET_SHARPENESS:
        case CMD_ISP_SET_LIGHTSENSITIVITY:
        case CMD_ISP_SET_SCENE:
        case CMD_ISP_SET_EXPOSURE_MODE:
        case CMD_ISP_SET_IRCUT:
        case CMD_ISP_SET_ROTATION:
            break;
        case CMD_ISP_SET_AUTO_WB:
        case CMD_ISP_SET_NIGHT_OPTIMIZE:
        case CMD_ISP_SET_NIGHT_SELF_ADAPTION:
        case CMD_ISP_SET_WIDE_DYNAMIC:
        case CMD_ISP_SET_DISPLAY_MIRROR:
        case CMD_ISP_SET_DISPLAY_REVERSAL:
        case CMD_ISP_SET_STAR:
        case CMD_ISP_SET_DEFOGGING:

        case CMD_ISP_GET_CONTRAST:
        case CMD_ISP_GET_BRIGHTNESS:
        case CMD_ISP_GET_SATURATION:
        case CMD_ISP_GET_SHARPENESS:
        case CMD_ISP_GET_LIGHTSENSITIVITY:
        case CMD_ISP_GET_SCENE:
        case CMD_ISP_GET_EXPOSURE_MODE:
        case CMD_ISP_GET_IRCUT:
        case CMD_ISP_GET_ROTATION:
        case CMD_ISP_GET_AUTO_WB:
        case CMD_ISP_GET_NIGHT_OPTIMIZE:
        case CMD_ISP_GET_NIGHT_SELF_ADAPTION:
        case CMD_ISP_GET_WIDE_DYNAMIC:
        case CMD_ISP_GET_DISPLAY_MIRROR:
        case CMD_ISP_GET_DISPLAY_REVERSAL:
        case CMD_ISP_GET_STAR:
        case CMD_ISP_GET_DEFOGGING:
            break;

        case CMD_ISP_LOAD_CALI_DATA:
            {
#if TARGET_CHIP_I5
                int ispCaliParam[3];
                int caliItem = 0;
                int bufsize = 128;
                char *filePath = NULL;

                cpLen = paramLen > sizeof(ispCaliParam) ? sizeof(ispCaliParam) : paramLen;
                memcpy(ispCaliParam, param, cpLen);
                filePath = (char*)ispCaliParam[0];
                caliItem = ispCaliParam[1];
                bufsize = ispCaliParam[2];

                for(MI_U32 i = 0; (i < SS_CALI_ITEM_MAX) && caliItem; i++)
                {
                    if(2 == i)
                    {
                        continue; //not support SS_CALI_ITEM_SDC
                    }

                    if(caliItem & (0x01 << i))
                    {
                        MI_ISP_API_CmdLoadCaliData(0, (MI_ISP_CALI_ITEM_e)i, (filePath + bufsize * i));
                        printf("MI_ISP_API_CmdLoadCaliData(i=%d) : caliItem = %s, filepath = %s\n", i,i == 0 ? "AWB" : i == 1 ? "OBC" :
                                i == 3 ? "ALSC" : i == 4 ? "LSC" : "Not Support", (filePath + bufsize * i));
                    }
                }
#elif TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
                int ispCaliParam[2];
                char *filePath;
                int caliItem;
                cpLen = paramLen > sizeof(ispCaliParam) ? sizeof(ispCaliParam) : paramLen;
                memcpy(ispCaliParam, param, cpLen);
                filePath = (char*)ispCaliParam[0];
                caliItem = ispCaliParam[1];
                if (access(filePath,0) == 0)
                {
                    printf("MI_ISP_API_CmdLoadCaliData : caliItem = %s, filepath = %s\n",
                        caliItem == 0 ? "AWB" :
                        caliItem == 1 ? "OBC" :
                        caliItem == 2 ? "SDC" :
                        caliItem == 3 ? "ALSC" :
                        caliItem == 4 ? "LSC" :
                        "Not Support", filePath);
                    MI_ISP_API_CmdLoadCaliData(0, (MI_ISP_CALI_ITEM_e)caliItem, filePath);
                }
#endif
            }
            break;

        case CMD_ISP_LOAD_CMDBIN:
            {
                int ispParam[2];
                char *filePath;
                int key = 0;
                cpLen = paramLen > sizeof(ispParam) ? sizeof(ispParam) : paramLen;
                memcpy(ispParam, param, cpLen);
                filePath = (char*)ispParam[0];
                key = ispParam[1];

                if (access(filePath,0) == 0)
                {
                    printf("MI_ISP_API_CmdLoadBinFile filepath = %s, key = %d\n", filePath, key);
                    MI_ISP_API_CmdLoadBinFile(0, filePath, key);
                }
            }
            break;
#if MIXER_CUS3A_ENABLE
        case CMD_ISP_OPEN_CUS3A:
            {
                ISP_AE_INTERFACE tAeIf;
                ISP_AWB_INTERFACE tAwbIf;
                ISP_AF_INTERFACE tAfIf;
                ISP_AE_INTERFACE* pAeIf = NULL;
                ISP_AWB_INTERFACE* pAwbIf = NULL;
                ISP_AF_INTERFACE* pAfIf = NULL;
                MI_S32 n3aispParam[3];
                MI_BOOL bAEenable = FALSE;
                MI_BOOL bAFenable = FALSE;
                MI_BOOL bAWBenable = FALSE;

                paramLen = paramLen > sizeof(n3aispParam) ? sizeof(n3aispParam) : paramLen;

                memset(n3aispParam, 0x00, sizeof(n3aispParam));
                cpLen = paramLen > sizeof(n3aispParam) ? sizeof(n3aispParam) : paramLen;
                memcpy(n3aispParam, param, cpLen);

                bAEenable = !!n3aispParam[0];
                bAFenable = !!n3aispParam[2];
                bAWBenable = !!n3aispParam[1];

                printf("== MIXER CUS3A ENABLE == , %d %d %d\n",bAEenable,bAWBenable,bAFenable);

                if(bAEenable || bAFenable || bAWBenable)
                {
                    printf("== MIXER CUS3A ENABLE ==\n");

                    if(!g_bCusAEenable)
                    {
                        CUS3A_Init();
                    }

                    if(bAEenable)
                    {
                        /*AE*/
                        tAeIf.ctrl = NULL;
                        tAeIf.pdata = NULL;
                        tAeIf.init = mod1_isp_ae_init;
                        tAeIf.release = mod1_isp_ae_release;
                        tAeIf.run = mod1_isp_ae_run;
                        pAeIf = &tAeIf;
                    }
                    if(bAWBenable)
                    {
                        /*AWB*/
                        tAwbIf.ctrl = NULL;
                        tAwbIf.pdata = NULL;
                        tAwbIf.init = mod1_isp_awb_init;
                        tAwbIf.release = mod1_isp_awb_release;
                        tAwbIf.run = mod1_isp_awb_run;
                        pAwbIf = &tAwbIf;
                    }
                    if(bAFenable)
                    {
                        /*AF*/
                        tAfIf.pdata = NULL;
                        tAfIf.init = mod1_isp_af_init;
                        tAfIf.release = mod1_isp_af_release;
                        tAfIf.run = mod1_isp_af_run;
                        tAfIf.ctrl = mod1_isp_af_ctrl;
                        pAfIf = &tAfIf;
                    }
                    CUS3A_RegInterface(0, pAeIf, pAwbIf, pAfIf);
                    g_bCusAEenable = TRUE;
                    printf(" enable cus3A !!\n");
                }
            }
            break;
        case CMD_ISP_CLOSE_CUS3A:
                {
                    if(g_bCusAEenable)
                    {
                        printf("== MIXER CUS3A DISABLE ==\n");
                        CUS3A_RegInterface(0,NULL,NULL,NULL);
                        CUS3A_Release();
                        g_bCusAEenable = FALSE;
                    }
                }
                break;
        case CMD_ISP_CUS3A_INTERFACE_UT:
                {
                    unsigned int nAeCnt = 0;
                    ISP_AE_INTERFACE tAeIf;
                    ISP_AWB_INTERFACE tAwbIf;
                    //ISP_AF_INTERFACE tAfIf;
                    ISP_AE_INTERFACE* pAeIf = NULL;
                    ISP_AWB_INTERFACE* pAwbIf = NULL;
                    //ISP_AF_INTERFACE* pAfIf = NULL;

                    printf("== MIXER CUS3A UT Start ==\n");

                    if(!g_bCusAEenable)
                    {
                        CUS3A_Init();
                    }

                    /*AE*/
                    tAeIf.ctrl = NULL;
                    tAeIf.pdata = &nAeCnt;
                    tAeIf.init = ut_isp_ae_init;
                    tAeIf.release = ut_isp_ae_release;
                    tAeIf.run = ut_isp_ae_run;
                    pAeIf = &tAeIf;

                    /*AWB*/
                    tAwbIf.ctrl = NULL;
                    tAwbIf.pdata = NULL;
                    tAwbIf.init = ut_isp_awb_init;
                    tAwbIf.release = ut_isp_awb_release;
                    tAwbIf.run = ut_isp_awb_run;
                    pAwbIf = &tAwbIf;

#if 0
                    /*AF*/
                    tAfIf.pdata = NULL;
                    tAfIf.init = NULL;
                    tAfIf.release = NULL;
                    tAfIf.run = NULL;
                    tAfIf.ctrl = NULL;
                    pAfIf = &tAfIf;
#endif
                    CUS3A_RegInterface(0, pAeIf, pAwbIf, NULL);

                    usleep(500*500);

                    CUS3A_RegInterface(0,NULL,NULL,NULL);
                    CUS3A_Release();
                    printf("== MIXER CUS3A UT End ==\n");
                }
                break;
#endif //MIXER_ENABLE_CUS3A

#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
        case CMD_ISP_GET_AESTATS:
                {

                }
                break;
        case CMD_ISP_SET_AEWINBLK:
                {
                    MS_CUST_AE_WIN_BLOCK_NUM_TYPE_e eBlkNum;
                    unsigned int channel = 0;
                    MI_S32 tParam[2]={0x0};
                    cpLen = paramLen > sizeof(tParam) ? sizeof(tParam) : paramLen;
                    memcpy(tParam, param, cpLen);

                    channel = tParam[0];
                    if(channel > 3)
                    {
                        MIXER_ERR("channel out of range.[0,3]. %d\n", channel);
                        return -1;
                    }

                    eBlkNum = (MS_CUST_AE_WIN_BLOCK_NUM_TYPE_e)tParam[1];

                    if(0 != MI_ISP_CUS3A_SetAEWindowBlockNumber(channel, &eBlkNum))
                    {
                        printf("SetAEWindowBlockNumber error\n");
                    }

                }
                break;
        case CMD_ISP_GET_AEHIST:
                {
                    MI_ISP_HISTO_HW_STATISTICS_t stHist;
                    unsigned int channel = 0, index = 0;
                    MI_S32 tParam[2]={0x0};
                    cpLen = paramLen > sizeof(tParam) ? sizeof(tParam) : paramLen;
                    memcpy(tParam, param, cpLen);
                    channel = tParam[0];
                    if(channel > 3)
                    {
                        MIXER_ERR("channel out of range.[0,3]. %d\n", channel);
                        return -1;
                    }
                    if(0 != MI_ISP_AE_GetHisto0HwStats(channel, &stHist))
                    {
                        printf("GetHisto0HwStats error\n");
                    }
                    else
                    {
                        printf("Show AeHwHist:\n");
                        for(index = 0; index < 128; ++index)
                        {
                            printf("%3d:%5d ", index, stHist.nHisto[index]);
                            if(index % 8 == 0)
                            {
                                printf("\n");
                            }
                        }
                    }
                }
                break;
        case CMD_ISP_GET_AWBSTATS:
                {
                    MI_ISP_AWB_HW_STATISTICS_t stStats;
                    MI_U32 channel = 0, indexX = 0, indexY = 0;
                    MI_S32 tParam[2]={0x0};
					cpLen = paramLen > sizeof(tParam) ? sizeof(tParam) : paramLen;
                    memcpy(tParam, param, cpLen);
                    channel = (MI_U32)tParam[0];
                    if(channel > 3)
                    {
                        MIXER_ERR("channel out of range.[0,3]. %d\n", channel);
                        return -1;
                    }

                    if(0 != MI_ISP_AWB_GetAwbHwAvgStats(channel, &stStats))
                    {
                        printf("GetAwbHwAvgStats error\n");
                    }
                    else
                    {
                        printf("Show AwbHwAvgStats:\n");
                        for(indexY = 0; indexY < stStats.nBlkY; ++indexY)
                        {
                            for(indexX = 0; indexX < stStats.nBlkX; ++indexX)
                            {
                                printf("(%d,%d,%d) ", stStats.nAvg[indexY * stStats.nBlkX + indexX].uAvgR,
                                        stStats.nAvg[indexY * stStats.nBlkX + indexX].uAvgG,
                                        stStats.nAvg[indexY * stStats.nBlkX + indexX].uAvgB);
                            }
                            printf("\n");
                        }
                    }
                }
                break;
        case CMD_ISP_SET_AWBSAMPLING:
                {
                    CusAWBSample_t stAwbS;
                    MI_S32 tParam[20]={0x0};
                    unsigned int channel = 0;
                    cpLen = paramLen > sizeof(tParam) ? sizeof(tParam) : paramLen;
                    memcpy(tParam, param, cpLen);
                    channel = (MI_U32)tParam[0];
                    if(channel > 3)
                    {
                        MIXER_ERR("channel out of range.[0,3]. %d\n", channel);
                        return -1;
                    }

                    memset(&stAwbS, 0x00, sizeof(stAwbS));
                    memcpy(&stAwbS, &tParam[1], sizeof(stAwbS));

                    if(0 != MI_ISP_CUS3A_SetAWBSampling(channel, &stAwbS))
                    {
                        printf("SetAWBSampling error\n");
                    }
                }
                break;
        case CMD_ISP_GET_AFSTATS:
                {
                    CusAFStats_t stStats;
                    unsigned int channel = 0, indexX = 0, indexY = 0;

                    if(paramLen >= sizeof(MI_S8))
                        channel = (unsigned int)param[0];
                    else
                        return -1;

                    if(channel > 3)
                    {
                        MIXER_ERR("channel out of range.[0,3]. %d\n", channel);
                        return -1;
                    }

                    if(0 != MI_ISP_CUS3A_GetAFStats(channel, &stStats))
                    {
                        printf("GetAFStats error\n");
                    }
                    else
                    {
                        printf("Show AFStats:\n");
                        for(indexX=0x0; indexX<sizeof(stStats.stParaAPI)/sizeof(stStats.stParaAPI[0]); ++indexX)
                        {
                            for(indexY=0x0; indexY<sizeof(stStats.stParaAPI[0]); ++indexY)
                            {
                                printf("%d ", *((MI_U8*) (&stStats.stParaAPI[indexX]) + indexY));
                                printf("%d ", *((MI_U8*) (&stStats.stParaAPI[indexX]) + indexY));
                                printf("%d ", *((MI_U8*) (&stStats.stParaAPI[indexX]) + indexY));
                                printf("%d ", *((MI_U8*) (&stStats.stParaAPI[indexX]) + indexY));
                                printf("%d ", *((MI_U8*) (&stStats.stParaAPI[indexX]) + indexY));

                                if(0x0 == indexY%16)
                                    printf("\n");
                            }
                        }
                    }
                }
                break;
        case CMD_ISP_GET_AFWIN:
                {
                    CusAFWin_t stAfWin[16];
                    unsigned int index = 0, channel = 0;

                    if(paramLen >= sizeof(MI_S8))
                        channel = (unsigned int)param[0];
                    else
                        return -1;

                    if(channel > 3)
                    {
                        MIXER_ERR("channel out of range.[0,3]. %d\n", channel);
                        return -1;
                    }

                    memset(stAfWin, 0x00, sizeof(stAfWin));

                    if(0 != MI_ISP_CUS3A_GetAFWindow(channel, stAfWin))
                    {
                        printf("GetAFWindow error\n");
                    }
                    else
                    {
                        for(index = 0; index < 16; ++index)
                        {
                            printf("win%d:%d,%d,%d,%d\n",index, stAfWin[index].stParaAPI.u32StartX,stAfWin[index].stParaAPI.u32EndX,
                                    stAfWin[index].stParaAPI.u32StartY,stAfWin[index].stParaAPI.u32EndY);
                        }
                    }
                }
                break;
        case CMD_ISP_SET_AFWIN:
                {
                    CusAFWin_t stAfWin[16];
                    unsigned int channel = 0;
                    MI_U32 tParam[100] = {0x0};
                    cpLen = paramLen > sizeof(tParam) ? sizeof(tParam) : paramLen;
                    memcpy(tParam, param, cpLen);
                    channel = (MI_U32)tParam[0];
                    if(channel > 3)
                    {
                        MIXER_ERR("channel out of range.[0,3]. %d\n", channel);
                        return -1;
                    }

                    memset(stAfWin, 0x00, sizeof(stAfWin));
                    memcpy(stAfWin, &tParam[1], sizeof(stAfWin));

                    if(0 != MI_ISP_CUS3A_GetAFWindow(channel, stAfWin))
                    {
                        printf("GetAFWindow error\n");
                    }
                }
                break;
        case CMD_ISP_GET_AFFILTER:
                {
                    CusAFFilter_t stAfFilter;
                    unsigned int channel = 0;

                    if(paramLen >= sizeof(MI_S8))
                        channel = (unsigned int)param[0];
                    else
                        return -1;

                    if(channel > 3)
                    {
                        MIXER_ERR("channel out of range.[0,3]. %d\n", channel);
                        return -1;
                    }

                    memset(&stAfFilter, 0x00, sizeof(stAfFilter));

                    if(0 != MI_ISP_CUS3A_SetAFFilter(channel, &stAfFilter))
                    {
                        printf("GetAFWindow error\n");
                    }
                    else
                    {
                        printf("IIR0:a0:%d,a1:%d,a2:%d,b1:%d,b2:%d\n", stAfFilter.u16IIR1_a0,stAfFilter.u16IIR1_a1,
                                stAfFilter.u16IIR1_a2,stAfFilter.u16IIR1_b1,stAfFilter.u16IIR1_b2);
                        printf("IIR1:a0:%d,a1:%d,a2:%d,b1:%d,b2:%d\n", stAfFilter.u16IIR2_a0,stAfFilter.u16IIR2_a1,
                                stAfFilter.u16IIR2_a2,stAfFilter.u16IIR2_b1,stAfFilter.u16IIR2_b2);
                    }
                }
                break;
        case CMD_ISP_SET_AFFILTER:
                {

                }
                break;
        case CMD_ISP_GET_AFFILTERSQ:
                {
                    CusAFFilterSq_t stAfFilterSQ;
                    unsigned int index = 0, channel = 0;

                    if(paramLen >= sizeof(MI_S8))
                        channel = (unsigned int)param[0];
                    else
                        return -1;

                    if(channel > 3)
                    {
                        MIXER_ERR("channel out of range.[0,3]. %d\n", channel);
                        return -1;
                    }

                    memset(&stAfFilterSQ, 0x00, sizeof(stAfFilterSQ));

                    if(0 != MI_ISP_CUS3A_GetAFFilterSq(channel, &stAfFilterSQ))
                    {
                        printf("GetAFFilterSq error\n");
                    }
                    else
                    {
                        printf("AF filter SQ:bSobelYSatEn:%d,u16SobelYThd:%d,bIIRSquareAccEn:%d,bSobelSquareAccEn:%d,"
                                "u16IIR1Thd:%d,u16IIR2Thd:%d,u16SobelHThd:%d,u16SobelVThd:%d\n",stAfFilterSQ.bSobelYSatEn,
                                stAfFilterSQ.u16SobelYThd,stAfFilterSQ.bIIRSquareAccEn,stAfFilterSQ.bSobelSquareAccEn,stAfFilterSQ.u16IIR1Thd,
                                stAfFilterSQ.u16IIR2Thd,stAfFilterSQ.u16SobelHThd,stAfFilterSQ.u16SobelVThd);
                        printf("AFtablX:");
                        for(index = 0; index < 12; ++index)
                        {
                            printf("%d ",stAfFilterSQ.u8AFTblX[index]);
                        }
                        printf("\n");
                        printf("AFtablY:");
                        for(index = 0; index < 13; ++index)
                        {
                            printf("%d ",stAfFilterSQ.u16AFTblY[index]);
                        }
                        printf("\n");
                    }
                }
                break;
        case CMD_ISP_SET_AFFILTERSQ:
                {

                }
                break;
        case CMD_ISP_GET_AFROIMODE:
                {
                    CusAFRoiMode_t stAFWinMode;
                    unsigned int channel = 0;

                    if(paramLen >= sizeof(MI_S8))
                        channel = (unsigned int)param[0];
                    else
                        return -1;

                    if(channel > 3)
                    {
                        MIXER_ERR("channel out of range.[0,3]. %d\n", channel);
                        return -1;
                    }

                    memset(&stAFWinMode, 0x00, sizeof(stAFWinMode));

                    if(0 != MI_ISP_CUS3A_GetAFRoiMode(channel, &stAFWinMode))
                    {
                        printf("GetAFRoiMode error\n");
                    }
                    else
                    {
                        printf("af win mode: %d, block number:%d\n",stAFWinMode.mode, stAFWinMode.u32_vertical_block_number);
                    }
                }
                break;
        case CMD_ISP_SET_AFROIMODE:
                {

                }
                break;
        case CMD_ISP_GET_IMAGERES:
                {

                }
                break;
        case CMD_ISP_GET_ISPIMAGE:
                {

                }
                break;
#if MIXER_CUS3A_ENABLE
        case CMD_ISP_TEST_CUS3A_API:
                {
                    FILE *fd;
                    MI_ISP_AE_HW_STATISTICS_t *pAe;
                    MI_ISP_AWB_HW_STATISTICS_t *pAwb;
                    MI_ISP_HISTO_HW_STATISTICS_t *pHisto;
                    CusAEInitParam_t *pAeInitParam;
                    CusAEInfo_t *pAeInfo;
                    CusAEResult_t *pAeResult;
                    CusAWBInfo_t *pAwbInfo;
                    CusAWBResult_t *pAwbResult;

                    //sync frame open
                    int fd0=0, fd1=0;
                    Cus3AOpenIspFrameSync(&fd0, &fd1);
                    printf("[CUS3A] FrameSync start: fd0(%x) fd1(%x)...\n", fd0, fd1);
                    unsigned int status = Cus3AWaitIspFrameSync(fd0, fd1, 500);
                    if(status != 0)
                    {

                        /*AE avg statistics*/
                        printf("========== TEST AE statistics ============\n");
                        pAe = (MI_ISP_AE_HW_STATISTICS_t *)malloc(sizeof(MI_ISP_AE_HW_STATISTICS_t));
                        if(!MI_ISP_AE_GetAeHwAvgStats(0,pAe))
                        {
                            fd = fopen("/customer/ae.bin","w+b");
                            fwrite(pAe->nAvg,sizeof(pAe->nAvg),1,fd);
                            fclose(fd);
                            printf("Save ae data to file ae.bin\n");
                            printf("AE statistics : PASS\n");
                        }
                        else
                            printf("AE statistics : FAIL\n");
                        free(pAe);

                        /*AWB avg statistics*/
                        printf("=========== TEST AWB statistics ============\n");
                        pAwb = (MI_ISP_AWB_HW_STATISTICS_t *)malloc(sizeof(MI_ISP_AWB_HW_STATISTICS_t));
                        if(!MI_ISP_AWB_GetAwbHwAvgStats(0,pAwb))
                        {
                            fd = fopen("/customer/awb.bin","w+b");
                            fwrite(pAwb->nAvg,sizeof(pAwb->nAvg),1,fd);
                            fclose(fd);
                            printf("Save awb data to file ae.bin\n");
                            printf("AWB statistics : PASS\n");
                        }
                        else
                            printf("AWB statistics : FAIL\n");
                        free(pAwb);

                        /*Histo0 avg statistics*/
                        printf("========== TEST Histogram0 ============\n");
                        pHisto = (MI_ISP_HISTO_HW_STATISTICS_t *)malloc(sizeof(MI_ISP_HISTO_HW_STATISTICS_t));
                        if(!MI_ISP_AE_GetHisto0HwStats(0,pHisto))
                        {
                            fd = fopen("/customer/histo0.bin","w+b");
                            fwrite(pHisto->nHisto,sizeof(pHisto->nHisto),1,fd);
                            fclose(fd);
                            printf("Save histo0 data to file ae.bin\n");
                            printf("Histogram0 statistics : PASS\n");
                        }
                        else
                            printf("Histogram0 statistics : FAIL\n");
                        free(pHisto);

                        /*Histo1 avg statistics*/
                        printf("========== TEST Histogram1 ============\n");
                        pHisto = (MI_ISP_HISTO_HW_STATISTICS_t *)malloc(sizeof(MI_ISP_HISTO_HW_STATISTICS_t));
                        if(!MI_ISP_AE_GetHisto1HwStats(0,pHisto))
                        {
                            fd = fopen("/customer/histo1.bin","w+b");
                            fwrite(pHisto->nHisto,sizeof(pHisto->nHisto),1,fd);
                            fclose(fd);
                            printf("Save histo1 data to file ae.bin\n");
                            printf("Histogram1 statistics : PASS\n");
                        }
                        else
                            printf("Histogram1 statistics : FAIL\n");
                        free(pHisto);

                        /*Check CUS3A*/
                        //pCus3AEnable = (Cus3AEnable_t *)malloc(sizeof(Cus3AEnable_t));
                        //pCus3AEnable->bAE = 1;
                        //pCus3AEnable->bAWB = 1;
                        //pCus3AEnable->bAF = 0;
                        //MI_ISP_CUS3A_Enable(0,pCus3AEnable);
                        //free(pCus3AEnable);

                        /*Check AE init param*/
                        printf("========== TEST AE init param ============\n");
                        pAeInitParam = (CusAEInitParam_t *)malloc(sizeof(CusAEInitParam_t));
                        memset(pAeInitParam,0,sizeof(CusAEInitParam_t));
                        if(!MI_ISP_CUS3A_GetAeInitStatus(0,pAeInitParam))
                        {
                            printf("AeInitParam:\n"
                                    " shutter=%d\n"
                                    " shutter_step=%d\n"
                                    " shutter_min=%d\n"
                                    " shutter_max=%d\n"
                                    " sensor_gain=%d\n"
                                    " sensor_gain_min=%d\n"
                                    " sensor_gain_max=%d\n"
                                    " isp_gain=%d\n"
                                    " isp_gain_max=%d\n"
                                    " FNx10=%d\n"
                                    " fps=%d\n",
                                    pAeInitParam->shutter,
                                    pAeInitParam->shutter_step,
                                    pAeInitParam->shutter_min,
                                    pAeInitParam->shutter_max,
                                    pAeInitParam->sensor_gain,
                                    pAeInitParam->sensor_gain_min,
                                    pAeInitParam->sensor_gain_max,
                                    pAeInitParam->isp_gain,
                                    pAeInitParam->isp_gain_max,
                                    pAeInitParam->FNx10,
                                    pAeInitParam->fps
                                  );
                            printf(" shutterHDRShort_step=%d\n"
                                    " shutterHDRShort_min=%d\n"
                                    " shutterHDRShort_max=%d\n"
                                    " sensor_gainHDRShort_min=%d\n"
                                    " sensor_gainHDRShort_max=%d\n"
                                    " AvgBlkX=%d\n"
                                    " AvgBlkY=%d\n",
                                    pAeInitParam->shutterHDRShort_step,
                                    pAeInitParam->shutterHDRShort_min,
                                    pAeInitParam->shutterHDRShort_max,
                                    pAeInitParam->sensor_gainHDRShort_min,
                                    pAeInitParam->sensor_gainHDRShort_max,
                                    pAeInitParam->AvgBlkX,
                                    pAeInitParam->AvgBlkY
                                    );
                        }
                        else
                            printf("AeInitParam : FAIL\n");
                        free(pAeInitParam);

                        /*Check AE param*/
                        printf("========== TEST AE get param ============\n");
                        pAeInfo = (CusAEInfo_t *)malloc(sizeof(CusAEInfo_t));
                        memset(pAeInfo,0,sizeof(CusAEInfo_t));
                        if(!MI_ISP_CUS3A_GetAeStatus(0,pAeInfo))
                        {
                            printf("AeParam:\n"
                                   " hist1=0x%X\n"
                                   " hist2=0x%X\n"
                                   " AvgBlkX=%d\n"
                                   " AvgBlkY=%d\n"
                                   " avgs=0x%X\n"
                                   " Shutter=%d\n"
                                   " SensorGain=%d\n"
                                   " IspGain=%d\n"
                                   " ShutterHdrShort=%d\n"
                                   " SensorGainHdrShort=%d\n"
                                   " IspGainHdrShort=%d\n"
                                   " PreAvgY=%d\n",
                                   (unsigned int)pAeInfo->hist1,
                                   (unsigned int)pAeInfo->hist2,
                                   pAeInfo->AvgBlkX,
                                   pAeInfo->AvgBlkY,
                                   (unsigned int)pAeInfo->avgs,
                                   pAeInfo->Shutter,
                                   pAeInfo->SensorGain,
                                   pAeInfo->IspGain,
                                   pAeInfo->ShutterHDRShort,
                                   pAeInfo->SensorGainHDRShort,
                                   pAeInfo->IspGainHDRShort,
                                   pAeInfo->PreAvgY
                            );
                            printf(" HDRCtlMode=%d\n"
                                   " FNx10=%d\n"
                                   " CurFPS=%d\n"
                                   " PreWeightY=%d\n",
                                   pAeInfo->HDRCtlMode,
                                   pAeInfo->FNx10,
                                   pAeInfo->CurFPS,
                                   pAeInfo->PreWeightY
                            );
                        }
                        else
                            printf("AeParam : FAIL\n");
                        free(pAeInfo);

#if 0
                        MI_U32 Size;           /**< struct size*/
                        MI_U32 AvgBlkX;
                        MI_U32 AvgBlkY;
                        MI_U32 CurRGain;
                        MI_U32 CurGGain;
                        MI_U32 CurBGain;
                        void *avgs; //ISP_AWB_SAMPLE
                        /*CUS3A V1.1*/
                        MI_U8  HDRMode;          /**< Noramal or HDR mode*/
                        void*  *pAwbStatisShort; /**< Short Shutter AWB statistic data */
                        MI_U32 u4BVx16384;      /**< From AE output, Bv * 16384 in APEX system, EV = Av + Tv = Sv + Bv */
                        MI_S32 WeightY;         /**< frame brightness with ROI weight*/
#endif

                        /*Check AWB param*/
                        printf("========== TEST AWB get param ============\n");
                        pAwbInfo = (CusAWBInfo_t *)malloc(sizeof(CusAWBInfo_t));
                        memset(pAwbInfo,0,sizeof(CusAWBInfo_t));
                        MI_ISP_CUS3A_GetAwbStatus(0,pAwbInfo);
                        printf("AwbParam:\n"
                                " AvgBlkX=%d\n"
                                " AvgBlkY=%d\n"
                                " Rgain=%d\n"
                                " Ggain=%d\n"
                                " Bgain=%d\n"
                                " avgs=0x%X\n",
                                pAwbInfo->AvgBlkX,
                                pAwbInfo->AvgBlkY,
                                pAwbInfo->CurRGain,
                                pAwbInfo->CurGGain,
                                pAwbInfo->CurBGain,
                                (unsigned int)pAwbInfo->avgs
                        );
                        printf( " HDRMode=%d\n"
                                " pAwbStatisShort=0x%X\n"
                                " u4BVx16384=%d\n"
                                " WeightY=%d\n",
                                pAwbInfo->HDRMode,
                                (unsigned int)pAwbInfo->pAwbStatisShort,
                                pAwbInfo->u4BVx16384,
                                pAwbInfo->WeightY
                        );
                        free(pAwbInfo);

                        /*Set AWB param*/
                        pAwbResult = (CusAWBResult_t *)malloc(sizeof(CusAWBResult_t));
                        memset(pAwbResult,0,sizeof(CusAWBResult_t));
                        pAwbResult->Size = sizeof(CusAWBResult_t);
                        pAwbResult->R_gain = 4096;
                        pAwbResult->G_gain = 1024;
                        pAwbResult->B_gain = 1024;
                        MI_ISP_CUS3A_SetAwbParam(0,pAwbResult);
                        free(pAwbResult);

#if 1
                        int isp_fe = open("/dev/isp_fe",O_RDWR);
                        int n =0;
                        unsigned int LasT = utimeGetTime();
                        unsigned int CurT = 0;
						/*Check open isp_fe*/
						if(isp_fe < (int)0 )
                        {
							MIXER_ERR("OPEN /dev/isp_fe Fail\n");
							printf("[CUS3A] Cus3AWaitIspFrameSync fail !!!\n!!! Cannot test CUS3A API, please try again...\n");
							Cus3ACloseIspFrameSync(fd0, fd1);
							break;

						}
                        /*Check AE init param*/
                        pAeResult = (CusAEResult_t *)malloc(sizeof(CusAEResult_t));
                        for(n=0;n<120;++n)
                        {
                            unsigned int status = poll_isp(isp_fe,500);
                            CurT = utimeGetTime();
                            printf("[%d]ISP CH%d FE, DT=%d us\n", n, status, CurT-LasT);
                            LasT = CurT;
                            memset(pAeResult,0,sizeof(CusAEResult_t));
                            pAeResult->Size = sizeof(CusAEResult_t);
                            pAeResult->Change = 1;
                            pAeResult->u4BVx16384 = 16384;
                            pAeResult->HdrRatio = 10;
                            pAeResult->ShutterHdrShort = 300;
                            pAeResult->Shutter = (300*n) % 30000;
                            pAeResult->IspGain = 2048;
                            pAeResult->SensorGain = 1024;
                            pAeResult->SensorGainHdrShort = 4096;
                            pAeResult->IspGainHdrShort = 1024;
                            pAeResult->AvgY = 128;
                            MI_ISP_CUS3A_SetAeParam(0,pAeResult);
                        }
                        free(pAeResult);
                        close(isp_fe);
#endif
                    }// status != 0
                    else
                    {
                        printf("[CUS3A] Cus3AWaitIspFrameSync fail !!!\n!!! Cannot test CUS3A API, please try again...\n");
                    }

                    Cus3ACloseIspFrameSync(fd0, fd1);
                    printf("[CUS3A] FrameSync finish...\n");
                }
                break;
#endif //MIXER_CUS3A_ENABLE
#endif
        default:
                break;
    }

    return 0;
}


