#include "dragonboat.h"
#include "settings.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "clientplayer.h"
#include "engine.h"
#include "maneuvering.h"
#include "roomscene.h"
#include "special3v3.h"

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

class Lianpian : public TriggerSkill
{
public :
    Lianpian() : TriggerSkill("lianpian")
    {
        events << TargetSpecified << EventPhaseChanging;
        frequency = Frequent;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data) const
    {
        TriggerList list;
        if (target->getPhase() != Player::Play) return list;
        if (triggerEvent == TargetSpecified)
        {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.index)
                return list;
            if (!target->tag["lianpian-last"].isNull())
                foreach (ServerPlayer *to, use.to)
                    if (target->tag["lianpian-last"].value<QList<ServerPlayer *> >().contains(to))
                    {
                        foreach (ServerPlayer *player, room->getAllPlayers())
                            if (player->hasSkill(objectName()) && target->getKingdom() == player->getKingdom())
                                list.insert(player, nameList());
                        break;
                    }
        }
        return list;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging)
            foreach (ServerPlayer *p, room->getAllPlayers(true))
            {
                p->tag.remove("lianpian-last");
                p->tag.remove("lianpian-this");
            }
        else
        {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.index)
                return;
            player->tag["lianpian-last"] = player->tag["lianpian-this"];
            player->tag["lianpian-this"].setValue(use.to);
        }
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *p) const
    {
        if (room->askForSkillInvoke(p, objectName()))
        {
            room->broadcastSkillInvoke(objectName(), p);
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, p->objectName(), player->objectName());
            room->drawCards(player, 1, objectName());
        }
        return false;
    }
};

class Dianhu : public TriggerSkill
{
public :
    Dianhu() : TriggerSkill("dianhu")
    {
        events << Damage << TurnStart;
        frequency = Compulsory;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data) const
    {
        TriggerList list;
        if (triggerEvent == TurnStart)
        {
            if (room->getTag("FirstRound").toBool())
                foreach (ServerPlayer *quan, room->getAllPlayers())
                    if (TriggerSkill::triggerable(quan))
                         list.insert(quan, nameList());
        }
        else
        {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.to->getMark("@dianhu"))
                foreach (ServerPlayer *quan, room->getAllPlayers())
                    if (quan->hasSkill(objectName()) && target->getKingdom() == quan->getKingdom())
                    {
                        QStringList ans;
                        for (int i = 0; i < damage.damage; i++)
                            ans << objectName();
                        list.insert(quan, ans);
                    }
        }
        return list;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *quan) const
    {
        if (triggerEvent == TurnStart)
        {
            room->broadcastSkillInvoke(objectName(), quan, 1);
            ServerPlayer *target = room->askForPlayerChosen(quan, room->getOtherPlayers(quan), objectName());
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, quan->objectName(), target->objectName());
            room->setPlayerMark(target, "@dianhu", 1);
        }
        else
        {
            room->broadcastSkillInvoke(objectName(), quan, 2);
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, quan->objectName(), player->objectName());
            room->drawCards(player, 1, objectName());
        }
        return false;
    }
};

JianjiCard::JianjiCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool JianjiCard::targetFixed() const
{
    return true;
}

void JianjiCard::onUse(Room *room, const CardUseStruct &use) const
{
    foreach (ServerPlayer *target, room->getOtherPlayers(use.from))
        if (target->getKingdom() == use.from->getKingdom())
        {
            int card_id = room->getNCards(1, false).first();
            Card *card = Sanguosha->getCard(card_id);
            //target->drawCard(card);
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, use.from->objectName(), target->objectName());
            room->broadcastSkillInvoke("jianji", use.from);
            room->obtainCard(target, card, true);
            room->askForUseCard(target, QString::number(card_id), "@jianji-use:::"+card->objectName());
            room->setPlayerFlag(use.from, "JianjiUsed");
        }

}

class Jianji : public ZeroCardViewAsSkill
{
public:
    Jianji() : ZeroCardViewAsSkill("jianji")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->hasSkill(objectName()) && !player->hasFlag("JianjiUsed");
    }

    virtual const Card *viewAs() const
    {
        JianjiCard *jianjiCard = new JianjiCard;
        return jianjiCard;
    }
};

class Xingzhao : public TriggerSkill
{
public:
    Xingzhao() : TriggerSkill("xingzhao")
    {
        events << Death << EventAcquireSkill << EventLoseSkill << MarkChanged << CardUsed << EventPhaseChanging;
        frequency = Compulsory;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data) const
    {
        TriggerList list;
        if (triggerEvent == Death)
        {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who->hasSkill(objectName()))
            {
                foreach (ServerPlayer *p, room->getOtherPlayers(death.who))
                    if (p->getKingdom() == death.who->getKingdom() && p->hasSkill(objectName()))
                        return list;
                foreach (ServerPlayer *p, room->getAllPlayers())
                    if (p->getKingdom() == death.who->getKingdom() && p->hasSkill("xunxun") && p->getMark("gained_xunxun"))
                        list.insert(p, nameList());
            }
        }
        else if (triggerEvent == EventLoseSkill)
        {
            if (data.toString() == objectName())
            {
                foreach (ServerPlayer *p, room->getOtherPlayers(target))
                    if (p->getKingdom() == target->getKingdom() && p->hasSkill(objectName()))
                        return list;
                foreach (ServerPlayer *p, room->getAllPlayers())
                    if (p->getKingdom() == target->getKingdom() && p->hasSkill("xunxun") && p->getMark("gained_xunxun"))
                        list.insert(target, nameList());
            }
            else if (data.toString() == "xunxun")
            {
                if (room->getBoatTreasure(target->getKingdom()) > 1)
                    foreach (ServerPlayer *tangzi, room->getAllPlayers())
                        if (tangzi->hasSkill(objectName()) && tangzi->getKingdom() == target->getKingdom())
                            if (tangWillEffect(tangzi, room))
                                list.insert(tangzi, nameList());
            }
        }
        else if (triggerEvent == EventAcquireSkill)
        {
            if (data.toString() == objectName() && target->isAlive() && room->getBoatTreasure(target->getKingdom()) > 1)
            {
                foreach (ServerPlayer *p, room->getOtherPlayers(target))
                    if (p->getKingdom() == target->getKingdom() && p->hasSkill(objectName()))
                        return list;
                if (tangWillEffect(target, room))
                    list.insert(target, nameList());
            }
        }
        else if (triggerEvent == MarkChanged)
        {
            MarkStruct mark = data.value<MarkStruct>();
            if (mark.name == "@boattreasure" && room->getBoatTreasure(target->getKingdom()) > 1)
                foreach (ServerPlayer *tangzi, room->getAllPlayers())
                    if (tangzi->hasSkill(objectName()) && tangzi->getKingdom() == target->getKingdom())
                        if (tangWillEffect(tangzi, room))
                            list.insert(tangzi, nameList());
        }
        else if (triggerEvent == CardUsed)
        {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("EquipCard"))
                foreach (ServerPlayer *p, room->getAllPlayers())
                    if (p->hasSkill(objectName()) && p->getKingdom() == target->getKingdom() && room->getBoatTreasure(p->getKingdom()) > 3)
                        list.insert(p, nameList());
        }
        else if (triggerEvent == EventPhaseChanging)
        {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Discard && target->isAlive())
                foreach (ServerPlayer *p, room->getAllPlayers())
                    if (p->hasSkill(objectName()) && p->getKingdom() == target->getKingdom() && room->getBoatTreasure(p->getKingdom()) > 5)
                        list.insert(p, nameList());
        }
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *target) const
    {
        if (triggerEvent == Death)
        {
            room->detachSkillFromPlayer(target, "xunxun");
            room->setPlayerMark(target, "gained_xunxun", 0);
        }
        else if (triggerEvent == EventLoseSkill)
        {
            if (data.toString() == objectName())
            {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    if (p->getKingdom() == target->getKingdom() && p->hasSkill("xunxun") && p->getMark("gained_xunxun"))
                    {
                        room->detachSkillFromPlayer(p, "xunxun");
                        room->setPlayerMark(p, "gained_xunxun", 0);
                    }
            }
            else if (data.toString() == "xunxun")
            {
                tangEffect(target, room);
            }
        }
        else if (triggerEvent == EventAcquireSkill)
        {
            tangEffect(target, room);
        }
        else if (triggerEvent == MarkChanged)
        {
            tangEffect(target, room);
        }
        else if (triggerEvent == CardUsed)
        {
            room->broadcastSkillInvoke(objectName(), target);
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, target->objectName(), player->objectName());
            room->drawCards(player, 1, objectName());
        }
        else if (triggerEvent == EventPhaseChanging)
        {
            room->broadcastSkillInvoke(objectName(), target);
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, target->objectName(), player->objectName());
            player->skip(Player::Discard);
        }
        return false;
    }
protected:
    virtual void tangEffect(ServerPlayer *tangzi, Room *room) const
    {
        if (!tangWillEffect(tangzi, room))
            return;
        room->broadcastSkillInvoke("xingzhao", tangzi);
        foreach (ServerPlayer *p, room->getAllPlayers(true))
            if (p->getKingdom() == tangzi->getKingdom() && !p->hasSkill("xunxun"))
            {
                room->acquireSkill(p, "xunxun");
                room->setPlayerMark(p, "gained_xunxun", 1);
            }
    }

    virtual bool tangWillEffect(ServerPlayer *tangzi, Room *room) const
    {
        foreach (ServerPlayer *p, room->getAllPlayers(true))
            if (p->getKingdom() == tangzi->getKingdom() && !p->hasSkill("xunxun"))
                return true;
        return false;
    }
};

class DWenji : public TriggerSkill
{
public:
    DWenji() : TriggerSkill("d_wenji")
    {
        events << EventPhaseStart << CardUsed;
        view_as_skill = new dummyVS;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&ask_who) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Play && TriggerSkill::triggerable(player)) {
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (!p->isNude() && p->getKingdom() == player->getKingdom())
                        return nameList();
                }
            }
        }
        else if (triggerEvent == CardUsed)
        {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") || use.card->isNDTrick()) {
                QStringList wenji_list = player->tag[objectName()].toStringList();

                QString classname = use.card->getClassName();
                if (use.card->isKindOf("Slash")) classname = "Slash";
                if (wenji_list.contains(classname))
                    return nameList();
            }
        }
        return QStringList();
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::NotActive) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                p->tag.remove(objectName());
            }
        }
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (!p->isNude() && p->getKingdom() == player->getKingdom())
                    targets << p;
            }
            if (targets.isEmpty()) return false;

            if (room->askForSkillInvoke(player, objectName()))
                foreach (ServerPlayer *target, targets) {
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());
                    player->broadcastSkillInvoke(objectName());
                    const Card *card = room->askForExchange(target, objectName(), 1, 1, true, "@wenji-give:" + player->objectName());

                    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, target->objectName(), player->objectName(), objectName(), QString());
                    reason.m_playerId = player->objectName();
                    room->moveCardTo(card, player, Player::PlaceHand, reason, true);


                    const Card *record_card = Sanguosha->getCard(card->getEffectiveId());

                    if (!player->getHandcards().contains(record_card)) return false;

                    QString classname = record_card->getClassName();
                    if (record_card->isKindOf("Slash")) classname = "Slash";
                    QStringList list = player->tag[objectName()].toStringList();
                    list.append(classname);
                    player->tag[objectName()] = list;

                }
        } else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") || use.card->isNDTrick()) {
                QStringList wenji_list = player->tag[objectName()].toStringList();

                QString classname = use.card->getClassName();
                if (use.card->isKindOf("Slash")) classname = "Slash";
                if (!wenji_list.contains(classname)) return false;


                QStringList fuji_tag = use.card->tag["Fuji_tag"].toStringList();

                QList<ServerPlayer *> players = room->getOtherPlayers(player);
                foreach (ServerPlayer *p, players) {
                    fuji_tag << p->objectName();
                }
                use.card->setTag("Fuji_tag", fuji_tag);
            }
        }

        return false;
    }
};

class DTunjiang : public PhaseChangeSkill
{
public:
    DTunjiang() : PhaseChangeSkill("d_tunjiang")
    {

    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList list;
        if (player->getPhase() != Player::Finish) return list;
        if (player->hasFlag("TunjiangDisabled") || player->hasSkipped(Player::Play)) return list;
        if (player->getMark("damage_point_play_phase") > 0) return list;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->getKingdom() == player->getKingdom() && TriggerSkill::triggerable(p))
                list.insert(p, nameList());
        }
        return list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *player) const
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->getKingdom() == player->getKingdom())
                targets << p;
        }
        if (targets.isEmpty()) return false;
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), QString(), true, true);
        if (target) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());
            player->broadcastSkillInvoke(objectName());
            target->drawCards(2, objectName());
        }
        return false;
    }
};

Zong::Zong(Suit suit, int number) : BasicCard(suit, number)
{
    setObjectName("zong");
    target_fixed = false;
}

QString Zong::getSubtype() const
{
    return "recover_card";
}

bool Zong::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *source) const
{
    return targets.length() < 2 && to_select->isWounded() && to_select->getKingdom() == source->getKingdom();
}

void Zong::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    room->recover(effect.to, RecoverStruct(effect.from, this));
}

bool Zong::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return !targets.isEmpty();
}

RealgarAnaleptic::RealgarAnaleptic(Card::Suit suit, int number)
    : BasicCard(suit, number)
{
    setObjectName("realgar_analeptic");
    target_fixed = true;
}

QString RealgarAnaleptic::getSubtype() const
{
    return "buff_card";
}

bool RealgarAnaleptic::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.isEmpty();
}

void RealgarAnaleptic::onUse(Room *room, const CardUseStruct &card_use) const
{
    room->setCardEmotion(card_use.from, this);
    BasicCard::onUse(room, card_use);
}

void RealgarAnaleptic::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    room->addPlayerMark(effect.to, "drank");
}

QList<ServerPlayer *> RealgarAnaleptic::defaultTargets(Room *, ServerPlayer *source) const
{
    return QList<ServerPlayer *>() << source;
}

SameDragon::SameDragon(Card::Suit suit, int number)
    : TrickCard(suit, number)
{
    setObjectName("same_dragon");
    target_fixed = true;
}

QString SameDragon::getSubtype() const
{
    return "double_target_trick";
}

bool SameDragon::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.isEmpty();
}

void SameDragon::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, effect.from->objectName(), effect.to->objectName());
    room->drawCards(effect.to, getDrawNum(effect.to));
}

int SameDragon::getDrawNum(ServerPlayer *player) const
{
    QString kingdom = player->getKingdom();
    Room *room = player->getRoom();
    if (room->getBoatTreasure(kingdom) == 0)
        return 1;
    QStringList kingdoms;
    kingdoms << "wei" << "shu" << "wu" << "qun";
    kingdoms.removeOne(kingdom);
    foreach (QString ekingdom, kingdoms)
        if (room->getBoatTreasure(ekingdom) > room->getBoatTreasure(kingdom))
            return 1;
    return 2;
}

QList<ServerPlayer *> SameDragon::defaultTargets(Room *room, ServerPlayer *source) const
{
    QList<ServerPlayer *> targets;
    foreach (ServerPlayer *p, room->getAllPlayers())
        if (p->getKingdom() == source->getKingdom())
            targets << p;
    return targets;
}

FightforUpstream::FightforUpstream(Card::Suit suit, int number)
    : GlobalEffect(suit, number)
{
    setObjectName("fightfor_upstream");
}

void FightforUpstream::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    if (effect.to->objectName() == effect.from->objectName())
        room->drawCards(effect.from, getDrawNum(effect.from));
    room->drawCards(effect.to, 1);
}

int FightforUpstream::getDrawNum(ServerPlayer *player) const
{
    QString kingdom = player->getKingdom();
    Room *room = player->getRoom();
    if (room->getBoatTreasure(kingdom) > 5)
        return 5;
    return room->getBoatTreasure(kingdom);
}

AgainstWater::AgainstWater(Card::Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("against_water");
}

bool AgainstWater::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->getKingdom() != Self->getKingdom();
}

void AgainstWater::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    room->damage(DamageStruct(this, effect.from, effect.to, 1));

    bool drawone = true, discone = true;
    QString kingdom = effect.from->getKingdom();
    if (room->getBoatTreasure(kingdom) == 0)
        discone = false;
    QStringList kingdoms;
    kingdoms << "wei" << "shu" << "wu" << "qun";
    kingdoms.removeOne(kingdom);
    foreach (QString ekingdom, kingdoms)
    {
        if (room->getBoatTreasure(ekingdom) > room->getBoatTreasure(kingdom))
            discone = false;
        else if (room->getBoatTreasure(ekingdom) < room->getBoatTreasure(kingdom))
            drawone = false;
    }
    if (discone)
        room->askForDiscard(effect.from, objectName(), 1, 1, false, true);
    if (drawone)
        room->drawCards(effect.from, 1, objectName());
}

DragonBoatPackage::DragonBoatPackage()
    : Package("DragonBoat")
{
    General *qun_sufei = new General(this, "sufei", "qun", 4, true, true);
    qun_sufei->addSkill(new Lianpian);

    General *wu_sufei = new General(this, "wu_sufei", "wu", 4, true, true);
    wu_sufei->addSkill("lianpian");

    General *shu_huangquan = new General(this, "huangquan", "shu", 3, true, true);
    shu_huangquan->addSkill(new Dianhu);
    shu_huangquan->addSkill(new Jianji);

    General *wei_huangquan = new General(this, "wei_huangquan", "wei", 3, true, true);
    wei_huangquan->addSkill("dianhu");
    wei_huangquan->addSkill("jianji");

    General *wei_tangzi = new General(this, "tangzi", "wei", 4, true, true);
    wei_tangzi->addSkill(new Xingzhao);
    wei_tangzi->addRelateSkill("xunxun");

    General *wu_tangzi = new General(this, "wu_tangzi", "wu", 4, true, true);
    wu_tangzi->addSkill("xingzhao");
    wu_tangzi->addRelateSkill("xunxun");

    General *d_liuqi = new General(this, "d_liuqi", "shu", 3, true, true);
    d_liuqi->addSkill(new DWenji);
    d_liuqi->addSkill(new DTunjiang);

    General *qd_liuqi = new General(this, "qun_d_liuqi", "qun", 3, true, true);
    qd_liuqi->addSkill("d_wenji");
    qd_liuqi->addSkill("d_tunjiang");

    addMetaObject<JianjiCard>();
}

DragonBoatCardPackage::DragonBoatCardPackage()
    : Package("DragonBoatCard", Package::CardPack)
{
    QList<Card *> cards;

    cards << new Zong(Card::Heart, 3)
          << new Zong(Card::Heart, 4)
          << new Zong(Card::Heart, 6)
          << new Zong(Card::Heart, 7)
          << new Zong(Card::Heart, 8)
          << new Zong(Card::Heart, 9)
          << new Zong(Card::Heart, 12)
          << new Zong(Card::Diamond, 12)
          << new Zong(Card::Heart, 5)
          << new Zong(Card::Heart, 6)
          << new Zong(Card::Diamond, 2)
          << new Zong(Card::Diamond, 3);

    cards << new RealgarAnaleptic(Card::Spade, 3)
          << new RealgarAnaleptic(Card::Spade, 9)
          << new RealgarAnaleptic(Card::Club, 3)
          << new RealgarAnaleptic(Card::Club, 9)
          << new RealgarAnaleptic(Card::Diamond, 9);

    cards << new SameDragon(Card::Heart, 7)
          << new SameDragon(Card::Heart, 8)
          << new SameDragon(Card::Heart, 9)
          << new SameDragon(Card::Heart, 11);

    cards << new FightforUpstream(Card::Heart, 1)
          << new FightforUpstream(Card::Heart, 3)
          << new FightforUpstream(Card::Heart, 4);

    cards << new AgainstWater(Card::Spade, 7)
          << new AgainstWater(Card::Spade, 13)
          << new AgainstWater(Card::Club, 7)
          << new AgainstWater(Card::Heart, 1);

    cards << new VSCrossbow(Card::Club, 1)
          << new VSCrossbow(Card::Diamond, 1);

    foreach (Card *card, cards)
        card->setParent(this);
}

ADD_PACKAGE(DragonBoat)
ADD_PACKAGE(DragonBoatCard)
