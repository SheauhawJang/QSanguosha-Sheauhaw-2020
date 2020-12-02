--Ȼ�����������ûʲô���ã����Բ�������С�_��
local function CanChangeJudgeCard(self, player)
	if player:hasSkill("guidao") then
	    if player:isKongcheng() then
		    for _, equip in sgs.qlist(player:getEquips()) do
				if equip:getSuit() == sgs.Card_Club then return true end
			end
		else
		    return player:getHandcardNum() > 2
		end
	elseif player:hasSkill("guicai") then
	    if player:isKongcheng() then
		    for _, equip in sgs.qlist(player:getEquips()) do
				if equip:getSuit() ~= sgs.Card_Spade then return true end
			end
		else
		    return true
		end
	end
end
--��ʣ��ж���
sgs.ai_skill_invoke.lutiandu = function(self, data)
	local effect = data:toCardEffect()
	if effect.card:getSuit() == sgs.Card_Spade then
	    return not (self:isWeak() and getCardsNum("Peach", self.player) == 0)
	else
	    local shengun
		--�ҵ����һ�����е���
	    for _, p in sgs.qlist(self.room:getAllPlayers()) do
		    if CanChangeJudgeCard(self, p) then
			    shengun = p
			end
		end
		if shengun and self:isEnemy(shengun) then return false end
	end
	--���ˣ��������һ�ɷ���
	return true
end
--��ʣ�ѡ����á��ơ��Ľ�ɫ��
sgs.ai_skill_playerchosen.lutiandu = function(self, targets)
	local id = self.player:getMark("tiandu_judge")
	local card = sgs.Sanguosha:getCard(id)
	local cards = { card }
	local c, friend = self:getCardNeedPlayer(cards, self.friends)
	if friend then return friend end
	return self.player
end
--�żƣ����Է����
sgs.ai_skill_invoke.luyiji = true
--�żƣ����Ʋ��ֲο��żƾ����ˣ�һ���ŵطֺ��ˣ��������һ�𷢳�ȥ��
sgs.ai_skill_use["@@luyiji_give"] = function(self, data, method)
    local player = self.player
	local extra = self:getCardsNum("Jink") - player:getHp() - self:getCardsNum("Peach") > 0
	local id = tonumber(player:property("luyiji"):toString():split("+")[1])
	local card = sgs.Sanguosha:getCard(id)
	if card:isKindOf("Peach") or card:isKindOf("Analeptic") then return "." end
    if not extra and card:isKindOf("Jink") then return "." end
	local ids = {}
	table.insert(ids, id)
	local target, cardid = sgs.ai_skill_askforyiji.nosyiji(self, ids)
    return "#lvyiji_giveCard:" .. id .. ":->" .. target:objectName()
end














