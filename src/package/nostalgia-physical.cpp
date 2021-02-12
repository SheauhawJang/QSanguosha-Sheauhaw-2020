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

NPhyRendeCard::NPhyRendeCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    mute = true;
}

void NPhyRendeCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{

    if (!source->tag.value("nphyrende_using", false).toBool())
        room->broadcastSkillInvoke("nphyrende");

    ServerPlayer *target = targets.first();

    int old_value = source->getMark("nphyrende");
    QList<int> nphyrende_list;
    if (old_value > 0)
        nphyrende_list = StringList2IntList(source->property("nphyrende").toString().split("+"));
    else
        nphyrende_list = source->handCards();
    foreach(int id, this->subcards)
        nphyrende_list.removeOne(id);
    room->setPlayerProperty(source, "nphyrende", IntList2StringList(nphyrende_list).join("+"));

    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), target->objectName(), "nphyrende", QString());
    room->obtainCard(target, this, reason, false);

    int new_value = old_value + subcards.length();
    room->setPlayerMark(source, "nphyrende", new_value);

    if (old_value < 2 && new_value >= 2)
        room->recover(source, RecoverStruct(source));

    if (room->getMode() == "04_1v3" && source->getMark("nphyrende") >= 2) return;
    if (source->isKongcheng() || source->isDead() || nphyrende_list.isEmpty()) return;
    room->addPlayerHistory(source, "NPhyRendeCard", -1);

    source->tag["nphyrende_using"] = true;

    if (!room->askForUseCard(source, "@@nphyrende", "@nphyrende-give", -1, Card::MethodNone))
        room->addPlayerHistory(source, "NPhyRendeCard");

    source->tag["nphyrende_using"] = false;
}

class NPhyRendeViewAsSkill : public ViewAsSkill
{
public:
    NPhyRendeViewAsSkill() : ViewAsSkill("nphyrende")
    {
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (ServerInfo.GameMode == "04_1v3" && selected.length() + Self->getMark("nphyrende") >= 2)
            return false;
        else {
            if (to_select->isEquipped()) return false;
            if (Sanguosha->currentRoomState()->getCurrentCardUsePattern() == "@@nphyrende") {
                QList<int> nphyrende_list = StringList2IntList(Self->property("nphyrende").toString().split("+"));
                return nphyrende_list.contains(to_select->getEffectiveId());
            } else
                return true;
        }
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        if (ServerInfo.GameMode == "04_1v3" && player->getMark("nphyrende") >= 2)
            return false;
        return !player->hasUsed("NPhyRendeCard") && !player->isKongcheng();
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "@@nphyrende";
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return NULL;

        NPhyRendeCard *nphyrende_card = new NPhyRendeCard;
        nphyrende_card->addSubcards(cards);
        return nphyrende_card;
    }
};

class NPhyRende : public TriggerSkill
{
public:
    NPhyRende() : TriggerSkill("nphyrende")
    {
        events << EventPhaseChanging;
        view_as_skill = new NPhyRendeViewAsSkill;
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
        room->setPlayerMark(player, "nphyrende", 0);
        room->setPlayerProperty(player, "nphyrende", QString());
    }
};

class NPhyJizhi : public TriggerSkill
{
public:
    NPhyJizhi() : TriggerSkill("nphyjizhi")
    {
        frequency = Frequent;
        events << CardUsed;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card && use.card->getTypeId() == Card::TypeTrick)
            return nameList();
        return QStringList();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *yueying, QVariant &data) const
    {
        if (room->askForSkillInvoke(yueying, objectName())) {
            yueying->broadcastSkillInvoke(objectName());

            QList<int> ids = room->getNCards(1, false);
            CardsMoveStruct move(ids, yueying, Player::PlaceTable,
                CardMoveReason(CardMoveReason::S_REASON_TURNOVER, yueying->objectName(), "nphyjizhi", QString()));
            room->moveCardsAtomic(move, true);

            int id = ids.first();
            const Card *card = Sanguosha->getCard(id);
            if (!card->isKindOf("BasicCard")) {
                CardMoveReason reason(CardMoveReason::S_REASON_DRAW, yueying->objectName(), "nphyjizhi", QString());
                room->obtainCard(yueying, card, reason);
            } else {
                const Card *card_ex = NULL;
                if (!yueying->isKongcheng())
                    card_ex = room->askForCard(yueying, ".", "@nphyjizhi-exchange:::" + card->objectName(),
                    QVariant::fromValue(card), Card::MethodNone);
                if (card_ex) {
                    CardMoveReason reason1(CardMoveReason::S_REASON_PUT, yueying->objectName(), "nphyjizhi", QString());
                    CardMoveReason reason2(CardMoveReason::S_REASON_DRAW, yueying->objectName(), "nphyjizhi", QString());
                    CardsMoveStruct move1(card_ex->getEffectiveId(), yueying, NULL, Player::PlaceUnknown, Player::DrawPile, reason1);
                    CardsMoveStruct move2(ids, yueying, yueying, Player::PlaceUnknown, Player::PlaceHand, reason2);

                    QList<CardsMoveStruct> moves;
                    moves.append(move1);
                    moves.append(move2);
                    room->moveCardsAtomic(moves, false);
                } else {
                    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, yueying->objectName(), "nphyjizhi", QString());
                    room->throwCard(card, reason, NULL);
                }
            }
        }

        return false;
    }
};

class NPhyQicai : public TargetModSkill
{
public:
    NPhyQicai() : TargetModSkill("nphyqicai")
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

class NPhyBiyue : public PhaseChangeSkill
{
public:
    NPhyBiyue() : PhaseChangeSkill("nphybiyue")
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

NostalPhysicalPackage::NostalPhysicalPackage()
    : Package("nostal_physical")
{
    General *liubei = new General(this, "nphy_liubei$", "shu", 4, true, true);
    liubei->addSkill(new NPhyRende);
    liubei->addSkill("jijiang");

    General *huangyueying = new General(this, "nphy_huangyueying", "shu", 3, false, true);
    huangyueying->addSkill(new NPhyJizhi);
    huangyueying->addSkill(new NPhyQicai);

    General *diaochan = new General(this, "nphy_diaochan", "qun", 3, false, true);
    diaochan->addSkill("lijian");
    diaochan->addSkill(new NPhyBiyue);

    addMetaObject<NPhyRendeCard>();
}

ADD_PACKAGE(NostalPhysical)
