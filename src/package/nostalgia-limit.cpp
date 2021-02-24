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

    QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *target, QVariant &data, ServerPlayer *&) const
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
        return false;
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

class NJieWusheng : public OneCardViewAsSkill
{
public:
    NJieWusheng() : OneCardViewAsSkill("njiewusheng")
    {
        response_or_use = true;
        filter_pattern = ".|red";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return hasAvailable(player) || Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "slash";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Card *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->addSubcard(originalCard->getId());
        slash->setSkillName(objectName());
        return slash;
    }
};

class NPhyXunxun : public PhaseChangeSkill
{
public:
    NPhyXunxun() : PhaseChangeSkill("nphyxunxun")
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

NJieYijueCard::NJieYijueCard()
{
}

bool NJieYijueCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void NJieYijueCard::use(Room *room, ServerPlayer *guanyu, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    bool success = guanyu->pindian(target, "njieyijue", NULL);
    if (success) {
        target->addMark("njieyijue");
        room->setPlayerCardLimitation(target, "use,response", ".|.|.|hand", true);
        room->addPlayerTip(target, "#njieyijue");

        foreach(ServerPlayer *p, room->getAllPlayers())
            room->filterCards(p, p->getCards("he"), true);
        JsonArray args;
        args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
    } else {
        if (!target->isWounded()) return;
        target->setFlags("NJieYijueTarget");
        if (room->askForSkillInvoke(guanyu, "njieyijue_recover", "prompt:" + target->objectName()))
            room->recover(target, RecoverStruct(guanyu));
        target->setFlags("-NJieYijueTarget");
    }
}

class NJieYijueViewAsSkill : public ZeroCardViewAsSkill
{
public:
    NJieYijueViewAsSkill() : ZeroCardViewAsSkill("njieyijue")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("NJieYijueCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs() const
    {
        return new NJieYijueCard;
    }
};

class NJieYijue : public TriggerSkill
{
public:
    NJieYijue() : TriggerSkill("njieyijue")
    {
        events << EventPhaseChanging << Death;
        view_as_skill = new NJieYijueViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *) const
    {
        return false;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return;
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != target || target != room->getCurrent())
                return;
        }
        QList<ServerPlayer *> players = room->getAllPlayers();
        foreach (ServerPlayer *player, players) {
            if (player->getMark("njieyijue") == 0) continue;
            player->removeMark("njieyijue");
            room->removePlayerTip(player, "#njieyijue");

            foreach(ServerPlayer *p, room->getAllPlayers())
                room->filterCards(p, p->getCards("he"), false);

            JsonArray args;
            args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

            room->removePlayerCardLimitation(player, "use,response", ".|.|.|hand$1");
        }
    }
};

class NJiePaoxiao : public TargetModSkill
{
public:
    NJiePaoxiao() : TargetModSkill("njiepaoxiao")
    {
        frequency = Compulsory;
    }

    virtual int getResidueNum(const Player *from, const Card *, const Player *) const
    {
        if (from->hasSkill(this))
            return 1000;
        else
            return 0;
    }
};

class NJieTishen : public TriggerSkill
{
public:
    NJieTishen() : TriggerSkill("njietishen")
    {
        events << EventPhaseChanging << EventPhaseStart;
        frequency = Limited;
        limit_mark = "@njiesubstitute";
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseStart && TriggerSkill::triggerable(player)
                && player->getMark("@njiesubstitute") > 0 && player->getPhase() == Player::Start) {
            QString hp_str = player->property("njietishen_hp").toString();
            if (hp_str.isEmpty()) return QStringList();
            int hp = hp_str.toInt();
            int x = qMin(hp - player->getHp(), player->getMaxHp() - player->getHp());
            if (x > 0)
                return nameList();
        }
        return QStringList();
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                room->setPlayerProperty(player, "njietishen_hp", QString::number(player->getHp()));
            }
        }
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        QString hp_str = player->property("njietishen_hp").toString();
        if (hp_str.isEmpty()) return false;
        int hp = hp_str.toInt();
        int x = qMin(hp - player->getHp(), player->getMaxHp() - player->getHp());
        if (x > 0 && room->askForSkillInvoke(player, objectName(), QVariant::fromValue(x))) {
            room->removePlayerMark(player, "@njiesubstitute");
            player->broadcastSkillInvoke(objectName());
            //room->doLightbox("$NJieTishenAnimate");

            room->recover(player, RecoverStruct(player, NULL, x));
            player->drawCards(x, objectName());
        }
        return false;
    }
};

class NJieYajiao : public TriggerSkill
{
public:
    NJieYajiao() : TriggerSkill("njieyajiao")
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
                                 CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), "yajiao", QString()));
            room->moveCardsAtomic(move, true);

            int id = ids.first();
            const Card *card = Sanguosha->getCard(id);
            bool dealt = false;
            player->setMark("yajiao", id); // For AI
            if (card->getTypeId() == cardstar->getTypeId()) {
                ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(),
                                       QString("@yajiao-give:::%1:%2\\%3").arg(card->objectName())
                                       .arg(card->getSuitString() + "_char")
                                       .arg(card->getNumberString()),
                                       true);
                if (target) {
                    dealt = true;
                    CardMoveReason reason(CardMoveReason::S_REASON_DRAW, target->objectName(), "yajiao", QString());
                    room->obtainCard(target, card, reason);
                }
            } else {
                if (room->askForSkillInvoke(player, "yajiao_discard", "prompt:::" + card->objectName())) {
                    dealt = true;
                    CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), "yajiao", QString());
                    room->throwCard(card, reason, NULL);
                }
            }
            if (!dealt)
                room->returnToTopDrawPile(ids);
        }
        return false;
    }
};

ChuliCard::ChuliCard()
{
}

bool ChuliCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (to_select == Self) return false;
    QSet<QString> kingdoms;
    foreach(const Player *p, targets)
        kingdoms << p->getKingdom();
    return Self->canDiscard(to_select, "he") && !kingdoms.contains(to_select->getKingdom());
}

void ChuliCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    QList<ServerPlayer *> draw_card;
    QList<ServerPlayer *> to_discards = targets;
    to_discards << source;
    room->sortByActionOrder(to_discards);
    foreach (ServerPlayer *target, to_discards) {
        if (!source->canDiscard(target, "he")) continue;
        int id = room->askForCardChosen(source, target, "he", "chuli", false, Card::MethodDiscard);
        room->throwCard(id, target, source);
        if (Sanguosha->getCard(id)->getSuit() == Card::Spade)
            draw_card << target;
    }

    foreach(ServerPlayer *p, draw_card)
        room->drawCards(p, 1, "chuli");
}

class Chuli : public ZeroCardViewAsSkill
{
public:
    Chuli() : ZeroCardViewAsSkill("chuli")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->canDiscard(player, "he") && !player->hasUsed("ChuliCard");
    }

    virtual const Card *viewAs() const
    {
        return new ChuliCard;
    }
};

class NJieLiyu : public TriggerSkill
{
public:
    NJieLiyu() : TriggerSkill("njieliyu")
    {
        events << Damage;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data) const
    {
        TriggerList list;
        if (!TriggerSkill::triggerable(player)) return list;
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to->isAlive() && player != damage.to && !damage.to->hasFlag("Global_DebutFlag") && !damage.to->isNude()
                && damage.card && damage.card->isKindOf("Slash")) {
            list.insert(player, comList());
        }
        return list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to->askForSkillInvoke("njieliyu_invoke", "prompt:" + player->objectName())) {

            LogMessage log;
            log.type = "#InvokeOthersSkill";
            log.from = damage.to;
            log.to << player;
            log.arg = objectName();
            room->sendLog(log);
            player->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(player, objectName());

            int id = room->askForCardChosen(player, damage.to, "he", objectName());
            CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, player->objectName());
            room->obtainCard(player, Sanguosha->getCard(id),
                             reason, room->getCardPlace(id) != Player::PlaceHand);

            Duel *duel = new Duel(Card::NoSuit, 0);
            duel->setSkillName("_njieliyu");

            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p != damage.to && !player->isProhibited(p, duel))
                    targets << p;
            }
            if (targets.isEmpty()) {
                delete duel;
            } else {
                ServerPlayer *target = room->askForPlayerChosen(damage.to, targets, objectName(), "@njieliyu:" + player->objectName());
                if (target) {
                    if (player->isAlive() && target->isAlive() && !player->isLocked(duel))
                        room->useCard(CardUseStruct(duel, player, target));
                    else
                        delete duel;
                }
            }
        }
        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        if (card->isKindOf("Duel"))
            return 0;
        else
            return -1;
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

    General *lidian = new General(this, "nol_lidian", "wei", 3, true, true);
    lidian->addSkill("wangxi");
    lidian->addSkill(new NPhyXunxun);

    General *zhangliao = new General(this, "njie_zhangliao", "wei", 4, true, true);
    zhangliao->addSkill(new NJieTuxi);

    General *guojia = new General(this, "njie_guojia", "wei", 3, true, true);
    guojia->addSkill("tiandu");
    guojia->addSkill(new NJieYiji);

    General *xuchu = new General(this, "njie_xuchu", "wei", 4, true, true);
    xuchu->addSkill(new NJieLuoyi);

    General *guanyu = new General(this, "njie_guanyu", "shu", 4, true, true);
    guanyu->addSkill(new NJieWusheng);
    guanyu->addSkill(new NJieYijue);

    General *zhangfei = new General(this, "njie_zhangfei", "shu", 4, true, true);
    zhangfei->addSkill(new NJiePaoxiao);
    zhangfei->addSkill(new NJieTishen);

    General *zhaoyun = new General(this, "njie_zhaoyun", "shu", 4, true, true);
    zhaoyun->addSkill("longdan");
    zhaoyun->addSkill(new NJieYajiao);

    General *lvbu = new General(this, "njie_lvbu", "qun", 5, true, true);
    lvbu->addSkill("wushuang");
    lvbu->addSkill(new NJieLiyu);

    General *huatuo = new General(this, "njie_huatuo", "qun", 3, true, true);
    huatuo->addSkill(new Chuli);
    huatuo->addSkill("jijiu");

    addMetaObject<NJieTuxiCard>();
    addMetaObject<NJieYijiCard>();
    addMetaObject<NJieYijueCard>();
    addMetaObject<ChuliCard>();
}

ADD_PACKAGE(NostalLimitationBroken)
