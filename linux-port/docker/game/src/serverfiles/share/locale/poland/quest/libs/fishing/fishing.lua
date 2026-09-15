fishing_manage = {}

FISHING_ROD_VALUE_BONUS = 0
FISHING_ROD_CHANCE_NEXT_POINT = 1
FISHING_ROD_NEEDED_POINTS = 2
FISHING_ROD_CHANGE_TO_REFINE = 3
FISHING_ROD_REDUCE_VNUM = 4
FISHING_ROD_BONUS_FISH_CHANCE = 5

FISHING_ROD_SOCKET_CURRENT_POINTS = 0
-- FISHING_ROD_SOCKET_UNUSED = 1
FISHING_ROD_SOCKET_BAIT = 2

local function get_bait_bonus(itemVnum)
    return item.proto_get_value(itemVnum, 0)
end

local function get_rod_level()
    if item.select_wear(WEAR_WEAPON) then
        if item.get_type() != ITEM_ROD then
            return 0
        end
        return (item.get_vnum() - 27400) / 10
    end
    return 0
end

local function get_rod_bonus()
    local bonus = 0
    if item.select_wear(WEAR_WEAPON) then
        if item.get_type() != ITEM_ROD then
            return 0
        end
        bonus = bonus + item.get_value(FISHING_ROD_VALUE_BONUS) + get_bait_bonus(item.get_socket(FISHING_ROD_SOCKET_BAIT))
    end

    if pc.get_premium_remain_sec(PREMIUM_FISH_MIND) > 0 then
        bonus = bonus + 10
    end

    return bonus
end

local function can_predict()
    return pc.has_affect(AFFECT_FISH_MIND_PILL) or pc.get_premium_remain_sec(PREMIUM_FISH_MIND) > 0
end

local function get_fishing_chance()
    local chance = 20
    if pc.has_affect(AFFECT_FISH_MIND_PILL) then
        chance = chance + 2
    end

    if pc.get_premium_remain_sec(PREMIUM_FISH_MIND) > 0 then
        chance = chance + 10
    end

    if item.select_wear(WEAR_WEAPON) then
        if item.get_type() != ITEM_ROD then
            return 0
        end
        chance = chance + item.get_value(FISHING_ROD_BONUS_FISH_CHANCE)
    end

    return chance
end

local function simulate_fishing(count, chanceToFailGame, fish_chance)
    chanceToFailGame = chanceToFailGame or 1
    fish_chance = fish_chance or get_fishing_chance()

    local fail = 0
    local failed_games = 0
    local chance_bonus = 0
    for i=1, count do
        -- debug(chance_bonus)
        if number(1,100) <= fish_chance + chance_bonus then
            if number(1,1000) > chanceToFailGame then
                local drop = fishing_drop.get_random_drop(get_rod_level(), get_rod_bonus())
                pc.give_item2(drop.itemVnum, drop.itemCount)
                chance_bonus = 0
            else
                failed_games = failed_games + 1
            end
        else
            chance_bonus = math.min(chance_bonus+2, math.max(25 - math.floor(fish_chance / 2)))
            fail = fail + 1
        end
        pc.practice_fishing()
    end
    local total_fails = fail + failed_games
    local total_success = count - total_fails

    local average_time_to_wait = 17.5 + 2 -- +2s (fish) react time
    local average_time_in_game = 6
    local time_in_game = average_time_in_game * total_success
    local time_waiting = count * average_time_to_wait
    local total_time = time_waiting + time_in_game
    debug("### FISHING SIMULATION RESULTS ###")
    debug("Fishing bonus: %d (rod Level %d)", get_rod_bonus(), get_rod_level())
    debug("Attempts: %d (fishing chance %d%%) (game fail chance %.1f%%)", count, fish_chance, chanceToFailGame/10)
    debug("Fails in total: %d (normal fails %d) (game fails %d)", total_fails, fail, failed_games)
    debug("Success %d", total_success)
    debug("Time in total: %ds (time waiting %ds) (time in game %ds)", total_time, time_waiting, time_in_game)
    debug("#########")
end

local function simulate_calm()
    local r = number(1,100)
    if r <= 50 then
        syschat(get_lctext("You find a simple Piece of Stone in the Clam."))
        pc.give_item2(27990)
    else
        local prob_table = {95, 97, 99}

        if r <= prob_table[1] then
            syschat(get_lctext("The Clam has vanished."))
        elseif r <= prob_table[2] then
            syschat(get_lctext("There is a White Pearl inside the Clam."))
            pc.give_item2(27992);
        elseif r <= prob_table[3] then
            syschat(get_lctext("There is a Blue Pearl inside the Clam."))
            pc.give_item2(27993);
        else
            syschat(get_lctext("There is a Blood-Red Pearl inside the Clam."))
            pc.give_item2(27994);
        end
    end
end

local function success()
    local dropIndex = pc.getf("fishing", "drop_index")
    local drop = fishing_drop.get_drop_by_index(dropIndex)
    if drop == nil then
        debug("FISHING DROP ERROR")
        syschat("Wystapil blad (fishing_drop_success).")
        return
    end

    local localeText = ""
    if item.proto_get_type(drop.itemVnum) == ITEM_FISH then
        localeText = get_lctext("FISHING_SUCCESS2")
    else
        localeText = get_lctext("FISHING_SUCCESS1")
    end

    local dropItem = drop.itemVnum
    syschat(string.format(localeText, item.proto_get_name(drop.itemVnum)))
    pc.fishing_log(dropItem, drop.itemCount)
    pc.give_item2(dropItem, drop.itemCount, -1, true)
end

local function predict_fish()
    local drop = fishing_drop.get_random_drop(get_rod_level(), get_rod_bonus())
    if drop == nil then
        debug("FISHING DROP ERROR")
        syschat("Wystapil blad (kod: fishing_predict).")
        return
    end
    debug("predict vnum: %s, itemCount: %d", tostring(drop.itemVnum), drop.itemCount)
    pc.setf("fishing", "drop_index", drop.index, true)

    if can_predict() then
        local localeText = ""
        if item.proto_get_type(drop.itemVnum) == ITEM_FISH then
            localeText = get_lctext("FISHING_NOTIFY2")
        else
            localeText = get_lctext("FISHING_NOTIFY1")
        end
        syschat(string.format(localeText, drop.name))
    else
        syschat(get_lctext("FISHING_UNKNOWN"))
    end
end

local function ShowRodRefineRequirement()
    local refineVnum = item.get_refine_set()
    debug(refineVnum)
    local recipe = {item.get_refine_recipe(refineVnum)}
    local length = table.getn(recipe)
    for i in range(1, length, 2) do
        local itemVnum = recipe[i]
        local itemCount = recipe[i+1]
        debug(itemVnum)
        debug(itemCount)
        if itemVnum == 0 then
            break
        else
            say(string.format("- %dx %s ", itemCount, item.proto_get_name(itemVnum)))
        end
    end
end

fishing_manage.success = success
fishing_manage.predict_fish = predict_fish
fishing_manage.simulate_fishing = simulate_fishing
fishing_manage.simulate_calm = simulate_calm
fishing_manage.ShowRodRefineRequirement = ShowRodRefineRequirement