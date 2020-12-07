#ifndef _WINDMS_H
#define _WINDMS_H

#include "package.h"
#include "card.h"
#include "skill.h"

#include "standard-skillcards.h"
#include "sp.h"

#include <QDialog>
#include <QAbstractButton>
#include <QGroupBox>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QCommandLinkButton>

class Jiuhshoou : public PhaseChangeSkill
{
public:
    Jiuhshoou();
    bool onPhaseChange(ServerPlayer *target) const;

protected:
    virtual int getJiuhshoouDrawNum(ServerPlayer *caoren) const;
};

class TianshiangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE TianshiangCard();

    void onEffect(const CardEffectStruct &effect) const;
};

class ShernsuhCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShernsuhCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class RernderCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE RernderCard();
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class LesbianLijianCard : public LijianCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LesbianLijianCard(bool cancelable = true);

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    //virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    //virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;

private:
    bool duel_cancelable;
};

class LesbianJieyinCard : public JieyinCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LesbianJieyinCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    //virtual void onEffect(const CardEffectStruct &effect) const;
};

class LesbianLihunCard : public LihunCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LesbianLihunCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    //virtual void extraCost(Room *room, const CardUseStruct &card_use) const;
    //virtual void onEffect(const CardEffectStruct &effect) const;
};

class NosLesbianLijianCard : public LesbianLijianCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NosLesbianLijianCard();
};

class LesbianLianliSlashCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LesbianLianliSlashCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    const Card *validate(CardUseStruct &cardUse) const;
};

class WindmsPackage : public Package
{
    Q_OBJECT

public:
    WindmsPackage();
};

class WestMianshaPackage : public Package
{
    Q_OBJECT

public:
    WestMianshaPackage();
};

class NostalThicketPackage : public Package
{
    Q_OBJECT

public:
    NostalThicketPackage();
};

class NostalFirePackage : public Package
{
    Q_OBJECT

public:
    NostalFirePackage();
};

class StandardmsPackage : public Package
{
    Q_OBJECT

public:
    StandardmsPackage();
};
#endif

