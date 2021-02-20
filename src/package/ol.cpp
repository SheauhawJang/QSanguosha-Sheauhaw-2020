#include "ol.h"
#include "general.h"
#include "skill.h"
#include "standard.h"
#include "yjcm2013.h"
#include "engine.h"
#include "clientplayer.h"
#include "json.h"
#include "maneuvering.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCommandLinkButton>
#include "settings.h"

class dummyVS : public ZeroCardViewAsSkill
{
public:
    dummyVS() : ZeroCardViewAsSkill("dummy")
    {
    }

    virtual const Card *viewAs() const
    {
        return NULL;
    }
};

class AocaiVeiw : public OneCardViewAsSkill
{
public:
    AocaiVeiw() : OneCardViewAsSkill("aocai_view")
    {
        expand_pile = "#aocai",
        response_pattern = "@@aocai_view";
    }

    bool viewFilter(const Card *to_select) const
    {
        QStringList aocai = Self->property("aocai").toString().split("+");
        foreach (QString id, aocai) {
            bool ok;
            if (id.toInt(&ok) == to_select->getEffectiveId() && ok)
                return true;
        }
        return false;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        return originalCard;
    }
};

class AocaiViewAsSkill : public ZeroCardViewAsSkill
{
public:
    AocaiViewAsSkill() : ZeroCardViewAsSkill("aocai")
    {
    }

    bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (player->getPhase() != Player::NotActive || player->hasFlag("Global_AocaiFailed")) return false;
        return pattern == "slash" || pattern == "jink" || pattern == "peach" || pattern.contains("analeptic");
    }

    const Card *viewAs() const
    {
        AocaiCard *aocai_card = new AocaiCard;
        aocai_card->setUserString(Sanguosha->currentRoomState()->getCurrentCardUsePattern());
        return aocai_card;
    }
};

class Aocai : public TriggerSkill
{
public:
    Aocai() : TriggerSkill("aocai")
    {
        events << CardAsked;
        view_as_skill = new AocaiViewAsSkill;
    }

    bool trigger(TriggerEvent, Room *, ServerPlayer *, QVariant &) const
    {
        return false;
    }

    static int view(Room *room, ServerPlayer *player, QList<int> &ids, QList<int> &enabled)
    {
        int result = -1, index = -1;
        LogMessage log;
        log.type = "$ViewDrawPile";
        log.from = player;
        log.card_str = IntList2StringList(ids).join("+");
        room->sendLog(log, player);

        player->broadcastSkillInvoke("aocai");
        room->notifySkillInvoked(player, "aocai");

        room->notifyMoveToPile(player, ids, "aocai", Player::PlaceTable, true, true);
        room->setPlayerProperty(player, "aocai", IntList2StringList(enabled).join("+"));
        const Card *card = room->askForCard(player, "@@aocai_view", "@aocai-view", QVariant(), Card::MethodNone);
        room->notifyMoveToPile(player, ids, "aocai", Player::PlaceTable, false, false);
        if (card == NULL)
            room->setPlayerFlag(player, "Global_AocaiFailed");
        else {
            result = card->getSubcards().first();
            index = ids.indexOf(result);
            LogMessage log;
            log.type = "#AocaiUse";
            log.from = player;
            log.arg = "aocai";
            log.arg2 = QString("CAPITAL(%1)").arg(index + 1);
            room->sendLog(log);
        }
        room->returnToTopDrawPile(ids);
        return result;
    }
};

AocaiCard::AocaiCard()
{
}

bool AocaiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    const Card *card = NULL;
    if (!user_string.isEmpty())
        card = Sanguosha->cloneCard(user_string.split("+").first());
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool AocaiCard::targetFixed() const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return true;

    const Card *card = NULL;
    if (!user_string.isEmpty())
        card = Sanguosha->cloneCard(user_string.split("+").first());
    return card && card->targetFixed();
}

bool AocaiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    const Card *card = NULL;
    if (!user_string.isEmpty())
        card = Sanguosha->cloneCard(user_string.split("+").first());
    return card && card->targetsFeasible(targets, Self);
}

const Card *AocaiCard::validateInResponse(ServerPlayer *user) const
{
    Room *room = user->getRoom();
    QList<int> ids = room->getNCards(2, false);
    QStringList names = user_string.split("+");
    if (names.contains("slash")) names << "fire_slash" << "thunder_slash";

    QList<int> enabled;
    foreach (int id, ids)
        if (names.contains(Sanguosha->getCard(id)->objectName()))
            enabled << id;

    LogMessage log;
    log.type = "#InvokeSkill";
    log.from = user;
    log.arg = "aocai";
    room->sendLog(log);

    int id = Aocai::view(room, user, ids, enabled);
    return Sanguosha->getCard(id);
}

const Card *AocaiCard::validate(CardUseStruct &cardUse) const
{
    ServerPlayer *user = cardUse.from;
    Room *room = user->getRoom();

    LogMessage log;
    log.from = user;
    log.to = cardUse.to;
    log.type = "#UseCard";
    log.card_str = toString();
    room->sendLog(log);

    QList<int> ids = room->getNCards(2, false);
    QStringList names = user_string.split("+");
    if (names.contains("slash")) names << "fire_slash" << "thunder_slash";

    QList<int> enabled;
    foreach (int id, ids)
        if (names.contains(Sanguosha->getCard(id)->objectName()))
            enabled << id;

    int id = Aocai::view(room, user, ids, enabled);
    const Card *card = Sanguosha->getCard(id);
    if (card)
        room->setCardFlag(card, "hasSpecialEffects");
    return card;
}

DuwuCard::DuwuCard()
{
}

bool DuwuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return (targets.isEmpty() && qMax(0, to_select->getHp()) == subcardsLength() && Self->inMyAttackRange(to_select));
}

void DuwuCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();

    room->damage(DamageStruct("duwu", effect.from, effect.to));
}

class DuwuViewAsSkill : public ViewAsSkill
{
public:
    DuwuViewAsSkill() : ViewAsSkill("duwu")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->canDiscard(player, "he") && !player->hasFlag("DuwuEnterDying");
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return !Self->isJilei(to_select);
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        DuwuCard *duwu = new DuwuCard;
        if (!cards.isEmpty())
            duwu->addSubcards(cards);
        return duwu;
    }
};

class Duwu : public TriggerSkill
{
public:
    Duwu() : TriggerSkill("duwu")
    {
        events << QuitDying;
        view_as_skill = new DuwuViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        DyingStruct dying = data.value<DyingStruct>();
        if (dying.damage && dying.damage->getReason() == "duwu" && !dying.damage->chain && !dying.damage->transfer) {
            ServerPlayer *from = dying.damage->from;
            if (from && from->isAlive()) {
                room->setPlayerFlag(from, "DuwuEnterDying");
                from->broadcastSkillInvoke(objectName());
                room->sendCompulsoryTriggerLog(from, objectName());
                room->loseHp(from, 1);
            }
        }
        return false;
    }
};

ShefuCard::ShefuCard()
{
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
}

void ShefuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    QString mark = "Shefu_" + user_string;
    room->setPlayerMark(source, mark, getEffectiveId() + 1);
    source->addToPile("ambush", this, false);
}

class ShefuViewAsSkill : public OneCardViewAsSkill
{
public:
    ShefuViewAsSkill() : OneCardViewAsSkill("shefu")
    {
        filter_pattern = ".|.|.|hand";
        response_pattern = "@@shefu";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        ShefuCard *card = new ShefuCard;
        card->addSubcard(originalCard);
        card->setSkillName("shefu");
        return card;
    }
};

class Shefu : public TriggerSkill
{
public:
    Shefu() : TriggerSkill("shefu")
    {
        events << EventPhaseStart << CardUsed << JinkEffect;
        view_as_skill = new ShefuViewAsSkill;
    }

    QString getSelectBox() const
    {
        return "guhuo_sbtd";
    }

    bool buttonEnabled(const QString &button_name, const QList<const Card *> &, const QList<const Player *> &) const
    {
        return Self->getMark("Shefu_" + button_name) == 0;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (triggerEvent == EventPhaseStart && TriggerSkill::triggerable(player)) {
            if (player->getPhase() == Player::Finish && !player->isKongcheng())
                skill_list.insert(player, nameList());
        } else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() != Card::TypeBasic && use.card->getTypeId() != Card::TypeTrick && use.m_isHandcard)
                return skill_list;
            QString card_name = use.card->objectName();
            if (card_name.contains("slash")) card_name = "slash";
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (ShefuTriggerable(p, player, card_name)) {
                    skill_list.insert(p, nameList());
                }
            }
        } else if (triggerEvent == JinkEffect) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (ShefuTriggerable(p, player, "jink")) {
                    skill_list.insert(p, nameList());
                }
            }
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *chengyu) const
    {
        if (triggerEvent == EventPhaseStart) {
            room->askForUseCard(chengyu, "@@shefu", "@shefu-prompt", QVariant(), Card::MethodNone);
        } else if (triggerEvent == JinkEffect) {
            room->setTag("ShefuData", data);
            if (!room->askForSkillInvoke(chengyu, "shefu_cancel", "data:::jink") || chengyu->getMark("Shefu_jink") == 0)
                return false;

            chengyu->broadcastSkillInvoke("shefu");

            LogMessage log;
            log.type = "#ShefuEffect";
            log.from = chengyu;
            log.to << player;
            log.arg = "jink";
            log.arg2 = objectName();
            room->sendLog(log);

            CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), "shefu", QString());
            int id = chengyu->getMark("Shefu_jink") - 1;
            room->setPlayerMark(chengyu, "Shefu_jink", 0);
            room->throwCard(Sanguosha->getCard(id), reason, NULL);


            return true;
        } else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();

            QString card_name = use.card->objectName();
            if (card_name.contains("slash")) card_name = "slash";

            room->setTag("ShefuData", data);
            if (!room->askForSkillInvoke(chengyu, "shefu_cancel", "data:::" + card_name) || chengyu->getMark("Shefu_" + card_name) == 0)
                return false;

            chengyu->broadcastSkillInvoke("shefu");

            LogMessage log;
            log.type = "#ShefuEffect";
            log.from = chengyu;
            log.to << player;
            log.arg = card_name;
            log.arg2 = objectName();
            room->sendLog(log);

            CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), "shefu", QString());
            int id = chengyu->getMark("Shefu_" + card_name) - 1;
            room->setPlayerMark(chengyu, "Shefu_" + card_name, 0);
            room->throwCard(Sanguosha->getCard(id), reason, NULL);
            use.to.clear();
            use.to_card = NULL;//for nullification
            data = QVariant::fromValue(use);

            return true;
        }
        return false;
    }

private:
    bool ShefuTriggerable(ServerPlayer *chengyu, ServerPlayer *user, QString card_name) const
    {
        return chengyu->getPhase() == Player::NotActive && chengyu != user && chengyu->hasSkill("shefu")
               && !chengyu->getPile("ambush").isEmpty() && chengyu->getMark("Shefu_" + card_name) > 0;
    }
};

class BenyuViewAsSkill : public ViewAsSkill
{
public:
    BenyuViewAsSkill() : ViewAsSkill("benyu")
    {
        response_pattern = "@@benyu";
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() < Self->getMark("benyu"))
            return NULL;

        DummyCard *card = new DummyCard;
        card->addSubcards(cards);
        return card;
    }
};

class Benyu : public MasochismSkill
{
public:
    Benyu() : MasochismSkill("benyu")
    {
        view_as_skill = new BenyuViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (MasochismSkill::triggerable(player)) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->isAlive()) {
                int num1 = damage.from->getHandcardNum(), num2 = player->getHandcardNum();
                if ((num1 > num2 && num2 < 5) || num1 < num2)
                    return nameList();
            }
        }

        return QStringList();
    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const
    {
        Room *room = target->getRoom();
        int from_handcard_num = damage.from->getHandcardNum(), handcard_num = target->getHandcardNum();
        if (handcard_num == from_handcard_num)
            return;
        else if (handcard_num < from_handcard_num && handcard_num < 5 && room->askForSkillInvoke(target, objectName(), QVariant::fromValue(damage.from))) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, target->objectName(), damage.from->objectName());
            target->broadcastSkillInvoke(objectName());
            target->fillHandCards(qMin(5, from_handcard_num), objectName());
        } else if (handcard_num > from_handcard_num) {
            room->setPlayerMark(target, objectName(), from_handcard_num + 1);
            if (room->askForCard(target, "@@benyu", QString("@benyu-discard::%1:%2").arg(damage.from->objectName()).arg(from_handcard_num + 1), QVariant(), objectName(), damage.from))
                room->damage(DamageStruct(objectName(), target, damage.from));
        }
        return;
    }
};

class Canshi : public TriggerSkill
{
public:
    Canshi() : TriggerSkill("canshi")
    {
        events << DrawNCards << CardUsed << CardResponded;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == DrawNCards && TriggerSkill::triggerable(player)) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->isWounded() || (player != p && player->hasLordSkill("guiming") && p->getKingdom() == "wu"))
                    return nameList();
            }
        } else if (triggerEvent == CardUsed || triggerEvent == CardResponded) {
            if (player->hasFlag(objectName())) {
                const Card *card = NULL;
                if (triggerEvent == CardUsed)
                    card = data.value<CardUseStruct>().card;
                else {
                    CardResponseStruct resp = data.value<CardResponseStruct>();
                    if (resp.m_isUse)
                        card = resp.m_card;
                }
                if (card != NULL && (card->isKindOf("Slash") || card->isNDTrick()))
                    return QStringList("canshi!");
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == DrawNCards) {
            int n = 0;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->isWounded() || (player != p && player->hasLordSkill("guiming") && p->getKingdom() == "wu"))
                    n++;
            }
            if (n > 0 && player->askForSkillInvoke(this, "prompt:::" + QString::number(n))) {
                player->broadcastSkillInvoke(objectName());
                player->setFlags(objectName());
                data = data.toInt() + n;
            }

        } else {
            LogMessage log;
            log.type = "#SkillForce";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);
            room->askForDiscard(player, objectName(), 1, 1, false, true, "@canshi-discard");
        }
        return false;
    }
};

class Chouhai : public TriggerSkill
{
public:
    Chouhai() : TriggerSkill("chouhai")
    {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player) && player->isKongcheng()) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("Slash")) {
                return nameList();
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        player->broadcastSkillInvoke(objectName());

        DamageStruct damage = data.value<DamageStruct>();
        ++damage.damage;
        data = QVariant::fromValue(damage);

        return false;
    }
};

class Biluan : public PhaseChangeSkill
{
public:
    Biluan() : PhaseChangeSkill("biluan")
    {

    }

    virtual QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (!PhaseChangeSkill::triggerable(player) || player->isNude()) return QStringList();
        if (player->getPhase() == Player::Finish) {
            QList<ServerPlayer *> players = room->getOtherPlayers(player);
            foreach(ServerPlayer *p, players) {
                if (p->distanceTo(player) == 1)
                    return nameList();
            }
        }
        return QStringList();
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        int x = qMin(room->alivePlayerCount(), 4);
        if (room->askForCard(player, "..", "@biluan:::" + QString::number(x), QVariant(), objectName()))
            room->addPlayerMark(player, "#shixie_distance", x);
        return false;
    }
};

class Lixia : public PhaseChangeSkill
{
public:
    Lixia() : PhaseChangeSkill("lixia")
    {
        frequency = Compulsory;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;

        if (player && player->isAlive() && player->getPhase() == Player::Finish) {
            QList<ServerPlayer *> shixies = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *shixie, shixies) {
                if (!player->inMyAttackRange(shixie) && player != shixie)
                    skill_list.insert(shixie, nameList());
            }
        }

        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *target, QVariant &, ServerPlayer *shixie) const
    {
        room->sendCompulsoryTriggerLog(shixie, objectName());
        shixie->broadcastSkillInvoke(objectName());
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, shixie->objectName(), target->objectName());

        if (room->askForChoice(shixie, objectName(), "self+other", QVariant(), "@lixia-choose::" + target->objectName()) == "other")
            target->drawCards(2, objectName());
        else
            shixie->drawCards(1, objectName());
        room->setPlayerMark(shixie, "#shixie_distance", shixie->getMark("#shixie_distance") - 1);
        return false;
    }

    bool onPhaseChange(ServerPlayer *) const
    {
        return false;
    }
};

class ShixieDistance : public DistanceSkill
{
public:
    ShixieDistance() : DistanceSkill("#shixie-distance")
    {
    }

    virtual int getCorrect(const Player *, const Player *to) const
    {
        return to->getMark("#shixie_distance");
    }
};

class Ranshang : public TriggerSkill
{
public:
    Ranshang() : TriggerSkill("ranshang")
    {
        events << EventPhaseStart << Damaged;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Finish) {
            if (player->getMark("#kindle") > 0) {
                room->sendCompulsoryTriggerLog(player, objectName());
                player->broadcastSkillInvoke(objectName());
                room->loseHp(player, player->getMark("#kindle"));
            }
        } else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.nature == DamageStruct::Fire) {
                room->sendCompulsoryTriggerLog(player, objectName());
                player->broadcastSkillInvoke(objectName());
                player->gainMark("#kindle", damage.damage);
            }
        }
        return false;
    }
};

class Hanyong : public TriggerSkill
{
public:
    Hanyong() : TriggerSkill("hanyong")
    {
        events << CardUsed << ConfirmDamage;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardUsed) {
            if (!TriggerSkill::triggerable(player))
                return false;
            CardUseStruct use = data.value<CardUseStruct>();
            if (player->getHp() >= room->getTurn())
                return false;
            if (use.card->isKindOf("SavageAssault") || use.card->isKindOf("ArcheryAttack")) {
                if (player->askForSkillInvoke(objectName(), data)) {
                    player->broadcastSkillInvoke(objectName());
                    room->setCardFlag(use.card, "HanyongEffect");
                }
            }
        } else if (triggerEvent == ConfirmDamage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (!damage.card || !damage.card->hasFlag("HanyongEffect"))
                return false;
            damage.damage++;
            data = QVariant::fromValue(damage);
        }
        return false;
    }
};

class Zhidao : public TriggerSkill
{
public:
    Zhidao() : TriggerSkill("zhidao")
    {
        events << Damage;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = damage.to;
        if (!player->hasFlag("ZhidaoInvoked") && !target->isAllNude() && player->getPhase() == Player::Play) {
            room->sendCompulsoryTriggerLog(player, objectName());
            player->broadcastSkillInvoke(objectName());
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());

            room->setPlayerFlag(player, "ZhidaoInvoked");
            room->setPlayerFlag(player, "DisabledOtherTargets");
            int n = 0;
            if (!target->isKongcheng()) n++;
            if (!target->getEquips().isEmpty()) n++;
            if (!target->getJudgingArea().isEmpty()) n++;
            QList<int> ids = room->askForCardsChosen(player, target, "hej", objectName(), n, n);
            DummyCard *dummy = new DummyCard(ids);
            CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, player->objectName());
            room->obtainCard(player, dummy, reason, false);
            delete dummy;
        }

        return false;
    }
};

class Jili : public TriggerSkill
{
public:
    Jili() : TriggerSkill("jili")
    {
        events << TargetConfirming;
        frequency = Compulsory;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (player->isDead()) return skill_list;
        CardUseStruct use = data.value<CardUseStruct>();
        if ((use.card->getTypeId() == Card::TypeBasic || use.card->isNDTrick()) && use.card->isRed()) {
            QList<ServerPlayer *> ybhs = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *ybh, ybhs) {
                if (use.from != ybh && !use.to.contains(ybh) && player->distanceTo(ybh) == 1)
                    skill_list.insert(ybh, nameList());
            }
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *target, QVariant &data, ServerPlayer *ybh) const
    {
        CardUseStruct use = data.value<CardUseStruct>();

        ybh->broadcastSkillInvoke(objectName(), isGoodEffect(use.card, ybh) ? 2 : 1);
        room->notifySkillInvoked(ybh, objectName());
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, ybh->objectName(), target->objectName());
        LogMessage log;
        log.type = "#JiliAdd";
        log.from = ybh;
        log.to << target;
        log.card_str = use.card->toString();
        log.arg = objectName();
        room->sendLog(log);
        use.to.append(ybh);
        room->sortByActionOrder(use.to);
        data = QVariant::fromValue(use);

        return false;
    }

private:
    static bool isGoodEffect(const Card *card, ServerPlayer *yanbaihu)
    {
        return card->isKindOf("Peach") || card->isKindOf("Analeptic") || card->isKindOf("ExNihilo")
               || card->isKindOf("AmazingGrace") || card->isKindOf("GodSalvation")
               || (card->isKindOf("IronChain") && yanbaihu->isChained());
    }
};

class Zhengnan : public TriggerSkill
{
public:
    Zhengnan() : TriggerSkill("zhengnan")
    {
        events << Dying;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player)) {
            DyingStruct dying = data.value<DyingStruct>();
            ServerPlayer *target = dying.who;
            QStringList zhengnan_used = player->tag["ZhengnanUsed"].toStringList();
            if (target && target->isAlive() && !zhengnan_used.contains(target->objectName())) {
                return nameList();
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *guansuo, QVariant &data, ServerPlayer *) const
    {
        DyingStruct dying = data.value<DyingStruct>();
        ServerPlayer *player = dying.who;
        if (guansuo->askForSkillInvoke(this, QVariant::fromValue(player))) {
            guansuo->broadcastSkillInvoke(objectName());

            QStringList zhengnan_used = guansuo->tag["ZhengnanUsed"].toStringList();
            zhengnan_used << player->objectName();
            guansuo->tag["ZhengnanUsed"] = zhengnan_used;

            if (guansuo->isWounded())
                room->recover(guansuo, RecoverStruct(guansuo));

            QStringList skill_list;
            if (!guansuo->hasSkill("wusheng", true))
                skill_list << "wusheng";
            if (!guansuo->hasSkill("dangxian", true))
                skill_list << "dangxian";
            if (!guansuo->hasSkill("zhiman", true))
                skill_list << "zhiman";

            guansuo->drawCards(skill_list.isEmpty() ? 3 : 1, objectName());

            if (!skill_list.isEmpty())
                room->acquireSkill(guansuo, room->askForChoice(guansuo, objectName(), skill_list.join("+"), QVariant(), "@zhengnan-choose"));
        }
        return false;
    }
};

class Xiefang : public DistanceSkill
{
public:
    Xiefang() : DistanceSkill("xiefang")
    {
    }

    virtual int getCorrect(const Player *from, const Player *) const
    {
        if (from->hasSkill(this)) {
            int extra = 0;
            if (from->isFemale())
                extra ++;
            QList<const Player *> players = from->getAliveSiblings();
            foreach (const Player *p, players) {
                if (p->isFemale())
                    extra ++;
            }
            return -extra;
        } else
            return 0;
    }
};

GusheCard::GusheCard()
{
}

bool GusheCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.length() < 3 && Self->canPindian(to_select);
}

void GusheCard::use(Room *, ServerPlayer *wangsitu, QList<ServerPlayer *> &targets) const
{
    PindianStruct *pd = wangsitu->pindianStart(targets, "gushe");
    for (int i = 1; i <= targets.length(); i++)
        wangsitu->pindianResult(pd, i);
    wangsitu->pindianFinish(pd);
}

class GusheViewAsSkill : public ZeroCardViewAsSkill
{
public:
    GusheViewAsSkill() : ZeroCardViewAsSkill("gushe")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->usedTimes("GusheCard") <= player->getMark("jici") && !player->isKongcheng();
    }

    virtual const Card *viewAs() const
    {
        return new GusheCard;
    }
};

class Gushe : public TriggerSkill
{
public:
    Gushe() : TriggerSkill("gushe")
    {
        events << Pindian;
        view_as_skill = new GusheViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *wangsitu, QVariant &data) const
    {
        PindianStruct *pindian = data.value<PindianStruct *>();
        if (pindian->reason != "gushe") return false;
        QList<ServerPlayer *> losers;
        if (pindian->success) {
            losers << pindian->to;
        } else {
            losers << wangsitu;
            if (pindian->from_number == pindian->to_number)
                losers << pindian->to;
            wangsitu->gainMark("#rap");
            if (wangsitu->getMark("#rap") >= 7)
                room->killPlayer(wangsitu);
        }
        foreach (ServerPlayer *loser, losers) {
            if (loser->isDead()) continue;
            if (!room->askForDiscard(loser, "gushe", 1, 1, wangsitu->isAlive(), true, "@gushe-discard:" + wangsitu->objectName())) {
                wangsitu->drawCards(1);
            }
        }
        return false;
    }
};

class Jici : public TriggerSkill
{
public:
    Jici() : TriggerSkill("jici")
    {
        events << PindianVerifying << EventPhaseChanging;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *wangsitu, QVariant &data) const
    {
        if (triggerEvent == PindianVerifying && TriggerSkill::triggerable(wangsitu)) {
            PindianStruct *pindian = data.value<PindianStruct *>();
            if (pindian->reason != "gushe" || pindian->from != wangsitu) return false;
            int n = wangsitu->getMark("#rap");
            if (pindian->from_number == n && wangsitu->askForSkillInvoke(objectName(), "prompt1")) {
                wangsitu->broadcastSkillInvoke(objectName());
                LogMessage log;
                log.type = "$JiciAddTimes";
                log.from = wangsitu;
                log.arg = "gushe";
                room->sendLog(log);
                room->addPlayerMark(wangsitu, "jici");
            } else if (pindian->from_number < n && wangsitu->askForSkillInvoke(objectName(), "prompt2:::" + QString::number(n))) {
                wangsitu->broadcastSkillInvoke(objectName());
                LogMessage log;
                log.type = "$JiciAddNumber";
                log.from = wangsitu;
                pindian->from_number = qMin(pindian->from_number + n, 13);
                log.arg = QString::number(n);
                log.arg2 = QString::number(pindian->from_number);
                room->sendLog(log);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to != Player::NotActive) return false;
            room->setPlayerMark(wangsitu, "jici", 0);
        }
        return false;
    }
};

class Cuifeng : public TriggerSkill
{
public:
    Cuifeng() : TriggerSkill("cuifeng")
    {
        events << Damaged << EventPhaseStart << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == Damaged && TriggerSkill::triggerable(player)) {
            DamageStruct damage = data.value<DamageStruct>();
            for (int i = 0; i < damage.damage; i++) {
                if (player->isNude()) break;
                const Card *card = room->askForCard(player, "..", "@cuifeng-put", data, Card::MethodNone);
                if (card) {
                    player->broadcastSkillInvoke(objectName());
                    room->notifySkillInvoked(player, objectName());
                    LogMessage log;
                    log.from = player;
                    log.type = "#InvokeSkill";
                    log.arg = objectName();
                    room->sendLog(log);
                    player->addToPile("cuifeng_feng", card);
                } else break;
            }
        } else if (triggerEvent == EventPhaseStart && TriggerSkill::triggerable(player)) {
            if (player->getPhase() != Player::Start || player->getPile("cuifeng_feng").isEmpty()) return false;
            player->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(player, objectName());
            int x = player->getPile("cuifeng_feng").length();
            player->clearOnePrivatePile("cuifeng_feng");
            player->drawCards(2 * x);
            room->setPlayerMark(player, "cuifeng", x);
        } else if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to != Player::NotActive) return false;
            room->setPlayerMark(player, "cuifeng", 0);
        }
        return false;
    }
};

class CuifengTargetMod : public TargetModSkill
{
public:
    CuifengTargetMod() : TargetModSkill("#cuifeng-target")
    {

    }

    virtual int getResidueNum(const Player *from, const Card *, const Player *) const
    {
        return from->getMark("cuifeng");
    }
};

LianzhuCard::LianzhuCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

void LianzhuCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *dongbai = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = dongbai->getRoom();
    room->showCard(dongbai, getEffectiveId());
    room->getThread()->delay(1000);

    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, dongbai->objectName(), target->objectName(), "lianzhu", QString());
    room->obtainCard(target, this, reason);

    if (target->isAlive() && isBlack() && !room->askForDiscard(target, "lianzhu", 2, 2, true, true, "@lianzhu-discard:" + dongbai->objectName()))
        dongbai->drawCards(2, "lianzhu");
}

class Lianzhu : public OneCardViewAsSkill
{
public:
    Lianzhu() : OneCardViewAsSkill("lianzhu")
    {
        filter_pattern = ".";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("LianzhuCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        LianzhuCard *card = new LianzhuCard;
        card->addSubcard(originalCard);
        card->setSkillName(objectName());
        return card;
    }
};

class Xiahui : public HideCardSkill
{
public:
    Xiahui() : HideCardSkill("xiahui")
    {

    }
    virtual bool isCardHided(const Player *player, const Card *card) const
    {
        if (!player->hasSkill(objectName())) return false;
        return card->isBlack();
    }
};

class XiahuiRecord : public TriggerSkill
{
public:
    XiahuiRecord() : TriggerSkill("#xiahui-record")
    {
        events << CardsMoveOneTime << PreCardsMoveOneTime << HpChanged;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *) const
    {
        return false;
    }

    void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == PreCardsMoveOneTime) {
            foreach (QVariant adata, data.toList()) {
                CardsMoveOneTimeStruct move = adata.value<CardsMoveOneTimeStruct>();
                if (move.from == player && !player->property("xiahui_record").toString().isEmpty()) {
                    QStringList xiahui_ids = player->property("xiahui_record").toString().split("+");
                    QStringList xiahui_copy = xiahui_ids;
                    foreach (QString card_data, xiahui_copy) {
                        int id = card_data.toInt();
                        if (move.card_ids.contains(id) && move.from_places[move.card_ids.indexOf(id)] == Player::PlaceHand) {
                            xiahui_ids.removeOne(card_data);
                        }
                    }
                    room->setPlayerProperty(player, "xiahui_record", xiahui_ids.join("+"));
                }
            }
        } else if (triggerEvent == CardsMoveOneTime && TriggerSkill::triggerable(player)) {
            foreach (QVariant adata, data.toList()) {
                CardsMoveOneTimeStruct move = adata.value<CardsMoveOneTimeStruct>();
                if (move.from == player && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
                        && (move.to != player && move.to_place == Player::PlaceHand)) {
                    ServerPlayer *target = (ServerPlayer *)move.to;
                    if (!target || target->isDead()) return;
                    QStringList xiahui_ids = target->property("xiahui_record").toString().split("+");
                    foreach (int id, move.card_ids) {
                        if (room->getCardOwner(id) == target && room->getCardPlace(id) == Player::PlaceHand && Sanguosha->getCard(id)->isBlack()) {
                            xiahui_ids << QString::number(id);
                        }
                    }
                    room->setPlayerProperty(target, "xiahui_record", xiahui_ids.join("+"));
                }
            }
        } else if (triggerEvent == HpChanged) {
            if (!data.isNull() && !data.canConvert<RecoverStruct>()) {
                room->setPlayerProperty(player, "xiahui_record", QStringList());
            }
        }
    }
};

class XiahuiCardLimited : public CardLimitedSkill
{
public:
    XiahuiCardLimited() : CardLimitedSkill("#xiahui-limited")
    {
    }
    virtual bool isCardLimited(const Player *player, const Card *card, Card::HandlingMethod method) const
    {
        if (player->property("xiahui_record").toString().isEmpty()) return false;
        if (method == Card::MethodUse || method == Card::MethodResponse || method == Card::MethodDiscard) {
            QStringList xiahui_ids = player->property("xiahui_record").toString().split("+");
            foreach (QString card_str, xiahui_ids) {
                bool ok;
                int id = card_str.toInt(&ok);
                if (!ok) continue;
                if (!card->isVirtualCard()) {
                    if (card->getEffectiveId() == id)
                        return true;
                } else if (card->getSubcards().contains(id))
                    return true;
            }
        }
        return false;
    }
};

FanghunCard::FanghunCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool FanghunCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Slash *slash = new Slash(getSuit(), getNumber());
    slash->addSubcard(this);
    slash->setSkillName("fanghun");
    return slash && slash->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, slash, targets);
}

bool FanghunCard::targetFixed() const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return true;

    return Sanguosha->getCard(getEffectiveId())->isKindOf("Slash");
}

bool FanghunCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    Slash *slash = new Slash(getSuit(), getNumber());
    slash->addSubcard(this);
    slash->setSkillName("fanghun");
    return slash && slash->targetsFeasible(targets, Self);
}

const Card *FanghunCard::validate(CardUseStruct &card_use) const
{
    return validateInResponse(card_use.from);
}

const Card *FanghunCard::validateInResponse(ServerPlayer *zhaoxiang) const
{
    const Card *card = Sanguosha->getCard(getEffectiveId());
    if (card->isKindOf("Jink")) {
        zhaoxiang->loseMark("#plumshadow");
        Slash *slash = new Slash(getSuit(), getNumber());
        slash->addSubcard(this);
        slash->setSkillName("fanghun");
        return slash;
    } else if (card->isKindOf("Slash")) {
        zhaoxiang->loseMark("#plumshadow");
        Jink *jink = new Jink(getSuit(), getNumber());
        jink->addSubcard(this);
        jink->setSkillName("fanghun");
        return jink;
    }
    return NULL;
}

void FanghunCard::validateInResponseAfter(ServerPlayer *zhaoxiang, const Card *) const
{
    zhaoxiang->drawCards(1, "fanghun");
}

class FanghunViewAsSkill : public OneCardViewAsSkill
{
public:
    FanghunViewAsSkill() : OneCardViewAsSkill("fanghun")
    {
        response_or_use = true;
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        const Card *card = to_select;

        switch (Sanguosha->currentRoomState()->getCurrentCardUseReason()) {
        case CardUseStruct::CARD_USE_REASON_PLAY: {
            if (card->isKindOf("Jink")) {
                Slash *slash = new Slash(card->getSuit(), card->getNumber());
                slash->addSubcard(card);
                slash->setSkillName(objectName());
                return slash->isAvailable(Self);
            }
            return false;
        }
        case CardUseStruct::CARD_USE_REASON_RESPONSE: {
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (pattern == "slash" && card->isKindOf("Jink")) {
                Slash *slash = new Slash(card->getSuit(), card->getNumber());
                slash->addSubcard(card);
                slash->setSkillName(objectName());
                return !Self->isCardLimited(slash, Card::MethodResponse);
            } else if (pattern == "jink" && card->isKindOf("Slash")) {
                Jink *jink = new Jink(card->getSuit(), card->getNumber());
                jink->addSubcard(card);
                jink->setSkillName(objectName());
                return !Self->isCardLimited(jink, Card::MethodResponse);
            }
        }
        case CardUseStruct::CARD_USE_REASON_RESPONSE_USE: {
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (pattern == "slash" && card->isKindOf("Jink")) {
                Slash *slash = new Slash(card->getSuit(), card->getNumber());
                slash->addSubcard(card);
                slash->setSkillName(objectName());
                return !Self->isLocked(slash);
            } else if (pattern == "jink" && card->isKindOf("Slash")) {
                Jink *jink = new Jink(card->getSuit(), card->getNumber());
                jink->addSubcard(card);
                jink->setSkillName(objectName());
                return !Self->isLocked(jink);
            }
        }
        default:
            return false;
        }
        return false;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return (hasAvailable(player) || Slash::IsAvailable(player)) && player->getMark("#plumshadow") > 0;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return (pattern == "jink" || pattern == "slash") && player->getMark("#plumshadow") > 0;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        FanghunCard *skill_card = new FanghunCard;
        skill_card->addSubcard(originalCard);
        return skill_card;
    }
};

class Fanghun : public TriggerSkill
{
public:
    Fanghun() : TriggerSkill("fanghun")
    {
        events << Damage << Damaged << EventLoseSkill;
        view_as_skill = new FanghunViewAsSkill;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventLoseSkill) {
            if (data.toString() == "fanghun")
                room->setPlayerMark(player, "#plumshadow", 0);
        }
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if ((triggerEvent == Damage || triggerEvent == Damaged) && TriggerSkill::triggerable(player)) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("Slash"))
                return nameList();
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *zhaoxiang, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        room->sendCompulsoryTriggerLog(zhaoxiang, objectName());
        zhaoxiang->broadcastSkillInvoke(objectName());
        zhaoxiang->gainMark("#plumshadow", damage.damage);
        room->addPlayerMark(zhaoxiang, "plumshadow", damage.damage);
        return false;
    }
};

class Fuhan : public PhaseChangeSkill
{
public:
    Fuhan() : PhaseChangeSkill("fuhan")
    {
        frequency = Limited;
        limit_mark = "@assisthan";
    }

    static QStringList GetAvailableGenerals(ServerPlayer *zuoci)
    {
        QSet<QString> all = Sanguosha->getLimitedGeneralNames("shu").toSet();
        Room *room = zuoci->getRoom();
        if (isNormalGameMode(room->getMode())
                || room->getMode().contains("_mini_")
                || room->getMode() == "custom_scenario")
            all.subtract(Config.value("Banlist/Roles", "").toStringList().toSet());
        else if (room->getMode() == "06_XMode") {
            foreach(ServerPlayer *p, room->getAlivePlayers())
                all.subtract(p->tag["XModeBackup"].toStringList().toSet());
        } else if (room->getMode() == "02_1v1") {
            all.subtract(Config.value("Banlist/1v1", "").toStringList().toSet());
            foreach(ServerPlayer *p, room->getAlivePlayers())
                all.subtract(p->tag["1v1Arrange"].toStringList().toSet());
        }
        if (room->getMode() == "08_hongyan")
            foreach (QString gen, Sanguosha->getLimitedGeneralNames("shu"))
                if (Sanguosha->getGeneral(gen)->isMale())
                    all.remove(gen);

        QSet<QString> huashen_set, room_set;
        foreach (ServerPlayer *player, room->getAlivePlayers()) {
            QVariantList huashens = player->tag["Huashens"].toList();
            foreach(QVariant huashen, huashens)
                huashen_set << huashen.toString();

            QString name = player->getGeneralName();
            if (Sanguosha->isGeneralHidden(name)) {
                QString fname = Sanguosha->getMainGeneral(name);
                if (!fname.isEmpty()) name = fname;
            }
            room_set << name;

            if (!player->getGeneral2()) continue;

            name = player->getGeneral2Name();
            if (Sanguosha->isGeneralHidden(name)) {
                QString fname = Sanguosha->getMainGeneral(name);
                if (!fname.isEmpty()) name = fname;
            }
            room_set << name;
        }

        static QSet<QString> banned;
        if (banned.isEmpty())
            banned << "zhaoxiang";

        return (all - banned - huashen_set - room_set).toList();
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target)
               && target->getPhase() == Player::RoundStart && target->getMark(limit_mark) > 0;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        if (room->askForSkillInvoke(player, objectName())) {
            player->broadcastSkillInvoke(objectName());
            room->removePlayerMark(player, limit_mark);
            int x = player->getMark("#plumshadow");
            room->setPlayerMark(player, "#plumshadow", 0);
            player->drawCards(x, objectName());

            QList<const Skill *> skills = player->getVisibleSkillList();
            QStringList detachList, acquireList;
            foreach (const Skill *skill, skills) {
                if (!skill->isAttachedLordSkill())
                    detachList.append("-" + skill->objectName());
            }
            room->handleAcquireDetachSkills(player, detachList);
            QStringList all_generals = GetAvailableGenerals(player);
            qShuffle(all_generals);
            if (all_generals.isEmpty()) return false;
            QStringList general_list = all_generals.mid(0, qMin(5, all_generals.length()));
            QString general_name = room->askForGeneral(player, general_list, true, "fuhan");
            const General *general = Sanguosha->getGeneral(general_name);
            foreach (const Skill *skill, general->getVisibleSkillList()) {
                if (skill->isLordSkill() && !player->isLord())
                    continue;

                acquireList.append(skill->objectName());
            }
            room->handleAcquireDetachSkills(player, acquireList);
            room->setPlayerProperty(player, "kingdom", general->getKingdom());
            player->setGender(general->getGender());
            player->setGeneralName(general_name);
            room->broadcastProperty(player, "general");


            int maxhp = qBound(2, player->getMark("plumshadow"), 8);

            room->setPlayerProperty(player, "maxhp", maxhp);

            room->recover(player, RecoverStruct(player));
        }
        return false;
    }
};

class Xiashu : public TriggerSkill
{
public:
    Xiashu() : TriggerSkill("xiashu")
    {
        events << EventPhaseStart;
        view_as_skill = new dummyVS;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() != Player::Play || player->isKongcheng()) return false;
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@xiashu-invoke", true, true);
        if (target) {
            player->broadcastSkillInvoke(objectName());
            DummyCard *handcards = player->wholeHandCards();
            handcards->deleteLater();
            CardMoveReason r(CardMoveReason::S_REASON_GIVE, player->objectName(), target->objectName(), objectName(), QString());
            room->obtainCard(target, handcards, r, false);
            if (target->isKongcheng()) return false;
            const Card *to_show = room->askForExchange(target, objectName(), 1000, 1, false, "@xiashu-show:" + player->objectName());
            room->showCard(target, to_show->getSubcards());
            DummyCard *other = new DummyCard;
            foreach (const Card *card, target->getHandcards()) {
                int id = card->getEffectiveId();
                if (!to_show->getSubcards().contains(id))
                    other->addSubcard(id);
            }
            QStringList choices;
            choices << "showcards";
            if (!other->getSubcards().isEmpty())
                choices << "othercards";
            QString choice = room->askForChoice(player, objectName(), choices.join("+"), QVariant(), "@xiashu-choice:" + target->objectName(), "showcards+othercards");
            CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, player->objectName());
            if (choice == "showcards")
                room->obtainCard(player, to_show, reason, true);
            else
                room->obtainCard(player, other, reason, false);
            delete other;
        }
        return false;
    }
};

class Kuanshi : public TriggerSkill
{
public:
    Kuanshi() : TriggerSkill("kuanshi")
    {
        events << EventPhaseStart << DamageInflicted;
        view_as_skill = new dummyVS;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            switch (player->getPhase()) {
            case Player::Finish: {
                if (!TriggerSkill::triggerable(player)) break;
                ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@kuanshi-invoke", true);
                if (target) {
                    LogMessage log;
                    log.type = "#InvokeSkill";
                    log.from = player;
                    log.arg = objectName();
                    room->sendLog(log);
                    room->notifySkillInvoked(player, objectName());
                    player->broadcastSkillInvoke(objectName());
                    player->tag["KuanshiTarget"] = QVariant::fromValue(target);
                }
                break;
            }
            case Player::RoundStart: {
                player->tag.remove("KuanshiTarget");
                if (player->getMark("#kuanshi") > 0) {
                    LogMessage log;
                    log.type = "#SkillForce";
                    log.from = player;
                    log.arg = objectName();
                    room->sendLog(log);
                    room->removePlayerTip(player, "#kuanshi");
                    player->skip(Player::Draw);
                }
                break;
            }
            default:
                break;
            }
        } else if (triggerEvent == DamageInflicted) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.damage > 1) {
                foreach (ServerPlayer *kanze, room->getAlivePlayers()) {
                    if (kanze->tag["KuanshiTarget"].value<ServerPlayer *>() != player) continue;
                    LogMessage log;
                    log.type = "#SkillForce";
                    log.from = kanze;
                    log.arg = objectName();
                    room->sendLog(log);
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, kanze->objectName(), player->objectName());
                    kanze->tag.remove("KuanshiTarget");
                    room->addPlayerTip(kanze, "#kuanshi");
                    return true;
                }
            }
        }
        return false;
    }
};

LianjiCard::LianjiCard()
{
    will_throw = false;
    will_sort = false;
    handling_method = Card::MethodNone;
}

bool LianjiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.length() < 2 && to_select != Self;
}

bool LianjiCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return targets.length() == 2;
}

void LianjiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.at(0);
    ServerPlayer *victim = targets.at(1);

    QList<int> weapons;
    foreach (int card_id, room->getDrawPile()) {
        const Card *card = Sanguosha->getCard(card_id);
        if (card->isKindOf("Weapon")) {
            weapons << card_id;
        }
    }

    if (weapons.isEmpty()) {
        LogMessage log;
        log.type = "$SearchFailed";
        log.from = source;
        log.arg = "weapon";
        room->sendLog(log);
    } else {
        int index = qrand() % weapons.length();
        int id = weapons.at(index);
        Card *weapon = Sanguosha->getCard(id);
        if (weapon->objectName() == "qinggang_sword") {
            int sword_id = -1;
            Package *package = PackageAdder::packages()["BestLoyalistCard"];
            if (package) {
                QList<Card *> all_cards = package->findChildren<Card *>();
                foreach (Card *card, all_cards) {
                    if (card->objectName() == "treasured_sword") {
                        sword_id = card->getEffectiveId();
                        break;
                    }
                }
            }
            if (sword_id > 0) {
                LogMessage log;
                log.type = "#RemoveCard";
                log.card_str = weapon->toString();
                room->sendLog(log);
                room->getDrawPile().removeOne(id);
                room->doBroadcastNotify(QSanProtocol::S_COMMAND_UPDATE_PILE, QVariant(room->getDrawPile().length()));
                weapon = Sanguosha->getCard(sword_id);
                log.type = "#AddCard";
                log.card_str = weapon->toString();
                room->sendLog(log);
                room->setCardMapping(sword_id, NULL, Player::DrawPile);
            }
        }
        CardMoveReason reason(CardMoveReason::S_REASON_USE, target->objectName(), target->objectName(), QString(), QString());
        room->moveCardTo(weapon, NULL, Player::PlaceTable, reason, true);
        room->useCard(CardUseStruct(weapon, target, target));
    }

    QStringList card_names;
    card_names << "slash" << "duel" << "fire_attack" << "savage_assault" << "archery_attack";

    QString card_name = card_names.at(qrand() % card_names.length());

    Card *to_use = Sanguosha->cloneCard(card_name, Card::NoSuit, 0, QStringList("Global_NoDistanceChecking"));
    to_use->setSkillName("_lianji");
    to_use->setTag("LianjiUser", QVariant::fromValue(source));
    if (to_use->isAvailable(target) && !room->isProhibited(target, victim, to_use) && to_use->targetFilter(QList<const Player *>(), victim, target)) {
        if (to_use->targetFixed())
            room->useCard(CardUseStruct(to_use, target, QList<ServerPlayer *>()));
        else
            room->useCard(CardUseStruct(to_use, target, victim));
    }
}

class LianjiViewAsSkill : public ZeroCardViewAsSkill
{
public:
    LianjiViewAsSkill() : ZeroCardViewAsSkill("lianji")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("LianjiCard");
    }

    virtual const Card *viewAs() const
    {
        return new LianjiCard;
    }
};

class Lianji : public TriggerSkill
{
public:
    Lianji() : TriggerSkill("lianji")
    {
        events << DamageDone;
        view_as_skill = new LianjiViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *, QVariant &data, ServerPlayer *&ask_who) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && !damage.transfer && !damage.chain && damage.by_user) {
            ServerPlayer *wangyun = damage.card->tag["LianjiUser"].value<ServerPlayer *>();
            if (wangyun && wangyun->isAlive() && wangyun->hasSkill(objectName(), true)) {
                ask_who = wangyun;
                return nameList();
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *wangyun) const
    {
        room->sendCompulsoryTriggerLog(wangyun, objectName());
        DamageStruct damage = data.value<DamageStruct>();
        room->addPlayerMark(wangyun, "#lianji", damage.damage);
        return false;
    }

};

class Moucheng : public TriggerSkill
{
public:
    Moucheng() : TriggerSkill("moucheng")
    {
        events << Damage;
        frequency = Wake;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *, QVariant &) const
    {
        TriggerList skill_list;
        QList<ServerPlayer *> wangyuns = room->findPlayersBySkillName(objectName());
        foreach (ServerPlayer *wangyun, wangyuns) {
            if (wangyun->getMark(objectName()) == 0 && wangyun->getMark("#lianji") > 2)
                skill_list.insert(wangyun, nameList());
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *wangyun) const
    {
        room->sendCompulsoryTriggerLog(wangyun, objectName());
        wangyun->broadcastSkillInvoke(objectName());
        room->setPlayerMark(wangyun, objectName(), 1);
        if (room->changeMaxHpForAwakenSkill(wangyun, 1)) {
            room->recover(wangyun, RecoverStruct(wangyun));
            room->handleAcquireDetachSkills(wangyun, "-lianji|jingong");

        }
        return false;
    }
};

SmileDagger::SmileDagger(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("SmileDagger");
}

void SmileDagger::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    effect.to->drawCards(effect.to->getLostHp(), objectName());
    room->damage(DamageStruct(this, effect.from, effect.to, 1));
}

HoneyTrap::HoneyTrap(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("HoneyTrap");
}

bool HoneyTrap::targetRated(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->isMale() && !to_select->isKongcheng() && to_select != Self;
}

void HoneyTrap::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *source = effect.from;
    Room *room = source->getRoom();
    ServerPlayer *target = effect.to;
    foreach (ServerPlayer *female, room->getAllPlayers()) {
        if (female->isFemale() && target->isAlive() && !target->isNude()) {
            int card_id = room->askForCardChosen(female, target, "he", objectName());
            CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, female->objectName());
            room->obtainCard(female, Sanguosha->getCard(card_id), reason, false);
            if (!female->isNude() && source->isAlive()) {
                const Card *card = room->askForExchange(female, objectName(), 1, 1, false, "@honeytrap-give::" + source->objectName());
                CardMoveReason reason(CardMoveReason::S_REASON_GIVE, female->objectName(),
                                      source->objectName(), objectName(), QString());
                reason.m_playerId = source->objectName();
                room->moveCardTo(card, female, source, Player::PlaceHand, reason);
                delete card;
            }
        }
    }
    if (source->isAlive() && target->isAlive()) {
        if (source->getHandcardNum() > target->getHandcardNum())
            room->damage(DamageStruct(this, target, source, 1));
        else if (source->getHandcardNum() < target->getHandcardNum())
            room->damage(DamageStruct(this, source, target, 1));
    }
}

JingongCard::JingongCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "jingong";
}

bool JingongCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Card *mutable_card = Sanguosha->cloneCard(Self->tag["jingong"].toString());
    if (mutable_card) {
        mutable_card->addSubcards(this->subcards);
        mutable_card->setCanRecast(false);
        mutable_card->deleteLater();
    }
    return mutable_card && mutable_card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, mutable_card, targets);
}

bool JingongCard::targetFixed() const
{
    Card *mutable_card = Sanguosha->cloneCard(Self->tag["jingong"].toString());
    if (mutable_card) {
        mutable_card->addSubcards(this->subcards);
        mutable_card->setCanRecast(false);
        mutable_card->deleteLater();
    }
    return mutable_card && mutable_card->targetFixed();
}

bool JingongCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    Card *mutable_card = Sanguosha->cloneCard(Self->tag["jingong"].toString());
    if (mutable_card) {
        mutable_card->addSubcards(this->subcards);
        mutable_card->setCanRecast(false);
        mutable_card->deleteLater();
    }
    return mutable_card && mutable_card->targetsFeasible(targets, Self);
}

const Card *JingongCard::validate(CardUseStruct &) const
{
    Card *use_card = Sanguosha->cloneCard(Self->tag["jingong"].toString());
    use_card->setSkillName("jingong");
    use_card->addSubcards(this->subcards);
    return use_card;
}

class JingongViewAsSkill : public OneCardViewAsSkill
{
public:
    JingongViewAsSkill() : OneCardViewAsSkill("jingong")
    {
        filter_pattern = "EquipCard,Slash";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        JingongCard *trick = new JingongCard;
        trick->addSubcard(originalCard->getId());
        return trick;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("JingongCard");
    }
};

class Jingong : public TriggerSkill
{
public:
    Jingong() : TriggerSkill("jingong")
    {
        events << PreCardUsed << EventPhaseChanging << EventPhaseStart << EventAcquireSkill;
        view_as_skill = new JingongViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *wangyun, QVariant &data) const
    {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getSkillName() != objectName()) return false;
            room->setPlayerFlag(wangyun, "jingongInvoked");
        } else if (triggerEvent == EventPhaseStart) {
            if (wangyun->getPhase() == Player::Play && wangyun->hasSkill("jingong", true))
                createRandomTrickCardName(room, wangyun);
        } else if (triggerEvent == EventAcquireSkill) {
            if (data.toString() == objectName())
                createRandomTrickCardName(room, wangyun);
        } else if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                if (wangyun->getMark("damage_point_round") == 0 && wangyun->hasFlag("jingongInvoked")) {
                    LogMessage log;
                    log.type = "#SkillForce";
                    log.from = wangyun;
                    log.arg = objectName();
                    room->sendLog(log);
                    room->notifySkillInvoked(wangyun, objectName());
                    wangyun->broadcastSkillInvoke(objectName());
                    room->loseHp(wangyun);
                }
            }
        }
        return false;
    }

    QString getSelectBox() const
    {
        return Self->property("jingong_tricks").toString();
    }

    bool buttonEnabled(const QString &button_name, const QList<const Card *> &selected, const QList<const Player *> &) const
    {
        if (button_name.isEmpty()) return true;
        if (selected.isEmpty()) return false;
        return true;
    }

private:
    void createRandomTrickCardName(Room *room, ServerPlayer *player) const
    {
        QStringList card_list, all_tricks;
        QList<const TrickCard *> tricks = Sanguosha->findChildren<const TrickCard *>();
        foreach (const TrickCard *card, tricks) {
            if (!ServerInfo.Extensions.contains("!" + card->getPackage()) && card->isNDTrick()
                    && !all_tricks.contains(card->objectName()) && !card->isKindOf("Nullification") && !card->isKindOf("Escape"))
                all_tricks.append(card->objectName());
        }
        qShuffle(all_tricks);

        foreach (QString name, all_tricks) {
            card_list.append(name);
            if (card_list.length() >= 2) break;
        }

        QString exclusive_trick;
        if (qrand() % 2 == 0)
            exclusive_trick = "SmileDagger";
        else
            exclusive_trick = "HoneyTrap";
        card_list.append(exclusive_trick);
        room->setPlayerProperty(player, "jingong_tricks", card_list.join("+"));

    }
};

class Fuji : public TriggerSkill
{
public:
    Fuji() : TriggerSkill("fuji")
    {
        events << CardUsed;
        frequency = Compulsory;
    }
    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") || use.card->isNDTrick()) {
            QStringList fuji_tag = use.card->tag["Fuji_tag"].toStringList();
            bool invoke_skill = false;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->distanceTo(player) == 1) {
                    fuji_tag << p->objectName();
                    invoke_skill = true;
                }
            }
            if (invoke_skill) {
                room->sendCompulsoryTriggerLog(player, objectName());
                player->broadcastSkillInvoke(objectName());
                use.card->setTag("Fuji_tag", fuji_tag);
            }
        }
        return false;
    }
};

class Jiaozi : public TriggerSkill
{
public:
    Jiaozi() : TriggerSkill("jiaozi")
    {
        events << DamageCaused << DamageInflicted;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->getHandcardNum() >= player->getHandcardNum()) {
                return false;
            }
        }
        room->sendCompulsoryTriggerLog(player, objectName());
        player->broadcastSkillInvoke(objectName());
        damage.damage++;
        data = QVariant::fromValue(damage);
        return false;
    }
};

class Wenji : public TriggerSkill
{
public:
    Wenji() : TriggerSkill("wenji")
    {
        events << EventPhaseStart << CardUsed;
        view_as_skill = new dummyVS;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Play && TriggerSkill::triggerable(player)) {
                QList<ServerPlayer *> targets;
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (!p->isNude())
                        targets << p;
                }
                if (!targets.isEmpty())
                    return nameList();
            }
        }
        return QStringList();
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::NotActive) {
                QList<ServerPlayer *> players = room->getAlivePlayers();
                foreach (ServerPlayer *p, players) {
                    p->tag.remove(objectName());
                }
            }
        }

        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            QStringList wenji_list = player->tag[objectName()].toStringList();

            QString classname = use.card->getClassName();
            if (use.card->isKindOf("Slash")) classname = "Slash";
            if (!wenji_list.contains(classname)) return;


            QStringList fuji_tag = use.card->tag["Fuji_tag"].toStringList();

            QList<ServerPlayer *> players = room->getOtherPlayers(player);
            foreach (ServerPlayer *p, players) {
                fuji_tag << p->objectName();
            }
            use.card->setTag("Fuji_tag", fuji_tag);
        }
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (triggerEvent == EventPhaseStart) {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (!p->isNude())
                    targets << p;
            }
            if (targets.isEmpty()) return false;

            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@wenji-invoke", true, true);
            if (target) {
                player->broadcastSkillInvoke(objectName());
                const Card *card = room->askForExchange(target, objectName(), 1, 1, true, "@wenji-give:" + player->objectName());

                CardMoveReason reason(CardMoveReason::S_REASON_GIVE, target->objectName(), player->objectName(), objectName(), QString());
                reason.m_playerId = player->objectName();
                room->moveCardTo(card, player, Player::PlaceHand, reason, true);


                const Card *record_card = Sanguosha->getCard(card->getEffectiveId());

                if (!player->getHandcards().contains(record_card)) return false;

                QString classname = record_card->getClassName();
                if (record_card->isKindOf("Slash")) classname = "Slash";
                QStringList list = player->tag[objectName()].toStringList();
                list.append(classname);
                player->tag[objectName()] = list;

            }
        }

        return false;
    }
};

class Tunjiang : public TriggerSkill
{
public:
    Tunjiang() : TriggerSkill("tunjiang")
    {
        events << EventPhaseStart << CardUsed;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            foreach (ServerPlayer *p, use.to)
                if (p != player) {
                    room->setPlayerFlag(player, "TunjiangDisabled");
                    return;
                }
        }
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (!TriggerSkill::triggerable(player)) return QStringList();
            if (player->getPhase() != Player::Finish) return QStringList();
            if (player->hasFlag("TunjiangDisabled") || player->hasSkipped(Player::Play)) return QStringList();
            return nameList();
        }
        return QStringList();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (room->askForSkillInvoke(player, objectName())) {
            player->broadcastSkillInvoke(objectName());
            QSet<QString> kingdomSet;
            foreach (ServerPlayer *p, room->getAlivePlayers())
                kingdomSet.insert(p->getKingdom());

            player->drawCards(kingdomSet.count(), objectName());
        }
        return false;
    }
};

class Qingzhong : public TriggerSkill
{
public:
    Qingzhong() : TriggerSkill("qingzhong")
    {
        events << EventPhaseStart << EventPhaseEnd;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() != Player::Play) return false;
        if (triggerEvent == EventPhaseStart && TriggerSkill::triggerable(player)) {
            if (room->askForSkillInvoke(player, objectName())) {
                player->broadcastSkillInvoke(objectName());
                player->drawCards(2, objectName());
                player->setFlags("qingzhongUsed");
            }
        } else if (triggerEvent == EventPhaseEnd && player->hasFlag("qingzhongUsed") && !player->isKongcheng()) {
            QList<ServerPlayer *> targets, players = room->getOtherPlayers(player);
            int x = player->getHandcardNum();
            foreach (ServerPlayer *p, players) {
                if (p->getHandcardNum() > x) continue;
                if (p->getHandcardNum() < x)
                    targets.clear();
                x = p->getHandcardNum();
                targets << p;

            }
            if (targets.isEmpty()) return false;
            room->sendCompulsoryTriggerLog(player, objectName());
            player->broadcastSkillInvoke(objectName());
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@qingzhong-choose", x == player->getHandcardNum());
            if (target)
                room->swapCards(player, target, "h", objectName());
        }
        return false;
    }
};

class WeijingViewAsSkill : public ZeroCardViewAsSkill
{
public:
    WeijingViewAsSkill() : ZeroCardViewAsSkill("weijing")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return Slash::IsAvailable(player) && player->getMark("weijing_times") == 0;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return (pattern == "jink" || pattern == "slash") && player->getMark("weijing_times") == 0;
    }

    virtual const Card *viewAs() const
    {
        QString card_name;
        switch (Sanguosha->currentRoomState()->getCurrentCardUseReason()) {
        case CardUseStruct::CARD_USE_REASON_PLAY: {
            card_name = "slash";
            break;
        }
        case CardUseStruct::CARD_USE_REASON_RESPONSE_USE: {
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            card_name = pattern;
        }
        default:
            break;
        }
        if (card_name.isEmpty()) return NULL;
        Card *use_card = Sanguosha->cloneCard(card_name);
        use_card->setSkillName(objectName());
        return use_card;
    }
};

class Weijing : public TriggerSkill
{
public:
    Weijing() : TriggerSkill("weijing")
    {
        events << PreCardUsed << PreCardResponded << RoundStart;
        view_as_skill = new WeijingViewAsSkill;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == RoundStart) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                room->setPlayerMark(p, "weijing_times", 0);
            }
        }
        const Card *card = NULL;
        if (triggerEvent == PreCardUsed)
            card = data.value<CardUseStruct>().card;
        else {
            CardResponseStruct response = data.value<CardResponseStruct>();
            if (response.m_isUse)
                card = response.m_card;
        }
        if (card && card->getSkillName() == objectName() && card->getHandlingMethod() == Card::MethodUse) {
            room->addPlayerMark(player, "weijing_times");
        }
        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        if (card->isKindOf("Slash"))
            return 1;
        else
            return 2;
    }
};




class JianjieViewAsSkill : public ZeroCardViewAsSkill
{
public:
    JianjieViewAsSkill() : ZeroCardViewAsSkill("jianjie")
    {
    }

    virtual const Card *viewAs() const
    {
        QString choice = Self->tag["jianjie"].toString();
        if (choice.isEmpty()) return NULL;
        return new JianjieCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->hasUsed("JianjieCard") || player->getMark("Global_TurnCount") == 1) return false;
        QList<const Player *> players = player->getAliveSiblings();
        players << player;
        foreach(const Player *sib, players) {
            if (sib->getMark("@loong_mark") > 0 || sib->getMark("@phoenix_mark") > 0)
                return true;
        }
        return false;
    }
};

class Jianjie : public TriggerSkill
{
public:
    Jianjie() : TriggerSkill("jianjie")
    {
        events << EventPhaseStart << Death;
        view_as_skill = new JianjieViewAsSkill;
    }

    static void MarkChange(ServerPlayer *player)
    {
        Room *room = player->getRoom();
        bool loong = player->getMark("@loong_mark") > 0, phoenix = player->getMark("@phoenix_mark") > 0;

        if (loong)
            room->acquireSkill(player, "huojii");
        else
            room->detachSkillFromPlayer(player, "huojii");

        if (phoenix)
            room->acquireSkill(player, "lianhuani");
        else
            room->detachSkillFromPlayer(player, "lianhuani");

        if (loong && phoenix)
            room->acquireSkill(player, "yeyani");
        else
            room->detachSkillFromPlayer(player, "yeyani");
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart && player->getMark("Global_TurnCount") == 1) {
            return QStringList("jianjie!");
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            ServerPlayer *dead = death.who;
            if (dead->getMark("@loong_mark") > 0 || dead->getMark("@phoenix_mark") > 0)
                return nameList();
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == EventPhaseStart) {
            QList<ServerPlayer *> all_players = room->getOtherPlayers(player);
            if (all_players.isEmpty()) return false;
            room->sendCompulsoryTriggerLog(player, objectName());
            player->broadcastSkillInvoke(objectName());
            ServerPlayer *target = room->askForPlayerChosen(player, all_players, objectName(), "@jianjie1-target");
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());
            target->gainMark("@loong_mark");
            room->acquireSkill(target, "huojii");
            all_players.removeOne(target);
            if (all_players.isEmpty()) return false;
            target = room->askForPlayerChosen(player, all_players, objectName(), "@jianjie2-target");
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());
            target->gainMark("@phoenix_mark");
            room->acquireSkill(target, "lianhuani");
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            ServerPlayer *dead = death.who;
            if (dead->getMark("@loong_mark") > 0) {
                ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@jianjie1-target", true, true);
                if (target) {
                    player->broadcastSkillInvoke(objectName());
                    dead->loseMark("@loong_mark");
                    MarkChange(dead);
                    target->gainMark("@loong_mark");
                    MarkChange(target);
                }
            }
            if (dead->getMark("@phoenix_mark") > 0) {
                ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@jianjie2-target", true, true);
                if (target) {
                    player->broadcastSkillInvoke(objectName());
                    dead->loseMark("@phoenix_mark");
                    MarkChange(dead);
                    target->gainMark("@phoenix_mark");
                    MarkChange(target);
                }
            }
        }
        return false;
    }

    QString getSelectBox() const
    {
        return "loong_mark+phoenix_mark";
    }

    bool buttonEnabled(const QString &button_name, const QList<const Card *> &, const QList<const Player *> &) const
    {
        if (button_name.isEmpty()) return true;
        QList<const Player *> players = Self->getAliveSiblings();
        players << Self;
        foreach(const Player *sib, players) {
            if (sib->getMark("@" + button_name) > 0)
                return true;
        }
        return false;

    }
};

JianjieCard::JianjieCard()
{
    m_skillName = "jianjie";
    will_sort = false;
}

bool JianjieCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    QString mark_name = Self->tag["jianjie"].toString();

    if (targets.isEmpty()) return to_select->getMark("@" + mark_name) > 0;
    else if (targets.length() == 1) return to_select->getMark("@" + mark_name) == 0;

    return false;
}

bool JianjieCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return targets.length() == 2;
}

void JianjieCard::use(Room *, ServerPlayer *, QList<ServerPlayer *> &targets) const
{
    QString mark_name = "@" + user_string;
    ServerPlayer *first = targets.first(), *second = targets.last();
    first->loseMark(mark_name);
    Jianjie::MarkChange(first);
    second->gainMark(mark_name);
    Jianjie::MarkChange(second);
}

class Huojii : public OneCardViewAsSkill
{
public:
    Huojii() : OneCardViewAsSkill("huojii")
    {
        filter_pattern = ".|red|.|hand";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@loong_mark") > 0 && player->usedTimes("ViewAsSkill_huojiiCard") < 3;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        FireAttack *fire_attack = new FireAttack(originalCard->getSuit(), originalCard->getNumber());
        fire_attack->addSubcard(originalCard->getId());
        fire_attack->setSkillName(objectName());
        return fire_attack;
    }
};

class Lianhuani : public OneCardViewAsSkill
{
public:
    Lianhuani() : OneCardViewAsSkill("lianhuani")
    {
        filter_pattern = ".|club|.|hand";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@phoenix_mark") > 0 && player->usedTimes("ViewAsSkill_lianhuaniCard") < 3;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        IronChain *chain = new IronChain(originalCard->getSuit(), originalCard->getNumber());
        chain->addSubcard(originalCard);
        chain->setSkillName(objectName());
        return chain;
    }
};

void YeyaniCard::damage(ServerPlayer *shenzhouyu, ServerPlayer *target, int point) const
{
    shenzhouyu->getRoom()->damage(DamageStruct("yeyani", shenzhouyu, target, point, DamageStruct::Fire));
}

GreatYeyaniCard::GreatYeyaniCard()
{
    m_skillName = "yeyani";
}

bool GreatYeyaniCard::targetFilter(const QList<const Player *> &, const Player *, const Player *) const
{
    Q_ASSERT(false);
    return false;
}

bool GreatYeyaniCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    if (subcards.length() != 4) return false;
    QList<Card::Suit> allsuits;
    foreach (int cardId, subcards) {
        const Card *card = Sanguosha->getCard(cardId);
        if (allsuits.contains(card->getSuit())) return false;
        allsuits.append(card->getSuit());
    }

    //We can only assign 2 damage to one player
    //If we select only one target only once, we assign 3 damage to the target
    if (targets.toSet().size() == 1)
        return true;
    else if (targets.toSet().size() == 2)
        return targets.size() == 3;
    return false;
}

bool GreatYeyaniCard::targetFilter(const QList<const Player *> &targets, const Player *to_select,
                                   const Player *, int &maxVotes) const
{
    int i = 0;
    foreach(const Player *player, targets)
        if (player == to_select) i++;
    maxVotes = qMax(3 - targets.size(), 0) + i;
    return maxVotes > 0;
}

void GreatYeyaniCard::use(Room *room, ServerPlayer *shenzhouyu, QList<ServerPlayer *> &targets) const
{
    int criticaltarget = 0;
    int totalvictim = 0;
    QMap<ServerPlayer *, int> map;

    foreach(ServerPlayer *sp, targets)
        map[sp]++;

    if (targets.size() == 1)
        map[targets.first()] += 2;

    foreach (ServerPlayer *sp, map.keys()) {
        if (map[sp] > 1) criticaltarget++;
        totalvictim++;
    }
    if (criticaltarget > 0) {
        room->removePlayerMark(shenzhouyu, "@loong_mark");
        room->removePlayerMark(shenzhouyu, "@phoenix_mark");
        Jianjie::MarkChange(shenzhouyu);
        room->loseHp(shenzhouyu, 3);

        QList<ServerPlayer *> targets = map.keys();
        room->sortByActionOrder(targets);
        foreach(ServerPlayer *sp, targets)
            damage(shenzhouyu, sp, map[sp]);
    }
}

SmallYeyaniCard::SmallYeyaniCard()
{
    m_skillName = "yeyani";
}

bool SmallYeyaniCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return !targets.isEmpty();
}

bool SmallYeyaniCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.length() < 3;
}

void SmallYeyaniCard::use(Room *room, ServerPlayer *shenzhouyu, QList<ServerPlayer *> &targets) const
{
    room->removePlayerMark(shenzhouyu, "@loong_mark");
    room->removePlayerMark(shenzhouyu, "@phoenix_mark");
    Jianjie::MarkChange(shenzhouyu);
    Card::use(room, shenzhouyu, targets);
}

void SmallYeyaniCard::onEffect(const CardEffectStruct &effect) const
{
    damage(effect.from, effect.to, 1);
}

class Yeyani : public ViewAsSkill
{
public:
    Yeyani() : ViewAsSkill("yeyani")
    {
        frequency = Limited;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@loong_mark") > 0 && player->getMark("@phoenix_mark") > 0;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (selected.length() >= 4)
            return false;

        if (to_select->isEquipped())
            return false;

        if (Self->isJilei(to_select))
            return false;

        foreach (const Card *item, selected) {
            if (to_select->getSuit() == item->getSuit())
                return false;
        }

        return true;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == 0)
            return new SmallYeyaniCard;
        if (cards.length() != 4)
            return NULL;

        GreatYeyaniCard *card = new GreatYeyaniCard;
        card->addSubcards(cards);

        return card;
    }
};

class Chenghao : public TriggerSkill
{
public:
    Chenghao() : TriggerSkill("chenghao")
    {
        events << Damaged;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;

        DamageStruct damage = data.value<DamageStruct>();
        if (player->isDead() || damage.nature == DamageStruct::Normal || damage.chain || !damage.flags.contains("is_chained"))
            return skill_list;

        QList<ServerPlayer *> simahuis = room->findPlayersBySkillName(objectName());
        foreach(ServerPlayer *simahui, simahuis)
            skill_list.insert(simahui, nameList());

        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *simahui) const
    {
        if (simahui->askForSkillInvoke(this)) {
            simahui->broadcastSkillInvoke(objectName());
            int x = 1;
            QList<ServerPlayer *> all_players = room->getAlivePlayers();
            foreach (ServerPlayer *p, all_players) {
                if (p->isChained()) x++;
            }
            QList<int> yiji_cards = room->getNCards(x, true);
            room->askForRende(simahui, yiji_cards, objectName(), true, false, true, -1, 0,
                              room->getOtherPlayers(simahui), CardMoveReason(), "@chenghao-give", "#chenghao");
        }
        return false;
    }
};

class Yinshi : public TriggerSkill
{
public:
    Yinshi() : TriggerSkill("yinshi")
    {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player) || player->getMark("@loong_mark") > 0 || player->getMark("@phoenix_mark") > 0
                || player->getArmor()) return QStringList();
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature != DamageStruct::Normal || (damage.card && damage.card->getTypeId() == Card::TypeTrick))
            return nameList();
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        player->broadcastSkillInvoke(objectName());
        room->preventDamage(data.value<DamageStruct>());
        return true;
    }
};

class Wuniang : public TriggerSkill
{
public:
    Wuniang() : TriggerSkill("wuniang")
    {
        events << CardUsed << CardResponded;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        const Card *cardstar = NULL;
        if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            cardstar = use.card;
        } else {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            cardstar = resp.m_card;
        }
        if (cardstar && cardstar->isKindOf("Slash")) {
            QList<ServerPlayer *> other_players = room->getOtherPlayers(player);
            foreach (ServerPlayer *p, other_players) {
                if (!p->isNude())
                    return nameList();
            }
        }

        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        QList<ServerPlayer *> targets, other_players = room->getOtherPlayers(player);
        foreach (ServerPlayer *p, other_players) {
            if (player->canGetCard(p, "he"))
                targets << p;
        }
        if (targets.isEmpty()) return false;
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@wuniang-target", true, true);
        if (target) {
            player->broadcastSkillInvoke(objectName());
            int card_id = room->askForCardChosen(player, target, "he", objectName(), false, Card::MethodGet);
            CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, player->objectName());
            room->obtainCard(player, Sanguosha->getCard(card_id), reason, false);
            target->drawCards(1, objectName());
            QList<ServerPlayer *> all_players = room->getAlivePlayers();
            foreach (ServerPlayer *p, all_players) {
                if (p->getGeneralName() == "guansuo" || (p->getGeneral2() && p->getGeneral2Name() == "guansuo")) {
                    p->drawCards(1, objectName());
                }
            }
        }
        return false;
    }
};


class Xushen : public TriggerSkill
{
public:
    Xushen() : TriggerSkill("xushen")
    {
        events << QuitDying << HpRecover;
        frequency = Limited;
        limit_mark = "@limit_xushen";
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == HpRecover && room->getCurrentDyingPlayer() == player) {
            RecoverStruct recover = data.value<RecoverStruct>();
            if (player->getHp() > 0 && recover.recover >= player->getHp()) {
                ServerPlayer *saver;
                CardEffectStruct effect = recover.effect;
                if (effect.card && effect.card->isKindOf("Peach") && effect.from)
                    saver = effect.from;
                QVariantList record = player->tag["XushenRecord"].toList();
                record << QVariant::fromValue(saver);
                player->tag["XushenRecord"] = record;
            }
        } else if (triggerEvent == QuitDying) {
            QVariantList record = player->tag["XushenRecord"].toList();
            if (record.isEmpty()) return;
            QVariant p_data = record.takeLast();
            player->tag["XushenRecord"] = record;
            ServerPlayer *saver = p_data.value<ServerPlayer *>();
            if (saver) {
                DyingStruct dying = data.value<DyingStruct>();
                dying.flags << "xushen->" + saver->objectName();
                data = QVariant::fromValue(dying);
            }
        }
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent != QuitDying || !TriggerSkill::triggerable(player) || player->getMark(limit_mark) == 0) return QStringList();
        QList<ServerPlayer *> all_players = room->getAlivePlayers();
        foreach (ServerPlayer *p, all_players) {
            if (p->getGeneralName() == "guansuo" || (p->getGeneral2() && p->getGeneral2Name() == "guansuo")) {
                return QStringList();
            }
        }
        DyingStruct dying = data.value<DyingStruct>();
        foreach (QString name_str, dying.flags) {
            if (name_str.startsWith("xushen")) {
                ServerPlayer *saver = room->findPlayer(name_str.mid(8));
                if (saver && saver->isMale())
                    return QStringList("xushen!");
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DyingStruct dying = data.value<DyingStruct>();
        ServerPlayer *saver = NULL;
        foreach (QString name_str, dying.flags) {
            if (name_str.startsWith("xushen")) {
                ServerPlayer *p = room->findPlayer(name_str.mid(8));
                if (p && p->isMale())
                    saver = p;
                break;
            }
        }
        if (saver && (room->askForChoice(saver, objectName(), "yes+no", data, "@xushen-invoke:" + player->objectName()) == "yes")) {
            LogMessage log;
            log.type = "#InvokeOthersSkill";
            log.from = saver;
            log.to << player;
            log.arg = objectName();
            room->sendLog(log);
            room->notifySkillInvoked(player, objectName());
            player->broadcastSkillInvoke(objectName());
            room->removePlayerMark(player, limit_mark);

            QList<const Skill *> skills = saver->getVisibleSkillList();
            QStringList detachList, acquireList;
            foreach (const Skill *skill, skills) {
                if (!skill->isAttachedLordSkill())
                    detachList.append("-" + skill->objectName());
            }
            room->handleAcquireDetachSkills(saver, detachList);

            const General *general = Sanguosha->getGeneral("guansuo");
            foreach (const Skill *skill, general->getVisibleSkillList()) {
                if (skill->isLordSkill() && !saver->isLord())
                    continue;

                acquireList.append(skill->objectName());
            }
            room->handleAcquireDetachSkills(saver, acquireList);
            room->setPlayerProperty(saver, "kingdom", general->getKingdom());
            saver->setGender(general->getGender());
            saver->setGeneralName("guansuo");
            room->broadcastProperty(saver, "general");

            int maxhp = general->getMaxHp();
            if (saver->isLord()) maxhp++;

            if (saver->getMaxHp() != maxhp)
                room->setPlayerProperty(saver, "maxhp", maxhp);

            room->recover(player, RecoverStruct(player));
            room->acquireSkill(player, "zhennan");
        }
        return false;
    }
};

class Zhennan : public TriggerSkill
{
public:
    Zhennan() : TriggerSkill("zhennan")
    {
        events << TargetConfirmed;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("SavageAssault"))
            return nameList();
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@zhennan-target", true, true);
        if (target) {
            player->broadcastSkillInvoke(objectName());
            int x = qrand() % 3 + 1;
            room->damage(DamageStruct(objectName(), player, target, x));
        }
        return false;
    }
};

class Lingren : public TriggerSkill
{
public:
    Lingren() : TriggerSkill("lingren")
    {
        events << TargetSpecified << EventPhaseChanging << EventPhaseStart << DamageInflicted;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (triggerEvent == EventPhaseChanging)
            room->setPlayerMark(player, "lingrenUsed", 0);
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart && player->getMark("LingrenThree") > 0) {
            room->setPlayerMark(player, "LingrenThree", 0);
            room->handleAcquireDetachSkills(player, "-jianxiong|-xingshang");
        }
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == TargetSpecified && TriggerSkill::triggerable(player) && player->getPhase() == Player::Play
                && player->getMark("lingrenUsed") == 0) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") || use.card->isKindOf("Duel") ||
                    use.card->isKindOf("FireAttack") || use.card->isKindOf("SavageAssault") || use.card->isKindOf("ArcheryAttack")) {
                if (use.index == 0) {
                    foreach (ServerPlayer *to, use.to) {
                        if (to != player)
                            return nameList();
                    }
                }
            }
        } else if (triggerEvent == DamageInflicted) {
            QStringList trigger_list;
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card == NULL) return trigger_list;
            QStringList lingren_targets = damage.card->tag["LingrenTargets"].toStringList();
            foreach (QString name, lingren_targets) {
                if (player->objectName() == name)
                    trigger_list << "lingren!";
            }
            return trigger_list;
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            QList<ServerPlayer *> players = use.to;
            players.removeAll(player);
            ServerPlayer *target = room->askForPlayerChosen(player, players, objectName(), "@lingren-target", true, true);
            if (target) {
                room->addPlayerMark(player, "lingrenUsed");
                player->broadcastSkillInvoke(objectName());

                QString has_basic = "no", has_trick = "no", has_equip = "no";

                foreach (const Card *c, target->getHandcards()) {
                    switch (c->getTypeId()) {
                    case Card::TypeBasic:
                        has_basic = "yes";
                        break;
                    case Card::TypeTrick:
                        has_trick = "yes";
                        break;
                    case Card::TypeEquip:
                        has_equip = "yes";
                        break;
                    default:
                        break;
                    }
                }
                QVariant _data = QVariant::fromValue(target);
                int x = 0;
                if (room->askForChoice(player, objectName(), "yes+no", _data, "@lingren-basic::" + target->objectName()) == has_basic) x++;
                if (room->askForChoice(player, objectName(), "yes+no", _data, "@lingren-trick::" + target->objectName()) == has_trick) x++;
                if (room->askForChoice(player, objectName(), "yes+no", _data, "@lingren-equip::" + target->objectName()) == has_equip) x++;
                if (x > 0) {
                    QStringList lingren_targets = use.card->tag["LingrenTargets"].toStringList();
                    lingren_targets << target->objectName();
                    use.card->tag["LingrenTargets"] = lingren_targets;
                }
                if (x > 1) player->drawCards(2, objectName());

                if (x == 3 && player->getMark("LingrenThree") == 0) {
                    room->setPlayerMark(player, "LingrenThree", 1);
                    room->handleAcquireDetachSkills(player, "jianxiong|xingshang");
                }
            }
        } else if (triggerEvent == DamageInflicted) {
            DamageStruct damage = data.value<DamageStruct>();
            damage.damage++;
            data = QVariant::fromValue(damage);
        }
        return false;
    }
};

class Fujian : public PhaseChangeSkill
{
public:
    Fujian() : PhaseChangeSkill("fujian")
    {
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        if (PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Finish) {
            QList<ServerPlayer *> all_players = target->getRoom()->getAlivePlayers();
            foreach (ServerPlayer *p, all_players) {
                if (p->isKongcheng())
                    return false;
            }
            return true;
        }
        return false;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();

        room->sendCompulsoryTriggerLog(player, objectName());
        player->broadcastSkillInvoke(objectName());
        QList<ServerPlayer *> all_players = room->getAlivePlayers();
        int x = player->getHandcardNum();
        foreach (ServerPlayer *p, all_players) {
            x = qMin(x, p->getHandcardNum());
        }
        all_players.removeOne(player);
        if (all_players.isEmpty()) return false;
        qShuffle(all_players);
        ServerPlayer *target = all_players.first();
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());

        QList<int> ids, handcards = target->handCards();

        qShuffle(handcards);

        foreach (int id, handcards) {
            if (ids.length() == x) break;
            ids << id;
        }
        room->fillAG(ids, player);
        room->getThread()->delay(3000);
        room->clearAG();
        return false;
    }
};

class Weicheng : public TriggerSkill
{
public:
    Weicheng() : TriggerSkill("weicheng")
    {
        events << CardsMoveOneTime;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player) || player->getHp() <= player->getHandcardNum()) return QStringList();
        QVariantList move_datas = data.toList();
        foreach(QVariant move_data, move_datas) {
            CardsMoveOneTimeStruct move = move_data.value<CardsMoveOneTimeStruct>();
            if (move.from == player && move.from_places.contains(Player::PlaceHand)
                    && move.to && move.to != move.from && move.to_place == Player::PlaceHand) {
                return nameList();
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName())) {
            player->broadcastSkillInvoke(objectName());
            player->drawCards(1, objectName());
        }
        return false;
    }
};

DaoshuCard::DaoshuCard()
{
    m_skillName = "daoshu";
}

bool DaoshuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && Self->canGetCard(to_select, "h") && to_select != Self;
}

void DaoshuCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *source = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = source->getRoom();

    int card_id = room->askForCardChosen(source, target, "h", "daoshu", false, Card::MethodGet);
    const Card *card = Sanguosha->getCard(card_id);
    CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, source->objectName());
    room->obtainCard(source, card, reason, false);

    QString suit_str = card->getSuitString();
    if (suit_str == user_string) {
        room->damage(DamageStruct("daoshu", source, target));
        room->addPlayerHistory(source, getClassName(), -1);
    } else {
        const Card *to_give = NULL;
        foreach (const Card *c, source->getHandcards()) {
            if (c->getSuitString() != suit_str) {
                to_give = c;
                break;
            }
        }
        if (to_give == NULL) {
            room->showAllCards(source);
            return;
        }
        room->setPlayerProperty(source, "DaoshuSuit", suit_str);
        const Card *select = room->askForCard(source, ".|^" + suit_str + "|.|hand!", "@daoshu-give::" + target->objectName()
                                              + ":" + suit_str, QVariant(), Card::MethodNone);
        room->setPlayerProperty(source, "DaoshuSuit", QVariant());

        if (select == NULL)
            select = to_give;

        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), target->objectName(), "daoshu", QString());
        room->obtainCard(target, select, reason, true);
    }
}

class DaoshuViewAsSkill : public ZeroCardViewAsSkill
{
public:
    DaoshuViewAsSkill() : ZeroCardViewAsSkill("daoshu")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("DaoshuCard");
    }

    virtual const Card *viewAs() const
    {
        QString choice = Self->tag["daoshu"].toString();
        if (choice.isEmpty()) return NULL;
        return new DaoshuCard;
    }
};

class Daoshu : public TriggerSkill
{
public:
    Daoshu() : TriggerSkill("daoshu")
    {
        view_as_skill = new DaoshuViewAsSkill;
    }

    bool trigger(TriggerEvent, Room *, ServerPlayer *, QVariant &) const
    {
        return false;
    }

    QString getSelectBox() const
    {
        return "heart+diamond+club+spade";
    }
};

class Lvli : public TriggerSkill
{
public:
    Lvli() : TriggerSkill("lvli")
    {
        events << Damage << Damaged << EventPhaseStart;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::NotActive) {
            QList<ServerPlayer *> players = room->getAlivePlayers();
            foreach (ServerPlayer *p, players) {
                room->setPlayerMark(p, "LvliUsedTimes", 0);
            }
        }
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();

        ServerPlayer *current = room->getCurrent();
        if (current == NULL || current->getPhase() == Player::NotActive) return QStringList();

        int x = player->getMark("LvliUsedTimes");

        if (x > 1 || (x == 1 && (current != player || player->getMark("LvliEvolution1") == 0)))
            return QStringList();

        if (triggerEvent == EventPhaseStart || (triggerEvent == Damaged && player->getMark("LvliEvolution2") == 0))
            return QStringList();

        if (player->getHandcardNum() < player->getHp() || (player->getHandcardNum() > player->getHp() && player->isWounded()))
            return nameList();

        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName())) {
            player->broadcastSkillInvoke(objectName());
            room->addPlayerMark(player, "LvliUsedTimes");
            int x = player->getHandcardNum() - player->getHp();

            if (x < 0)
                player->drawCards(-x, objectName());
            else if (x > 0 && player->isWounded())
                room->recover(player, RecoverStruct(player, NULL, x));
        }
        return false;
    }
};

class Choujue : public TriggerSkill
{
public:
    Choujue() : TriggerSkill("choujue")
    {
        events << EventPhaseChanging;
        frequency = Wake;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        TriggerList skill_list;

        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::NotActive) {
            QList<ServerPlayer *> players = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *p, players) {
                if (qAbs(p->getHandcardNum() - p->getHp()) > 2 && p->getMark(objectName()) == 0)
                    skill_list.insert(p, nameList());
            }
        }

        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &, ServerPlayer *wenyang) const
    {
        room->sendCompulsoryTriggerLog(wenyang, objectName());
        wenyang->broadcastSkillInvoke(objectName());
        room->setPlayerMark(wenyang, objectName(), 1);
        if (room->changeMaxHpForAwakenSkill(wenyang) && wenyang->getMark(objectName()) == 1) {
            room->acquireSkill(wenyang, "beishui");
            if (wenyang->hasSkill("lvli", true))
                room->addPlayerMark(wenyang, "LvliEvolution1");
        }
        return false;
    }
};

class Beishui : public PhaseChangeSkill
{
public:
    Beishui() : PhaseChangeSkill("beishui")
    {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target)
               && target->getMark(objectName()) == 0
               && target->getPhase() == Player::Start
               && (target->getHp() < 2 || target->getHandcardNum() < 2);
    }

    virtual bool onPhaseChange(ServerPlayer *wenyang) const
    {
        Room *room = wenyang->getRoom();
        room->sendCompulsoryTriggerLog(wenyang, objectName());

        wenyang->broadcastSkillInvoke(objectName());

        room->setPlayerMark(wenyang, objectName(), 1);
        if (room->changeMaxHpForAwakenSkill(wenyang) && wenyang->getMark(objectName()) == 1) {
            room->acquireSkill(wenyang, "qingjiao");
            if (wenyang->hasSkill("lvli", true))
                room->addPlayerMark(wenyang, "LvliEvolution2");
        }
        return false;
    }
};

class Qingjiao : public PhaseChangeSkill
{
public:
    Qingjiao() : PhaseChangeSkill("qingjiao")
    {

    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (player->getPhase() == Player::Play && PhaseChangeSkill::triggerable(player) && !player->isKongcheng())
            return nameList();
        else if (player->getPhase() == Player::Finish && player->hasFlag("QingjiaoUsed"))
            return QStringList("qingjiao!");
        return QStringList();

    }

    virtual bool onPhaseChange(ServerPlayer *wenyang) const
    {
        Room *room = wenyang->getRoom();
        if (wenyang->getPhase() == Player::Play) {
            if (wenyang->askForSkillInvoke(objectName())) {
                wenyang->broadcastSkillInvoke(objectName());
                room->setPlayerFlag(wenyang, "QingjiaoUsed");
                wenyang->throwAllHandCards();

                QList<int> cards1, cards2;
                QStringList card_names;

                while (cards1.length() < 8) {
                    QList<int> all_cards = room->getDrawPile(), _cards;
                    foreach (int id, all_cards) {
                        if (!card_names.contains(QingjiaoCardName(id)))
                            _cards << id;
                    }
                    if (_cards.isEmpty()) break;
                    qShuffle(_cards);
                    int id = _cards.first();
                    cards1 << id;
                    card_names << QingjiaoCardName(id);
                }

                if (!cards1.isEmpty()) {
                    DummyCard *dummy = new DummyCard(cards1);
                    room->obtainCard(wenyang, dummy);
                    delete dummy;
                }

                while (cards1.length() + cards2.length() < 8) {
                    QList<int> all_cards = room->getDiscardPile(), _cards;
                    foreach (int id, all_cards) {
                        if (!card_names.contains(QingjiaoCardName(id)))
                            _cards << id;
                    }
                    if (_cards.isEmpty()) break;
                    qShuffle(_cards);
                    int id = _cards.first();
                    cards2 << id;
                    card_names << QingjiaoCardName(id);
                }


                if (!cards2.isEmpty()) {
                    DummyCard *dummy = new DummyCard(cards2);
                    room->obtainCard(wenyang, dummy);
                    delete dummy;
                }

            }
        } else if (wenyang->getPhase() == Player::Finish)
            wenyang->throwAllHandCardsAndEquips();
        return false;
    }

private:
    QString QingjiaoCardName(int id) const
    {
        const Card *card = Sanguosha->getCard(id);
        if (card->isKindOf("Slash"))
            return "slash";
        else if (card->isKindOf("Weapon"))
            return "weapon";
        else if (card->isKindOf("Armor"))
            return "armor";
        else if (card->isKindOf("OffensiveHorse"))
            return "offensivehorse";
        else if (card->isKindOf("DefensiveHorse"))
            return "defensivehorse";
        else if (card->isKindOf("Treasure"))
            return "treasure";
        return card->objectName();
    }

};

class Tuiyan : public PhaseChangeSkill
{
public:
    Tuiyan() : PhaseChangeSkill("tuiyan")
    {

    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Play;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const
    {
        Room *room = target->getRoom();
        if (room->askForSkillInvoke(target, objectName())) {
            target->broadcastSkillInvoke(objectName());

            QList<int> guanxing = room->getNCards(2, false);

            LogMessage log;
            log.type = "$ViewDrawPile";
            log.from = target;
            log.card_str = IntList2StringList(guanxing).join("+");
            room->sendLog(log, target);

            room->fillAG(guanxing, target);
            room->getThread()->delay(3000);
            room->clearAG();

            room->returnToTopDrawPile(guanxing);
        }
        return false;
    }
};

BusuanCard::BusuanCard()
{
}

void BusuanCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *source = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = source->getRoom();
    room->addPlayerTip(target, "#busuan");

    const Card *card = room->askForCard(source, "@@busuan_select!", "@busuan-declare:" + target->objectName(), QVariant(), Card::MethodNone);

    QStringList card_names;
    if (card != NULL) {
        QStringList flags = card->getFlags();
        foreach (QString name, flags) {
            if (name.startsWith("Busuan_")) {
                card_names << name.mid(7);
            }
        }
    } else
        card_names << "Slash";

    target->tag["BusuanCardNames"] = card_names.join("+");
}

class BusuanViewAsSkill : public ZeroCardViewAsSkill
{
public:
    BusuanViewAsSkill() : ZeroCardViewAsSkill("busuan")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("BusuanCard");
    }

    virtual const Card *viewAs() const
    {
        return new BusuanCard;
    }
};

class Busuan : public PhaseChangeSkill
{
public:
    Busuan() : PhaseChangeSkill("busuan")
    {
        view_as_skill = new BusuanViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *&) const
    {
        if (player->getPhase() == Player::Draw && player->getMark("#busuan") > 0) {
            return QStringList("busuan!");
        }
        return QStringList();
    }

    virtual bool onPhaseChange(ServerPlayer *target) const
    {
        Room *room = target->getRoom();
        room->removePlayerTip(target, "#busuan");
        QStringList card_names = target->tag["BusuanCardNames"].toString().split("+");
        target->tag.remove("BusuanCardNames");

        QList<int> cards;


        if (!cards.isEmpty()) {
            DummyCard *dummy = new DummyCard(cards);
            room->obtainCard(target, dummy);
            delete dummy;
        }

        return true;
    }
};

class BusuanSelect : public ViewAsSkill
{
public:
    BusuanSelect() : ViewAsSkill("busuan_select")
    {
        response_pattern = "@@busuan_select!";
        guhuo_type = "sbtd";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.length() < 2 && to_select->isVirtualCard();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty()) return NULL;
        DummyCard *dummy = new DummyCard;
        foreach (const Card *c, cards) {
            dummy->setFlags("Busuan_" + c->getClassName());
        }
        return dummy;
    }
};

class Mingjie : public PhaseChangeSkill
{
public:
    Mingjie() : PhaseChangeSkill("mingjie")
    {

    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Finish;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const
    {
        Room *room = target->getRoom();
        if (room->askForSkillInvoke(target, objectName())) {
            LogMessage log;
            log.type = "#InvokeSkill";
            log.from = target;
            log.arg = objectName();
            room->sendLog(log);
            room->notifySkillInvoked(target, objectName());
            target->broadcastSkillInvoke(objectName());
            for (int i = 1; i <= 3; i++) {
                bool from_up = true;
                if (target->hasSkill("cunmu")) {
                    room->sendCompulsoryTriggerLog(target, "cunmu");
                    target->broadcastSkillInvoke("cunmu");
                    from_up = false;
                }
                int id = room->drawCard(from_up);
                const Card *card = Sanguosha->getCard(id);
                CardMoveReason reason(CardMoveReason::S_REASON_DRAW, target->objectName(), objectName(), QString());
                room->obtainCard(target, card, reason, false);
                if (card->isBlack()) {
                    room->loseHp(target);
                    break;
                }
                if (i == 3 || !room->askForSkillInvoke(target, objectName()))
                    break;
            }
        }
        return false;
    }
};

class Lianhua : public TriggerSkill
{
public:
    Lianhua() : TriggerSkill("lianhua")
    {
        events << Damaged << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Play) {
            room->setPlayerMark(player, "#blood", 0);
            room->setPlayerMark(player, "lianhua_red", 0);
            room->setPlayerMark(player, "lianhua_black", 0);
        }
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (triggerEvent == Damaged) {
            QList<ServerPlayer *> gexuans = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *gexuan, gexuans) {
                if (gexuan->getPhase() == Player::NotActive && gexuan != player)
                    skill_list.insert(gexuan, nameList());
            }
        } else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Start && TriggerSkill::triggerable(player))
            skill_list.insert(player, nameList());

        return skill_list;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *source) const
    {
        room->sendCompulsoryTriggerLog(source, objectName());
        source->broadcastSkillInvoke(objectName());
        if (triggerEvent == Damaged) {
            room->addPlayerMark(source, "#blood");
            room->addPlayerMark(source, isSameFaction(source, player) ? "lianhua_red" : "lianhua_black");
        } else if (triggerEvent == EventPhaseStart) {
            int x = player->getMark("lianhua_red"), y = player->getMark("lianhua_black");
            if (x + y < 4) {
                room->acquireSkill(source, "yingzi", true, true);
                searchCard(source, "Peach");
            } else if (x == y) {
                room->acquireSkill(source, "gongxin", true, true);
                searchCard(source, "Slash");
                searchCard(source, "Duel");
            } else if (x > y) {
                room->acquireSkill(source, "guanxing", true, true);
                searchCard(source, "ExNihilo");
            } else if (x < y) {
                room->acquireSkill(source, "zhiyan", true, true);
                searchCard(source, "Snatch");
            }
        }
        return false;
    }


    virtual bool isSameFaction(ServerPlayer *p1, ServerPlayer *p2) const
    {
        //lord, loyalist, rebel, renegade
        return (p1->getRole().startsWith("lo") && p2->getRole().startsWith("lo")) ||
               (p1->getRole() == "rebel" && p2->getRole() == "rebel");
    }

    virtual void searchCard(ServerPlayer *player, QString pattern) const
    {
        int id = player->getRoom()->getRandomCardInPile(pattern, true);
        if (id > -1)
            player->obtainCard(Sanguosha->getCard(id));
        else {
            id = player->getRoom()->getRandomCardInPile(pattern, false);
            if (id > -1)
                player->obtainCard(Sanguosha->getCard(id));
        }
    }
};

ZhafuCard::ZhafuCard()
{
}

void ZhafuCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    ServerPlayer *source = card_use.from;
    QVariant data = QVariant::fromValue(use);
    RoomThread *thread = room->getThread();

    thread->trigger(PreCardUsed, room, card_use.from, data);
    use = data.value<CardUseStruct>();

    LogMessage log;
    log.from = source;
    log.type = "#UseCard";
    log.card_str = toString();
    log.to = card_use.to;
    room->sendLog(log);

    room->removePlayerMark(source, "@limit_incantation");

    thread->trigger(CardUsed, room, source, data);
    use = data.value<CardUseStruct>();
    thread->trigger(CardFinished, room, source, data);
}

void ZhafuCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *source = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = source->getRoom();
    room->addPlayerTip(target, "#zhafu");
    QStringList zhafu_list = target->tag["ZhafuSource"].toStringList();
    zhafu_list << source->objectName();
    target->tag["ZhafuSource"] = QVariant::fromValue(zhafu_list);
}

class ZhafuViewAsSkill : public ZeroCardViewAsSkill
{
public:
    ZhafuViewAsSkill() : ZeroCardViewAsSkill("zhafu")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@limit_incantation") > 0;
    }

    virtual const Card *viewAs() const
    {
        return new ZhafuCard;
    }
};

class Zhafu : public TriggerSkill
{
public:
    Zhafu() : TriggerSkill("zhafu")
    {
        events << EventPhaseStart << EventPhaseChanging;
        frequency = Limited;
        limit_mark = "@limit_incantation";
        view_as_skill = new ZhafuViewAsSkill;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Discard && !player->hasSkipped(Player::Discard)) {
                room->removePlayerTip(player, "#zhafu");
                player->tag.remove("ZhafuSource");
            }
        }
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Discard && player->isAlive() && player->getHandcardNum() > 1) {
            QStringList zhafu_list = player->tag["ZhafuSource"].toStringList();
            QList<ServerPlayer *> allplayers = room->getAlivePlayers();
            foreach (ServerPlayer *p, allplayers) {
                if (zhafu_list.contains(p->objectName())) {
                    skill_list.insert(p, QStringList("zhafu!"));
                }
            }
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *gexuan) const
    {
        if (player->getHandcardNum() > 1) {

            const Card *card = room->askForExchange(player, objectName(), 1, 1, false, "@zhafu-give:" + gexuan->objectName());

            QList<int> handcards = player->handCards();
            handcards.removeOne(card->getEffectiveId());
            DummyCard *dummy = new DummyCard(handcards);
            CardMoveReason reason(CardMoveReason::S_REASON_GIVE, player->objectName(), gexuan->objectName(), "zhafu", QString());
            room->obtainCard(gexuan, dummy, reason, false);
            delete dummy;
        }
        return false;
    }
};

class Yuxu : public TriggerSkill
{
public:
    Yuxu() : TriggerSkill("yuxu")
    {
        events << CardFinished << EventPhaseChanging;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (triggerEvent == EventPhaseChanging) {
            room->setPlayerMark(player, "yuxuNumber", 0);
        }
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == CardFinished && TriggerSkill::triggerable(player)) {
            const Card *card = data.value<CardUseStruct>().card;
            if (card == NULL) return QStringList();

            int x = card->tag["XuJingNumber"].toInt();
            if (x > 0) {
                int y = player->getMark("yuxuNumber");
                if (y == 0)
                    return nameList();
                else if (x == y + 1)
                    return QStringList("yuxu!");
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (player->getMark("yuxuNumber") == 0) {
            if (player->askForSkillInvoke(objectName())) {
                player->broadcastSkillInvoke(objectName());
                player->drawCards(1, objectName());
                const Card *card = data.value<CardUseStruct>().card;
                if (card)
                    room->setPlayerMark(player, "yuxuNumber", card->tag["XuJingNumber"].toInt());
            }
        } else {
            room->sendCompulsoryTriggerLog(player, objectName());
            player->broadcastSkillInvoke(objectName());
            room->setPlayerMark(player, "yuxuNumber", 0);
            room->askForDiscard(player, objectName(), 1, 1, false, true, "@yuxu-discard");
        }
        return false;
    }
};

class Shijian : public TriggerSkill
{
public:
    Shijian() : TriggerSkill("shijian")
    {
        events << CardFinished;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        const Card *card = data.value<CardUseStruct>().card;
        if (card && card->tag["XuJingNumber"].toInt() == 2) {
            QList<ServerPlayer *> xujins = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *xujin, xujins) {
                if (player != xujin)
                    skill_list.insert(xujin, nameList());
            }

        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const
    {
        if (room->askForCard(ask_who, "..", "@shijian", data, objectName(), player)) {
            room->acquireSkill(player, "yuxu", true, true);
        }
        return false;
    }
};

class Manyi : public TriggerSkill
{
public:
    Manyi() : TriggerSkill("manyi")
    {
        events << CardEffected;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if (player == NULL || player->isDead() || !player->hasSkill(objectName())) return QStringList();
        return (effect.card->isKindOf("SavageAssault")) ? QStringList(objectName()) : QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        player->broadcastSkillInvoke(objectName());
        room->notifySkillInvoked(player, objectName());

        LogMessage log;
        log.type = "#SkillNullify";
        log.from = player;
        log.arg = objectName();
        log.arg2 = "savage_assault";
        room->sendLog(log);

        effect.nullified = true;

        data = QVariant::fromValue(effect);
        return true;
    }

};

class Mansi : public TriggerSkill
{
public:
    Mansi() : TriggerSkill("mansi")
    {
        events << CardFinished;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        TriggerList skill_list;
        const Card *card = data.value<CardUseStruct>().card;
        if (card && card->isKindOf("SavageAssault") && !card->tag["GlobalCardDamagedTag"].isNull()) {
            QList<ServerPlayer *> huamans = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *huaman, huamans)
                skill_list.insert(huaman, nameList());

        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *huaman) const
    {
        int x = 0;
        QStringList damaged_tag = data.value<CardUseStruct>().card->tag["GlobalCardDamagedTag"].toStringList();
        QList<ServerPlayer *> players = room->getAllPlayers(true);
        foreach(ServerPlayer *p, players) {
            if (damaged_tag.contains(p->objectName()))
                x++;
        }

        if (x > 0 && huaman->askForSkillInvoke(objectName(), QVariant("prompt:::" + QString::number(x)))) {
            huaman->broadcastSkillInvoke(objectName());
            room->addPlayerMark(huaman, "#mansi", x);
            huaman->drawCards(x, objectName());
        }
        return false;
    }
};

class Souying : public TriggerSkill
{
public:
    Souying() : TriggerSkill("souying")
    {
        events << DamageCaused << DamageInflicted;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player) && !player->hasFlag("SouyingInvoked") && !player->isKongcheng()) {
            DamageStruct damage = data.value<DamageStruct>();
            if (triggerEvent == DamageCaused) {
                if (damage.to && damage.to->isMale()) {
                    QStringList damaged_tag = damage.to->tag["GlobalRoundDamagedTag"].toStringList();
                    if (damaged_tag.count(player->objectName()) == 1)
                        return nameList();
                }

            } else {
                QStringList damaged_tag = player->tag["GlobalRoundDamagedTag"].toStringList();
                if (damage.from && damage.from->isMale() && damaged_tag.count(damage.from->objectName()) == 1)
                    return nameList();

            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        QString prompt;
        if (triggerEvent == DamageCaused)
            prompt = "@souying-increase:" + damage.to->objectName();
        else
            prompt = "@souying-decrease:" + damage.from->objectName();

        if (room->askForCard(player, ".", prompt, data, objectName())) {
            room->setPlayerFlag(player, "SouyingInvoked");
            if (triggerEvent == DamageCaused)
                damage.damage++;
            else
                damage.damage--;
            data = QVariant::fromValue(damage);
            if (damage.damage < 1) {
                room->preventDamage(damage);
                return true;
            }
        }
        return false;
    }
};

class Zhanyuan : public PhaseChangeSkill
{
public:
    Zhanyuan() : PhaseChangeSkill("zhanyuan")
    {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target)
               && target->getMark("zhanyuan") == 0
               && target->getPhase() == Player::Start
               && target->getMark("#mansi") > 6;
    }

    virtual bool onPhaseChange(ServerPlayer *huaman) const
    {
        Room *room = huaman->getRoom();
        room->sendCompulsoryTriggerLog(huaman, objectName());

        huaman->broadcastSkillInvoke(objectName());

        room->setPlayerMark(huaman, objectName(), 1);
        if (room->changeMaxHpForAwakenSkill(huaman, 1) && huaman->getMark(objectName()) == 1) {
            QList<ServerPlayer *> males, players = room->getOtherPlayers(huaman);
            foreach (ServerPlayer *p, players) {
                if (p->isMale())
                    males << p;
            }
            ServerPlayer *vic = room->askForPlayerChosen(huaman, males, objectName(), "@zhanyuan-choose", true);
            if (vic) {
                room->acquireSkill(huaman, "xili");
                room->acquireSkill(vic, "xili");
                room->detachSkillFromPlayer(huaman, "mansi");
            }
        }
        return false;
    }
};

class Xili : public TriggerSkill
{
public:
    Xili() : TriggerSkill("xili")
    {
        events << TargetSpecified;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (player->isDead() || !player->hasSkill(objectName(), true) || player->getPhase() == Player::NotActive) return skill_list;
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") && use.index == 0) {
            QList<ServerPlayer *> asks = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *ask, asks) {
                if (player != ask && !ask->isKongcheng())
                    skill_list.insert(ask, nameList());
            }
        }

        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *ask_who) const
    {
        if (room->askForCard(ask_who, ".", "@xili-discard::" + player->objectName(), data, objectName(), player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            use.card->setTag("addcardinality", use.card->tag["addcardinality"].toInt() + 1);
        }
        return false;
    }
};

class NeifaDiscard : public OneCardViewAsSkill
{
public:
    NeifaDiscard() : OneCardViewAsSkill("neifa_discard")
    {
        filter_pattern = ".!";
        response_pattern = "@@neifa_discard!";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        return originalCard;
    }
};


NeifaCard::NeifaCard()
{
    m_skillName = "neifa";
}

bool NeifaCard::targetFixed() const
{
    return user_string == "draw";
}

bool NeifaCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && Self->canGetCard(to_select, "ej");
}

bool NeifaCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return targets.length() < 2;
}

void NeifaCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    if (targets.isEmpty())
        source->drawCards(2, "neifa");
    else  {
        ServerPlayer *target = targets.first();
        if (source->canGetCard(target, "he")) {
            int card_id = room->askForCardChosen(source, target, "ej", "neifa", false, Card::MethodGet);
            CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, source->objectName());
            room->obtainCard(source, Sanguosha->getCard(card_id), reason, false);
        }
    }

}

class NeifaViewAsSkill : public ZeroCardViewAsSkill
{
public:
    NeifaViewAsSkill() : ZeroCardViewAsSkill("neifa")
    {
        response_pattern = "@@neifa";
    }

    virtual const Card *viewAs() const
    {
        return new NeifaCard;
    }
};

class Neifa : public TriggerSkill
{
public:
    Neifa() : TriggerSkill("neifa")
    {
        events << EventPhaseStart << CardUsed << TargetChosed;
        view_as_skill = new NeifaViewAsSkill;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::NotActive) {
            room->setPlayerMark(player, "#neifa", 0);
            room->setPlayerMark(player, "NeifaTimes", 0);
        }
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Play && TriggerSkill::triggerable(player)) {
            return nameList();
        } else if (triggerEvent == CardUsed && player->hasFlag("NeifaNotBasic")
                   && player->getMark("#neifa") > 0 && player->getMark("NeifaTimes") < 2) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() == Card::TypeEquip)
                return QStringList("neifa!");
        } else if (triggerEvent == TargetChosed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (((use.card->isKindOf("Slash") && player->hasFlag("NeifaBasic"))
                    || (use.card->isNDTrick() && player->hasFlag("NeifaNotBasic")))) {
                return nameList();
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (room->askForUseCard(player, "@@neifa", "@neifa-use", data, Card::MethodNone)) {
                QList<int> cards = player->forceToDiscard(1, true);
                if (cards.isEmpty()) return false;

                const Card *card = room->askForCard(player, "@@neifa_discard!", "@neifa-discard", data, Card::MethodNone);
                if (card == NULL)
                    card = Sanguosha->getCard(cards.first());

                bool isBasicCard = card->getTypeId() == Card::TypeBasic;

                CardMoveReason mreason(CardMoveReason::S_REASON_THROW, player->objectName(), QString(), "neifa", QString());
                room->throwCard(card, mreason, player);

                if (isBasicCard) {
                    room->setPlayerFlag(player, "NeifaBasic");
                    room->setPlayerCardLimitation(player, "use", "^BasicCard", true);

                } else {
                    room->setPlayerFlag(player, "NeifaNotBasic");

                    room->setPlayerCardLimitation(player, "use", "BasicCard", true);
                }

                int x = 0;
                foreach (const Card *card, player->getHandcards()) {
                    if (isBasicCard != (card->getTypeId() == Card::TypeBasic))
                        x++;
                }
                room->setPlayerMark(player, "#neifa", qMin(5, x));

            }
        } else if (triggerEvent == CardUsed) {
            LogMessage log;
            log.type = "#SkillForce";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);
            room->addPlayerMark(player, "NeifaTimes");
            player->drawCards(player->getMark("#neifa"), objectName());

        } else if (triggerEvent == TargetChosed) {
            CardUseStruct use = data.value<CardUseStruct>();
            QList<ServerPlayer *> targets = player->getUseExtraTargets(use, true);
            targets.append(use.to);
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@neifa-target:::" + use.card->objectName(), true);
            if (target) {
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());
                LogMessage log;
                log.from = player;
                log.to << target;
                log.card_str = use.card->toString();
                log.arg = objectName();

                if (use.to.contains(target)) {
                    log.type = "#QiaoshuiRemove";
                    use.to.removeOne(target);
                } else {
                    log.type = "#QiaoshuiAdd";
                    use.to.append(target);
                    room->sortByActionOrder(use.to);
                }
                room->sendLog(log);

                data = QVariant::fromValue(use);
            }
        }
        return false;
    }

    QString getSelectBox() const
    {
        return "draw+get";
    }

    bool buttonEnabled(const QString &button_name, const QList<const Card *> &, const QList<const Player *> &targets) const
    {
        return ((button_name == "draw") == targets.isEmpty());
    }

};

class NeifaTargetMod : public TargetModSkill
{
public:
    NeifaTargetMod() : TargetModSkill("#Neifa-target")
    {
        frequency = NotFrequent;
    }

    virtual int getResidueNum(const Player *from, const Card *, const Player *) const
    {
        if (from->hasFlag("NeifaBasic")) {
            return from->getMark("#neifa");
        }
        return 0;
    }
};






class Zhuilie : public TriggerSkill
{
public:
    Zhuilie() : TriggerSkill("zhuilie")
    {
        events << TargetSpecified;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card != NULL && use.card->isKindOf("Slash")) {
                ServerPlayer *to = use.to.at(use.index);

                if (to && to->isAlive() && !player->inMyAttackRange(to))
                    return nameList();
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *player) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        int index = use.index;
        ServerPlayer *to = use.to.at(index);
        room->sendCompulsoryTriggerLog(player, objectName());
        player->broadcastSkillInvoke(objectName());
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), to->objectName());

        if (use.m_addHistory) {
            room->addPlayerHistory(player, use.card->getClassName(), -1);
            use.m_addHistory = false;
            data = QVariant::fromValue(use);
        }

        JudgeStruct judge;
        judge.pattern = ".";
        judge.patterns << "Weapon,Horse";
        judge.play_animation = false;
        judge.reason = objectName();
        judge.who = player;

        room->judge(judge);

        if (judge.pattern == "Weapon,Horse") {

            QVariantList damage_list = use.card->tag["Damage_List"].toList();
            damage_list[index] = damage_list[index].toInt() + to->getHp() - 1;
            use.card->setTag("Damage_List", damage_list);

        } else
            room->loseHp(player);

        return false;
    }
};

class ZhuilieTargetMod : public TargetModSkill
{
public:
    ZhuilieTargetMod() : TargetModSkill("#zhuilie-target")
    {
        frequency = Compulsory;
    }

    virtual int getDistanceLimit(const Player *from, const Card *, const Player *) const
    {
        if (from->hasSkill("zhuilie"))
            return 1000;
        else
            return 0;
    }

};



















OLPackage::OLPackage()
    : Package("OL")
{
    General *zhugeke = new General(this, "zhugeke", "wu", 3); // OL 002
    zhugeke->addSkill(new Aocai);
    zhugeke->addSkill(new Duwu);

    General *chengyu = new General(this, "chengyu", "wei", 3);
    chengyu->addSkill(new Shefu);
    chengyu->addSkill(new DetachEffectSkill("shefu", "ambush"));
    related_skills.insertMulti("shefu", "#shefu-clear");
    chengyu->addSkill(new Benyu);

    General *sunhao = new General(this, "sunhao$", "wu", 5); // SP 041
    sunhao->addSkill(new Canshi);
    sunhao->addSkill(new Chouhai);
    sunhao->addSkill(new Skill("guiming$", Skill::Compulsory));

    General *wutugu = new General(this, "wutugu", "qun", 15);
    wutugu->addSkill(new Ranshang);
    wutugu->addSkill(new Hanyong);

    General *shixie = new General(this, "shixie", "qun", 3);
    shixie->addSkill(new Biluan);
    shixie->addSkill(new Lixia);

    General *yanbaihu = new General(this, "yanbaihu", "qun");
    yanbaihu->addSkill(new Zhidao);
    yanbaihu->addSkill(new Jili);

    General *guansuo = new General(this, "guansuo", "shu");
    guansuo->addSkill(new Zhengnan);
    guansuo->addSkill(new Xiefang);
    guansuo->addRelateSkill("wusheng");
    guansuo->addRelateSkill("dangxian");
    guansuo->addRelateSkill("zhiman");

    General *wanglang = new General(this, "wanglang", "wei", 3);
    wanglang->addSkill(new Gushe);
    wanglang->addSkill(new Jici);

    General *litong = new General(this, "litong", "wei");
    litong->addSkill(new Cuifeng);
    litong->addSkill(new CuifengTargetMod);
    litong->addSkill(new DetachEffectSkill("cuifeng", "cuifeng_feng"));
    related_skills.insertMulti("cuifeng", "#cuifeng-target");
    related_skills.insertMulti("cuifeng", "#cuifeng-clear");

    General *dongbai = new General(this, "dongbai", "qun", 3, false);
    dongbai->addSkill(new Lianzhu);
    dongbai->addSkill(new Xiahui);
    dongbai->addSkill(new XiahuiRecord);
    dongbai->addSkill(new XiahuiCardLimited);
    related_skills.insertMulti("xiahui", "#xiahui-record");
    related_skills.insertMulti("xiahui", "#xiahui-limited");

    General *zhaoxiang = new General(this, "zhaoxiang", "shu", 4, false);
    zhaoxiang->addSkill(new Fanghun);
    zhaoxiang->addSkill(new Fuhan);

    General *kanze = new General(this, "kanze", "wu", 3);
    kanze->addSkill(new Xiashu);
    kanze->addSkill(new Kuanshi);

    General *wangyun = new General(this, "wangyun", "qun", 3);
    wangyun->addSkill(new Lianji);
    wangyun->addSkill(new DetachEffectSkill("lianji", QString(), "#lianji"));
    related_skills.insertMulti("lianji", "#lianji-clear");
    wangyun->addSkill(new Moucheng);
    wangyun->addRelateSkill("jingong");

    General *quyi = new General(this, "quyi", "qun");
    quyi->addSkill(new Fuji);
    quyi->addSkill(new Jiaozi);

    General *liuqi = new General(this, "liuqi", "qun", 3);
    liuqi->addSkill(new Wenji);
    liuqi->addSkill(new Tunjiang);

    General *luzhi = new General(this, "luuzhi", "wei", 3);
    luzhi->addSkill(new Qingzhong);
    luzhi->addSkill(new Weijing);

    General *simahui = new General(this, "simahui", "qun", 3);
    simahui->addSkill(new Jianjie);
    simahui->addSkill(new Chenghao);
    simahui->addSkill(new Yinshi);
    simahui->addRelateSkill("huojii");
    simahui->addRelateSkill("lianhuani");
    simahui->addRelateSkill("yeyani");

    General *baosanniang = new General(this, "baosanniang", "shu", 3, false);
    baosanniang->addSkill(new Wuniang);
    baosanniang->addSkill(new Xushen);
    baosanniang->addRelateSkill("zhennan");

    General *caoying = new General(this, "caoying", "wei", 4, false);
    caoying->addSkill(new Lingren);
    caoying->addSkill(new Fujian);
    caoying->addRelateSkill("jianxiong");
    caoying->addRelateSkill("xingshang");

    General *jianggan = new General(this, "jianggan", "wei", 3);
    jianggan->addSkill(new Weicheng);
    jianggan->addSkill(new Daoshu);

    General *wenyang = new General(this, "wenyang", "wei", 5);
    wenyang->addSkill(new Lvli);
    wenyang->addSkill(new Choujue);
    wenyang->addRelateSkill("beishui");
    wenyang->addRelateSkill("qingjiao");

    General *guanlu = new General(this, "guanlu", "wei", 3);
    guanlu->addSkill(new Tuiyan);
    guanlu->addSkill(new Busuan);
    guanlu->addSkill(new Mingjie);

    General *gexuan = new General(this, "gexuan", "wu", 3);
    gexuan->addSkill(new Lianhua);
    gexuan->addSkill(new Zhafu);
    gexuan->addRelateSkill("yingzi");
    gexuan->addRelateSkill("guanxing");
    gexuan->addRelateSkill("zhiyan");
    gexuan->addRelateSkill("gongxin");

    General *xujing = new General(this, "xujing", "shu", 3);
    xujing->addSkill(new Yuxu);
    xujing->addSkill(new Shijian);

    General *huaman = new General(this, "huaman", "shu", 3, false);
    huaman->addSkill(new Manyi);
    huaman->addSkill(new Mansi);
    huaman->addSkill(new Souying);
    huaman->addSkill(new Zhanyuan);
    huaman->addRelateSkill("xili");

    General *yuantanyuanshang = new General(this, "yuantanyuanshang", "qun");
    yuantanyuanshang->addSkill(new Neifa);
    yuantanyuanshang->addSkill(new NeifaTargetMod);
    related_skills.insertMulti("neifa", "#neifa-target");

    General *wangshuang = new General(this, "wangshuang", "wei", 8);
    wangshuang->addSkill(new Zhuilie);
    wangshuang->addSkill(new ZhuilieTargetMod);
    related_skills.insertMulti("zhuilie", "#zhuilie-target");

    addMetaObject<AocaiCard>();
    addMetaObject<DuwuCard>();
    addMetaObject<ShefuCard>();
    addMetaObject<GusheCard>();
    addMetaObject<LianzhuCard>();
    addMetaObject<FanghunCard>();
    addMetaObject<LianjiCard>();
    addMetaObject<SmileDagger>();
    addMetaObject<HoneyTrap>();
    addMetaObject<JingongCard>();
    addMetaObject<JianjieCard>();
    addMetaObject<YeyaniCard>();
    addMetaObject<GreatYeyaniCard>();
    addMetaObject<SmallYeyaniCard>();
    addMetaObject<DaoshuCard>();
    addMetaObject<BusuanCard>();
    addMetaObject<ZhafuCard>();
    addMetaObject<NeifaCard>();

    skills << new AocaiVeiw << new ShixieDistance << new Jingong << new Huojii << new Lianhuani << new Yeyani << new Zhennan
           << new Beishui << new Qingjiao << new BusuanSelect << new Xili << new NeifaDiscard;
}

ADD_PACKAGE(OL)
