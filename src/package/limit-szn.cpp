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
#include "limit-szn.h"

class SznTishen : public TriggerSkill
{
public:
    SznTishen() : TriggerSkill("szntishen")
    {
        events << EventPhaseEnd << EventPhaseStart << CardFinished;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart && player->getMark("#szntishen") > 0)
            room->removePlayerTip(player, "#szntishen");
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (triggerEvent == EventPhaseEnd && TriggerSkill::triggerable(player) && player->getPhase() == Player::Play)
            skill_list.insert(player, nameList());
        else if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card && use.card->isKindOf("Slash") && room->isAllOnPlace(use.card, Player::PlaceTable)) {
                QStringList damaged_tag = use.card->tag["GlobalCardDamagedTag"].toStringList();
                foreach (ServerPlayer *to, use.to) {
                    if (to->getMark("#szntishen") > 0 && !damaged_tag.contains(to->objectName()))
                        skill_list.insert(to, QStringList("szntishen!"));
                }
            }
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *zhangfei) const
    {
        if (triggerEvent == EventPhaseEnd && player->askForSkillInvoke(objectName())) {
            player->broadcastSkillInvoke(objectName());
            DummyCard *dummy = new DummyCard;
            foreach (const Card *card, player->getCards("he")) {
                if (card->getTypeId() == Card::TypeTrick || card->isKindOf("Horse"))
                    dummy->addSubcard(card);
            }
            if (dummy->subcardsLength() > 0)
                room->throwCard(dummy, player);
            delete dummy;
            room->addPlayerTip(player, "#szntishen");
        } else if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            zhangfei->obtainCard(use.card);
        }
        return false;
    }
};

class SznJizhi : public TriggerSkill
{
public:
    SznJizhi() : TriggerSkill("sznjizhi")
    {
        frequency = Frequent;
        events << CardUsed << EventPhaseStart;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::NotActive) {
            QList<ServerPlayer *> all_players = room->getAllPlayers();
            foreach (ServerPlayer *p, all_players) {
                room->setPlayerMark(p, "#sznjizhi", 0);
            }
        }
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent != CardUsed || !TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card && use.card->getTypeId() == Card::TypeTrick)
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
                    && room->askForChoice(yueying, objectName(), "yes+no", QVariant::fromValue(card), "@sznjizhi-discard:::" + card->objectName()) == "yes") {
                room->throwCard(card, yueying);
                room->addPlayerMark(yueying, "#sznjizhi");
                room->addPlayerMark(yueying, "Global_MaxcardsIncrease");
            }
        }
        return false;
    }
};

class SZNJieweiViewAsSkill : public OneCardViewAsSkill
{
public:
    SZNJieweiViewAsSkill() : OneCardViewAsSkill("sznjiewei")
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

SZNJieweiMoveCard::SZNJieweiMoveCard()
{
    mute = true;
}

bool SZNJieweiMoveCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return targets.length() == 2;
}

bool SZNJieweiMoveCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    if (targets.isEmpty())
        return (!to_select->getJudgingArea().isEmpty() || !to_select->getEquips().isEmpty());
    else if (targets.length() == 1) {
        for (int i = 0; i < S_EQUIP_AREA_LENGTH; i++) {
            if (targets.first()->getEquip(i) && to_select->canSetEquip(i))
                return true;
        }
        foreach(const Card *card, targets.first()->getJudgingArea()) {
            if (!Sanguosha->isProhibited(NULL, to_select, card))
                return true;
        }

    }
    return false;
}

void SZNJieweiMoveCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    ServerPlayer *caoren = use.from;
    if (use.to.length() != 2) return;

    ServerPlayer *from = use.to.first();
    ServerPlayer *to = use.to.last();

    QList<int> all, ids, disabled_ids;
    for (int i = 0; i < S_EQUIP_AREA_LENGTH; i++) {
        if (from->getEquip(i)) {
            if (!to->getEquip(i))
                ids << from->getEquip(i)->getEffectiveId();
            else
                disabled_ids << from->getEquip(i)->getEffectiveId();
            all << from->getEquip(i)->getEffectiveId();
        }
    }

    foreach(const Card *card, from->getJudgingArea()) {
        if (!Sanguosha->isProhibited(NULL, to, card))
            ids << card->getEffectiveId();
        else
            disabled_ids << card->getEffectiveId();
        all << card->getEffectiveId();
    }

    room->fillAG(all, caoren, disabled_ids);
    from->setFlags("SZNJieweiTarget");
    int card_id = room->askForAG(caoren, ids, true, "sznjiewei");
    from->setFlags("-SZNJieweiTarget");
    room->clearAG(caoren);

    if (card_id != -1)
        room->moveCardTo(Sanguosha->getCard(card_id), from, to, room->getCardPlace(card_id), CardMoveReason(CardMoveReason::S_REASON_TRANSFER, caoren->objectName(), "sznjiewei", QString()));
}

class SZNJieweiMove : public ZeroCardViewAsSkill
{
public:
    SZNJieweiMove() : ZeroCardViewAsSkill("sznjiewei_move")
    {
        response_pattern = "@@sznjiewei_move";
    }

    virtual const Card *viewAs() const
    {
        return new SZNJieweiMoveCard;
    }
};

class SZNJiewei : public TriggerSkill
{
public:
    SZNJiewei() : TriggerSkill("sznjiewei")
    {
        events << TurnedOver;
        view_as_skill = new SZNJieweiViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->faceUp() && !target->isNude();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (room->askForCard(player, "..", "@sznjiewei", data, objectName()))
            room->askForUseCard(player, "@@sznjiewei_move", "@sznjiewei-move");
        return false;
    }
};

LimitSZNPackage::LimitSZNPackage()
    : Package("limit_szn")
{
    General *zhangfei = new General(this, "szn_zhangfei", "shu", 4, true, true);
    zhangfei->addSkill("paoxiao");
    zhangfei->addSkill(new SznTishen);

    General *huangyueying = new General(this, "szn_huangyueying", "shu", 3, false, true);
    huangyueying->addSkill(new SznJizhi);
    huangyueying->addSkill("qicai");

    General *caoren = new General(this, "szn_caoren", "wei", 4, true, true);
    caoren->addSkill("jushou");
    caoren->addSkill(new SZNJiewei);

    addMetaObject<SZNJieweiMoveCard>();
    skills << new SZNJieweiMove;
}

ADD_PACKAGE(LimitSZN)
