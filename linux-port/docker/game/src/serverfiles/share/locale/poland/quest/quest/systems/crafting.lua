quest crafting_manage begin
    state start begin
        when write with mes.get_message() == "craft" begin
            local args = mes.get_arguments()
            if table.getn(args) < 1 then
                return
            end

            local command = args[1]
            -- if command == "request" then
                
            --     local is_item = false
            --     if table.getn(args) > 1 then
            --         is_item = args[2] == "item"
            --     end
            --     if is_item then
            --         crafting.send_recipes()
            --     end
            --     crafting.send_avail()
            if command == "create" then
                if table.getn(args) < 3 then
                    return
                end
                local vnum = tonumber(args[2])
                local quantity = tonumber(args[3])
                if quantity then
                    quantity = minmax(1, quantity, 99)
                else
                    quantity = 1
                end
                if vnum then
                    crafting.create(vnum, quantity)
                end
            elseif command == "close" then
                crafting.close()
            end
        end

        when 20016.chat.translate.crafting.npc_chat begin
            crafting.open(CRAFTING_BLACKSMITH)
            setskin(NOWINDOW)
        end

        when 20008.chat.translate.crafting.npc_chat begin
            npc_name()
            say_split(translate.crafting.npc.chef.talk1)
            wait()
            crafting.open(CRAFTING_CHEF)
            setskin(NOWINDOW)
        end

        when 20001.chat.translate.magic_ore.crafting begin
            npc_name()
            say_split(translate.magic_ore.chat)
            wait()
            crafting.open(CRAFTING_ALCHEMIST_ORE)
            setskin(NOWINDOW)
        end

        when 20090.chat.translate.crafting.npc_chat begin
            npc_name()
            say_split(translate.crafting.npc.halfdemon.talk1)
            wait()
            crafting.open(CRAFTING_HALFDEMON)
            setskin(NOWINDOW)
        end
        
        when 11001.chat.translate.crafting.npc_chat_arrow
        or   11003.chat.translate.crafting.npc_chat_arrow
        or   11005.chat.translate.crafting.npc_chat_arrow begin
            crafting.open(CRAFTING_ARROW)
            setskin(NOWINDOW)
        end

        when 20364.chat.translate.crafting.npc_chat begin
            crafting.open(CRAFTING_NAKAJIMA)
            setskin(NOWINDOW)
        end

        when 20041.chat.translate.crafting.npc.bum.chat begin
            npc_name()
            say_split(translate.crafting.npc.bum.talk1)
            say_split(translate.crafting.npc.bum.talk2)
            say_split(translate.crafting.npc.bum.talk3)
            say()
            wait()
            crafting.open(CRAFTING_BUM)
            setskin(NOWINDOW)
        end
    end
end