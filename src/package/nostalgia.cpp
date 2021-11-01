#include "standard.h"
#include "skill.h"
#include "wind.h"
#include "client.h"
#include "engine.h"
#include "nostalgia.h"
#include "yjcm.h"
#include "yjcm2013.h"
#include "settings.h"

class NosWuyan : public TriggerSkill
{
public:
    NosWuyan() : TriggerSkill("noswuyan")
    {
        events << CardEffected;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (effect.to == effect.from)
            return false;
        if (effect.card->isNDTrick()) {
            if (effect.from && effect.from->hasSkill(this)) {
                LogMessage log;
                log.type = "#WuyanBaD";
                log.from = effect.from;
                log.to << effect.to;
                log.arg = effect.card->objectName();
                log.arg2 = objectName();
                room->sendLog(log);
                room->notifySkillInvoked(effect.from, objectName());
                effect.from->broadcastSkillInvoke("noswuyan");
                return true;
            }
            if (effect.to->hasSkill(this) && effect.from) {
                LogMessage log;
                log.type = "#WuyanGooD";
                log.from = effect.to;
                log.to << effect.from;
                log.arg = effect.card->objectName();
                log.arg2 = objectName();
                room->sendLog(log);
                room->notifySkillInvoked(effect.to, objectName());
                effect.to->broadcastSkillInvoke("noswuyan");
                return true;
            }
        }
        return false;
    }
};

NosJujianCard::NosJujianCard()
{
}

void NosJujianCard::onEffect(const CardEffectStruct &effect) const
{
    int n = subcardsLength();
    effect.to->drawCards(n, "nosjujian");
    Room *room = effect.from->getRoom();

    if (effect.from->isAlive() && n == 3) {
        QSet<Card::CardType> types;
        foreach(int card_id, effect.card->getSubcards())
            types << Sanguosha->getCard(card_id)->getTypeId();

        if (types.size() == 1) {
            LogMessage log;
            log.type = "#JujianRecover";
            log.from = effect.from;
            const Card *card = Sanguosha->getCard(subcards.first());
            log.arg = card->getType();
            room->sendLog(log);
            room->recover(effect.from, RecoverStruct(effect.from));
        }
    }
}

class NosJujian : public ViewAsSkill
{
public:
    NosJujian() : ViewAsSkill("nosjujian")
    {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.length() < 3 && !Self->isJilei(to_select);
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->canDiscard(player, "he") && !player->hasUsed("NosJujianCard");
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return NULL;

        NosJujianCard *card = new NosJujianCard;
        card->addSubcards(cards);
        return card;
    }
};

class NosEnyuan : public TriggerSkill
{
public:
    NosEnyuan() : TriggerSkill("nosenyuan")
    {
        events << HpRecover << Damaged;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == HpRecover) {
            RecoverStruct recover = data.value<RecoverStruct>();
            if (recover.who && recover.who != player) {
                room->broadcastSkillInvoke("nosenyuan", player, QList<int>() << 1 << 2);
                room->sendCompulsoryTriggerLog(player, objectName());
                recover.who->drawCards(recover.recover, objectName());
            }
        } else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *source = damage.from;
            if (source && source != player) {
                room->broadcastSkillInvoke("nosenyuan", player, QList<int>() << 3 << 4);
                room->sendCompulsoryTriggerLog(player, objectName());

                const Card *card = room->askForCard(source, ".|heart|.|hand", "@nosenyuan-heart", data, Card::MethodNone);
                if (card)
                    player->obtainCard(card);
                else
                    room->loseHp(source);
            }
        }

        return false;
    }
};

NosXuanhuoCard::NosXuanhuoCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

void NosXuanhuoCard::onEffect(const CardEffectStruct &effect) const
{
    effect.to->obtainCard(this);

    Room *room = effect.from->getRoom();
    int card_id = room->askForCardChosen(effect.from, effect.to, "he", "nosxuanhuo");
    CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, effect.from->objectName());
    room->obtainCard(effect.from, Sanguosha->getCard(card_id), reason, room->getCardPlace(card_id) != Player::PlaceHand);

    QList<ServerPlayer *> targets = room->getOtherPlayers(effect.to);
    ServerPlayer *target = room->askForPlayerChosen(effect.from, targets, "nosxuanhuo", "@nosxuanhuo-give:" + effect.to->objectName());
    if (target != effect.from) {
        CardMoveReason reason2(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), target->objectName(), "nosxuanhuo", QString());
        room->obtainCard(target, Sanguosha->getCard(card_id), reason2, false);
    }
}

class NosXuanhuo : public OneCardViewAsSkill
{
public:
    NosXuanhuo() : OneCardViewAsSkill("nosxuanhuo")
    {
        filter_pattern = ".|heart|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->isKongcheng() && !player->hasUsed("NosXuanhuoCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        NosXuanhuoCard *xuanhuoCard = new NosXuanhuoCard;
        xuanhuoCard->addSubcard(originalCard);
        return xuanhuoCard;
    }
};

class NosXuanfeng : public TriggerSkill
{
public:
    NosXuanfeng() : TriggerSkill("nosxuanfeng")
    {
        events << CardsMoveOneTime;
        view_as_skill = new dummyVS;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &datas, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == CardsMoveOneTime)
            foreach (QVariant data, datas.toList()) {
                CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
                if (move.from == player && move.from_places.contains(Player::PlaceEquip))
                    return nameList();
            }

        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *lingtong, QVariant &, ServerPlayer *) const
    {
        QStringList choicelist;
        QList<ServerPlayer *> targets1;
        foreach (ServerPlayer *target, room->getAlivePlayers()) {
            if (lingtong->canSlash(target, NULL, false))
                targets1 << target;
        }
        Slash *slashx = new Slash(Card::NoSuit, 0);
        if (!targets1.isEmpty() && !lingtong->isCardLimited(slashx, Card::MethodUse))
            choicelist << "slash";
        slashx->deleteLater();
        QList<ServerPlayer *> targets2;
        foreach (ServerPlayer *p, room->getOtherPlayers(lingtong)) {
            if (lingtong->distanceTo(p) <= 1)
                targets2 << p;
        }
        if (!targets2.isEmpty()) choicelist << "damage";
        choicelist << "nothing";

        QString choice = room->askForChoice(lingtong, objectName(), choicelist.join("+"));
        if (choice == "slash") {
            ServerPlayer *target = room->askForPlayerChosen(lingtong, targets1, "nosxuanfeng_slash", "@dummy-slash");
            lingtong->broadcastSkillInvoke(objectName());
            Slash *slash = new Slash(Card::NoSuit, 0);
            slash->setSkillName(objectName());
            room->useCard(CardUseStruct(slash, lingtong, target));
        } else if (choice == "damage") {
            lingtong->broadcastSkillInvoke(objectName());

            LogMessage log;
            log.type = "#InvokeSkill";
            log.from = lingtong;
            log.arg = objectName();
            room->sendLog(log);
            room->notifySkillInvoked(lingtong, objectName());

            ServerPlayer *target = room->askForPlayerChosen(lingtong, targets2, "nosxuanfeng_damage", "@nosxuanfeng-damage");
            room->damage(DamageStruct("nosxuanfeng", lingtong, target));
        }

        return false;
    }
};

XinzhanCard::XinzhanCard()
{
    target_fixed = true;
}

void XinzhanCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    QList<int> card_ids = room->getNCards(3);

    LogMessage log;
    log.type = "$ViewDrawPile";
    log.from = source;
    log.card_str = IntList2StringList(card_ids).join("+");
    room->sendLog(log, source);
    AskForMoveCardsStruct result = room->askForMoveCards(source, card_ids, QList<int>(), true, "xinzhan", "heart", 0, 0, false, false, QList<int>() << -1);
    for (int i = result.top.length() - 1; i >= 0; i--)
        room->getDrawPile().prepend(result.top.at(i));
    room->doBroadcastNotify(QSanProtocol::S_COMMAND_UPDATE_PILE, QVariant(room->getDrawPile().length()));
    QList<int> selected = result.bottom;
    if (selected.isEmpty()) return;
    DummyCard *dummy = new DummyCard(selected);
    room->obtainCard(source, dummy, true);
    delete dummy;
}

class Xinzhan : public ZeroCardViewAsSkill
{
public:
    Xinzhan() : ZeroCardViewAsSkill("xinzhan")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("XinzhanCard") && player->getHandcardNum() > player->getMaxHp();
    }

    virtual const Card *viewAs() const
    {
        return new XinzhanCard;
    }
};

class Huilei : public TriggerSkill
{
public:
    Huilei() : TriggerSkill("huilei")
    {
        events << Death;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->hasSkill(this);
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who != player)
            return false;
        ServerPlayer *killer = death.damage ? death.damage->from : NULL;
        if (killer && killer != player) {
            LogMessage log;
            log.type = "#HuileiThrow";
            log.from = player;
            log.to << killer;
            log.arg = objectName();
            room->sendLog(log);
            room->notifySkillInvoked(player, objectName());

            room->broadcastSkillInvoke(objectName(), player);

            killer->throwAllHandCardsAndEquips();
        }

        return false;
    }
};

class NosPojun : public TriggerSkill
{
public:
    NosPojun() : TriggerSkill("nospojun")
    {
        events << Damage;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Slash") && !damage.chain && !damage.transfer
                && damage.to->isAlive() && !damage.to->hasFlag("Global_DebutFlag")
                && room->askForSkillInvoke(player, objectName(), QVariant::fromValue(damage.to))) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.to->objectName());
            int x = qMin(5, damage.to->getHp());
            player->broadcastSkillInvoke(objectName());
            damage.to->drawCards(x, objectName());
            damage.to->turnOver();
        }
        return false;
    }
};

class Yizhong : public TriggerSkill
{
public:
    Yizhong() : TriggerSkill("yizhong")
    {
        events << SlashEffected;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && TriggerSkill::triggerable(target) && target->getArmor() == NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (effect.slash->isBlack()) {
            player->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(player, objectName());

            LogMessage log;
            log.type = "#SkillNullify";
            log.from = player;
            log.arg = objectName();
            log.arg2 = effect.slash->objectName();
            room->sendLog(log);

            return true;
        }
        return false;
    }
};

class NosFuhun : public TriggerSkill
{
public:
    NosFuhun() : TriggerSkill("nosfuhun")
    {
        events << EventPhaseStart << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *shuangying, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart && shuangying->getPhase() == Player::Draw && TriggerSkill::triggerable(shuangying)) {
            if (shuangying->askForSkillInvoke(this)) {
                shuangying->broadcastSkillInvoke(objectName());
                int card1 = room->drawCard();
                int card2 = room->drawCard();
                QList<int> ids;
                ids << card1 << card2;
                bool diff = (Sanguosha->getCard(card1)->getColor() != Sanguosha->getCard(card2)->getColor());

                CardsMoveStruct move;
                move.card_ids = ids;
                move.reason = CardMoveReason(CardMoveReason::S_REASON_TURNOVER, shuangying->objectName(), "fuhun", QString());
                move.to_place = Player::PlaceTable;
                room->moveCardsAtomic(move, true);
                room->getThread()->delay();

                DummyCard *dummy = new DummyCard(move.card_ids);
                room->obtainCard(shuangying, dummy);
                delete dummy;

                if (diff) {
                    room->handleAcquireDetachSkills(shuangying, "wusheng|paoxiao");
                    shuangying->setFlags(objectName());
                }

                return true;
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive && shuangying->hasFlag(objectName()))
                room->handleAcquireDetachSkills(shuangying, "-wusheng|-paoxiao", true);
        }

        return false;
    }
};

class NosGongqi : public OneCardViewAsSkill
{
public:
    NosGongqi() : OneCardViewAsSkill("nosgongqi")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return hasAvailable(player) || Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "slash";
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        if (to_select->getTypeId() != Card::TypeEquip)
            return false;

        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
            Slash *slash = new Slash(Card::SuitToBeDecided, -1);
            slash->addSubcard(to_select->getEffectiveId());
            slash->deleteLater();
            return slash->isAvailable(Self);
        }
        return true;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->addSubcard(originalCard);
        slash->setSkillName(objectName());
        return slash;
    }
};

class NosGongqiTargetMod : public TargetModSkill
{
public:
    NosGongqiTargetMod() : TargetModSkill("#nosgongqi-target")
    {
        frequency = NotFrequent;
    }

    virtual int getDistanceLimit(const Player *, const Card *card, const Player *) const
    {
        if (card->getSkillName() == "nosgongqi")
            return 1000;
        else
            return 0;
    }
};

NosJiefanCard::NosJiefanCard()
{
    target_fixed = true;
    mute = true;
}

void NosJiefanCard::use(Room *room, ServerPlayer *handang, QList<ServerPlayer *> &) const
{
    ServerPlayer *current = room->getCurrent();
    if (!current || current->isDead() || current->getPhase() == Player::NotActive) return;
    ServerPlayer *who = room->getCurrentDyingPlayer();
    if (!who) return;

    handang->setFlags("NosJiefanUsed");
    room->setTag("NosJiefanTarget", QVariant::fromValue(who));
    bool use_slash = room->askForUseSlashTo(handang, current, "nosjiefan-slash:" + current->objectName(), false);
    if (!use_slash) {
        handang->setFlags("-NosJiefanUsed");
        room->removeTag("NosJiefanTarget");
        room->setPlayerFlag(handang, "Global_NosJiefanFailed");
    }
}

class NosJiefanViewAsSkill : public ZeroCardViewAsSkill
{
public:
    NosJiefanViewAsSkill() : ZeroCardViewAsSkill("nosjiefan")
    {
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (!pattern.contains("peach")) return false;
        if (player->hasFlag("Global_NosJiefanFailed")) return false;
        foreach (const Player *p, player->getAliveSiblings()) {
            if (p->getPhase() != Player::NotActive)
                return true;
        }
        return false;
    }

    virtual const Card *viewAs() const
    {
        return new NosJiefanCard;
    }
};

class NosJiefan : public TriggerSkill
{
public:
    NosJiefan() : TriggerSkill("nosjiefan")
    {
        events << DamageCaused << CardFinished << PreCardUsed;
        view_as_skill = new NosJiefanViewAsSkill;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *handang, QVariant &data) const
    {
        if (triggerEvent == PreCardUsed) {
            if (!handang->hasFlag("NosJiefanUsed"))
                return false;

            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash")) {
                handang->setFlags("-NosJiefanUsed");
                room->setCardFlag(use.card, "nosjiefan-slash");
            }
        } else if (triggerEvent == DamageCaused) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("Slash") && damage.card->hasFlag("nosjiefan-slash")) {
                LogMessage log2;
                log2.type = "#NosJiefanPrevent";
                log2.from = handang;
                log2.to << damage.to;
                room->sendLog(log2);

                ServerPlayer *target = room->getTag("NosJiefanTarget").value<ServerPlayer *>();
                if (target && target->getHp() > 0) {
                    LogMessage log;
                    log.type = "#NosJiefanNull1";
                    log.from = target;
                    room->sendLog(log);
                } else if (target && target->isDead()) {
                    LogMessage log;
                    log.type = "#NosJiefanNull2";
                    log.from = target;
                    log.to << handang;
                    room->sendLog(log);
                } else {
                    Peach *peach = new Peach(Card::NoSuit, 0);
                    peach->setSkillName("_nosjiefan");

                    room->setCardFlag(damage.card, "nosjiefan_success");
                    if ((target != NULL && (target->getGeneralName().contains("sunquan")
                                            || target->getGeneralName().contains("sunce")
                                            || target->getGeneralName().contains("sunjian"))
                            && target->isLord()))
                        handang->setFlags("NosJiefanToLord");
                    room->useCard(CardUseStruct(peach, handang, target));
                    handang->setFlags("-NosJiefanToLord");
                }
                return true;
            }
            return false;
        } else if (triggerEvent == CardFinished && !room->getTag("NosJiefanTarget").isNull()) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") && use.card->hasFlag("nosjiefan-slash")) {
                if (!use.card->hasFlag("nosjiefan_success"))
                    room->setPlayerFlag(handang, "Global_NosJiefanFailed");
                room->removeTag("NosJiefanTarget");
            }
        }

        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *player, const Card *) const
    {
        if (player->hasFlag("NosJiefanToLord"))
            return 2;
        else
            return 1;
    }
};

class NosQianxi : public TriggerSkill
{
public:
    NosQianxi() : TriggerSkill("nosqianxi")
    {
        events << DamageCaused;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();

        if (player->distanceTo(damage.to) == 1 && damage.card && damage.card->isKindOf("Slash")
                && damage.by_user && !damage.chain && !damage.transfer && player->askForSkillInvoke(this, QVariant::fromValue(damage.to))) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.to->objectName());
            player->broadcastSkillInvoke(objectName());
            JudgeStruct judge;
            judge.pattern = ".|heart";
            judge.good = false;
            judge.who = player;
            judge.reason = objectName();

            room->judge(judge);
            if (judge.isGood()) {
                room->loseMaxHp(damage.to);
                return true;
            }
        }
        return false;
    }
};

class NosZhenlie : public TriggerSkill
{
public:
    NosZhenlie() : TriggerSkill("noszhenlie")
    {
        events << AskForRetrial;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        if (judge->who != player)
            return false;

        if (player->askForSkillInvoke(this, data)) {
            int card_id = room->drawCard();
            player->broadcastSkillInvoke(objectName());
            const Card *card = Sanguosha->getCard(card_id);

            room->retrial(card, player, judge, objectName());
        }
        return false;
    }
};

class NosMiji : public PhaseChangeSkill
{
public:
    NosMiji() : PhaseChangeSkill("nosmiji")
    {
        frequency = Frequent;
    }

    virtual bool onPhaseChange(ServerPlayer *wangyi) const
    {
        if (!wangyi->isWounded())
            return false;
        if (wangyi->getPhase() == Player::Start || wangyi->getPhase() == Player::Finish) {
            if (!wangyi->askForSkillInvoke(this))
                return false;
            Room *room = wangyi->getRoom();
            wangyi->broadcastSkillInvoke(objectName());
            JudgeStruct judge;
            judge.pattern = ".|black";
            judge.good = true;
            judge.reason = objectName();
            judge.who = wangyi;

            room->judge(judge);

            if (judge.isGood() && wangyi->isAlive()) {
                QList<int> pile_ids = room->getNCards(wangyi->getLostHp(), false);
                room->fillAG(pile_ids, wangyi);
                ServerPlayer *target = room->askForPlayerChosen(wangyi, room->getAllPlayers(), objectName());
                room->clearAG(wangyi);
                DummyCard *dummy = new DummyCard(pile_ids);
                wangyi->setFlags("Global_GongxinOperator");
                target->obtainCard(dummy, false);
                wangyi->setFlags("-Global_GongxinOperator");
                delete dummy;
            }
        }
        return false;
    }
};

class NosChengxiang : public Chengxiang
{
public:
    NosChengxiang() : Chengxiang()
    {
        setObjectName("noschengxiang");
        total_point = 12;
    }
};

NosRenxinCard::NosRenxinCard()
{
    target_fixed = true;
    mute = true;
}

void NosRenxinCard::use(Room *room, ServerPlayer *player, QList<ServerPlayer *> &) const
{
    if (player->isKongcheng()) return;
    ServerPlayer *who = room->getCurrentDyingPlayer();
    if (!who) return;

    player->broadcastSkillInvoke("renxin");
    DummyCard *handcards = player->wholeHandCards();
    player->turnOver();
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, player->objectName(), who->objectName(), "nosrenxin", QString());
    room->obtainCard(who, handcards, reason, false);
    delete handcards;
    room->recover(who, RecoverStruct(player));
}

class NosRenxin : public ZeroCardViewAsSkill
{
public:
    NosRenxin() : ZeroCardViewAsSkill("nosrenxin")
    {
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern == "peach" && !player->isKongcheng();
    }

    virtual const Card *viewAs() const
    {
        return new NosRenxinCard;
    }
};

class NosDanshou : public TriggerSkill
{
public:
    NosDanshou() : TriggerSkill("nosdanshou")
    {
        events << Damage;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (room->askForSkillInvoke(player, objectName(), data)) {
            player->drawCards(1, objectName());
            ServerPlayer *current = room->getCurrent();
            if (current && current->isAlive() && current->getPhase() != Player::NotActive) {
                player->broadcastSkillInvoke("danshou");
                LogMessage log;
                log.type = "#SkipAllPhase";
                log.from = current;
                room->sendLog(log);
            }
            throw TurnBroken;
        }
        return false;
    }
};

class NosJuece : public TriggerSkill
{
public:
    NosJuece() : TriggerSkill("nosjuece")
    {
        events << CardsMoveOneTime;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *from, QVariant &datas) const
    {
        TriggerList list;
        foreach (QVariant data, datas.toList()) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from && from == move.from && move.from_places.contains(Player::PlaceHand) && move.is_last_handcard && from->getHp() > 0)
                foreach (ServerPlayer *player, room->getAlivePlayers())
                    if (TriggerSkill::triggerable(player) && player->getPhase() != Player::NotActive)
                        list.insert(player, nameList());
        }
        return list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *from, QVariant &, ServerPlayer *player) const
    {
        if (room->askForSkillInvoke(player, objectName(), QVariant::fromValue(from))) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), from->objectName());
            player->broadcastSkillInvoke("juece");
            room->damage(DamageStruct(objectName(), player, from));
        }
        return false;
    }
};

class NosMieji : public TargetModSkill
{
public:
    NosMieji() : TargetModSkill("#nosmieji")
    {
        pattern = "SingleTargetTrick|black"; // deal with Ex Nihilo and Collateral later
    }

    virtual int getExtraTargetNum(const Player *from, const Card *) const
    {
        if (from->hasSkill("nosmieji"))
            return 1;
        else
            return 0;
    }
};

class NosMiejiViewAsSkill : public ZeroCardViewAsSkill
{
public:
    NosMiejiViewAsSkill() : ZeroCardViewAsSkill("nosmieji")
    {
        response_pattern = "@@nosmieji";
    }

    virtual const Card *viewAs() const
    {
        return new ExtraCollateralCard;
    }
};

class NosMiejiForExNihiloAndCollateral : public TriggerSkill
{
public:
    NosMiejiForExNihiloAndCollateral() : TriggerSkill("nosmieji")
    {
        events << PreCardUsed;
        frequency = Compulsory;
        view_as_skill = new NosMiejiViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isBlack() && (use.card->isKindOf("ExNihilo") || use.card->isKindOf("Collateral"))) {
            QList<ServerPlayer *> targets;
            ServerPlayer *extra = NULL;
            if (use.card->isKindOf("ExNihilo")) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (!use.to.contains(p) && !room->isProhibited(player, p, use.card))
                        targets << p;
                }
                if (targets.isEmpty()) return false;
                extra = room->askForPlayerChosen(player, targets, objectName(), "@qiaoshui-add:::" + use.card->objectName(), true);
            } else if (use.card->isKindOf("Collateral")) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (use.to.contains(p) || room->isProhibited(player, p, use.card)) continue;
                    if (use.card->targetFilter(QList<const Player *>(), p, player))
                        targets << p;
                }
                if (targets.isEmpty()) return false;

                QStringList tos;
                foreach(ServerPlayer *t, use.to)
                    tos.append(t->objectName());
                room->setPlayerProperty(player, "extra_collateral", use.card->toString());
                room->setPlayerProperty(player, "extra_collateral_current_targets", tos.join("+"));
                bool used = room->askForUseCard(player, "@@nosmieji", "@qiaoshui-add:::collateral");
                room->setPlayerProperty(player, "extra_collateral", QString());
                room->setPlayerProperty(player, "extra_collateral_current_targets", QString());
                if (!used) return false;
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (p->hasFlag("ExtraCollateralTarget")) {
                        p->setFlags("-ExtraCollateralTarget");
                        extra = p;
                        break;
                    }
                }
            }
            if (!extra) return false;
            use.to.append(extra);
            room->sortByActionOrder(use.to);
            data = QVariant::fromValue(use);

            LogMessage log;
            log.type = "#QiaoshuiAdd";
            log.from = player;
            log.to << extra;
            log.card_str = use.card->toString();
            log.arg = objectName();
            room->sendLog(log);
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), extra->objectName());

            if (use.card->isKindOf("Collateral")) {
                ServerPlayer *victim = extra->tag["collateralVictim"].value<ServerPlayer *>();
                if (victim) {
                    LogMessage log;
                    log.type = "#CollateralSlash";
                    log.from = player;
                    log.to << victim;
                    room->sendLog(log);
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, extra->objectName(), victim->objectName());
                }
            }
        }
        return false;
    }
};

class NosMiejiEffect : public TriggerSkill
{
public:
    NosMiejiEffect() : TriggerSkill("#nosmieji-effect")
    {
        events << PreCardUsed;
    }

    virtual int getPriority(TriggerEvent) const
    {
        return 6;
    }

    virtual bool trigger(TriggerEvent, Room *, ServerPlayer *, QVariant &) const
    {

        return false;
    }
};

class NosFencheng : public ZeroCardViewAsSkill
{
public:
    NosFencheng() : ZeroCardViewAsSkill("nosfencheng")
    {
        frequency = Limited;
        limit_mark = "@nosburn";
    }

    virtual const Card *viewAs() const
    {
        return new NosFenchengCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@nosburn") >= 1;
    }
};

NosFenchengCard::NosFenchengCard()
{
    target_fixed = true;
}

void NosFenchengCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    foreach (ServerPlayer *p, room->getOtherPlayers(use.from))
        use.to << p;
    SkillCard::onUse(room, use);
}

void NosFenchengCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->removePlayerMark(source, "@nosburn");
    //room->doLightbox("$NosFenchengAnimate", 3000);

    QList<ServerPlayer *> players = room->getOtherPlayers(source);
    source->setFlags("NosFenchengUsing");
    try {
        foreach (ServerPlayer *player, players) {
            if (player->isAlive()) {
                room->cardEffect(this, source, player);
                room->getThread()->delay();
            }
        }
        source->setFlags("-NosFenchengUsing");
    } catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken || triggerEvent == StageChange)
            source->setFlags("-NosFenchengUsing");
        throw triggerEvent;
    }
}

void NosFenchengCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();

    int length = qMax(1, effect.to->getEquips().length());
    if (!effect.to->canDiscard(effect.to, "he") || !room->askForDiscard(effect.to, "nosfencheng", length, length, true, true))
        room->damage(DamageStruct("nosfencheng", effect.from, effect.to, 1, DamageStruct::Fire));
}

class NosZhuikong : public TriggerSkill
{
public:
    NosZhuikong() : TriggerSkill("noszhuikong")
    {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() != Player::RoundStart || player->isKongcheng())
            return false;

        bool skip = false;
        foreach (ServerPlayer *fuhuanghou, room->getAllPlayers()) {
            if (TriggerSkill::triggerable(fuhuanghou)
                    && fuhuanghou->isWounded() && fuhuanghou->canPindian(player)
                    && room->askForSkillInvoke(fuhuanghou, objectName())) {
                fuhuanghou->broadcastSkillInvoke("zhuikong");
                if (fuhuanghou->pindian(player, objectName(), NULL)) {
                    if (!skip) {
                        player->skip(Player::Play);
                        skip = true;
                    }
                } else
                    room->setFixedDistance(player, fuhuanghou, 1);
            }
        }
        return false;
    }
};

class NosQiuyuan : public TriggerSkill
{
public:
    NosQiuyuan() : TriggerSkill("nosqiuyuan")
    {
        events << TargetConfirming;
        view_as_skill = new dummyVS;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")) {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (!p->isKongcheng() && p != use.from)
                    targets << p;
            }
            if (targets.isEmpty()) return false;
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "nosqiuyuan-invoke", true, true);
            if (target) {
                player->broadcastSkillInvoke("qiuyuan");
                const Card *card = NULL;
                if (target->getHandcardNum() > 1) {
                    card = room->askForCard(target, ".!", "@nosqiuyuan-give:" + player->objectName(), data, Card::MethodNone);
                    if (!card)
                        card = target->getHandcards().first();
                } else {
                    Q_ASSERT(target->getHandcardNum() == 1);
                    card = target->getHandcards().first();
                }
                CardMoveReason reason(CardMoveReason::S_REASON_GIVE, target->objectName(), player->objectName(), "nosqiuyuan", QString());
                room->obtainCard(player, card, reason, true);
                if (!card->isKindOf("Jink")) {
                    if (use.from->canSlash(target, use.card, false)) {
                        LogMessage log;
                        log.type = "#BecomeTarget";
                        log.from = target;
                        log.card_str = use.card->toString();
                        room->sendLog(log);

                        use.to.append(target);
                        room->sortByActionOrder(use.to);
                        data = QVariant::fromValue(use);
                    }
                }
            }
        }
        return false;
    }
};

// old stantard generals

class NosJianxiong : public MasochismSkill
{
public:
    NosJianxiong() : MasochismSkill("nosjianxiong")
    {
    }

    virtual void onDamaged(ServerPlayer *caocao, const DamageStruct &damage) const
    {
        Room *room = caocao->getRoom();
        const Card *card = damage.card;
        if (!card) return;

        QList<int> ids;
        if (card->isVirtualCard())
            ids = card->getSubcards();
        else
            ids << card->getEffectiveId();

        if (ids.isEmpty()) return;
        foreach (int id, ids) {
            if (room->getCardPlace(id) != Player::PlaceTable) return;
        }
        QVariant data = QVariant::fromValue(damage);
        if (room->askForSkillInvoke(caocao, objectName(), data)) {
            caocao->broadcastSkillInvoke(objectName());
            caocao->obtainCard(card);
        }
    }
};

class NosFankui : public MasochismSkill
{
public:
    NosFankui() : MasochismSkill("nosfankui")
    {
    }

    virtual void onDamaged(ServerPlayer *simayi, const DamageStruct &damage) const
    {
        ServerPlayer *from = damage.from;
        Room *room = simayi->getRoom();
        QVariant data = QVariant::fromValue(from);
        if (from && !from->isNude() && room->askForSkillInvoke(simayi, "nosfankui", data)) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, simayi->objectName(), from->objectName());
            simayi->broadcastSkillInvoke(objectName());
            int card_id = room->askForCardChosen(simayi, from, "he", "nosfankui");
            CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, simayi->objectName());
            room->obtainCard(simayi, Sanguosha->getCard(card_id),
                             reason, room->getCardPlace(card_id) != Player::PlaceHand);
        }
    }
};

class NosGuicai : public TriggerSkill
{
public:
    NosGuicai() : TriggerSkill("nosguicai")
    {
        events << AskForRetrial;
        view_as_skill = new dummyVS;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->isKongcheng())
            return false;
        JudgeStruct *judge = data.value<JudgeStruct *>();
        QStringList prompt_list;
        prompt_list << "@nosguicai-card" << judge->who->objectName()
                    << objectName() << judge->reason << QString::number(judge->card->getEffectiveId());
        QString prompt = prompt_list.join(":");
        const Card *card = room->askForCard(player, ".", prompt, data, Card::MethodResponse, judge->who, true);
        if (card) {
            player->broadcastSkillInvoke(objectName());
            if (judge->who)
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), judge->who->objectName());
            room->retrial(card, player, judge, objectName());
        }
        return false;
    }
};

class NosGanglie : public MasochismSkill
{
public:
    NosGanglie() : MasochismSkill("nosganglie")
    {
    }

    virtual void onDamaged(ServerPlayer *xiahou, const DamageStruct &damage) const
    {
        ServerPlayer *from = damage.from;
        Room *room = xiahou->getRoom();

        if (room->askForSkillInvoke(xiahou, "nosganglie", QVariant::fromValue(from))) {
            xiahou->broadcastSkillInvoke("nosganglie");

            if (from)
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, xiahou->objectName(), from->objectName());
            JudgeStruct judge;
            judge.pattern = ".|heart";
            judge.good = false;
            judge.reason = objectName();
            judge.who = xiahou;

            room->judge(judge);
            if (!from || from->isDead()) return;
            if (judge.isGood()) {
                if (!room->askForDiscard(from, objectName(), 2, 2, true))
                    room->damage(DamageStruct(objectName(), xiahou, from));
            }
        }
    }
};

NosTuxiCard::NosTuxiCard()
{
}

bool NosTuxiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (targets.length() >= 2 || to_select == Self)
        return false;

    return !to_select->isKongcheng();
}

void NosTuxiCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    if (effect.from->isAlive() && !effect.to->isKongcheng()) {
        int card_id = room->askForCardChosen(effect.from, effect.to, "h", "tuxi");
        CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, effect.from->objectName());
        room->obtainCard(effect.from, Sanguosha->getCard(card_id), reason, false);
    }
}

class NosTuxiViewAsSkill : public ZeroCardViewAsSkill
{
public:
    NosTuxiViewAsSkill() : ZeroCardViewAsSkill("nostuxi")
    {
        response_pattern = "@@nostuxi";
    }

    virtual const Card *viewAs() const
    {
        return new NosTuxiCard;
    }
};

class NosTuxi : public PhaseChangeSkill
{
public:
    NosTuxi() : PhaseChangeSkill("nostuxi")
    {
        view_as_skill = new NosTuxiViewAsSkill;
    }

    virtual bool onPhaseChange(ServerPlayer *zhangliao) const
    {
        if (zhangliao->getPhase() == Player::Draw) {
            Room *room = zhangliao->getRoom();
            bool can_invoke = false;
            QList<ServerPlayer *> other_players = room->getOtherPlayers(zhangliao);
            foreach (ServerPlayer *player, other_players) {
                if (!player->isKongcheng()) {
                    can_invoke = true;
                    break;
                }
            }

            if (can_invoke && room->askForUseCard(zhangliao, "@@nostuxi", "@nostuxi-card"))
                return true;
        }

        return false;
    }
};

class NosLuoyiBuff : public TriggerSkill
{
public:
    NosLuoyiBuff() : TriggerSkill("#nosluoyi")
    {
        events << DamageCaused;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->hasFlag("nosluoyi") && target->isAlive();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *xuchu, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.chain || damage.transfer || !damage.by_user) return false;
        const Card *reason = damage.card;
        if (reason && (reason->isKindOf("Slash") || reason->isKindOf("Duel"))) {
            LogMessage log;
            log.type = "#LuoyiBuff";
            log.from = xuchu;
            log.to << damage.to;
            log.arg = QString::number(damage.damage);
            log.arg2 = QString::number(++damage.damage);
            room->sendLog(log);

            data = QVariant::fromValue(damage);
        }

        return false;
    }
};

class NosLuoyi : public DrawCardsSkill
{
public:
    NosLuoyi() : DrawCardsSkill("nosluoyi")
    {
    }

    virtual int getDrawNum(ServerPlayer *xuchu, int n) const
    {
        Room *room = xuchu->getRoom();
        if (room->askForSkillInvoke(xuchu, objectName())) {
            xuchu->broadcastSkillInvoke(objectName());
            xuchu->setFlags(objectName());
            return n - 1;
        } else
            return n;
    }
};

NosYiji::NosYiji() : MasochismSkill("nosyiji")
{
    frequency = Frequent;
    n = 2;
}

void NosYiji::onDamaged(ServerPlayer *guojia, const DamageStruct &damage) const
{
    Room *room = guojia->getRoom();
    int x = damage.damage;
    for (int i = 0; i < x; i++) {
        if (!guojia->isAlive() || !room->askForSkillInvoke(guojia, objectName()))
            return;
        guojia->broadcastSkillInvoke("nosyiji");

        QList<ServerPlayer *> _guojia;
        _guojia.append(guojia);
        QList<int> yiji_cards = room->getNCards(n, false);

        CardsMoveStruct move(yiji_cards, guojia, guojia, Player::PlaceSpecial, Player::PlaceHand,
                             CardMoveReason(CardMoveReason::S_REASON_PREVIEW, guojia->objectName(), objectName(), QString()));
        QList<CardsMoveStruct> moves;
        moves.append(move);
        room->notifyMoveCards(true, moves, false, _guojia);
        room->notifyMoveCards(false, moves, false, _guojia);

        QList<int> origin_yiji = yiji_cards;
        while (room->askForYiji(guojia, yiji_cards, objectName(), true, false, true, -1, room->getAlivePlayers())) {
            CardsMoveStruct move(QList<int>(), guojia, NULL, Player::PlaceHand, Player::PlaceTable,
                                 CardMoveReason(CardMoveReason::S_REASON_PREVIEW, guojia->objectName(), objectName(), QString()));
            foreach (int id, origin_yiji) {
                if (room->getCardPlace(id) != Player::DrawPile) {
                    move.card_ids << id;
                    yiji_cards.removeOne(id);
                }
            }
            origin_yiji = yiji_cards;
            QList<CardsMoveStruct> moves;
            moves.append(move);
            room->notifyMoveCards(true, moves, false, _guojia);
            room->notifyMoveCards(false, moves, false, _guojia);
            if (!guojia->isAlive())
                return;
        }

        if (!yiji_cards.isEmpty()) {
            CardsMoveStruct move(yiji_cards, guojia, NULL, Player::PlaceHand, Player::PlaceTable,
                                 CardMoveReason(CardMoveReason::S_REASON_PREVIEW, guojia->objectName(), objectName(), QString()));
            QList<CardsMoveStruct> moves;
            moves.append(move);
            room->notifyMoveCards(true, moves, false, _guojia);
            room->notifyMoveCards(false, moves, false, _guojia);

            DummyCard *dummy = new DummyCard(yiji_cards);
            guojia->obtainCard(dummy, false);
            delete dummy;
        }
    }
}

NosRendeCard::NosRendeCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

void NosRendeCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();

    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), target->objectName(), "nosrende", QString());
    room->obtainCard(target, this, reason, false);

    int old_value = source->getMark("nosrende");
    int new_value = old_value + subcards.length();
    room->setPlayerMark(source, "nosrende", new_value);

    if (old_value < 2 && new_value >= 2)
        room->recover(source, RecoverStruct(source));
}

class NosRendeViewAsSkill : public ViewAsSkill
{
public:
    NosRendeViewAsSkill() : ViewAsSkill("nosrende")
    {
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return !to_select->isEquipped();
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return true;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return NULL;

        NosRendeCard *rende_card = new NosRendeCard;
        rende_card->addSubcards(cards);
        return rende_card;
    }
};

class NosRende : public TriggerSkill
{
public:
    NosRende() : TriggerSkill("nosrende")
    {
        events << EventPhaseChanging;
        view_as_skill = new NosRendeViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->getMark("nosrende") > 0;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to != Player::NotActive)
            return false;
        room->setPlayerMark(player, "nosrende", 0);
        return false;
    }
};

class NosGuanxing : public PhaseChangeSkill
{
public:
    NosGuanxing() : PhaseChangeSkill("nosguanxing")
    {
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Start;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const
    {
        Room *room = target->getRoom();
        if (room->askForSkillInvoke(target, objectName())) {
            QList<int> guanxing = room->getNCards(getGuanxingNum(room));

            LogMessage log;
            log.type = "$ViewDrawPile";
            log.from = target;
            log.card_str = IntList2StringList(guanxing).join("+");
            room->sendLog(log, target);

            room->askForGuanxing(target, guanxing);
        }
        return false;
    }

    virtual int getGuanxingNum(Room *room) const
    {
        return qMin(5, room->alivePlayerCount());
    }
};

class NosTieji : public TriggerSkill
{
public:
    NosTieji() : TriggerSkill("nostieji")
    {
        events << TargetSpecified;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash"))
            return false;
        QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
        int index = 0;
        foreach (ServerPlayer *p, use.to) {
            if (!player->isAlive()) break;
            if (player->askForSkillInvoke(this, QVariant::fromValue(p))) {
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), p->objectName());
                player->broadcastSkillInvoke(objectName());

                p->setFlags("NosTiejiTarget"); // For AI

                JudgeStruct judge;
                judge.pattern = ".|red";
                judge.good = true;
                judge.reason = objectName();
                judge.who = player;

                try {
                    room->judge(judge);
                } catch (TriggerEvent triggerEvent) {
                    if (triggerEvent == TurnBroken || triggerEvent == StageChange)
                        p->setFlags("-NosTiejiTarget");
                    throw triggerEvent;
                }

                if (judge.isGood()) {
                    LogMessage log;
                    log.type = "#NoJink";
                    log.from = p;
                    room->sendLog(log);
                    jink_list.replace(index, QVariant(0));
                }

                p->setFlags("-NosTiejiTarget");
            }
            index++;
        }
        player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        return false;
    }
};

class NosJizhi : public TriggerSkill
{
public:
    NosJizhi() : TriggerSkill("nosjizhi")
    {
        frequency = Frequent;
        events << CardUsed;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *yueying, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();

        if (use.card->isNDTrick() && room->askForSkillInvoke(yueying, objectName())) {
            yueying->broadcastSkillInvoke(objectName());

            yueying->drawCards(1, objectName());
        }

        return false;
    }
};

class NosQicai : public TargetModSkill
{
public:
    NosQicai() : TargetModSkill("nosqicai")
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

NosKurouCard::NosKurouCard()
{
    target_fixed = true;
}

void NosKurouCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->loseHp(source);
    if (source->isAlive())
        room->drawCards(source, 2, "noskurou");
}

class NosKurou : public ZeroCardViewAsSkill
{
public:
    NosKurou() : ZeroCardViewAsSkill("noskurou")
    {
    }

    virtual const Card *viewAs() const
    {
        return new NosKurouCard;
    }
};

class NosYingzi : public DrawCardsSkill
{
public:
    NosYingzi() : DrawCardsSkill("nosyingzi")
    {
        frequency = Frequent;
    }

    virtual int getDrawNum(ServerPlayer *zhouyu, int n) const
    {
        Room *room = zhouyu->getRoom();
        if (room->askForSkillInvoke(zhouyu, objectName())) {
            zhouyu->broadcastSkillInvoke(objectName());
            return n + 1;
        } else
            return n;
    }
};

NosFanjianCard::NosFanjianCard()
{
}

void NosFanjianCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *zhouyu = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = zhouyu->getRoom();

    int card_id = zhouyu->getRandomHandCardId();
    const Card *card = Sanguosha->getCard(card_id);
    Card::Suit suit = room->askForSuit(target, "nosfanjian");

    LogMessage log;
    log.type = "#ChooseSuit";
    log.from = target;
    log.arg = Card::Suit2String(suit);
    room->sendLog(log);

    room->getThread()->delay();
    target->obtainCard(card);

    if (card->getSuit() != suit)
        room->damage(DamageStruct("nosfanjian", zhouyu, target));
}

class NosFanjian : public ZeroCardViewAsSkill
{
public:
    NosFanjian() : ZeroCardViewAsSkill("nosfanjian")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->isKongcheng() && !player->hasUsed("NosFanjianCard");
    }

    virtual const Card *viewAs() const
    {
        return new NosFanjianCard;
    }
};

class NosGuose : public OneCardViewAsSkill
{
public:
    NosGuose() : OneCardViewAsSkill("nosguose")
    {
        filter_pattern = ".|diamond";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Indulgence *indulgence = new Indulgence(originalCard->getSuit(), originalCard->getNumber());
        indulgence->addSubcard(originalCard->getId());
        indulgence->setSkillName(objectName());
        return indulgence;
    }
};

class NosQianxun : public ProhibitSkill
{
public:
    NosQianxun() : ProhibitSkill("nosqianxun")
    {
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        return to->hasSkill(this) && (card->isKindOf("Snatch") || card->isKindOf("Indulgence"));
    }
};

class NosLianying : public TriggerSkill
{
public:
    NosLianying() : TriggerSkill("noslianying")
    {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *luxun, QVariant &datas, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(luxun)) return QStringList();
        foreach (QVariant data, datas.toList()) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == luxun && move.from_places.contains(Player::PlaceHand) && luxun->isKongcheng())
                return nameList();
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *luxun, QVariant &data, ServerPlayer *) const
    {
        if (room->askForSkillInvoke(luxun, objectName(), data)) {
            luxun->broadcastSkillInvoke(objectName());
            luxun->drawCards(1, objectName());
        }
        return false;
    }
};

NosQingnangCard::NosQingnangCard()
{
}

bool NosQingnangCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    return targets.isEmpty() && to_select->isWounded();
}

void NosQingnangCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.value(0, source);
    room->cardEffect(this, source, target);
}

void NosQingnangCard::onEffect(const CardEffectStruct &effect) const
{
    effect.to->getRoom()->recover(effect.to, RecoverStruct(effect.from));
}

class NosQingnang : public OneCardViewAsSkill
{
public:
    NosQingnang() : OneCardViewAsSkill("nosqingnang")
    {
        filter_pattern = ".|.|.|hand!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("NosQingnangCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        NosQingnangCard *qingnang_card = new NosQingnangCard;
        qingnang_card->addSubcard(originalCard->getId());
        return qingnang_card;
    }
};

NosLijianCard::NosLijianCard() : LijianCard(false)
{
}

class NosLijian : public OneCardViewAsSkill
{
public:
    NosLijian() : OneCardViewAsSkill("noslijian")
    {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getAliveSiblings().length() > 1
               && player->canDiscard(player, "he") && !player->hasUsed("NosLijianCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        NosLijianCard *lijian_card = new NosLijianCard;
        lijian_card->addSubcard(originalCard->getId());
        return lijian_card;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        return card->isKindOf("Duel") ? 0 : -1;
    }
};

class NosLuoshen : public TriggerSkill
{
public:
    NosLuoshen() : TriggerSkill("nosluoshen")
    {
        events << EventPhaseStart << FinishJudge;
        frequency = Frequent;
    }

    int getPriority(TriggerEvent triggerEvent) const
    {
        if (triggerEvent == FinishJudge)
            return 5;
        return TriggerSkill::getPriority(triggerEvent);
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Start && TriggerSkill::triggerable(player))
            return nameList();
        else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == objectName() && judge->isGood() && room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge)
                return QStringList("nosluoshen!");
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *zhenji, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == EventPhaseStart) {
            bool first = true;
            while (zhenji->isAlive() && zhenji->askForSkillInvoke(objectName())) {
                if (first) {
                    LogMessage log;
                    log.type = "#InvokeSkill";
                    log.from = zhenji;
                    log.arg = objectName();
                    room->sendLog(log);
                    room->notifySkillInvoked(zhenji, objectName());
                    zhenji->broadcastSkillInvoke(objectName());
                    first = false;
                }

                JudgeStruct judge;
                judge.pattern = ".|black";
                judge.good = true;
                judge.reason = objectName();
                judge.who = zhenji;
                judge.time_consuming = true;
                room->judge(judge);

                if (judge.isBad()) break;
            }
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            const Card *card = judge->card;
            zhenji->obtainCard(card);
        }
        return false;
    }
};

NosZhihengCard::NosZhihengCard()
{
    target_fixed = true;
}

void NosZhihengCard::extraCost(Room *room, const CardUseStruct &card_use) const
{
    CardMoveReason reason(CardMoveReason::S_REASON_THROW, card_use.from->objectName(), QString(), card_use.card->getSkillName(), QString());
    room->moveCardTo(this, NULL, Player::DiscardPile, reason, true);
}

void NosZhihengCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    if (source->isAlive())
        source->drawCards(subcards.length(), "noszhiheng");
}

class NosZhiheng : public ViewAsSkill
{
public:
    NosZhiheng() : ViewAsSkill("noszhiheng")
    {
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return NULL;

        NosZhihengCard *noszhiheng_card = new NosZhihengCard;
        noszhiheng_card->addSubcards(cards);
        noszhiheng_card->setSkillName(objectName());
        return noszhiheng_card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("NosZhihengCard");
    }
};

class NosJiuyuan : public TriggerSkill
{
public:
    NosJiuyuan() : TriggerSkill("nosjiuyuan$")
    {
        events << TargetConfirmed << PreHpRecover;
        frequency = Compulsory;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *sunquan, QVariant &data) const
    {
        if (triggerEvent == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Peach") && use.from && use.from->getKingdom() == "wu"
                    && sunquan->hasLordSkill("nosjiuyuan") && sunquan != use.from && sunquan->hasFlag("Global_Dying")) {
                room->setCardFlag(use.card, "nosjiuyuan");
            }
        }
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *target, QVariant &data, ServerPlayer *&) const
    {
        if (target && target->hasLordSkill("nosjiuyuan"))
            if (triggerEvent == PreHpRecover) {
                RecoverStruct rec = data.value<RecoverStruct>();
                if (rec.card && rec.card->hasFlag("nosjiuyuan"))
                    return nameList();
            }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *sunquan, QVariant &data, ServerPlayer *) const
    {
        RecoverStruct rec = data.value<RecoverStruct>();
        room->notifySkillInvoked(sunquan, "nosjiuyuan");
        sunquan->broadcastSkillInvoke("nosjiuyuan");

        LogMessage log;
        log.type = "#NosJiuyuanExtraRecover";
        log.from = sunquan;
        log.to << rec.who;
        log.arg = objectName();
        room->sendLog(log);

        rec.recover++;
        data = QVariant::fromValue(rec);

        return false;
    }
};

NosJieyinCard::NosJieyinCard()
{
}

bool NosJieyinCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty())
        return false;

    return to_select->isMale() && to_select->isWounded() && to_select != Self;
}

void NosJieyinCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    RecoverStruct recover(effect.from);
    room->recover(effect.from, recover, true);
    room->recover(effect.to, recover, true);
}

class NosJieyin : public ViewAsSkill
{
public:
    NosJieyin() : ViewAsSkill("nosjieyin")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("NosJieyinCard");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (selected.length() > 1 || Self->isJilei(to_select))
            return false;

        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() != 2)
            return NULL;

        NosJieyinCard *nosjieyin_card = new NosJieyinCard();
        nosjieyin_card->addSubcards(cards);
        return nosjieyin_card;
    }
};

class NosPaoxiao : public TargetModSkill
{
public:
    NosPaoxiao() : TargetModSkill("nospaoxiao")
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

class NosYaowu : public TriggerSkill
{
public:
    NosYaowu() : TriggerSkill("nosyaowu")
    {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player)) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("Slash") && damage.card->isRed()
                    && damage.from && damage.from->isAlive()) {
                return nameList();
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Slash")) {
            if (damage.from && damage.from->isAlive()) {
                room->sendCompulsoryTriggerLog(player, objectName());
                player->broadcastSkillInvoke(objectName());
                if (damage.from->isWounded() && room->askForChoice(damage.from, objectName(), "recover+draw", data) == "recover")
                    room->recover(damage.from, RecoverStruct(damage.to));
                else
                    damage.from->drawCards(1, objectName());
            }
        }
        return false;
    }
};

NostalStandardPackage::NostalStandardPackage()
    : Package("nostal_standard")
{
    General *nos_caocao = new General(this, "nos_caocao$", "wei");
    nos_caocao->addSkill(new NosJianxiong);
    nos_caocao->addSkill("hujia");

    General *nos_simayi = new General(this, "nos_simayi", "wei", 3);
    nos_simayi->addSkill(new NosFankui);
    nos_simayi->addSkill(new NosGuicai);

    General *nos_xiahoudun = new General(this, "nos_xiahoudun", "wei");
    nos_xiahoudun->addSkill(new NosGanglie);

    General *nos_zhangliao = new General(this, "nos_zhangliao", "wei");
    nos_zhangliao->addSkill(new NosTuxi);

    General *nos_xuchu = new General(this, "nos_xuchu", "wei");
    nos_xuchu->addSkill(new NosLuoyi);
    nos_xuchu->addSkill(new NosLuoyiBuff);
    related_skills.insertMulti("nosluoyi", "#nosluoyi");

    General *nos_guojia = new General(this, "nos_guojia", "wei", 3);
    nos_guojia->addSkill("tiandu");
    nos_guojia->addSkill(new NosYiji);

    General *nos_zhenji = new General(this, "nos_zhenji", "wei", 3, false, true);
    nos_zhenji->addSkill("qingguo");
    nos_zhenji->addSkill(new NosLuoshen);

    General *nos_liubei = new General(this, "nos_liubei$", "shu");
    nos_liubei->addSkill(new NosRende);
    nos_liubei->addSkill("jijiang");

    General *nos_guanyu = new General(this, "nos_guanyu", "shu");
    nos_guanyu->addSkill("njiewusheng");

    General *nos_zhangfei = new General(this, "nos_zhangfei", "shu");
    nos_zhangfei->addSkill(new NosPaoxiao);

    General *nos_zhugeliang = new General(this, "nos_zhugeliang", "shu", 3, true, true);
    nos_zhugeliang->addSkill(new NosGuanxing);
    nos_zhugeliang->addSkill("kongcheng");

    General *nos_zhaoyun = new General(this, "nos_zhaoyun", "shu");
    nos_zhaoyun->addSkill("longdan");

    General *nos_machao = new General(this, "nos_machao", "shu");
    nos_machao->addSkill("mashu");
    nos_machao->addSkill(new NosTieji);

    General *nos_huangyueying = new General(this, "nos_huangyueying", "shu", 3, false);
    nos_huangyueying->addSkill(new NosJizhi);
    nos_huangyueying->addSkill(new NosQicai);

    General *nos_sunquan = new General(this, "nos_sunquan$", "wu", 4, true, true);
    nos_sunquan->addSkill(new NosZhiheng);
    nos_sunquan->addSkill(new NosJiuyuan);

    General *nos_ganning = new General(this, "nos_ganning", "wu");
    nos_ganning->addSkill("qixi");

    General *nos_lvmeng = new General(this, "nos_lvmeng", "wu");
    nos_lvmeng->addSkill("keji");

    General *nos_huanggai = new General(this, "nos_huanggai", "wu");
    nos_huanggai->addSkill(new NosKurou);

    General *nos_zhouyu = new General(this, "nos_zhouyu", "wu", 3);
    nos_zhouyu->addSkill(new NosYingzi);
    nos_zhouyu->addSkill(new NosFanjian);

    General *nos_daqiao = new General(this, "nos_daqiao", "wu", 3, false);
    nos_daqiao->addSkill(new NosGuose);
    nos_daqiao->addSkill("liuli");

    General *nos_luxun = new General(this, "nos_luxun", "wu", 3);
    nos_luxun->addSkill(new NosQianxun);
    nos_luxun->addSkill(new NosLianying);

    General *nos_sunshangxiang = new General(this, "nos_sunshangxiang", "wu", 3, false, true);
    nos_sunshangxiang->addSkill(new NosJieyin);
    nos_sunshangxiang->addSkill("xiaoji");

    General *nos_huatuo = new General(this, "nos_huatuo", "qun", 3);
    nos_huatuo->addSkill(new NosQingnang);
    nos_huatuo->addSkill("jijiu");

    General *nos_lvbu = new General(this, "nos_lvbu", "qun");
    nos_lvbu->addSkill("wushuang");

    General *nos_diaochan = new General(this, "nos_diaochan", "qun", 3, false);
    nos_diaochan->addSkill(new NosLijian);
    nos_diaochan->addSkill("nos2013biyue");

    General *nos_huaxiong = new General(this, "nos_huaxiong", "qun", 6, true, true);
    nos_huaxiong->addSkill(new NosYaowu);

    addMetaObject<NosTuxiCard>();
    addMetaObject<NosRendeCard>();
    addMetaObject<NosKurouCard>();
    addMetaObject<NosFanjianCard>();
    addMetaObject<NosLijianCard>();
    addMetaObject<NosQingnangCard>();
    addMetaObject<NosJieyinCard>();
    addMetaObject<NosZhihengCard>();
}

NostalYJCMPackage::NostalYJCMPackage()
    : Package("nostal_yjcm")
{
    General *nos_fazheng = new General(this, "nos_fazheng", "shu", 3);
    nos_fazheng->addSkill(new NosEnyuan);
    nos_fazheng->addSkill(new NosXuanhuo);

    General *nos_lingtong = new General(this, "nos_lingtong", "wu");
    nos_lingtong->addSkill(new NosXuanfeng);
    nos_lingtong->addSkill(new SlashNoDistanceLimitSkill("nosxuanfeng"));
    related_skills.insertMulti("nosxuanfeng", "#nosxuanfeng-slash-ndl");

    General *nos_masu = new General(this, "nos_masu", "shu", 3);
    nos_masu->addSkill(new Xinzhan);
    nos_masu->addSkill(new Huilei);

    General *nos_xusheng = new General(this, "nos_xusheng", "wu");
    nos_xusheng->addSkill(new NosPojun);

    General *nos_xushu = new General(this, "nos_xushu", "shu", 3);
    nos_xushu->addSkill(new NosWuyan);
    nos_xushu->addSkill(new NosJujian);

    General *nos_yujin = new General(this, "nos_yujin", "wei"); // YJ 010
    nos_yujin->addSkill(new Yizhong);

    addMetaObject<NosXuanhuoCard>();
    addMetaObject<XinzhanCard>();
    addMetaObject<NosJujianCard>();
}

NostalYJCM2012Package::NostalYJCM2012Package()
    : Package("nostal_yjcm2012")
{
    General *nos_guanxingzhangbao = new General(this, "nos_guanxingzhangbao", "shu");
    nos_guanxingzhangbao->addSkill(new NosFuhun);

    General *nos_handang = new General(this, "nos_handang", "wu");
    nos_handang->addSkill(new NosGongqi);
    nos_handang->addSkill(new NosGongqiTargetMod);
    nos_handang->addSkill(new NosJiefan);
    related_skills.insertMulti("nosgongqi", "#nosgongqi-target");

    General *nos_madai = new General(this, "nos_madai", "shu");
    nos_madai->addSkill("mashu");
    nos_madai->addSkill(new NosQianxi);

    General *nos_wangyi = new General(this, "nos_wangyi", "wei", 3, false);
    nos_wangyi->addSkill(new NosZhenlie);
    nos_wangyi->addSkill(new NosMiji);

    addMetaObject<NosJiefanCard>();
}

NostalYJCM2013Package::NostalYJCM2013Package()
    : Package("nostal_yjcm2013")
{
    General *nos_caochong = new General(this, "nos_caochong", "wei", 3);
    nos_caochong->addSkill(new NosChengxiang);
    nos_caochong->addSkill(new NosRenxin);

    General *nos_fuhuanghou = new General(this, "nos_fuhuanghou", "qun", 3, false);
    nos_fuhuanghou->addSkill(new NosZhuikong);
    nos_fuhuanghou->addSkill(new NosQiuyuan);

    General *nos_liru = new General(this, "nos_liru", "qun", 3);
    nos_liru->addSkill(new NosJuece);
    nos_liru->addSkill(new NosMieji);
    nos_liru->addSkill(new NosMiejiForExNihiloAndCollateral);
    nos_liru->addSkill(new NosMiejiEffect);
    nos_liru->addSkill(new NosFencheng);
    related_skills.insertMulti("nosmieji", "#nosmieji");
    related_skills.insertMulti("nosmieji", "#nosmieji-effect");

    General *nos_zhuran = new General(this, "nos_zhuran", "wu");
    nos_zhuran->addSkill(new NosDanshou);

    addMetaObject<NosRenxinCard>();
    addMetaObject<NosFenchengCard>();
}

ADD_PACKAGE(NostalStandard)
ADD_PACKAGE(NostalYJCM)
ADD_PACKAGE(NostalYJCM2012)
ADD_PACKAGE(NostalYJCM2013)
