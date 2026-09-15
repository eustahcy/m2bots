busy_action = {}

busy_action.start = function(name, type, duration)
    if pc.is_dead() then
        syschat(get_lctext("CANNOT_WHILE_DEAD"))
        return
    end
    
    pc.set_busy_action(type, duration)
    pc.setf("busy_action", name, 1, true)
end

busy_action.is_busy = function (name)
    if pc.getf("busy_action", "delay") > get_time() then
        return true
    end

    if not pc.is_busy_action() then
        return false
    end

    if pc.getf("busy_action", name) < 1 then
        return false
    end

    return true
end

busy_action.stop = function(type, name, is_success)
    pc.set_busy_action(type, 0, is_success)
    pc.setf("busy_action", name, 0, true)
    pc.setf("busy_action", "delay", get_time()+1, true)
end
