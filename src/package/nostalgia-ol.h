#ifndef NOSTALOL_H
#define NOSTALOL_H

#include "package.h"
#include "card.h"
#include "skill.h"

class NostalOLPackage : public Package
{
    Q_OBJECT

public:
    NostalOLPackage();
};

class NOLQingjianAllotCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NOLQingjianAllotCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class NOLQimouCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NOLQimouCard();

    virtual void extraCost(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class NOLGuhuoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NOLGuhuoCard();
    bool nolguhuo(ServerPlayer *yuji) const;

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

#endif // NOSTALOL_H
