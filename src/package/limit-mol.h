#ifndef NOSTALMOL_H
#define NOSTALMOL_H

#include "package.h"
#include "card.h"
#include "skill.h"

class LimitMOLPackage : public Package
{
    Q_OBJECT

public:
    LimitMOLPackage();
};

class MOLQingjianAllotCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MOLQingjianAllotCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class MOLNiepanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MOLNiepanCard();

    virtual void extraCost(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;

    static void doNiepan(Room *room, ServerPlayer *player);
};

#endif // NOSTALMOL_H
