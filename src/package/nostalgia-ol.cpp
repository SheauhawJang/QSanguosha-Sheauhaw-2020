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
        return false;
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

class NOLLeiji : public TriggerSkill
{
public:
    NOLLeiji() : TriggerSkill("nolleiji")
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
            zhangjiao->broadcastSkillInvoke("nolleiji");

            JudgeStruct judge;
            judge.patterns << ".|spade" << ".|club";
            judge.good = false;
            judge.negative = true;
            judge.reason = objectName();
            judge.who = target;

            room->judge(judge);

            int damage = 0;
            if (judge.pattern == ".|spade") {
                damage = 2;
            } else if (judge.pattern == ".|club") {
                damage = 1;
                room->recover(zhangjiao, RecoverStruct(zhangjiao));
            }
            if (damage)
                room->damage(DamageStruct(objectName(), zhangjiao, target, damage, DamageStruct::Thunder));
        }
        return false;
    }
};

class NOLGuidao : public TriggerSkill
{
public:
    NOLGuidao() : TriggerSkill("nolguidao")
    {
        events << AskForRetrial;
        view_as_skill = new dummyVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && !(target->isNude() && target->getHandPile().isEmpty());
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();

        QStringList prompt_list;
        prompt_list << "@nolguidao-card" << judge->who->objectName()
                    << objectName() << judge->reason << QString::number(judge->card->getEffectiveId());
        QString prompt = prompt_list.join(":");
        const Card *card = room->askForCard(player, ".|black", prompt, data, Card::MethodResponse, judge->who, true, "nolguidao");

        if (card != NULL) {
            room->retrial(card, player, judge, objectName(), true);
        }
        return false;
    }
};

NOLQimouCard::NOLQimouCard()
{
    target_fixed = true;
}

void NOLQimouCard::extraCost(Room *room, const CardUseStruct &card_use) const
{
    room->removePlayerMark(card_use.from, "@scheme");
    room->loseHp(card_use.from, user_string.toInt());
}

void NOLQimouCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->setPlayerMark(source, "#nolqimou", user_string.toInt());
}

class NOLQimouViewAsSkill : public ZeroCardViewAsSkill
{
public:
    NOLQimouViewAsSkill() : ZeroCardViewAsSkill("nolqimou")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@scheme") > 0 && player->getHp() > 0;
    }

    virtual const Card *viewAs() const
    {
        QString user_string = Self->tag["nolqimou"].toString();
        if (user_string.isEmpty()) return NULL;
        NOLQimouCard *skill_card = new NOLQimouCard;
        skill_card->setUserString(user_string);
        skill_card->setSkillName("nolqimou");
        return skill_card;
    }
};

class NOLQimou : public TriggerSkill
{
public:
    NOLQimou() : TriggerSkill("nolqimou")
    {
        events << EventPhaseStart;
        frequency = Limited;
        limit_mark = "@scheme";
        view_as_skill = new NOLQimouViewAsSkill;
    }

    virtual void record(TriggerEvent, Room *room, ServerPlayer *weiyan, QVariant &) const
    {
        if (weiyan->getPhase() == Player::NotActive)
            room->setPlayerMark(weiyan, "#nolqimou", 0);
    }

    virtual bool triggerable(const ServerPlayer *) const
    {
        return false;
    }

    QString getSelectBox() const
    {
        QStringList hp_num;
        for (int i = 1; i <= Self->getHp(); hp_num << QString::number(i++)) {}
        return hp_num.join("+");
    }

};

class NOLQimouDistance : public DistanceSkill
{
public:
    NOLQimouDistance() : DistanceSkill("#nolqimou-distance")
    {
    }

    virtual int getCorrect(const Player *from, const Player *) const
    {
        return -from->getMark("#nolqimou");
    }
};

class NOLQimouTargetMod : public TargetModSkill
{
public:
    NOLQimouTargetMod() : TargetModSkill("#nolqimou-target")
    {
    }

    virtual int getResidueNum(const Player *from, const Card *, const Player *) const
    {
        return from->getMark("#nolqimou");
    }
};

class NOLHongyan : public FilterSkill
{
public:
    NOLHongyan() : FilterSkill("nolhongyan")
    {
    }

    static WrappedCard *changeToHeart(int cardId)
    {
        WrappedCard *new_card = Sanguosha->getWrappedCard(cardId);
        new_card->setSkillName("nolhongyan");
        new_card->setSuit(Card::Heart);
        new_card->setModified(true);
        return new_card;
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        return to_select->getSuit() == Card::Spade;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        return changeToHeart(originalCard->getEffectiveId());
    }
};

NOLGuhuoCard::NOLGuhuoCard()
{
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "nolguhuo";
}

bool NOLGuhuoCard::nolguhuo(ServerPlayer *yuji) const
{
    Room *room = yuji->getRoom();

    CardMoveReason reason1(CardMoveReason::S_REASON_SECRETLY_PUT, yuji->objectName(), QString(), "nolguhuo", QString());
    room->moveCardTo(this, NULL, Player::PlaceTable, reason1, false);

    QList<ServerPlayer *> players = room->getOtherPlayers(yuji);

    room->setTag("NOLGuhuoType", user_string);

    ServerPlayer *questioned = NULL;
    foreach (ServerPlayer *player, players) {
        if (player->hasSkill("nolchanyuan")) {
            LogMessage log;
            log.type = "#NOLChanyuan";
            log.from = player;
            log.arg = "nolchanyuan";
            room->sendLog(log);

            continue;
        }

        QString choice = room->askForChoice(player, "nolguhuo", "noquestion+question");

        LogMessage log;
        log.type = "#NOLGuhuoQuery";
        log.from = player;
        log.arg = choice;
        room->sendLog(log);
        if (choice == "question") {
            questioned = player;
            break;
        }
    }

    LogMessage log;
    log.type = "$NOLGuhuoResult";
    log.from = yuji;
    log.card_str = QString::number(subcards.first());
    room->sendLog(log);

    const Card *card = Sanguosha->getCard(subcards.first());

    CardMoveReason reason_ui(CardMoveReason::S_REASON_TURNOVER, yuji->objectName(), QString(), "nolguhuo", QString());
    CardResponseStruct resp(card);
    reason_ui.m_extraData = QVariant::fromValue(resp);
    room->showVirtualMove(reason_ui);

    bool success = true;
    if (questioned) {
        success = (card->objectName() == user_string);

        if (success) {
            room->acquireSkill(questioned, "nolchanyuan");
        } else {
            room->moveCardTo(this, yuji, NULL, Player::DiscardPile,
                             CardMoveReason(CardMoveReason::S_REASON_PUT, yuji->objectName(), QString(), "nolguhuo"), true);
        }
    }
    room->setPlayerFlag(yuji, "NOLGuhuoUsed");
    return success;
}

bool NOLGuhuoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return false;
    }

    Card *card = Sanguosha->cloneCard(user_string);
    if (card == NULL)
        return false;
    card->setCanRecast(false);
    card->deleteLater();
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool NOLGuhuoCard::targetFixed() const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetFixed();
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    Card *card = Sanguosha->cloneCard(user_string);
    if (card == NULL)
        return false;
    card->setCanRecast(false);
    card->deleteLater();
    return card && card->targetFixed();
}

bool NOLGuhuoCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetsFeasible(targets, Self);
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    Card *card = Sanguosha->cloneCard(user_string);
    if (card == NULL)
        return false;
    card->setCanRecast(false);
    card->deleteLater();
    return card && card->targetsFeasible(targets, Self);
}

const Card *NOLGuhuoCard::validate(CardUseStruct &card_use) const
{
    ServerPlayer *yuji = card_use.from;
    Room *room = yuji->getRoom();

    QString to_nolguhuo = user_string;
    yuji->broadcastSkillInvoke("nolguhuo");

    LogMessage log;
    log.type = card_use.to.isEmpty() ? "#NOLGuhuoNoTarget" : "#NOLGuhuo";
    log.from = yuji;
    log.to = card_use.to;
    log.arg = to_nolguhuo;
    log.arg2 = "nolguhuo";
    room->sendLog(log);

    const Card *nolguhuo_card = Sanguosha->cloneCard(user_string, Card::NoSuit, 0);
    if (card_use.to.isEmpty())
        card_use.to = nolguhuo_card->defaultTargets(room, yuji);

    foreach (ServerPlayer *to, card_use.to)
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, yuji->objectName(), to->objectName());

    CardMoveReason reason_ui(CardMoveReason::S_REASON_USE, yuji->objectName(), QString(), "nolguhuo", QString());
    if (card_use.to.size() == 1 && !card_use.card->targetFixed())
        reason_ui.m_targetId = card_use.to.first()->objectName();

    CardUseStruct nolguhuo_use = card_use;
    nolguhuo_use.card = nolguhuo_card;
    reason_ui.m_extraData = QVariant::fromValue(nolguhuo_use);
    room->showVirtualMove(reason_ui);

    if (nolguhuo(card_use.from)) {
        const Card *card = Sanguosha->getCard(subcards.first());
        Card *use_card = Sanguosha->cloneCard(to_nolguhuo, card->getSuit(), card->getNumber());
        use_card->setSkillName("_nolguhuo");
        use_card->addSubcard(subcards.first());
        use_card->deleteLater();
        return use_card;
    } else
        return NULL;
}

const Card *NOLGuhuoCard::validateInResponse(ServerPlayer *yuji) const
{
    Room *room = yuji->getRoom();
    yuji->broadcastSkillInvoke("nolguhuo");

    QString to_nolguhuo = user_string;

    LogMessage log;
    log.type = "#NOLGuhuoNoTarget";
    log.from = yuji;
    log.arg = to_nolguhuo;
    log.arg2 = "nolguhuo";
    room->sendLog(log);

    CardMoveReason reason_ui(CardMoveReason::S_REASON_RESPONSE, yuji->objectName(), QString(), "nolguhuo", QString());
    const Card *nolguhuo_card = Sanguosha->cloneCard(user_string, Card::NoSuit, 0);
    CardResponseStruct resp(nolguhuo_card);
    reason_ui.m_extraData = QVariant::fromValue(resp);
    room->showVirtualMove(reason_ui);

    if (nolguhuo(yuji)) {
        const Card *card = Sanguosha->getCard(subcards.first());
        Card *use_card = Sanguosha->cloneCard(to_nolguhuo, card->getSuit(), card->getNumber());
        use_card->setSkillName("_nolguhuo");
        use_card->addSubcard(subcards.first());
        use_card->deleteLater();
        return use_card;
    } else
        return NULL;
}

class NOLGuhuo : public OneCardViewAsSkill
{
public:
    NOLGuhuo() : OneCardViewAsSkill("nolguhuo")
    {
        filter_pattern = ".|.|.|hand";
        response_or_use = true;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        bool current = false;
        QList<const Player *> players = player->getAliveSiblings();
        players.append(player);
        foreach (const Player *p, players) {
            if (p->getPhase() != Player::NotActive) {
                current = true;
                break;
            }
        }
        if (!current) return false;

        if (player->hasFlag("NOLGuhuoUsed") || pattern.startsWith(".") || pattern.startsWith("@"))
            return false;
        for (int i = 0; i < pattern.length(); i++) {
            QChar ch = pattern[i];
            if (ch.isUpper() || ch.isDigit()) return false; // This is an extremely dirty hack!! For we need to prevent patterns like 'BasicCard'
        }
        return true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        bool current = false;
        QList<const Player *> players = player->getAliveSiblings();
        players.append(player);
        foreach (const Player *p, players) {
            if (p->getPhase() != Player::NotActive) {
                current = true;
                break;
            }
        }
        if (!current) return false;
        return !player->hasFlag("NOLGuhuoUsed");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        NOLGuhuoCard *card = new NOLGuhuoCard;
        card->addSubcard(originalCard);
        return card;
    }

    QString getSelectBox() const
    {
        return "guhuo_bt";
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        if (card->isKindOf("NOLGuhuoCard"))
            return -1;
        else
            return 0;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        ServerPlayer *current = player->getRoom()->getCurrent();
        if (!current || current->isDead() || current->getPhase() == Player::NotActive) return false;
        return !player->isKongcheng() && !player->hasFlag("NOLGuhuoUsed");
    }
};

class NOLChanyuan : public TriggerSkill
{
public:
    NOLChanyuan() : TriggerSkill("nolchanyuan")
    {
        events << GameStart << HpChanged << MaxHpChanged << EventAcquireSkill << EventLoseSkill;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual int getPriority(TriggerEvent) const
    {
        return 5;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventLoseSkill) {
            if (data.toString() != objectName()) return false;
            room->removePlayerTip(player, "#nolchanyuan");
        } else if (triggerEvent == EventAcquireSkill) {
            if (data.toString() != objectName()) return false;
            room->addPlayerTip(player, "#nolchanyuan");
        }
        if (triggerEvent != EventLoseSkill && !player->hasSkill(this)) return false;

        foreach(ServerPlayer *p, room->getOtherPlayers(player))
            room->filterCards(p, p->getCards("he"), true);
        JsonArray args;
        args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
        return false;
    }
};

class NOLChanyuanInvalidity : public InvaliditySkill
{
public:
    NOLChanyuanInvalidity() : InvaliditySkill("#nolchanyuan-inv")
    {
    }

    virtual bool isSkillValid(const Player *player, const Skill *skill) const
    {
        return skill->objectName().contains("chanyuan") || !player->hasSkill("nolchanyuan")
               || player->getHp() != 1 || skill->isAttachedLordSkill();
    }
};

class NOLJianchu : public TriggerSkill
{
public:
    NOLJianchu() : TriggerSkill("noljianchu")
    {
        events << TargetSpecified;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card != NULL && use.card->isKindOf("Slash")) {
                ServerPlayer *to = use.to.at(use.index);

                if (to && to->isAlive() && player->canDiscard(to, "he"))
                    return nameList();
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

class NOLTianxiangViewAsSkill : public OneCardViewAsSkill
{
public:
    NOLTianxiangViewAsSkill() : OneCardViewAsSkill("noltianxiang")
    {
        filter_pattern = ".|heart|.|hand!";
        response_pattern = "@@noltianxiang";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        TianxiangCard *noltianxiangCard = new TianxiangCard;
        noltianxiangCard->addSubcard(originalCard);
        return noltianxiangCard;
    }
};

class NOLTianxiang : public TriggerSkill
{
public:
    NOLTianxiang() : TriggerSkill("noltianxiang")
    {
        events << DamageInflicted;
        view_as_skill = new NOLTianxiangViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && !target->isKongcheng();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *xiaoqiao, QVariant &data, ServerPlayer *) const
    {
        xiaoqiao->tag["TianxiangDamage"] = data;
        if (room->askForUseCard(xiaoqiao, "@@noltianxiang", "@noltianxiang-card", QVariant(), Card::MethodDiscard)) {

            return true;
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

    General *xiahouyuan = new General(this, "nol_xiahouyuan", "wei", 4, true, true);
    xiahouyuan->addSkill("shensu");

    General *weiyan = new General(this, "nol_weiyan", "shu", 4, true, true);
    weiyan->addSkill("kuanggu");
    weiyan->addSkill(new NOLQimou);
    weiyan->addSkill(new NOLQimouDistance);
    weiyan->addSkill(new NOLQimouTargetMod);
    related_skills.insertMulti("nolqimou", "#nolqimou-distance");
    related_skills.insertMulti("nolqimou", "#nolqimou-target");

    General *xiaoqiao = new General(this, "nol_xiaoqiao", "wu", 3, false, true);
    xiaoqiao->addSkill(new NOLTianxiang);
    xiaoqiao->addSkill(new NOLHongyan);

    General *zhangjiao = new General(this, "nol_zhangjiao$", "qun", 3, true, true);
    zhangjiao->addSkill(new NOLLeiji);
    zhangjiao->addSkill(new NOLGuidao);
    zhangjiao->addSkill("huangtian");

    General *yuji = new General(this, "nol_yuji", "qun", 3, true, true);
    yuji->addSkill(new NOLGuhuo);
    yuji->addRelateSkill("nolchanyuan");

    General *pangde = new General(this, "nol_pangde", "qun", 4, true, true);
    pangde->addSkill("mashu");
    pangde->addSkill(new NOLJianchu);

    addMetaObject<NOLQingjianAllotCard>();
    addMetaObject<NOLQimouCard>();
    addMetaObject<NOLGuhuoCard>();

    skills << new NOLQingjianAllot << new NOLChanyuan << new NOLChanyuanInvalidity;
    //skills << new NOLQingjianAllot;
    related_skills.insertMulti("nolchanyuan", "#nolchanyuan-inv");
}

ADD_PACKAGE(NostalOL)
