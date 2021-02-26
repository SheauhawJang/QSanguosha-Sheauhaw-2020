#ifndef _NOSTALMYTH_H
#define _NOSTALMYTH_H

#include "package.h"
#include "card.h"
#include "standard.h"
#include "standard-skillcards.h"

class NostalWindPackage : public Package
{
    Q_OBJECT

public:
    NostalWindPackage();
};

class NostalFirePackage : public Package
{
    Q_OBJECT

public:
    NostalFirePackage();
};

class NostalThicketPackage : public Package
{
    Q_OBJECT

public:
    NostalThicketPackage();
};

class NosGuhuoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NosGuhuoCard();
    bool nosguhuo(ServerPlayer *yuji) const;

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class NosShensuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NosShensuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class NosTianxiangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NosTianxiangCard();

    void onEffect(const CardEffectStruct &effect) const;
};

class NosQiangxiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NosQiangxiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void extraCost(Room *room, const CardUseStruct &card_use) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

#endif // _NOSTALMYTH_H
