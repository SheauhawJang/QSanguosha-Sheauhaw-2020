extension = sgs.Package("newlua", sgs.Package_GeneralPack)
benwei = sgs.General(extension, "benwei", "qun", 4, true, true)

LuaCishan = sgs.CreateTriggerSkill {
	name = "cishan",
	--priority = 3,
	frequency = sgs.Skill_Compulsory,
	events = {sgs.Damaged},
	can_trigger_list = function(self, event, player, data)
		local list = {}
		local damage = data:toDamage()
		if damage.to:hasSkill(self:objectName()) then
			for _,p in sgs.qlist(player:getRoom():getAlivePlayers()) do
				for i = 1, damage.damage do
					table.insert(list, p)
				end
			end
		end
		return list
	end,
	on_trigger_effect = function(self, event, player, target, data)
		room:sendCompulsoryTriggerLog(player, self:objectName())
		room:broadcastSkillInvoke(self:objectName())
		target:drawCards(2)
	end
}

LuaDuguai = sgs.CreateTriggerSkill {
	name = "duguai",
	--priority = 3,
	frequency = sgs.Skill_Compulsory,
	events = {sgs.Damaged},
	can_trigger_list = function(self, event, player, data)
		local list = {}
		for _,p in sgs.qlist(player:getRoom():getAlivePlayers()) do
			if p:hasSkill(self:objectName()) then
				table.insert(list, p)
			end
		end
		return list
	end,
	on_trigger_effect = function(self, event, player, target, data, room)
		room:sendCompulsoryTriggerLog(target, self:objectName())
		room:broadcastSkillInvoke(self:objectName())
		target:drawCards(2)
		room:askForDiscard(target, self:objectName(), 1, 1)
		if (data:toDamage().from ~= target) then
			room:askForDiscard(target, self:objectName(), 1, 1)
		end
	end
}

benwei:addSkill(LuaCishan)
benwei:addSkill(LuaDuguai)

--[[
LuaYiji = sgs.CreateTriggerSkill {
	name = "luayiji",
	frequency = sgs.Skill_Compulsory,
	events = {sgs.Damaged},
	on_trigger = function(self, event, player, data)
		player:drawCards(2)
	end
}
benwei:addSkill(LuaYiji)


LuaLingxiu = sgs.CreateTriggerSkill {
	name = "n_lingxiu",
	frequency = sgs.Skill_Compulsory,
	priority = 3,
	events = {sgs.CardsMoveOneTime},
	can_trigger = function(self, target)
		return true
	end,
	on_trigger = function(self, event, player, data)
		local move = data:toMoveOneTime()
		player:speak("1")
		local room = player:getRoom()
		player:speak("2")
		if true then
			player:speak("3")
			if move.to:objectName() == player:objectName() then
				player:speak("4")
				if move.to_place == sgs.Player_PlaceHand then
					player:speak("5")
					if not room:getTag("FirstRound"):toBool() then
						player:drawCards(1)
					end
				end
			end
		end
		return false
	end
}
]]--

king = sgs.General(extension, "wangbadaxian", "god", 20, true, true)

LuaXianji = sgs.CreateTriggerSkill {
	name = "luaxianji",
	events = {sgs.Damage},
	can_trigger_list = function(self, event, player, data)
		local list = {}
		local damage = data:toDamage()
		if player:hasSkill(self:objectName()) and damage.to:objectName() ~= player:objectName() then
			table.insert(list, player)
		end
		return list
	end,
	on_trigger = function(self, event, player, data, room)
		if room:askForSkillInvoke(player, self:objectName()) then
			local damage = sgs.DamageStruct()
			damage.from = player
			damage.to = player
			damage.damage = 1
			room:damage(damage)
			player:drawCards(2 + player:getLostHp())
		end
	end
}

LuaChaiqian = sgs.CreateViewAsSkill{
	name = "luachaiqian", 
	n = 1, 
	view_filter = function(self, selected, to_selected)
		return true
	end, 
	view_as = function(self, cards) 
		if #cards == 0 then--一张卡牌也没选是不能发动技能的
			return nil--直接返回，nil表示无效
		elseif #cards == 1 then--选择了一张卡牌
			local card = cards[1]--获得发动技能的卡牌
			local suit = card:getSuit()--卡牌的花色
			local point = card:getNumber()--卡牌的点数
			local id = card:getId()--卡牌的编号
			--创建一张虚构的（被视作的）决斗卡牌
			local vs_card = sgs.Sanguosha:cloneCard("dismantlement", suit, point)
			--描述虚构决斗卡牌的构成
			vs_card:addSubcard(id)--用被选择的卡牌填充虚构卡牌
			vs_card:setSkillName(self:objectName())--创建虚构卡牌的技能名称
			return vs_card--返回一张虚构的决斗卡牌
		end
	end, 
}

LuaJiefu = sgs.CreateViewAsSkill{
	name = "luajiefu", 
	n = 1, 
	view_filter = function(self, selected, to_selected)
		return true
	end, 
	view_as = function(self, cards) 
		if #cards == 0 then--一张卡牌也没选是不能发动技能的
			return nil--直接返回，nil表示无效
		elseif #cards == 1 then--选择了一张卡牌
			local card = cards[1]--获得发动技能的卡牌
			local suit = card:getSuit()--卡牌的花色
			local point = card:getNumber()--卡牌的点数
			local id = card:getId()--卡牌的编号
			--创建一张虚构的（被视作的）决斗卡牌
			local vs_card = sgs.Sanguosha:cloneCard("snatch", suit, point)
			--描述虚构决斗卡牌的构成
			vs_card:addSubcard(id)--用被选择的卡牌填充虚构卡牌
			vs_card:setSkillName(self:objectName())--创建虚构卡牌的技能名称
			return vs_card--返回一张虚构的决斗卡牌
		end
	end, 
}
king:addSkill(LuaXianji)
king:addSkill(LuaChaiqian)
king:addSkill(LuaJiefu)
king:addSkill("qiaobian")
king:addSkill("guzheng")
king:addSkill("zhijian")
king:addSkill("rende")
king:addSkill("wusheng")
king:addSkill("paoxiao")
king:addSkill("ollongdan")
king:addSkill("liegong")
king:addSkill("tieji")
king:addSkill("guicai")
king:addSkill("noslianying")

sgs.LoadTranslationTable{
	["newlua"] = "接口样例",
	["cishan"] = "慈善",
	[":cishan"] = "锁定技。每当你受到1点伤害后，所有角色摸2张牌。",
	["duguai"] = "赌怪",
	[":duguai"] = "锁定技。一名角色受到伤害后，你摸2张牌，然后弃1张牌，若伤害来源不为你，则你再弃1张牌。",
	["benwei"] = "卢本伟",
	["#benwei"] = "慈善赌怪",
	["designer:benwei"] = "接口样例",
	["wangbadaxian"] = "王八大仙",
	["luaxianji"] = "献祭",
	[":luaxianji"] = "你对一名其他角色造成伤害后，你可以对自己造成一点伤害，之后摸2+X张牌。（X为你已损失的体力值）",
	["luachaiqian"] = "拆迁",
	[":luachaiqian"] = "你可以将一张牌当做【过河拆桥】使用。",
	["luajiefu"] = "劫富",
	[":luajiefu"] = "你可以将一张牌当做【顺手牵羊】使用。",
}

return extension