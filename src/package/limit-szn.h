#ifndef NOSTALOL_H
#define NOSTALOL_H

#include "package.h"
#include "card.h"
#include "skill.h"

class LimitSZNPackage : public Package
{
    Q_OBJECT

public:
    LimitSZNPackage();
};


class SZNJieweiMoveCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SZNJieweiMoveCard();

    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &use) const;
};

#endif // LIMITSZNPACKAGE_H
