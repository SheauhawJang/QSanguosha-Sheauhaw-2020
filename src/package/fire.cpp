#include "fire.h"
#include "general.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "engine.h"
#include "maneuvering.h"

class dummyVS : public ZeroCardViewAsSkill
{
public:
    dummyVS() : ZeroCardViewAsSkill("dummy")
    {
    }

    virtual const Card *viewAs() const
    {
        return NULL;
    }
};

QuhuCard::QuhuCard()
{
}

bool QuhuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->getHp() > Self->getHp() && Self->canPindian(to_select);
}

void QuhuCard::use(Room *room, ServerPlayer *xunyu, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *tiger = targets.first();
    bool success = xunyu->pindian(tiger, "quhu", NULL);
    if (success) {
        QList<ServerPlayer *> players = room->getOtherPlayers(tiger), wolves;
        foreach (ServerPlayer *player, players) {
            if (tiger->inMyAttackRange(player))
                wolves << player;
        }

        if (wolves.isEmpty()) {
            LogMessage log;
            log.type = "#QuhuNoWolf";
            log.from = xunyu;
            log.to << tiger;
            room->sendLog(log);

            return;
        }

        ServerPlayer *wolf = room->askForPlayerChosen(xunyu, wolves, "quhu", QString("@quhu-damage:%1").arg(tiger->objectName()));
        room->damage(DamageStruct("quhu", tiger, wolf));
    } else {
        room->damage(DamageStruct("quhu", tiger, xunyu));
    }
}

class Quhu : public ZeroCardViewAsSkill
{
public:
    Quhu() : ZeroCardViewAsSkill("quhu")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("QuhuCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs() const
    {
        return new QuhuCard;
    }
};

class Jieming : public MasochismSkill
{
public:
    Jieming() : MasochismSkill("jieming")
    {
        view_as_skill = new dummyVS;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player)) {
            DamageStruct damage = data.value<DamageStruct>();
            return nameList(damage.damage);
        }

        return QStringList();
    }

    virtual void onDamaged(ServerPlayer *xunyu, const DamageStruct &) const
    {
        Room *room = xunyu->getRoom();
        ServerPlayer *to = room->askForPlayerChosen(xunyu, room->getAlivePlayers(), objectName(), "jieming-invoke", true, true);
        if (to != NULL) {
            xunyu->broadcastSkillInvoke(objectName());
            room->drawCards(to, 2, objectName());
            if (to->getHandcardNum() < to->getMaxHp())
                room->drawCards(xunyu, 1, objectName());
        }
    }
};

QiangxiCard::QiangxiCard()
{
}

bool QiangxiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty() || to_select == Self)
        return false;

    QStringList qiangxi_prop = Self->property("qiangxi").toString().split("+");
    if (qiangxi_prop.contains(to_select->objectName()))
        return false;

    int rangefix = 0;
    if (!subcards.isEmpty() && Self->getWeapon() && Self->getWeapon()->getId() == subcards.first()) {
        const Weapon *card = qobject_cast<const Weapon *>(Self->getWeapon()->getRealCard());
        rangefix += card->getRange() - Self->getAttackRange(false);;
    }

    return Self->inMyAttackRange(to_select, rangefix);
}

void QiangxiCard::extraCost(Room *room, const CardUseStruct &card_use) const
{
    if (subcards.isEmpty())
        room->loseHp(card_use.from);
    else {
        CardMoveReason reason(CardMoveReason::S_REASON_THROW, card_use.from->objectName(), QString(), card_use.card->getSkillName(), QString());
        room->moveCardTo(this, NULL, Player::DiscardPile, reason, true);
    }
}

void QiangxiCard::onEffect(const CardEffectStruct &effect) const
{
    QSet<QString> qiangxi_prop = effect.from->property("qiangxi").toString().split("+").toSet();
    qiangxi_prop.insert(effect.to->objectName());
    effect.from->getRoom()->setPlayerProperty(effect.from, "qiangxi", QStringList(qiangxi_prop.toList()).join("+"));
    effect.to->getRoom()->damage(DamageStruct("qiangxi", effect.from, effect.to));
}

class QiangxiViewAsSkill : public ViewAsSkill
{
public:
    QiangxiViewAsSkill() : ViewAsSkill("qiangxi")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->usedTimes("QiangxiCard") < 2;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.isEmpty() && to_select->isKindOf("Weapon") && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return new QiangxiCard;
        else if (cards.length() == 1) {
            QiangxiCard *card = new QiangxiCard;
            card->addSubcards(cards);

            return card;
        } else
            return NULL;
    }
};

class Qiangxi : public TriggerSkill
{
public:
    Qiangxi() : TriggerSkill("qiangxi")
    {
        events << EventPhaseChanging;
        view_as_skill = new QiangxiViewAsSkill;
    }

    bool triggerable(const ServerPlayer *) const
    {
        return false;
    }

    void record(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
            room->setPlayerProperty(player, "qiangxi", QVariant());
        }
    }
};

class LuanjiViewAsSkill : public ViewAsSkill
{
public:
    LuanjiViewAsSkill() : ViewAsSkill("luanji")
    {
        response_or_use = true;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (selected.isEmpty())
            return !to_select->isEquipped();
        else if (selected.length() == 1) {
            const Card *card = selected.first();
            return !to_select->isEquipped() && to_select->getSuit() == card->getSuit();
        } else
            return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 2) {
            ArcheryAttack *aa = new ArcheryAttack(Card::SuitToBeDecided, 0);
            aa->addSubcards(cards);
            aa->setSkillName(objectName());
            return aa;
        } else
            return NULL;
    }
};

class Luanji : public TriggerSkill
{
public:
    Luanji() : TriggerSkill("luanji")
    {
        events << TargetChosed;
        view_as_skill = new LuanjiViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("ArcheryAttack"))
                return nameList();
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        ServerPlayer *target = room->askForPlayerChosen(player, use.to, objectName(), "@luanji-cancel", true, true);
        if (target) {
            player->broadcastSkillInvoke(objectName());
            use.to.removeOne(target);
            data = QVariant::fromValue(use);
        }
        return false;
    }
};

class Xueyi : public TriggerSkill
{
public:
    Xueyi() : TriggerSkill("xueyi$")
    {
        events << TurnStart << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (triggerEvent == TurnStart && room->getTag("FirstRound").toBool()) {
            QList<ServerPlayer *> players = room->getAlivePlayers();
            foreach (ServerPlayer *p, players) {
                if (p->getKingdom() == "qun") {
                    QList<ServerPlayer *> yuanshaos = room->findPlayersBySkillName(objectName());
                    foreach (ServerPlayer *yuanshao, yuanshaos)
                        skill_list.insert(yuanshao, nameList());
                    break;
                }
            }
        } else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart) {
            if (TriggerSkill::triggerable(player) && player->getMark("#descendant") > 0)
                skill_list.insert(player, nameList());
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *yuanshao) const
    {
        if (triggerEvent == TurnStart) {
            room->sendCompulsoryTriggerLog(yuanshao, objectName());
            yuanshao->broadcastSkillInvoke(objectName());
            int x = 0;
            QList<ServerPlayer *> players = room->getAlivePlayers();
            foreach (ServerPlayer *p, players) {
                if (p->getKingdom() == "qun")
                    x++;
            }

            yuanshao->gainMark("#descendant", x);

        } else if (triggerEvent == EventPhaseStart) {
            if (yuanshao->askForSkillInvoke(this, "prompt")) {
                yuanshao->broadcastSkillInvoke(objectName());
                yuanshao->loseMark("#descendant");
                yuanshao->drawCards(1, objectName());
            }
        }
        return false;
    }
};

class XueyiMaxCards : public MaxCardsSkill
{
public:
    XueyiMaxCards() : MaxCardsSkill("#xueyi-maxcards")
    {
    }

    virtual int getExtra(const Player *target) const
    {
        if (target->hasLordSkill("xueyi")) {
            return target->getMark("#descendant") * 2;
        } else
            return 0;
    }
};

class ShuangxiongViewAsSkill : public OneCardViewAsSkill
{
public:
    ShuangxiongViewAsSkill() : OneCardViewAsSkill("shuangxiong")
    {
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("shuangxiong") != 0;
    }

    virtual bool viewFilter(const Card *card) const
    {
        if (card->isEquipped())
            return false;

        int value = Self->getMark("shuangxiong");
        if (value == 1)
            return card->isBlack();
        else if (value == 2)
            return card->isRed();

        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Duel *duel = new Duel(originalCard->getSuit(), originalCard->getNumber());
        duel->addSubcard(originalCard);
        duel->setSkillName(objectName());
        return duel;
    }
};

class Shuangxiong : public TriggerSkill
{
public:
    typedef QMap<ServerPlayer *, QVariantList> ShuangxiongMap;

    Shuangxiong() : TriggerSkill("shuangxiong")
    {
        events << EventPhaseStart << Damaged << CardResponded;
        view_as_skill = new ShuangxiongViewAsSkill;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *shuangxiong, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart && shuangxiong->getPhase() == Player::NotActive) {
            room->setPlayerMark(shuangxiong, "shuangxiong", 0);
            room->setPlayerMark(shuangxiong, "ViewAsSkill_shuangxiongEffect", 0);
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct res = data.value<CardResponseStruct>();
            if (res.m_card) {
                QVariant m_data = res.m_data;
                CardEffectStruct effect = m_data.value<CardEffectStruct>();
                if (effect.card && effect.card->isKindOf("Duel") && effect.card->getSkillName() == objectName()) {
                    QVariant qvar = effect.card->tag[objectName() + "Response"];
                    ShuangxiongMap map = qvar.value<ShuangxiongMap>();
                    map[shuangxiong] << QVariant::fromValue(res.m_card);
                    effect.card->tag[objectName() + "Response"] = QVariant::fromValue(map);
                }
            }
        }
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Draw) {
            if (TriggerSkill::triggerable(player))
                return nameList();
        } else if (triggerEvent == Damaged) {
            if (TriggerSkill::triggerable(player)) {
                DamageStruct damage = data.value<DamageStruct>();
                if (damage.card && damage.card->isKindOf("Duel") && damage.card->getSkillName() == objectName()) {
                    QVariant qvar = damage.card->tag[objectName() + "Response"];
                    ShuangxiongMap map = qvar.value<ShuangxiongMap>();
                    for (ShuangxiongMap::iterator it = map.begin(); it != map.end(); ++it) {
                        if (it.key() != player) {
                            foreach (QVariant cardvar, it.value()) {
                                const Card *card = cardvar.value<const Card *>();
                                if (card && room->isAllOnPlace(card, Player::DiscardPile)) {
                                    return nameList();
                                }
                            }
                        }
                    }
                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *shuangxiong, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (shuangxiong->askForSkillInvoke(this)) {
                shuangxiong->broadcastSkillInvoke(objectName());
                QList<int> ids = room->getNCards(2, false);
                CardsMoveStruct move(ids, shuangxiong, Player::PlaceTable,
                                     CardMoveReason(CardMoveReason::S_REASON_TURNOVER, shuangxiong->objectName(), objectName(), QString()));
                room->moveCardsAtomic(move, true);

                room->fillAG(ids);
                int id = room->askForAG(shuangxiong, ids, false, objectName());
                room->clearAG();

                const Card *card = Sanguosha->getCard(id);
                room->obtainCard(shuangxiong, card);

                ids.removeOne(id);
                DummyCard *dummy = new DummyCard(ids);
                CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, shuangxiong->objectName(), objectName(), QString());
                room->throwCard(dummy, reason, NULL);
                dummy->deleteLater();

                room->setPlayerMark(shuangxiong, "shuangxiong", card->isRed() ? 1 : 2);
                room->setPlayerMark(shuangxiong, "ViewAsSkill_shuangxiongEffect", 1);
                return true;
            }
        } else if (triggerEvent == Damaged) {
            if (room->askForSkillInvoke(shuangxiong, objectName())) {
                shuangxiong->broadcastSkillInvoke(objectName());
                DamageStruct damage = data.value<DamageStruct>();
                if (damage.card) {
                    QVariant qvar = damage.card->tag[objectName() + "Response"];
                    DummyCard *dummy = new DummyCard;
                    dummy->deleteLater();
                    ShuangxiongMap map = qvar.value<ShuangxiongMap>();
                    for (ShuangxiongMap::iterator it = map.begin(); it != map.end(); ++it) {
                        if (it.key() != shuangxiong) {
                            foreach (QVariant cardvar, it.value()) {
                                const Card *card = cardvar.value<const Card *>();
                                if (card && room->isAllOnPlace(card, Player::DiscardPile)) {
                                    dummy->addSubcards(card->getSubcards());
                                }
                            }
                        }
                    }
                    room->obtainCard(shuangxiong, dummy);
                }
            }
        }
        return false;
    }
};

class JianchuTargetMod : public TargetModSkill
{
public:
    JianchuTargetMod() : TargetModSkill("#jianchu-target")
    {

    }

    virtual int getResidueNum(const Player *from, const Card *, const Player *) const
    {
        return from->getMark("JianchuResidue");
    }
};

class Jianchu : public TriggerSkill
{
public:
    Jianchu() : TriggerSkill("jianchu")
    {
        events << TargetSpecified << EventPhaseChanging;
    }

    virtual void record(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging && data.value<PhaseChangeStruct>().from == Player::Play)
            player->setMark("JianchuResidue", 0);
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == TargetSpecified) {
            if (TriggerSkill::triggerable(player)) {
                CardUseStruct use = data.value<CardUseStruct>();
                if (use.card != NULL && use.card->isKindOf("Slash")) {
                    ServerPlayer *to = use.to.at(use.index);

                    if (to && to->isAlive() && player->canDiscard(to, "he"))
                        return nameList();
                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *player) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        int index = use.index;
        ServerPlayer *to = use.to.at(index);
        if (player->askForSkillInvoke(this, QVariant::fromValue(to))) {
            player->broadcastSkillInvoke(objectName());
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), to->objectName());

            int to_throw = room->askForCardChosen(player, to, "he", objectName(), false, Card::MethodDiscard);
            bool no_jink = Sanguosha->getCard(to_throw)->getTypeId() == Card::TypeEquip;
            room->throwCard(to_throw, to, player);
            if (no_jink) {
                player->addMark("JianchuResidue");
                LogMessage log;
                log.type = "#NoJink";
                log.from = to;
                room->sendLog(log);
                QVariantList jink_list = use.card->tag["Jink_List"].toList();
                jink_list[index] = 0;
                use.card->setTag("Jink_List", jink_list);

            } else if (room->isAllOnPlace(use.card, Player::PlaceTable))
                to->obtainCard(use.card);
        }

        return false;
    }
};

class Lianhuan : public OneCardViewAsSkill
{
public:
    Lianhuan() : OneCardViewAsSkill("lianhuan")
    {
        filter_pattern = ".|club|.|hand";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        IronChain *chain = new IronChain(originalCard->getSuit(), originalCard->getNumber());
        chain->addSubcard(originalCard);
        chain->setSkillName(objectName());
        return chain;
    }
};

class LianhuanTargetMod : public TargetModSkill
{
public:
    LianhuanTargetMod() : TargetModSkill("#lianhuan-target")
    {
        pattern = "IronChain";
    }

    int getExtraTargetNum(const Player *from, const Card *card) const
    {
        if (from->hasSkill("lianhuan"))
            return 1;
        return 0;
    }
};

class Niepan : public TriggerSkill
{
public:
    Niepan() : TriggerSkill("niepan")
    {
        events << AskForPeaches;
        frequency = Limited;
        limit_mark = "@nirvana";
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *target, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(target) && target->getMark("@nirvana") > 0) {
            DyingStruct dying_data = data.value<DyingStruct>();

            if (target->getHp() > 0)
                return QStringList();

            if (dying_data.who != target)
                return QStringList();
            return nameList();
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *pangtong, QVariant &, ServerPlayer *) const
    {
        if (pangtong->askForSkillInvoke(this)) {
            pangtong->broadcastSkillInvoke(objectName());

            room->removePlayerMark(pangtong, "@nirvana");

            pangtong->throwAllHandCardsAndEquips();
            QList<const Card *> tricks = pangtong->getJudgingArea();
            foreach (const Card *trick, tricks) {
                CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, pangtong->objectName());
                room->throwCard(trick, reason, NULL);
            }

            if (pangtong->isChained())
                room->setPlayerProperty(pangtong, "chained", false);

            if (!pangtong->faceUp())
                pangtong->turnOver();

            pangtong->drawCards(3, objectName());
            room->recover(pangtong, RecoverStruct(pangtong, NULL, 3 - pangtong->getHp()));
            QStringList skill_list;
            if (!pangtong->hasSkill("bazhen", true))
                skill_list << "bazhen";
            if (!pangtong->hasSkill("huoji", true))
                skill_list << "huoji";
            if (!pangtong->hasSkill("kanpo", true))
                skill_list << "kanpo";
            if (!skill_list.isEmpty())
                room->acquireSkill(pangtong, room->askForChoice(pangtong, objectName(), skill_list.join("+"), QVariant(), "@niepan-choose"));


        }

        return false;
    }
};

class Huoji : public OneCardViewAsSkill
{
public:
    Huoji() : OneCardViewAsSkill("huoji")
    {
        filter_pattern = ".|red";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        FireAttack *fire_attack = new FireAttack(originalCard->getSuit(), originalCard->getNumber());
        fire_attack->addSubcard(originalCard->getId());
        fire_attack->setSkillName(objectName());
        return fire_attack;
    }
};

class Bazhen : public ViewHasSkill
{
public:
    Bazhen() : ViewHasSkill("bazhen")
    {
    }
    virtual bool ViewHas(const Player *player, const QString &skill_name, const QString &flag) const
    {
        if (flag == "armor" && skill_name == "eight_diagram" && player->isAlive() && player->hasSkill(objectName()) && !player->getArmor())
            return true;
        return false;
    }
};

class Kanpo : public OneCardViewAsSkill
{
public:
    Kanpo() : OneCardViewAsSkill("kanpo")
    {
        filter_pattern = ".|black";
        response_pattern = "nullification";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Card *ncard = new Nullification(originalCard->getSuit(), originalCard->getNumber());
        ncard->addSubcard(originalCard);
        ncard->setSkillName(objectName());
        return ncard;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        return !player->isNude() || !player->getHandPile().isEmpty();
    }
};

class Cangzhuo : public PhaseChangeSkill
{
public:
    Cangzhuo() : PhaseChangeSkill("cangzhuo")
    {
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Discard
               && target->getCardUsedTimes("TrickCard") == 0;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        room->sendCompulsoryTriggerLog(player, objectName());
        player->broadcastSkillInvoke(objectName());
        room->setPlayerFlag(player, "CangzhuoEffect");
        return false;
    }
};

class CangzhuoHide : public HideCardSkill
{
public:
    CangzhuoHide() : HideCardSkill("#cangzhuo-hide")
    {
    }
    virtual bool isCardHided(const Player *player, const Card *card) const
    {
        return (player->hasFlag("CangzhuoEffect") && card->getTypeId() == Card::TypeTrick);
    }
};

TianyiCard::TianyiCard()
{
}

bool TianyiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && Self->canPindian(to_select);
}

void TianyiCard::use(Room *room, ServerPlayer *taishici, QList<ServerPlayer *> &targets) const
{
    bool success = taishici->pindian(targets.first(), "tianyi", NULL);
    if (success)
        room->setPlayerFlag(taishici, "TianyiSuccess");
    else
        room->setPlayerCardLimitation(taishici, "use", "Slash", true);
}

class Tianyi : public ZeroCardViewAsSkill
{
public:
    Tianyi() : ZeroCardViewAsSkill("tianyi")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("TianyiCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs() const
    {
        return new TianyiCard;
    }
};

class TianyiTargetMod : public TargetModSkill
{
public:
    TianyiTargetMod() : TargetModSkill("#tianyi-target")
    {
        frequency = NotFrequent;
    }

    virtual int getResidueNum(const Player *from, const Card *, const Player *) const
    {
        if (from->hasFlag("TianyiSuccess"))
            return 1;
        else
            return 0;
    }

    virtual int getDistanceLimit(const Player *from, const Card *, const Player *) const
    {
        if (from->hasFlag("TianyiSuccess"))
            return 1000;
        else
            return 0;
    }

    virtual int getExtraTargetNum(const Player *from, const Card *) const
    {
        if (from->hasFlag("TianyiSuccess"))
            return 1;
        else
            return 0;
    }
};

class Hanzhan : public TriggerSkill
{
public:
    Hanzhan() : TriggerSkill("hanzhan")
    {
        events << AskForPindianCard << PindianSummary;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList list;
        PindianStruct *pindian = data.value<PindianStruct *>();
        if (triggerEvent == AskForPindianCard) {
            if (pindian->from == player) {
                foreach (ServerPlayer *p, pindian->tos)
                    if (TriggerSkill::triggerable(p))
                        list.insert(p, nameList());
            } else {
                if (TriggerSkill::triggerable(pindian->from))
                    list.insert(pindian->from, nameList());
            }
        } else {
            bool hasMax = false;
            QList<const Card *> cards;
            cards << pindian->from_card << pindian->to_cards;
            int max = 0;
            foreach (const Card *card, cards)
                if (card->isKindOf("Slash"))
                    max = qMax(max, card->getNumber());
            if (max > 0)
                foreach (const Card *card, cards)
                    if (card->isKindOf("Slash") && card->getNumber() == max && room->getCardPlace(card->getEffectiveId()) == Player::PlaceTable) {
                        hasMax = true;
                        break;
                    }
            if (hasMax) {
                QList<ServerPlayer *> players;
                players << pindian->from << pindian->tos;
                foreach (ServerPlayer *p, players)
                    if (TriggerSkill::triggerable(p))
                        list.insert(p, nameList());
            }
        }
        return list;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *taishici) const
    {
        PindianStruct *pindian = data.value<PindianStruct *>();
        if (triggerEvent == AskForPindianCard) {
            if (room->askForSkillInvoke(taishici, objectName(), QVariant::fromValue(player))) {
                if (player == pindian->from) {
                    pindian->fromtag << "hanzhan";
                } else {
                    for (int i = 0; i < pindian->tos.size(); ++i)
                        if (player == pindian->tos.at(i)) {
                            pindian->totags[i] << "hanzhan";
                        }
                }
            }
        } else {
            QList<const Card *> cards;
            cards << pindian->from_card << pindian->to_cards;
            int max = 0;
            foreach (const Card *card, cards)
                if (card->isKindOf("Slash"))
                    max = qMax(max, card->getNumber());
            DummyCard *dummy = new DummyCard;
            foreach (const Card *card, cards)
                if (card->isKindOf("Slash") && card->getNumber() == max && room->getCardPlace(card->getEffectiveId()) == Player::PlaceTable) {
                    dummy->addSubcard(card);
                }
            if (room->askForSkillInvoke(taishici, objectName()))
                taishici->obtainCard(dummy);
            dummy->deleteLater();
        }
        return false;
    }
};

FirePackage::FirePackage()
    : Package("fire")
{
    General *dianwei = new General(this, "dianwei", "wei"); // WEI 012
    dianwei->addSkill(new Qiangxi);

    General *xunyu = new General(this, "xunyu", "wei", 3); // WEI 013
    xunyu->addSkill(new Quhu);
    xunyu->addSkill(new Jieming);

    General *pangtong = new General(this, "pangtong", "shu", 3); // SHU 010
    pangtong->addSkill(new Lianhuan);
    pangtong->addSkill(new LianhuanTargetMod);
    pangtong->addSkill(new Niepan);
    pangtong->addRelateSkill("bazhen");
    pangtong->addRelateSkill("huoji");
    pangtong->addRelateSkill("kanpo");
    related_skills.insertMulti("lianhuan", "#lianhuan-target");

    General *wolong = new General(this, "wolong", "shu", 3); // SHU 011
    wolong->addSkill(new Bazhen);
    wolong->addSkill(new Huoji);
    wolong->addSkill(new Kanpo);
    wolong->addSkill(new Cangzhuo);
    wolong->addSkill(new CangzhuoHide);
    related_skills.insertMulti("cangzhuo", "#cangzhuo-hide");

    General *taishici = new General(this, "taishici", "wu"); // WU 012
    taishici->addSkill(new Tianyi);
    taishici->addSkill(new TianyiTargetMod);
    taishici->addSkill(new Hanzhan);
    related_skills.insertMulti("tianyi", "#tianyi-target");

    General *yuanshao = new General(this, "yuanshao$", "qun"); // QUN 004
    yuanshao->addSkill(new Luanji);
    yuanshao->addSkill(new Xueyi);
    yuanshao->addSkill(new XueyiMaxCards);
    related_skills.insertMulti("xueyi", "#xueyi-maxcards");

    General *yanliangwenchou = new General(this, "yanliangwenchou", "qun"); // QUN 005
    yanliangwenchou->addSkill(new Shuangxiong);

    General *pangde = new General(this, "pangde", "qun"); // QUN 008
    pangde->addSkill("mashu");
    pangde->addSkill(new Jianchu);
    pangde->addSkill(new JianchuTargetMod);
    related_skills.insertMulti("jianchu", "#jianchu-target");

    addMetaObject<QuhuCard>();
    addMetaObject<QiangxiCard>();
    addMetaObject<TianyiCard>();
}

ADD_PACKAGE(Fire)

