crafting = {}

local function set_temp_flag(name, val)
    pc.setf("crafting", name, val, true)
end

local function set_flag(name, val)
    pc.setf("crafting", name, val)
end

local function get_flag(name)
    return pc.getf("crafting", name)
end

local function send_recipe_data_to_client(recipe_data)
    recipe_data = string.sub(recipe_data, 2)
    local send_msg = string.format("craft_recipe %d %s", get_flag("vnum"), recipe_data)
    debug(send_msg)
    cmdchat(send_msg)
end

local function is_crafting_open()
    if get_flag("vnum") < 1 then
        debug("no crafting opened!")
        return false
    end

    if npc.get_vid() != get_flag("npc_vid") then
        return false
    end

    return pc.is_crafting()
end

local function get_progress(vnum)
    return get_flag(string.format("progress_%d", vnum))
end

crafting.learn_recipe = function(vnum, base_chance, max_read_count)
    local data = crafting_data.get_data(vnum)
    if data.item_vnum == 0 then
        debug("no crafting data with vnum %s", vnum)
        return false
    end

    if get_progress(vnum) >= max_read_count then
        syschat(translate.crafting.recipe_known)
        return false
    end

    if pc.get_level() < data.requirements.min_level then
        syschat(string.format(translate.crafting.recipe_low_lvl, data.requirements.min_level))
        return false
    end

    local delta_delay = get_flag("learn_delay"..vnum) - get_time()
    local is_delay = delta_delay > 0
    local has_delay_remove = pc.has_affect(AFFECT_SKILL_NO_BOOK_DELAY)
    if is_delay and not has_delay_remove then
        debug(delta_delay)
        pc.learn_wait_message(delta_delay)
        return false
    end

    local chance_bonus = pc.get_point(POINT_LEARN_CHANCE)

    if pc.has_affect(AFFECT_SKILL_BOOK_BONUS) then
        affect.remove_new(AFFECT_SKILL_BOOK_BONUS)
    end

    if pc.has_affect(AFFECT_POTION_GENERAL_USE, POINT_LEARN_CHANCE) then
        affect.remove_new(AFFECT_POTION_GENERAL_USE, POINT_LEARN_CHANCE)
    end

    if is_delay and has_delay_remove then
        affect.remove_new(AFFECT_SKILL_NO_BOOK_DELAY)
        locale_chat("You have escaped the evil ghost curse with the help of an Exorcism Scroll.")
    end

    local chance = base_chance * (100 + chance_bonus) / 100
    debug("base_chance %d current_chance %d", base_chance, chance)
    if number(1,100) <= chance then
        set_flag("progress_"..vnum, get_progress(vnum) + 1)
        set_flag("learn_delay"..vnum, get_time() + SKILLBOOK_LEARN_DELAY)
        syschat(translate.crafting.recipe_learn_success)
    else
        syschat(translate.crafting.recipe_learn_fail)
    end
    return true
end

local function get_crafting_recipe_data(craft_vnum, craft_item_vnum)
    if crafting_data[craft_vnum] == nil then
        debug("no crafting data with vnum %s", craft_vnum)
        return nil
    end

    if not table_is_in(crafting_data[craft_vnum], craft_item_vnum) then
        debug("crafting item %d not available for craft vnum %d", craft_item_vnum, craft_vnum)
        return nil
    end

    local data = crafting_data.get_data(craft_item_vnum)

    if data.item_vnum == 0 then
        debug("no crafting data with vnum %s item %s", craft_vnum, craft_item_vnum)
        return nil
    end

    return data
end

local function is_crafting_item_available(craft_vnum, craft_item_vnum)
    local data = get_crafting_recipe_data(craft_vnum, craft_item_vnum)
    if data == nil then
        return false
    end

    if data.requirements == nil then
        return false
    end

    if data.requirements.min_level != nil and pc.get_level() < data.requirements.min_level then
        return false
    end

    if data.requirements.progress != nil then
        if get_progress(data.requirements.progress.vnum) < data.requirements.progress.max then
            if is_test_server() then
                return game.get_event_flag("herbalism_no_recipe") > 0
            else
                return false
            end
        end
    end

    return true
end

local function check_recipe_items(craft_vnum, craft_item_vnum, quantity)
    local data = get_crafting_recipe_data(craft_vnum, craft_item_vnum)
    if data == nil then
        return false
    end
    for j in range(1, table.getn(data.recipe), 2) do
        local material_vnum_data = data.recipe[j]
        if type(material_vnum_data) == "number" then
            material_vnum_data = {material_vnum_data}
        end
        local material_count = data.recipe[j+1] * quantity
        for i,material_vnum in ipairs(material_vnum_data) do
            if pc.count_item(material_vnum) < material_count then
                return false
            end
        end
    end
    return true
end

local function remove_recipe_items(craft_vnum, craft_item_vnum, quantity)
    local data = get_crafting_recipe_data(craft_vnum, craft_item_vnum)
    if data == nil then
        return false
    end
    for j in range(1, table.getn(data.recipe), 2) do
        local material_vnum_data = data.recipe[j]
        if type(material_vnum_data) == "number" then
            material_vnum_data = {material_vnum_data}
        end
        local material_count = data.recipe[j+1] * quantity
        for i,material_vnum in ipairs(material_vnum_data) do
            pc.remove_item(material_vnum, material_count)
        end
    end
    return true
end


crafting.send_recipes = function ()
    if not is_crafting_open() then
        return
    end

    local craft_vnum = get_flag("vnum")
    local data = crafting_data[craft_vnum]
    if data == nil then
        debug("no crafting data with vnum %s", craft_vnum)
        return
    end

    local send_data = ""
    for i,cvnum in pairs(data) do
        local v = crafting_data.get_data(cvnum)

        local material_data = ""
        for j in range(1, table.getn(v.recipe), 2) do
            local material_vnum_data = v.recipe[j]
            local material_count = v.recipe[j+1]

            local material_vnum_string = "";
            if type(material_vnum_data) == "table" then
                material_vnum_string = tableToString(material_vnum_data, "-")
            else
                material_vnum_string = tostring(material_vnum_data)
            end
            material_data = string.format("%s.%s.%d", material_data, material_vnum_string, material_count)
        end
        material_data = string.sub(material_data, 2)

        local req_min_level = 0
        local req_min_progress = 0
        if v.requirements != nil then
            if v.requirements.min_level != nil then
                req_min_level = v.requirements.min_level
            end

            if v.requirements.progress != nil then
                req_min_progress = v.requirements.progress.max
            end
        end

        local price = v.price
        if v.is_guild then
            if npc.get_guild() == pc.get_guild() then
                price = price - math.floor(price * 5 / 100) -- discount 5% if my guild
            end
        end

        local new_recipe_data = string.format("%d,%d,%d,%d,%d,%d,%d,%s", cvnum, v.item_vnum, v.count, price, v.chance, req_min_level, req_min_progress, material_data)
        local temp = string.format("%s|%s", send_data, new_recipe_data)
        if string.len(temp) > 480 then
            send_recipe_data_to_client(send_data)
            send_data = "|"..new_recipe_data
        else
            send_data = temp
        end
    end

    if string.len(send_data) > 0 then
        send_recipe_data_to_client(send_data)
    end
end

crafting.send_avail = function ()
    if not is_crafting_open() then
        return
    end

    local craft_vnum = get_flag("vnum")
    local data = crafting_data[craft_vnum]
    if data == nil then
        debug("no crafting data with vnum %s", craft_vnum)
        return
    end
    local send_data = ""
    for i,v in pairs(data) do
        local recipe_vnum = 0
        local data = get_crafting_recipe_data(craft_vnum, v)
        if data.requirements.progress != nil then
            recipe_vnum = data.requirements.progress.vnum
        end
        send_data = string.format("%s,%d:%d.%d", send_data, v, (is_crafting_item_available(craft_vnum, v) and 1 or 0), get_progress(recipe_vnum))
        if string.len(send_data) > 480 then
            send_data = string.sub(send_data, 2)
            local cmd = string.format("craft_avail %d %s", craft_vnum, send_data)
            send_data = ""
            debug(cmd)
            cmdchat(cmd)
        end
    end
    if string.len(send_data) > 0 then
        send_data = string.sub(send_data, 2)
        local cmd = string.format("craft_avail %d %s", craft_vnum, send_data)
        if string.len(cmd) > 512 then
            debug("CRAFTING ERROR SEND_AVAIL TOO LONG (len > 512)")
            return
        end
        debug(cmd)
        cmdchat(cmd)
    end
    cmdchat("craft_open")
end

crafting.create = function (craft_item_vnum, quantity)
    if game.get_event_flag("block_crafting") > 0 then
        syschat(get_lctext("This feature is temporarily unavailable."))
        return
    end

    if pc.is_busy(BUSY_CRAFTING) then
        debug("you are busy")
        return
    end

    if pc.is_dead() then
        syschat(get_lctext("CANNOT_WHILE_DEAD"))
        return
    end

    if not is_crafting_open() then
        return
    end

    local craft_vnum = get_flag("vnum")
    if not is_crafting_item_available(craft_vnum, craft_item_vnum) then
        return
    end

    if pc.is_busy(BUSY_CRAFTING) then
        syschat(translate.general.wait_after_exchange)
        return
    end

    if get_flag("npc_vid") == 0 or npc.select(get_flag("npc_vid")) == 0 then
        syschat(translate.general.error)
        return
    end

    if not npc.is_near(10) then
        syschat(translate.general.too_far)
        return
    end

    local data = get_crafting_recipe_data(craft_vnum, craft_item_vnum)
    
    if not item.has_flag(data.item_vnum, ITEM_FLAG_STACKABLE) and quantity > 1 then
        quantity = 1
    end
    if item.get_size(data.item_vnum) > 1 then
        quantity = 1
    end

    local max_stack = item.get_max_stack(data.item_vnum)
    local total_item_count = data.count * quantity
    local stack_count = total_item_count / max_stack
    local rest = total_item_count - stack_count * max_stack
    local need_inv_space = stack_count
    if rest > 0 then
        need_inv_space = need_inv_space + 1
    end

    if pc.get_empty_inventory_count() < need_inv_space then
        syschat(translate.general.no_inventory_space)
        return
    end

    local price = data.price
        if data.is_guild then
            if npc.get_guild() == pc.get_guild() then
                price = price - math.floor(price * 5 / 100) -- discount 5% if my guild
            end
        end

    if pc.get_gold() < price * quantity then
        syschat(translate.general.no_gold)
        return
    end

    if not check_recipe_items(craft_vnum, craft_item_vnum, quantity) then
        syschat(translate.crafting.no_materials)
        return
    end

    if remove_recipe_items(craft_vnum, craft_item_vnum, quantity) then
        local craftedItemID = 0
        local success_count = 0

        if price > 0 then
            pc.change_money(-price * quantity)
        end

        for i=1, quantity do
            if number(1,100) <= data.chance then
                success_count = success_count + 1
            end
        end

        local total_count = data.count * success_count
        if success_count > 0 then
            if total_count > max_stack then
                local stack_count = math.floor(total_count / max_stack)
                local rest = total_count - stack_count * max_stack
                for i=1,stack_count do
                    craftedItemID = pc.give_item2(data.item_vnum, max_stack, -1, true)
                end
                if rest > 0 then
                    craftedItemID = pc.give_item2(data.item_vnum, rest, -1, true)
                end
            else 
                craftedItemID = pc.give_item2(data.item_vnum, total_count, -1, true)
            end
        end

        if quantity == 1 then
            if success_count > 0 then
                syschat(translate.crafting.success)
            else
                syschat(translate.crafting.fail)
            end
        else
            syschat(string.format(translate.crafting.result, quantity, success_count))
        end

        write_log(string.format("INSERT INTO crafting_log VALUES (%d, NOW(), %d, %d, %d, %d, %d, %d, %d)", pc.get_player_id(), craft_item_vnum, quantity, data.item_vnum, craftedItemID, total_count, success_count, quantity - success_count))
    end
    debug("create item %d", craft_item_vnum)
    cmdchat("craft_update "..craft_item_vnum)

    -- herbalism onboarding hardcode
    if pc.getf("herbalism_onboarding", "completed") == 1 and craft_item_vnum == 11 then
        say_info()
        say_split(translate.herbalism.prime_quest.state_craft.success)
        pc.setf("herbalism_onboarding", "completed", 2)
        set_quest_state("herbalism_onboarding", "reward")
        crafting.close()
    end
    -- end of herbalism onboarding
end

crafting.open = function (craft_vnum)
    if pc.is_busy() then
        debug("you are busy")
        return
    end

    if pc.is_dead() then
        syschat(get_lctext("CANNOT_WHILE_DEAD"))
        return
    end

    if pc.is_crafting() then
        debug("crafting jest juz odpalony")
        return
    end

    if get_flag("open_delay") > get_time() then
        syschat(translate.crafting.open_delay)
        return
    end

    set_temp_flag("npc_vid", npc.get_vid())
    pc.set_crafting(true)
    debug("open crafting %d", craft_vnum)
    set_temp_flag("vnum", craft_vnum)
    crafting.send_recipes()
    crafting.send_avail()
    set_temp_flag("open_delay", get_time()+5)
    -- cmdchat("craft_request "..craft_vnum)
end

crafting.close = function ()
    if not pc.is_crafting() then
        debug("crafting jest wylaczony przeciez?")
        return
    end

    set_temp_flag("npc_vid", 0)
    pc.set_crafting(false)
    debug("close crafting")
    set_temp_flag("vnum", 0)
    pc.set_exchange_time()
end
