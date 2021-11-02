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
#include "limit-mol.h"

MOLQingjianAllotCard::MOLQingjianAllotCard()
{
    will_throw = false;
    mute = true;
    handling_method = Card::MethodNone;
}

void MOLQingjianAllotCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    QList<int> rende_list = StringList2IntList(source->property("molqingjian_give").toString().split("+"));
    foreach (int id, subcards) {
        rende_list.removeOne(id);
    }
    room->setPlayerProperty(source, "molqingjian_give", IntList2StringList(rende_list).join("+"));

    QList<int> give_list = StringList2IntList(target->property("rende_give").toString().split("+"));
    foreach (int id, subcards) {
        give_list.append(id);
    }
    room->setPlayerProperty(target, "rende_give", IntList2StringList(give_list).join("+"));
}

class MOLQingjianAllot : public ViewAsSkill
{
public:
    MOLQingjianAllot() : ViewAsSkill("molqingjian_allot")
    {
        expand_pile = "molqingjian";
        response_pattern = "@@molqingjian_allot!";
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        QList<int> molqingjian_list = StringList2IntList(Self->property("molqingjian_give").toString().split("+"));
        return molqingjian_list.contains(to_select->getEffectiveId());
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty()) return NULL;
        MOLQingjianAllotCard *molqingjian_card = new MOLQingjianAllotCard;
        molqingjian_card->addSubcards(cards);
        return molqingjian_card;
    }
};

class MOLQingjianViewAsSkill : public ViewAsSkill
{
public:
    MOLQingjianViewAsSkill() : ViewAsSkill("molqingjian")
    {
        response_pattern = "@@molqingjian";
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        QList<int> molqingjian_list = StringList2IntList(Self->property("molqingjian").toString().split("+"));
        return molqingjian_list.contains(to_select->getEffectiveId());
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

class MOLQingjian : public TriggerSkill
{
public:
    MOLQingjian() : TriggerSkill("molqingjian")
    {
        events << CardsMoveOneTime << EventPhaseChanging;
        view_as_skill = new MOLQingjianViewAsSkill;
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
                if (xiahou->getPile("molqingjian").length() > 0 && TriggerSkill::triggerable(xiahou)) {
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
            room->setPlayerProperty(player, "molqingjian", IntList2StringList(ids).join("+"));
            const Card *card = room->askForCard(player, "@@molqingjian", "@molqingjian-put", data, Card::MethodNone);
            room->setPlayerProperty(player, "molqingjian", QString());
            if (card) {
                LogMessage log;
                log.type = "#InvokeSkill";
                log.arg = objectName();
                log.from = player;
                room->sendLog(log);
                room->notifySkillInvoked(player, objectName());
                player->broadcastSkillInvoke(objectName());
                player->addToPile("molqingjian", card, false);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive) return false;
            if (xiahou->getPile("molqingjian").length() > 0 && TriggerSkill::triggerable(xiahou)) {
                bool will_draw = xiahou->getPile("molqingjian").length() > 1;
                room->sendCompulsoryTriggerLog(xiahou, objectName());
                xiahou->broadcastSkillInvoke(objectName());
                QList<int> ids = xiahou->getPile("molqingjian");
                room->setPlayerProperty(xiahou, "molqingjian_give", IntList2StringList(ids).join("+"));
                do {
                    const Card *use = room->askForUseCard(xiahou, "@@molqingjian_allot!", "@molqingjian-give", QVariant(), Card::MethodNone);
                    ids = StringList2IntList(xiahou->property("molqingjian_give").toString().split("+"));
                    if (use == NULL) {
                        MOLQingjianAllotCard *molqingjian_card = new MOLQingjianAllotCard;
                        molqingjian_card->addSubcards(ids);
                        QList<ServerPlayer *> targets;
                        targets << room->getOtherPlayers(xiahou).first();
                        molqingjian_card->use(room, xiahou, targets);
                        delete molqingjian_card;
                        break;
                    }
                } while (!ids.isEmpty() && xiahou->isAlive());
                room->setPlayerProperty(xiahou, "molqingjian_give", QString());
                QList<CardsMoveStruct> moves;
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    QList<int> give_list = StringList2IntList(p->property("rende_give").toString().split("+"));
                    if (give_list.isEmpty())
                        continue;
                    room->setPlayerProperty(p, "rende_give", QString());
                    CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, p->objectName(), "molqingjian", QString());
                    CardsMoveStruct move(give_list, p, Player::PlaceHand, reason);
                    moves.append(move);
                }
                if (!moves.isEmpty())
                    room->moveCardsAtomic(moves, false);
                if (will_draw)
                    room->drawCards(xiahou, 1);
            }
        }
        return false;
    }
};

class MOLZaiqi : public TriggerSkill
{
public:
    MOLZaiqi() : TriggerSkill("molzaiqi")
    {
        events << CardsMoveOneTime << EventPhaseEnd << EventPhaseChanging;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardsMoveOneTime && player->getPhase() != Player::NotActive) {
            int x = 0;
            QVariantList list = data.toList();
            foreach (QVariant qvar, list) {
                CardsMoveOneTimeStruct move = qvar.value<CardsMoveOneTimeStruct>();
                if (move.to_place == Player::DiscardPile) {
                    foreach (int id, move.card_ids) {
                        if (Sanguosha->getCard(id)->getColor() == Card::Red)
                            ++x;
                    }
                }
            }
            room->setTag("MOLZaiqiRecord", room->getTag("MOLZaiqiRecord").toInt() + x);
        } else if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                room->removeTag("MOLZaiqiRecord");
            }
        }
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseEnd && TriggerSkill::triggerable(player) &&
                player->getPhase() == Player::Discard && room->getTag("MOLZaiqiRecord").toInt())
            return nameList();
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        int x = room->getTag("MOLZaiqiRecord").toInt();
        QList<ServerPlayer *> targets = room->askForPlayersChosen(player, room->getAlivePlayers(), objectName(), 0, x, QString(), true);
        foreach (ServerPlayer *p, targets) {
            if (!player->isWounded() ||
                    room->askForChoice(p, objectName(), "draw+recover", QVariant::fromValue(player), "@molzaiqi-choose:" + player->objectName()) == "draw")
                room->drawCards(p, 1);
            else {
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, p->objectName(), player->objectName());
                room->recover(player, RecoverStruct(p));
            }
        }
        return false;
    }
};

class Poluu : public TriggerSkill
{
public:
    Poluu() : TriggerSkill("poluu")
    {
        events << Death;
        //frequency = Frequent;
    }

    QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (player == NULL || !player->hasSkill(this)) return QStringList();
        DeathStruct death = data.value<DeathStruct>();
        if (death.who == player)
            return nameList();
        if (death.damage && death.damage->from == player)
            return nameList();
        return QStringList();
    }

    bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (room->askForSkillInvoke(player, objectName())) {
            room->addPlayerMark(player, "#poluu");
            int x = player->getMark("#poluu");
            QList<ServerPlayer *> targets =
                room->askForPlayersChosen(player, room->getAlivePlayers(), objectName(), 1,
                                          room->getAlivePlayers().size(), "@poluu-invoke:::" + QString::number(x));
            foreach (ServerPlayer *target, targets) {
                room->drawCards(target, x);
            }
        }
        return false;
    }
};

class MOLJiuchiViewAsSkill : public OneCardViewAsSkill
{
public:
    MOLJiuchiViewAsSkill() : OneCardViewAsSkill("moljiuchi")
    {
        filter_pattern = ".|spade|.|hand";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return hasAvailable(player) || Analeptic::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return  pattern.contains("analeptic");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Analeptic *analeptic = new Analeptic(originalCard->getSuit(), originalCard->getNumber());
        analeptic->setSkillName(objectName());
        analeptic->addSubcard(originalCard->getId());
        return analeptic;
    }
};

class MOLJiuchi : public TriggerSkill
{
public:
    MOLJiuchi() : TriggerSkill("moljiuchi")
    {
        events << Damage;
        view_as_skill = new MOLJiuchiViewAsSkill;
    }

    void record(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (!TriggerSkill::triggerable(player)) return;
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Slash") && damage.card->hasFlag("drank"))
            room->setPlayerFlag(player, "noBenghuai");
    }

    bool triggerable(const ServerPlayer *) const
    {
        return false;
    }
};

class MOLBaonue : public TriggerSkill
{
public:
    MOLBaonue() : TriggerSkill("molbaonue$")
    {
        events << Damage;
        view_as_skill = new dummyVS;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (player->isAlive() && player->getKingdom() == "qun") {
            foreach (ServerPlayer *dongzhuo, room->getOtherPlayers(player)) {
                if (dongzhuo->hasLordSkill(objectName()))
                    skill_list.insert(dongzhuo, QStringList("molbaonue!"));
            }
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *dongzhuo) const
    {
        if (room->askForChoice(player, objectName(), "yes+no", data, "@molbaonue-to:" + dongzhuo->objectName()) == "yes") {
            LogMessage log;
            log.type = "#InvokeOthersSkill";
            log.from = player;
            log.to << dongzhuo;
            log.arg = objectName();
            room->sendLog(log);
            room->notifySkillInvoked(dongzhuo, objectName());
            dongzhuo->broadcastSkillInvoke(objectName());

            JudgeStruct judge;
            judge.pattern = ".|spade";
            judge.good = true;
            judge.reason = objectName();
            judge.who = player;

            room->judge(judge);

            if (judge.isGood())
                room->recover(dongzhuo, RecoverStruct(player));
        }
        return false;
    }
};

class MOLTuntian : public TriggerSkill
{
public:
    MOLTuntian() : TriggerSkill("moltuntian")
    {
        events << CardsMoveOneTime << FinishJudge;
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
        if (triggerEvent == CardsMoveOneTime && TriggerSkill::triggerable(player) && player->getPhase() == Player::NotActive) {
            QVariantList move_datas = data.toList();
            foreach(QVariant move_data, move_datas) {
                CardsMoveOneTimeStruct move = move_data.value<CardsMoveOneTimeStruct>();
                if (move.from == player && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
                        && !(move.to == player && (move.to_place == Player::PlaceHand || move.to_place == Player::PlaceEquip))) {
                    return nameList();
                }
            }
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason == objectName() && room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge)
                return QStringList("moltuntian!");
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == CardsMoveOneTime) {
            if (player->askForSkillInvoke(objectName(), data)) {
                player->broadcastSkillInvoke(objectName());
                JudgeStruct judge;
                judge.pattern = ".|heart";
                judge.good = false;
                judge.reason = objectName();
                judge.who = player;
                room->judge(judge);
            }
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->card && room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge)
                if (judge->isGood())
                    player->addToPile("molfield", judge->card->getEffectiveId());
                else
                    player->obtainCard(judge->card);
        }

        return false;
    }
};

class MOLTuntianDistance : public DistanceSkill
{
public:
    MOLTuntianDistance() : DistanceSkill("#moltuntian-dist")
    {
    }

    virtual int getCorrect(const Player *from, const Player *) const
    {
        if (from->hasSkill("moltuntian"))
            return -from->getPile("molfield").length();
        else
            return 0;
    }
};

class MOLZaoxian : public PhaseChangeSkill
{
public:
    MOLZaoxian() : PhaseChangeSkill("molzaoxian")
    {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target)
               && target->getPhase() == Player::Start
               && target->getMark("molzaoxian") == 0
               && target->getPile("molfield").length() >= 3;
    }

    virtual bool onPhaseChange(ServerPlayer *dengai) const
    {
        Room *room = dengai->getRoom();
        room->sendCompulsoryTriggerLog(dengai, objectName());

        dengai->broadcastSkillInvoke(objectName());
        //room->doLightbox("$MOLZaoxianAnimate", 4000);

        room->setPlayerMark(dengai, "molzaoxian", 1);
        if (room->changeMaxHpForAwakenSkill(dengai) && dengai->getMark("molzaoxian") == 1)
            room->acquireSkill(dengai, "moljixi");

        return false;
    }
};

class MOLJixi : public OneCardViewAsSkill
{
public:
    MOLJixi() : OneCardViewAsSkill("moljixi")
    {
        filter_pattern = ".|.|.|molfield";
        expand_pile = "molfield";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->getPile("molfield").isEmpty();
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Snatch *snatch = new Snatch(originalCard->getSuit(), originalCard->getNumber());
        snatch->addSubcard(originalCard);
        snatch->setSkillName(objectName());
        return snatch;
    }
};

class MOLFangquan : public TriggerSkill
{
public:
    MOLFangquan() : TriggerSkill("molfangquan")
    {
        events << EventPhaseChanging;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player)) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Play && !player->isSkipped(Player::Play)) {
                return nameList();
            } else if (change.to == Player::NotActive && player->hasFlag("molfangquanInvoked"))
                return QStringList("molfangquan!");
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *liushan, QVariant &data, ServerPlayer *) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Play && liushan->askForSkillInvoke(this)) {
            liushan->broadcastSkillInvoke(objectName(), 1);
            liushan->skip(Player::Play, true);
            liushan->setFlags("molfangquanInvoked");
        } else if (change.to == Player::NotActive) {
            room->sendCompulsoryTriggerLog(liushan, objectName());
            liushan->broadcastSkillInvoke(objectName(), 2);
            room->askForUseCard(liushan, "@@fangquanask", "@fangquan-give", data, Card::MethodDiscard);
        }
        return false;
    }
};

class MOLFangquanMaxCards : public MaxCardsSkill
{
public:
    MOLFangquanMaxCards() : MaxCardsSkill("#molfangquan")
    {

    }

    virtual int getFixed(const Player *target) const
    {
        if (target->hasSkill("molfangquan") && target->hasFlag("molfangquanInvoked"))
            return target->getMaxHp();
        else
            return -1;
    }
};

class MOLRuoyu : public PhaseChangeSkill
{
public:
    MOLRuoyu() : PhaseChangeSkill("molruoyu$")
    {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        if (target != NULL && target->isAlive() && target->hasLordSkill("molruoyu") && target->getMark("molruoyu") == 0 && target->getPhase() == Player::Start) {
            QList<ServerPlayer *> players = target->getRoom()->getAlivePlayers();
            foreach (ServerPlayer *p, players) {
                if (target->getHp() > p->getHp()) return false;
            }
            return true;
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *liushan) const
    {
        Room *room = liushan->getRoom();

        room->sendCompulsoryTriggerLog(liushan, objectName());

        liushan->broadcastSkillInvoke(objectName());

        room->setPlayerMark(liushan, "molruoyu", 1);
        if (room->changeMaxHpForAwakenSkill(liushan, 1)) {
            room->recover(liushan, RecoverStruct(liushan));
            room->acquireSkill(liushan, "jijiang");
        }

        return false;
    }
};

class MOLFenji : public TriggerSkill
{
public:
    MOLFenji() : TriggerSkill("molfenji")
    {
        events << EventPhaseChanging;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList list;
        if (data.value<PhaseChangeStruct>().to == Player::NotActive && player->isKongcheng())
            foreach (ServerPlayer *sp, room->getAllPlayers())
                if (TriggerSkill::triggerable(sp))
                    list.insert(sp, nameList());
        return list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *target, QVariant &, ServerPlayer *zhoutai) const
    {
        if (room->askForSkillInvoke(zhoutai, objectName(), QVariant::fromValue(target))) {
            zhoutai->broadcastSkillInvoke(objectName());
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, zhoutai->objectName(), target->objectName());

            if (target->isAlive())
                target->drawCards(2, objectName());
            room->loseHp(zhoutai);
        }
        return false;
    }
};

LimitMOLPackage::LimitMOLPackage()
    : Package("limit_mol")
{
    General *xiahoudun = new General(this, "mol_xiahoudun", "wei", 4, true, true);
    xiahoudun->addSkill("ganglie");
    xiahoudun->addSkill(new MOLQingjian);
    xiahoudun->addSkill(new DetachEffectSkill("molqingjian", "molqingjian"));
    related_skills.insertMulti("molqingjian", "#molqingjian-clear");

    General *zhoutai = new General(this, "mol_zhoutai", "wu", 4, true, true);
    zhoutai->addSkill("buqu");
    zhoutai->addSkill(new MOLFenji);

    General *menghuo = new General(this, "mol_menghuo", "shu", 4, true, true);
    menghuo->addSkill(new MOLZaiqi);
    menghuo->addSkill("huoshou");

    General *sunjian = new General(this, "mol_sunjian", "wu", 4, true, true);
    sunjian->addSkill("yinghun");
    sunjian->addSkill(new Poluu);

    General *dongzhuo = new General(this, "mol_dongzhuo", "qun", 8, true, true);
    dongzhuo->addSkill(new MOLJiuchi);
    dongzhuo->addSkill("roulin");
    dongzhuo->addSkill("benghuai");
    dongzhuo->addSkill(new MOLBaonue);

    General *dengai = new General(this, "mol_dengai", "wei", 4, true, true);
    dengai->addSkill(new MOLTuntian);
    dengai->addSkill(new MOLTuntianDistance);
    dengai->addSkill(new DetachEffectSkill("moltuntian", "molfield"));
    related_skills.insertMulti("moltuntian", "#moltuntian-dist");
    related_skills.insertMulti("moltuntian", "#moltuntian-clear");
    dengai->addSkill(new MOLZaoxian);
    dengai->addRelateSkill("moljixi");

    General *liushan = new General(this, "mol_liushan$", "shu", 3, true, true);
    liushan->addSkill("xiangle");
    liushan->addSkill(new MOLFangquan);
    liushan->addSkill(new MOLFangquanMaxCards);
    liushan->addSkill(new MOLRuoyu);
    related_skills.insertMulti("molfangquan", "#molfangquan");

    addMetaObject<MOLQingjianAllotCard>();
    skills << new MOLQingjianAllot << new MOLJixi;
}

ADD_PACKAGE(LimitMOL)
