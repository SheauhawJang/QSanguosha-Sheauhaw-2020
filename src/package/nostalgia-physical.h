#ifndef NOSTALPHYSICAL_H
#define NOSTALPHYSICAL_H

#include "package.h"
#include "card.h"
#include "skill.h"

class Nostal2013Package : public Package
{
    Q_OBJECT

public:
    Nostal2013Package();
};

class Nostal2015Package : public Package
{
    Q_OBJECT

public:
    Nostal2015Package();
};

class Nostal2017Package : public Package
{
    Q_OBJECT

public:
    Nostal2017Package();
};

class Nostal2020Package : public Package
{
    Q_OBJECT

public:
    Nostal2020Package();
};

class NostalRenewPackage : public Package
{
    Q_OBJECT

public:
    NostalRenewPackage();
};

class Nos2013RendeCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE Nos2013RendeCard();
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class Nos2017QingjianAllotCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE Nos2017QingjianAllotCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

#endif // NOSTALPHYSICAL_H
