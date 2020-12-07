#include "settings.h"
#include "standard.h"
#include "skill.h"
#include "physics.h"
#include "client.h"
#include "engine.h"
#include "ai.h"
#include "general.h"
#include "clientplayer.h"
#include "clientstruct.h"
#include "wrapped-card.h"
#include "room.h"
#include "roomthread.h"
#include "maneuvering.h"
#include "json.h"
#include "yjcm2015.h"

class dummyVS : public ZeroCardViewAsSkill
{
public:
    dummyVS() : ZeroCardViewAsSkill("dummy")
    {
    }

    virtual const Card *viewAs() const
    {
        return NULL;
    }
};

class Leirji : public TriggerSkill
{
public:
    Leirji() : TriggerSkill("leirji")
    {
        events << CardResponded << DamageCaused;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *zhangjiaoms, QVariant &data) const
    {
        if (triggerEvent == CardResponded && TriggerSkill::triggerable(zhangjiaoms)) {
            const Card *card_star = data.value<CardResponseStruct>().m_card;
            if (card_star->isKindOf("Jink")) {
                ServerPlayer *target = room->askForPlayerChosen(zhangjiaoms, room->getAlivePlayers(), objectName(), "Leiji-invoke", true, true);
                if (target) {
                    room->broadcastSkillInvoke(objectName());

                    JudgeStruct judge;
                    judge.pattern = ".|black";
                    judge.good = false;
                    judge.negative = true;
                    judge.reason = objectName();
                    judge.who = target;

                    room->judge(judge);

                    if (judge.isBad())
                        room->damage(DamageStruct(objectName(), zhangjiaoms, target, 1, DamageStruct::Thunder));
                }
            }
        } else if (triggerEvent == DamageCaused && zhangjiaoms->isAlive() && zhangjiaoms->isWounded()) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.reason == objectName() && !damage.chain)
                room->recover(zhangjiaoms, RecoverStruct(zhangjiaoms));
        }
        return false;
    }
};

Jiuhshoou::Jiuhshoou() : PhaseChangeSkill("jiuhshoou")
{
}

int Jiuhshoou::getJiuhshoouDrawNum(ServerPlayer *) const
{
    return 1;
}

bool Jiuhshoou::onPhaseChange(ServerPlayer *target) const
{
    if (target->getPhase() == Player::Finish) {
        Room *room = target->getRoom();
        if (room->askForSkillInvoke(target, objectName())) {
            room->broadcastSkillInvoke(objectName());
            target->drawCards(getJiuhshoouDrawNum(target), objectName());
            target->turnOver();
        }
    }

    return false;
}

class Jieewei : public TriggerSkill
{
public:
    Jieewei() : TriggerSkill("jieewei")
    {
        events << TurnedOver;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (!room->askForSkillInvoke(player, objectName())) return false;
        room->broadcastSkillInvoke(objectName());
        player->drawCards(1, objectName());

        const Card *card = room->askForUseCard(player, "TrickCard+^Nullification,EquipCard|.|.|hand", "@Jieewei");
        if (!card) return false;

        QList<ServerPlayer *> targets;
        if (card->getTypeId() == Card::TypeTrick) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                bool can_discard = false;
                foreach (const Card *judge, p->getJudgingArea()) {
                    if (judge->getTypeId() == Card::TypeTrick && player->canDiscard(p, judge->getEffectiveId())) {
                        can_discard = true;
                        break;
                    } else if (judge->getTypeId() == Card::TypeSkill) {
                        const Card *real_card = Sanguosha->getEngineCard(judge->getEffectiveId());
                        if (real_card->getTypeId() == Card::TypeTrick && player->canDiscard(p, real_card->getEffectiveId())) {
                            can_discard = true;
                            break;
                        }
                    }
                }
                if (can_discard) targets << p;
            }
        } else if (card->getTypeId() == Card::TypeEquip) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (!p->getEquips().isEmpty() && player->canDiscard(p, "e"))
                    targets << p;
                else {
                    foreach (const Card *judge, p->getJudgingArea()) {
                        if (judge->getTypeId() == Card::TypeSkill) {
                            const Card *real_card = Sanguosha->getEngineCard(judge->getEffectiveId());
                            if (real_card->getTypeId() == Card::TypeEquip && player->canDiscard(p, real_card->getEffectiveId())) {
                                targets << p;
                                break;
                            }
                        }
                    }
                }
            }
        }
        if (targets.isEmpty()) return false;
        ServerPlayer *to_discard = room->askForPlayerChosen(player, targets, objectName(), "@Jieewei-discard", true);
        if (to_discard) {
            QList<int> disabled_ids;
            foreach (const Card *c, to_discard->getCards("ej")) {
                const Card *pcard = c;
                if (pcard->getTypeId() == Card::TypeSkill)
                    pcard = Sanguosha->getEngineCard(c->getEffectiveId());
                if (pcard->getTypeId() != card->getTypeId())
                    disabled_ids << pcard->getEffectiveId();
            }
            int id = room->askForCardChosen(player, to_discard, "ej", objectName(), false, Card::MethodDiscard, disabled_ids);
            room->throwCard(id, to_discard, player);
        }
        return false;
    }
};

TianshiangCard::TianshiangCard()
{
}

void TianshiangCard::onEffect(const CardEffectStruct &effect) const
{
    DamageStruct damage = effect.from->tag.value("TianshiangDamage").value<DamageStruct>();
    damage.to = effect.to;
    damage.transfer = true;
    damage.flags.append("tianshiang");
    effect.from->tag["TransferDamage"] = QVariant::fromValue(damage);
}

class TianshiangViewAsSkill : public OneCardViewAsSkill
{
public:
    TianshiangViewAsSkill() : OneCardViewAsSkill("tianshiang")
    {
        filter_pattern = ".|heart|.|hand!";
        response_pattern = "@@tianshiang";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        TianshiangCard *tianshiangCard = new TianshiangCard;
        tianshiangCard->addSubcard(originalCard);
        return tianshiangCard;
    }
};

class Tianshiang : public TriggerSkill
{
public:
    Tianshiang() : TriggerSkill("tianshiang")
    {
        events << DamageInflicted;
        view_as_skill = new TianshiangViewAsSkill;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *xiaoqiao, QVariant &data) const
    {
        if (xiaoqiao->canDiscard(xiaoqiao, "h")) {
            xiaoqiao->tag["TianshiangDamage"] = data;
            return room->askForUseCard(xiaoqiao, "@@tianshiang", "@tianshiang-card", -1, Card::MethodDiscard);
        }
        return false;
    }
};

class TianshiangDraw : public TriggerSkill
{
public:
    TianshiangDraw() : TriggerSkill("#tianshiang")
    {
        events << DamageComplete;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (player->isAlive() && damage.transfer && damage.flags.contains("tianshiang"))
            player->drawCards(player->getLostHp(), objectName());
        return false;
    }
};

class Liehgong : public TriggerSkill
{
public:
    Liehgong() : TriggerSkill("liehgong")
    {
        events << TargetSpecified;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (player != use.from || player->getPhase() != Player::Play || !use.card->isKindOf("Slash"))
            return false;
        QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
        int index = 0;
        foreach (ServerPlayer *p, use.to) {
            int handcardnum = p->getHandcardNum();
            if ((player->getHp() <= handcardnum || player->getAttackRange() >= handcardnum)
                && player->askForSkillInvoke(this, QVariant::fromValue(p))) {
                room->broadcastSkillInvoke(objectName());

                LogMessage log;
                log.type = "#NoJink";
                log.from = p;
                room->sendLog(log);
                jink_list.replace(index, QVariant(0));
            }
            index++;
        }
        player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        return false;
    }
};

class Kwangguu : public TriggerSkill
{
public:
    Kwangguu() : TriggerSkill("kwangguu")
    {
        frequency = Compulsory;
        events << Damage;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        bool invoke = player->tag.value("InvokeKwangguu", false).toBool();
        player->tag["InvokeKwangguu"] = false;
        if (invoke && player->isWounded()) {
            room->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(player, objectName());

            room->recover(player, RecoverStruct(player, NULL, damage.damage));
        }
        return false;
    }
};

class KwangguuRecord : public TriggerSkill
{
public:
    KwangguuRecord() : TriggerSkill("#kwangguu-record")
    {
        events << PreDamageDone;
        global = true;
    }

    bool trigger(TriggerEvent triggerEvent, Room *, ServerPlayer *, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (triggerEvent == PreDamageDone) {
            ServerPlayer *weiyan = damage.from;
            if (weiyan)
                weiyan->tag["InvokeKwangguu"] = (weiyan->distanceTo(damage.to) <= 1);
        }
        return false;
    }
};

ShernsuhCard::ShernsuhCard()
{
    mute = true;
}

bool ShernsuhCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("shernsuh");
    slash->deleteLater();
    return slash->targetFilter(targets, to_select, Self);
}

void ShernsuhCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    foreach (ServerPlayer *target, targets) {
        if (!source->canSlash(target, NULL, false))
            targets.removeOne(target);
    }

    if (targets.length() > 0) {
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_shernsuh");
        room->useCard(CardUseStruct(slash, source, targets));
    }
}

class ShernsuhViewAsSkill : public ViewAsSkill
{
public:
    ShernsuhViewAsSkill() : ViewAsSkill("shernsuh")
    {
    }

    bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern.startsWith("@@shernsuh");
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUsePattern().endsWith("1"))
            return false;
        else
            return selected.isEmpty() && to_select->isKindOf("EquipCard") && !Self->isJilei(to_select);
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUsePattern().endsWith("1")) {
            return cards.isEmpty() ? new ShernsuhCard : NULL;
        } else {
            if (cards.length() != 1)
                return NULL;

            ShernsuhCard *card = new ShernsuhCard;
            card->addSubcards(cards);

            return card;
        }
    }
};

class Shernsuh : public TriggerSkill
{
public:
    Shernsuh() : TriggerSkill("shernsuh")
    {
        events << EventPhaseChanging;
        view_as_skill = new ShernsuhViewAsSkill;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *xiahouyuan, QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Judge && !xiahouyuan->isSkipped(Player::Judge)
            && !xiahouyuan->isSkipped(Player::Draw)) {
            if (Slash::IsAvailable(xiahouyuan) && room->askForUseCard(xiahouyuan, "@@shernsuh1", "@shernsuh1", 1)) {
                xiahouyuan->skip(Player::Judge, true);
                xiahouyuan->skip(Player::Draw, true);
            }
        } else if (Slash::IsAvailable(xiahouyuan) && change.to == Player::Play && !xiahouyuan->isSkipped(Player::Play)) {
            if (xiahouyuan->canDiscard(xiahouyuan, "he") && room->askForUseCard(xiahouyuan, "@@shernsuh2", "@shernsuh2", 2, Card::MethodDiscard))
                xiahouyuan->skip(Player::Play, true);
        }
        return false;
    }
};

class Conqueror : public TriggerSkill
{
public:
    Conqueror() : TriggerSkill("conqueror")
    {
        events << TargetSpecified;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card != NULL && use.card->isKindOf("Slash")) {
            int n = 0;
            room->broadcastSkillInvoke(objectName());
            foreach (ServerPlayer *target, use.to) {
                if (player->askForSkillInvoke(this, QVariant::fromValue(target))) {
                    QString choice = room->askForChoice(player, objectName(), "BasicCard+EquipCard+TrickCard", QVariant::fromValue(target));


                    const Card *c = room->askForCard(target, choice, QString("@conqueror-exchange:%1::%2").arg(player->objectName()).arg(choice), choice, Card::MethodNone);
                    if (c != NULL) {
                        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, target->objectName(), player->objectName(), objectName(), QString());
                        room->obtainCard(player, c, reason);
                        use.nullified_list << target->objectName();
                        data = QVariant::fromValue(use);
                    } else {
                        QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
                        jink_list[n] = 0;
                        player->tag["Jink_" + use.card->toString()] = jink_list;
                        LogMessage log;
                        log.type = "#NoJink";
                        log.from = target;
                        room->sendLog(log);
                    }
                }
                ++n;
            }
        }
        return false;
    }
};

class Duannlyang : public OneCardViewAsSkill
{
public:
    Duannlyang() : OneCardViewAsSkill("duannlyang")
    {
        filter_pattern = "BasicCard,EquipCard|black";
        response_or_use = true;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        SupplyShortage *shortage = new SupplyShortage(originalCard->getSuit(), originalCard->getNumber());
        shortage->setSkillName(objectName());
        shortage->addSubcard(originalCard);

        return shortage;
    }
};

class DuannlyangTargetMod : public TargetModSkill
{
public:
    DuannlyangTargetMod() : TargetModSkill("#duannlyang-target")
    {
        frequency = NotFrequent;
        pattern = "SupplyShortage";
    }

    virtual int getDistanceLimit(const Player *from, const Card *, const Player *to) const
    {
        if (from->hasSkill("duannlyang") && to && from->distanceTo(to) <= 2)
            return 1000;
        else
            return 0;
    }
};

class Gueishin : public MasochismSkill
{
public:
    Gueishin() : MasochismSkill("gueishin")
    {
    }

    void onDamaged(ServerPlayer *shencc, const DamageStruct &damage) const
    {
        Room *room = shencc->getRoom();
        int n = shencc->getMark("GueishinTimes"); // mark for AI
        shencc->setMark("GueishinTimes", 0);
        QVariant data = QVariant::fromValue(damage);
        QList<ServerPlayer *> players = room->getOtherPlayers(shencc);
        try {
            for (int i = 0; i < damage.damage; i++) {
                shencc->addMark("GueishinTimes");
                if (shencc->askForSkillInvoke(this, data)) {
                    room->broadcastSkillInvoke(objectName());

                    shencc->setFlags("GueishinUsing");
                    /*
                    if (players.length() >= 4 && (shencc->getGeneralName() == "shencaocao" || shencc->getGeneral2Name() == "shencaocao"))
                    room->doLightbox("$GueishinAnimate");
                    */
                    if (shencc->getGeneralName() != "shencaocao" && (shencc->getGeneralName() == "pr_shencaocao" || shencc->getGeneral2Name() == "pr_shencaocao"))
                        room->doSuperLightbox("pr_shencaocao", "gueishin");  // todo:pr_shencaocao's avatar
                    else
                        room->doSuperLightbox("shencaocao", "gueishin");

                    foreach (ServerPlayer *player, players) {
                        if (player->isAlive() && !player->isAllNude()) {
                            CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, shencc->objectName());
                            int card_id = room->askForCardChosen(shencc, player, "hej", objectName());
                            room->obtainCard(shencc, Sanguosha->getCard(card_id), reason, room->getCardPlace(card_id) != Player::PlaceHand);

                            if (shencc->isDead()) {
                                shencc->setFlags("-GueishinUsing");
                                shencc->setMark("GueishinTimes", 0);
                                return;
                            }

                        }
                    }

                    shencc->turnOver();
                    shencc->setFlags("-GueishinUsing");
                } else
                    break;
            }
            shencc->setMark("GueishinTimes", n);
        }
        catch (TriggerEvent triggerEvent) {
            if (triggerEvent == TurnBroken || triggerEvent == StageChange) {
                shencc->setFlags("-GueishinUsing");
                shencc->setMark("GueishinTimes", n);
            }
            throw triggerEvent;
        }
    }
};

class Mengjin : public TriggerSkill
{
public:
    Mengjin() :TriggerSkill("mengjin")
    {
        events << SlashMissed;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *pangde, QVariant &data) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (effect.to->isAlive() && pangde->canDiscard(effect.to, "he")) {
            if (pangde->askForSkillInvoke(this, data)) {
                room->broadcastSkillInvoke(objectName());
                int to_throw = room->askForCardChosen(pangde, effect.to, "he", objectName(), false, Card::MethodDiscard);
                room->throwCard(Sanguosha->getCard(to_throw), effect.to, pangde);
            }
        }

        return false;
    }
};

RernderCard::RernderCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    mute = true;
}

void RernderCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{

    if (!source->tag.value("rernder_using", false).toBool())
        room->broadcastSkillInvoke("rernder");

    ServerPlayer *target = targets.first();

    int old_value = source->getMark("rernder");
    QList<int> rernder_list;
    if (old_value > 0)
        rernder_list = StringList2IntList(source->property("rernder").toString().split("+"));
    else
        rernder_list = source->handCards();
    foreach(int id, this->subcards)
        rernder_list.removeOne(id);
    room->setPlayerProperty(source, "rernder", IntList2StringList(rernder_list).join("+"));

    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), target->objectName(), "rernder", QString());
    room->obtainCard(target, this, reason, false);

    int new_value = old_value + subcards.length();
    room->setPlayerMark(source, "rernder", new_value);

    if (old_value < 2 && new_value >= 2)
        room->recover(source, RecoverStruct(source));

    if (room->getMode() == "04_1v3" && source->getMark("rernder") >= 2) return;
    if (source->isKongcheng() || source->isDead() || rernder_list.isEmpty()) return;
    room->addPlayerHistory(source, "RernderCard", -1);

    source->tag["rernder_using"] = true;

    if (!room->askForUseCard(source, "@@rernder", "@rernder-give", -1, Card::MethodNone))
        room->addPlayerHistory(source, "RernderCard");

    source->tag["rernder_using"] = false;
}

class RernderViewAsSkill : public ViewAsSkill
{
public:
    RernderViewAsSkill() : ViewAsSkill("rernder")
    {
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (ServerInfo.GameMode == "04_1v3" && selected.length() + Self->getMark("rernder") >= 2)
            return false;
        else {
            if (to_select->isEquipped()) return false;
            if (Sanguosha->currentRoomState()->getCurrentCardUsePattern() == "@@rernder") {
                QList<int> rernder_list = StringList2IntList(Self->property("rernder").toString().split("+"));
                return rernder_list.contains(to_select->getEffectiveId());
            } else
                return true;
        }
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        if (ServerInfo.GameMode == "04_1v3" && player->getMark("rernder") >= 2)
            return false;
        return !player->hasUsed("RernderCard") && !player->isKongcheng();
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "@@rernder";
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return NULL;

        RernderCard *rernder_card = new RernderCard;
        rernder_card->addSubcards(cards);
        return rernder_card;
    }
};

class Rernder : public TriggerSkill
{
public:
    Rernder() : TriggerSkill("rernder")
    {
        events << EventPhaseChanging;
        view_as_skill = new RernderViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->getMark("rernder") > 0;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to != Player::NotActive)
            return false;
        room->setPlayerMark(player, "rernder", 0);
        room->setPlayerProperty(player, "rernder", QString());
        return false;
    }
};

class Shyunshyun : public PhaseChangeSkill
{
public:
    Shyunshyun() : PhaseChangeSkill("shyunshyun")
    {
        frequency = Frequent;
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
                QList<int> obtained;
                room->fillAG(card_ids, lidian);
                int id1 = room->askForAG(lidian, card_ids, false, objectName());
                card_ids.removeOne(id1);
                obtained << id1;
                room->takeAG(lidian, id1, false, p_list);
                int id2 = room->askForAG(lidian, card_ids, false, objectName());
                card_ids.removeOne(id2);
                obtained << id2;
                room->clearAG(lidian);

                room->askForGuanxing(lidian, card_ids, Room::GuanxingDownOnly);
                DummyCard *dummy = new DummyCard(obtained);
                lidian->obtainCard(dummy, false);
                delete dummy;

                return true;
            }
        }

        return false;
    }
};

class Chingjean : public TriggerSkill
{
public:
    Chingjean() : TriggerSkill("chingjean")
    {
        events << CardsMoveOneTime;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (!room->getTag("FirstRound").toBool() && player->getPhase() != Player::Draw && move.to == player && move.to_place == Player::PlaceHand) {
            QList<int> ids;
            foreach (int id, move.card_ids) {
                if (room->getCardOwner(id) == player && room->getCardPlace(id) == Player::PlaceHand)
                    ids << id;
            }
            if (ids.isEmpty())
                return false;
            player->tag["ChingjeanCurrentMoveSkill"] = QVariant(move.reason.m_skillName);
            while (room->askForYiji(player, ids, objectName(), false, false, true, -1, QList<ServerPlayer *>(), CardMoveReason(), "@qingjian-distribute", true)) {
                if (player->isDead()) return false;
            }
        }
        return false;
    }
};

LesbianLijianCard::LesbianLijianCard(bool cancelable) : duel_cancelable(cancelable)
{
    will_sort = false;
}

bool LesbianLijianCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Duel *duel = new Duel(Card::NoSuit, 0);
    duel->deleteLater();
    if (targets.isEmpty() && Sanguosha->isProhibited(NULL, to_select, duel))
        return false;

    if (targets.length() == 1 && to_select->isCardLimited(duel, Card::MethodUse))
        return false;

    return targets.length() < 2 && to_select != Self;
}

class LesbianLijian : public OneCardViewAsSkill
{
public:
    LesbianLijian() : OneCardViewAsSkill("lesbianlijian")
    {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("LesbianLijianCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        LesbianLijianCard *lesbianlijian_card = new LesbianLijianCard;
        lesbianlijian_card->addSubcard(originalCard->getId());
        return lesbianlijian_card;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        return card->isKindOf("Duel") ? 0 : -1;
    }
};

LesbianJieyinCard::LesbianJieyinCard()
{
}

bool LesbianJieyinCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty())
        return false;

    return to_select->isWounded() && to_select != Self;
}

class LesbianJieyin : public ViewAsSkill
{
public:
    LesbianJieyin() : ViewAsSkill("lesbianjieyin")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("LesbianJieyinCard");
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

        LesbianJieyinCard *lesbianjieyin_card = new LesbianJieyinCard();
        lesbianjieyin_card->addSubcards(cards);
        return lesbianjieyin_card;
    }
};

class LesbianJiaojin : public TriggerSkill
{
public:
    LesbianJiaojin() : TriggerSkill("lesbianjiaojin")
    {
        events << DamageInflicted;
        view_as_skill = new dummyVS;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && player->canDiscard(player, "he")) {
            if (room->askForCard(player, ".Equip", "@lesbianjiaojin", data, objectName())) {

                LogMessage log;
                log.type = "#LesbianJiaojin";
                log.from = player;
                log.arg = QString::number(damage.damage);
                log.arg2 = QString::number(--damage.damage);
                room->sendLog(log);

                if (damage.damage < 1)
                    return true;
                data = QVariant::fromValue(damage);
            }
        }
        return false;
    }
};

class LesbianYanyuVS : public OneCardViewAsSkill
{
public:
    LesbianYanyuVS() : OneCardViewAsSkill("lesbianyanyu")
    {
        filter_pattern = "Slash";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        if (Self->isCardLimited(originalCard, Card::MethodRecast))
            return NULL;

        YanyuCard *recast = new YanyuCard;
        recast->addSubcard(originalCard);
        return recast;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        Slash *s = new Slash(Card::NoSuit, 0);
        s->deleteLater();
        return !player->isCardLimited(s, Card::MethodRecast);
    }
};

class LesbianYanyu : public TriggerSkill
{
public:
    LesbianYanyu() : TriggerSkill("lesbianyanyu")
    {
        view_as_skill = new LesbianYanyuVS;
        events << EventPhaseEnd;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() != Player::Play) return false;
        int recastNum = player->getMark("yanyu");
        player->setMark("yanyu", 0);

        if (recastNum < 2)
            return false;

        QList<ServerPlayer *> femalelist;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (p && p->objectName() != player->objectName())
                femalelist << p;
        }

        if (femalelist.isEmpty())
            return false;

        ServerPlayer *female = room->askForPlayerChosen(player, femalelist, objectName(), "@lesbianyanyu-give", true, true);

        if (female != NULL){
            player->broadcastSkillInvoke(objectName());
            female->drawCards(2, objectName());
        }

        return false;
    }
};

class LesbianFuzhu : public TriggerSkill
{
public:
    LesbianFuzhu() : TriggerSkill("lesbianfuzhu")
    {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() != Player::Finish) return false;
        foreach (ServerPlayer *xushi, room->getOtherPlayers(player)) {
            if (!TriggerSkill::triggerable(xushi) || room->getDrawPile().length() > xushi->getHp()*10) continue;
            int times = room->getTag("SwapPile").toInt();
            int max_times = Sanguosha->getPlayerCount(room->getMode());
            bool swap_pile = false;
            for (int i = 0; i < max_times; i++) {
                if (xushi->isDead() || player->isDead() || room->getDrawPile().isEmpty() || room->getTag("SwapPile").toInt() > times) break;
                const Card *slash = NULL;
                for (int i = room->getDrawPile().length()-1; i >= 0; i--) {
                    const Card *card = Sanguosha->getCard(room->getDrawPile().at(i));
                    if (card->isKindOf("Slash") && xushi->canSlash(player, card, false)) {
                        slash = card;
                        break;
                    }
                }
                if (slash) {
                    if (i == 0) {
                        if (!room->askForSkillInvoke(xushi, objectName(), QVariant::fromValue(player))) break;
                        xushi->broadcastSkillInvoke(objectName());
                        swap_pile = true;
                    }
                    room->useCard(CardUseStruct(slash, xushi, player));
                } else
                    break;
            }
            if (swap_pile)
                room->swapPile();
        }
        return false;
    }
};

LesbianLihunCard::LesbianLihunCard()
{

}

bool LesbianLihunCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self;
}

class LesbianLihunSelect : public OneCardViewAsSkill
{
public:
    LesbianLihunSelect() : OneCardViewAsSkill("lesbianlihun")
    {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("LesbianLihunCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        LesbianLihunCard *card = new LesbianLihunCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class LesbianLihun : public TriggerSkill
{
public:
    LesbianLihun() : TriggerSkill("lesbianlihun")
    {
        events << EventPhaseStart << EventPhaseEnd;
        view_as_skill = new LesbianLihunSelect;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->hasUsed("LesbianLihunCard");
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *diaochan, QVariant &) const
    {
        if (triggerEvent == EventPhaseEnd && diaochan->getPhase() == Player::Play) {
            ServerPlayer *target = NULL;
            foreach (ServerPlayer *other, room->getOtherPlayers(diaochan)) {
                if (other->hasFlag("LesbianLihunTarget")) {
                    other->setFlags("-LesbianLihunTarget");
                    target = other;
                    break;
                }
            }

            if (!target || target->getHp() < 1 || diaochan->isNude())
                return false;

            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, diaochan->objectName(), target->objectName());
            diaochan->broadcastSkillInvoke(objectName(), 2);
            DummyCard *to_goback;
            if (diaochan->getCardCount() <= target->getHp()) {
                to_goback = diaochan->isKongcheng() ? new DummyCard : diaochan->wholeHandCards();
                for (int i = 0; i < 4; i++)
                    if (diaochan->getEquip(i))
                        to_goback->addSubcard(diaochan->getEquip(i)->getEffectiveId());
            } else
                to_goback = (DummyCard *)room->askForExchange(diaochan, objectName(), target->getHp(), target->getHp(), true, "LesbianLihunGoBack");

            CardMoveReason reason(CardMoveReason::S_REASON_GIVE, diaochan->objectName(),
                target->objectName(), objectName(), QString());
            room->moveCardTo(to_goback, diaochan, target, Player::PlaceHand, reason);
            to_goback->deleteLater();
        } else if (triggerEvent == EventPhaseStart && diaochan->getPhase() == Player::NotActive) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasFlag("LesbianLihunTarget"))
                    p->setFlags("-LesbianLihunTarget");
            }
        }

        return false;
    }

    int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        if (card->isKindOf("LesbianLihunCard"))
            return 1;
        return -1;
    }
};

class LesbianXingwu : public TriggerSkill
{
public:
    LesbianXingwu() : TriggerSkill("lesbianxingwu")
    {
        events << PreCardUsed << CardResponded << EventPhaseStart << CardsMoveOneTime;
        view_as_skill = new dummyVS;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == PreCardUsed || triggerEvent == CardResponded) {
            const Card *card = NULL;
            if (triggerEvent == PreCardUsed)
                card = data.value<CardUseStruct>().card;
            else {
                CardResponseStruct response = data.value<CardResponseStruct>();
                if (response.m_isUse)
                    card = response.m_card;
            }
            if (card && card->getTypeId() != Card::TypeSkill && card->getHandlingMethod() == Card::MethodUse) {
                int n = player->getMark(objectName());
                if (card->isBlack())
                    n |= 1;
                else if (card->isRed())
                    n |= 2;
                player->setMark(objectName(), n);
            }
        } else if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Discard) {
                int n = player->getMark(objectName());
                bool red_avail = ((n & 2) == 0), black_avail = ((n & 1) == 0);
                if (player->isKongcheng() || (!red_avail && !black_avail))
                    return false;
                QString pattern = ".|.|.|hand";
                if (red_avail != black_avail)
                    pattern = QString(".|%1|.|hand").arg(red_avail ? "red" : "black");
                const Card *card = room->askForCard(player, pattern, "@lesbianxingwu", QVariant(), Card::MethodNone);
                if (card) {
                    room->notifySkillInvoked(player, objectName());
                    player->broadcastSkillInvoke(objectName());

                    LogMessage log;
                    log.type = "#InvokeSkill";
                    log.from = player;
                    log.arg = objectName();
                    room->sendLog(log);

                    player->addToPile(objectName(), card);
                }
            } else if (player->getPhase() == Player::RoundStart) {
                player->setMark(objectName(), 0);
            }
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to == player && move.to_place == Player::PlaceSpecial && player->getPile(objectName()).length() >= 3) {
                player->clearOnePrivatePile(objectName());
                QList<ServerPlayer *> females;
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p && p->objectName() != player->objectName())
                        females << p;
                }
                if (females.isEmpty()) return false;

                ServerPlayer *target = room->askForPlayerChosen(player, females, objectName(), "@lesbianxingwu-choose");
                room->damage(DamageStruct(objectName(), player, target, 2));

                if (!player->isAlive()) return false;
                QList<const Card *> equips = target->getEquips();
                if (!equips.isEmpty()) {
                    DummyCard *dummy = new DummyCard;
                    dummy->deleteLater();
                    foreach (const Card *equip, equips) {
                        if (player->canDiscard(target, equip->getEffectiveId()))
                            dummy->addSubcard(equip);
                    }
                    if (dummy->subcardsLength() > 0)
                        room->throwCard(dummy, target, player);
                }
            }
        }
        return false;
    }
};

class LesbianLuoyan : public TriggerSkill
{
public:
    LesbianLuoyan() : TriggerSkill("#lesbianluoyan")
    {
        events << CardsMoveOneTime << EventAcquireSkill << EventLoseSkill;
        frequency = Compulsory;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventLoseSkill && data.toString() == objectName()) {
            room->handleAcquireDetachSkills(player, "-tianxiang|-liuli", true);
        } else if (triggerEvent == EventAcquireSkill && data.toString() == objectName()) {
            if (!player->getPile("lesbianxingwu").isEmpty()) {
                room->notifySkillInvoked(player, objectName());
                room->handleAcquireDetachSkills(player, "tianxiang|liuli");
            }
        } else if (triggerEvent == CardsMoveOneTime && player->isAlive() && player->hasSkill(this, true)) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to == player && move.to_place == Player::PlaceSpecial && move.to_pile_name == "lesbianxingwu") {
                if (player->getPile("lesbianxingwu").length() == 1) {
                    room->notifySkillInvoked(player, objectName());
                    room->handleAcquireDetachSkills(player, "tianxiang|liuli");
                }
            } else if (move.from == player && move.from_places.contains(Player::PlaceSpecial)
                && move.from_pile_names.contains("lesbianxingwu")) {
                if (player->getPile("lesbianxingwu").isEmpty())
                    room->handleAcquireDetachSkills(player, "-tianxiang|-liuli", true);
            }
        }
        return false;
    }
};

NosLesbianLijianCard::NosLesbianLijianCard() : LesbianLijianCard(false)
{
}

class NosLesbianLijian : public OneCardViewAsSkill
{
public:
    NosLesbianLijian() : OneCardViewAsSkill("noslesbianlijian")
    {
        filter_pattern = ".!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getAliveSiblings().length() > 1
            && player->canDiscard(player, "he") && !player->hasUsed("NosLesbianLijianCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        NosLesbianLijianCard *lijian_card = new NosLesbianLijianCard;
        lijian_card->addSubcard(originalCard->getId());
        return lijian_card;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        return card->isKindOf("Duel") ? 0 : -1;
    }
};

LesbianLianliSlashCard::LesbianLianliSlashCard()
{

}

bool LesbianLianliSlashCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Slash *slash = new Slash(NoSuit, 0);
    slash->deleteLater();
    return slash->targetFilter(targets, to_select, Self);
}

const Card *LesbianLianliSlashCard::validate(CardUseStruct &cardUse) const
{
    cardUse.m_isOwnerUse = false;
    ServerPlayer *zhangfei = cardUse.from;
    Room *room = zhangfei->getRoom();

    ServerPlayer *xiahoujuan = room->findPlayerBySkillName("lesbianlianli");
    if (xiahoujuan) {
        const Card *slash = room->askForCard(xiahoujuan, "slash", "@lesbianlianli-slash", QVariant(), Card::MethodResponse, NULL, false, QString(), true);
        if (slash)
            return slash;
    }
    room->setPlayerFlag(zhangfei, "Global_LesbianLianliFailed");
    return NULL;
}

class LesbianLianliSlashViewAsSkill :public ZeroCardViewAsSkill
{
public:
    LesbianLianliSlashViewAsSkill() :ZeroCardViewAsSkill("lesbianlianli-slash")
    {
        attached_lord_skill = true;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@tied") > 0 && Slash::IsAvailable(player) && !player->hasFlag("Global_LesbianLianliFailed");
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern == "slash"
            && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE
            && !player->hasFlag("Global_LesbianLianliFailed");
    }

    const Card *viewAs() const
    {
        return new LesbianLianliSlashCard;
    }
};

class LesbianLianliSlash : public TriggerSkill
{
public:
    LesbianLianliSlash() :TriggerSkill("#lesbianlianli-slash")
    {
        events << CardAsked;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->getMark("@tied") > 0 && !target->hasSkill("lesbianlianli");
    }

    bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const
    {
        QString pattern = data.toStringList().first();
        if (pattern != "slash")
            return false;

        if (!player->askForSkillInvoke("lesbianlianli-slash", data))
            return false;

        ServerPlayer *xiahoujuan = room->findPlayerBySkillName("lesbianlianli");
        if (xiahoujuan) {
            const Card *slash = room->askForCard(xiahoujuan, "slash", "@lesbianlianli-slash", data, Card::MethodResponse, NULL, false, QString(), true);
            if (slash) {
                room->provide(slash);
                return true;
            }
        }

        return false;
    }
};

class LesbianLianliJink : public TriggerSkill
{
public:
    LesbianLianliJink() :TriggerSkill("#lesbianlianli-jink")
    {
        events << CardAsked;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && TriggerSkill::triggerable(target) && target->getMark("@tied") > 0;
    }

    bool trigger(TriggerEvent, Room* room, ServerPlayer *xiahoujuan, QVariant &data) const
    {
        QString pattern = data.toStringList().first();
        if (pattern != "jink")
            return false;

        if (!xiahoujuan->askForSkillInvoke("lesbianlianli-jink", data))
            return false;

        QList<ServerPlayer *> players = room->getOtherPlayers(xiahoujuan);
        foreach (ServerPlayer *player, players) {
            if (player->getMark("@tied") > 0) {
                ServerPlayer *zhangfei = player;

                const Card *jink = room->askForCard(zhangfei, "jink", "@lesbianlianli-jink", data, Card::MethodResponse, NULL, false, QString(), true);
                if (jink) {
                    room->provide(jink);
                    return true;
                }

                break;
            }
        }

        return false;
    }
};
/*

class LesbianLianliViewAsSkill: public ZeroCardViewAsSkill{
public:
LesbianLianliViewAsSkill():ZeroCardViewAsSkill("lesbianlianli"){

}

bool isEnabledAtPlay(const Player *player) const{
return false;
}

bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
return pattern == "@@lesbianlianli";
}

const Card *viewAs() const{
return new LesbianLianliCard;
}
};
*/

class LesbianLianli : public PhaseChangeSkill
{
public:
    LesbianLianli() :PhaseChangeSkill("lesbianlianli")
    {
        //view_as_skill = new LesbianLianliViewAsSkill;
    }

    bool onPhaseChange(ServerPlayer *target) const
    {
        if (target->getPhase() == Player::Start) {
            Room *room = target->getRoom();
            //bool used = room->askForUseCard(target, "@@lesbianlianli", "@lesbianlianli-card");

            QList<ServerPlayer *> females;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p && p->objectName() != target->objectName())
                    females << p;
            }

            ServerPlayer *zhangfei;
            if (females.isEmpty() || ((zhangfei = room->askForPlayerChosen(target, females, objectName(), "@lesbianlianli-card", true, true)) == NULL)) {
                if (target->hasSkill("liqian") && target->getKingdom() != "wei")
                    room->setPlayerProperty(target, "kingdom", "wei");

                QList<ServerPlayer *> players = room->getAllPlayers();
                foreach (ServerPlayer *player, players) {
                    if (player->getMark("@tied") > 0) {
                        player->loseMark("@tied");
                        if (player)
                            room->detachSkillFromPlayer(player, "lesbianlianli-slash", true, true);
                    }
                }
                return false;
            }

            room->broadcastSkillInvoke(objectName());
            LogMessage log;
            log.type = "#LesbianLianliConnection";
            log.from = target;
            log.to << zhangfei;
            room->sendLog(log);

            if (target->getMark("@tied") == 0)
                target->gainMark("@tied");

            if (zhangfei->getMark("@tied") == 0) {
                QList<ServerPlayer *> players = room->getOtherPlayers(target);
                foreach (ServerPlayer *player, players) {
                    if (player->getMark("@tied") > 0) {
                        player->loseMark("@tied");
                        room->detachSkillFromPlayer(player, "lesbianlianli-slash", true, true);
                        break;
                    }
                }

                zhangfei->gainMark("@tied");
                room->attachSkillToPlayer(zhangfei, "lesbianlianli-slash");
            }

            if (target->hasSkill("liqian") && target->getKingdom() != zhangfei->getKingdom())
                room->setPlayerProperty(target, "kingdom", zhangfei->getKingdom());
        }
        return false;
    }
};

class LesbianTongxin : public MasochismSkill
{
public:
    LesbianTongxin() :MasochismSkill("lesbiantongxin")
    {
        frequency = Frequent;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->getMark("@tied") > 0;
    }

    void onDamaged(ServerPlayer *target, const DamageStruct &damage) const
    {
        Room *room = target->getRoom();
        ServerPlayer *xiahoujuan = room->findPlayerBySkillName(objectName());
        ServerPlayer *zhangfei = NULL;
        if (target == xiahoujuan) {
            QList<ServerPlayer *> players = room->getOtherPlayers(xiahoujuan);
            foreach (ServerPlayer *player, players) {
                if (player->getMark("@tied") > 0) {
                    zhangfei = player;
                    break;
                }
            }
        } else
            zhangfei = target;
        for (int i = 0; i < damage.damage; i++) {
            if (xiahoujuan && xiahoujuan->askForSkillInvoke(this, QVariant::fromValue(damage))) {
                room->broadcastSkillInvoke(objectName());
                room->notifySkillInvoked(xiahoujuan, objectName());
                xiahoujuan->drawCards(1);
                if (zhangfei)
                    zhangfei->drawCards(1);
            } else
                break;
        }

    }
};

class LesbianLianliClear : public TriggerSkill
{
public:
    LesbianLianliClear() :TriggerSkill("#lesbianlianli-clear")
    {
        events << Death << EventLoseSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player || !player->hasSkill(this))
                return false;
        } else if (triggerEvent == EventLoseSkill) {
            if (data.toString() != "lesbianlianli")
                return false;
        }

        foreach (ServerPlayer *player, room->getAlivePlayers()) {
            if (player->getMark("@tied") > 0) {
                player->loseMark("@tied");
                if (player)
                    room->detachSkillFromPlayer(player, "lesbianlianli-slash", true, true);
            }
        }

        return false;
    }
};

class LesbianFuhan : public TriggerSkill
{
public:
    LesbianFuhan() : TriggerSkill("lesbianfuhan")
    {
        events << EventPhaseStart;
        frequency = Limited;
        limit_mark = "@assisthan";
    }

    static QStringList GetAvailableGenerals(ServerPlayer *zuoci)
    {
        QSet<QString> all = Sanguosha->getLimitedGeneralNames("shu").toSet();

        //QSet<QString> all = all_generals.toSet();
        Room *room = zuoci->getRoom();
        all.subtract(Config.value("Banlist/Roles", "").toStringList().toSet());
        QSet<QString> huashen_set, room_set;
        foreach (ServerPlayer *player, room->getAlivePlayers()) {
            QVariantList huashens = player->tag["Huashens"].toList();
            foreach(QVariant huashen, huashens)
                huashen_set << huashen.toString();

            QString name = player->getGeneralName();
            if (Sanguosha->isGeneralHidden(name)) {
                QString fname = Sanguosha->getMainGeneral(name);
                if (!fname.isEmpty()) name = fname;
            }
            room_set << name;

            if (!player->getGeneral2()) continue;

            name = player->getGeneral2Name();
            if (Sanguosha->isGeneralHidden(name)) {
                QString fname = Sanguosha->getMainGeneral(name);
                if (!fname.isEmpty()) name = fname;
            }
            room_set << name;
        }

        static QSet<QString> banned;
        if (banned.isEmpty())
            banned << "zhaoxiang";

        QStringList all_usable_generals = (all - banned - huashen_set - room_set).toList();
        QStringList all_generals;
        foreach (QString name, all_usable_generals)
        {
            const General *general = Sanguosha->getGeneral(name);
            if (general && general->isFemale())
                all_generals << general->objectName();
            else if (general && name.contains("luxun"))
                all_generals << name;
        }
        return all_generals;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() != Player::RoundStart)
            return false;
        if (player->getMark(limit_mark) > 0 && (player->getMark("#plumshadow") > 0 || player->getMark("plumshadow_removed") > 0) && player->askForSkillInvoke(this)) {
            room->removePlayerMark(player, limit_mark);
            room->addPlayerMark(player, "plumshadow_removed", player->getMark("#plumshadow"));
            room->setPlayerMark(player, "#plumshadow", 0);
            player->broadcastSkillInvoke(objectName());
            QList<const Skill *> skills = player->getVisibleSkillList();
            QStringList detachList, acquireList;
            foreach (const Skill *skill, skills) {
                if (!skill->isAttachedLordSkill())
                    detachList.append("-" + skill->objectName());
            }
            room->handleAcquireDetachSkills(player, detachList);
            QStringList all_generals = GetAvailableGenerals(player);
            qShuffle(all_generals);
            if (all_generals.isEmpty()) return false;
            QStringList general_list = all_generals.mid(0, qMin(5, all_generals.length()));
            QString general_name = room->askForGeneral(player, general_list, true, "lesbianfuhan");
            const General *general = Sanguosha->getGeneral(general_name);
            foreach (const Skill *skill, general->getVisibleSkillList()) {
                if (skill->isLordSkill() && !player->isLord())
                    continue;

                acquireList.append(skill->objectName());
            }
            room->handleAcquireDetachSkills(player, acquireList);
            room->setPlayerProperty(player, "kingdom", general->getKingdom());
            //player->setGender(General::Female);
            player->setGeneralName(general_name);
            room->broadcastProperty(player, "general");

            int maxhp = qMin(player->getMark("plumshadow_removed"), Sanguosha->getPlayerCount(room->getMode()));
            room->setPlayerProperty(player, "maxhp", maxhp);
            QList<ServerPlayer *> players = room->getOtherPlayers(player);
            foreach (ServerPlayer *p, players) {
                if (player->getHp() > p->getHp()) {
                    return false;
                }
            }
            room->recover(player, RecoverStruct(player));
        }
        return false;
    }
};

StandardmsPackage::StandardmsPackage()
    : Package("Standardms")
{
    General *liubei = new General(this, "miansha_liubei$", "shu");
    liubei->addSkill(new Rernder);
    liubei->addSkill("jijiang");

    General *lidian = new General(this, "miansha_lidian", "wei", 3);
    lidian->addSkill(new Shyunshyun);
    lidian->addSkill("wangxi");

    General *xiahoudun = new General(this, "miansha_xiahoudun", "wei");
    xiahoudun->addSkill("ganglie");
    xiahoudun->addSkill(new Chingjean);

    addMetaObject<RernderCard>();

    addMetaObject<LesbianLijianCard>();
    addMetaObject<LesbianJieyinCard>();
    addMetaObject<LesbianLihunCard>();
    addMetaObject<NosLesbianLijianCard>();
    addMetaObject<LesbianLianliSlashCard>();

    skills << new LesbianLijian << new LesbianJieyin << new LesbianJiaojin
           << new LesbianYanyu << new LesbianFuzhu << new LesbianLihun << new NosLesbianLijian
           << new LesbianXingwu << new LesbianLuoyan << new LesbianLianli
           << new LesbianLianliClear << new LesbianLianliJink << new LesbianLianliSlash
           << new LesbianLianliSlashViewAsSkill << new LesbianTongxin << new LesbianFuhan;
}

ADD_PACKAGE(Standardms)

WindmsPackage::WindmsPackage()
    :Package("Windms")
{

    General *miansha_caoren = new General(this, "miansha_caoren", "wei"); // WEI 011
    miansha_caoren->addSkill(new Jiuhshoou);
    miansha_caoren->addSkill(new Jieewei);

    General *miansha_zhangjiao = new General(this, "miansha_zhangjiao$", "qun", 3); // QUN 010
    miansha_zhangjiao->addSkill(new Leirji);
    miansha_zhangjiao->addSkill("guidao");
    miansha_zhangjiao->addSkill("huangtian");

    General *miansha_xiaoqiao = new General(this, "miansha_xiaoqiao", "wu", 3, false); // WU 011
    miansha_xiaoqiao->addSkill(new Tianshiang);
    miansha_xiaoqiao->addSkill(new TianshiangDraw);
    miansha_xiaoqiao->addSkill("hongyan");
    related_skills.insertMulti("tianshiang", "#tianshiang");

    General *miansha_huangzhong = new General(this, "miansha_huangzhong", "shu"); // SHU 008
    miansha_huangzhong->addSkill(new Liehgong);

    General *miansha_xiahouyuan = new General(this, "miansha_xiahouyuan", "wei"); // WEI 008
    miansha_xiahouyuan->addSkill(new Shernsuh);
    miansha_xiahouyuan->addSkill(new SlashNoDistanceLimitSkill("shernsuh"));
    related_skills.insertMulti("shernsuh", "#shernsuh-slash-ndl");

    General *miansha_weiyan = new General(this, "miansha_weiyan", "shu"); // SHU 009
    miansha_weiyan->addSkill(new Kwangguu);
    miansha_weiyan->addSkill(new KwangguuRecord);
    related_skills.insertMulti("kwangguu", "#kwangguu-record");

    addMetaObject<TianshiangCard>();
    addMetaObject<ShernsuhCard>();
}

ADD_PACKAGE(Windms)

NostalFirePackage::NostalFirePackage()
    :Package("NostalFire")
{
    General *pangde = new General(this, "nos_pangde", "qun"); // QUN 008
    pangde->addSkill("mashu");
    pangde->addSkill(new Mengjin);
}

ADD_PACKAGE(NostalFire)

NostalThicketPackage::NostalThicketPackage()
    :Package("NostalThicket")
{
    General *nos_xuhuang = new General(this, "nos_xuhuang", "wei"); // WEI 010
    nos_xuhuang->addSkill(new Duannlyang);
    nos_xuhuang->addSkill(new DuannlyangTargetMod);
    related_skills.insertMulti("duannlyang", "#duannlyang-target");

    General *nos_shencaocao = new General(this, "nosms_shencaocao", "god", 3);
    nos_shencaocao->addSkill(new Gueishin);
    nos_shencaocao->addSkill("feiying");
}

ADD_PACKAGE(NostalThicket)

WestMianshaPackage::WestMianshaPackage()
    :Package("WestMiansha")
{
    General *Caesar = new General(this, "caesar", "god", 4); // E.SP 001
    Caesar->addSkill(new Conqueror);
}

ADD_PACKAGE(WestMiansha)
