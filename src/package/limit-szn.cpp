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

LimitSZNPackage::LimitSZNPackage()
    : Package("limit_szn")
{
    General *zhangfei = new General(this, "szn_zhangfei", "shu", 4, true, true);
    zhangfei->addSkill("paoxiao");
    zhangfei->addSkill(new SznTishen);

    General *huangyueying = new General(this, "szn_huangyueying", "shu", 3, false, true);
    huangyueying->addSkill(new SznJizhi);
    huangyueying->addSkill("qicai");
}

ADD_PACKAGE(LimitSZN)
