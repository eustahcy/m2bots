potion_system = {}


local DURATION_VALUE_INDEX = 0
local GROUP_VALUE_INDEX = 1
local COOLDOWN_VALUE_INDEX = 2
local SHARED_COOLDOWN_VALUE_INDEX = 3
local CUSTOM_AFFECT_VALUE_INDEX = 4

local GROUP_TYPE_GENERAL_USE = 0
local GROUP_TYPE_BOOST = 1
local GROUP_TYPE_OFFENISE = 2
local GROUP_TYPE_DEFENSIVE = 3
local GROUP_TYPE_FOOD = 4

local function get_max_affect_count_for_group(group)
    local data = {
        [GROUP_TYPE_BOOST] = 5,
        [GROUP_TYPE_OFFENISE] = 3,
        [GROUP_TYPE_DEFENSIVE] = 2,
        [GROUP_TYPE_FOOD] = 2,
    }
    return data[group]
end

potion_system.use_potion = function ()
    local level_limit = item.get_level_limit()
    if pc.get_level() < level_limit then
        syschat(string.format(translate.general.low_lvl, level_limit))
        return
    end

    local applies = {item.get_apply()}

    local item_vnum = item.get_vnum()
    local group = item.get_value(GROUP_VALUE_INDEX)
    local affect_type = AFFECT_POTION_START + group
    local duration = item.get_value(DURATION_VALUE_INDEX)
    local cooldown = item.get_value(COOLDOWN_VALUE_INDEX)
    local shared_cooldown_index = item.get_value(SHARED_COOLDOWN_VALUE_INDEX)
    local custom_affect = item.get_value(CUSTOM_AFFECT_VALUE_INDEX)
    if duration == 0 then
        duration = INFINITE_AFFECT_DURATION
    end

    local extend_duration = item.get_socket(0)
    if extend_duration == 50 or extend_duration == 100 then
        duration = duration * (100+item.get_socket(0)) / 100
    end

    if cooldown > 0 then
        local current_cooldown = pc.getf("potion", "cooldown"..shared_cooldown_index) - get_time()
        debug(current_cooldown)
        if current_cooldown > 0 then
            local minutes = math.floor(current_cooldown / 60)
            if minutes > 0 then
                syschat(string.format(get_lctext("POTION_COOLDOWN_MINUTE%d"), minutes+1))
            else
                local seconds = current_cooldown - minutes * 60
                syschat(string.format(get_lctext("POTION_COOLDOWN_SECOND%d"), seconds))
            end
            return
        end

        pc.setf("potion", "cooldown"..shared_cooldown_index, get_time() + cooldown)
    end

    debug("level limit: %d duration: %d group: %d", level_limit, duration, group)
    local is_new_affect = true
    for i in range(1, table.getn(applies), 2) do
        local point = applies[i]
        local value = applies[i+1]
        debug("type: %d value: %d", applies[i], applies[i+1])

        -- block mall potions stack
        if point == POINT_MOV_SPEED and pc.has_affect(AFFECT_MALL, POINT_MOV_SPEED) then
            syschat(translate.potions.already_used)
            return
        end

        if point == POINT_ATT_SPEED and pc.has_affect(AFFECT_MALL, POINT_ATT_SPEED) then
            syschat(translate.potions.already_used)
            return
        end
        -- end block mall

        local current_affect_value = pc.get_affect_value(affect_type, point)
        if current_affect_value > 0 then
            is_new_affect = false
        end
        if current_affect_value >= value then
            syschat(translate.potions.already_used)
            return
        end
    end

    if custom_affect > 0 and pc.has_affect(custom_affect) then
        syschat(translate.potions.already_used)
        return
    end

    if is_new_affect and group != GROUP_TYPE_GENERAL_USE then
        local count = affect.get_count(affect_type)
        local max_count = get_max_affect_count_for_group(group)
        debug("%d/%d", count, max_count)
        if count >= max_count then
            syschat(string.format(translate.potions.limit_use, max_count))
            return
        end
    end

    if custom_affect > 0 then
        if pc.has_affect(CUSTOM_AFFECT_VALUE_INDEX) then
            locale_chat("This effect is already activated.")
            return
        end
        affect_type = custom_affect
    end

    if not affect.is_loaded() then
        return
    end

    if item.remove(1) then

        local potion_effect_type = SE_DRINK_POTION

        point_len = table.getn(applies)
        if point_len > 0 then
            for i in range(1, point_len, 2) do
                local point = applies[i]
                local value = applies[i+1]
                affect.add_new(affect_type, point, value, duration, AFF_NONE, true, true, item_vnum)

                if point == POINT_MOV_SPEED then
                    potion_effect_type = SE_DXUP_PURPLE
                elseif point == POINT_ATT_SPEED then
                    potion_effect_type = SE_SPEEDUP_GREEN
                end
            end
        else
            affect.add_new(affect_type, POINT_NONE, 0, duration, AFF_NONE, true, true, item_vnum)
        end

        pc.show_effect(potion_effect_type)
    end
end