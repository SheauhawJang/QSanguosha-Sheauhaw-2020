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
#include "nostalgia-mol.h"

NMOLQingjianAllotCard::NMOLQingjianAllotCard()
{
    will_throw = false;
    mute = true;
    handling_method = Card::MethodNone;
}

void NMOLQingjianAllotCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    QList<int> rende_list = StringList2IntList(source->property("nmolqingjian_give").toString().split("+"));
    foreach (int id, subcards) {
        rende_list.removeOne(id);
    }
    room->setPlayerProperty(source, "nmolqingjian_give", IntList2StringList(rende_list).join("+"));

    QList<int> give_list = StringList2IntList(target->property("rende_give").toString().split("+"));
    foreach (int id, subcards) {
        give_list.append(id);
    }
    room->setPlayerProperty(target, "rende_give", IntList2StringList(give_list).join("+"));
}

class NMOLQingjianAllot : public ViewAsSkill
{
public:
    NMOLQingjianAllot() : ViewAsSkill("nmolqingjian_allot")
    {
        expand_pile = "nmolqingjian";
        response_pattern = "@@nmolqingjian_allot!";
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        QList<int> nmolqingjian_list = StringList2IntList(Self->property("nmolqingjian_give").toString().split("+"));
        return nmolqingjian_list.contains(to_select->getEffectiveId());
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty()) return NULL;
        NMOLQingjianAllotCard *nmolqingjian_card = new NMOLQingjianAllotCard;
        nmolqingjian_card->addSubcards(cards);
        return nmolqingjian_card;
    }
};

class NMOLQingjianViewAsSkill : public ViewAsSkill
{
public:
    NMOLQingjianViewAsSkill() : ViewAsSkill("nmolqingjian")
    {
        response_pattern = "@@nmolqingjian";
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        QList<int> nmolqingjian_list = StringList2IntList(Self->property("nmolqingjian").toString().split("+"));
        return nmolqingjian_list.contains(to_select->getEffectiveId());
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

class NMOLQingjian : public TriggerSkill
{
public:
    NMOLQingjian() : TriggerSkill("nmolqingjian")
    {
        events << CardsMoveOneTime << EventPhaseChanging;
        view_as_skill = new NMOLQingjianViewAsSkill;
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
                if (xiahou->getPile("nmolqingjian").length() > 0 && TriggerSkill::triggerable(xiahou)) {
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
            room->setPlayerProperty(player, "nmolqingjian", IntList2StringList(ids).join("+"));
            const Card *card = room->askForCard(player, "@@nmolqingjian", "@nmolqingjian-put", data, Card::MethodNone);
            room->setPlayerProperty(player, "nmolqingjian", QString());
            if (card) {
                LogMessage log;
                log.type = "#InvokeSkill";
                log.arg = objectName();
                log.from = player;
                room->sendLog(log);
                room->notifySkillInvoked(player, objectName());
                player->broadcastSkillInvoke(objectName());
                player->addToPile("nmolqingjian", card, false);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive) return false;
            if (xiahou->getPile("nmolqingjian").length() > 0 && TriggerSkill::triggerable(xiahou)) {
                bool will_draw = xiahou->getPile("nmolqingjian").length() > 1;
                room->sendCompulsoryTriggerLog(xiahou, objectName());
                xiahou->broadcastSkillInvoke(objectName());
                QList<int> ids = xiahou->getPile("nmolqingjian");
                room->setPlayerProperty(xiahou, "nmolqingjian_give", IntList2StringList(ids).join("+"));
                do {
                    const Card *use = room->askForUseCard(xiahou, "@@nmolqingjian_allot!", "@nmolqingjian-give", QVariant(), Card::MethodNone);
                    ids = StringList2IntList(xiahou->property("nmolqingjian_give").toString().split("+"));
                    if (use == NULL) {
                        NMOLQingjianAllotCard *nmolqingjian_card = new NMOLQingjianAllotCard;
                        nmolqingjian_card->addSubcards(ids);
                        QList<ServerPlayer *> targets;
                        targets << room->getOtherPlayers(xiahou).first();
                        nmolqingjian_card->use(room, xiahou, targets);
                        delete nmolqingjian_card;
                        break;
                    }
                } while (!ids.isEmpty() && xiahou->isAlive());
                room->setPlayerProperty(xiahou, "nmolqingjian_give", QString());
                QList<CardsMoveStruct> moves;
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    QList<int> give_list = StringList2IntList(p->property("rende_give").toString().split("+"));
                    if (give_list.isEmpty())
                        continue;
                    room->setPlayerProperty(p, "rende_give", QString());
                    CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, p->objectName(), "nmolqingjian", QString());
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

class NMOLJizhi : public TriggerSkill
{
public:
    NMOLJizhi() : TriggerSkill("nmoljizhi")
    {
        frequency = Frequent;
        events << CardUsed << EventPhaseStart;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::NotActive) {
            QList<ServerPlayer *> all_players = room->getAllPlayers();
            foreach (ServerPlayer *p, all_players) {
                room->setPlayerMark(p, "#nmoljizhi", 0);
            }
        }
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent != CardUsed || !TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card && use.card->isNDTrick())
            return nameList();
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *yueying, QVariant &, ServerPlayer *) const
    {
        if (room->askForSkillInvoke(yueying, objectName())) {
            yueying->broadcastSkillInvoke(objectName());

            bool from_up = true;
            if (yueying->hasSkill("cunmu")) {
                room->sendCompulsoryTriggerLog(yueying, "cunmu");
                yueying->broadcastSkillInvoke("cunmu");
                from_up = false;
            }
            int id = room->drawCard(from_up);
            const Card *card = Sanguosha->getCard(id);
            CardMoveReason reason(CardMoveReason::S_REASON_DRAW, yueying->objectName(), objectName(), QString());
            room->obtainCard(yueying, card, reason, false);
            if (card->getTypeId() == Card::TypeBasic && yueying->handCards().contains(id)
                    && room->askForChoice(yueying, objectName(), "yes+no", QVariant::fromValue(card), "@nmoljizhi-discard:::" + card->objectName()) == "yes") {
                room->throwCard(card, yueying);
                room->addPlayerMark(yueying, "#nmoljizhi");
                room->addPlayerMark(yueying, "Global_MaxcardsIncrease");
            }
        }
        return false;
    }
};

class NMOLJieweiViewAsSkill : public OneCardViewAsSkill
{
public:
    NMOLJieweiViewAsSkill() : OneCardViewAsSkill("nmoljiewei")
    {
        filter_pattern = ".|.|.|equipped";
        response_or_use = true;
        response_pattern = "nullification";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Nullification *ncard = new Nullification(originalCard->getSuit(), originalCard->getNumber());
        ncard->addSubcard(originalCard);
        ncard->setSkillName(objectName());
        return ncard;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        return player->hasEquip();
    }
};

class NMOLJiewei : public TriggerSkill
{
public:
    NMOLJiewei() : TriggerSkill("nmoljiewei")
    {
        events << TurnedOver;
        view_as_skill = new NMOLJieweiViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->faceUp() && !target->isNude();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (room->askForCard(player, ".", "@jiewei", data, objectName()))
            room->askForUseCard(player, "@@jiewei_move", "@jiewei-move");
        return false;
    }
};

NMOLQiangxiCard::NMOLQiangxiCard()
{
}

bool NMOLQiangxiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty() || to_select == Self)
        return false;

    QStringList nmolqiangxi_prop = Self->property("nmolqiangxi").toString().split("+");
    if (nmolqiangxi_prop.contains(to_select->objectName()))
        return false;

    int rangefix = 0;
    if (!subcards.isEmpty() && Self->getWeapon() && Self->getWeapon()->getId() == subcards.first()) {
        const Weapon *card = qobject_cast<const Weapon *>(Self->getWeapon()->getRealCard());
        rangefix += card->getRange() - Self->getAttackRange(false);;
    }

    return Self->inMyAttackRange(to_select, rangefix);
}

void NMOLQiangxiCard::extraCost(Room *room, const CardUseStruct &card_use) const
{
    if (subcards.isEmpty())
        room->loseHp(card_use.from);
    else {
        CardMoveReason reason(CardMoveReason::S_REASON_THROW, card_use.from->objectName(), QString(), card_use.card->getSkillName(), QString());
        room->moveCardTo(this, NULL, Player::DiscardPile, reason, true);
    }
}

void NMOLQiangxiCard::onEffect(const CardEffectStruct &effect) const
{
    QSet<QString> nmolqiangxi_prop = effect.from->property("nmolqiangxi").toString().split("+").toSet();
    nmolqiangxi_prop.insert(effect.to->objectName());
    effect.from->getRoom()->setPlayerProperty(effect.from, "nmolqiangxi", QStringList(nmolqiangxi_prop.toList()).join("+"));
    effect.to->getRoom()->damage(DamageStruct("nmolqiangxi", effect.from, effect.to));
}

class NMOLQiangxiViewAsSkill : public ViewAsSkill
{
public:
    NMOLQiangxiViewAsSkill() : ViewAsSkill("nmolqiangxi")
    {
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return true;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.isEmpty() && to_select->isKindOf("Weapon") && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return new NMOLQiangxiCard;
        else if (cards.length() == 1) {
            NMOLQiangxiCard *card = new NMOLQiangxiCard;
            card->addSubcards(cards);

            return card;
        } else
            return NULL;
    }
};

class NMOLQiangxi : public TriggerSkill
{
public:
    NMOLQiangxi() : TriggerSkill("nmolqiangxi")
    {
        events << EventPhaseChanging;
        view_as_skill = new NMOLQiangxiViewAsSkill;
    }

    bool triggerable(const ServerPlayer *) const
    {
        return false;
    }

    void record(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (data.value<PhaseChangeStruct>().from == Player::Play) {
            room->setPlayerProperty(player, "nmolqiangxi", QVariant());
        }
    }
};

NMOLNiepanCard::NMOLNiepanCard()
{
    target_fixed = true;
}

void NMOLNiepanCard::extraCost(Room *room, const CardUseStruct &card_use) const
{
    room->removePlayerMark(card_use.from, "@nmolnirvana");
}

void NMOLNiepanCard::use(Room *room, ServerPlayer *pangtong, QList<ServerPlayer *> &) const
{
    doNiepan(room, pangtong);
}

void NMOLNiepanCard::doNiepan(Room *room, ServerPlayer *pangtong)
{
    pangtong->throwAllHandCardsAndEquips();
    QList<const Card *> tricks = pangtong->getJudgingArea();
    foreach (const Card *trick, tricks) {
        CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, pangtong->objectName());
        room->throwCard(trick, reason, NULL);
    }

    if (pangtong->isChained())
        room->setPlayerProperty(pangtong, "chained", false);

    if (!pangtong->faceUp())
        pangtong->turnOver();

    pangtong->drawCards(3, "nmolniepan");
    room->recover(pangtong, RecoverStruct(pangtong, NULL, 3 - pangtong->getHp()));
}

class NMOLNiepanViewAsSkill : public ZeroCardViewAsSkill
{
public:
    NMOLNiepanViewAsSkill() : ZeroCardViewAsSkill("nmolniepan")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@nmolnirvana") > 0;
    }

    virtual const Card *viewAs() const
    {
        return new NMOLNiepanCard;
    }
};

class NMOLNiepan : public TriggerSkill
{
public:
    NMOLNiepan() : TriggerSkill("nmolniepan")
    {
        events << AskForPeaches;
        frequency = Limited;
        limit_mark = "@nmolnirvana";
        view_as_skill = new NMOLNiepanViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *target, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(target) && target->getMark("@nmolnirvana") > 0) {
            DyingStruct dying_data = data.value<DyingStruct>();

            if (target->getHp() > 0)
                return QStringList();

            if (dying_data.who != target)
                return QStringList();
            return nameList();
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *pangtong, QVariant &, ServerPlayer *) const
    {
        if (pangtong->askForSkillInvoke(this)) {
            pangtong->broadcastSkillInvoke(objectName());

            room->removePlayerMark(pangtong, "@nmolnirvana");

            NMOLNiepanCard::doNiepan(room, pangtong);
        }

        return false;
    }
};

class NMOLLuanjiViewAsSkill : public ViewAsSkill
{
public:
    NMOLLuanjiViewAsSkill() : ViewAsSkill("nmolluanji")
    {
        response_or_use = true;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        QSet<QString> used = Self->property("nmolluanjiUsedSuits").toString().split("+").toSet();
        return selected.length() < 2 && !to_select->isEquipped() && !used.contains(to_select->getSuitString());
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 2) {
            ArcheryAttack *aa = new ArcheryAttack(Card::SuitToBeDecided, 0);
            aa->addSubcards(cards);
            aa->setSkillName(objectName());
            return aa;
        } else
            return NULL;
    }
};

class NMOLLuanji : public TriggerSkill
{
public:
    NMOLLuanji() : TriggerSkill("nmolluanji")
    {
        events << PreCardUsed << EventPhaseChanging << TargetSpecified << Damage << CardResponded << CardFinished;
        view_as_skill = new NMOLLuanjiViewAsSkill;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        switch (triggerEvent) {
        case PreCardUsed: {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card && use.card->isKindOf("ArcheryAttack") && use.card->getSkillName() == objectName()) {
                QSet<QString> used = player->property("nphyluanjiUsedSuits").toString().split("+").toSet();
                foreach (int id, use.card->getSubcards())
                    used.insert(Sanguosha->getCard(id)->getSuitString());
                room->setPlayerProperty(player, "nphyluanjiUsedSuits", QStringList(used.toList()).join("+"));
            }
            break;
        }
        case EventPhaseChanging: {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                room->setPlayerProperty(player, "nmolluanjiUsedSuits", QVariant());
            }
            break;
        }
        case TargetSpecified: {
            int cnt = 0;
            CardUseStruct use = data.value<CardUseStruct>();
            foreach (ServerPlayer *p, room->getAllPlayers())
                if (use.to.contains(p))
                    ++cnt;
            use.card->tag["nmolluanjiTargetsCount"] = cnt;
            break;
        }
        case Damage: {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("ArcheryAttack") && damage.card->getSkillName() == objectName()) {
                damage.card->tag["nmolluanjiDamageCount"] = damage.card->tag["nmolluanjiDamageCount"].toInt() + 1;
            }
            break;
        }
        default:
            break;
        }
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList list;
        if (triggerEvent == CardResponded) {
            CardResponseStruct res = data.value<CardResponseStruct>();
            CardEffectStruct effect = res.m_data.value<CardEffectStruct>();
            if (effect.card && effect.card->isKindOf("ArcheryAttack") && effect.card->getSkillName() == objectName()) {
                list.insert(player, comList());
            }
        } else if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card && use.card->isKindOf("ArcheryAttack") && use.card->getSkillName() == objectName()) {
                if (use.card->tag["nmolluanjiDamageCount"].toInt())
                    if (use.from)
                        list.insert(use.from, comList());
            }
        }
        return list;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *yuanshao) const
    {
        if (triggerEvent == CardResponded) {
            room->drawCards(player, 1, objectName());
        } else if (triggerEvent == CardFinished) {
            yuanshao->broadcastSkillInvoke(objectName());
            CardUseStruct use = data.value<CardUseStruct>();
            room->drawCards(yuanshao, use.card->tag["nmolluanjiTargetsCount"].toInt(), objectName());
        }
        return false;
    }
};

class NMOLXueyi : public MaxCardsSkill
{
public:
    NMOLXueyi() : MaxCardsSkill("nmolxueyi$")
    {
    }

    virtual int getExtra(const Player *target) const
    {
        if (target->hasLordSkill(this)) {
            int extra = 0;
            QList<const Player *> players = target->getAliveSiblings();
            foreach (const Player *player, players) {
                if (player->getKingdom() == "qun")
                    extra += 2;
            }
            return extra;
        } else
            return 0;
    }
};

class NMOLZaiqi : public TriggerSkill
{
public:
    NMOLZaiqi() : TriggerSkill("nmolzaiqi")
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
            room->setTag("NMOLZaiqiRecord", room->getTag("NMOLZaiqiRecord").toInt() + x);
        } else if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                room->removeTag("NMOLZaiqiRecord");
            }
        }
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseEnd && TriggerSkill::triggerable(player) &&
                player->getPhase() == Player::Discard && room->getTag("NMOLZaiqiRecord").toInt())
            return nameList();
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        int x = room->getTag("NMOLZaiqiRecord").toInt();
        QList<ServerPlayer *> targets = room->askForPlayersChosen(player, room->getAlivePlayers(), objectName(), 0, x, QString(), true);
        foreach (ServerPlayer *p, targets) {
            if (!player->isWounded() ||
                    room->askForChoice(p, objectName(), "draw+recover", QVariant::fromValue(player), "@nmolzaiqi-choose:" + player->objectName()) == "draw")
                room->drawCards(p, 1);
            else {
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, p->objectName(), player->objectName());
                room->recover(player, RecoverStruct(p));
            }
        }
        return false;
    }
};

NostalMOLPackage::NostalMOLPackage()
    : Package("nostalgia_mol")
{
    General *xiahoudun = new General(this, "nmol_xiahoudun", "wei", 4, true, true);
    xiahoudun->addSkill("ganglie");
    xiahoudun->addSkill(new NMOLQingjian);
    xiahoudun->addSkill(new DetachEffectSkill("nmolqingjian", "nmolqingjian"));
    related_skills.insertMulti("nmolqingjian", "#nmolqingjian-clear");

    General *huangyueying = new General(this, "nmol_huangyueying", "shu", 3, false, true);
    huangyueying->addSkill(new NMOLJizhi);
    huangyueying->addSkill("qicai");

    General *caoren = new General(this, "nmol_caoren", "wei", 4, true, true);
    caoren->addSkill("jushou");
    caoren->addSkill(new NMOLJiewei);

    General *dianwei = new General(this, "nmol_dianwei", "wei", 4, true, true);
    dianwei->addSkill(new NMOLQiangxi);

    General *pangtong = new General(this, "nmol_pangtong", "shu", 3, true, true);
    pangtong->addSkill("lianhuan");
    pangtong->addSkill("#lianhuan-target");
    pangtong->addSkill(new NMOLNiepan);

    General *wolong = new General(this, "nmol_wolong", "shu", 3, true, true);
    wolong->addSkill("bazhen");
    wolong->addSkill("huoji");
    wolong->addSkill("kanpo");

    General *yuanshao = new General(this, "nmol_yuanshao$", "qun", 4, true, true);
    yuanshao->addSkill(new NMOLLuanji);
    yuanshao->addSkill(new NMOLXueyi);

    General *menghuo = new General(this, "nmol_menghuo", "shu", 4, true, true);
    menghuo->addSkill(new NMOLZaiqi);
    menghuo->addSkill("huoshou");
    menghuo->addSkill("#sa_avoid_huoshou");

    addMetaObject<NMOLQingjianAllotCard>();
    addMetaObject<NMOLQiangxiCard>();
    addMetaObject<NMOLNiepanCard>();
    skills << new NMOLQingjianAllot;
}

ADD_PACKAGE(NostalMOL)
