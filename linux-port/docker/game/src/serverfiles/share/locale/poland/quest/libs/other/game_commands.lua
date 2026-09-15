game_commands = {}

local function refresh()
	local x = pc.get_x()
	local y = pc.get_y()
	pc.warp(x*100, y*100)
end

local function set_socket(args)
	local socket = tonumber(args[1])
	local value = tonumber(args[2])
	if item.select_cell(0) then
		item.set_socket(socket, value)
	end
end

local function item_setattr()
	if table.getn(mes.get_arguments()) < 3 or mes.get_arguments()[1] == "help" then
		syschat("wrong arguments")
		return
	end
	if not item.select_cell(0) then
		return
	end
	
	if item.get_vnum() == nil or item.get_vnum() <= 0 then
		syschat("Error occured.")
		return
	end
	local attrindex = mes.get_arguments()[1]
	local attrtype = mes.get_arguments()[2]
	local attrvalue = mes.get_arguments()[3]
	item.set_value(attrindex,attrtype,attrvalue)
end

local function kill_map()
	if pc.in_dungeon() then
		syschat("You can't use this command in dungeon!")
		return
	end
	syschat("Everyone was killed.")
	kill_all_in_map(pc.get_map_index())
end

local function xmas_tree()
	local val = mes.get_arguments()[1]
	game.set_event_flag("xmas_tree", val)
	cmdchat("xmas_tree "..val)
	syschat("redi!")
end

local function simulate_fish()
	local args = mes.get_arguments()
	local length = table.getn(args)
	local count = tonumber(args[1])
	if length == 1 then
		fishing_manage.simulate_fishing(count)
		return
	end

	local chance_to_fail = tonumber(args[2])
	if length == 2 then
		fishing_manage.simulate_fishing(count, chance_to_fail)
		return
	end
	
	local fish_chance = tonumber(args[3])
	if length == 3 then
		fishing_manage.simulate_fishing(count, chance_to_fail, fish_chance)
		return
	end
end

local function simulate_calm()
	local args = mes.get_arguments()
	local count = tonumber(args[1])
	for i=1, count do
		fishing_manage.simulate_calm()
	end
end

local function get_refine_recipe()
	local args = mes.get_arguments()
	local vnum = tonumber(args[1])
	local test = {item.get_refine_recipe(vnum)}
	for i,v in pairs(test) do
		debug(v)
	end
end

local function set_quest_state_cmd()
	if !is_test_server() then
		syschat("command available only in test mode")
		return
	end
	local args = mes.get_arguments()
	local questName = args[1]
	local questState = args[2]
	set_quest_state(questName, questState)
	syschat("przeloguj sie np. /refresh")
end

local function set_player_empire()
	local args = mes.get_arguments()
	local target_name = args[1]
	local empire = tonumber(args[2])
	local targetVID = find_pc_by_name(target_name)
	if targetVID == 0 then
		syschat(string.format("Nie znaleziono gracza \"%s\"! ", target_name))
	else
		local empires = {gameforge.change_empire._190_select, gameforge.change_empire._200_select, gameforge.change_empire._205_select, gameforge.locale.cancel}
		if !empire then
			say(string.format("Do kt�rego kr�lestwa chcesz przenie�� gracza \"%s\"? ", target_name))
			empire = select_table(empires)
		end
		if empire == 4 then
			return false
		end
		local oldVID = pc.get_vid()
		
		pc.select(targetVID)
		local oldempire = pc.get_empire()
		local errer_message = ""
	   	if pc.has_guild() or pc.is_engaged() or pc.is_married() or pc.is_polymorphed() then
			errer_message = string.format("Gracz %s jest w gildii, zar�czony, w zwi��ku ma��e�skim albo u�ywa polimorfii ", target_name)
		elseif oldempire == empire then
			errer_message = string.format("Gracz \"%s\" nale�y ju� do %s ", target_name, empires[empire])
		else
			local ret = pc.change_empire(empire)
			debug("pc.change_empire ret %d", ret)
			if ret == 999 then
				chat(string.format("Twoje kr�lestwo zosta�o zmienione na %s ", empires[empire]))
				chat("Zaloguj si� ponownie aby doko�czy� proces zmiany kr�lestwa! ")
				char_log( 0, "CHANGE_EMPIRE", string.format(gameforge.change_empire._240_stringFormat, oldempire , empire)) 
			end
		end
		pc.select(oldVID)
		if errer_message == "" then
			chat(string.format("Gracz \"%s\" pczeniesiony do %s ", target_name, empires[empire]))
		else
			syschat(errer_message)
		end
	end
end

local function set_happy_hour()
	local args = mes.get_arguments()
	local argLen = table.getn(args)

	if argLen < 1 then
		syschat("wrong arguments")
		return
	end

	local value = args[1]

	if value == "end" or value == "stop" then
		game.set_event_flag("happy_hour_value", 0)
		game.set_event_flag("happy_hour_end_time", 0)
		syschat("Wylaczono Happy Hour.")
		return
	end

	if argLen < 3 then
		syschat("wrong arguments (missing value, date dd/mm/yyyy HH:MM)")
		return
	end

	local date_str = args[2]
	local time_str = args[3]

	if tonumber(value) < 1 or tonumber(value) > 50 then
		syschat("Mozna podac tylko wartosci od 1 do 50")
	end

	local end_time = dateStringtoUnixTimestamp(date_str, time_str)
	if get_time() > end_time then
		syschat("podano zly czas (musi byc dd/mm/yyyy HH:MM)")
		return
	end

	syschat(string.format("Ustawiono Happy Hour %s%% do %d", value, end_time))
	game.set_event_flag("happy_hour_value", tonumber(value))
	game.set_event_flag("happy_hour_end_time", end_time)
end

local function clear_affect(args)
	local args_len = table.getn(args)
	local affect_type = 0
	local point_type = 0
	if args_len == 1 then
		affect_type = tonumber(args[1])
	elseif args_len == 2 then
		affect_type = tonumber(args[1])
		point_type = tonumber(args[2])
	else
		syschat("clear_affect command: wrong parameters (aff_type, optional: point_type)")
		return
	end
	affect.remove_new(affect_type, point_type)
	syschat(msg("clear affect type %d point %d", affect_type, point_type))
end


local function test_mob_spawn(args)
	local vnum = tonumber(args[1])
	local victimName = args[2]

	local victimVID = find_pc_by_name(victimName)
	if victimVID < 1 then
		debug("no player with name %s found", victimName)
		return
	end

	for i=1, 5 do
		local random_pos = number(1,100) / 100

		-- local spawnPos = {x=710, y=594}
		-- local mapX, mapY = map_local_to_global_position(pc.get_map_index(), spawnPos.x, spawnPos.y)

		local x, y = getPositionOnCircle(pc.get_x(), pc.get_y(), 10, random_pos)
		local vid = spawn_mob(vnum, x*100, y*100, 0, victimVID)
		if vid > 0 then
			npc.set_custom_npc_flag(vid, CUSTOM_NPC_FLAG_PURGE_ON_AGGRO_LOSE)
			debug("spawn mob with vid %d", vid)
		else
			debug("spawn mob ERROR")
		end
	end
end

local function test_goto(args)
	local vnum = tonumber(args[1])
	local x = tonumber(args[2])
	local y = tonumber(args[3])

	local vid = spawn_mob(vnum, pc.get_x()*100, pc.get_y()*100, 0, false)
	if vid > 0 then
		npc.select(vid)
		debug("hp: %d/%d", npc.get_point(vid, POINT_HP), npc.get_point(vid, POINT_MAX_HP))
		npc.point_change(vid, POINT_MOV_SPEED, -60)
		npc.point_change(vid, POINT_MAX_HP, 10000)
		npc.point_change(vid, POINT_HP, 10000)
		npc.point_change(vid, POINT_ATT_GRADE_BONUS, 10000)
		debug("hp: %d/%d", npc.get_point(vid, POINT_HP), npc.get_point(vid, POINT_MAX_HP))
		npc.go_to(x, y)
		debug("idziemy")
	else
		debug("moba nie bedzie")
	end
end

local function give_special(args)
	local args_len = table.getn(args)
	if args_len < 1 then
		return
	end

	local vnum = tonumber(args[1])
	local count = 1
	if args_len > 1 then
		count = tonumber(args[2])
	end

	local is_unqiue = args_len > 2

	if not is_unqiue then
		for i=1,count do
			pc.give_item_from_special_item_group(vnum)
		end
	else
		local drop_items = get_special_items(vnum, count, true)
		for i,v in pairs(drop_items) do
			pc.give_item2(v.vnum, v.count)
		end
	end
end

local function has_affect(args)
	local r = pc.has_affect(tonumber(args[1]))
	syschat(tostring(r))
end

local function knockback(args)
	syschat("knockback")
	pc.knockback(tonumber(args[1]))
	affect.add_new(AFFECT_STUN, POINT_NONE, 0, 3, AFF_STUN, true)
end

local function fnotice(args)
	local for_level = tonumber(args[1])
	local select_index = tonumber(args[2])

	setskin(SCROLL)
    setbgimage('level_bg.tga')
    say("")
    say("")
    say("")

	local data =  HUNTING_QUEST_DATA[for_level]
	local mob_vnum = data.mobData[select_index][1]
	local mob_count = data.mobData[select_index][2]

	if special.levelup_img[mob_vnum] == nil then
		debug("nie ma foty %d", mob_vnum)
	end
    addimage(20,12,special.levelup_img[mob_vnum])
    setcolor(0,0,0)

    say(string.format(translate.hunting.special_letter_kill, mob_count, mob_name(mob_vnum)))
    say(string.format(translate.hunting.special_letter_left, 8))
	say()
	-- say(translate.hunting.special_letter_reward_title)
    say(string.format(translate.hunting.special_letter_reward_exp, data.xp, 1))
	if data.gold > 0 then
    	say(string.format(translate.hunting.special_letter_reward_gold, parse_number(data.gold)))
	end
    say(string.format(translate.hunting.special_letter_reward_item, item_name(13000)))
	-- hunting_test.show_mob_pos(for_level)
end

local function test_effect(args)
	pc.show_effect(tonumber(args[1]))
end

local function affect_test(args)
	local aff = tonumber(args[1])
	local is_override = table.getn(args) > 2
	affect.add_new(aff, POINT_NONE, 0, INFINITE_AFFECT_DURATION, AFF_NONE, true)
	syschat("nadalem")
end

local function test_recipe(args)
	local recipe_items = {get_crafting_recipe(tonumber(args[1]))}
	for j in range(1, table.getn(recipe_items), 2) do
		local v = recipe_items[j]
		local count = recipe_items[j+1]
		for j,k in pairs(count) do
			debug("%d", k)
		end
	end
end

local function cinematic_mode()
	cmdchat("ToggleCameraMode")
end

local function give_quest_reward(args)
	if table.getn(args) < 1 then
		return
	elseif table.getn(args) == 1 then
		give_reward(args[1], 2)
	elseif table.getn(args) == 2 then
		local args = mes.get_arguments()
		local target_name = args[1]
		local quest_name = args[2]
		local targetVID = find_pc_by_name(target_name)
		if targetVID == 0 then
			syschat(string.format("Nie znaleziono gracza \"%s\"! ", target_name))
		else
			local oldVID = pc.get_vid()
			pc.select(targetVID)
			local target_pid = pc.get_player_id()
			give_reward(quest_name, 2)
			pc.select(oldVID)
			char_log(target_pid, "GM_QUEST_REWARD", quest_name)
		end
	end
end

local function special_shop_seed(args)
	if table.getn(args) < 1 then
		syschat("usage: /special_shop_seed shop_vnum")
		return
	end
	if tonumber(args[1]) then
		local shop_vnum = tonumber(args[1])
		special_shop_set_seed(shop_vnum, number(1,99999))
	end
end

local function stop_shutdown()
	stop_maintenance()
end

local function delay_shutdown(args)
	if table.getn(args) < 2 then
		syschat("usage: /delay_shutdown <hour>:<minute> <duration_in_minutes>")
		return
	end

	local time = split(args[1], ":")
	local hour = tonumber(time[1])
	local minute = tonumber(time[2])

	if !hour or !minute then
		syschat("wrong syntax!")
		return
	end

	local time_until_shutdown = time_until_hour(hour, minute)

	if !tonumber(args[2]) then
		syschat("wrong syntax!")
		return
	end
	local duration_in_seconds = tonumber(args[2]) * 60
	debug("%d dur: %d", time_until_shutdown, duration_in_seconds)
	start_maintenance(time_until_shutdown, duration_in_seconds)
end

local function server_version()
	syschat(msg("Current server version: %d", get_server_version()))
end

local function test_priv()
	__give_char_priv(2, 10)
end

local function command_list()
	-- command,	command_function	gm_level
	return {
		{"item_socket",					set_socket,					3},
		{"item_setattr",				item_setattr, 				3},
		{"kill_map",					kill_map, 					2},
		-- commands tutor
		{"refresh",						refresh,					1},
		{"clear_affect",					clear_affect,					3},
		{"xmas_tree",						xmas_tree,					3},
		{"simulate_fish",						simulate_fish,					3},
		{"get_refine_recipe",						get_refine_recipe,					3},
		{"simulate_calm",						simulate_calm,					3},
		{"set_quest_state",						set_quest_state_cmd,					3},
		{"set_player_empire",						set_player_empire,					3},
		{"set_happy_hour",						set_happy_hour,					3},
		{"test_mob_spawn",						test_mob_spawn,					3},
		{"test_goto",						test_goto,					3},
		{"test_box",						test_box,					3},
		{"knockback",						knockback,					3},
		{"has_affect",						has_affect,					3},
		{"fnotice",						fnotice,					3},
		{"affect_test",						affect_test,					3},
		{"test_effect",						test_effect,					3},
		{"test_recipe",						test_recipe,					3},
		{"give_special",					give_special,					3},
		{"cinematic_mode",					cinematic_mode,					3},
		{"give_quest_reward",				give_quest_reward,					3},
		{"special_shop_seed",				special_shop_seed,					3},
		{"delay_shutdown",					delay_shutdown,					3},
		{"stop_shutdown",					stop_shutdown,					3},
		{"server_version",					server_version,					3},
		{"test_priv",					test_priv,					3},
	}
end

game_commands.command_list = command_list
