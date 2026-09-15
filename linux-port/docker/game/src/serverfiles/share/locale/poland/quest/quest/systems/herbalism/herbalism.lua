define HERBALISM_SHOP 14
quest herbalism begin
    state start begin
        function can_craft()
            return pc.getf("herbalism_onboarding", "completed") > 0
        end

        function is_unlocked()
            return pc.getf("herbalism_onboarding", "completed") > 2
        end

        function upgrade_knife(is_success)
            if not is_success and number(1,2) == 1 then
                return
            end
            
            local current_points = item.get_socket(0)
            local max_points = item.get_value(2)

            if current_points >= max_points then
                return
            end

            local new_points = current_points + 1
            item.set_socket(0, new_points)
            syschat(string.format(translate.herbalism.knife_upgrade_point, new_points, max_points))
            if new_points >= max_points then
                syschat(translate.herbalism.knife_upgrade_max)
            end
        end


        when 20602.click or
        20606.click or
        20609.click or
        20610.click or
        20614.click or
        20617.click or
        20619.click or
        20620.click or
        20623.click or
        20624.click or
        20628.click or
        20629.click or
        20631.click or
        20632.click or
        20641.click or
        20644.click begin
            if busy_action.is_busy("herb_pick") then
                debug("already busy")
                return
            elseif not npc.is_near(2) then
                syschat(translate.herbalism.too_far)
                return
            elseif pc.is_mount() then
                syschat(translate.herbalism.no_horse)
                return
            elseif pc.is_polymorphed() then
                syschat(translate.general.polymorph)
                return
            elseif pc.get_level() < npc.get_level() then
                syschat(string.format(translate.herbalism.low_lvl, npc.get_level()))
                return
            end

            local itemVnum = pc.get_wear(WEAR_WEAPON)
            if itemVnum == nil or itemVnum < 29201 or itemVnum > 29210 then
                syschat(translate.herbalism.knife_needed)
                return
            end

            timer("herb_pick", 3)
            busy_action.start("herb_pick", BUSY_ACTION_HERB, 3)
            save_npc()
        end


        when herb_pick.timer begin
            if busy_action.is_busy("herb_pick") then
                if not item.select_wear(WEAR_WEAPON) or item.get_vnum() < 29201 or item.get_vnum() > 29210 then
                    syschat(translate.herbalism.knife_needed)
                    busy_action.stop(BUSY_ACTION_HERB, "herb_pick", false)
                    return
                end

                local knife_add_chance = item.get_value(0)

                local npc_vid = get_saved_npc()
                local fail_bonus = pc.getf("herbalism","fail_bonus")
                local gather_chance = 30 + knife_add_chance + pc.get_mining_skill_bonus() + fail_bonus
                
                local has_herbalist_bonus = pc.has_affect(AFFECT_HERBALIST_BONUS)
                if has_herbalist_bonus then
                    gather_chance = gather_chance + 2
                end

                debug("gather chance %d%%", gather_chance)
                local is_success = number(1,100) <= gather_chance
                if not release_npc() then
                    syschat(translate.herbalism.fail)
                    busy_action.stop(BUSY_ACTION_HERB, "herb_pick", false)
                    return
                end

                herbalism.upgrade_knife(is_success)

                local drop_item = npc.get_drop_item()
                
                busy_action.stop(BUSY_ACTION_HERB, "herb_pick", is_success)
                if is_success and drop_item > 0 then
                    syschat(table_get_random_item(translate.herbalism.success_info))

                    pc.setf("herbalism", "fail_bonus", 0)
                    pc.pickup(npc_vid)

                    local item_count = number(2,3)
                    if has_herbalist_bonus and number(1,5) == 1 then
                        item_count = item_count + 2
                        syschat(translate.herbalism.count_2_bonus)
                    end

                    pc.give_item2(drop_item, item_count, -1, true)
                    pc.add_player_stat(PLAYER_STATS_HERBALISM_FLAG, item_count)

                    npc.set_string_flag("herb", npc.get_string_flag("herb")+1)
                    local current_progress = npc.get_string_flag("herb")
                    local purge_chance_data = {0, 15, 40, 60, 80, 100}
                    local purge_chance = purge_chance_data[current_progress]
                    if current_progress >= table.getn(purge_chance_data) or number(1,100) <= purge_chance then
                        npc.purge()
                    end
                else
                    syschat(table_get_random_item(translate.herbalism.fail_info))
                    pc.setf("herbalism", "fail_bonus", math.min(fail_bonus + 4, 20), true)
                end
            end
        end

        when 3.use_type with item.get_sub_type() == USE_CRAFT_RECIPE begin
            if not herbalism.can_craft() then
                syschat(translate.herbalism.prime_quest.complete_quest_first)
                return
            end

            local vnum = item.get_value(0)
            local chance = item.get_value(1)
            local max_read = item.get_value(2)
            if crafting.learn_recipe(vnum, chance, max_read) then
                item.remove(1)
            end
        end

        when 20018.chat.translate.crafting.npc_chat with herbalism.can_craft() begin
            crafting.open(CRAFTING_BAEKGO)
            setskin(NOWINDOW)
        end

        when 20018.chat.translate.general.open_shop begin
            npc_name()
            say_split(translate.herbalism.open_shop1)
            wait()
            pc.open_special_shop(HERBALISM_SHOP)
            setskin(NOWINDOW)
        end

        when 20018.chat.translate.herbalism.refine_knife with herbalism.is_unlocked() begin
            npc_name()
            say_split(translate.herbalism.refine_knife_cannot)
        end

        when 20017.chat.translate.herbalism.refine_knife with herbalism.is_unlocked() begin
            npc_name()
            say_split(translate.herbalism.refine_knife_can_refine)
        end

        when 20017.take with item.vnum >= 29201 and item.vnum < 29210 and item.get_socket(0) != item.get_value(2) begin
            if not check_safe_item() then return end
			npc_name()
			say_split(translate.herbalism.refine_low_points)
		end

        when 20022.chat.translate.herbalism.extend_duration.npc_chat begin
            npc_name()
            say_split(translate.herbalism.extend_duration.talk1)
            wait()
            npc_name()
            say_split(translate.herbalism.extend_duration.talk2)
            say()
            say_split(translate.herbalism.extend_duration.talk3)
        end

        when 20022.take with item.get_type() == ITEM_POTION begin
            if not check_safe_item() then return end
            if item.get_vnum() >= 27863 and item.get_vnum() < 27900 then
                return
            end
            pc.set_crafting(true)
            npc_name()
            if item.get_socket(0) > 0 or item.get_value(1) == 0 or item.get_value(1) > 1200 then
                say_split(translate.herbalism.extend_duration.cannot_extend)
                pc.set_crafting(false)
                return
            end
            say_split(translate.herbalism.extend_duration.take_item_info1)
            say_split(translate.herbalism.extend_duration.take_item_info2)
            option_table = translate.herbalism.extend_duration.take_item_select
            local x = select_table(option_table)
            if x == table.getn(option_table) then
                pc.set_crafting(false)
                return
            end
            local cost_per_potion = 2500
            local dust_per_potion = 1
            local level_limit = get_item_limit(item.get_vnum(), LIMIT_LEVEL)

            local item_vnum = item.get_vnum()  -- wyjatki dla ziel i fiol potek
            if item_vnum >= 27100 and item_vnum <= 27105 then
                if item_vnum == 27101 or item_vnum == 27105 then
                    level_limit = 30
                elseif item_vnum == 27102 then
                    level_limit = 50
                end
            end

            if level_limit >= 60 then
                cost_per_potion = 24000
                dust_per_potion = 3
            elseif level_limit >= 50 then
                cost_per_potion = 18000
                dust_per_potion = 3
            elseif level_limit >= 30 then
                cost_per_potion = 6000
                dust_per_potion = 2
            end
            local count = item.get_count()
            local cost = cost_per_potion * count
            local dust_count = dust_per_potion * count
            if x == 1 then -- 50%
                cost = math.floor(cost / 2)
                dust_count = math.ceil(dust_count / 2)
            end

            npc_name()
            say_split(translate.herbalism.extend_duration.take_item_info3)
            say_item_vnum(30360)
            local percent = {50, 100}
            say_split(string.format(translate.herbalism.extend_duration.take_item_info4, percent[x], dust_count, parse_number(cost)))
            say_reward(translate.herbalism.extend_duration.take_item_warning)
            say()
            if select(translate.general.yes, translate.general.no) == 2 then
                pc.set_crafting(false)
                return
            end

            if not check_safe_item(BUSY_CRAFTING) then return end

            if not pc.is_crafting() then
                npc_name()
                say(translate.general.wait_after_exchange)
                return
            end

            if pc.get_gold() < cost then
                npc_name()
                say(translate.general.no_gold)
                pc.set_crafting(false)
                return
            end

            if pc.count_item(30360) < dust_count then
                npc_name()
                say(translate.herbalism.extend_duration.no_dust)
                pc.set_crafting(false)
                return
            end

            npc_name()
            say_split(translate.herbalism.extend_duration.extend_success)
            pc.change_money(-cost)
            pc.remove_item(30360, dust_count)
            item.set_socket(0, percent[x])
            pc.set_crafting(false)
        end
    end
end