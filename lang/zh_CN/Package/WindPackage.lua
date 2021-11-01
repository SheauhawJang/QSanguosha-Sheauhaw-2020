-- translation for Wind Package

return {
	["wind"] = "风包",

	["#xiahouyuan"] = "疾行的猎豹",
	["xiahouyuan"] = "夏侯渊",
	["shensu"] = "神速",
	[":shensu"] = "你可以选择一至三项：1.跳过判定阶段和摸牌阶段。2.跳过出牌阶段并弃置一张装备牌。3.跳过弃牌阶段并翻面。你每选择一项，视为你使用一张无距离限制的【杀】。",
	["@shensu1"] = "你可以发动“神速”，跳过判定阶段和摸牌阶段，视为使用【杀】",
	["@shensu2"] = "你可以发动“神速”，跳过出牌阶段并弃置一张装备牌，视为使用【杀】",
	["@shensu3"] = "你可以发动“神速”，跳过弃牌阶段并将翻面，视为使用【杀】",

	["#caoren"] = "大将军",
	["caoren"] = "曹仁",
	["illustrator:caoren"] = "Ccat",
	["jushou"] = "据守",
	[":jushou"] = "结束阶段开始时，你可翻面▶你摸四张牌，选择：1.弃置一张不为装备牌的手牌；2.使用一张对应的所有实体牌均为手牌的装备牌。",
	["@jushou"] = "据守：请弃置一张非装备牌；或使用一张装备牌",
	["jiewei"] = "解围",
	[":jiewei"] = "①当你需要使用普【无懈可击】时，你可使用对应的实体牌为你的装备区里的一张牌的【无懈可击】。"..
		"②当你翻面后，若你的武将牌正面朝上，你可弃置一张手牌▶你可将一名角色的判定/装备区里的一张牌置入另一名角色的判定/装备区。",
	["@jiewei"] = "你可以发动“解围”，弃置一张手牌",
	["@jiewei-move"] = "解围：你可以移动场上的一张牌",

	["#huangzhong"] = "老当益壮",
	["huangzhong"] = "黄忠",
	["liegong"] = "烈弓",
	[":liegong"] = "你使用的对应的实体牌数为1的【杀】的使用目标改为“你攻击范围内的一名其他角色或你至其的距离不大于X的一名其他角色（X为此【杀】的点数）”。；当你使用【杀】指定一个目标后，若该角色满足以下至少一项条件，你可以执行每项条件对应的效果：1.若其手牌数不大于你的手牌数，其不能使用【闪】响应此【杀】；2.若其体力值不小于你的体力值，此【杀】对其造成的伤害+1。",

	["#weiyan"] = "嗜血的独狼",
	["weiyan"] = "魏延",
	["illustrator:weiyan"] = "SoniaTang",
	["kuanggu"] = "狂骨",
	[":kuanggu"] = "当你对一名角色造成1点伤害后，若你至其的距离于其因受到此伤害而扣减体力前小于2，你可以选择：1.回复1点体力；2.摸一张牌。",
	["kuanggu:draw"] = "摸一张牌",
	["kuanggu:recover"] = "回复体力",
	["qimou"] = "奇谋",
	[":qimou"] = "限定技，出牌阶段，你可以失去任意点体力，你至其他角色的距离于此回合内-X且你于此回合内使用【杀】的次数上限+X（X为你以此法失去的体力数）。",

	["#zhangjiao"] = "天公将军",
	["zhangjiao"] = "张角",
	["illustrator:zhangjiao"] = "LiuHeng",
	["leiji"] = "雷击",
	[":leiji"] = "每当你使用或打出【闪】时，你可以令一名其他角色进行判定，若结果为：黑桃，你对该角色造成2点雷电伤害；梅花，你回复1点体力，然后对该角色造成1点雷电伤害。",
	["leiji-invoke"] = "你可以对一名其他角色发动“雷击”",
	["@leiji-choose"] = "雷击：请选择一名角色对其造成%arg点雷电伤害",
	["guidao"] = "鬼道",
	[":guidao"] = "当一名角色的判定牌生效前，你可以打出一张黑色牌替换之。",
	["@guidao-card"] = CommonTranslationTable["@askforretrial"],
	["huangtian"] = "黄天",
	[":huangtian"] = "主公技，其他群势力角色的出牌阶段限一次，该角色可以将一张【闪】或【闪电】交给你。",
	["huangtian_attach"] = "黄天送牌",

	["#xiaoqiao"] = "矫情之花",
	["xiaoqiao"] = "小乔",
	["hongyan"] = "红颜",
	[":hongyan"] = "锁定技，你的黑桃牌或你的黑桃判定牌的花色视为红桃。",
	["tianxiang"] = "天香",
	[":tianxiang"] = "当你受到伤害时，你可以弃置一张红桃手牌并选择一名其他角色。你防止此伤害。选择：1.其受到来源的1点伤害。其摸X张牌（X为其已损失的体力值，且至多为5）；2.其失去1点体力。其获得弃牌堆或牌堆中的此牌。",
	["@tianxiang-card"] = "你可以弃置一张红桃手牌对一名其他角色发动“天香”",
	["@tianxiang-choose"] = "天香：请选择令%dest受到伤害并摸牌，或令%dest失去体力并获得【%arg】",
	["tianxiang:damage"] = "受到伤害",
	["tianxiang:losehp"] = "失去体力",

	["#zhoutai"] = "历战之驱",
	["zhoutai"] = "周泰",
	["illustrator:zhoutai"] = "Thinking",
	["buqu"] = "不屈",
	[":buqu"] = "锁定技，当你处于濒死状态时，你将牌堆顶的一张牌置于你的武将牌上，称为“创”，若此牌的点数与你武将牌上已有的“创”点数均不同，则你将体力回复至1点。若出现相同点数则将此牌置入弃牌堆。若你的武将牌上有“创”，则你的手牌上限与“创”的数量相等。",
	["buqu_chuang"] = "创",
	["fenji"] = "奋激",
	[":fenji"] = "当一名角色因另一名角色的弃置或获得而失去手牌后，你可以失去1点体力。若如此做，失去手牌的角色摸两张牌。",

	["#yuji"] = "太平道人",
	["yuji"] = "于吉",
	["illustrator:yuji"] = "魔鬼鱼",
	["guhuo"] = "蛊惑",
	[":guhuo"] = "每名角色的回合限一次，你可以扣置一张手牌当任意一张基本牌或普通锦囊牌使用或打出。此时，一旦有其他角色质疑则翻开此牌：若为假：则此牌作废，质疑角色摸一张牌。若为真：则质疑角色须选择一项：1.弃置一张牌，获得技能“缠怨”；2.失去一点体力，获得技能“缠怨”。\n缠怨：锁定技，你不能质疑“蛊惑”；若你的体力值不大于1，则你的其他技能失效。",
	["chanyuan"] = "缠怨",
	[":chanyuan"] = "锁定技，你不能质疑“蛊惑”；若你的体力值不大于1，则你的其他技能失效。",
	["question"] = "质疑",
	["noquestion"] = "不质疑",
	["guhuo_saveself"] = "“蛊惑”【桃】或【酒】",
	["guhuo_slash"] = "“蛊惑”【杀】",
	["normal_slash"] = "普通杀",
	["#Guhuo"] = "%from 发动了“%arg2”，声明此牌为 【%arg】，指定的目标为 %to",
	["#GuhuoNoTarget"] = "%from 发动了“%arg2”，声明此牌为 【%arg】",
	["#GuhuoCannotQuestion"] = "%from 当前体力值为 %arg，无法质疑",
	["#GuhuoQuery"] = "%from 表示 %arg",
	["$GuhuoResult"] = "%from 的“<font color=\"yellow\"><b>蛊惑</b></font>”牌是 %card",
	["#Chanyuan"] = "%from 的“%arg”被触发，无法质疑",
}
