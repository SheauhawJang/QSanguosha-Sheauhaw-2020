#ifndef NOSTALPHYSICAL_H
#define NOSTALPHYSICAL_H

#include "package.h"
#include "card.h"
#include "skill.h"

class NostalPhysicalPackage : public Package
{
    Q_OBJECT

public:
    NostalPhysicalPackage();
};

class NPhyRendeCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NPhyRendeCard();
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

#endif // NOSTALPHYSICAL_H
