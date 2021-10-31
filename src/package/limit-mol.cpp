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
#include "limit-mol.h"

MOLQingjianAllotCard::MOLQingjianAllotCard()
{
    will_throw = false;
    mute = true;
    handling_method = Card::MethodNone;
}

void MOLQingjianAllotCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    QList<int> rende_list = StringList2IntList(source->property("molqingjian_give").toString().split("+"));
    foreach (int id, subcards) {
        rende_list.removeOne(id);
    }
    room->setPlayerProperty(source, "molqingjian_give", IntList2StringList(rende_list).join("+"));

    QList<int> give_list = StringList2IntList(target->property("rende_give").toString().split("+"));
    foreach (int id, subcards) {
        give_list.append(id);
    }
    room->setPlayerProperty(target, "rende_give", IntList2StringList(give_list).join("+"));
}

class MOLQingjianAllot : public ViewAsSkill
{
public:
    MOLQingjianAllot() : ViewAsSkill("molqingjian_allot")
    {
        expand_pile = "molqingjian";
        response_pattern = "@@molqingjian_allot!";
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        QList<int> molqingjian_list = StringList2IntList(Self->property("molqingjian_give").toString().split("+"));
        return molqingjian_list.contains(to_select->getEffectiveId());
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty()) return NULL;
        MOLQingjianAllotCard *molqingjian_card = new MOLQingjianAllotCard;
        molqingjian_card->addSubcards(cards);
        return molqingjian_card;
    }
};

class MOLQingjianViewAsSkill : public ViewAsSkill
{
public:
    MOLQingjianViewAsSkill() : ViewAsSkill("molqingjian")
    {
        response_pattern = "@@molqingjian";
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        QList<int> molqingjian_list = StringList2IntList(Self->property("molqingjian").toString().split("+"));
        return molqingjian_list.contains(to_select->getEffectiveId());
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

class MOLQingjian : public TriggerSkill
{
public:
    MOLQingjian() : TriggerSkill("molqingjian")
    {
        events << CardsMoveOneTime << EventPhaseChanging;
        view_as_skill = new MOLQingjianViewAsSkill;
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
                if (xiahou->getPile("molqingjian").length() > 0 && TriggerSkill::triggerable(xiahou)) {
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
            room->setPlayerProperty(player, "molqingjian", IntList2StringList(ids).join("+"));
            const Card *card = room->askForCard(player, "@@molqingjian", "@molqingjian-put", data, Card::MethodNone);
            room->setPlayerProperty(player, "molqingjian", QString());
            if (card) {
                LogMessage log;
                log.type = "#InvokeSkill";
                log.arg = objectName();
                log.from = player;
                room->sendLog(log);
                room->notifySkillInvoked(player, objectName());
                player->broadcastSkillInvoke(objectName());
                player->addToPile("molqingjian", card, false);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive) return false;
            if (xiahou->getPile("molqingjian").length() > 0 && TriggerSkill::triggerable(xiahou)) {
                bool will_draw = xiahou->getPile("molqingjian").length() > 1;
                room->sendCompulsoryTriggerLog(xiahou, objectName());
                xiahou->broadcastSkillInvoke(objectName());
                QList<int> ids = xiahou->getPile("molqingjian");
                room->setPlayerProperty(xiahou, "molqingjian_give", IntList2StringList(ids).join("+"));
                do {
                    const Card *use = room->askForUseCard(xiahou, "@@molqingjian_allot!", "@molqingjian-give", QVariant(), Card::MethodNone);
                    ids = StringList2IntList(xiahou->property("molqingjian_give").toString().split("+"));
                    if (use == NULL) {
                        MOLQingjianAllotCard *molqingjian_card = new MOLQingjianAllotCard;
                        molqingjian_card->addSubcards(ids);
                        QList<ServerPlayer *> targets;
                        targets << room->getOtherPlayers(xiahou).first();
                        molqingjian_card->use(room, xiahou, targets);
                        delete molqingjian_card;
                        break;
                    }
                } while (!ids.isEmpty() && xiahou->isAlive());
                room->setPlayerProperty(xiahou, "molqingjian_give", QString());
                QList<CardsMoveStruct> moves;
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    QList<int> give_list = StringList2IntList(p->property("rende_give").toString().split("+"));
                    if (give_list.isEmpty())
                        continue;
                    room->setPlayerProperty(p, "rende_give", QString());
                    CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, p->objectName(), "molqingjian", QString());
                    CardsMoveStruct move(give_list, p, Player::PlaceHand, reason);
                    moves.append(move);
                }
                if (!moves.isEmpty())
                    room->moveCardsAtomic(moves, false);
                if (will_draw)
                    room->drawCards(xiahou, 1);
            }
        }
        return false;
    }
};

class MOLJizhi : public TriggerSkill
{
public:
    MOLJizhi() : TriggerSkill("moljizhi")
    {
        frequency = Frequent;
        events << CardUsed << EventPhaseStart;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::NotActive) {
            QList<ServerPlayer *> all_players = room->getAllPlayers();
            foreach (ServerPlayer *p, all_players) {
                room->setPlayerMark(p, "#moljizhi", 0);
            }
        }
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent != CardUsed || !TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card && use.card->isNDTrick())
            return nameList();
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *yueying, QVariant &, ServerPlayer *) const
    {
        if (room->askForSkillInvoke(yueying, objectName())) {
            yueying->broadcastSkillInvoke(objectName());

            bool from_up = true;
            if (yueying->hasSkill("cunmu")) {
                room->sendCompulsoryTriggerLog(yueying, "cunmu");
                yueying->broadcastSkillInvoke("cunmu");
                from_up = false;
            }
            int id = room->drawCard(from_up);
            const Card *card = Sanguosha->getCard(id);
            CardMoveReason reason(CardMoveReason::S_REASON_DRAW, yueying->objectName(), objectName(), QString());
            room->obtainCard(yueying, card, reason, false);
            if (card->getTypeId() == Card::TypeBasic && yueying->handCards().contains(id)
                    && room->askForChoice(yueying, objectName(), "yes+no", QVariant::fromValue(card), "@moljizhi-discard:::" + card->objectName()) == "yes") {
                room->throwCard(card, yueying);
                room->addPlayerMark(yueying, "#moljizhi");
                room->addPlayerMark(yueying, "Global_MaxcardsIncrease");
            }
        }
        return false;
    }
};

class MOLJieweiViewAsSkill : public OneCardViewAsSkill
{
public:
    MOLJieweiViewAsSkill() : OneCardViewAsSkill("moljiewei")
    {
        filter_pattern = ".|.|.|equipped";
        response_or_use = true;
        response_pattern = "nullification";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Nullification *ncard = new Nullification(originalCard->getSuit(), originalCard->getNumber());
        ncard->addSubcard(originalCard);
        ncard->setSkillName(objectName());
        return ncard;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        return player->hasEquip();
    }
};

class MOLJiewei : public TriggerSkill
{
public:
    MOLJiewei() : TriggerSkill("moljiewei")
    {
        events << TurnedOver;
        view_as_skill = new MOLJieweiViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->faceUp() && !target->isNude();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (room->askForCard(player, ".", "@jiewei", data, objectName()))
            room->askForUseCard(player, "@@jiewei_move", "@jiewei-move");
        return false;
    }
};

MOLQiangxiCard::MOLQiangxiCard()
{
}

bool MOLQiangxiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty() || to_select == Self)
        return false;

    QStringList molqiangxi_prop = Self->property("molqiangxi").toString().split("+");
    if (molqiangxi_prop.contains(to_select->objectName()))
        return false;

    int rangefix = 0;
    if (!subcards.isEmpty() && Self->getWeapon() && Self->getWeapon()->getId() == subcards.first()) {
        const Weapon *card = qobject_cast<const Weapon *>(Self->getWeapon()->getRealCard());
        rangefix += card->getRange() - Self->getAttackRange(false);;
    }

    return Self->inMyAttackRange(to_select, rangefix);
}

void MOLQiangxiCard::extraCost(Room *room, const CardUseStruct &card_use) const
{
    if (subcards.isEmpty())
        room->loseHp(card_use.from);
    else {
        CardMoveReason reason(CardMoveReason::S_REASON_THROW, card_use.from->objectName(), QString(), card_use.card->getSkillName(), QString());
        room->moveCardTo(this, NULL, Player::DiscardPile, reason, true);
    }
}

void MOLQiangxiCard::onEffect(const CardEffectStruct &effect) const
{
    QSet<QString> molqiangxi_prop = effect.from->property("molqiangxi").toString().split("+").toSet();
    molqiangxi_prop.insert(effect.to->objectName());
    effect.from->getRoom()->setPlayerProperty(effect.from, "molqiangxi", QStringList(molqiangxi_prop.toList()).join("+"));
    effect.to->getRoom()->damage(DamageStruct("molqiangxi", effect.from, effect.to));
}

class MOLQiangxiViewAsSkill : public ViewAsSkill
{
public:
    MOLQiangxiViewAsSkill() : ViewAsSkill("molqiangxi")
    {
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return true;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.isEmpty() && to_select->isKindOf("Weapon") && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return new MOLQiangxiCard;
        else if (cards.length() == 1) {
            MOLQiangxiCard *card = new MOLQiangxiCard;
            card->addSubcards(cards);

            return card;
        } else
            return NULL;
    }
};

class MOLQiangxi : public TriggerSkill
{
public:
    MOLQiangxi() : TriggerSkill("molqiangxi")
    {
        events << EventPhaseChanging;
        view_as_skill = new MOLQiangxiViewAsSkill;
    }

    bool triggerable(const ServerPlayer *) const
    {
        return false;
    }

    void record(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (data.value<PhaseChangeStruct>().from == Player::Play) {
            room->setPlayerProperty(player, "molqiangxi", QVariant());
        }
    }
};

MOLNiepanCard::MOLNiepanCard()
{
    target_fixed = true;
}

void MOLNiepanCard::extraCost(Room *room, const CardUseStruct &card_use) const
{
    room->removePlayerMark(card_use.from, "@molnirvana");
}

void MOLNiepanCard::use(Room *room, ServerPlayer *pangtong, QList<ServerPlayer *> &) const
{
    doNiepan(room, pangtong);
}

void MOLNiepanCard::doNiepan(Room *room, ServerPlayer *pangtong)
{
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

    pangtong->drawCards(3, "molniepan");
    room->recover(pangtong, RecoverStruct(pangtong, NULL, 3 - pangtong->getHp()));
}

class MOLNiepanViewAsSkill : public ZeroCardViewAsSkill
{
public:
    MOLNiepanViewAsSkill() : ZeroCardViewAsSkill("molniepan")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@molnirvana") > 0;
    }

    virtual const Card *viewAs() const
    {
        return new MOLNiepanCard;
    }
};

class MOLNiepan : public TriggerSkill
{
public:
    MOLNiepan() : TriggerSkill("molniepan")
    {
        events << AskForPeaches;
        frequency = Limited;
        limit_mark = "@molnirvana";
        view_as_skill = new MOLNiepanViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *target, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(target) && target->getMark("@molnirvana") > 0) {
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

            room->removePlayerMark(pangtong, "@molnirvana");

            MOLNiepanCard::doNiepan(room, pangtong);
        }

        return false;
    }
};

class MOLLuanjiViewAsSkill : public ViewAsSkill
{
public:
    MOLLuanjiViewAsSkill() : ViewAsSkill("molluanji")
    {
        response_or_use = true;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        QSet<QString> used = Self->property("molluanjiUsedSuits").toString().split("+").toSet();
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

class MOLLuanji : public TriggerSkill
{
public:
    MOLLuanji() : TriggerSkill("molluanji")
    {
        events << PreCardUsed << EventPhaseChanging << TargetSpecified << Damage << CardResponded << CardFinished;
        view_as_skill = new MOLLuanjiViewAsSkill;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        switch (triggerEvent) {
        case PreCardUsed: {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card && use.card->isKindOf("ArcheryAttack") && use.card->getSkillName() == objectName()) {
                QSet<QString> used = player->property("nos2013luanjiUsedSuits").toString().split("+").toSet();
                foreach (int id, use.card->getSubcards())
                    used.insert(Sanguosha->getCard(id)->getSuitString());
                room->setPlayerProperty(player, "nos2013luanjiUsedSuits", QStringList(used.toList()).join("+"));
            }
            break;
        }
        case EventPhaseChanging: {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                room->setPlayerProperty(player, "molluanjiUsedSuits", QVariant());
            }
            break;
        }
        case TargetSpecified: {
            int cnt = 0;
            CardUseStruct use = data.value<CardUseStruct>();
            foreach (ServerPlayer *p, room->getAllPlayers())
                if (use.to.contains(p))
                    ++cnt;
            use.card->tag["molluanjiTargetsCount"] = cnt;
            break;
        }
        case Damage: {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("ArcheryAttack") && damage.card->getSkillName() == objectName()) {
                damage.card->tag["molluanjiDamageCount"] = damage.card->tag["molluanjiDamageCount"].toInt() + 1;
            }
            break;
        }
        default:
            break;
        }
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
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
                if (use.card->tag["molluanjiDamageCount"].toInt())
                    if (use.from)
                        list.insert(use.from, comList());
            }
        }
        return list;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *yuanshao) const
    {
        if (triggerEvent == CardResponded) {
            room->drawCards(player, 1, objectName());
        } else if (triggerEvent == CardFinished) {
            yuanshao->broadcastSkillInvoke(objectName());
            CardUseStruct use = data.value<CardUseStruct>();
            room->drawCards(yuanshao, use.card->tag["molluanjiTargetsCount"].toInt(), objectName());
        }
        return false;
    }
};

class MOLXueyi : public MaxCardsSkill
{
public:
    MOLXueyi() : MaxCardsSkill("molxueyi$")
    {
    }

    virtual int getExtra(const Player *target) const
    {
        if (target->hasLordSkill(this)) {
            int extra = 0;
            QList<const Player *> players = target->getAliveSiblings();
            foreach (const Player *player, players) {
                if (player->getKingdom() == "qun")
                    extra += 2;
            }
            return extra;
        } else
            return 0;
    }
};

class MOLZaiqi : public TriggerSkill
{
public:
    MOLZaiqi() : TriggerSkill("molzaiqi")
    {
        events << CardsMoveOneTime << EventPhaseEnd << EventPhaseChanging;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardsMoveOneTime && player->getPhase() != Player::NotActive) {
            int x = 0;
            QVariantList list = data.toList();
            foreach (QVariant qvar, list) {
                CardsMoveOneTimeStruct move = qvar.value<CardsMoveOneTimeStruct>();
                if (move.to_place == Player::DiscardPile) {
                    foreach (int id, move.card_ids) {
                        if (Sanguosha->getCard(id)->getColor() == Card::Red)
                            ++x;
                    }
                }
            }
            room->setTag("MOLZaiqiRecord", room->getTag("MOLZaiqiRecord").toInt() + x);
        } else if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                room->removeTag("MOLZaiqiRecord");
            }
        }
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseEnd && TriggerSkill::triggerable(player) &&
                player->getPhase() == Player::Discard && room->getTag("MOLZaiqiRecord").toInt())
            return nameList();
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        int x = room->getTag("MOLZaiqiRecord").toInt();
        QList<ServerPlayer *> targets = room->askForPlayersChosen(player, room->getAlivePlayers(), objectName(), 0, x, QString(), true);
        foreach (ServerPlayer *p, targets) {
            if (!player->isWounded() ||
                    room->askForChoice(p, objectName(), "draw+recover", QVariant::fromValue(player), "@molzaiqi-choose:" + player->objectName()) == "draw")
                room->drawCards(p, 1);
            else {
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, p->objectName(), player->objectName());
                room->recover(player, RecoverStruct(p));
            }
        }
        return false;
    }
};

class Poluu : public TriggerSkill
{
public:
    Poluu() : TriggerSkill("poluu")
    {
        events << Death;
        //frequency = Frequent;
    }

    QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (player == NULL || !player->hasSkill(this)) return QStringList();
        DeathStruct death = data.value<DeathStruct>();
        if (death.who == player)
            return nameList();
        if (death.damage && death.damage->from == player)
            return nameList();
        return QStringList();
    }

    bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (room->askForSkillInvoke(player, objectName())) {
            room->addPlayerMark(player, "#poluu");
            int x = player->getMark("#poluu");
            QList<ServerPlayer *> targets =
                room->askForPlayersChosen(player, room->getAlivePlayers(), objectName(), 1,
                                          room->getAlivePlayers().size(), "@poluu-invoke:::" + QString::number(x));
            foreach (ServerPlayer *target, targets) {
                room->drawCards(target, x);
            }
        }
        return false;
    }
};

class MOLJiuchiViewAsSkill : public OneCardViewAsSkill
{
public:
    MOLJiuchiViewAsSkill() : OneCardViewAsSkill("moljiuchi")
    {
        filter_pattern = ".|spade|.|hand";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return hasAvailable(player) || Analeptic::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return  pattern.contains("analeptic");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Analeptic *analeptic = new Analeptic(originalCard->getSuit(), originalCard->getNumber());
        analeptic->setSkillName(objectName());
        analeptic->addSubcard(originalCard->getId());
        return analeptic;
    }
};

class MOLJiuchi : public TriggerSkill
{
public:
    MOLJiuchi() : TriggerSkill("moljiuchi")
    {
        events << Damage;
        view_as_skill = new MOLJiuchiViewAsSkill;
    }

    void record(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (!TriggerSkill::triggerable(player)) return;
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Slash") && damage.card->hasFlag("drank"))
            room->setPlayerFlag(player, "noBenghuai");
    }

    bool triggerable(const ServerPlayer *) const
    {
        return false;
    }
};

class MOLBaonue : public TriggerSkill
{
public:
    MOLBaonue() : TriggerSkill("molbaonue$")
    {
        events << Damage;
        view_as_skill = new dummyVS;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (player->isAlive() && player->getKingdom() == "qun") {
            foreach (ServerPlayer *dongzhuo, room->getOtherPlayers(player)) {
                if (dongzhuo->hasLordSkill(objectName()))
                    skill_list.insert(dongzhuo, QStringList("molbaonue!"));
            }
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *dongzhuo) const
    {
        if (room->askForChoice(player, objectName(), "yes+no", data, "@molbaonue-to:" + dongzhuo->objectName()) == "yes") {
            LogMessage log;
            log.type = "#InvokeOthersSkill";
            log.from = player;
            log.to << dongzhuo;
            log.arg = objectName();
            room->sendLog(log);
            room->notifySkillInvoked(dongzhuo, objectName());
            dongzhuo->broadcastSkillInvoke(objectName());

            JudgeStruct judge;
            judge.pattern = ".|spade";
            judge.good = true;
            judge.reason = objectName();
            judge.who = player;

            room->judge(judge);

            if (judge.isGood())
                room->recover(dongzhuo, RecoverStruct(player));
        }
        return false;
    }
};

class MOLTuntian : public TriggerSkill
{
public:
    MOLTuntian() : TriggerSkill("moltuntian")
    {
        events << CardsMoveOneTime << FinishJudge;
        frequency = Frequent;
    }

    int getPriority(TriggerEvent triggerEvent) const
    {
        if (triggerEvent == FinishJudge)
            return 5;
        return TriggerSkill::getPriority(triggerEvent);
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == CardsMoveOneTime && TriggerSkill::triggerable(player) && player->getPhase() == Player::NotActive) {
            QVariantList move_datas = data.toList();
            foreach(QVariant move_data, move_datas) {
                CardsMoveOneTimeStruct move = move_data.value<CardsMoveOneTimeStruct>();
                if (move.from == player && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
                        && !(move.to == player && (move.to_place == Player::PlaceHand || move.to_place == Player::PlaceEquip))) {
                    return nameList();
                }
            }
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == objectName() && room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge)
                return QStringList("moltuntian!");
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == CardsMoveOneTime) {
            if (player->askForSkillInvoke(objectName(), data)) {
                player->broadcastSkillInvoke(objectName());
                JudgeStruct judge;
                judge.pattern = ".|heart";
                judge.good = false;
                judge.reason = objectName();
                judge.who = player;
                room->judge(judge);
            }
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->card && room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge)
                if (judge->isGood())
                    player->addToPile("molfield", judge->card->getEffectiveId());
                else
                    player->obtainCard(judge->card);
        }

        return false;
    }
};

class MOLTuntianDistance : public DistanceSkill
{
public:
    MOLTuntianDistance() : DistanceSkill("#moltuntian-dist")
    {
    }

    virtual int getCorrect(const Player *from, const Player *) const
    {
        if (from->hasSkill("moltuntian"))
            return -from->getPile("molfield").length();
        else
            return 0;
    }
};

class MOLZaoxian : public PhaseChangeSkill
{
public:
    MOLZaoxian() : PhaseChangeSkill("molzaoxian")
    {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target)
               && target->getPhase() == Player::Start
               && target->getMark("molzaoxian") == 0
               && target->getPile("molfield").length() >= 3;
    }

    virtual bool onPhaseChange(ServerPlayer *dengai) const
    {
        Room *room = dengai->getRoom();
        room->sendCompulsoryTriggerLog(dengai, objectName());

        dengai->broadcastSkillInvoke(objectName());
        //room->doLightbox("$MOLZaoxianAnimate", 4000);

        room->setPlayerMark(dengai, "molzaoxian", 1);
        if (room->changeMaxHpForAwakenSkill(dengai) && dengai->getMark("molzaoxian") == 1)
            room->acquireSkill(dengai, "moljixi");

        return false;
    }
};

class MOLJixi : public OneCardViewAsSkill
{
public:
    MOLJixi() : OneCardViewAsSkill("moljixi")
    {
        filter_pattern = ".|.|.|molfield";
        expand_pile = "molfield";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->getPile("molfield").isEmpty();
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Snatch *snatch = new Snatch(originalCard->getSuit(), originalCard->getNumber());
        snatch->addSubcard(originalCard);
        snatch->setSkillName(objectName());
        return snatch;
    }
};

class MOLFangquan : public TriggerSkill
{
public:
    MOLFangquan() : TriggerSkill("molfangquan")
    {
        events << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player)) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Play && !player->isSkipped(Player::Play)) {
                return nameList();
            } else if (change.to == Player::NotActive && player->hasFlag("molfangquanInvoked"))
                return QStringList("molfangquan!");
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *liushan, QVariant &data, ServerPlayer *) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Play && liushan->askForSkillInvoke(this)) {
            liushan->broadcastSkillInvoke(objectName(), 1);
            liushan->skip(Player::Play, true);
            liushan->setFlags("molfangquanInvoked");
        } else if (change.to == Player::NotActive) {
            room->sendCompulsoryTriggerLog(liushan, objectName());
            liushan->broadcastSkillInvoke(objectName(), 2);
            room->askForUseCard(liushan, "@@fangquanask", "@fangquan-give", data, Card::MethodDiscard);
        }
        return false;
    }
};

class MOLFangquanMaxCards : public MaxCardsSkill
{
public:
    MOLFangquanMaxCards() : MaxCardsSkill("#molfangquan")
    {

    }

    virtual int getFixed(const Player *target) const
    {
        if (target->hasSkill("molfangquan") && target->hasFlag("molfangquanInvoked"))
            return target->getMaxHp();
        else
            return -1;
    }
};

class MOLRuoyu : public PhaseChangeSkill
{
public:
    MOLRuoyu() : PhaseChangeSkill("molruoyu$")
    {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        if (target != NULL && target->isAlive() && target->hasLordSkill("molruoyu") && target->getMark("molruoyu") == 0 && target->getPhase() == Player::Start) {
            QList<ServerPlayer *> players = target->getRoom()->getAlivePlayers();
            foreach (ServerPlayer *p, players) {
                if (target->getHp() > p->getHp()) return false;
            }
            return true;
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *liushan) const
    {
        Room *room = liushan->getRoom();

        room->sendCompulsoryTriggerLog(liushan, objectName());

        liushan->broadcastSkillInvoke(objectName());

        room->setPlayerMark(liushan, "molruoyu", 1);
        if (room->changeMaxHpForAwakenSkill(liushan, 1)) {
            room->recover(liushan, RecoverStruct(liushan));
            room->acquireSkill(liushan, "jijiang");
        }

        return false;
    }
};

class MOLHunzi : public PhaseChangeSkill
{
public:
    MOLHunzi() : PhaseChangeSkill("molhunzi")
    {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target)
               && target->getMark("molhunzi") == 0
               && target->getPhase() == Player::Start
               && target->getHp() < 3;
    }

    virtual bool onPhaseChange(ServerPlayer *sunce) const
    {
        Room *room = sunce->getRoom();
        room->sendCompulsoryTriggerLog(sunce, objectName());

        sunce->broadcastSkillInvoke(objectName());
        //room->doLightbox("$HunziAnimate", 5000);

        room->setPlayerMark(sunce, "molhunzi", 1);
        room->setPlayerMark(sunce, "ZhibaReject", 1);
        if (room->changeMaxHpForAwakenSkill(sunce) && sunce->getMark("molhunzi") == 1) {
            room->handleAcquireDetachSkills(sunce, "yingzi|yinghun");
        }
        return false;
    }
};

MOLZhibaCard::MOLZhibaCard()
{
    mute = true;
    m_skillName = "molzhiba_pindian";
}

bool MOLZhibaCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->hasLordSkill("molzhiba") && Self->canPindian(to_select) && !to_select->hasFlag("MOLZhibaInvoked");
}

void MOLZhibaCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *sunce = targets.first();
    LogMessage log;
    log.type = "#InvokeOthersSkill";
    log.from = source;
    log.to << sunce;
    log.arg = "molzhiba";
    room->sendLog(log);
    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), sunce->objectName());
    room->setPlayerFlag(sunce, "MOLZhibaInvoked");
    room->notifySkillInvoked(sunce, "molzhiba");
    sunce->broadcastSkillInvoke("molzhiba");
    if (sunce->getMark("ZhibaReject") > 0 && room->askForChoice(sunce, "molzhiba_pindian", "yes+no", QVariant(), "@molzhiba-pindianstart" + source->objectName()) == "no") {
        LogMessage log;
        log.type = "#ZhibaReject";
        log.from = sunce;
        log.to << source;
        log.arg = "molzhiba";
        room->sendLog(log);

        return;
    }

    source->pindian(sunce, "molzhiba");
}

class MOLZhibaPindian : public ZeroCardViewAsSkill
{
public:
    MOLZhibaPindian() : ZeroCardViewAsSkill("molzhiba_pindian")
    {
        attached_lord_skill = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (!shouldBeVisible(player) || player->isKongcheng()) return false;
        foreach(const Player *sib, player->getAliveSiblings())
            if (sib->hasLordSkill("molzhiba") && !sib->hasFlag("MOLZhibaInvoked") && !sib->isKongcheng())
                return true;
        return false;
    }

    virtual bool shouldBeVisible(const Player *Self) const
    {
        return Self && Self->getKingdom() == "wu";
    }

    virtual const Card *viewAs() const
    {
        return new MOLZhibaCard;
    }
};

class MOLZhiba : public TriggerSkill
{
public:
    MOLZhiba() : TriggerSkill("molzhiba$")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << Pindian << EventPhaseChanging;
        view_as_skill = new dummyVS;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList list;
        if (player == NULL) return list;
        if (triggerEvent == Pindian) {
            PindianStruct *pindian = data.value<PindianStruct *>();
            if (pindian->reason == "molzhiba") {
                if (!pindian->isSuccess()) {
                    bool on_table = false;
                    if (room->getCardPlace(pindian->from_card->getEffectiveId()) == Player::PlaceTable)
                        on_table = true;
                    if (room->getCardPlace(pindian->to_card->getEffectiveId()) == Player::PlaceTable)
                        on_table = true;
                    if (on_table)
                        list.insert(pindian->to, nameList());
                }
            }
        }
        return list;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player == NULL) return;
        if ((triggerEvent == GameStart && player->isLord())
                || (triggerEvent == EventAcquireSkill && data.toString() == "molzhiba")) {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(this))
                    lords << p;
            }
            if (lords.isEmpty()) return;

            QList<ServerPlayer *> players;
            if (lords.length() > 1)
                players = room->getAlivePlayers();
            else
                players = room->getOtherPlayers(lords.first());
            foreach (ServerPlayer *p, players) {
                if (!p->hasSkill("molzhiba_pindian"))
                    room->attachSkillToPlayer(p, "molzhiba_pindian");
            }
        } else if (triggerEvent == EventLoseSkill && data.toString() == "molzhiba") {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(this))
                    lords << p;
            }
            if (lords.length() > 2) return;

            QList<ServerPlayer *> players;
            if (lords.isEmpty())
                players = room->getAlivePlayers();
            else
                players << lords.first();
            foreach (ServerPlayer *p, players) {
                if (p->hasSkill("molzhiba_pindian"))
                    room->detachSkillFromPlayer(p, "molzhiba_pindian", true);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from != Player::Play)
                return;
            QList<ServerPlayer *> players = room->getOtherPlayers(player);
            foreach (ServerPlayer *p, players) {
                if (p->hasFlag("MOLZhibaInvoked"))
                    room->setPlayerFlag(p, "-MOLZhibaInvoked");
            }
        }
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *sunce) const
    {
        PindianStruct *pindian = data.value<PindianStruct *>();
        DummyCard *dummy = new DummyCard;
        if (room->getCardPlace(pindian->from_card->getEffectiveId()) == Player::PlaceTable)
            dummy->addSubcard(pindian->from_card);
        if (room->getCardPlace(pindian->to_card->getEffectiveId()) == Player::PlaceTable)
            dummy->addSubcard(pindian->to_card);
        if (room->askForChoice(sunce, "molzhiba", "yes+no", data, "@molzhiba-pindianfinish") == "yes")
            sunce->obtainCard(dummy);
        delete dummy;
        return false;
    }
};

LimitMOLPackage::LimitMOLPackage()
    : Package("limit_mol")
{
    General *xiahoudun = new General(this, "mol_xiahoudun", "wei", 4, true, true);
    xiahoudun->addSkill("ganglie");
    xiahoudun->addSkill(new MOLQingjian);
    xiahoudun->addSkill(new DetachEffectSkill("molqingjian", "molqingjian"));
    related_skills.insertMulti("molqingjian", "#molqingjian-clear");

    General *huangyueying = new General(this, "mol_huangyueying", "shu", 3, false, true);
    huangyueying->addSkill(new MOLJizhi);
    huangyueying->addSkill("qicai");

    General *caoren = new General(this, "mol_caoren", "wei", 4, true, true);
    caoren->addSkill("jushou");
    caoren->addSkill(new MOLJiewei);

    General *dianwei = new General(this, "mol_dianwei", "wei", 4, true, true);
    dianwei->addSkill(new MOLQiangxi);

    General *pangtong = new General(this, "mol_pangtong", "shu", 3, true, true);
    pangtong->addSkill("lianhuan");
    //pangtong->addSkill("#lianhuan-target");
    pangtong->addSkill(new MOLNiepan);

    General *wolong = new General(this, "mol_wolong", "shu", 3, true, true);
    wolong->addSkill("bazhen");
    wolong->addSkill("huoji");
    wolong->addSkill("kanpo");

    General *yuanshao = new General(this, "mol_yuanshao$", "qun", 4, true, true);
    yuanshao->addSkill(new MOLLuanji);
    yuanshao->addSkill(new MOLXueyi);

    General *menghuo = new General(this, "mol_menghuo", "shu", 4, true, true);
    menghuo->addSkill(new MOLZaiqi);
    menghuo->addSkill("huoshou");

    General *sunjian = new General(this, "mol_sunjian", "wu", 4, true, true);
    sunjian->addSkill("yinghun");
    sunjian->addSkill(new Poluu);

    General *dongzhuo = new General(this, "mol_dongzhuo", "qun", 8, true, true);
    dongzhuo->addSkill(new MOLJiuchi);
    dongzhuo->addSkill("roulin");
    dongzhuo->addSkill("benghuai");
    dongzhuo->addSkill(new MOLBaonue);

    General *dengai = new General(this, "mol_dengai", "wei", 4, true, true);
    dengai->addSkill(new MOLTuntian);
    dengai->addSkill(new MOLTuntianDistance);
    dengai->addSkill(new DetachEffectSkill("moltuntian", "molfield"));
    related_skills.insertMulti("moltuntian", "#moltuntian-dist");
    related_skills.insertMulti("moltuntian", "#moltuntian-clear");
    dengai->addSkill(new MOLZaoxian);
    dengai->addRelateSkill("moljixi");

    General *liushan = new General(this, "mol_liushan$", "shu", 3, true, true);
    liushan->addSkill("xiangle");
    liushan->addSkill(new MOLFangquan);
    liushan->addSkill(new MOLFangquanMaxCards);
    liushan->addSkill(new MOLRuoyu);
    related_skills.insertMulti("molfangquan", "#molfangquan");

    General *sunce = new General(this, "mol_sunce$", "wu", 4, true, true);
    sunce->addSkill("jiang");
    sunce->addSkill(new MOLHunzi);
    sunce->addSkill(new MOLZhiba);

    addMetaObject<MOLQingjianAllotCard>();
    addMetaObject<MOLQiangxiCard>();
    addMetaObject<MOLNiepanCard>();
    addMetaObject<MOLZhibaCard>();
    skills << new MOLQingjianAllot << new MOLJixi << new MOLZhibaPindian;
}

ADD_PACKAGE(LimitMOL)
