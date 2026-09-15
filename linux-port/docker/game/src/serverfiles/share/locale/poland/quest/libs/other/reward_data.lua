reward_data = {}

reward_data.herbalism_onboarding = {
    gold = 15000, -- 12.5k krafting potki +2.5k nagroda
    exp = 10000, -- 130k potrzebne do lvl up na 15lvlu
    items = {
        {27103, 5} -- bonusowe 5 małych fioletowych potek
    }
}

--trza sie nalatać po pustyni i ubić Bera
reward_data.fishing_onboarding = {
    gold = 20000, --było 0, 20k to tyle co nic
    exp = 150000, --2kk do lvl up na 30lvlu
}

--zajeb 9 dzikich psów
reward_data.main_quest_lv2 = { 
    gold = 2500, --było 1k
    exp = 250, --800 do lvl up
    items = {
        {27001, 20}, 
        {27004, 10}, 
        -- zestaw 20 małych czerwonych potek i 10 niebieskich
        {30347, 1}, --Gourou, aby pokazać graczom nowe ulepki (quest na psy, a Gurou to mięso z psa)
    }
}

--"pójdź i podaj" książke dla starego
reward_data.main_quest_lv3 = { 
    gold = 5000, --bez zmian
    exp = 600, --1500 do lvl up, daje 600, było 450
    items = {
        {27001, 20},
        {27004, 10},
    }
}

--"pójdź i przynieś" nos świni
reward_data.main_quest_lv6 = { 
    gold = 7000, --więcej golda, mniej expa za taką krótką, prostą misje "pójdź i przynieś"
    exp = 3600, --7.2k do lvl up na 6lvlu; dałem 3.6k a było 6k
    items = {
        {30070, 1}, --futro wilka+
        {27001, 20},
        {27004, 10},
    }
}

--"pójdź i podaj" wędkę rybakowi, misja max 2 minuty
reward_data.main_quest_lv7 = { 
    gold = 8000, 
    items = {
        {27001, 20},
        {27004, 10},
    }
}

--"pójdź i pogadaj" ze strażnikiem
reward_data.main_quest_lv9 = { 
    exp = 8000, --24k do lvlup
    items = {
        {27001, 20},
        {27004, 10},
    }
}

--"pójdź i przynieś" do m2 i z powrotem m1
reward_data.main_quest_lv10 = {
    gold = 12000, --było 2k
    exp = 6000, --33k do lvlup
    items = {
        {27001, 20},
        {27004, 10},
    }
}

--zajeb 10 szarych alfa wilków (misja między 5min a 15min)
reward_data.main_quest_lv12 = {
    gold = 8000, --było 3.5k; 
    exp = 12000, --było 10k; 58k do lvlup
    items = {
        {16003, 1} --Drewniany naszyjnik +3 (regen staminki na 12lvlu)
    }
}

--Zajeb 1 metin bitwy (rather hard)
reward_data.main_quest_lv14 = {
    gold = 15000,
    exp = 25000, --100k do lvlup
    items = {
    {25051, 1}, --Gwarancja rzemiosła (zbroja) - czyli +1 w zbrojce/tarczy/hełmie za zbicie metka
    }
}

--"pójdź, zanieś" do m2 i z powrotem do m1
reward_data.main_quest_lv15 = {
    gold = 15000,
    exp = 30000, --130k do lvlup
    items = {
        {27001, 20},
        {27004, 10},
    }
}

--dropnij list z zaprzysów
reward_data.main_quest_lv16 = {
    gold = 15000, --było 5k
    exp = 40000, --169k do lvlup
    -- items = {
    --     {30027, 1},
    -- }
}

--zniszcz metek(25lv) Czerni 
reward_data.main_quest_lv27 = {
    gold = 55000, --było 15k
    exp = 420000, --1.418kk do lvlup
    items = {
        {30018, 1},
        {30031, 1}, --ulepki na eq do 25lvla
    }
}

-- dużo łażenia
reward_data.main_quest_lv30 = {
    gold = 40000, --było 20k
    exp = 820000, --2.122kk do lvlup; było 420k
}
reward_data.main_quest_lv32 = {}

-- 20 srebrnych mieczy i trochę łażenia 
reward_data.main_quest_lv32.teacher2 = {
    gold = 50000,
    exp = 1000000, --2.761kk do lvlup
    items = {
        {70012, 1},
    }
}

-- zabij elit. ork czarodziej, 5% szans na drop
reward_data.main_quest_lv32.teacher3 = {
    gold = 100000, --było 50k
    exp = 1250000, --2.761kk do lvlup
    items = {
        {70008, 10}, --10 białych flag, jak w oryginale
    }
}

reward_data.main_quest_lv40 = {}

--zabij elit. ezot fanatyk, 5% szans na drop
reward_data.main_quest_lv40.gototeacher1 = {
    gold = 75000, --było 50k
    exp = 2000000, --6.837kk do lvlup
    items = {
        --[[
        TA MISJA CZEKA NA SWOJĄ NAGRODĘ
            Tu chcemy wprowadzić funkcje, która daje graczowi potke z jego przedziału lvlowego
        
        TU BĘDZIE POTKA Z JEGO PRZEDZIAŁU LVLOWEGO

        old:
        {71018, 1}, --quest daje setke (nie wiem w jakiej formie będą setki więc warto zwrócić uwagę) 
        !!update: usuwamy setki z gry!!
        --]]
    }
}

--"pójdź i przynieś" oraz polataj po normalnym lochu małp
reward_data.main_quest_lv40.gototeacher2 = {
    gold = 100000, --było 50k
    exp = 2000000, --6.837kk do lvlup
    items = {
        --[[
        TA MISJA CZEKA NA SWOJĄ NAGRODĘ
        
        TU BĘDZIE POTKA Z JEGO PRZEDZIAŁU LVLOWEGO

        old:
        {71019, 1}, -- błogosławieństwo magii xDDDD
        --]]
    }
}

reward_data.main_quest_lv47 = {}

--"pójdź i przynieś" z lochu pająków v1
reward_data.main_quest_lv47.gototeacher3 = {
    gold = 50000,
    exp = 2000000, --12.320kk do lvlup
    items = {
        --{VNUM RECEPTURY ZIELONEJ POTKI, 1}
       
        --[[
        old:
        {71019, 1}, -- znowu błogosławieństwo magii xDDDD
        --]]
    }
}

--zabij pustynny zawadiaka, 5% szans na drop
reward_data.main_quest_lv47.gototeacher4 = {
    gold = 75000,
    exp = 3000000, --12.320kk do lvlup 
    items = {
        --1 LOSOWA RECEPTURA DLA JEGO PRZEDZIAŁU LVLOWEGO

        --[[
        old:
        {71020, 1}, -- błogosławieństwo smoka
        --]]
    }
}

--zabij wężowy miecznik/łucznik, 5% szans na drop
reward_data.main_quest_lv47.gototeacher5 = {
    gold = 100000, --było 75k
    exp = 3000000, --12.320kk do lvlup 
    items = {
        {70038, 20}
    }

    --na peelu dają pelerynki, imo spoko nagroda
}


reward_data.main_quest_lv50 = {}

reward_data.main_quest_lv50.gototeacher6 = {
    gold = 125000,
    exp = 5000000,
    items = {
        {70050, 1},
    }
}

reward_data.main_quest_lv50.gototeacher7 = {
    gold = 137500,
    exp = 5500000,
    items = {
        {70051, 1}, -- Rękawica Króla Przepowiedni
    }
}

reward_data.main_quest_lv50.gototeacher8 = {
    gold = 150000,
    exp = 6000000,
    items = {
        {71004, 1},
    }
}

reward_data.main_quest_lv55 = {}

reward_data.main_quest_lv55.gototeacher9 = {
    gold = 162500,
    exp = 6500000,
    -- kolejny farmazon xD gameforge.main_quest_lv55._180_sayReward = "Otrzymałeś 26.000 Punktów Doświadczenia.[ENTER]Otrzymałeś 15.000 Yang.[ENTER]Otrzymałeś Nos Świni. "
}

reward_data.main_quest_lv55.gototeacher10 = {
    gold = 175000,
    exp = 7000000,
    items = {
        {71012, 1}, --Książkę Lidera
    }
}

reward_data.main_quest_lv60 = {}

reward_data.main_quest_lv60.quest2_end = {
    gold = 125000,
    exp = 3300000,
    items = {
        {71094, 3},
        {30056, 1},
        {30025, 1},
    }
}

reward_data.main_quest_lv60.quest3_end2 = {
    gold = 175000,
    exp = 980000,
    items = {
        {71001, 10},
        {50721, 10},
        {50725, 10},
    }
}

reward_data.test_quest1 = {
    gold = 5000,
    exp = 1000,
    items = {
        {19, 1},
        {27001, 5}
    }
}

reward_data.new_quest_lv17 = {}

reward_data.new_quest_lv17.a = {
    gold = 50000,
    exp = 30000,
    items = {
        {27005, 200}
    }
}

reward_data.new_quest_lv17.b = {
    gold = 10000,
    exp = 40000,
    -- 	pc.give_item_from_special_item_group(50300)
}

reward_data.new_quest_lv19 = {
    gold = 20000,
    exp = 150000,
    items = {
        {51700, 2},
    }
}

reward_data.new_quest_lv22 = {}

reward_data.new_quest_lv22.fail = {
    gold = 25000,
    exp = 75000,
}

reward_data.new_quest_lv22.success = {
    gold = 50000,
    exp = 150000,
    items = {
        {15064, 1},
        {27105, 10},
        {27101, 5},
    }
}

reward_data.new_quest_lv24 = {}

reward_data.new_quest_lv24.fail = {
    gold = 30000,
    exp = 400000,
}

reward_data.new_quest_lv24.success = {
    gold = 30000,
    exp = 800000,
}

reward_data.new_quest_lv26 = {
    gold = 17500,
    exp = 35000,
    items = {
        {27868, 3}
    }
}

reward_data.new_quest_lv28 = {}

reward_data.new_quest_lv28.exp = {
    exp = 850000,
}

reward_data.new_quest_lv28.gold = {
    gold = 70000,
    exp = 400000,
}

reward_data.new_quest_lv29 = {}

reward_data.new_quest_lv29.fail = {
    gold = 9500,
    exp = 150000,
}

reward_data.new_quest_lv29.success = {
    gold = 38000,
    exp = 600000,
    -- aditional exp bonus (weird calculation: 820*4*3*1000/pc.getqf("dropProb") or 820*4*3*1000/pc.getqf("dropProb")/4) didnt tested yet
}

reward_data.new_quest_lv42 = {
    gold = 500000,
    exp = 750000,
    items = {
        {27868, 3}
    }
}

reward_data.new_quest_lv43 = {
    gold = 32000,
    exp = 1250000,
}


reward_data.new_quest_lv52 = {}

reward_data.new_quest_lv52.giveup = {
    gold = 25000,
    exp = 20000,
}

reward_data.new_quest_lv52.finish = {
    gold = 270000,
    exp = 3300000,
    items = {

    }
}

reward_data.new_quest_lv54 = {
    gold = 125000,
    exp = 3750000,
    items = {
        {13083, 1},
    }
}

reward_data.subquest_01 = {
    gold = 1000,
    exp = 1500,
}

reward_data.subquest_02 = {
    gold = 1500,
    exp = 1000
}

reward_data.subquest_03 = {
    gold = 5000,
    exp = 3000,
    items = {
        {30027, 1},
    }
}

reward_data.subquest_04 = {
    gold = 3000,
    exp = 4000,
}

reward_data.subquest_05 = {
    gold = 2000,
    exp = 6000,
}

reward_data.subquest_06 = {
    gold = 15000,
    exp = 26000,
    items = {
        {30003, 1}
        -- + one item based on class: "11223", "11423", "11623", "11823"
    }
}

reward_data.subquest_07 = {
    gold = 5000,
    exp = 12000,
    items = {
        {30028, 1},
    }
}
reward_data.subquest_08 = {
    gold = 10000,
    exp = 26000,
}

reward_data.subquest_09 = {
    gold = 10000,
    exp = 36000,
    items = {
        {30031, 1}
    }
}

reward_data.subquest_10 = {
    gold = 20000,
    exp = 60000,
    items = {
        {50083, 1},
    }
}

reward_data.subquest_11 = {
    gold = 5000,
    exp = 122000,
}

reward_data.subquest_12 = {
    gold = 20000,
    exp = 122000,
    items = {
        {14040, 1},
    }
}

reward_data.subquest_13 = {
    gold = 20000,
    exp = 122000,
    -- item depending on class: ("53", "1023", "53", "7043")
}

reward_data.subquest_15 = {
    gold = 25000,
    exp = 150000,
    items = {
        {16064, 1},
    }
}

reward_data.subquest_16 = {
    gold = 30000,
    exp = 150000,
    items = {
        {17060, 1},
    }
}

reward_data.subquest_17 = {
    gold = 10000,
    exp = 300000,
}

reward_data.subquest_18 = {
    gold = 10000,
    exp = 7000,
}

reward_data.subquest_19 = {
    exp = 15000,
    -- and one random item from bellow:
    -- 01,875% ("11204","11214","11404","11414","11604","11614","11804","11814","11203","11213","11403","11413","11603","11613","11803","11813","11202","11212","11402","11412","11602","11612","11802","11812","11201","11211","11401","11411","11601","11611","11801","11811")
    -- 04,125% ("11200","11210","11400","11410","11600","11610","11800","11810")
}


reward_data.subquest_20 = {
    gold = 20000,
    exp = 300000,
}

reward_data.subquest_21 = {
    gold = 30000,
    exp = 1900000,
}

reward_data.subquest_22 = {
    gold = 30000,
    exp = 400000,
}

reward_data.subquest_23 = {
    gold = 35000,
    exp = 600000,
}

reward_data.subquest_24 = {
    exp = 900000,
    gold = 25000,
}

reward_data.subquest_25 = {
    exp = 400000,
    gold = 12000,
}

reward_data.subquest_26 = {
    exp = 5000000,
    gold = 50000,
}


reward_data.subquest_27 = {
    exp = 850000,
    gold = 20000,
}

reward_data.subquest_30 = {
    exp = 200000,
    gold = 20000,
    -- give iem based on class: (00063, 00063, 00063, 07053)
}

reward_data.subquest_31 = {
    exp = 200000,
    gold = 20000,
    -- give iem based on class: (11233, 11433, 11633, 11833)
}

reward_data.subquest_32 = {
    exp = 1100000,
    items = {
        {14060, 1},
        {16060, 1},
        {17060, 1}
    }
}

reward_data.subquest_33 = {
    exp = 1100000,
    items = {
        {14060, 1},
        {16060, 1},
        {17060, 1}
    }
    -- give iem based on class: (12241, 12521, 12381, 12661)
}

reward_data.subquest_33 = {
    exp = 1400000,
    gold = 25000,
}

reward_data.subquest_34 = {
    exp = 1000000,
    items = {
        {71007, 1}
    }
}

reward_data.subquest_36 = {
    exp = 1800000,
} 

reward_data.subquest_36 = {
    exp = 1800000,
} 

reward_data.subquest_37 = {
    gold = 35000,
    exp = 2500000,
    items = {
        {15143, 1}
    }
} 

reward_data.subquest_38 = {
    gold = 35000,
    exp = 1500000,
    items = {
        {70038, 3}
    }
} 

reward_data.subquest_39 = {
    gold = 65000,
    exp = 4000000
} 

reward_data.subquest_40 = {
    gold = 50000,
    exp = 4500000,
    items = {
        {71044, 3},
        {71045, 3},
    }
} 

reward_data.subquest_41 = {
    gold = 20000,
    exp = 2100000,
}  

reward_data.subquest_42 = {
    gold = 30000,
    exp = 1700000,
    items = {
        {50621, 3},
    }
} 
reward_data.subquest_44 = {}

reward_data.subquest_44.lie1 = {
    gold = 20000,
    exp = 100000
} 

reward_data.subquest_44.lie2 = {
    gold = 50000,
    exp = 100000
}  

reward_data.subquest_44.truth = {
    gold = 20000,
    exp = 700000
}  
		
reward_data.subquest_45 = {
    gold = 70000,
    exp = 7000000,
    items = {
        {70003, 1}
    }
}  

reward_data.subquest_46 = {
    gold = 30000,
    exp = 3000000
}  

reward_data.subquest_48 = {
    gold = 20000,
    exp = 2300000
}               

reward_data.subquest_48 = {
    gold = 50000,
    exp = 5500000
}

reward_data.subquest_49 = {
    gold = 70000,
    exp = 6000000,
    items = {
        {71021, 1}
    }
}

reward_data.hwang_introduction = {
    items = {
        {72731, 1},
    },
}