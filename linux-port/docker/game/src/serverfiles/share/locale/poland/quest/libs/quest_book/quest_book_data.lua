quest_book_data = {}

QUEST_SCROLL_TYPE_KILL_MOB = 0

local QUEST_DIFFICULTY_EASY      = 0
local QUEST_DIFFICULTY_MEDIUM    = 1
local QUEST_DIFFICULTY_HARD      = 2
local QUEST_DIFFICULTY_EXPERT    = 3

local QUEST_REWARD_TYPE_LOW1 = 0
local QUEST_REWARD_TYPE_LOW2 = 0
local QUEST_REWARD_TYPE_LOW3 = 0
local QUEST_REWARD_TYPE_MEDIUM1 = 1
local QUEST_REWARD_TYPE_MEDIUM2 = 1
local QUEST_REWARD_TYPE_MEDIUM3 = 1
local QUEST_REWARD_TYPE_HIGH1 = 2
local QUEST_REWARD_TYPE_HIGH2 = 2
local QUEST_REWARD_TYPE_HIGH3 = 2
local QUEST_REWARD_TYPE_EXPERT1 = 3
local QUEST_REWARD_TYPE_EXPERT2 = 3
local QUEST_REWARD_TYPE_EXPERT3 = 3

quest_book_data.QUEST_LIST_BY_DIFFICULTY = {
    [QUEST_DIFFICULTY_EASY] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
        11, 12, 13, 14,
    },

    [QUEST_DIFFICULTY_MEDIUM] = {
       51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
       61, 62, 63, 64, 65, 66, 67, 68,
    },

    [QUEST_DIFFICULTY_HARD] = {
        101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
        111, 112, 113, 114, 115, 116, 117, 118,
     },

     [QUEST_DIFFICULTY_EXPERT] = {
        151, 152, 153, 154, 155, 156, 157, 158, 159, 160,
        161, 162, 163
     },
}


quest_book_data.QUEST_LIST = {
    [1] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_1_hunt,
        desc = translate.quest_book.mission_1_text,
        what = translate.quest_book.mission_1_mis,
        rewardType = QUEST_REWARD_TYPE_LOW3,

        mobs = {8002, 2},
    },

    [2] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_2_hunt,
        desc = translate.quest_book.mission_2_text,
        what = translate.quest_book.mission_2_mis,
        rewardType = QUEST_REWARD_TYPE_LOW3,

        mobs = {8001, 2},
    },

    [3] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_3_hunt,
        desc = translate.quest_book.mission_3_text,
        what = translate.quest_book.mission_3_mis,
        rewardType = QUEST_REWARD_TYPE_LOW1,

        mobs = {107, 30},
    },

    [4] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_4_hunt,
        desc = translate.quest_book.mission_4_text,
        what = translate.quest_book.mission_4_mis,
        rewardType = QUEST_REWARD_TYPE_LOW1,

        mobs = {106, 35},
    },

    [5] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_5_hunt,
        desc = translate.quest_book.mission_5_text,
        what = translate.quest_book.mission_5_mis,
        rewardType = QUEST_REWARD_TYPE_LOW2,

        mobs = {108, 20},
    },

    [6] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_6_hunt,
        desc = translate.quest_book.mission_6_text,
        what = translate.quest_book.mission_6_mis,
        rewardType = QUEST_REWARD_TYPE_LOW1,

        mobs = {114, 25, 110, 30},
    },

    [7] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_7_hunt,
        desc = translate.quest_book.mission_7_text,
        what = translate.quest_book.mission_7_mis,
        rewardType = QUEST_REWARD_TYPE_LOW3,

        mobs = {8004, 2},
    },

    [8] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_8_hunt,
        desc = translate.quest_book.mission_8_text,
        what = translate.quest_book.mission_8_mis,
        rewardType = QUEST_REWARD_TYPE_LOW3,

        mobs = {8003, 2},
    },

    [9] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_9_hunt,
        desc = translate.quest_book.mission_9_text,
        what = translate.quest_book.mission_9_mis,
        rewardType = QUEST_REWARD_TYPE_LOW2,

        mobs = {109, 30},
    },

    [10] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_10_hunt,
        desc = translate.quest_book.mission_10_text,
        what = translate.quest_book.mission_10_mis,
        rewardType = QUEST_REWARD_TYPE_LOW1,

        mobs = {111, 25, 112, 25},
    },

    [11] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_11_hunt,
        desc = translate.quest_book.mission_11_text,
        what = translate.quest_book.mission_11_mis,
        rewardType = QUEST_REWARD_TYPE_LOW1,

        mobs = {104, 35, 105, 25},
    },

    [12] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.quest_lowlvl1.title,
        desc =  translate.quest_book.quest_lowlvl1.desc,
        what =  translate.quest_book.quest_lowlvl1.what,
        rewardType = QUEST_REWARD_TYPE_LOW1,

        mobs = {352, 15},
    },

    [13] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.quest_lowlvl2.title,
        desc =  translate.quest_book.quest_lowlvl2.desc,
        what =  translate.quest_book.quest_lowlvl2.what,
        rewardType = QUEST_REWARD_TYPE_LOW1,

        mobs = {115, 25},
    },

    [14] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.quest_lowlvl3.title,
        desc =  translate.quest_book.quest_lowlvl3.desc,
        what =  translate.quest_book.quest_lowlvl3.what,
        rewardType = QUEST_REWARD_TYPE_LOW1,

        mobs = {140, 25},
    },

    [51] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_12_hunt,
        desc = translate.quest_book.mission_12_text,
        what = translate.quest_book.mission_12_mis,
        rewardType = QUEST_REWARD_TYPE_MEDIUM1,

        mobs = {304, 20},
    },

    [52] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_13_hunt,
        desc = translate.quest_book.mission_13_text,
        what = translate.quest_book.mission_13_mis,
        rewardType = QUEST_REWARD_TYPE_MEDIUM2,

        mobs = {404, 25, 405, 25},
    },

    [53] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_14_hunt,
        desc = translate.quest_book.mission_14_text,
        what = translate.quest_book.mission_14_mis,
        rewardType = QUEST_REWARD_TYPE_MEDIUM1,

        mobs = {631, 30, 632, 30},
    },

    [54] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_15_hunt,
        desc = translate.quest_book.mission_15_text,
        what = translate.quest_book.mission_15_mis,
        rewardType = QUEST_REWARD_TYPE_MEDIUM1,

        mobs = {501, 30, 502, 30},
    },

    [55] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_16_hunt,
        desc = translate.quest_book.mission_16_text,
        what = translate.quest_book.mission_16_mis,
        rewardType = QUEST_REWARD_TYPE_MEDIUM3,

        mobs = {8006, 2},
    },

    [56] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_17_hunt,
        desc = translate.quest_book.mission_17_text,
        what = translate.quest_book.mission_17_mis,
        rewardType = QUEST_REWARD_TYPE_MEDIUM3,

        mobs = {8005, 2},
    },

    [57] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_18_hunt,
        desc = translate.quest_book.mission_18_text,
        what = translate.quest_book.mission_18_mis,
        rewardType = QUEST_REWARD_TYPE_MEDIUM2,

        mobs = {393, 10},
    },

    [58] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_19_hunt,
        desc = translate.quest_book.mission_19_text,
        what = translate.quest_book.mission_19_mis,
        rewardType = QUEST_REWARD_TYPE_MEDIUM1,

        mobs = {633, 20, 634, 15},
    },

    [59] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_20_hunt,
        desc = translate.quest_book.mission_20_text,
        what = translate.quest_book.mission_20_mis,
        rewardType = QUEST_REWARD_TYPE_MEDIUM2,

        mobs = {5111, 30, 5112, 30},
    },

    [60] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_21_hunt,
        desc = translate.quest_book.mission_21_text,
        what = translate.quest_book.mission_21_mis,
        rewardType = QUEST_REWARD_TYPE_MEDIUM1,

        mobs = {5101, 30, 5102, 30},
    },

    [61] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_22_hunt,
        desc = translate.quest_book.mission_22_text,
        what = translate.quest_book.mission_22_mis,
        rewardType = QUEST_REWARD_TYPE_MEDIUM3,

        mobs = {494, 1},
    },

    [61] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_23_hunt,
        desc = translate.quest_book.mission_23_text,
        what = translate.quest_book.mission_23_mis,
        rewardType = QUEST_REWARD_TYPE_MEDIUM3,

        mobs = {8007, 2},
    },

    [62] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_24_hunt,
        desc = translate.quest_book.mission_24_text,
        what = translate.quest_book.mission_24_mis,
        rewardType = QUEST_REWARD_TYPE_MEDIUM2,

        mobs = {5113, 15, 5114, 15},
    },

    [63] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_25_hunt,
        desc = translate.quest_book.mission_25_text,
        what = translate.quest_book.mission_25_mis,
        rewardType = QUEST_REWARD_TYPE_MEDIUM1,

        mobs = {5103, 15, 5104, 15},
    },

    [64] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_26_hunt,
        desc = translate.quest_book.mission_26_text,
        what = translate.quest_book.mission_26_mis,
        rewardType = QUEST_REWARD_TYPE_MEDIUM3,

        mobs = {8008, 2},
    },

    [65] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_27_hunt,
        desc = translate.quest_book.mission_27_text,
        what = translate.quest_book.mission_27_mis,
        rewardType = QUEST_REWARD_TYPE_MEDIUM1,

        mobs = {2001, 40},
    },

    [66] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_28_hunt,
        desc = translate.quest_book.mission_28_text,
        what = translate.quest_book.mission_28_mis,
        rewardType = QUEST_REWARD_TYPE_MEDIUM1,

        mobs = {2002, 40},
    },

    [67] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_29_hunt,
        desc = translate.quest_book.mission_29_text,
        what = translate.quest_book.mission_29_mis,
        rewardType = QUEST_REWARD_TYPE_MEDIUM1,

        mobs = {703, 20, 704, 25},
    },

    [68] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_30_hunt,
        desc = translate.quest_book.mission_30_text,
        what = translate.quest_book.mission_30_mis,
        rewardType = QUEST_REWARD_TYPE_MEDIUM1,

        mobs = {701, 30, 702, 30},
    },

    [101] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_31_hunt,
        desc = translate.quest_book.mission_31_text,
        what = translate.quest_book.mission_31_mis,
        rewardType = QUEST_REWARD_TYPE_HIGH1,

        mobs = {706, 40, 707, 40},
    },

    [102] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_32_hunt,
        desc = translate.quest_book.mission_32_text,
        what = translate.quest_book.mission_32_mis,
        rewardType = QUEST_REWARD_TYPE_HIGH3,

        mobs = {8009, 2},
    },

    [103] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_33_hunt,
        desc = translate.quest_book.mission_33_text,
        what = translate.quest_book.mission_33_mis,
        rewardType = QUEST_REWARD_TYPE_HIGH1,

        mobs = {2031, 35, 2032, 35},
    },

    [104] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_35_hunt,
        desc = translate.quest_book.mission_35_text,
        what = translate.quest_book.mission_35_mis,
        rewardType = QUEST_REWARD_TYPE_HIGH1,

        mobs = {2033, 35, 2034, 35},
    },

    [105] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_36_hunt,
        desc = translate.quest_book.mission_36_text,
        what = translate.quest_book.mission_36_mis,
        rewardType = QUEST_REWARD_TYPE_HIGH1,

        mobs = {2035, 40},
    },

    [106] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_37_hunt,
        desc = translate.quest_book.mission_37_text,
        what = translate.quest_book.mission_37_mis,
        rewardType = QUEST_REWARD_TYPE_HIGH3,

        mobs = {8010, 2},
    },

    [107] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_38_hunt,
        desc = translate.quest_book.mission_38_text,
        what = translate.quest_book.mission_38_mis,
        rewardType = QUEST_REWARD_TYPE_HIGH1,

        mobs = {2103, 30, 2104, 20},
    },

    [108] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_39_hunt,
        desc = translate.quest_book.mission_39_text,
        what = translate.quest_book.mission_39_mis,
        rewardType = QUEST_REWARD_TYPE_HIGH1,

        mobs = {1001, 50, 1002, 50},
    },

    [109] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_40_hunt,
        desc = translate.quest_book.mission_40_text,
        what = translate.quest_book.mission_40_mis,
        rewardType = QUEST_REWARD_TYPE_HIGH1,

        mobs = {1003, 30, 1004, 25},
    },

    [110] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_41_hunt,
        desc = translate.quest_book.mission_41_text,
        what = translate.quest_book.mission_41_mis,
        rewardType = QUEST_REWARD_TYPE_HIGH1,

        mobs = {5123, 15, 5124, 15},
    },

    [111] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_42_hunt,
        desc = translate.quest_book.mission_42_text,
        what = translate.quest_book.mission_42_mis,
        rewardType = QUEST_REWARD_TYPE_HIGH2,

        mobs = {1107, 25},
    },

    [112] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_43_hunt,
        desc = translate.quest_book.mission_43_text,
        what = translate.quest_book.mission_43_mis,
        rewardType = QUEST_REWARD_TYPE_HIGH1,

        mobs = {5121, 30, 5122, 30},
    },

    [113] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_44_hunt,
        desc = translate.quest_book.mission_44_text,
        what = translate.quest_book.mission_44_mis,
        rewardType = QUEST_REWARD_TYPE_HIGH2,

        mobs = {1305, 30},
    },

    [114] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_45_hunt,
        desc = translate.quest_book.mission_45_text,
        what = translate.quest_book.mission_45_mis,
        rewardType = QUEST_REWARD_TYPE_HIGH1,

        mobs = {1105, 35, 1106, 30},
    },

    [115] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_46_hunt,
        desc = translate.quest_book.mission_46_text,
        what = translate.quest_book.mission_46_mis,
        rewardType = QUEST_REWARD_TYPE_HIGH1,

        mobs = {1301, 45, 1302, 35},
    },

    [116] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_47_hunt,
        desc = translate.quest_book.mission_47_text,
        what = translate.quest_book.mission_47_mis,
        rewardType = QUEST_REWARD_TYPE_HIGH2,

        mobs = {1101, 45},
    },

    [117] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_48_hunt,
        desc = translate.quest_book.mission_48_text,
        what = translate.quest_book.mission_48_mis,
        rewardType = QUEST_REWARD_TYPE_HIGH1,

        mobs = {1063, 45},
    },

    [118] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_49_hunt,
        desc = translate.quest_book.mission_49_text,
        what = translate.quest_book.mission_49_mis,
        rewardType = QUEST_REWARD_TYPE_HIGH2,

        mobs = {1031, 60},
    },

    [151] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_50_hunt,
        desc = translate.quest_book.mission_50_text,
        what = translate.quest_book.mission_50_mis,
        rewardType = QUEST_REWARD_TYPE_EXPERT1,

        mobs = {2201, 45},
    },

    [152] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_51_hunt,
        desc = translate.quest_book.mission_51_text,
        what = translate.quest_book.mission_51_mis,
        rewardType = QUEST_REWARD_TYPE_EXPERT1,

        mobs = {2202, 45},
    },

    [153] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_52_hunt,
        desc = translate.quest_book.mission_52_text,
        what = translate.quest_book.mission_52_mis,
        rewardType = QUEST_REWARD_TYPE_EXPERT1,

        mobs = {2201, 60, 2203, 40},
    },

    [154] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_53_hunt,
        desc = translate.quest_book.mission_53_text,
        what = translate.quest_book.mission_53_mis,
        rewardType = QUEST_REWARD_TYPE_EXPERT1,

        mobs = {2204, 40},
    },

    [155] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_54_hunt,
        desc = translate.quest_book.mission_54_text,
        what = translate.quest_book.mission_54_mis,
        rewardType = QUEST_REWARD_TYPE_EXPERT1,

        mobs = {2205, 60},
    },

    [156] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_55_hunt,
        desc = translate.quest_book.mission_55_text,
        what = translate.quest_book.mission_55_mis,
        rewardType = QUEST_REWARD_TYPE_EXPERT1,

        mobs = {2301, 30},
    },

    [157] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.mission_56_hunt,
        desc = translate.quest_book.mission_56_text,
        what = translate.quest_book.mission_56_mis,
        rewardType = QUEST_REWARD_TYPE_EXPERT1,

        mobs = {2302, 40, 2303, 30},
    },

    [158] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.quest57.title,
        desc  = translate.quest_book.quest57.desc,
        what  = translate.quest_book.quest57.what,
        rewardType = QUEST_REWARD_TYPE_EXPERT3,

        mobs = {8024, 2},
    },

    [159] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.quest58.title,
        desc  = translate.quest_book.quest58.desc,
        what  = translate.quest_book.quest58.what,
        rewardType = QUEST_REWARD_TYPE_EXPERT3,

        mobs = {2381, 1},
    },

    [160] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.quest59.title,
        desc  = translate.quest_book.quest59.desc,
        what  = translate.quest_book.quest59.what,
        rewardType = QUEST_REWARD_TYPE_EXPERT1,

        mobs = {2313, 50},
    },

    [161] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.quest60.title,
        desc  = translate.quest_book.quest60.desc,
        what  = translate.quest_book.quest60.what,
        rewardType = QUEST_REWARD_TYPE_EXPERT1,

        mobs = {2315, 70},
    },

    [162] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.quest61.title,
        desc  = translate.quest_book.quest61.desc,
        what  = translate.quest_book.quest61.what,
        rewardType = QUEST_REWARD_TYPE_EXPERT2,

        mobs = {1602, 15},
    },

    [163] = {
        type = QUEST_SCROLL_TYPE_KILL_MOB,
        title = translate.quest_book.quest62.title,
        desc  = translate.quest_book.quest62.desc,
        what  = translate.quest_book.quest62.what,
        rewardType = QUEST_REWARD_TYPE_EXPERT1,

        mobs = {1067, 30, 1065, 25},
    },
}

quest_book_data.QUEST_REWARD = {
    exp = {
        [QUEST_REWARD_TYPE_LOW1] = {
            10000, 40,
            20000, 25,
            30000, 10,
            50000, 5,
            80000, 2,
        },

        [QUEST_REWARD_TYPE_MEDIUM1] = {
            50000, 40,
            80000, 25,
            12000, 10,
            170000, 5,
            230000, 2,
        },

        [QUEST_REWARD_TYPE_HIGH1] = {
            230000, 40,
            300000, 25,
            380000, 10,
            470000, 5,
            570000, 2,
        },

        [QUEST_REWARD_TYPE_EXPERT1] = {
            470000, 40,
            570000, 25,
            680000, 10,
            800000, 5,
            930000, 2,
        },
    },

    gold = {
        [QUEST_REWARD_TYPE_LOW1] = {
            5000, 40,
            10000, 25,
            20000, 10,
            50000, 5,
            100000, 2,
        },

        [QUEST_REWARD_TYPE_MEDIUM1] = {
            20000, 40,
            50000, 25,
            100000, 10,
            160000, 5,
            230000, 2,
        },

        [QUEST_REWARD_TYPE_HIGH1] = {
            100000, 40,
            160000, 25,
            230000, 10,
            310000, 5,
            400000, 2,
        },

        [QUEST_REWARD_TYPE_EXPERT1] = {
            230000, 40,
            310000, 25,
            400000, 10,
            500000, 5,
            610000, 2,
        },
    },

    special_group = {
        [QUEST_REWARD_TYPE_LOW1] = 50307,
        [QUEST_REWARD_TYPE_MEDIUM1] = 50308,
        [QUEST_REWARD_TYPE_HIGH1] = 50309,
        [QUEST_REWARD_TYPE_EXPERT1] = 50310,
    },
}

quest_book_data.QUEST_SCROLL_TYPE_KILL_MOB = QUEST_SCROLL_TYPE_KILL_MOB
quest_book_data.QUEST_DIFFICULTY_EASY      = QUEST_DIFFICULTY_EASY
quest_book_data.QUEST_DIFFICULTY_MEDIUM    = QUEST_DIFFICULTY_MEDIUM
quest_book_data.QUEST_DIFFICULTY_HARD      = QUEST_DIFFICULTY_HARD
quest_book_data.QUEST_DIFFICULTY_EXPERT    = QUEST_DIFFICULTY_EXPERT