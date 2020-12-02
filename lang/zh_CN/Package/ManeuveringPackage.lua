-- translation for ManeuveringPackage

return {
	["maneuvering"] = "军争篇",

	["fire_slash"] = "火杀",
	[":fire_slash"] = "基本牌\n\n使用时机：出牌阶段限一次。\n使用目标：你攻击范围内的一名角色。\n作用效果：你对目标角色造成1点火焰伤害。",

	["thunder_slash"] = "雷杀",
	[":thunder_slash"] = "基本牌\n\n使用时机：出牌阶段限一次。\n使用目标：你攻击范围内的一名角色。\n作用效果：你对目标角色造成1点雷电伤害。",

	["analeptic"] = "酒",
	[":analeptic"] = "基本牌\n\n使用方法Ⅰ：\n使用时机：出牌阶段（每名角色的回合限一次）。\n使用目标：包括你在内的一名角色。\n作用效果：目标角色于此回合内使用的下一张【杀】的伤害值基数+1。"
	.."\n\n使用方法Ⅱ：\n使用时机：当你处于濒死状态时。\n使用目标：你。\n作用效果：你回复1点体力。",
	["#UnsetDrankEndOfTurn"] = "%from 回合结束，%to 的【<font color=\"yellow\"><b>酒</b></font>】效果消失",

	["fan"] = "朱雀羽扇",
	[":fan"] = "装备牌·武器\n\n攻击范围：4\n技能：当你使用普【杀】时，你可将此【杀】改为火【杀】。",

	["guding_blade"] = "古锭刀",
	[":guding_blade"] = "装备牌·武器\n\n攻击范围：4\n技能：锁定技，当你使用【杀】对目标角色造成伤害时，若其没有手牌，你令伤害值+1。",
	["#GudingBladeEffect"] = "%from 的【<font color=\"yellow\"><b>古锭刀</b></font>】效果被触发， %to 没有手牌，伤害从 %arg 增加至 %arg2",

	["vine"] = "藤甲",
	[":vine"] = "装备牌·防具\n\n技能：\n锁定技，①当【南蛮入侵】、【万箭齐发】或普【杀】对目标的使用结算开始时，若此目标对应的角色为你，你令此牌对此目标无效。②当你受到火焰伤害时，你令伤害值+1。",
	["#VineDamage"] = "%from 的防具【<font color=\"yellow\"><b>藤甲</b></font>】效果被触发，火焰伤害由 %arg 点增加至 %arg2 点",

	["silver_lion"] = "白银狮子",
	[":silver_lion"] = "装备牌·防具\n\n技能：\n锁定技，当你受到伤害时，若伤害值大于1，你将伤害值改为1。\n锁定技，当你失去装备区里的【白银狮子】前，你令你于失去此牌后回复1点体力。",
	["#SilverLion"] = "%from 的防具【%arg2】防止了 %arg 点伤害，减至 <font color=\"yellow\"><b>1</b></font> 点",

	["fire_attack"] = "火攻",
	[":fire_attack"] = "普通锦囊牌\n\n使用时机：出牌阶段。\n使用目标：一名有手牌的角色。\n作用效果：目标角色展示一张手牌，然后你可弃置与之花色相同的一张手牌，若如此做，其受到你造成的1点火焰伤害。",
	["@fire-attack"] = "%src 展示的牌的花色为 %arg，请弃置一张与其花色相同的手牌",

	["iron_chain"] = "铁索连环",
	[":iron_chain"] = "普通锦囊牌\n\n使用时机：出牌阶段。\n使用目标：一至两名角色。\n作用效果：目标角色选择一项：1.横置；2. 重置。\n◆你能重铸此牌。",

	["supply_shortage"] = "兵粮寸断",
	[":supply_shortage"] = "延时锦囊牌\n\n使用时机：出牌阶段。\n使用目标：距离为1的一名其他角色。\n作用效果：目标角色判定，若结果不为梅花，其跳过摸牌阶段。",

	["hualiu"] = "骅骝",
}
