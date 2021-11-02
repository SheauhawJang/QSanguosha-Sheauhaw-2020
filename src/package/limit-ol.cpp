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
#include "limit-ol.h"

class OLPaoxiaoTargetMod : public TargetModSkill
{
public:
    OLPaoxiaoTargetMod() : TargetModSkill("#olpaoxiao-target")
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

class OLPaoxiao : public TriggerSkill
{
public:
    OLPaoxiao() : TriggerSkill("olpaoxiao")
    {
        frequency = Compulsory;
        events << DamageCaused << SlashMissed << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == DamageCaused && player && player->isAlive() && player->getMark("#olpaoxiao") > 0) {
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
                room->setPlayerMark(player, "#olpaoxiao", 0);
        } else if (triggerEvent == SlashMissed && TriggerSkill::triggerable(player)) {
            room->addPlayerMark(player, "#olpaoxiao");
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
        damage.damage += player->getMark("#olpaoxiao");
        data = QVariant::fromValue(damage);
        room->setPlayerMark(player, "#olpaoxiao", 0);
        return false;
    }
};

class OLTishen : public TriggerSkill
{
public:
    OLTishen() : TriggerSkill("oltishen")
    {
        events << EventPhaseStart;
        frequency = Limited;
        limit_mark = "@olsubstitute";
    }

    virtual bool triggerable(const ServerPlayer *player) const
    {
        return TriggerSkill::triggerable(player) && player->getMark("@olsubstitute") > 0 && player->getPhase() == Player::Start && player->isWounded();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (triggerEvent == EventPhaseStart) {
            int delta = player->getMaxHp() - player->getHp();
            if (room->askForSkillInvoke(player, objectName(), QVariant::fromValue(delta))) {
                room->removePlayerMark(player, "@olsubstitute");
                player->broadcastSkillInvoke(objectName());
                //room->doLightbox("$OLTishenAnimate");

                room->recover(player, RecoverStruct(player, NULL, delta));
                player->drawCards(delta, objectName());
            }
        }
        return false;
    }
};

class OLLongdan : public OneCardViewAsSkill
{
public:
    OLLongdan() : OneCardViewAsSkill("ollongdan")
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

class OLYajiao : public TriggerSkill
{
public:
    OLYajiao() : TriggerSkill("olyajiao")
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
                                 CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), "olyajiao", QString()));
            room->moveCardsAtomic(move, true);

            int id = ids.first();
            const Card *card = Sanguosha->getCard(id);
            bool dealt = false;
            player->setMark("olyajiao", id); // For AI
            if (card->getTypeId() == cardstar->getTypeId()) {
                ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(),
                                       QString("@olyajiao-give:::%1:%2\\%3").arg(card->objectName())
                                       .arg(card->getSuitString() + "_char")
                                       .arg(card->getNumberString()),
                                       true);
                if (target) {
                    dealt = true;
                    CardMoveReason reason(CardMoveReason::S_REASON_DRAW, target->objectName(), "olyajiao", QString());
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

OLQimouCard::OLQimouCard()
{
    target_fixed = true;
}

void OLQimouCard::extraCost(Room *room, const CardUseStruct &card_use) const
{
    room->removePlayerMark(card_use.from, "@scheme");
    room->loseHp(card_use.from, user_string.toInt());
}

void OLQimouCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->drawCards(user_string.toInt(), "olqimou");
    room->setPlayerMark(source, "#olqimou", user_string.toInt());
}

class OLQimouViewAsSkill : public ZeroCardViewAsSkill
{
public:
    OLQimouViewAsSkill() : ZeroCardViewAsSkill("olqimou")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@scheme") > 0 && player->getHp() > 0;
    }

    virtual const Card *viewAs() const
    {
        QString user_string = Self->tag["olqimou"].toString();
        if (user_string.isEmpty()) return NULL;
        OLQimouCard *skill_card = new OLQimouCard;
        skill_card->setUserString(user_string);
        skill_card->setSkillName("olqimou");
        return skill_card;
    }
};

class OLQimou : public TriggerSkill
{
public:
    OLQimou() : TriggerSkill("olqimou")
    {
        events << EventPhaseStart;
        frequency = Limited;
        limit_mark = "@scheme";
        view_as_skill = new OLQimouViewAsSkill;
    }

    virtual void record(TriggerEvent, Room *room, ServerPlayer *weiyan, QVariant &) const
    {
        if (weiyan->getPhase() == Player::NotActive)
            room->setPlayerMark(weiyan, "#olqimou", 0);
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

class OLQimouDistance : public DistanceSkill
{
public:
    OLQimouDistance() : DistanceSkill("#olqimou-distance")
    {
    }

    virtual int getCorrect(const Player *from, const Player *) const
    {
        return -from->getMark("#olqimou");
    }
};

class OLQimouTargetMod : public TargetModSkill
{
public:
    OLQimouTargetMod() : TargetModSkill("#olqimou-target")
    {
    }

    virtual int getResidueNum(const Player *from, const Card *, const Player *) const
    {
        return from->getMark("#olqimou");
    }
};

OLGuhuoCard::OLGuhuoCard()
{
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "olguhuo";
}

bool OLGuhuoCard::olguhuo(ServerPlayer *yuji) const
{
    Room *room = yuji->getRoom();

    CardMoveReason reason1(CardMoveReason::S_REASON_SECRETLY_PUT, yuji->objectName(), QString(), "olguhuo", QString());
    room->moveCardTo(this, NULL, Player::PlaceTable, reason1, false);

    QList<ServerPlayer *> players = room->getOtherPlayers(yuji);

    room->setTag("OLGuhuoType", user_string);

    ServerPlayer *questioned = NULL;
    foreach (ServerPlayer *player, players) {
        if (player->hasSkill("olchanyuan")) {
            LogMessage log;
            log.type = "#OLChanyuan";
            log.from = player;
            log.arg = "olchanyuan";
            room->sendLog(log);

            continue;
        }

        QString choice = room->askForChoice(player, "olguhuo", "noquestion+question");

        LogMessage log;
        log.type = "#OLGuhuoQuery";
        log.from = player;
        log.arg = choice;
        room->sendLog(log);
        if (choice == "question") {
            questioned = player;
            break;
        }
    }

    LogMessage log;
    log.type = "$OLGuhuoResult";
    log.from = yuji;
    log.card_str = QString::number(subcards.first());
    room->sendLog(log);

    const Card *card = Sanguosha->getCard(subcards.first());

    CardMoveReason reason_ui(CardMoveReason::S_REASON_TURNOVER, yuji->objectName(), QString(), "olguhuo", QString());
    CardResponseStruct resp(card);
    reason_ui.m_extraData = QVariant::fromValue(resp);
    room->showVirtualMove(reason_ui);

    bool success = true;
    if (questioned) {
        success = (card->objectName() == user_string);

        if (success) {
            QString choice = room->askForChoice(questioned, objectName(), "discard+losshp");
            if (choice == "discard")
                room->askForDiscard(questioned, objectName(), 1, 1, false, true);
            else
                room->loseHp(questioned);
            room->acquireSkill(questioned, "olchanyuan");
        } else {
            room->moveCardTo(this, yuji, NULL, Player::DiscardPile,
                             CardMoveReason(CardMoveReason::S_REASON_PUT, yuji->objectName(), QString(), "olguhuo"), true);
            room->drawCards(questioned, 1, objectName());
        }
    }
    room->setPlayerFlag(yuji, "OLGuhuoUsed");
    return success;
}

bool OLGuhuoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
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

bool OLGuhuoCard::targetFixed() const
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

bool OLGuhuoCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
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

const Card *OLGuhuoCard::validate(CardUseStruct &card_use) const
{
    ServerPlayer *yuji = card_use.from;
    Room *room = yuji->getRoom();

    QString to_olguhuo = user_string;
    yuji->broadcastSkillInvoke("olguhuo");

    LogMessage log;
    log.type = card_use.to.isEmpty() ? "#OLGuhuoNoTarget" : "#OLGuhuo";
    log.from = yuji;
    log.to = card_use.to;
    log.arg = to_olguhuo;
    log.arg2 = "olguhuo";
    room->sendLog(log);

    const Card *olguhuo_card = Sanguosha->cloneCard(user_string, Card::NoSuit, 0);
    if (card_use.to.isEmpty())
        card_use.to = olguhuo_card->defaultTargets(room, yuji);

    foreach (ServerPlayer *to, card_use.to)
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, yuji->objectName(), to->objectName());

    CardMoveReason reason_ui(CardMoveReason::S_REASON_USE, yuji->objectName(), QString(), "olguhuo", QString());
    if (card_use.to.size() == 1 && !card_use.card->targetFixed())
        reason_ui.m_targetId = card_use.to.first()->objectName();

    CardUseStruct olguhuo_use = card_use;
    olguhuo_use.card = olguhuo_card;
    reason_ui.m_extraData = QVariant::fromValue(olguhuo_use);
    room->showVirtualMove(reason_ui);

    if (olguhuo(card_use.from)) {
        const Card *card = Sanguosha->getCard(subcards.first());
        Card *use_card = Sanguosha->cloneCard(to_olguhuo, card->getSuit(), card->getNumber());
        use_card->setSkillName("_olguhuo");
        use_card->addSubcard(subcards.first());
        use_card->deleteLater();
        return use_card;
    } else
        return NULL;
}

const Card *OLGuhuoCard::validateInResponse(ServerPlayer *yuji) const
{
    Room *room = yuji->getRoom();
    yuji->broadcastSkillInvoke("olguhuo");

    QString to_olguhuo = user_string;

    LogMessage log;
    log.type = "#OLGuhuoNoTarget";
    log.from = yuji;
    log.arg = to_olguhuo;
    log.arg2 = "olguhuo";
    room->sendLog(log);

    CardMoveReason reason_ui(CardMoveReason::S_REASON_RESPONSE, yuji->objectName(), QString(), "olguhuo", QString());
    const Card *olguhuo_card = Sanguosha->cloneCard(user_string, Card::NoSuit, 0);
    CardResponseStruct resp(olguhuo_card);
    reason_ui.m_extraData = QVariant::fromValue(resp);
    room->showVirtualMove(reason_ui);

    if (olguhuo(yuji)) {
        const Card *card = Sanguosha->getCard(subcards.first());
        Card *use_card = Sanguosha->cloneCard(to_olguhuo, card->getSuit(), card->getNumber());
        use_card->setSkillName("_olguhuo");
        use_card->addSubcard(subcards.first());
        use_card->deleteLater();
        return use_card;
    } else
        return NULL;
}

class OLGuhuo : public OneCardViewAsSkill
{
public:
    OLGuhuo() : OneCardViewAsSkill("olguhuo")
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

        if (player->hasFlag("OLGuhuoUsed") || pattern.startsWith(".") || pattern.startsWith("@"))
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
        return !player->hasFlag("OLGuhuoUsed");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        OLGuhuoCard *card = new OLGuhuoCard;
        card->addSubcard(originalCard);
        return card;
    }

    QString getSelectBox() const
    {
        return "olguhuo_bt";
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        if (card->isKindOf("OLGuhuoCard"))
            return -1;
        else
            return 0;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        ServerPlayer *current = player->getRoom()->getCurrent();
        if (!current || current->isDead() || current->getPhase() == Player::NotActive) return false;
        return !player->isKongcheng() && !player->hasFlag("OLGuhuoUsed");
    }
};

class OLChanyuan : public TriggerSkill
{
public:
    OLChanyuan() : TriggerSkill("olchanyuan")
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
            room->removePlayerTip(player, "#olchanyuan");
        } else if (triggerEvent == EventAcquireSkill) {
            if (data.toString() != objectName()) return false;
            room->addPlayerTip(player, "#olchanyuan");
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

class OLChanyuanInvalidity : public InvaliditySkill
{
public:
    OLChanyuanInvalidity() : InvaliditySkill("#olchanyuan-inv")
    {
    }

    virtual bool isSkillValid(const Player *player, const Skill *skill) const
    {
        return skill->objectName().contains("olchanyuan") || !player->hasSkill("olchanyuan")
               || player->getHp() > 1 || skill->isAttachedLordSkill();
    }
};

class OLJianchu : public TriggerSkill
{
public:
    OLJianchu() : TriggerSkill("oljianchu")
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

class OLYaowu : public TriggerSkill
{
public:
    OLYaowu() : TriggerSkill("olyaowu")
    {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player)) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && (!damage.card->isRed() || (damage.from && damage.from->isAlive()))) {
                return nameList();
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {

        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card) {
            if (damage.card->isRed()) {
                if (damage.from && damage.from->isAlive()) {
                    room->sendCompulsoryTriggerLog(player, objectName());
                    player->broadcastSkillInvoke(objectName());
                    damage.from->drawCards(1, objectName());
                }
            } else {
                room->sendCompulsoryTriggerLog(player, objectName());
                player->broadcastSkillInvoke(objectName());
                player->drawCards(1, objectName());
            }
        }
        return false;
    }
};

class Shebian : public TriggerSkill
{
public:
    Shebian() : TriggerSkill("shebian")
    {
        events << TurnedOver;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {

        if (room->askForSkillInvoke(player, objectName())) {
            player->broadcastSkillInvoke(objectName());
            room->askForUseCard(player, "@@oljiewei_move", "@shebian-move");
        }
        return false;
    }
};

class OLHongyan : public FilterSkill
{
public:
    OLHongyan() : FilterSkill("olhongyan")
    {
    }

    static WrappedCard *changeToHeart(int cardId)
    {
        WrappedCard *new_card = Sanguosha->getWrappedCard(cardId);
        new_card->setSkillName("olhongyan");
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

class OLHongyanMaxCard : public MaxCardsSkill
{
public:
    OLHongyanMaxCard() : MaxCardsSkill("#olhongyan")
    {
    }

    virtual int getFixed(const Player *target) const
    {
        if (target->hasSkill("olhongyan"))
            foreach (const Card *card, target->getEquips())
                if (card->getSuit() == Card::Heart)
                    return target->getMaxHp();
        return -1;
    }
};

OLTianxiangCard::OLTianxiangCard()
{
}

void OLTianxiangCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *xiaoqiao = effect.from, *target = effect.to;
    Room *room = xiaoqiao->getRoom();
    QVariant data = xiaoqiao->tag.value("OLTianxiangDamage");
    DamageStruct damage = data.value<DamageStruct>();
    room->preventDamage(damage);
    const Card *card = Sanguosha->getCard(getEffectiveId());
    QStringList choices;
    choices << "losehp";
    if (damage.from && damage.from->isAlive())
        choices << "damage";

    if (room->askForChoice(xiaoqiao, "oltianxiang", choices.join("+"), data,
                           "@oltianxiang-choose::" + target->objectName() + ":" + card->objectName(), "damage+losehp") == "damage") {
        room->damage(DamageStruct("oltianxiang", damage.from, target));
        if (target->isAlive() && target->getLostHp() > 0)
            target->drawCards(qMin(target->getLostHp(), 5), "oltianxiang");
    } else {
        room->loseHp(target);
        int id = getEffectiveId();
        Player::Place place = room->getCardPlace(id);
        if (target->isAlive() && (place == Player::DiscardPile || place == Player::DrawPile))
            target->obtainCard(this);
    }
}

class OLTianxiangViewAsSkill : public OneCardViewAsSkill
{
public:
    OLTianxiangViewAsSkill() : OneCardViewAsSkill("oltianxiang")
    {
        filter_pattern = ".|heart!";
        response_pattern = "@@oltianxiang";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        OLTianxiangCard *oltianxiangCard = new OLTianxiangCard;
        oltianxiangCard->addSubcard(originalCard);
        return oltianxiangCard;
    }
};

class OLTianxiang : public TriggerSkill
{
public:
    OLTianxiang() : TriggerSkill("oltianxiang")
    {
        events << DamageInflicted;
        view_as_skill = new OLTianxiangViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && !target->isKongcheng();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *xiaoqiao, QVariant &data, ServerPlayer *) const
    {
        xiaoqiao->tag["OLTianxiangDamage"] = data;
        if (room->askForUseCard(xiaoqiao, "@@oltianxiang", "@oltianxiang-card", QVariant(), Card::MethodDiscard)) {

            return true;
        }
        return false;
    }
};

class Piaoling : public TriggerSkill
{
public:
    Piaoling() : TriggerSkill("piaoling")
    {
        events << EventPhaseChanging << FinishJudge;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseChanging) {
            if (!TriggerSkill::triggerable(player)) return QStringList();
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                return nameList();
        } else {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == objectName() && judge->isGood() && room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge)
                return comList();
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *xiaoqiao, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == EventPhaseChanging) {
            if (room->askForSkillInvoke(xiaoqiao, objectName(), data)) {
                xiaoqiao->broadcastSkillInvoke(objectName());
                JudgeStruct judge;
                judge.good = true;
                judge.pattern = ".|heart";
                judge.who = xiaoqiao;
                judge.reason = objectName();

                room->judge(judge);
            }
        } else {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            ServerPlayer *target = room->askForPlayerChosen(xiaoqiao, room->getAlivePlayers(), objectName(), "@piaoling-target", true);
            if (target) {
                target->obtainCard(judge->card);
                if (target == xiaoqiao)
                    room->askForDiscard(xiaoqiao, objectName(), 1, 1, false, true);
            } else {
                CardMoveReason reason(CardMoveReason::S_REASON_PUT, xiaoqiao->objectName(), objectName(), QString());
                room->moveCardTo(judge->card, NULL, Player::DrawPile, reason);
            }
        }
        return false;
    }
};

class OLLeiji : public TriggerSkill
{
public:
    OLLeiji() : TriggerSkill("olleiji")
    {
        events << CardUsed << CardResponded << FinishJudge;
        view_as_skill = new dummyVS;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player)) {
            if (triggerEvent == FinishJudge) {
                JudgeStruct *judge = data.value<JudgeStruct *>();
                if (judge->card->isBlack())
                    return QStringList("olleiji!");
                return QStringList();
            }
            const Card *card = NULL;
            if (triggerEvent == CardUsed)
                card = data.value<CardUseStruct>().card;
            else if (triggerEvent == CardResponded)
                card = data.value<CardResponseStruct>().m_card;
            if (card == NULL) return QStringList();

            if (card->isKindOf("Jink") || card->isKindOf("Lightning"))
                return nameList();
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *zhangjiao, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == FinishJudge) {
            room->sendCompulsoryTriggerLog(zhangjiao, objectName());
            zhangjiao->broadcastSkillInvoke(objectName());
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->card->getSuit() == Card::Club)
                room->recover(zhangjiao, RecoverStruct(zhangjiao));
            int x = (judge->card->getSuit() == Card::Club ? 1 : 2);
            ServerPlayer *vic = room->askForPlayerChosen(zhangjiao, room->getAlivePlayers(), objectName(),
                                "@olleiji-choose:::" + QString::number(x));
            room->damage(DamageStruct(objectName(), zhangjiao, vic, x, DamageStruct::Thunder));

        } else {
            if (zhangjiao->askForSkillInvoke(objectName())) {
                zhangjiao->broadcastSkillInvoke(objectName());
                JudgeStruct judge;
                judge.pattern = ".";
                judge.who = zhangjiao;
                judge.reason = objectName();
                judge.play_animation = false;
                room->judge(judge);
            }
        }
        return false;
    }
};

class OLGuidao : public TriggerSkill
{
public:
    OLGuidao() : TriggerSkill("olguidao")
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
        prompt_list << "@olguidao-card" << judge->who->objectName()
                    << objectName() << judge->reason << QString::number(judge->card->getEffectiveId());
        QString prompt = prompt_list.join(":");
        const Card *card = room->askForCard(player, ".|black", prompt, data, Card::MethodResponse, judge->who, true, "olguidao");

        if (card != NULL) {
            room->retrial(card, player, judge, objectName(), true);
            const Card *judge_card = Sanguosha->getCard(card->getEffectiveId());
            if (judge_card->getSuit() == Card::Spade && judge_card->getNumber() > 1 && judge_card->getNumber() < 10)
                player->drawCards(1, objectName());
        }
        return false;
    }
};

OLQiangxiCard::OLQiangxiCard()
{
}

bool OLQiangxiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty() || to_select == Self)
        return false;

    QStringList olqiangxi_prop = Self->property("olqiangxi").toString().split("+");
    if (olqiangxi_prop.contains(to_select->objectName()))
        return false;

    return true;
}

void OLQiangxiCard::extraCost(Room *room, const CardUseStruct &card_use) const
{
    if (subcards.isEmpty())
        room->loseHp(card_use.from);
    else {
        CardMoveReason reason(CardMoveReason::S_REASON_THROW, card_use.from->objectName(), QString(), card_use.card->getSkillName(), QString());
        room->moveCardTo(this, NULL, Player::DiscardPile, reason, true);
    }
}

void OLQiangxiCard::onEffect(const CardEffectStruct &effect) const
{
    QSet<QString> olqiangxi_prop = effect.from->property("olqiangxi").toString().split("+").toSet();
    olqiangxi_prop.insert(effect.to->objectName());
    effect.from->getRoom()->setPlayerProperty(effect.from, "olqiangxi", QStringList(olqiangxi_prop.toList()).join("+"));
    effect.to->getRoom()->damage(DamageStruct("olqiangxi", effect.from, effect.to));
}

class OLQiangxiViewAsSkill : public ViewAsSkill
{
public:
    OLQiangxiViewAsSkill() : ViewAsSkill("olqiangxi")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return  player->usedTimes("OLQiangxiCard") < 2;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.isEmpty() && to_select->isKindOf("Weapon") && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return new OLQiangxiCard;
        else if (cards.length() == 1) {
            OLQiangxiCard *card = new OLQiangxiCard;
            card->addSubcards(cards);

            return card;
        } else
            return NULL;
    }
};

class OLQiangxi : public TriggerSkill
{
public:
    OLQiangxi() : TriggerSkill("olqiangxi")
    {
        events << EventPhaseChanging;
        view_as_skill = new OLQiangxiViewAsSkill;
    }

    bool triggerable(const ServerPlayer *) const
    {
        return false;
    }

    void record(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
            room->setPlayerProperty(player, "olqiangxi", QVariant());
        }
    }
};

LimitOLPackage::LimitOLPackage()
    : Package("limit_ol")
{

    General *zhangfei = new General(this, "ol_zhangfei", "shu", 4, true, true);
    zhangfei->addSkill(new OLPaoxiao);
    zhangfei->addSkill(new OLPaoxiaoTargetMod);
    zhangfei->addSkill(new OLTishen);
    related_skills.insertMulti("olpaoxiao", "#olpaoxiao-target");

    General *zhaoyun = new General(this, "ol_zhaoyun", "shu", 4, true, true);
    zhaoyun->addSkill(new OLLongdan);
    zhaoyun->addSkill(new OLYajiao);

    General *xiahouyuan = new General(this, "ol_xiahouyuan", "wei", 4, true, true);
    xiahouyuan->addSkill("shensu");
    xiahouyuan->addSkill(new Shebian);

    General *huaxiong = new General(this, "ol_huaxiong", "qun", 6, true, true);
    huaxiong->addSkill(new OLYaowu);

    General *weiyan = new General(this, "ol_weiyan", "shu", 4, true, true);
    weiyan->addSkill("kuanggu");
    weiyan->addSkill(new OLQimou);
    weiyan->addSkill(new OLQimouDistance);
    weiyan->addSkill(new OLQimouTargetMod);
    related_skills.insertMulti("olqimou", "#olqimou-distance");
    related_skills.insertMulti("olqimou", "#olqimou-target");

    General *xiaoqiao = new General(this, "ol_xiaoqiao", "wu", 3, false, true);
    xiaoqiao->addSkill(new OLTianxiang);
    xiaoqiao->addSkill(new OLHongyan);
    xiaoqiao->addSkill(new OLHongyanMaxCard);
    xiaoqiao->addSkill(new Piaoling);
    related_skills.insertMulti("olhongyan", "#olhongyan");

    General *zhangjiao = new General(this, "ol_zhangjiao$", "qun", 3, true, true);
    zhangjiao->addSkill(new OLLeiji);
    zhangjiao->addSkill(new OLGuidao);
    zhangjiao->addSkill("huangtian");

    General *yuji = new General(this, "ol_yuji", "qun", 3, true, true);
    yuji->addSkill(new OLGuhuo);
    yuji->addRelateSkill("olchanyuan");

    General *dianwei = new General(this, "ol_dianwei", "wei", 4, true, true);
    dianwei->addSkill(new OLQiangxi);

    General *pangde = new General(this, "ol_pangde", "qun", 4, true, true);
    pangde->addSkill("mashu");
    pangde->addSkill(new OLJianchu);

    addMetaObject<OLQimouCard>();
    addMetaObject<OLGuhuoCard>();
    addMetaObject<OLTianxiangCard>();
    addMetaObject<OLQiangxiCard>();

    skills << new OLChanyuan << new OLChanyuanInvalidity;
    related_skills.insertMulti("olchanyuan", "#olchanyuan-inv");
}

ADD_PACKAGE(LimitOL)
