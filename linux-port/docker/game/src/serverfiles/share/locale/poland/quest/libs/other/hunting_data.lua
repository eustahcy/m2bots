hunting_test = {}
hunting_test.show_mob_pos = function(lev)
    map_index = pc.get_map_index()

    if map_index <= 0 then
        test_chat(string.format(gameforge.levelup._210_chat, map_index))
        return
    end

    if map_index > table.getn(special.lvq_map) then
        test_chat(string.format(gameforge.levelup._220_chat, map_index..") > max("..table.getn(special.lvq_map)))
        return
    end

    lev_quest_list = special.lvq_map[map_index]
    if not lev_quest_list then
        test_chat(string.format(gameforge.levelup._230_chat, map_index))
        return
    end

    mob_pos_list = lev_quest_list[lev]
    if not mob_pos_list then
        test_chat(string.format(gameforge.levelup._230_chat, map_index..")][lev("..lev))
        return
    end

    table.foreachi(
    mob_pos_list, 
    function (n, mob_pos) 
        test_chat(string.format(gameforge.levelup._240_chat, mob_pos[1],mob_pos[2]))
        addmapsignal(mob_pos[1]*100, mob_pos[2]*100) 
    end
    )
    setmapcenterposition(200, 0)
end

HUNTING_REWARD_PRIMARY_ARMOR = {
	[0] = 11200,
	[1] = 11400,
	[2] = 11600,
	[3] = 11800,
}

HUNTING_REWARD_PRIMARY_HELMET = {
	[0] = 12200,
	[1] = 12340,
	[2] = 12480,
	[3] = 12620,
}

HUNTING_QUEST_DATA = {
	-- reqLvl, mobData={{mobVnum0, mobCount0}}, gold, exp, item={item1,count1...itemN,countN}
	{reqLvl = 2,	mobData={{171, 10}, {172, 5}},		gold=0,	xp=30,	item={"primary_armor", 1}},
	{reqLvl = 3,	mobData={{171, 20}, {172, 10}},		gold=0,	xp=25,	item={13000, 1}},
	{reqLvl = 4,	mobData={{172, 15}, {173, 5}},		gold=200,	xp=25,	item={"primary_helmet", 1}},
	{reqLvl = 5,	mobData={{173, 10}, {174, 10}},		gold=500,	xp=20,	item={27104, 1}},
	{reqLvl = 6,	mobData={{174, 20}, {178, 10}},		gold=1000,	xp=20,	item={27104, 2}},
	{reqLvl = 7,	mobData={{178, 10}, {175, 5}},		gold=1500,	xp=20,	item={27104, 2}},
	{reqLvl = 8,	mobData={{178, 20}, {175, 10}},		gold=2000,	xp=20,	item={27100, 2}},
	{reqLvl = 9,	mobData={{175, 15}, {179, 5}},		gold=2500,	xp=20,	item={27104, 2}},
	{reqLvl = 10,	mobData={{175, 20}, {179, 10}},		gold=3000,	xp=20,	item={27100, 2}},
		
	{reqLvl = 11,	mobData={{179, 10}, {180, 5}},		gold=4000,	xp=20,	item={27104, 2}},
	{reqLvl = 12,	mobData={{180, 15}, {176, 10}},		gold=4600,	xp=20,	item={27100, 2}},
	{reqLvl = 13,	mobData={{176, 20}, {181, 5}},		gold=5200,	xp=15,	item={27104, 2}},
	{reqLvl = 14,	mobData={{181, 15}, {177, 5}},		gold=5800,	xp=15,	item={27100, 2}},
	{reqLvl = 15,	mobData={{181, 20}, {177, 10}},		gold=6400,	xp=15,	item={27104, 2}},
	{reqLvl = 16,	mobData={{177, 15}, {184, 5}},		gold=7000,	xp=15,	item={27100, 2}},
	{reqLvl = 17,	mobData={{177, 20}, {184, 10}},		gold=9000,	xp=15,	item={27104, 2}},
	{reqLvl = 18,	mobData={{184, 20}, {182, 10}},		gold=10000,	xp=10,	item={27100, 2}},
	{reqLvl = 19,	mobData={{182, 20}, {183, 10}},		gold=10500,	xp=10,	item={27104, 2}},
	{reqLvl = 20,	mobData={{183, 20}, {352, 15}},		gold=11000,	xp=10,	item={27100, 2}},
		
	{reqLvl = 21,	mobData={{352, 20}, {185, 10}},		gold=11500,	xp=7,	item={27104, 2}},
	{reqLvl = 22,	mobData={{185, 25}, {303, 10}},		gold=12000,	xp=7,	item={27101, 2}},
	{reqLvl = 23,	mobData={{303, 20}, {401, 40}},		gold=12500,	xp=7,	item={27104, 2}},
	{reqLvl = 24,	mobData={{401, 60}, {402, 80}},		gold=13000,	xp=7,	item={27101, 2}},
	{reqLvl = 25,	mobData={{551, 80}, {454, 20}},		gold=13500,	xp=7,	item={27104, 2}},
	{reqLvl = 26,	mobData={{552, 80}, {456, 20}},		gold=14000,	xp=7,	item={27101, 2}},
	{reqLvl = 27,	mobData={{456, 30}, {554, 20}},		gold=14500,	xp=7,	item={27104, 2}},
	{reqLvl = 28,	mobData={{651, 20}, {554, 30}},		gold=15000,	xp=7,	item={27101, 2}},
	{reqLvl = 29,	mobData={{651, 40}, {652, 30}},		gold=15500,	xp=7,	item={27104, 2}},
	{reqLvl = 30,	mobData={{652, 40}, {2102, 30}},	gold=20000,	xp=7,	item={27101, 2}},
	
	{reqLvl = 31,	mobData={{652, 50}, {2102, 45}},	gold=21000,	xp=5,	item={27104, 2}},
	{reqLvl = 32,	mobData={{653, 50}, {2051, 40}},	gold=22000,	xp=5,	item={27101, 2}},
	{reqLvl = 33,	mobData={{751, 35}, {2103, 30}},	gold=23000,	xp=5,	item={27104, 2}},
	{reqLvl = 34,	mobData={{751, 40}, {2103, 40}},	gold=24000,	xp=5,	item={27101, 2}},
	{reqLvl = 35,	mobData={{752, 40}, {2052, 40}},	gold=25000,	xp=5,	item={27104, 2}},
	{reqLvl = 36,	mobData={{754, 20}, {2106, 20}},	gold=26000,	xp=5,	item={27101, 2}},
	{reqLvl = 37,	mobData={{773, 30}, {2003, 20}},	gold=27000,	xp=5,	item={27104, 2}},
	{reqLvl = 38,	mobData={{774, 40}, {2004, 20}},	gold=28000,	xp=5,	item={27104, 2}},
	{reqLvl = 39,	mobData={{756, 40}, {2005, 30}},	gold=29000,	xp=5,	item={27101, 2}},
	{reqLvl = 40,	mobData={{757, 40}, {2158, 20}},	gold=30000,	xp=5,	item={27104, 2}},
	
	{reqLvl = 41,	mobData={{931, 40}, {5123, 25}},	gold=31000,	xp=5,	item={27102, 2}},
	{reqLvl = 42,	mobData={{932, 30}, {5123, 30}},	gold=32000,	xp=5,	item={27105, 2}},
	{reqLvl = 43,	mobData={{932, 40}, {2031, 35}},	gold=33000,	xp=5,	item={27102, 2}},
	{reqLvl = 44,	mobData={{933, 40}, {2031, 40}},	gold=34000,	xp=5,	item={27105, 2}},
	{reqLvl = 45,	mobData={{771, 50}, {2032, 45}},	gold=35000,	xp=5,	item={27102, 2}},
	{reqLvl = 46,	mobData={{772, 35}, {5124, 30}},	gold=36000,	xp=5,	item={27105, 2}},
	{reqLvl = 47,	mobData={{933, 35}, {5125, 35}},	gold=37000,	xp=5,	item={27102, 2}},
	{reqLvl = 48,	mobData={{934, 40}, {5125, 35}},	gold=38000,	xp=5,	item={27105, 2}},
	{reqLvl = 49,	mobData={{773, 40}, {2033, 45}},	gold=39000,	xp=5,	item={27102, 2}},
	{reqLvl = 50,	mobData={{774, 40}, {5126, 30}},	gold=40000,	xp=5,	item={27105, 2}},
	
	{reqLvl = 51,	mobData={{5126, 30}, {775, 50}},	gold=41000,	xp=5,	item={27102, 2}},
	{reqLvl = 52,	mobData={{2034, 45}, {934, 45}},	gold=41000,	xp=5,	item={27105, 2}},
	{reqLvl = 53,	mobData={{2034, 50}, {934, 50}},	gold=42000,	xp=5,	item={27102, 2}},
	{reqLvl = 54,	mobData={{1001, 30}, {776, 40}},	gold=42000,	xp=5,	item={27105, 2}},
	{reqLvl = 55,	mobData={{1301, 45}, {777, 40}},	gold=43000,	xp=5,	item={27102, 2}},
	{reqLvl = 56,	mobData={{1002, 30}, {935, 50}},	gold=43000,	xp=5,	item={27105, 2}},
	{reqLvl = 57,	mobData={{1002, 40}, {936, 60}},	gold=44000,	xp=5,	item={27102, 2}},
	{reqLvl = 58,	mobData={{1303, 40}, {936, 45}},	gold=44000,	xp=5,	item={27105, 2}},
	{reqLvl = 59,	mobData={{1303, 50}, {936, 45}},	gold=45000,	xp=5,	item={27102, 2}},
	{reqLvl = 60,	mobData={{1003, 40}, {937, 45}},	gold=45000,	xp=5,	item={27105, 2}},
	
	{reqLvl = 61,	mobData={{1004, 50}, {2061, 60}},	gold=46000,	xp=4,	item={27102, 2}},
	{reqLvl = 62,	mobData={{1305, 45}, {2131, 55}},	gold=46000,	xp=4,	item={27102, 2}},
	{reqLvl = 63,	mobData={{1305, 50}, {1101, 45}},	gold=47000,	xp=4,	item={27102, 2}},
	{reqLvl = 64,	mobData={{2062, 50}, {1102, 45}},	gold=47000,	xp=4,	item={27102, 2}},
	{reqLvl = 65,	mobData={{1104, 40}, {2063, 40}},	gold=48000,	xp=4,	item={27102, 2}},
	{reqLvl = 66,	mobData={{2301, 50}, {1105, 45}},	gold=48000,	xp=4,	item={27102, 2}},
	{reqLvl = 67,	mobData={{2301, 55}, {1105, 50}},	gold=49000,	xp=4,	item={27102, 2}},
	{reqLvl = 68,	mobData={{1106, 50}, {1061, 50}},	gold=49000,	xp=4,	item={27102, 2}},
	{reqLvl = 69,	mobData={{1107, 45}, {1031, 40}},	gold=50000,	xp=4,	item={27102, 2}},
	{reqLvl = 70,	mobData={{2302, 55}, {2201, 55}},	gold=50000,	xp=4,	item={27102, 2}},
	
	{reqLvl = 71,	mobData={{2303, 55}, {2202, 55}},	gold=51000,	xp=3,	item={27102, 3}},
	{reqLvl = 72,	mobData={{2303, 60}, {2202, 60}},	gold=51000,	xp=3,	item={27102, 3}},
	{reqLvl = 73,	mobData={{2304, 55}, {1033, 55}},	gold=52000,	xp=3,	item={27102, 3}},
	{reqLvl = 74,	mobData={{2305, 50}, {1033, 55}},	gold=52000,	xp=3,	item={27102, 3}},
	{reqLvl = 75,	mobData={{2204, 50}, {1034, 50}},	gold=53000,	xp=3,	item={27102, 3}},
	{reqLvl = 76,	mobData={{2205, 45}, {1035, 50}},	gold=53000,	xp=3,	item={27102, 3}},
	{reqLvl = 77,	mobData={{2311, 50}, {1068, 50}},	gold=54000,	xp=3,	item={27102, 3}},
	{reqLvl = 78,	mobData={{1070, 50}, {1066, 55}},	gold=54000,	xp=3,	item={27102, 3}},
	{reqLvl = 79,	mobData={{1069, 50}, {1070, 50}},	gold=55000,	xp=3,	item={27102, 3}},
	{reqLvl = 80,	mobData={{1071, 50}, {2312, 55}},	gold=55000,	xp=3,	item={27102, 3}},
	
	-- {reqLvl = 81,	mobData={{1041, 55}, {2312, 50}},	gold=60000,	xp=2,	item={27102, 3}},
	-- {reqLvl = 82,	mobData={{2313, 55}, {2314, 45}},	gold=61000,	xp=2,	item={27102, 3}},
	-- {reqLvl = 83,	mobData={{2313, 55}, {2314, 45}},	gold=62000,	xp=2,	item={27102, 3}},
	-- {reqLvl = 84,	mobData={{1131, 45}, {2315, 45}},	gold=63000,	xp=2,	item={27102, 3}},
	-- {reqLvl = 85,	mobData={{1133, 60}, {2315, 45}},	gold=64000,	xp=2,	item={27102, 3}},
	-- {reqLvl = 86,	mobData={{1132, 60}, {1135, 50}},	gold=65000,	xp=2,	item={27102, 3}},
	-- {reqLvl = 87,	mobData={{1132, 60}, {1135, 50}},	gold=66000,	xp=2,	item={27102, 3}},
	-- {reqLvl = 88,	mobData={{1134, 60}, {1136, 60}},	gold=67000,	xp=2,	item={27102, 3}},
	-- {reqLvl = 89,	mobData={{1133, 60}, {1137, 50}},	gold=68000,	xp=2,	item={27102, 3}},
	-- {reqLvl = 90,	mobData={{1132, 60}, {1137, 50}},	gold=69000,	xp=2,	item={27102, 3}},
	
	{reqLvl = 999},
}