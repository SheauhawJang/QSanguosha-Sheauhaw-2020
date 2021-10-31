#ifndef NOSTALOL_H
#define NOSTALOL_H

#include "package.h"
#include "card.h"
#include "skill.h"

class LimitOLPackage : public Package
{
    Q_OBJECT

public:
    LimitOLPackage();
};

class OLQimouCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE OLQimouCard();

    virtual void extraCost(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class OLGuhuoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE OLGuhuoCard();
    bool olguhuo(ServerPlayer *yuji) const;

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

#endif // NOSTALOL_H
