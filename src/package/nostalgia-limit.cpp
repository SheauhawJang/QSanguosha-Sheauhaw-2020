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
#include "nostalgia-limit.h"

class NJieJianxiong : public MasochismSkill
{
public:
    NJieJianxiong() : MasochismSkill("njiejianxiong")
    {
    }

    void onDamaged(ServerPlayer *caocao, const DamageStruct &damage) const
    {
        Room *room = caocao->getRoom();
        QVariant data = QVariant::fromValue(damage);
        QStringList choices;
        choices << "draw";

        const Card *card = damage.card;
        if (card) {
            QList<int> ids;
            if (card->isVirtualCard())
                ids = card->getSubcards();
            else
                ids << card->getEffectiveId();
            if (ids.length() > 0) {
                bool all_place_table = true;
                foreach (int id, ids) {
                    if (room->getCardPlace(id) != Player::PlaceTable) {
                        all_place_table = false;
                        break;
                    }
                }
                if (all_place_table) choices.append("obtain");
            }
        }
        choices << "cancel";
        QString choice = room->askForChoice(caocao, objectName(), choices.join("+"), data);
        if (choice != "cancel") {
            LogMessage log;
            log.type = "#InvokeSkill";
            log.from = caocao;
            log.arg = objectName();
            room->sendLog(log);

            room->notifySkillInvoked(caocao, objectName());
            room->broadcastSkillInvoke(objectName());
            if (choice == "obtain")
                caocao->obtainCard(card);
            else
                caocao->drawCards(1, objectName());
        }
    }
};

class NJieQingjian : public TriggerSkill
{
public:
    NJieQingjian() : TriggerSkill("njieqingjian")
    {
        events << CardsMoveOneTime;
    }

    QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        foreach (QVariant qvar, data.toList()) {
            CardsMoveOneTimeStruct move = qvar.value<CardsMoveOneTimeStruct>();
            if (!room->getTag("FirstRound").toBool() && player->getPhase() != Player::Draw && move.to == player && move.to_place == Player::PlaceHand) {
                foreach (int id, move.card_ids) {
                    if (room->getCardOwner(id) == player && room->getCardPlace(id) == Player::PlaceHand)
                        return nameList();
                }
            }
        }
        return QStringList();
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
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
        while (room->askForYiji(player, ids, objectName(), false, false, true, -1, QList<ServerPlayer *>(), CardMoveReason(), "@njieqingjian-distribute", QString(), true)) {
            if (player->isDead()) return false;
        }
        return false;
    }
};

NJieTuxiCard::NJieTuxiCard()
{
}

bool NJieTuxiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.length() < Self->getMark("njietuxi") && to_select->getHandcardNum() >= Self->getHandcardNum() && to_select != Self && !to_select->isKongcheng();
}

void NJieTuxiCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    room->addPlayerMark(effect.from, "njietuxi_target_num");
    if (effect.from->isAlive() && !effect.to->isKongcheng()) {
        int card_id = room->askForCardChosen(effect.from, effect.to, "h", "njietuxi");
        CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, effect.from->objectName());
        room->obtainCard(effect.from, Sanguosha->getCard(card_id), reason, false);
    }
}

class NJieTuxiViewAsSkill : public ZeroCardViewAsSkill
{
public:
    NJieTuxiViewAsSkill() : ZeroCardViewAsSkill("njietuxi")
    {
        response_pattern = "@@njietuxi";
    }

    virtual const Card *viewAs() const
    {
        return new NJieTuxiCard;
    }
};

class NJieTuxi : public DrawCardsSkill
{
public:
    NJieTuxi() : DrawCardsSkill("njietuxi")
    {
        view_as_skill = new NJieTuxiViewAsSkill;
    }

    virtual int getDrawNum(ServerPlayer *zhangliao, int n) const
    {
        Room *room = zhangliao->getRoom();
        QList<ServerPlayer *> targets;
        foreach(ServerPlayer *p, room->getOtherPlayers(zhangliao))
            if (p->getHandcardNum() >= zhangliao->getHandcardNum())
                targets << p;
        int num = qMin(targets.length(), n);

        if (num > 0) {
            room->setPlayerMark(zhangliao, "njietuxi", num);
            room->setPlayerMark(zhangliao, "njietuxi_target_num", 0);
            int count = 0;
            if (room->askForUseCard(zhangliao, "@@njietuxi", "@njietuxi-card:::" + QString::number(num))) {
                count = zhangliao->getMark("njietuxi_target_num");
                room->setPlayerMark(zhangliao, "njietuxi_target_num", 0);
            }
            room->setPlayerMark(zhangliao, "njietuxi", 0);
            return n - count;
        } else
            return n;
    }
};

NJieYijiCard::NJieYijiCard()
{
    mute = true;
}

bool NJieYijiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (to_select == Self) return false;
    if (Self->getHandcardNum() == 1)
        return targets.isEmpty();
    else
        return targets.length() < 2;
}

void NJieYijiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    foreach (ServerPlayer *target, targets) {
        if (!source->isAlive() || source->isKongcheng()) break;
        if (!target->isAlive()) continue;
        int max = qMin(2, source->getHandcardNum());
        if (source->getHandcardNum() == 2 && targets.length() == 2 && targets.last()->isAlive() && target == targets.first())
            max = 1;
        const Card *dummy = room->askForExchange(source, "njieyiji", max, 1, false, "NJieYijiGive::" + target->objectName());
        target->addToPile("njieyiji", dummy, false);
        delete dummy;
    }
}

class NJieYijiViewAsSkill : public ZeroCardViewAsSkill
{
public:
    NJieYijiViewAsSkill() : ZeroCardViewAsSkill("njieyiji")
    {
        response_pattern = "@@njieyiji";
    }

    virtual const Card *viewAs() const
    {
        return new NJieYijiCard;
    }
};

class NJieYiji : public TriggerSkill
{
public:
    NJieYiji() : TriggerSkill("njieyiji")
    {
        view_as_skill = new NJieYijiViewAsSkill;
        frequency = Frequent;
        events << Damaged << EventPhaseStart;
    }

    QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseStart)
            if (target->getPhase() == Player::Draw && !target->getPile("njieyiji").isEmpty())
                return comList();
        if (triggerEvent == Damaged)
            if (TriggerSkill::triggerable(target))
                return nameList(data.value<DamageStruct>().damage);
        return QStringList();
    }

    bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == EventPhaseStart) {
            DummyCard *dummy = new DummyCard(target->getPile("njieyiji"));
            CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, target->objectName(), "njieyiji", QString());
            room->obtainCard(target, dummy, reason, false);
            dummy->deleteLater();
        } else {
            if (room->askForSkillInvoke(target, objectName(), QVariant::fromValue(data.value<DamageStruct>().damage))) {
                target->broadcastSkillInvoke(objectName());
                target->drawCards(2, objectName());
                room->askForUseCard(target, "@@njieyiji", "@njieyiji");
            }
        }
    }
};

class NJieLuoyi : public TriggerSkill
{
public:
    NJieLuoyi() : TriggerSkill("njieluoyi")
    {
        events << EventPhaseStart << DamageCaused;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart && player->getMark("#njieluoyi") > 0)
            room->removePlayerTip(player, "#njieluoyi");
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseStart && TriggerSkill::triggerable(player) && player->getPhase() == Player::Draw)
            return QStringList("njieluoyi");
        else if (triggerEvent == DamageCaused && player && player->isAlive() && player->getMark("#njieluoyi") > 0) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.chain || damage.transfer) return QStringList();
            const Card *reason = damage.card;
            if (reason && (reason->isKindOf("Slash") || reason->isKindOf("Duel")))
                return QStringList("njieluoyi!");
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (room->askForSkillInvoke(player, "njieluoyi")) {

                room->addPlayerTip(player, "#njieluoyi");

                QList<int> ids = room->getNCards(3, false);
                CardsMoveStruct move(ids, player, Player::PlaceTable,
                                     CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), objectName(), QString()));
                room->moveCardsAtomic(move, true);

                QList<int> card_to_gotback;
                for (int i = 0; i < 3; i++) {
                    const Card *card = Sanguosha->getCard(ids[i]);
                    if (card->getTypeId() == Card::TypeBasic || card->isKindOf("Weapon") || card->isKindOf("Duel"))
                        card_to_gotback << ids[i];

                }

                if (!card_to_gotback.isEmpty()) {
                    DummyCard *dummy = new DummyCard(card_to_gotback);
                    room->obtainCard(player, dummy);
                    dummy->deleteLater();
                }

                QList<int> card_to_throw = room->getCardIdsOnTable(ids);
                if (!card_to_throw.isEmpty()) {
                    DummyCard *dummy = new DummyCard(card_to_throw);
                    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), "njieluoyi", QString());
                    room->throwCard(dummy, reason, NULL);
                    dummy->deleteLater();
                }

                return true;
            }
        } else {
            LogMessage log;
            log.type = "#SkillForce";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);
            DamageStruct damage = data.value<DamageStruct>();
            damage.damage++;
            data = QVariant::fromValue(damage);
        }
        return false;
    }
};

NostalLimitationBrokenPackage::NostalLimitationBrokenPackage()
    : Package("nostal_limitation_broken")
{
    General *caocao = new General(this, "njie_caocao$", "wei", 4, true, true);
    caocao->addSkill(new NJieJianxiong);
    caocao->addSkill("hujia");

    General *xiahoudun = new General(this, "njie_xiahoudun", "wei", 4, true, true);
    xiahoudun->addSkill("ganglie");
    xiahoudun->addSkill(new NJieQingjian);

    General *zhangliao = new General(this, "njie_zhangliao", "wei", 4, true, true);
    zhangliao->addSkill(new NJieTuxi);

    General *guojia = new General(this, "njie_guojia", "wei", 3, true, true);
    guojia->addSkill("tiandu");
    guojia->addSkill(new NJieYiji);

    General *xuchu = new General(this, "njie_xuchu", "wei", 4, true, true);
    xuchu->addSkill(new NJieLuoyi);

    addMetaObject<NJieTuxiCard>();
    addMetaObject<NJieYijiCard>();
}

ADD_PACKAGE(NostalLimitationBroken)
