potion_recharge = {}

local AUTOPOTION_TYPE_HP = 0
local AUTOPOTION_TYPE_SP = 1

potion_recharge.charge = function(potion_inven_pos, items_inven_slots)
    if game.get_event_flag("block_potion_recharge") > 0 then
        syschat(get_lctext("This feature is temporarily unavailable."))
        return
    end

    if pc.is_busy(BUSY_POTION_RECHARGE) then
        debug("is busy")
        return
    end

    if pc.is_dead() then
        syschat(get_lctext("CANNOT_WHILE_DEAD"))
        return
    end

    if not pc.is_potion_recharge() then
        debug("potion recharge is closed")
        return
    end

    if table.getn(items_inven_slots) < 1 then
        debug("empty slots")
        return
    end

    -- check main potion
    local potion_type = -1
    local missing_value = 0
    local potion_used_value = 0
    local potion_max_value = 0
    if item.select_cell(potion_inven_pos) then
        if item.get_vnum() >= 72723 and item.get_vnum() <= 72726 then
            potion_type = AUTOPOTION_TYPE_HP
        elseif item.get_vnum() >= 72727 and item.get_vnum() <= 72730 then
            potion_type = AUTOPOTION_TYPE_SP
        end

        if potion_type < 0 then
            debug("wrong item")
            return
        end

        if item.get_socket(0) == 1 then
            debug("item is used")
            return
        end

        if item.get_count() > 1 then
            debug("item count > 1")
            return
        end

        if not item.is_horse_inventory_usable() then
            syschat(get_lctext("Summon your horse to get access to horse inventory."))
            return
        end
        
        potion_used_value = item.get_socket(1)
        if potion_used_value == 0 then
            debug("item is full")
            return
        end

        potion_max_value = item.get_socket(2)
        missing_value = potion_used_value
    end

    items_inven_slots = table_remove_duplicate(items_inven_slots)

    -- check items
    for i, inven_pos in ipairs(items_inven_slots) do
        if item.select_cell(inven_pos) then
            if not item.is_horse_inventory_usable() then
                syschat(get_lctext("Summon your horse to get access to horse inventory."))
                return
            end

            if item.get_type() != ITEM_USE or item.get_sub_type() != USE_POTION then
                debug("item is not use_potion")
                return
            end
            
            if potion_type == AUTOPOTION_TYPE_HP and item.get_value(0) < 1 then
                debug("wrong charge item")
                return
            elseif potion_type == AUTOPOTION_TYPE_SP and item.get_value(1) < 1 then
                debug("wrong charge item")
                return
            end
        end
    end

    local recharge_value = 0
    -- start recharge
    for i,inven_pos in pairs(items_inven_slots) do
        if missing_value < 1 then
            debug("potion is full2")
            break
        end
        
        if item.select_cell(inven_pos) then
            local item_count = item.get_count()
            local item_value = 0

            if potion_type == AUTOPOTION_TYPE_HP then
                item_value = item.get_value(0)
            elseif potion_type == AUTOPOTION_TYPE_SP then
                item_value = item.get_value(1)
            end

            if item_value < 1 then
                debug("value is 0")
                return
            end

            local total_value = item_value * item_count

            if missing_value >= total_value then
                recharge_value = recharge_value + total_value
                missing_value = missing_value - total_value
                item.remove()
            else
                local need_count = math.ceil(missing_value / item_value)
                recharge_value = recharge_value + item_value * need_count
                missing_value = 0
                item.remove(need_count)
                break
            end
        end
    end

    debug("recharge_value: %d", recharge_value)
    if recharge_value > 0 then
        local set_value = math.max(potion_used_value - recharge_value, 0)
        if item.select_cell(potion_inven_pos) then
            item.set_socket(1, set_value)
            syschat(translate.potion_recharge.success)
        end
    end
end

potion_recharge.open = function()
    if pc.is_busy() then
        debug("you are busy")
        return
    end

    if pc.is_dead() then
        syschat(get_lctext("CANNOT_WHILE_DEAD"))
        return
    end
    
    pc.set_potion_recharge(true)
    cmdchat("PotionRechargeOpen")
end

potion_recharge.close = function()
    pc.set_potion_recharge(false)
    pc.set_exchange_time()
end