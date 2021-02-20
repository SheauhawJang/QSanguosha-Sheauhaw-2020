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

class NOLPaoxiaoTargetMod : public TargetModSkill
{
public:
    NOLPaoxiaoTargetMod() : TargetModSkill("#nolpaoxiao-target")
    {

    }

    virtual int getResidueNum(const Player *from, const Card *, const Player *) const
    {
        if (from->hasSkill(this))
            return 1000;
        else
            return 0;
    }
};

class NOLPaoxiao : public TriggerSkill
{
public:
    NOLPaoxiao() : TriggerSkill("nolpaoxiao")
    {
        frequency = Compulsory;
        events << DamageCaused << SlashMissed << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == DamageCaused && player && player->isAlive() && player->getMark("#nolpaoxiao") > 0) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.chain || damage.transfer) return QStringList();
            const Card *reason = damage.card;
            if (reason && reason->isKindOf("Slash"))
                return comList();
        }
        return QStringList();
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                room->setPlayerMark(player, "#nolpaoxiao", 0);
        } else if (triggerEvent == SlashMissed && TriggerSkill::triggerable(player)) {
            room->addPlayerMark(player, "#nolpaoxiao");
        }
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        LogMessage log;
        log.type = "#SkillForce";
        log.from = player;
        log.arg = objectName();
        room->sendLog(log);
        DamageStruct damage = data.value<DamageStruct>();
        damage.damage += player->getMark("#nolpaoxiao");
        data = QVariant::fromValue(damage);
        room->setPlayerMark(player, "#nolpaoxiao", 0);
    }
};

class NOLTishen : public TriggerSkill
{
public:
    NOLTishen() : TriggerSkill("noltishen")
    {
        events << EventPhaseStart;
        frequency = Limited;
        limit_mark = "@nolsubstitute";
    }

    virtual bool triggerable(const ServerPlayer *player) const
    {
        return TriggerSkill::triggerable(player) && player->getMark("@nolsubstitute") > 0 && player->getPhase() == Player::Start && player->isWounded();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (triggerEvent == EventPhaseStart) {
            int delta = player->getMaxHp() - player->getHp();
            if (room->askForSkillInvoke(player, objectName(), QVariant::fromValue(delta))) {
                room->removePlayerMark(player, "@nolsubstitute");
                player->broadcastSkillInvoke(objectName());
                //room->doLightbox("$NOLTishenAnimate");

                room->recover(player, RecoverStruct(player, NULL, delta));
                player->drawCards(delta, objectName());
            }
        }
        return false;
    }
};

class NOLLongdan : public OneCardViewAsSkill
{
public:
    NOLLongdan() : OneCardViewAsSkill("nollongdan")
    {
        response_or_use = true;
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        const Card *card = to_select;

        switch (Sanguosha->currentRoomState()->getCurrentCardUseReason()) {
        case CardUseStruct::CARD_USE_REASON_PLAY: {
            if (card->isKindOf("Jink")) {
                Slash *slash = new Slash(card->getSuit(), card->getNumber());
                slash->addSubcard(card);
                slash->setSkillName(objectName());
                slash->deleteLater();
                return slash->isAvailable(Self);
            }
            if (card->isKindOf("Peach")) {
                Analeptic *analeptic = new Analeptic(card->getSuit(), card->getNumber());
                analeptic->addSubcard(card);
                analeptic->setSkillName(objectName());
                analeptic->deleteLater();
                return analeptic->isAvailable(Self);
            }
            if (card->isKindOf("Analeptic")) {
                Peach *peach = new Peach(card->getSuit(), card->getNumber());
                peach->addSubcard(card);
                peach->setSkillName(objectName());
                peach->deleteLater();
                return peach->isAvailable(Self);
            }
            return false;
        }
        case CardUseStruct::CARD_USE_REASON_RESPONSE:
        case CardUseStruct::CARD_USE_REASON_RESPONSE_USE: {
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (pattern == "slash")
                return card->isKindOf("Jink");
            else if (pattern == "jink")
                return card->isKindOf("Slash");
            else if (pattern.contains("peach") && card->isKindOf("Analeptic")) return true;
            else if (pattern.contains("analeptic") && card->isKindOf("Peach")) return true;
            return false;
        }
        default:
            return false;
        }
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return hasAvailable(player) || Slash::IsAvailable(player) || Analeptic::IsAvailable(player) || player->isWounded();
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "slash" || pattern == "jink" || pattern == "peach" || pattern.contains("analeptic");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard->isKindOf("Slash")) {
            Jink *jink = new Jink(originalCard->getSuit(), originalCard->getNumber());
            jink->addSubcard(originalCard);
            jink->setSkillName(objectName());
            return jink;
        } else if (originalCard->isKindOf("Jink")) {
            Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
            slash->addSubcard(originalCard);
            slash->setSkillName(objectName());
            return slash;
        } else if (originalCard->isKindOf("Peach")) {
            Analeptic *analeptic = new Analeptic(originalCard->getSuit(), originalCard->getNumber());
            analeptic->addSubcard(originalCard);
            analeptic->setSkillName(objectName());
            return analeptic;
        } else if (originalCard->isKindOf("Analeptic")) {
            Peach *peach = new Peach(originalCard->getSuit(), originalCard->getNumber());
            peach->addSubcard(originalCard);
            peach->setSkillName(objectName());
            return peach;
        } else
            return NULL;
    }
};

class NOLYajiao : public TriggerSkill
{
public:
    NOLYajiao() : TriggerSkill("nolyajiao")
    {
        events << CardUsed << CardResponded;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player) || player->getPhase() != Player::NotActive) return QStringList();
        const Card *cardstar = NULL;
        bool isHandcard = false;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            cardstar = use.card;
            isHandcard = use.m_isHandcard;
        } else {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            cardstar = resp.m_card;
            isHandcard = resp.m_isHandcard;
        }
        if (cardstar && cardstar->getTypeId() != Card::TypeSkill && isHandcard)
            return nameList();

        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        const Card *cardstar = NULL;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            cardstar = use.card;
        } else {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            cardstar = resp.m_card;
        }
        if (room->askForSkillInvoke(player, objectName(), data)) {
            player->broadcastSkillInvoke(objectName());
            QList<int> ids = room->getNCards(1, false);
            CardsMoveStruct move(ids, NULL, Player::PlaceTable,
                                 CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), "nolyajiao", QString()));
            room->moveCardsAtomic(move, true);

            int id = ids.first();
            const Card *card = Sanguosha->getCard(id);
            bool dealt = false;
            player->setMark("nolyajiao", id); // For AI
            if (card->getTypeId() == cardstar->getTypeId()) {
                ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(),
                                       QString("@nolyajiao-give:::%1:%2\\%3").arg(card->objectName())
                                       .arg(card->getSuitString() + "_char")
                                       .arg(card->getNumberString()),
                                       true);
                if (target) {
                    dealt = true;
                    CardMoveReason reason(CardMoveReason::S_REASON_DRAW, target->objectName(), "nolyajiao", QString());
                    room->obtainCard(target, card, reason);
                }
            } else {
                QList<ServerPlayer *> targets;
                foreach (ServerPlayer *p, room->getOtherPlayers(player))
                    if (p->inMyAttackRange(player) && !player->canDiscard(p, "hej"))
                        targets << p;
                ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());
                if (target) {
                    int will_throw = room->askForCardChosen(player, target, "hej", objectName());
                    room->throwCard(room->getCard(will_throw), target, player);
                }
            }
            if (!dealt)
                room->returnToTopDrawPile(ids);
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

    General *zhangfei = new General(this, "nol_zhangfei", "shu", 4, true, true);
    zhangfei->addSkill(new NOLPaoxiao);
    zhangfei->addSkill(new NOLPaoxiaoTargetMod);
    zhangfei->addSkill(new NOLTishen);
    related_skills.insertMulti("nolpaoxiao", "#nolpaoxiao-target");

    General *zhaoyun = new General(this, "nol_zhaoyun", "shu", 4, true, true);
    zhaoyun->addSkill(new NOLLongdan);
    zhaoyun->addSkill(new NOLYajiao);


    General *lidian = new General(this, "nol_lidian", "wei", 3, true, true);
    lidian->addSkill("wangxi");
    lidian->addSkill(new NOLXunxun);

    addMetaObject<NOLQingjianAllotCard>();
    skills << new NOLQingjianAllot;
}

ADD_PACKAGE(NostalOL)
