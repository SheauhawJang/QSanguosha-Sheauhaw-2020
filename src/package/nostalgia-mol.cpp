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
#include "nostalgia-mol.h"

NMOLQingjianAllotCard::NMOLQingjianAllotCard()
{
    will_throw = false;
    mute = true;
    handling_method = Card::MethodNone;
}

void NMOLQingjianAllotCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    QList<int> rende_list = StringList2IntList(source->property("nmolqingjian_give").toString().split("+"));
    foreach (int id, subcards) {
        rende_list.removeOne(id);
    }
    room->setPlayerProperty(source, "nmolqingjian_give", IntList2StringList(rende_list).join("+"));

    QList<int> give_list = StringList2IntList(target->property("rende_give").toString().split("+"));
    foreach (int id, subcards) {
        give_list.append(id);
    }
    room->setPlayerProperty(target, "rende_give", IntList2StringList(give_list).join("+"));
}

class NMOLQingjianAllot : public ViewAsSkill
{
public:
    NMOLQingjianAllot() : ViewAsSkill("nmolqingjian_allot")
    {
        expand_pile = "nmolqingjian";
        response_pattern = "@@nmolqingjian_allot!";
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        QList<int> nmolqingjian_list = StringList2IntList(Self->property("nmolqingjian_give").toString().split("+"));
        return nmolqingjian_list.contains(to_select->getEffectiveId());
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty()) return NULL;
        NMOLQingjianAllotCard *nmolqingjian_card = new NMOLQingjianAllotCard;
        nmolqingjian_card->addSubcards(cards);
        return nmolqingjian_card;
    }
};

class NMOLQingjianViewAsSkill : public ViewAsSkill
{
public:
    NMOLQingjianViewAsSkill() : ViewAsSkill("nmolqingjian")
    {
        response_pattern = "@@nmolqingjian";
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        QList<int> nmolqingjian_list = StringList2IntList(Self->property("nmolqingjian").toString().split("+"));
        return nmolqingjian_list.contains(to_select->getEffectiveId());
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

class NMOLQingjian : public TriggerSkill
{
public:
    NMOLQingjian() : TriggerSkill("nmolqingjian")
    {
        events << CardsMoveOneTime << EventPhaseChanging;
        view_as_skill = new NMOLQingjianViewAsSkill;
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
                if (xiahou->getPile("nmolqingjian").length() > 0 && TriggerSkill::triggerable(xiahou)) {
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
            room->setPlayerProperty(player, "nmolqingjian", IntList2StringList(ids).join("+"));
            const Card *card = room->askForCard(player, "@@nmolqingjian", "@nmolqingjian-put", data, Card::MethodNone);
            room->setPlayerProperty(player, "nmolqingjian", QString());
            if (card) {
                LogMessage log;
                log.type = "#InvokeSkill";
                log.arg = objectName();
                log.from = player;
                room->sendLog(log);
                room->notifySkillInvoked(player, objectName());
                player->broadcastSkillInvoke(objectName());
                player->addToPile("nmolqingjian", card, false);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive) return false;
            if (xiahou->getPile("nmolqingjian").length() > 0 && TriggerSkill::triggerable(xiahou)) {
                bool will_draw = xiahou->getPile("nmolqingjian").length() > 1;
                room->sendCompulsoryTriggerLog(xiahou, objectName());
                xiahou->broadcastSkillInvoke(objectName());
                QList<int> ids = xiahou->getPile("nmolqingjian");
                room->setPlayerProperty(xiahou, "nmolqingjian_give", IntList2StringList(ids).join("+"));
                do {
                    const Card *use = room->askForUseCard(xiahou, "@@nmolqingjian_allot!", "@nmolqingjian-give", QVariant(), Card::MethodNone);
                    ids = StringList2IntList(xiahou->property("nmolqingjian_give").toString().split("+"));
                    if (use == NULL) {
                        NMOLQingjianAllotCard *nmolqingjian_card = new NMOLQingjianAllotCard;
                        nmolqingjian_card->addSubcards(ids);
                        QList<ServerPlayer *> targets;
                        targets << room->getOtherPlayers(xiahou).first();
                        nmolqingjian_card->use(room, xiahou, targets);
                        delete nmolqingjian_card;
                        break;
                    }
                } while (!ids.isEmpty() && xiahou->isAlive());
                room->setPlayerProperty(xiahou, "nmolqingjian_give", QString());
                QList<CardsMoveStruct> moves;
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    QList<int> give_list = StringList2IntList(p->property("rende_give").toString().split("+"));
                    if (give_list.isEmpty())
                        continue;
                    room->setPlayerProperty(p, "rende_give", QString());
                    CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, p->objectName(), "nmolqingjian", QString());
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

class NMOLJizhi : public TriggerSkill
{
public:
    NMOLJizhi() : TriggerSkill("nmoljizhi")
    {
        frequency = Frequent;
        events << CardUsed << EventPhaseStart;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::NotActive) {
            QList<ServerPlayer *> all_players = room->getAllPlayers();
            foreach (ServerPlayer *p, all_players) {
                room->setPlayerMark(p, "#nmoljizhi", 0);
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
                    && room->askForChoice(yueying, objectName(), "yes+no", QVariant::fromValue(card), "@nmoljizhi-discard:::" + card->objectName()) == "yes") {
                room->throwCard(card, yueying);
                room->addPlayerMark(yueying, "#nmoljizhi");
                room->addPlayerMark(yueying, "Global_MaxcardsIncrease");
            }
        }
        return false;
    }
};

class NMOLJieweiViewAsSkill : public OneCardViewAsSkill
{
public:
    NMOLJieweiViewAsSkill() : OneCardViewAsSkill("nmoljiewei")
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

class NMOLJiewei : public TriggerSkill
{
public:
    NMOLJiewei() : TriggerSkill("nmoljiewei")
    {
        events << TurnedOver;
        view_as_skill = new NMOLJieweiViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->faceUp() && !target->isNude();
    }

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (room->askForCard(player, ".", "@jiewei", data, objectName()))
            room->askForUseCard(player, "@@jiewei_move", "@jiewei-move");
        return false;
    }
};

NostalMOLPackage::NostalMOLPackage()
    : Package("nostalgia_mol")
{
    General *xiahoudun = new General(this, "nmol_xiahoudun", "wei", 4, true, true);
    xiahoudun->addSkill("ganglie");
    xiahoudun->addSkill(new NMOLQingjian);
    xiahoudun->addSkill(new DetachEffectSkill("nmolqingjian", "nmolqingjian"));
    related_skills.insertMulti("nmolqingjian", "#nmolqingjian-clear");

    General *huangyueying = new General(this, "nmol_huangyueying", "shu", 3, false, true);
    huangyueying->addSkill(new NMOLJizhi);
    huangyueying->addSkill("qicai");

    General *caoren = new General(this, "nmol_caoren", "wei", 4, true, true);
    caoren->addSkill("jushou");
    caoren->addSkill(new NMOLJiewei);


    addMetaObject<NMOLQingjianAllotCard>();
    skills << new NMOLQingjianAllot;
}

ADD_PACKAGE(NostalMOL)
