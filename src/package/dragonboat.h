#ifndef DRAGONBOAT_H
#define DRAGONBOAT_H

#include "package.h"
#include "card.h"
#include "standard.h"

class JianjiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE JianjiCard();

    virtual bool targetFixed() const;
    virtual void onUse(Room *room, const CardUseStruct &use) const;
};

class Zong : public BasicCard
{
    Q_OBJECT

public:
    Q_INVOKABLE Zong(Card::Suit suit, int number);
    virtual QString getSubtype() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *source) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *) const;
};

class RealgarAnaleptic : public BasicCard
{
    Q_OBJECT

public:
    Q_INVOKABLE RealgarAnaleptic(Card::Suit suit, int number);
    virtual QString getSubtype() const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
    virtual QList<ServerPlayer *> defaultTargets(Room *room, ServerPlayer *source) const;
};

class SameDragon : public TrickCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SameDragon(Card::Suit suit, int number);

    virtual QString getSubtype() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
    int getDrawNum(ServerPlayer *player) const;
    virtual QList<ServerPlayer *> defaultTargets(Room *room, ServerPlayer *source) const;
};

class FightforUpstream : public GlobalEffect
{
    Q_OBJECT

public:
    Q_INVOKABLE FightforUpstream(Card::Suit suit, int number);

    virtual void onEffect(const CardEffectStruct &effect) const;
    int getDrawNum(ServerPlayer *player) const;
};

class AgainstWater : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE AgainstWater(Card::Suit suit, int number);

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class DragonBoatPackage : public Package
{
    Q_OBJECT

public:
    DragonBoatPackage();
};

class DragonBoatCardPackage : public Package
{
    Q_OBJECT

public:
    DragonBoatCardPackage();
};

#endif // DRAGONBOAT_H


