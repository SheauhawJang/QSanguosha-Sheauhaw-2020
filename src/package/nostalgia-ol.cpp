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
#include "nostalgia-ol.h"

NOLQingjianAllotCard::NOLQingjianAllotCard()
{
    will_throw = false;
    mute = true;
    handling_method = Card::MethodNone;
}

void NOLQingjianAllotCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    QList<int> rende_list = StringList2IntList(source->property("nolqingjian_give").toString().split("+"));
    foreach (int id, subcards) {
        rende_list.removeOne(id);
    }
    room->setPlayerProperty(source, "nolqingjian_give", IntList2StringList(rende_list).join("+"));

    QList<int> give_list = StringList2IntList(target->property("rende_give").toString().split("+"));
    foreach (int id, subcards) {
        give_list.append(id);
    }
    room->setPlayerProperty(target, "rende_give", IntList2StringList(give_list).join("+"));
}

class NOLQingjianAllot : public ViewAsSkill
{
public:
    NOLQingjianAllot() : ViewAsSkill("nolqingjian_allot")
    {
        expand_pile = "nolqingjian";
        response_pattern = "@@nolqingjian_allot!";
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        QList<int> nolqingjian_list = StringList2IntList(Self->property("nolqingjian_give").toString().split("+"));
        return nolqingjian_list.contains(to_select->getEffectiveId());
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty()) return NULL;
        NOLQingjianAllotCard *nolqingjian_card = new NOLQingjianAllotCard;
        nolqingjian_card->addSubcards(cards);
        return nolqingjian_card;
    }
};

class NOLQingjianViewAsSkill : public ViewAsSkill
{
public:
    NOLQingjianViewAsSkill() : ViewAsSkill("nolqingjian")
    {
        response_pattern = "@@nolqingjian";
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        QList<int> nolqingjian_list = StringList2IntList(Self->property("nolqingjian").toString().split("+"));
        return nolqingjian_list.contains(to_select->getEffectiveId());
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

class NOLQingjian : public TriggerSkill
{
public:
    NOLQingjian() : TriggerSkill("nolqingjian")
    {
        events << CardsMoveOneTime << EventPhaseChanging;
        view_as_skill = new NOLQingjianViewAsSkill;
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
                if (xiahou->getPile("nolqingjian").length() > 0 && TriggerSkill::triggerable(xiahou)) {
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
            room->setPlayerProperty(player, "nolqingjian", IntList2StringList(ids).join("+"));
            const Card *card = room->askForCard(player, "@@nolqingjian", "@nolqingjian-put", data, Card::MethodNone);
            room->setPlayerProperty(player, "nolqingjian", QString());
            if (card) {
                LogMessage log;
                log.type = "#InvokeSkill";
                log.arg = objectName();
                log.from = player;
                room->sendLog(log);
                room->notifySkillInvoked(player, objectName());
                player->broadcastSkillInvoke(objectName());
                player->addToPile("nolqingjian", card, false);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive) return false;
            if (xiahou->getPile("nolqingjian").length() > 0 && TriggerSkill::triggerable(xiahou)) {
                room->sendCompulsoryTriggerLog(xiahou, objectName());
                xiahou->broadcastSkillInvoke(objectName());
                QList<int> ids = xiahou->getPile("nolqingjian");
                room->setPlayerProperty(xiahou, "nolqingjian_give", IntList2StringList(ids).join("+"));
                do {
                    const Card *use = room->askForUseCard(xiahou, "@@nolqingjian_allot!", "@nolqingjian-give", QVariant(), Card::MethodNone);
                    ids = StringList2IntList(xiahou->property("nolqingjian_give").toString().split("+"));
                    if (use == NULL) {
                        NOLQingjianAllotCard *nolqingjian_card = new NOLQingjianAllotCard;
                        nolqingjian_card->addSubcards(ids);
                        QList<ServerPlayer *> targets;
                        targets << room->getOtherPlayers(xiahou).first();
                        nolqingjian_card->use(room, xiahou, targets);
                        delete nolqingjian_card;
                        break;
                    }
                } while (!ids.isEmpty() && xiahou->isAlive());
                room->setPlayerProperty(xiahou, "nolqingjian_give", QString());
                QList<CardsMoveStruct> moves;
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    QList<int> give_list = StringList2IntList(p->property("rende_give").toString().split("+"));
                    if (give_list.isEmpty())
                        continue;
                    room->setPlayerProperty(p, "rende_give", QString());
                    CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, p->objectName(), "nolqingjian", QString());
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

class NOLXunxun : public PhaseChangeSkill
{
public:
    NOLXunxun() : PhaseChangeSkill("nolxunxun")
    {
        frequency = Frequent;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getPhase() == Player::Draw;
    }

    bool onPhaseChange(ServerPlayer *lidian) const
    {
        if (lidian->getPhase() == Player::Draw) {
            Room *room = lidian->getRoom();
            if (room->askForSkillInvoke(lidian, objectName())) {
                room->broadcastSkillInvoke(objectName());
                QList<ServerPlayer *> p_list;
                p_list << lidian;
                QList<int> card_ids = room->getNCards(4);

                LogMessage log;
                log.type = "$ViewDrawPile";
                log.from = lidian;
                log.card_str = IntList2StringList(card_ids).join("+");
                room->sendLog(log, lidian);
                AskForMoveCardsStruct result = room->askForMoveCards(lidian, card_ids, QList<int>(), true, objectName(), "", 2, 2, false, false, QList<int>() << -1);
                QList<int> top_cards = result.bottom, bottom_cards = result.top;

                DummyCard *dummy = new DummyCard(top_cards);
                lidian->obtainCard(dummy, false);
                dummy->deleteLater();

                LogMessage logbuttom;
                logbuttom.type = "$GuanxingBottom";
                logbuttom.from = lidian;
                logbuttom.card_str = IntList2StringList(bottom_cards).join("+");
                room->doNotify(lidian, QSanProtocol::S_COMMAND_LOG_SKILL, logbuttom.toVariant());

                QListIterator<int> i = bottom_cards;
                while (i.hasNext())
                    room->getDrawPile().append(i.next());

                room->doBroadcastNotify(QSanProtocol::S_COMMAND_UPDATE_PILE, QVariant(room->getDrawPile().length()));

                return true;
            }
        }

        return false;
    }
};

NostalOLPackage::NostalOLPackage()
    : Package("nostalgia_ol")
{
    General *xiahoudun = new General(this, "nol_xiahoudun", "wei", 4, true, true);
    xiahoudun->addSkill("ganglie");
    xiahoudun->addSkill(new NOLQingjian);
    xiahoudun->addSkill(new DetachEffectSkill("nolqingjian", "nolqingjian"));
    related_skills.insertMulti("nolqingjian", "#nolqingjian-clear");

    General *lidian = new General(this, "nol_lidian", "wei", 3, true, true);
    lidian->addSkill("wangxi");
    lidian->addSkill(new NOLXunxun);


    addMetaObject<NOLQingjianAllotCard>();
    skills << new NOLQingjianAllot;
}

ADD_PACKAGE(NostalOL)
