itemshop = {}

itemshop.OpenItemShop = function(is_first_open)
    if is_first_open then
        local delay = pc.getf("itemshop", "refresh_delay")
        if get_time() < delay then
            syschat(string.format(get_lctext("NEXT_ACTION_WAIT"), 10))
            return
        end
        pc.setf("itemshop", "refresh_delay", get_time()+10, true)
    end
    pc.open_itemshop(is_first_open)
end

itemshop.CloseItemShop = function()
    pc.close_itemshop()
    pc.set_exchange_time()
end

itemshop.RefreshItemShop = function()
    local delay = pc.getf("itemshop", "refresh_delay")
    if get_time() < delay then
        syschat(string.format(get_lctext("NEXT_ACTION_WAIT"), 10))
        return
    end
    pc.refresh_itemshop()
    pc.setf("itemshop", "refresh_delay", get_time()+10, true)
end

itemshop.BuyItem = function(item_index, item_quantity)
    if game.get_event_flag("block_itemshop") > 0 then
        syschat(get_lctext("This feature is temporarily unavailable."))
        return
    end

    local delay = pc.getf("itemshop", "buy_delay")
    if get_time() < delay then
        syschat(string.format(get_lctext("NEXT_ACTION_WAIT"), 1))
        return
    end

    if  item_quantity == nil or item_quantity < 1 or item_quantity > 10 then
        return
    end

    pc.buy_itemshop_item(item_index, item_quantity)
    pc.setf("itemshop", "buy_delay", get_time()+1, true)
end