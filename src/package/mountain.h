#ifndef _MOUNTAIN_H
#define _MOUNTAIN_H

#include "package.h"
#include "card.h"
#include "skill.h"

class QiaobianMoveCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE QiaobianMoveCard();

    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &use) const;
	virtual void onEffect(const CardEffectStruct &effect) const;
};

class TiaoxinCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE TiaoxinCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ZhijianCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZhijianCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
	virtual void extraCost(Room *room, const CardUseStruct &card_use) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ZhibaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZhibaCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ZhibaAttachCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZhibaAttachCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class FangquanAskCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE FangquanAskCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class MountainPackage : public Package
{
    Q_OBJECT

public:
    MountainPackage();
};

class Huashen : public TriggerSkill
{
public:
    Huashen();

    static void AcquireGenerals(ServerPlayer *zuoci, int n);
    static QStringList GetAvailableGenerals(ServerPlayer *zuoci);
    static void SelectSkill(ServerPlayer *zuoci);
    static int ThrowGenerals(ServerPlayer *zuoci, int min = 0, int max = -1);
    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const;
    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *zuoci) const;

};

#endif

