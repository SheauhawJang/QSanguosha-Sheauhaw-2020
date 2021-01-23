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

class AiyeNerverDie : public TriggerSkill
{
public :
    AiyeNerverDie() : TriggerSkill("#aiyeneverdie")
    {
        events << Dying;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data) const
    {
        foreach (ServerPlayer *player, room->getAllPlayers())
            if (player->getHp() <= 0)
            {
                room->sendCompulsoryTriggerLog(player, "aiye");
                room->notifySkillInvoked(player, "aiye");
                room->recover(player, RecoverStruct(player, NULL, 1 - player->getHp()));
            }
        return false;
    }

    virtual int getPriority(TriggerEvent triggerEvent) const
    {
        return 100;
    }
};

class AiyeGainMark : public TriggerSkill
{
public :
    AiyeGainMark() : TriggerSkill("#aiyegainmark")
    {
        events << Damage;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        QString kingdom = player->getKingdom();
        if (damage.to->getKingdom() == kingdom)
            return false;
        QStringList kingdoms;
        kingdoms << "wei" << "shu" << "wu" << "qun";
        bool twt = true;
        bool tot = true;
        if (room->getBoatTreasure(kingdom) != 12)
        {
            foreach (QString kingdomeach, kingdoms)
            {
                if (kingdomeach == kingdom) continue;
                if (room->getBoatTreasure(kingdomeach) == room->getBoatTreasure(kingdom))
                    foreach (ServerPlayer *other, room->getOtherPlayers(player, true))
                        if (other->getMark("boat2") > player->getMark("boat2"))
                            room->addPlayerMark(other, "boat2", -1);
                room->setPlayerMark(player, "boat2", 0);
                foreach (ServerPlayer *mate, room->getOtherPlayers(player, true))
                    if (mate->getKingdom() == kingdom)
                        room->setPlayerMark(mate, "boat2", 0);
            }
            twt = false;
        }
        if (room->getBoatTreasure(kingdom) < 2)
            tot = false;
        if (room->getBoatTreasure(kingdom) + damage.damage <= 12)
            player->gainMark("@boattreasure", damage.damage);
        else if (room->getBoatTreasure(kingdom) < 12)
            player->gainMark("@boattreasure", 12 - room->getBoatTreasure(kingdom));
        foreach (QString kingdomeach, kingdoms)
        {
            if (kingdomeach == kingdom) continue;
            if (room->getBoatTreasure(kingdomeach) == room->getBoatTreasure(kingdom))
            {
                room->addPlayerMark(player, "boat2", 1);
                foreach (ServerPlayer *mate, room->getOtherPlayers(player, true))
                    if (mate->getKingdom() == kingdom)
                        room->addPlayerMark(mate, "boat2", 1);
            }
        }
        if (room->getBoatTreasure(kingdom) >= 2 && !tot)
            foreach (ServerPlayer *tangzi, room->getAllPlayers(true))
                if (tangzi->hasSkill("xingzhao") && tangzi->getKingdom() == kingdom)
                {
                    room->broadcastSkillInvoke("xingzhao", tangzi);
                    foreach (ServerPlayer *p, room->getAllPlayers(true))
                        if (p->getKingdom() == kingdom && !p->hasSkill("xunxun"))
                        {
                            room->acquireSkill(p, "xunxun");
                            room->setPlayerMark(p, "gained_xunxun", 1);
                        }
                }
        if (!twt) room->speakRanks();
        return false;
    }
};

class DragonBoatGameover : public TriggerSkill
{
public :
    DragonBoatGameover() : TriggerSkill("dragonboat_over")
    {
        events << RoundStart;
        global = true;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (room->getTurn() < 3 || room->getMode() != "08_dragonboat") return false;
        foreach (ServerPlayer *tangzi, room->getAllPlayers())
            if (tangzi->hasSkill("xingzhao") && tangzi->getMark("boat2") && room->getBoatTreasure(tangzi->getKingdom()) == 12)
            {
                room->broadcastSkillInvoke("xingzhao", tangzi);
                QStringList kingdoms;
                kingdoms << "wei" << "shu" << "wu" << "qun";
                QString tKingdom = tangzi->getKingdom();
                foreach (QString kingdomeach, kingdoms)
                {
                    if (kingdomeach == tKingdom) continue;
                    if (room->getBoatTreasure(kingdomeach) == room->getBoatTreasure(tKingdom))
                        foreach (ServerPlayer *other, room->getOtherPlayers(tangzi, true))
                            if (other->getMark("boat2") < tangzi->getMark("boat2"))
                                room->addPlayerMark(other, "boat2", 1);
                    room->setPlayerMark(tangzi, "boat2", 0);
                    foreach (ServerPlayer *mate, room->getOtherPlayers(tangzi, true))
                        if (mate->getKingdom() == tKingdom)
                            room->setPlayerMark(mate, "boat2", 0);
                }
            }
        room->speakRanks(true);
        QString winkingdom = room->getRankKingdom(1);
        if (winkingdom == "ZeroKingdom")
            room->gameOver(".");
        else
        {
            QString winner = "dragon_" + winkingdom;
            room->gameOver(winner);
        }
        return false;
    }
};

class Lianpian : public TriggerSkill
{
public :
    Lianpian() : TriggerSkill("lianpian")
    {
        events << TargetSpecified << EventPhaseEnd;
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *target, Room *room) const
    {
        if (target->hasSkill(objectName()))
            return true;
        foreach (ServerPlayer *player, room->getAllPlayers())
            if (player->hasSkill(objectName()) && target->getKingdom() == player->getKingdom())
                return true;
        return false;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->getPhase() != Player::Play) return false;
        if (triggerEvent == TargetSpecified)
        {
            CardUseStruct use = data.value<CardUseStruct>();
            bool oncedraw = false;
            foreach (ServerPlayer *useto, use.to)
            {
                if (useto->getMark("lianpianuseto"))
                    foreach (ServerPlayer *p, room->getAllPlayers())
                        if (p->hasSkill(objectName()) && p->getKingdom() == player->getKingdom())
                            if (room->askForSkillInvoke(p, objectName()))
                            {
                                room->broadcastSkillInvoke(objectName(), p);
                                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, p->objectName(), player->objectName());
                                room->drawCards(player, 1, objectName());
                                oncedraw = true;
                            }
                if (oncedraw) break;
            }
            foreach (ServerPlayer *willclean, room->getAllPlayers(true))
                room->setPlayerMark(willclean, "lianpianuseto", 0);
            foreach (ServerPlayer *useto, use.to)
                room->setPlayerMark(useto, "lianpianuseto", 1);
        }
        else
            foreach (ServerPlayer *willclean, room->getAllPlayers(true))
                room->setPlayerMark(willclean, "lianpianuseto", 0);
        return false;
    }
};

class Dianhu : public TriggerSkill
{
public :
    Dianhu() : TriggerSkill("dianhu")
    {
        events << Damage;
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *target, Room *room) const
    {
        if (target->hasSkill(objectName()))
            return true;
        foreach (ServerPlayer *player, room->getAllPlayers())
            if (player->hasSkill(objectName()) && target->getKingdom() == player->getKingdom())
                return true;
        return false;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to->getMark("@dianhu"))
            foreach (ServerPlayer *p, room->getAllPlayers())
                if (p->hasSkill(objectName()) && p->getKingdom() == player->getKingdom())
                    if (room->askForSkillInvoke(p, objectName()))
                    {
                        room->broadcastSkillInvoke(objectName(), p, 2);
                        for (int i = 0; i < damage.damage; i++)
                        {
                            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, p->objectName(), player->objectName());
                            room->drawCards(player, 1, objectName());
                        }
                    }
        return false;
    }
};

class DianhuGainMark : public TriggerSkill
{
public :
    DianhuGainMark() : TriggerSkill("#dianhu-gainmark")
    {
        events << TurnStart;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target, Room *room) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (room->getTag("FirstRound").toBool())
            foreach (ServerPlayer *p, room->getAllPlayers())
                if (p->hasSkills("dianhu"))
                {
                    room->broadcastSkillInvoke("dianhu", p, 1);
                    ServerPlayer *target = room->askForPlayerChosen(p, room->getOtherPlayers(p), "dianhu");
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, p->objectName(), target->objectName());
                    room->setPlayerMark(target, "@dianhu", 1);
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
            QList<int> card_ids = room->getNCards(1, false);
            int card_id = card_ids.first();
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
        events << CardUsed << EventPhaseChanging;
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *target, Room *room) const
    {
        if (target->hasSkill(objectName()))
            return true;
        foreach (ServerPlayer *player, room->getAllPlayers())
            if (player->hasSkill(objectName()) && target->getKingdom() == player->getKingdom())
                return true;
        return false;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardUsed)
        {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("EquipCard"))
                foreach (ServerPlayer *p, room->getAllPlayers())
                    if (p->hasSkill(objectName()) && p->getKingdom() == player->getKingdom() && room->getBoatTreasure(p->getKingdom()) > 3)
                        if (room->askForSkillInvoke(p, objectName()))
                        {
                            room->broadcastSkillInvoke(objectName(), p);
                            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, p->objectName(), player->objectName());
                            room->drawCards(player, 1, objectName());
                        }
        }
        else if (triggerEvent == EventPhaseChanging)
        {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::Discard && player->isAlive())
                foreach (ServerPlayer *p, room->getAllPlayers())
                    if (p->hasSkill(objectName()) && p->getKingdom() == player->getKingdom() && room->getBoatTreasure(p->getKingdom()) > 5)
                        if (room->askForSkillInvoke(p, objectName()))
                        {
                            room->broadcastSkillInvoke(objectName(), p);
                            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, p->objectName(), player->objectName());
                            player->skip(Player::Discard);
                            return false;
                        }
        }
        return false;
    }
};

class XingzhaoGlobal : public TriggerSkill
{
public:
    XingzhaoGlobal() : TriggerSkill("#xingzhao-global")
    {
        events << Death << EventAcquireSkill << EventLoseSkill;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target, Room *room) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == Death)
        {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who->hasSkill("xingzhao"))
            {
                foreach (ServerPlayer *p, room->getOtherPlayers(death.who, false))
                    if (p->getKingdom() == death.who->getKingdom() && p->hasSkill("xingzhao"))
                        return false;
                foreach (ServerPlayer *p, room->getAllPlayers(true))
                    if (p->getKingdom() == death.who->getKingdom() && p->hasSkill("xunxun") && p->getMark("gained_xunxun"))
                    {
                        room->detachSkillFromPlayer(p, "xunxun");
                        room->setPlayerMark(p, "gained_xunxun", 0);
                    }
            }
        }
        else if (triggerEvent == EventAcquireSkill)
        {
            if (data.toString() == "xingzhao" && player->isAlive() && room->getBoatTreasure(player->getKingdom()) > 1)
            {
                foreach (ServerPlayer *p, room->getOtherPlayers(player, false))
                    if (p->getKingdom() == player->getKingdom() && p->hasSkill("xingzhao"))
                        return false;
                foreach (ServerPlayer *p, room->getAllPlayers(true))
                {
                    room->broadcastSkillInvoke("xingzhao", player);
                    if (p->getKingdom() == player->getKingdom() && !p->hasSkill("xunxun"))
                    {
                        room->acquireSkill(p, "xunxun");
                        room->setPlayerMark(p, "gained_xunxun", 1);
                    }
                }
            }
        }
        else if (triggerEvent == EventLoseSkill)
        {
            if (data.toString() == "xingzhao")
            {
                foreach (ServerPlayer *p, room->getOtherPlayers(player, false))
                    if (p->getKingdom() == player->getKingdom() && p->hasSkill("xingzhao"))
                        return false;
                foreach (ServerPlayer *p, room->getAllPlayers(true))
                    if (p->getKingdom() == player->getKingdom() && p->hasSkill("xunxun") && p->getMark("gained_xunxun"))
                    {
                        room->detachSkillFromPlayer(p, "xunxun");
                        room->setPlayerMark(p, "gained_xunxun", 0);
                    }
            }
            else if (data.toString() == "xunxun")
            {
                if (room->getBoatTreasure(player->getKingdom()) > 1)
                    foreach (ServerPlayer *tangzi, room->getAllPlayers(true))
                        if (tangzi->hasSkill("xingzhao") && tangzi->getKingdom() == player->getKingdom())
                        {
                            room->broadcastSkillInvoke("xingzhao", tangzi);
                            foreach (ServerPlayer *p, room->getAllPlayers(true))
                                if (p->getKingdom() == tangzi->getKingdom() && !p->hasSkill("xunxun"))
                                {
                                    room->acquireSkill(p, "xunxun");
                                    room->setPlayerMark(p, "gained_xunxun", 1);
                                }
                        }
            }
        }
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

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Play && TriggerSkill::triggerable(player)) {
                QList<ServerPlayer *> targets;
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (!p->isNude() && p->getKingdom() == player->getKingdom())
                        targets << p;
                }
                if (targets.isEmpty()) return false;
                ServerPlayer *target;
                if (targets.length() == 1)
                {
                    if (room->askForSkillInvoke(player, objectName()));
                        target = targets.first();
                }
                else
                {
                    ServerPlayer *temptarget = room->askForPlayerChosen(player, targets, objectName(), "@d_wenji-invoke", true, true);
                    target = temptarget;
                }
                if (target) {
                    room->broadcastSkillInvoke(objectName(), player);
                    const Card *card = room->askForExchange(target, objectName(), 1, 1, true, "@d_wenji-give:" + player->objectName());

                    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, target->objectName(), player->objectName(), objectName(), QString());
                    reason.m_playerId = player->objectName();
                    room->moveCardTo(card, target, player, Player::PlaceHand, reason, true);


                    const Card *record_card = Sanguosha->getCard(card->getEffectiveId());

                    if (!player->getHandcards().contains(record_card)) return false;

                    QString classname = record_card->getClassName();
                    if (record_card->isKindOf("Slash")) classname = "Slash";
                    QStringList list = player->tag[objectName()].toStringList();
                    list.append(classname);
                    player->tag[objectName()] = list;

                }
            } else if (player->getPhase() == Player::NotActive) {
                QList<ServerPlayer *> players = room->getAlivePlayers();
                foreach (ServerPlayer *p, players) {
                    p->tag.remove(objectName());
                }
            }
        } else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") || use.card->isNDTrick()) {
                QStringList d_wenji_list = player->tag[objectName()].toStringList();

                QString classname = use.card->getClassName();
                if (use.card->isKindOf("Slash")) classname = "Slash";
                if (!d_wenji_list.contains(classname)) return false;


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

    virtual bool triggerable(const ServerPlayer *target, Room *room) const
    {
        if (target->hasSkill(objectName()))
            return true;
        foreach (ServerPlayer *player, room->getAllPlayers())
            if (player->hasSkill(objectName()) && target->getKingdom() == player->getKingdom())
                return true;
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        if (player->getPhase() != Player::Finish) return false;
        if (player->hasFlag("DTunjiangDisabled")) return false;
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAllPlayers())
            if (p->getKingdom() == player->getKingdom())
                targets << p;
        foreach (ServerPlayer *p, room->getAllPlayers())
            if (p->hasSkill(objectName()) && p->getKingdom() == player->getKingdom())
            {
                ServerPlayer *target = room->askForPlayerChosen(p, targets, objectName(), "@d_tunjiang-invoke", true, true);
                if (target) {
                    room->broadcastSkillInvoke(objectName(), p);
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, p->objectName(), target->objectName());
                    target->drawCards(2, objectName());
                }
            }
    }
};

class DTunjiangRecord : public TriggerSkill
{
public:
    DTunjiangRecord() : TriggerSkill("#d_tunjiang-record")
    {
        events << Damage;
        global = true;
    }

    virtual int getPriority(TriggerEvent) const
    {
        return 6;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (player->getPhase() == Player::NotActive) return false;

        foreach (ServerPlayer *p, room->getAllPlayers())
            if (p->hasSkill("d_tunjiang") && p->getKingdom() == player->getKingdom())
                if (damage.from == player && damage.to && damage.damage > 0)
                    room->setPlayerFlag(p, "DTunjiangDisabled");
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

bool SameDragon::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
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
    General *wu_sufei = new General(this, "wu_sufei", "wu", 4, true, true);
    wu_sufei->addSkill(new Lianpian);

    General *qun_sufei = new General(this, "sufei", "qun", 4, true, true);
    qun_sufei->addSkill("lianpian");

    General *shu_huangquan = new General(this, "huangquan", "shu", 3, true, true);
    shu_huangquan->addSkill(new Dianhu);
    shu_huangquan->addSkill(new DianhuGainMark);
    shu_huangquan->addSkill(new Jianji);

    General *wei_huangquan = new General(this, "wei_huangquan", "wei", 3, true, true);
    wei_huangquan->addSkill("dianhu");
    wei_huangquan->addSkill("#dianhu-gainmark");
    wei_huangquan->addSkill("jianji");

    General *wei_tangzi = new General(this, "tangzi", "wei", 4, true, true);
    wei_tangzi->addSkill(new Xingzhao);
    wei_tangzi->addSkill(new XingzhaoGlobal);
    wei_tangzi->addRelateSkill("xunxun");

    General *wu_tangzi = new General(this, "wu_tangzi", "wu", 4, true, true);
    wu_tangzi->addSkill("xingzhao");
    wu_tangzi->addSkill("#xingzhao-global");
    wu_tangzi->addRelateSkill("xunxun");

    General *qd_liuqi = new General(this, "qdragon_liuqi", "qun", 3, true, true);
    qd_liuqi->addSkill(new DWenji);
    qd_liuqi->addSkill(new DTunjiang);
    qd_liuqi->addSkill(new DTunjiangRecord);

    General *d_liuqi = new General(this, "dragon_liuqi", "shu", 3, true, true);
    d_liuqi->addSkill("d_wenji");
    d_liuqi->addSkill("d_tunjiang");
    d_liuqi->addSkill("#d_tunjiang-record");

    addMetaObject<JianjiCard>();

    skills << new AiyeNerverDie << new AiyeGainMark << new DragonBoatGameover;
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
