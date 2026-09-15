collect_data = {}

collect_data.is_no_time_on_fail = function()
    return pc.has_affect(AFFECT_COLLECT_NO_FAIL_TIME)
end

collect_data.is_no_time = function()
    return pc.has_affect(AFFECT_COLLECT_NO_TIME)
end

collect_data.get_chance_bonus = function()
    return pc.get_point(POINT_COLLECT_CHANCE)
end

collect_data.set_wait_time = function()
    local t = time_until_hour(GLOBAL_COOLDOWN_RESET_HOUR)

    if game.get_event_flag("beta_server") > 0 then
        t = 60
    end

    pc.setf("collect_quest", "wait", get_time() + t)
    collect_data.send_delay()
end

collect_data.reset_time = function()
    pc.setf("collect_quest", "wait", 0)
    collect_data.send_delay()
end


collect_data.is_research_in_progress = function()
    return get_time() < pc.getf("collect_quest", "wait")
end

collect_data.is_wait = function()
    if not collect_data.is_research_in_progress() then
        return false
    end

    if collect_data.is_no_time() then
        return false
    end

    return true
end

collect_data.send_delay = function ()
    if collect_data.is_wait() then
        local left_seconds = pc.getf("collect_quest", "wait") - get_time()
        q.set_clock(translate.general.left, left_seconds)
    else
        q.set_clock(translate.general.left, 0)
    end
end

collect_data.say_delay = function ()
    if collect_data.is_wait() then
        say()
        local left_seconds = pc.getf("collect_quest", "wait") - get_time()
		say_split(msg(translate.collect_quest.wait_delay, second_to_smart_time(left_seconds)), say_reward)
    end
end

collect_data.take_chance = function (chance)
    chance = chance * (100+collect_data.get_chance_bonus()) / 100
    if number(1,100) <= chance then
        if collect_data.is_research_in_progress() then
            affect.remove_new(AFFECT_COLLECT_NO_TIME)
        end

        collect_data.set_wait_time()
        return true
    else
        if collect_data.is_no_time_on_fail() then
            affect.remove_new(AFFECT_COLLECT_NO_FAIL_TIME)
        else
            collect_data.set_wait_time()
        end
    end
    return false
end