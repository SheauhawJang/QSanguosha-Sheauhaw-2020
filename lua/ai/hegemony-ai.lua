local duoshi_skill = {}
duoshi_skill.name = "duoshi"
table.insert(sgs.ai_skills, duoshi_skill)
duoshi_skill.getTurnUseCard = function(self, inclusive)
	if self.player:usedTimes("DuoshiCard") >= 4 then return end
	if sgs.turncount <= 1 and #self.friends_noself == 0 and not self:isWeak() and self:getOverflow() <= 0 then return end
	local cards = self.player:getCards("h")
	cards = sgs.QList2Table(cards)

	local red_card
	if self.player:getHandcardNum() <= 2 then return end
	if self:needBear() then return end
	self:sortByUseValue(cards, true)

	for _, card in ipairs(cards) do
		if card:isRed() then
			local shouldUse = true
			if card:isKindOf("Slash") then
				local dummy_use = { isDummy = true }
				if self:getCardsNum("Slash") == 1 then
					self:useBasicCard(card, dummy_use)
					if dummy_use.card then shouldUse = false end
				end
			end

			if self:getUseValue(card) > sgs.ai_use_value.DuoshiCard and card:isKindOf("TrickCard") then
				local dummy_use = { isDummy = true }
				self:useTrickCard(card, dummy_use)
				if dummy_use.card then shouldUse = false end
			end

			if shouldUse and not card:isKindOf("Peach") then
				red_card = card
				break
			end

		end
	end

	if red_card then
		local card_id = red_card:getEffectiveId()
		local card_str = ("@DuoshiCard=" .. card_id)
		local await = sgs.Card_Parse(card_str)
		assert(await)
		return await
	end
end

sgs.ai_skill_use_func.DuoshiCard = function(card, use, self)
	use.card = card
	if use.to then use.to:append(self.player) end
	for _, player in ipairs(self.friends) do
		if use.to and not player:hasSkill("manjuan") and player:objectName() ~= self.player:objectName() then
			use.to:append(player)
		end
	end
	for _, enemy in ipairs(self.enemies) do
		if use.to and enemy:hasSkill("manjuan") then
			use.to:append(enemy)
		end
	end
end

sgs.ai_use_value.DuoshiCard = 3
sgs.ai_use_priority.DuoshiCard = 2.2
sgs.ai_card_intention.DuoshiCard = function(self, card, from, tos, source)
	for _, to in ipairs(tos) do
		sgs.updateIntention(from, to, to:hasSkill("manjuan") and 50 or -50)
	end
end

sgs.ai_skill_askforyiji.lirang = function(self, card_ids)
	local Shenfen_user
	for _, player in sgs.qlist(self.room:getAllPlayers()) do
		if player:hasFlag("ShenfenUsing") then
			Shenfen_user = player
			break
		end
	end

	self:updatePlayers()
	local available_friends = {}
	for _, friend in ipairs(self.friends_noself) do
		local insert = true
		if insert and hasManjuanEffect(friend) then insert = false end
		if insert and friend:hasFlag("DimengTarget") then
			local another
			for _, p in sgs.qlist(self.room:getOtherPlayers(friend)) do
				if p:hasFlag("DimengTarget") then
					another = p
					break
				end
			end
			if not another or not self:isFriend(another) then insert = false end
		end
		if insert and Shenfen_user and friend:objectName() ~= Shenfen_user:objectName() and friend:getHandcardNum() < 4 then insert = false end
		if insert and self:isLihunTarget(friend) then insert = false end
		if insert then table.insert(available_friends, friend) end
	end

	local cards = {}
	for _, card_id in ipairs(card_ids) do
		table.insert(cards, sgs.Sanguosha:getCard(card_id))
	end
	local id = card_ids[1]

	local card, friend = self:getCardNeedPlayer(cards)
	if card and friend and table.contains(available_friends, friend) then return friend, card:getId() end
	if #available_friends > 0 then
		self:sort(available_friends, "handcard")
		if Shenfen_user and table.contains(available_friends, Shenfen_user) then
			return Shenfen_user, id
		end
		for _, afriend in ipairs(available_friends) do
			if not self:needKongcheng(afriend, true) then
				return afriend, id
			end
		end
		self:sort(available_friends, "defense")
		return available_friends[1], id
	end
	return nil, -1
end

sgs.ai_skill_playerchosen.sijian = function(self, targets)
	return self:findPlayerToDiscard()
end

sgs.ai_playerchosen_intention.sijian = function(self, from, to)
	local intention = 80
	if (to:hasSkill("kongcheng") and to:getHandcardNum() == 1) or self:needToThrowArmor(to) then
		intention = 0
	end
	sgs.updateIntention(from, to, intention)
end

sgs.ai_skill_invoke.suishi = function(self, data)
	local promptlist = data:toString():split(":")
	local effect = promptlist[1]
	local tianfeng = findPlayerByObjectName(self.room, promptlist[2])
	if effect == "draw" then
		return tianfeng and self:isFriend(tianfeng)
	elseif effect == "losehp" then
		return tianfeng and self:isEnemy(tianfeng)
	end
	return false
end

sgs.ai_skill_use["@@shuangren"] = function(self, prompt)
	if self.player:isKongcheng() then return "." end
	self:sort(self.enemies, "handcard")
	local max_card = self:getMaxCard()
	local max_point = max_card:getNumber()

	local slash = sgs.Sanguosha:cloneCard("slash")
	local dummy_use = { isDummy = true }
	self.player:setFlags("slashNoDistanceLimit")
	self:useBasicCard(slash, dummy_use)
	self.player:setFlags("-slashNoDistanceLimit")

	if dummy_use.card then
		for _, enemy in ipairs(self.enemies) do
			if not (enemy:hasSkill("kongcheng") and enemy:getHandcardNum() == 1) and not enemy:isKongcheng() then
				local enemy_max_card = self:getMaxCard(enemy)
				local enemy_max_point = enemy_max_card and enemy_max_card:getNumber() or 100
				if max_point > enemy_max_point then
					self.shuangren_card = max_card:getEffectiveId()
					return "@ShuangrenCard=.->" .. enemy:objectName()
				end
			end
		end
		for _, enemy in ipairs(self.enemies) do
			if not (enemy:hasSkill("kongcheng") and enemy:getHandcardNum() == 1) and not enemy:isKongcheng() then
				if max_point >= 10 then
					self.shuangren_card = max_card:getEffectiveId()
					return "@ShuangrenCard=.->" .. enemy:objectName()
				end
			end
		end
		if #self.enemies < 1 then return end
		self:sort(self.friends_noself, "handcard")
		for index = #self.friends_noself, 1, -1 do
			local friend = self.friends_noself[index]
			if not friend:isKongcheng() then
				local friend_min_card = self:getMinCard(friend)
				local friend_min_point = friend_min_card and friend_min_card:getNumber() or 100
				if max_point > friend_min_point then
					self.shuangren_card = max_card:getEffectiveId()
					return "@ShuangrenCard=.->" .. friend:objectName()
				end
			end
		end

		local zhugeliang = self.room:findPlayerBySkillName("kongcheng")
		if zhugeliang and self:isFriend(zhugeliang) and zhugeliang:getHandcardNum() == 1 and zhugeliang:objectName() ~= self.player:objectName() then
			if max_point >= 7 then
				self.shuangren_card = max_card:getEffectiveId()
				return "@ShuangrenCard=.->" .. zhugeliang:objectName()
			end
		end

		for index = #self.friends_noself, 1, -1 do
			local friend = self.friends_noself[index]
			if not friend:isKongcheng() then
				if max_point >= 7 then
					self.shuangren_card = max_card:getEffectiveId()
					return "@ShuangrenCard=.->" .. friend:objectName()
				end
			end
		end
	end
	return "."
end

function sgs.ai_skill_pindian.shuangren(minusecard, self, requestor)
	local maxcard = self:getMaxCard()
	return self:isFriend(requestor) and self:getMinCard() or (maxcard:getNumber() < 6 and minusecard or maxcard)
end


sgs.ai_skill_playerchosen.shuangren = sgs.ai_skill_playerchosen.zero_card_as_slash
sgs.ai_card_intention.ShuangrenCard = sgs.ai_card_intention.TianyiCard
sgs.ai_cardneed.shuangren = sgs.ai_cardneed.bignumber

xiongyi_skill = {}
xiongyi_skill.name = "xiongyi"
table.insert(sgs.ai_skills, xiongyi_skill)
xiongyi_skill.getTurnUseCard = function(self)
	if self.player:getMark("@arise") < 1 then return end
	if (#self.friends <= #self.enemies and sgs.turncount > 2 and self.player:getLostHp() > 0) or (sgs.turncount > 1 and self:isWeak()) then
		return sgs.Card_Parse("@XiongyiCard=.")
	end
end

sgs.ai_skill_use_func.XiongyiCard = function(card, use, self)
	use.card = card
	for i = 1, #self.friends do
		if use.to then use.to:append(self.friends[i]) end
	end
end

sgs.ai_card_intention.XiongyiCard = -80
sgs.ai_use_priority.XiongyiCard = 9.31

local qingcheng_skill = {}
qingcheng_skill.name = "qingcheng"
table.insert(sgs.ai_skills, qingcheng_skill)
qingcheng_skill.getTurnUseCard = function(self, inclusive)
	local equipcard
	if self:needBear() then return end
	if self:needToThrowArmor() then
		equipcard = self.player:getArmor()
	else
		for _, card in sgs.qlist(self.player:getHandcards()) do
			if card:isKindOf("EquipCard") then
				equipcard = card
				break
			end
		end
		if not equipcard then
			for _, card in sgs.qlist(self.player:getCards("he")) do
				if card:isKindOf("EquipCard") and not card:isKindOf("Armor") and not card:isKindOf("DefensiveHorse") then
					equipcard = card
				end
			end
		end
	end

	if equipcard then
		local card_id = equipcard:getEffectiveId()
		local card_str = ("@QingchengCard=" .. card_id)
		local qc_card = sgs.Card_Parse(card_str)

		return qc_card
	end
end

sgs.ai_skill_use_func.QingchengCard = function(card, use, self)
	if self.room:alivePlayerCount() == 2 then
		local only_enemy = self.room:getOtherPlayers(self.player):first()
		if only_enemy:getLostHp() < 3 then return end
	end
	local target
	self:sort(self.enemies, "hp")
	for _, enemy in ipairs(self.enemies) do
		if self:getFriendNumBySeat(self.player, enemy) > 1 then
			if enemy:getHp() < 1 and enemy:hasSkill("nosbuqu", true) and enemy:getMark("Qingchengnosbuqu") == 0 then
				target = enemy
				break
			end
			if self:isWeak(enemy) then
				for _, askill in ipairs((sgs.exclusive_skill .. "|" .. sgs.save_skill):split("|")) do
					if enemy:hasSkill(askill, true) and enemy:getMark("Qingcheng" .. askill) == 0 then
						target = enemy
						break
					end
				end
				if target then break end
			end
			for _, askill in ipairs(("noswuyan|weimu|wuyan|guixin|fenyong|liuli|yiji|jieming|neoganglie|fankui|fangzhu|enyuan|nosenyuan|" ..
						"vsganglie|ganglie|langgu|qingguo|luoying|guzheng|jianxiong|longdan|xiangle|renwang|huangen|tianming|yizhong|bazhen|jijiu|" ..
						"beige|longhun|gushou|buyi|mingzhe|danlao|qianxun|jiang|yanzheng|juxiang|huoshou|anxian|zhichi|feiying|" ..
						"tianxiang|xiaoji|xuanfeng|nosxuanfeng|xiaoguo|guhuo|guidao|guicai|nosshangshi|lianying|sijian|mingshi|" ..
						"yicong|zhiyu|lirang|xingshang|shushen|shangshi|leiji|wusheng|wushuang|tuntian|quanji|kongcheng|jieyuan|" ..
						"jilve|wuhun|kuangbao|tongxin|shenjun|ytchengxiang|sizhan|toudu|xiliang|tanlan|shien"):split("|")) do
				if enemy:hasSkill(askill, true) and enemy:getMark("Qingcheng" .. askill) == 0 then
					target = enemy
					break
				end
			end
			if target then break end
		end
	end
	if not target then
		for _, friend in ipairs(self.friends_noself) do
			if friend:hasSkill("shiyong", true) and friend:getMark("Qingchengshiyong") == 0 then
				target = friend
				break
			end
		end
	end

	if not target then return end
	use.card = card
	if use.to then
		use.to:append(target)
	end
	return
end

sgs.ai_skill_choice.qingcheng = function(self, choices, data)
	local target = data:toPlayer()
	if self:isFriend(target) then
		if target:hasSkill("shiyong", true) and target:getMark("Qingchengshiyong") == 0 then return "shiyong" end
	end
	if target:getHp() < 1 and target:hasSkill("buqu", true) and target:getMark("Qingchengbuqu") == 0 then return "buqu" end
	if self:isWeak(target) then
		for _, askill in ipairs((sgs.exclusive_skill .. "|" .. sgs.save_skill):split("|")) do
			if target:hasSkill(askill, true) and target:getMark("Qingcheng" .. askill) == 0 then
				return askill
			end
		end
	end
	for _, askill in ipairs(("noswuyan|weimu|wuyan|guixin|fenyong|liuli|yiji|jieming|neoganglie|fankui|fangzhu|enyuan|nosenyuan|" ..
						"ganglie|vsganglie|langgu|qingguo|luoying|guzheng|jianxiong|longdan|xiangle|renwang|huangen|tianming|yizhong|bazhen|jijiu|" ..
						"beige|longhun|gushou|buyi|mingzhe|danlao|qianxun|jiang|yanzheng|juxiang|huoshou|anxian|zhichi|feiying|" ..
						"tianxiang|xiaoji|xuanfeng|nosxuanfeng|xiaoguo|guhuo|guidao|guicai|nosshangshi|lianying|sijian|mingshi|" ..
						"yicong|zhiyu|lirang|xingshang|shushen|shangshi|leiji|wusheng|wushuang|tuntian|quanji|kongcheng|jieyuan|" ..
						"jilve|wuhun|kuangbao|tongxin|shenjun|ytchengxiang|sizhan|toudu|xiliang|tanlan|shien"):split("|")) do
		if target:hasSkill(askill, true) and target:getMark("Qingcheng" .. askill) == 0 then
			return askill
		end
	end
end


sgs.ai_use_value.QingchengCard = 2
sgs.ai_use_priority.QingchengCard = 7.2
sgs.ai_card_intention.QingchengCard = 0

sgs.ai_choicemade_filter.skillChoice.qingcheng = function(self, player, promptlist)
	local choice = promptlist[#promptlist]
	local target = nil
	for _, p in sgs.qlist(self.room:getOtherPlayers(player)) do
		if p:hasSkill(choice, true) then
			target = p
			break
		end
	end
	if not target then return end
	if choice == "shiyong" then sgs.updateIntention(player, target, -10) else sgs.updateIntention(player, target, 10) end
end

sgs.ai_skill_invoke.cv_caopi = function(self, data)
	if math.random(0, 2) == 0 then return true end
	return false
end

sgs.ai_skill_invoke.cv_zhugeliang = function(self, data)
	if math.random(0, 2) > 0 then return false end
	if math.random(0, 4) == 0 then sgs.ai_skill_choice.cv_zhugeliang = "tw_zhugeliang" return true
	else sgs.ai_skill_choice.cv_zhugeliang = "heg_zhugeliang" return true end
end

sgs.ai_skill_invoke.cv_nos_huangyueying = function(self, data)
	if math.random(0, 2) > 0 then return false end
	if math.random(0, 4) == 0 then sgs.ai_skill_choice.cv_nos_huangyueying = "tw_huangyueying" return true
	else sgs.ai_skill_choice.cv_nos_huangyueying = "heg_huangyueying" return true end
end
