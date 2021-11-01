#include "general.h"
#include "standard.h"
#include "skill.h"
#include "engine.h"
#include "client.h"
#include "serverplayer.h"
#include "room.h"
#include "standard-skillcards.h"
#include "ai.h"
#include "settings.h"
#include "sp.h"
#include "wind.h"
#include "god.h"
#include "maneuvering.h"
#include "json.h"
#include "clientplayer.h"
#include "clientstruct.h"
#include "util.h"
#include "wrapped-card.h"
#include "roomthread.h"
#include "nostalgia-physical.h"

Nos2013RendeCard::Nos2013RendeCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    mute = true;
}

void Nos2013RendeCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{

    if (!source->tag.value("nos2013rende_using", false).toBool())
        room->broadcastSkillInvoke("nos2013rende");

    ServerPlayer *target = targets.first();

    int old_value = source->getMark("nos2013rende");
    QList<int> nos2013rende_list;
    if (old_value > 0)
        nos2013rende_list = StringList2IntList(source->property("nos2013rende").toString().split("+"));
    else
        nos2013rende_list = source->handCards();
    foreach(int id, this->subcards)
        nos2013rende_list.removeOne(id);
    room->setPlayerProperty(source, "nos2013rende", IntList2StringList(nos2013rende_list).join("+"));

    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), target->objectName(), "nos2013rende", QString());
    room->obtainCard(target, this, reason, false);

    int new_value = old_value + subcards.length();
    room->setPlayerMark(source, "nos2013rende", new_value);

    if (old_value < 2 && new_value >= 2)
        room->recover(source, RecoverStruct(source));

    if (room->getMode() == "04_1v3" && source->getMark("nos2013rende") >= 2) return;
    if (source->isKongcheng() || source->isDead() || nos2013rende_list.isEmpty()) return;
    room->addPlayerHistory(source, "Nos2013RendeCard", -1);

    source->tag["nos2013rende_using"] = true;

    if (!room->askForUseCard(source, "@@nos2013rende", "@nos2013rende-give", -1, Card::MethodNone))
        room->addPlayerHistory(source, "Nos2013RendeCard");

    source->tag["nos2013rende_using"] = false;
}

class Nos2013RendeViewAsSkill : public ViewAsSkill
{
public:
    Nos2013RendeViewAsSkill() : ViewAsSkill("nos2013rende")
    {
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (ServerInfo.GameMode == "04_1v3" && selected.length() + Self->getMark("nos2013rende") >= 2)
            return false;
        else {
            if (to_select->isEquipped()) return false;
            if (Sanguosha->currentRoomState()->getCurrentCardUsePattern() == "@@nos2013rende") {
                QList<int> nos2013rende_list = StringList2IntList(Self->property("nos2013rende").toString().split("+"));
                return nos2013rende_list.contains(to_select->getEffectiveId());
            } else
                return true;
        }
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        if (ServerInfo.GameMode == "04_1v3" && player->getMark("nos2013rende") >= 2)
            return false;
        return !player->hasUsed("Nos2013RendeCard") && !player->isKongcheng();
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "@@nos2013rende";
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return NULL;

        Nos2013RendeCard *nos2013rende_card = new Nos2013RendeCard;
        nos2013rende_card->addSubcards(cards);
        return nos2013rende_card;
    }
};

class Nos2013Rende : public TriggerSkill
{
public:
    Nos2013Rende() : TriggerSkill("nos2013rende")
    {
        events << EventPhaseChanging;
        view_as_skill = new Nos2013RendeViewAsSkill;
    }

    bool triggerable(const ServerPlayer *) const
    {
        return false;
    }

    void record(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to != Player::NotActive)
            return;
        room->setPlayerMark(player, "nos2013rende", 0);
        room->setPlayerProperty(player, "nos2013rende", QString());
    }
};

class Nos2013Jizhi : public TriggerSkill
{
public:
    Nos2013Jizhi() : TriggerSkill("nos2013jizhi")
    {
        frequency = Frequent;
        events << CardUsed;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card && use.card->getTypeId() == Card::TypeTrick)
            return nameList();
        return QStringList();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *yueying, QVariant &) const
    {
        if (room->askForSkillInvoke(yueying, objectName())) {
            yueying->broadcastSkillInvoke(objectName());

            QList<int> ids = room->getNCards(1, false);
            CardsMoveStruct move(ids, yueying, Player::PlaceTable,
                                 CardMoveReason(CardMoveReason::S_REASON_TURNOVER, yueying->objectName(), "nos2013jizhi", QString()));
            room->moveCardsAtomic(move, true);

            int id = ids.first();
            const Card *card = Sanguosha->getCard(id);
            if (!card->isKindOf("BasicCard")) {
                CardMoveReason reason(CardMoveReason::S_REASON_DRAW, yueying->objectName(), "nos2013jizhi", QString());
                room->obtainCard(yueying, card, reason);
            } else {
                const Card *card_ex = NULL;
                if (!yueying->isKongcheng())
                    card_ex = room->askForCard(yueying, ".", "@nos2013jizhi-exchange:::" + card->objectName(),
                                               QVariant::fromValue(card), Card::MethodNone);
                if (card_ex) {
                    CardMoveReason reason1(CardMoveReason::S_REASON_PUT, yueying->objectName(), "nos2013jizhi", QString());
                    CardMoveReason reason2(CardMoveReason::S_REASON_DRAW, yueying->objectName(), "nos2013jizhi", QString());
                    CardsMoveStruct move1(card_ex->getEffectiveId(), yueying, NULL, Player::PlaceUnknown, Player::DrawPile, reason1);
                    CardsMoveStruct move2(ids, yueying, yueying, Player::PlaceUnknown, Player::PlaceHand, reason2);

                    QList<CardsMoveStruct> moves;
                    moves.append(move1);
                    moves.append(move2);
                    room->moveCardsAtomic(moves, false);
                } else {
                    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, yueying->objectName(), "nos2013jizhi", QString());
                    room->throwCard(card, reason, NULL);
                }
            }
        }

        return false;
    }
};

class Nos2013Qicai : public TargetModSkill
{
public:
    Nos2013Qicai() : TargetModSkill("nos2013qicai")
    {
        pattern = "TrickCard";
    }

    virtual int getDistanceLimit(const Player *from, const Card *, const Player *) const
    {
        if (from->hasSkill(this))
            return 1000;
        else
            return 0;
    }
};

class Nos2013Biyue : public PhaseChangeSkill
{
public:
    Nos2013Biyue() : PhaseChangeSkill("nos2013biyue")
    {
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Finish;
    }

    virtual bool onPhaseChange(ServerPlayer *diaochan) const
    {
        Room *room = diaochan->getRoom();
        if (room->askForSkillInvoke(diaochan, objectName())) {
            diaochan->broadcastSkillInvoke(objectName());
            diaochan->drawCards(1, objectName());
        }
        return false;
    }
};

class Nos2013Jushou : public PhaseChangeSkill
{
public:
    Nos2013Jushou() : PhaseChangeSkill("nos2013jushou")
    {

    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        return PhaseChangeSkill::triggerable(triggerEvent, room, player, data);
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Finish;
    }

    virtual bool onPhaseChange(ServerPlayer *caoren) const
    {
        if (caoren->getPhase() == Player::Finish) {
            Room *room = caoren->getRoom();
            if (room->askForSkillInvoke(caoren, objectName())) {
                caoren->broadcastSkillInvoke(objectName());
                caoren->drawCards(1, objectName());
                caoren->turnOver();
            }
        }
        return false;
    }
};

class Nos2013Jiewei : public TriggerSkill
{
public:
    Nos2013Jiewei() : TriggerSkill("nos2013jiewei")
    {
        events << TurnedOver;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (!room->askForSkillInvoke(player, objectName())) return false;
        room->broadcastSkillInvoke(objectName());
        player->drawCards(1, objectName());

        const Card *card = room->askForUseCard(player, "TrickCard+^Nullification,EquipCard|.|.|hand", "@Nos2013Jiewei");
        if (!card) return false;

        QList<ServerPlayer *> targets;
        if (card->getTypeId() == Card::TypeTrick) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                bool can_discard = false;
                foreach (const Card *judge, p->getJudgingArea()) {
                    if (judge->getTypeId() == Card::TypeTrick && player->canDiscard(p, judge->getEffectiveId())) {
                        can_discard = true;
                        break;
                    } else if (judge->getTypeId() == Card::TypeSkill) {
                        const Card *real_card = Sanguosha->getEngineCard(judge->getEffectiveId());
                        if (real_card->getTypeId() == Card::TypeTrick && player->canDiscard(p, real_card->getEffectiveId())) {
                            can_discard = true;
                            break;
                        }
                    }
                }
                if (can_discard) targets << p;
            }
        } else if (card->getTypeId() == Card::TypeEquip) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (!p->getEquips().isEmpty() && player->canDiscard(p, "e"))
                    targets << p;
                else {
                    foreach (const Card *judge, p->getJudgingArea()) {
                        if (judge->getTypeId() == Card::TypeSkill) {
                            const Card *real_card = Sanguosha->getEngineCard(judge->getEffectiveId());
                            if (real_card->getTypeId() == Card::TypeEquip && player->canDiscard(p, real_card->getEffectiveId())) {
                                targets << p;
                                break;
                            }
                        }
                    }
                }
            }
        }
        if (targets.isEmpty()) return false;
        ServerPlayer *to_discard = room->askForPlayerChosen(player, targets, objectName(), "@Nos2013Jiewei-discard", true);
        if (to_discard) {
            QList<int> disabled_ids;
            foreach (const Card *c, to_discard->getCards("ej")) {
                const Card *pcard = c;
                if (pcard->getTypeId() == Card::TypeSkill)
                    pcard = Sanguosha->getEngineCard(c->getEffectiveId());
                if (pcard->getTypeId() != card->getTypeId())
                    disabled_ids << pcard->getEffectiveId();
            }
            int id = room->askForCardChosen(player, to_discard, "ej", objectName(), false, Card::MethodDiscard, disabled_ids);
            room->throwCard(id, to_discard, player);
        }
        return false;
    }
};

class Nos2015Jushou : public TriggerSkill
{
public:
    Nos2015Jushou() : TriggerSkill("nos2015jushou")
    {
        events << EventPhaseStart << TurnedOver;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == TurnedOver) {
            if (TriggerSkill::triggerable(player)) {
                TurnStruct turn = data.value<TurnStruct>();
                if (turn.name == objectName() || turn.name == "turn_start")
                    return nameList();
            }
            return QStringList();
        }
        if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Finish)
            return nameList();
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *caoren) const
    {
        if (triggerEvent == TurnedOver) {
            if (!room->askForSkillInvoke(player, objectName())) return false;
            room->broadcastSkillInvoke(objectName());
            player->drawCards(1, objectName());

            const Card *card = room->askForUseCard(player, "TrickCard+^Nullification,EquipCard|.|.|hand", "@Nos2015Jushou");
            if (!card) return false;

            QList<ServerPlayer *> targets;
            if (card->getTypeId() == Card::TypeTrick) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    bool can_discard = false;
                    foreach (const Card *judge, p->getJudgingArea()) {
                        if (judge->getTypeId() == Card::TypeTrick && player->canDiscard(p, judge->getEffectiveId())) {
                            can_discard = true;
                            break;
                        } else if (judge->getTypeId() == Card::TypeSkill) {
                            const Card *real_card = Sanguosha->getEngineCard(judge->getEffectiveId());
                            if (real_card->getTypeId() == Card::TypeTrick && player->canDiscard(p, real_card->getEffectiveId())) {
                                can_discard = true;
                                break;
                            }
                        }
                    }
                    if (can_discard) targets << p;
                }
            } else if (card->getTypeId() == Card::TypeEquip) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (!p->getEquips().isEmpty() && player->canDiscard(p, "e"))
                        targets << p;
                    else {
                        foreach (const Card *judge, p->getJudgingArea()) {
                            if (judge->getTypeId() == Card::TypeSkill) {
                                const Card *real_card = Sanguosha->getEngineCard(judge->getEffectiveId());
                                if (real_card->getTypeId() == Card::TypeEquip && player->canDiscard(p, real_card->getEffectiveId())) {
                                    targets << p;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            if (targets.isEmpty()) return false;
            ServerPlayer *to_discard = room->askForPlayerChosen(player, targets, objectName(), "@Nos2015Jushou-discard", true);
            if (to_discard) {
                QList<int> disabled_ids;
                foreach (const Card *c, to_discard->getCards("ej")) {
                    const Card *pcard = c;
                    if (pcard->getTypeId() == Card::TypeSkill)
                        pcard = Sanguosha->getEngineCard(c->getEffectiveId());
                    if (pcard->getTypeId() != card->getTypeId())
                        disabled_ids << pcard->getEffectiveId();
                }
                int id = room->askForCardChosen(player, to_discard, "ej", objectName(), false, Card::MethodDiscard, disabled_ids);
                room->throwCard(id, to_discard, player);
            }
        } else {
            if (caoren->getPhase() == Player::Finish) {
                Room *room = caoren->getRoom();
                if (room->askForSkillInvoke(caoren, objectName())) {
                    caoren->broadcastSkillInvoke(objectName());
                    caoren->drawCards(1, objectName());
                    caoren->turnOver(objectName());
                }
            }
        }
        return false;
    }
};

class Nos2013Leiji : public TriggerSkill
{
public:
    Nos2013Leiji() : TriggerSkill("nos2013leiji")
    {
        events << CardResponded;
        view_as_skill = new dummyVS;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        const Card *card_star = data.value<CardResponseStruct>().m_card;
        if (card_star->isKindOf("Jink"))
            return nameList();
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *zhangjiao, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = room->askForPlayerChosen(zhangjiao, room->getAlivePlayers(), objectName(), "leiji-invoke", true, true);
        if (target) {
            zhangjiao->broadcastSkillInvoke("nos2013leiji");

            JudgeStruct judge;
            judge.pattern = ".|black";
            judge.good = false;
            judge.negative = true;
            judge.reason = objectName();
            judge.who = target;

            room->judge(judge);

            if (judge.isBad()) {
                room->damage(DamageStruct(objectName(), zhangjiao, target, 1, DamageStruct::Thunder));
                room->recover(zhangjiao, RecoverStruct(zhangjiao));
            }
        }
        return false;
    }
};

class Nos2020LuanjiViewAsSkill : public ViewAsSkill
{
public:
    Nos2020LuanjiViewAsSkill() : ViewAsSkill("nos2020luanji")
    {
        response_or_use = true;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        QSet<QString> used = Self->property("nos2020luanjiUsedSuits").toString().split("+").toSet();
        return selected.length() < 2 && !to_select->isEquipped() && !used.contains(to_select->getSuitString());
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

class Nos2020Luanji : public TriggerSkill
{
public:
    Nos2020Luanji() : TriggerSkill("nos2020luanji")
    {
        events << PreCardUsed << EventPhaseChanging << Damage << CardResponded << CardFinished;
        view_as_skill = new Nos2020LuanjiViewAsSkill;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        switch (triggerEvent) {
        case PreCardUsed: {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card && use.card->isKindOf("ArcheryAttack") && use.card->getSkillName() == objectName()) {
                QSet<QString> used = player->property("nos2020luanjiUsedSuits").toString().split("+").toSet();
                foreach (int id, use.card->getSubcards())
                    used.insert(Sanguosha->getCard(id)->getSuitString());
                room->setPlayerProperty(player, "nos2020luanjiUsedSuits", QStringList(used.toList()).join("+"));
            }
            break;
        }
        case EventPhaseChanging: {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                room->setPlayerProperty(player, "nos2020luanjiUsedSuits", QVariant());
            }
            break;
        }
        case Damage: {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("ArcheryAttack") && damage.card->getSkillName() == objectName()) {
                damage.card->tag["nos2020luanjiDamageCount"] = damage.card->tag["nos2020luanjiDamageCount"].toInt() + 1;
            }
            break;
        }
        default:
            break;
        }
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data) const
    {
        TriggerList list;
        if (triggerEvent == CardResponded) {
            CardResponseStruct res = data.value<CardResponseStruct>();
            CardEffectStruct effect = res.m_data.value<CardEffectStruct>();
            if (effect.card && effect.card->isKindOf("ArcheryAttack") && effect.card->getSkillName() == objectName()) {
                list.insert(player, comList());
            }
        } else if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card && use.card->isKindOf("ArcheryAttack") && use.card->getSkillName() == objectName()) {
                if (!use.card->tag["nos2020luanjiDamageCount"].toInt())
                    if (use.from)
                        list.insert(use.from, comList());
            }
        }
        return list;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *yuanshao) const
    {
        if (triggerEvent == CardResponded) {
            room->drawCards(player, 1, objectName());
        } else if (triggerEvent == CardFinished) {
            yuanshao->broadcastSkillInvoke(objectName());
            room->drawCards(yuanshao, 1, objectName());
        }
        return false;
    }
};

Nos2017QingjianAllotCard::Nos2017QingjianAllotCard()
{
    will_throw = false;
    mute = true;
    handling_method = Card::MethodNone;
}

void Nos2017QingjianAllotCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    QList<int> rende_list = StringList2IntList(source->property("nos2017qingjian_give").toString().split("+"));
    foreach (int id, subcards) {
        rende_list.removeOne(id);
    }
    room->setPlayerProperty(source, "nos2017qingjian_give", IntList2StringList(rende_list).join("+"));

    QList<int> give_list = StringList2IntList(target->property("rende_give").toString().split("+"));
    foreach (int id, subcards) {
        give_list.append(id);
    }
    room->setPlayerProperty(target, "rende_give", IntList2StringList(give_list).join("+"));
}

class Nos2017QingjianAllot : public ViewAsSkill
{
public:
    Nos2017QingjianAllot() : ViewAsSkill("nos2017qingjian_allot")
    {
        expand_pile = "nos2017qingjian";
        response_pattern = "@@nos2017qingjian_allot!";
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        QList<int> nos2017qingjian_list = StringList2IntList(Self->property("nos2017qingjian_give").toString().split("+"));
        return nos2017qingjian_list.contains(to_select->getEffectiveId());
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty()) return NULL;
        Nos2017QingjianAllotCard *nos2017qingjian_card = new Nos2017QingjianAllotCard;
        nos2017qingjian_card->addSubcards(cards);
        return nos2017qingjian_card;
    }
};

class Nos2017QingjianViewAsSkill : public ViewAsSkill
{
public:
    Nos2017QingjianViewAsSkill() : ViewAsSkill("nos2017qingjian")
    {
        response_pattern = "@@nos2017qingjian";
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        QList<int> nos2017qingjian_list = StringList2IntList(Self->property("nos2017qingjian").toString().split("+"));
        return nos2017qingjian_list.contains(to_select->getEffectiveId());
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() > 0) {
            DummyCard *xt = new DummyCard;
            xt->addSubcards(cards);
            return xt;
        }

        return NULL;
    }
};

class Nos2017Qingjian : public TriggerSkill
{
public:
    Nos2017Qingjian() : TriggerSkill("nos2017qingjian")
    {
        events << CardsMoveOneTime << EventPhaseChanging;
        view_as_skill = new Nos2017QingjianViewAsSkill;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList list;
        if (triggerEvent == CardsMoveOneTime && TriggerSkill::triggerable(player)) {
            QList<int> ids;
            foreach (QVariant qvar, data.toList()) {
                CardsMoveOneTimeStruct move = qvar.value<CardsMoveOneTimeStruct>();
                if (!room->getTag("FirstRound").toBool() && player->getPhase() != Player::Draw && move.to == player && move.to_place == Player::PlaceHand) {
                    foreach (int id, move.card_ids) {
                        if (room->getCardOwner(id) == player && room->getCardPlace(id) == Player::PlaceHand)
                            ids << id;
                    }
                }
            }
            if (!ids.isEmpty())
                list.insert(player, nameList());
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive) return list;
            foreach (ServerPlayer *xiahou, room->getAllPlayers()) {
                if (xiahou->getPile("nos2017qingjian").length() > 0 && TriggerSkill::triggerable(xiahou)) {
                    list.insert(xiahou, comList());
                }
            }
        }
        return list;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *xiahou) const
    {
        if (triggerEvent == CardsMoveOneTime && TriggerSkill::triggerable(player)) {
            QList<int> ids;
            foreach (QVariant qvar, data.toList()) {
                CardsMoveOneTimeStruct move = qvar.value<CardsMoveOneTimeStruct>();
                if (!room->getTag("FirstRound").toBool() && player->getPhase() != Player::Draw && move.to == player && move.to_place == Player::PlaceHand) {
                    foreach (int id, move.card_ids) {
                        if (room->getCardOwner(id) == player && room->getCardPlace(id) == Player::PlaceHand)
                            ids << id;
                    }
                }
            }
            if (ids.isEmpty())
                return false;
            room->setPlayerProperty(player, "nos2017qingjian", IntList2StringList(ids).join("+"));
            const Card *card = room->askForCard(player, "@@nos2017qingjian", "@nos2017qingjian-put", data, Card::MethodNone);
            room->setPlayerProperty(player, "nos2017qingjian", QString());
            if (card) {
                LogMessage log;
                log.type = "#InvokeSkill";
                log.arg = objectName();
                log.from = player;
                room->sendLog(log);
                room->notifySkillInvoked(player, objectName());
                player->broadcastSkillInvoke(objectName());
                player->addToPile("nos2017qingjian", card, false);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive) return false;
            if (xiahou->getPile("nos2017qingjian").length() > 0 && TriggerSkill::triggerable(xiahou)) {
                room->sendCompulsoryTriggerLog(xiahou, objectName());
                xiahou->broadcastSkillInvoke(objectName());
                QList<int> ids = xiahou->getPile("nos2017qingjian");
                room->setPlayerProperty(xiahou, "nos2017qingjian_give", IntList2StringList(ids).join("+"));
                do {
                    const Card *use = room->askForUseCard(xiahou, "@@nos2017qingjian_allot!", "@nos2017qingjian-give", QVariant(), Card::MethodNone);
                    ids = StringList2IntList(xiahou->property("nos2017qingjian_give").toString().split("+"));
                    if (use == NULL) {
                        Nos2017QingjianAllotCard *nos2017qingjian_card = new Nos2017QingjianAllotCard;
                        nos2017qingjian_card->addSubcards(ids);
                        QList<ServerPlayer *> targets;
                        targets << room->getOtherPlayers(xiahou).first();
                        nos2017qingjian_card->use(room, xiahou, targets);
                        delete nos2017qingjian_card;
                        break;
                    }
                } while (!ids.isEmpty() && xiahou->isAlive());
                room->setPlayerProperty(xiahou, "nos2017qingjian_give", QString());
                QList<CardsMoveStruct> moves;
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    QList<int> give_list = StringList2IntList(p->property("rende_give").toString().split("+"));
                    if (give_list.isEmpty())
                        continue;
                    room->setPlayerProperty(p, "rende_give", QString());
                    CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, p->objectName(), "nos2017qingjian", QString());
                    CardsMoveStruct move(give_list, p, Player::PlaceHand, reason);
                    moves.append(move);
                }
                if (!moves.isEmpty())
                    room->moveCardsAtomic(moves, false);
            }
        }
        return false;
    }
};

class NosRnTishen : public TriggerSkill
{
public:
    NosRnTishen() : TriggerSkill("nosrntishen")
    {
        events << EventPhaseChanging << EventPhaseStart << HpChanged;
        frequency = Limited;
        limit_mark = "@nosrnsubstitute";
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseStart && TriggerSkill::triggerable(player)
                && player->getMark("@nosrnsubstitute") > 0 && player->getMark("#nosrntishen") > 0 && player->getPhase() == Player::Start)
            return nameList();
        return QStringList();
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (!TriggerSkill::triggerable(player))
            return;
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                room->setPlayerMark(player, "#nosrntishen", 0);
            }
        } else {
            if (!data.isNull() && !data.canConvert<RecoverStruct>() && player->getMark("#nosrntishen") < 4) {
                room->addPlayerMark(player, "#nosrntishen", 1);
            }
        }
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        int x = player->getMark("#nosrntishen");
        if (room->askForSkillInvoke(player, objectName(), QVariant::fromValue(x))) {
            room->removePlayerMark(player, "@nosrnsubstitute");
            player->broadcastSkillInvoke(objectName());
            //room->doLightbox("$NJieTishenAnimate");

            room->recover(player, RecoverStruct(player, NULL, x));
            player->drawCards(x, objectName());
        }
        return false;
    }
};

class Nos2015Paoxiao : public TargetModSkill
{
public:
    Nos2015Paoxiao() : TargetModSkill("nos2015paoxiao")
    {
        frequency = Compulsory;
    }

    virtual int getResidueNum(const Player *from, const Card *, const Player *) const
    {
        if (from->hasSkill(this))
            return 1000;
        else
            return 0;
    }
};

class Shanyu : public DistanceSkill
{
public:
    Shanyu() : DistanceSkill("shanyu")
    {
    }

    virtual int getCorrect(const Player *from, const Player *) const
    {
        if (from->hasSkill(this) && (from->getDefensiveHorse() || from->getOffensiveHorse()))
            return -1;
        else
            return 0;
    }
};

Nostal2013Package::Nostal2013Package()
    : Package("nostal_2013")
{
    General *liubei = new General(this, "nos_2013_liubei$", "shu", 4, true, true);
    liubei->addSkill(new Nos2013Rende);
    liubei->addSkill("jijiang");

    General *huangyueying = new General(this, "nos_2013_huangyueying", "shu", 3, false, true);
    huangyueying->addSkill(new Nos2013Jizhi);
    huangyueying->addSkill(new Nos2013Qicai);

    General *diaochan = new General(this, "nos_2013_diaochan", "qun", 3, false, true);
    diaochan->addSkill("lijian");
    diaochan->addSkill(new Nos2013Biyue);

    General *caoren = new General(this, "nos_2013_caoren", "wei", 4, true, true);
    caoren->addSkill(new Nos2013Jushou);
    caoren->addSkill(new Nos2013Jiewei);

    General *zhangjiao = new General(this, "nos_2013_zhangjiao$", "qun", 3, true, true);
    zhangjiao->addSkill(new Nos2013Leiji);
    zhangjiao->addSkill("olguidao");
    zhangjiao->addSkill("huangtian");

    addMetaObject<Nos2013RendeCard>();
}

Nostal2015Package::Nostal2015Package()
    : Package("nostal_2015")
{
    General *caoren = new General(this, "nos_2015_caoren", "wei", 4, true, true);
    caoren->addSkill(new Nos2015Jushou);

    General *zhangfei = new General(this, "nos_2015_zhangfei", "shu", 4, true, true);
    zhangfei->addSkill(new Nos2015Paoxiao);
}

Nostal2017Package::Nostal2017Package()
    : Package("nostal_2017")
{
    General *xiahoudun = new General(this, "nos_2017_xiahoudun", "wei", 4, true, true);
    xiahoudun->addSkill("ganglie");
    xiahoudun->addSkill(new Nos2017Qingjian);
    xiahoudun->addSkill(new DetachEffectSkill("nos2017qingjian", "nos2017qingjian"));
    related_skills.insertMulti("nos2017qingjian", "#nos2017qingjian-clear");
    addMetaObject<Nos2017QingjianAllotCard>();

    skills << new Nos2017QingjianAllot;
}

Nostal2020Package::Nostal2020Package()
    : Package("nostal_2020")
{
    General *yuanshao = new General(this, "nos_2020_yuanshao$", "qun", 4, true, true);
    yuanshao->addSkill(new Nos2020Luanji);
    yuanshao->addSkill("molxueyi");
}

NostalRenewPackage::NostalRenewPackage()
    : Package("nostal_renew")
{
    General *guanyu = new General(this, "nos_rn_guanyu", "shu", 4, true, true);
    guanyu->addSkill("wusheng");

    General *zhangfei = new General(this, "nos_rn_zhangfei", "shu", 4, true, true);
    zhangfei->addSkill("nos2015paoxiao");
    zhangfei->addSkill(new NosRnTishen);

    General *lvbu = new General(this, "nos_rn_lvbu", "qun", 5, true, true);
    lvbu->addSkill("wushuang");
    lvbu->addSkill(new Shanyu);
}

ADD_PACKAGE(Nostal2013)
ADD_PACKAGE(Nostal2015)
ADD_PACKAGE(Nostal2017)
ADD_PACKAGE(Nostal2020)
ADD_PACKAGE(NostalRenew)
