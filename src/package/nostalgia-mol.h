#ifndef NOSTALMOL_H
#define NOSTALMOL_H

#include "package.h"
#include "card.h"
#include "skill.h"

class NostalMOLPackage : public Package
{
    Q_OBJECT

public:
    NostalMOLPackage();
};

class NMOLQingjianAllotCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NMOLQingjianAllotCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class NMOLQiangxiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NMOLQiangxiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void extraCost(Room *room, const CardUseStruct &card_use) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class NMOLNiepanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NMOLNiepanCard();

    virtual void extraCost(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;

    static void doNiepan(Room *room, ServerPlayer *player);
};

#endif // NOSTALMOL_H
