return {
	["limit_mol"] = "界限突破-手杀",
	
	["mol_xiahoudun"] = "夏侯惇-手杀",
	["&mol_xiahoudun"] = "夏侯惇",
	["molqingjian"] = "清俭",
	[":molqingjian"] = "每回合限一次，当你于摸牌阶段外获得牌后，你可以将任意张手牌扣置于武将牌上；下个任意角色的回合结束时，你将这些牌交给其他角色，然后你以此法交出的牌大于一张，则你摸一张牌。",
	["@molqingjian-put"] = "你可以发动“清俭”将其中任意张牌置于武将牌上",
	["@molqingjian-give"] = "请将“清俭”牌分配给其他角色",

	["mol_zhoutai"] = "周泰-手杀",
	["&mol_zhoutai"] = "周泰",
	["molfenji"] = "奋激",
	[":molfenji"] = "一名角色的回合结束后，若其没有手牌，你可以令其摸两张牌，然后你失去1点体力。",
	
	["mol_dongzhuo"] = "董卓-手杀",
	["&mol_dongzhuo"] = "董卓",
	["moljiuchi"] = "酒池",
	[":moljiuchi"] = "当你需要使用【酒】时，你可使用对应的实体牌为你的一张黑桃手牌的【酒】。你使用因【酒】生效而伤害值基数+1的【杀】造成伤害后，本回合“崩坏”失效。",
	["molbaonue"] = "暴虐",
	[":molbaonue"] = "主公技，当其他角色造成伤害后，若其为群势力角色，你令其选择：1.可判定▶若结果为黑桃，你回复1点体力；2.弃1枚“虐”▶你获得1枚“凌”。",
	
	["mol_dengai"] = "邓艾-手杀",
	["&mol_dengai"] = "邓艾",
	["moltuntian"] = "屯田",
	[":moltuntian"] = "当你于回合外失去牌后，你可以进行判定，若为红桃，你获得判定牌，否则你将判定牌置于你的武将牌上，称为“田”；你计算与其他角色的距离-X（X为“田”的数量）。",
	["#moltuntian-dist"] = "屯田",
	["molfield"] = "田",
	["molzaoxian"] = "凿险",
	[":molzaoxian"] = "觉醒技，准备阶段开始时，若“田”的数量不小于3，你减1点体力上限，然后获得技能“急袭”（你可以将一张“田”当【顺手牵羊】使用）。",
	["moljixi"] = "急袭",
	[":moljixi"] = "你可以将一张“田”当【顺手牵羊】使用。",
	
	["mol_liushan"] = "刘禅-手杀",
	["&mol_liushan"] = "刘禅",
	["molfangquan"] = "放权",
	["molfangquanask"] = "放权",
	["#molfangquan-next"] = "放权",
	[":molfangquan"] = "你可以跳过出牌阶段。若如此做，本回合手牌上限等于你的体力上限，且本回合结束时，你可以弃置一张手牌并选择一名其他角色，然后令其获得一个额外的回合。",
	["@molfangquan-give"] = "你可以弃置一张手牌令一名其他角色进行一个额外的回合",
	["molruoyu"] = "若愚",
	[":molruoyu"] = "主公技，觉醒技，准备阶段开始时，若你是体力值最小的角色（或之一），你加1点体力上限，回复1点体力，然后获得技能“激将”。",
	["$MOLRuoyuAnimate"] = "image=image/animate/ruoyu.png",
	["#MOLFangquan"] = "%to 将进行一个额外的回合",
	["#MOLRuoyuWake"] = "%from 的体力值 %arg 为场上最少，触发“%arg2”觉醒",
	
	["mol_sunce"] = "孙策-手杀",
	["&mol_sunce"] = "孙策",
	["molhunzi"] = "魂姿",
	[":molhunzi"] = "觉醒技，准备阶段开始时，若你的体力值不大于2，你减1点体力上限，然后获得技能“英姿”和“英魂”。",
	["molzhiba"] = "制霸",
	["molzhiba_pindian"] = "制霸",
	[":molzhiba"] = "主公技，其他吴势力角色的出牌阶段限一次，该角色可以与你拼点，若你已觉醒，则你可以拒绝此拼点，若其没赢，你可以获得此次拼点的两张牌。",
	["@molzhiba-pindianstart"] = "是否接受 %src 发起的拼点（制霸）",
	["@molzhiba-pindianfinish"] = "制霸：是否获两张拼点牌",
	["#ZhibaReject"] = "%from 拒绝 %to 发起的拼点（%arg）",
}