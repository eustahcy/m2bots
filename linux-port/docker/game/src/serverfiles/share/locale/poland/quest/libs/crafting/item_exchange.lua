item_exchange = {}

local SOUL_STONE_EXCHANGE = 1
local SKILLBOOK_EXCHANGE = 2
local REFINEITEM_EXCHANGE = 3

item_exchange.SOUL_STONE_EXCHANGE = SOUL_STONE_EXCHANGE
item_exchange.SKILLBOOK_EXCHANGE = SKILLBOOK_EXCHANGE
item_exchange.REFINEITEM_EXCHANGE = REFINEITEM_EXCHANGE

local EXCHANGE_BY_VNUM = {
    [SOUL_STONE_EXCHANGE] = {
        result_vnum = 30360,
        price = 500,
        chance = 100,
    },
    [SKILLBOOK_EXCHANGE] = {
        result_vnum = 30366,
        price = 1000,
        chance = 100,
    },
    [REFINEITEM_EXCHANGE] = {
        result_vnum = 30378,
        price = 1000,
        chance = 55,
    }
}

local function get_item_grade()
    return math.floor(item.get_vnum() / 100) - 280
end

local function check_slot(inventory_slot, exchange_vnum)
    if item.select_cell(inventory_slot) then
        if exchange_vnum == SOUL_STONE_EXCHANGE then
            if item.get_type() != ITEM_METIN then
                return false
            end
            local grade = get_item_grade()
            if grade > 3 then
                return false
            end
        elseif exchange_vnum == SKILLBOOK_EXCHANGE then
            if item.get_type() != ITEM_SKILLBOOK then
                return false
            end
        elseif exchange_vnum == REFINEITEM_EXCHANGE then
            local vnum = item.get_vnum()
            if not (vnum >= 30000 and vnum <= 30092 or
            vnum >= 30192 and vnum <= 30199 or
            vnum >= 30343 and vnum <= 30359 or
            vnum == 30367) then
                return false
            end
        else
            return false
        end
        return true
    end
    return false
end

local function save_exchange_vnum(vnum)
    pc.setf("item_exchange", "vnum", vnum, true)
end

local function get_exchange_vnum()
    return pc.getf("item_exchange", "vnum", vnum)
end


item_exchange.get_inventory_slots_from_argument = function (inven_slot_string)
    local exchange_vnum = get_exchange_vnum()
    if exchange_vnum == 0 then
        return {}
    end

    local valid_pattern = "[^0-9,]*$"

    if not string.find(inven_slot_string, valid_pattern) then
        return {}
    end

    local string_slots = split(inven_slot_string, ",")
    string_slots = table_remove_duplicate(string_slots)

    local inventory_slots = {}
    for i,v in pairs(string_slots) do
        local slot_number = tonumber(v)
        if check_slot(slot_number, exchange_vnum) then
            table.insert(inventory_slots, tonumber(v))
        end
    end

    return inventory_slots
end

item_exchange.exchange = function(inventory_slots)

    if game.get_event_flag("block_item_exchange") > 0 then
        syschat(get_lctext("This feature is temporarily unavailable."))
        return
    end

    if pc.is_busy(BUSY_ITEM_EXCHANGE) then
        debug("you are busy")
        return
    end

    if pc.is_dead() then
        syschat(get_lctext("CANNOT_WHILE_DEAD"))
        return
    end

    if not pc.is_item_exchange() then
        debug("item exchange is closed")
        return
    end

    if table.getn(inventory_slots) < 1 then
        debug("invalid slots")
        return
    end

    if get_exchange_vnum() == 0 then
        return
    end
    local exchange_vnum = get_exchange_vnum()
    local exchange_data = EXCHANGE_BY_VNUM[exchange_vnum]
    local RESULT_ITEM = exchange_data.result_vnum
    local PRICE_PER_RESULT_ITEM = exchange_data.price

    local count = 0
    for i,v in pairs(inventory_slots) do
        if item.select_cell(v) then
            if not item.is_horse_inventory_usable() then
                syschat(get_lctext("Summon your horse to get access to horse inventory."))
                return
            end
            
            local item_count = item.get_count()
            if exchange_vnum == SOUL_STONE_EXCHANGE then
                local grade = get_item_grade()
                count = count + (grade + 1) * item_count
            else
                count = count + item_count
            end
        end
    end

    local cost = count * PRICE_PER_RESULT_ITEM

    if pc.get_gold() < cost then
        syschat(translate.general.no_gold)
        return
    end

    for i,v in pairs(inventory_slots) do
        if item.select_cell(v) then
            item.remove()
        end
    end

    local success_count = 0
    for i=1, count do
        if number(1,100) <= exchange_data.chance then
            success_count = success_count + 1
        end
    end

    pc.change_money(-cost)
    local stack_count = math.floor(success_count / ITEM_MAX_STACK)
    local rest_count = success_count - stack_count * ITEM_MAX_STACK

    if stack_count > 0 then
        for i=1, stack_count do
            pc.give_item2(RESULT_ITEM, ITEM_MAX_STACK)
        end
    end

    if rest_count > 0 then
        pc.give_item2(RESULT_ITEM, rest_count)
    end

    syschat(msg(translate.item_exchange.summary, success_count, count))
end

item_exchange.open = function(vnum)
    if pc.is_busy() then
        debug("you are busy")
        return
    end

    if pc.is_dead() then
        syschat(get_lctext("CANNOT_WHILE_DEAD"))
        return
    end

    pc.set_item_exchange(true)

    local exchange_data = EXCHANGE_BY_VNUM[vnum]
    save_exchange_vnum(vnum)
    cmdchat(string.format("item_exchange_open %d %d %d %d", exchange_data.result_vnum, exchange_data.price, exchange_data.chance, vnum))
end

item_exchange.close = function()
    pc.set_item_exchange(false)
    save_exchange_vnum(0)
    pc.set_exchange_time()
end