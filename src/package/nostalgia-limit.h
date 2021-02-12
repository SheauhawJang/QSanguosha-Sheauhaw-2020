#ifndef NOSTALLIMIT_H
#define NOSTALLIMIT_H

#include "package.h"
#include "card.h"
#include "skill.h"

class NostalLimitationBrokenPackage : public Package
{
    Q_OBJECT

public:
    NostalLimitationBrokenPackage();
};


class NJieTuxiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NJieTuxiCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class NJieYijiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NJieYijiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class NJieYijueCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NJieYijueCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ChuliCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ChuliCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};


#endif // NOSTALLIMIT_H
