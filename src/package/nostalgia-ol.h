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

#endif // NOSTALOL_H
