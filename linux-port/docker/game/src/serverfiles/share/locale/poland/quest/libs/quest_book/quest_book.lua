quest_book = {}

local function get_abandon_price(difficulty)
    if difficulty == 0 then
        return 5000
    elseif difficulty == 1 then
        return 20000
    elseif difficulty == 2 then
        return 80000
    elseif difficulty == 3 then
        return 200000
    else
        return 1
    end
end

local function get_quest_difficulty(quest_index)
    for difficulty,quest_list in pairs(quest_book_data.QUEST_LIST_BY_DIFFICULTY) do
        if table_is_in(quest_list, quest_index) then
            return difficulty
        end        
    end
    return -1
end

quest_book.is_on_quest = function ()
    return pc.getf("quest_book_manage","quest_index") > 0
end

quest_book.get_quest_data = function (quest_index)
    return quest_book_data.QUEST_LIST[quest_index]
end

quest_book.get_current_quest_data = function ()
    return quest_book.get_quest_data(pc.getf("quest_book_manage", "quest_index"))
end

quest_book.abandon = function (is_item_remove)
    say_info()
    local difficulty = get_quest_difficulty(pc.getf("quest_book_manage","quest_index"))
    local price = get_abandon_price(difficulty)
    say_split(string.format(translate.quest_book.on_quest, parse_number(price)))
    if select(translate.general.yes, translate.general.no) == 2 then
        return false
    end
    if is_item_remove and not check_safe_item() then
        say_info()
        say_reward(translate.general.not_available)
        say()
        return
    end
    if pc.get_gold() < price then
        say_info()
        say_split(translate.general.no_gold)
        say()
        return false
    end

    if not pc.can_warp() then
        say_info()
        say_split(translate.general.wait_after_exchange)
        say()
        return false
    end

    pc.change_money(-price)

    local data = quest_book.get_current_quest_data()
    if data.type == QUEST_SCROLL_TYPE_KILL_MOB then
        set_quest_state("quest_book_kill", "start")
    end

    pc.setf("quest_book_manage","quest_index", 0)
    pc.setf("quest_book_manage","exclude_index", 1)
    clear_letter()
    return true
end

local function get_random_quest_index(difficulty, exclude)
    local quest_list = quest_book_data.QUEST_LIST_BY_DIFFICULTY[difficulty]
    local quest_index = table_get_random_item(quest_list)
    if table.getn(quest_list) > 1 and quest_index == exclude then
        return get_random_quest_index(difficulty, exclude)
    end
    return quest_index
end

quest_book.start_quest = function ()
    local difficulty = item.get_value(0)
    local exclude = 0
    if quest_book.is_on_quest(true) then
        say_info()
        say_split(translate.quest_book.is_on_quest, say_reward)
        return
        -- exclude = pc.getf("quest_book_manage", "quest_index")
        -- if not quest_book.abandon() then
        --     return
        -- end
    end

    if item.remove(1) then
        local quest_index = get_random_quest_index(difficulty, pc.getf("quest_book_manage","exclude_index"))
        local quest_data = quest_book.get_quest_data(quest_index)

        syschat(msg(translate.quest_book.new_quest, quest_data.title))
        pc.setf("quest_book_manage", "quest_index", quest_index)
        if quest_data.type == QUEST_SCROLL_TYPE_KILL_MOB then
            for i in range(1, table.getn(quest_data.mobs), 2) do
                local index = math.floor(i / 2)
                pc.setf("quest_book_kill", "progress"..index, quest_data.mobs[i+1])
            end
            set_quest_state("quest_book_kill", "progress")
        else
            debug("unknown type %d", quest_data.type)
        end
    end
end

local function get_value_from_pool_table(pool_table)
    local total_pool = 0
    local table_len = table.getn(pool_table)
    for i in range(1, table_len, 2) do
        total_pool = total_pool + pool_table[i + 1]
    end

    local pool_check = 1
    local lucky_number = number(1, total_pool)
    for i in range(1, table_len, 2) do
        local value = pool_table[i]
        local pool_item = pool_table[i + 1]
        local next_pool = pool_check + pool_item
        if lucky_number >= pool_check and lucky_number < next_pool then
            return value
        end
        pool_check = next_pool
    end
    return 0
end

quest_book.reward = function ()
    say_info()
    say(translate.quest_book.reward)
    say()

    local data = quest_book.get_current_quest_data()
    local reward_type = data.rewardType
    local exp = get_value_from_pool_table(quest_book_data.QUEST_REWARD.exp[reward_type])
    local gold = get_value_from_pool_table(quest_book_data.QUEST_REWARD.gold[reward_type])

    local item_count = 1
    if number(1,4) == 1 then -- 25% szans na dodatkowy item
        item_count = item_count + 1
    end

    local is_drop_premium = pc.get_premium_remain_sec(PREMIUM_ITEM)
    if is_drop_premium then -- jezeli masz premium to 1 wiecej
        item_count = item_count + 1
    end

    pc.setf("quest_book_manage", "quest_index", 0)

    pc.add_player_stat(PLAYER_STATS_QUESTBOOK_FLAG)
    pc.give_exp2(exp)
    pc.change_money(gold)

    say_reward(msg("%s %s", parse_number(exp), translate.general.xp))
    say_reward(msg("%s %s", parse_number(gold), translate.general.gold))

    local drop_items = get_special_items(quest_book_data.QUEST_REWARD.special_group[reward_type], item_count, true)
    for i,v in pairs(drop_items) do
        local rare_pct = math.max(v.rare_pct, 50)
        pc.give_item2(v.vnum, v.count, rare_pct)
        say_reward(msg("%dx %s", v.count, item_name(v.vnum)))
    end
    say()
end