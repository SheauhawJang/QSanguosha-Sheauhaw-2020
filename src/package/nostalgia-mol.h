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

#endif // NOSTALMOL_H