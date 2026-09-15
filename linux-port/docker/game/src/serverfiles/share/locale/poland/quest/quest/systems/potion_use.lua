quest potion_use begin
    state start begin
        when 36.use_type begin
            potion_system.use_potion()
        end
    end
end