#ifndef _YJCM_H
#define _YJCM_H

#include "package.h"
#include "card.h"
#include "skill.h"

class YJCMPackage : public Package
{
    Q_OBJECT

public:
    YJCMPackage();
};

class JiushiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE JiushiCard();

    virtual const Card *validate(CardUseStruct &cardUse) const;
};

class MingceCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MingceCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
	virtual void extraCost(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class GanluCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE GanluCard();
    //void swapEquip(ServerPlayer *first, ServerPlayer *second) const;

    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class XianzhenCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XianzhenCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class PaiyiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE PaiyiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class SanyaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SanyaoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class JianyanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE JianyanCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

#endif

