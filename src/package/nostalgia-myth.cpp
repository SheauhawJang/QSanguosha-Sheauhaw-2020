#include "standard.h"
#include "skill.h"
#include "wind.h"
#include "client.h"
#include "engine.h"
#include "nostalgia.h"
#include "yjcm.h"
#include "yjcm2013.h"
#include "settings.h"
#include "guhuodialog.h"
#include "nostalgia-myth.h"
#include "maneuvering.h"

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

class NosJushou : public PhaseChangeSkill
{
public:
    NosJushou() : PhaseChangeSkill("nosjushou")
    {

    }

    virtual bool onPhaseChange(ServerPlayer *caoren) const
    {
        if (caoren->getPhase() == Player::Finish) {
            Room *room = caoren->getRoom();
            if (room->askForSkillInvoke(caoren, objectName())) {
                caoren->broadcastSkillInvoke(objectName());
                caoren->drawCards(3, objectName());
                caoren->turnOver();
            }
        }
        return false;
    }
};

class NosBuquRemove : public TriggerSkill
{
public:
    NosBuquRemove() : TriggerSkill("#nosbuqu-remove")
    {
        events << HpRecover;
    }

    static void Remove(ServerPlayer *zhoutai)
    {
        Room *room = zhoutai->getRoom();
        QList<int> nosbuqu(zhoutai->getPile("nosbuqu"));

        CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), "nosbuqu", QString());
        int need = 1 - zhoutai->getHp();
        if (need <= 0) {
            // clear all the buqu cards
            foreach (int card_id, nosbuqu) {
                LogMessage log;
                log.type = "$NosBuquRemove";
                log.from = zhoutai;
                log.card_str = Sanguosha->getCard(card_id)->toString();
                room->sendLog(log);

                room->throwCard(Sanguosha->getCard(card_id), reason, NULL);
            }
        } else {
            int to_remove = nosbuqu.length() - need;
            for (int i = 0; i < to_remove; i++) {
                room->fillAG(nosbuqu);
                int card_id = room->askForAG(zhoutai, nosbuqu, false, "nosbuqu");

                LogMessage log;
                log.type = "$NosBuquRemove";
                log.from = zhoutai;
                log.card_str = Sanguosha->getCard(card_id)->toString();
                room->sendLog(log);

                nosbuqu.removeOne(card_id);
                room->throwCard(Sanguosha->getCard(card_id), reason, NULL);
                room->clearAG();
            }
        }
    }

    virtual bool trigger(TriggerEvent, Room *, ServerPlayer *zhoutai, QVariant &) const
    {
        if (zhoutai->getPile("nosbuqu").length() > 0)
            Remove(zhoutai);

        return false;
    }
};

class NosBuqu : public TriggerSkill
{
public:
    NosBuqu() : TriggerSkill("nosbuqu")
    {
        events << HpChanged << AskForPeachesDone;
    }

    virtual int getPriority(TriggerEvent triggerEvent) const
    {
        if (triggerEvent == HpChanged)
            return 1;
        else
            return TriggerSkill::getPriority(triggerEvent);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *zhoutai, QVariant &data) const
    {
        if (triggerEvent == HpChanged && !data.isNull() && !data.canConvert<RecoverStruct>() && zhoutai->getHp() < 1) {
            if (room->askForSkillInvoke(zhoutai, objectName(), data)) {
                room->setTag("NosBuqu", zhoutai->objectName());
                zhoutai->broadcastSkillInvoke("nosbuqu");
                const QList<int> &nosbuqu = zhoutai->getPile("nosbuqu");

                int need = 1 - zhoutai->getHp(); // the buqu cards that should be turned over
                int n = need - nosbuqu.length();
                if (n > 0) {
                    QList<int> card_ids = room->getNCards(n, false);
                    zhoutai->addToPile("nosbuqu", card_ids);
                }
                const QList<int> &nosbuqunew = zhoutai->getPile("nosbuqu");
                QList<int> duplicate_numbers;

                QSet<int> numbers;
                foreach (int card_id, nosbuqunew) {
                    const Card *card = Sanguosha->getCard(card_id);
                    int number = card->getNumber();

                    if (numbers.contains(number))
                        duplicate_numbers << number;
                    else
                        numbers << number;
                }

                if (duplicate_numbers.isEmpty()) {
                    room->setTag("NosBuqu", QVariant());
                    return true;
                }
            }
        } else if (triggerEvent == AskForPeachesDone) {
            const QList<int> &nosbuqu = zhoutai->getPile("nosbuqu");

            if (zhoutai->getHp() > 0)
                return false;
            if (room->getTag("NosBuqu").toString() != zhoutai->objectName())
                return false;
            room->setTag("NosBuqu", QVariant());

            QList<int> duplicate_numbers;
            QSet<int> numbers;
            foreach (int card_id, nosbuqu) {
                const Card *card = Sanguosha->getCard(card_id);
                int number = card->getNumber();

                if (numbers.contains(number) && !duplicate_numbers.contains(number))
                    duplicate_numbers << number;
                else
                    numbers << number;
            }

            if (duplicate_numbers.isEmpty()) {
                zhoutai->broadcastSkillInvoke("nosbuqu");
                room->setPlayerFlag(zhoutai, "-Global_Dying");
                return true;
            } else {
                LogMessage log;
                log.type = "#NosBuquDuplicate";
                log.from = zhoutai;
                log.arg = QString::number(duplicate_numbers.length());
                room->sendLog(log);

                for (int i = 0; i < duplicate_numbers.length(); i++) {
                    int number = duplicate_numbers.at(i);

                    LogMessage log;
                    log.type = "#NosBuquDuplicateGroup";
                    log.from = zhoutai;
                    log.arg = QString::number(i + 1);
                    if (number == 10)
                        log.arg2 = "10";
                    else {
                        const char *number_string = "-A23456789-JQK";
                        log.arg2 = QString(number_string[number]);
                    }
                    room->sendLog(log);

                    foreach (int card_id, nosbuqu) {
                        const Card *card = Sanguosha->getCard(card_id);
                        if (card->getNumber() == number) {
                            LogMessage log;
                            log.type = "$NosBuquDuplicateItem";
                            log.from = zhoutai;
                            log.card_str = QString::number(card_id);
                            room->sendLog(log);
                        }
                    }
                }
            }
        }
        return false;
    }
};

class NosBuquClear : public DetachEffectSkill
{
public:
    NosBuquClear() : DetachEffectSkill("nosbuqu")
    {
    }

    virtual void onSkillDetached(Room *room, ServerPlayer *player) const
    {
        if (player->getHp() <= 0)
            room->enterDying(player, NULL);
    }
};

NosGuhuoCard::NosGuhuoCard()
{
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "nosguhuo";
}

bool NosGuhuoCard::nosguhuo(ServerPlayer *yuji) const
{
    Room *room = yuji->getRoom();
    QList<ServerPlayer *> players = room->getOtherPlayers(yuji);
    QSet<ServerPlayer *> questioned;

    QList<int> used_cards;
    QList<CardsMoveStruct> moves;
    foreach(int card_id, getSubcards())
        used_cards << card_id;
    room->setTag("NosGuhuoType", user_string);

    foreach (ServerPlayer *player, players) {
        if (player->getHp() <= 0) {
            LogMessage log;
            log.type = "#GuhuoCannotQuestion";
            log.from = player;
            log.arg = QString::number(player->getHp());
            room->sendLog(log);

            room->setEmotion(player, "no-question");
            continue;
        }

        QString choice = room->askForChoice(player, "nosguhuo", "noquestion+question");
        if (choice == "question") {
            room->setEmotion(player, "question");
            questioned << player;
        } else
            room->setEmotion(player, "no-question");

        LogMessage log;
        log.type = "#GuhuoQuery";
        log.from = player;
        log.arg = choice;

        room->sendLog(log);
    }

    LogMessage log;
    log.type = "$GuhuoResult";
    log.from = yuji;
    log.card_str = QString::number(subcards.first());
    room->sendLog(log);

    bool success = false;
    if (questioned.isEmpty()) {
        success = true;
        foreach(ServerPlayer *player, players)
            room->setEmotion(player, ".");

        CardMoveReason reason(CardMoveReason::S_REASON_USE, yuji->objectName(), QString(), "nosguhuo");
        CardsMoveStruct move(used_cards, yuji, NULL, Player::PlaceUnknown, Player::PlaceTable, reason);
        moves.append(move);
        room->moveCardsAtomic(moves, true);
    } else {
        const Card *card = Sanguosha->getCard(subcards.first());
        bool real;
        if (user_string == "peach+analeptic")
            real = card->objectName() == yuji->tag["NosGuhuoSaveSelf"].toString();
        else if (user_string == "slash")
            real = card->objectName().contains("slash");
        else if (user_string == "normal_slash")
            real = card->objectName() == "slash";
        else
            real = card->match(user_string);

        success = real && card->getSuit() == Card::Heart;
        if (success) {
            CardMoveReason reason(CardMoveReason::S_REASON_USE, yuji->objectName(), QString(), "nosguhuo");
            CardsMoveStruct move(used_cards, yuji, NULL, Player::PlaceUnknown, Player::PlaceTable, reason);
            moves.append(move);
            room->moveCardsAtomic(moves, true);
        } else {
            room->moveCardTo(this, yuji, NULL, Player::DiscardPile,
                             CardMoveReason(CardMoveReason::S_REASON_PUT, yuji->objectName(), QString(), "nosguhuo"), true);
        }
        foreach (ServerPlayer *player, players) {
            room->setEmotion(player, ".");
            if (questioned.contains(player)) {
                if (real)
                    room->loseHp(player);
                else
                    player->drawCards(1, "nosguhuo");
            }
        }
    }
    yuji->tag.remove("NosGuhuoSaveSelf");
    yuji->tag.remove("NosGuhuoSlash");
    return success;
}

bool NosGuhuoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return false;
    }

    const Card *_card = Self->tag.value("nosguhuo").value<const Card *>();
    if (_card == NULL)
        return false;

    Card *card = Sanguosha->cloneCard(_card);
    card->setCanRecast(false);
    card->deleteLater();
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool NosGuhuoCard::targetFixed() const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetFixed();
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *_card = Self->tag.value("nosguhuo").value<const Card *>();
    if (_card == NULL)
        return false;

    Card *card = Sanguosha->cloneCard(_card);
    card->setCanRecast(false);
    card->deleteLater();
    return card && card->targetFixed();
}

bool NosGuhuoCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetsFeasible(targets, Self);
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    const Card *_card = Self->tag.value("nosguhuo").value<const Card *>();
    if (_card == NULL)
        return false;

    Card *card = Sanguosha->cloneCard(_card);
    card->setCanRecast(false);
    card->deleteLater();
    return card && card->targetsFeasible(targets, Self);
}

const Card *NosGuhuoCard::validate(CardUseStruct &card_use) const
{
    ServerPlayer *yuji = card_use.from;
    Room *room = yuji->getRoom();

    QString to_nosguhuo = user_string;
    if (user_string == "slash"
            && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        QStringList nosguhuo_list;
        nosguhuo_list << "slash";
        if (!Config.BanPackages.contains("maneuvering"))
            nosguhuo_list << "normal_slash" << "thunder_slash" << "fire_slash";
        to_nosguhuo = room->askForChoice(yuji, "nosguhuo_slash", nosguhuo_list.join("+"));
        yuji->tag["NosGuhuoSlash"] = QVariant(to_nosguhuo);
    }
    room->broadcastSkillInvoke("nosguhuo");

    LogMessage log;
    log.type = card_use.to.isEmpty() ? "#GuhuoNoTarget" : "#Guhuo";
    log.from = yuji;
    log.to = card_use.to;
    log.arg = to_nosguhuo;
    log.arg2 = "nosguhuo";

    room->sendLog(log);

    if (nosguhuo(card_use.from)) {
        const Card *card = Sanguosha->getCard(subcards.first());
        QString user_str;
        if (to_nosguhuo == "slash") {
            if (card->isKindOf("Slash"))
                user_str = card->objectName();
            else
                user_str = "slash";
        } else if (to_nosguhuo == "normal_slash")
            user_str = "slash";
        else
            user_str = to_nosguhuo;
        Card *use_card = Sanguosha->cloneCard(user_str, card->getSuit(), card->getNumber());
        use_card->setSkillName("nosguhuo");
        use_card->addSubcard(subcards.first());
        use_card->deleteLater();
        return use_card;
    } else
        return NULL;
}

const Card *NosGuhuoCard::validateInResponse(ServerPlayer *yuji) const
{
    Room *room = yuji->getRoom();
    room->broadcastSkillInvoke("nosguhuo");

    QString to_nosguhuo;
    if (user_string == "peach+analeptic") {
        QStringList nosguhuo_list;
        nosguhuo_list << "peach";
        if (!Config.BanPackages.contains("maneuvering"))
            nosguhuo_list << "analeptic";
        to_nosguhuo = room->askForChoice(yuji, "nosguhuo_saveself", nosguhuo_list.join("+"));
        yuji->tag["NosGuhuoSaveSelf"] = QVariant(to_nosguhuo);
    } else if (user_string == "slash") {
        QStringList nosguhuo_list;
        nosguhuo_list << "slash";
        if (!Config.BanPackages.contains("maneuvering"))
            nosguhuo_list << "normal_slash" << "thunder_slash" << "fire_slash";
        to_nosguhuo = room->askForChoice(yuji, "nosguhuo_slash", nosguhuo_list.join("+"));
        yuji->tag["NosGuhuoSlash"] = QVariant(to_nosguhuo);
    } else
        to_nosguhuo = user_string;

    LogMessage log;
    log.type = "#GuhuoNoTarget";
    log.from = yuji;
    log.arg = to_nosguhuo;
    log.arg2 = "nosguhuo";
    room->sendLog(log);

    if (nosguhuo(yuji)) {
        const Card *card = Sanguosha->getCard(subcards.first());
        QString user_str;
        if (to_nosguhuo == "slash") {
            if (card->isKindOf("Slash"))
                user_str = card->objectName();
            else
                user_str = "slash";
        } else if (to_nosguhuo == "normal_slash")
            user_str = "slash";
        else
            user_str = to_nosguhuo;
        Card *use_card = Sanguosha->cloneCard(user_str, card->getSuit(), card->getNumber());
        use_card->setSkillName("nosguhuo");
        use_card->addSubcard(subcards.first());
        use_card->deleteLater();
        return use_card;
    } else
        return NULL;
}

class NosGuhuo : public OneCardViewAsSkill
{
public:
    NosGuhuo() : OneCardViewAsSkill("nosguhuo")
    {
        filter_pattern = ".|.|.|hand";
        response_or_use = true;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (player->isKongcheng() || pattern.startsWith(".") || pattern.startsWith("@")) return false;
        for (int i = 0; i < pattern.length(); i++) {
            QChar ch = pattern[i];
            if (ch.isUpper() || ch.isDigit()) return false; // This is an extremely dirty hack!! For we need to prevent patterns like 'BasicCard'
        }
        return true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->isKongcheng();
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE
                || Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
            NosGuhuoCard *card = new NosGuhuoCard;
            card->setUserString(Sanguosha->currentRoomState()->getCurrentCardUsePattern());
            card->addSubcard(originalCard);
            return card;
        }

        const Card *c = Self->tag.value("nosguhuo").value<const Card *>();
        if (c) {
            NosGuhuoCard *card = new NosGuhuoCard;
            if (!c->objectName().contains("slash"))
                card->setUserString(c->objectName());
            else
                card->setUserString(Self->tag["NosGuhuoSlash"].toString());
            card->addSubcard(originalCard);
            return card;
        } else
            return NULL;
    }

    virtual GuhuoDialog *getDialog() const
    {
        return GuhuoDialog::getInstance("nosguhuo");
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        if (!card->isKindOf("NosGuhuoCard"))
            return -2;
        else
            return -1;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        return !player->isKongcheng();
    }
};

class NosLeiji : public TriggerSkill
{
public:
    NosLeiji() : TriggerSkill("nosleiji")
    {
        events << CardResponded;
        view_as_skill = new dummyVS;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        const Card *card_star = data.value<CardResponseStruct>().m_card;
        if (card_star->isKindOf("Jink"))
            return nameList();
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *zhangjiao, QVariant &, ServerPlayer *) const
    {
        ServerPlayer *target = room->askForPlayerChosen(zhangjiao, room->getAlivePlayers(), objectName(), "leiji-invoke", true, true);
        if (target) {
            zhangjiao->broadcastSkillInvoke("nosleiji");

            JudgeStruct judge;
            judge.pattern = ".|spade";
            judge.good = false;
            judge.negative = true;
            judge.reason = objectName();
            judge.who = target;

            room->judge(judge);

            if (judge.isBad())
                room->damage(DamageStruct(objectName(), zhangjiao, target, 2, DamageStruct::Thunder));
        }
        return false;
    }
};

NosShensuCard::NosShensuCard()
{
}

bool NosShensuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("nosshensu");
    slash->setFlags("Global_NoDistanceChecking");
    slash->addCostcards(this->getSubcards());
    slash->deleteLater();
    return targets.isEmpty() && slash->targetFilter(targets, to_select, Self);
}

void NosShensuCard::use(Room *room, ServerPlayer *xiahouyuan, QList<ServerPlayer *> &targets) const
{
    if (targets.isEmpty()) return;
    switch (xiahouyuan->getMark("NosShensuIndex")) {
    case 1: {
        xiahouyuan->skip(Player::Judge, true);
        xiahouyuan->skip(Player::Draw, true);
        break;
    }
    case 2: {
        xiahouyuan->skip(Player::Play, true);
        break;
    }
    default:
        break;
    }
    Slash *slash = new Slash(Card::NoSuit, 0);
    slash->setSkillName("_nosshensu");
    room->useCard(CardUseStruct(slash, xiahouyuan, targets));
}

class NosShensuViewAsSkill : public ViewAsSkill
{
public:
    NosShensuViewAsSkill() : ViewAsSkill("nosshensu")
    {
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern.startsWith("@@nosshensu");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUsePattern().endsWith("2"))
            return selected.isEmpty() && to_select->isKindOf("EquipCard") && !Self->isJilei(to_select);
        else
            return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUsePattern().endsWith("2")) {
            if (cards.length() != 1) return NULL;
            NosShensuCard *card = new NosShensuCard;
            card->addSubcards(cards);
            return card;
        } else
            return cards.isEmpty() ? new NosShensuCard : NULL;
    }
};

class NosShensu : public TriggerSkill
{
public:
    NosShensu() : TriggerSkill("nosshensu")
    {
        events << EventPhaseChanging;
        view_as_skill = new NosShensuViewAsSkill;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *xiahouyuan, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(xiahouyuan)) return QStringList();
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to == Player::Judge && !xiahouyuan->isSkipped(Player::Judge)
                && !xiahouyuan->isSkipped(Player::Draw) && Slash::IsAvailable(xiahouyuan))
            return nameList();
        else if (Slash::IsAvailable(xiahouyuan) && change.to == Player::Play && !xiahouyuan->isSkipped(Player::Play)
                 && xiahouyuan->canDiscard(xiahouyuan, "he"))
            return nameList();
        return QStringList();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *xiahouyuan, QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        int index = 0;
        if (change.to == Player::Judge && !xiahouyuan->isSkipped(Player::Judge)
                && !xiahouyuan->isSkipped(Player::Draw) && Slash::IsAvailable(xiahouyuan))
            index = 1;
        else if (Slash::IsAvailable(xiahouyuan) && change.to == Player::Play && !xiahouyuan->isSkipped(Player::Play)
                 && xiahouyuan->canDiscard(xiahouyuan, "he"))
            index = 2;
        else
            return false;
        room->setPlayerMark(xiahouyuan, "NosShensuIndex", index);
        room->askForUseCard(xiahouyuan, "@@nosshensu" + QString::number(index), "@nosshensu" + QString::number(index));
        room->setPlayerMark(xiahouyuan, "NosShensuIndex", 0);
        return false;
    }
};

class NosLiegong : public TriggerSkill
{
public:
    NosLiegong() : TriggerSkill("nosliegong")
    {
        events << TargetSpecified;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card != NULL && use.card->isKindOf("Slash")) {
                ServerPlayer *to = use.to.at(use.index);

                if (to && to->isAlive()) {
                    if (to->getHandcardNum() >= player->getHp() || to->getHandcardNum() <= player->getAttackRange())
                        return nameList();
                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *player) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        int index = use.index;
        ServerPlayer *to = use.to.at(index);
        if (player->askForSkillInvoke(this, QVariant::fromValue(to))) {
            player->broadcastSkillInvoke(objectName());
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), to->objectName());

            if (to->getHandcardNum() >= player->getHp() || to->getHandcardNum() <= player->getAttackRange()) {
                LogMessage log;
                log.type = "#NoJink";
                log.from = to;
                room->sendLog(log);
                QVariantList jink_list = use.card->tag["Jink_List"].toList();
                jink_list[index] = 0;
                use.card->setTag("Jink_List", jink_list);
            }
        }

        return false;
    }
};

class NosKuanggu : public TriggerSkill
{
public:
    NosKuanggu() : TriggerSkill("noskuanggu")
    {
        events << Damage;
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (TriggerSkill::triggerable(player) && player->isWounded()) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.flags.contains("kuanggu")) {
                QStringList skill_list;
                for (int i = 0; i < damage.damage; i++)
                    skill_list << objectName();
                return skill_list;
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(this)) {
            player->broadcastSkillInvoke(objectName());
            RecoverStruct recover;
            recover.who = player;
            room->recover(player, recover);
        }
        return false;
    }

};


NosTianxiangCard::NosTianxiangCard()
{
}

void NosTianxiangCard::onEffect(const CardEffectStruct &effect) const
{
    DamageStruct damage = effect.from->tag.value("NosTianxiangDamage").value<DamageStruct>();
    damage.to = effect.to;
    damage.transfer = true;
    damage.transfer_reason = "nostianxiang";
    effect.from->tag["TransferDamage"] = QVariant::fromValue(damage);
}

class NosTianxiangViewAsSkill : public OneCardViewAsSkill
{
public:
    NosTianxiangViewAsSkill() : OneCardViewAsSkill("nostianxiang")
    {
        filter_pattern = ".|heart|.|hand!";
        response_pattern = "@@nostianxiang";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        NosTianxiangCard *nostianxiangCard = new NosTianxiangCard;
        nostianxiangCard->addSubcard(originalCard);
        return nostianxiangCard;
    }
};

class NosTianxiang : public TriggerSkill
{
public:
    NosTianxiang() : TriggerSkill("nostianxiang")
    {
        events << DamageInflicted << DamageComplete;
        view_as_skill = new NosTianxiangViewAsSkill;
    }

    TriggerList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data) const
    {
        TriggerList list;
        if (triggerEvent == DamageInflicted) {
            if (TriggerSkill::triggerable(player) && player->canDiscard(player, "h"))
                list.insert(player, nameList());
        } else if (triggerEvent == DamageComplete) {
            DamageStruct damage = data.value<DamageStruct>();
            if (player && player->isAlive() && player->isWounded() && damage.transfer && damage.transfer_reason == objectName())
                list.insert(player, comList());
        }
        return list;
    }

    bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *xiaoqiao, QVariant &data, ServerPlayer *drawer) const
    {
        if (triggerEvent == DamageInflicted) {
            if (xiaoqiao->canDiscard(xiaoqiao, "h")) {
                xiaoqiao->tag["NosTianxiangDamage"] = data;
                return room->askForUseCard(xiaoqiao, "@@nostianxiang", "@nostianxiang-card", -1, Card::MethodDiscard);
            }
        } else {
            drawer->drawCards(drawer->getLostHp(), objectName());
        }
        return false;
    }
};

NosQiangxiCard::NosQiangxiCard()
{
}

bool NosQiangxiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty() || to_select == Self)
        return false;

    int rangefix = 0;
    if (!subcards.isEmpty() && Self->getWeapon() && Self->getWeapon()->getId() == subcards.first()) {
        const Weapon *card = qobject_cast<const Weapon *>(Self->getWeapon()->getRealCard());
        rangefix += card->getRange() - Self->getAttackRange(false);;
    }

    return Self->inMyAttackRange(to_select, rangefix);
}

void NosQiangxiCard::extraCost(Room *room, const CardUseStruct &card_use) const
{
    if (subcards.isEmpty())
        room->loseHp(card_use.from);
    else {
        CardMoveReason reason(CardMoveReason::S_REASON_THROW, card_use.from->objectName(), QString(), card_use.card->getSkillName(), QString());
        room->moveCardTo(this, NULL, Player::DiscardPile, reason, true);
    }
}

void NosQiangxiCard::onEffect(const CardEffectStruct &effect) const
{
    effect.to->getRoom()->damage(DamageStruct("nosqiangxi", effect.from, effect.to));
}

class NosQiangxi : public ViewAsSkill
{
public:
    NosQiangxi() : ViewAsSkill("nosqiangxi")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("NosQiangxiCard");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.isEmpty() && to_select->isKindOf("Weapon") && !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return new NosQiangxiCard;
        else if (cards.length() == 1) {
            NosQiangxiCard *card = new NosQiangxiCard;
            card->addSubcards(cards);

            return card;
        } else
            return NULL;
    }
};

class NosJieming : public MasochismSkill
{
public:
    NosJieming() : MasochismSkill("nosjieming")
    {
        view_as_skill = new dummyVS;
    }

    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
    {
        if (TriggerSkill::triggerable(player)) {
            DamageStruct damage = data.value<DamageStruct>();
            QStringList trigger_list;
            for (int i = 1; i <= damage.damage; i++) {
                trigger_list << objectName();
            }
            return trigger_list;
        }

        return QStringList();
    }

    virtual void onDamaged(ServerPlayer *xunyu, const DamageStruct &) const
    {
        Room *room = xunyu->getRoom();
        ServerPlayer *to = room->askForPlayerChosen(xunyu, room->getAlivePlayers(), objectName(), "nosjieming-invoke", true, true);
        if (to != NULL) {
            xunyu->broadcastSkillInvoke(objectName());
            to->fillHandCards(qMin(5, to->getMaxHp()), objectName());
        }
    }
};

class NosLianhuan : public OneCardViewAsSkill
{
public:
    NosLianhuan() : OneCardViewAsSkill("noslianhuan")
    {
        filter_pattern = ".|club|.|hand";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        IronChain *chain = new IronChain(originalCard->getSuit(), originalCard->getNumber());
        chain->addSubcard(originalCard);
        chain->setSkillName(objectName());
        return chain;
    }
};

class NosNiepan : public TriggerSkill
{
public:
    NosNiepan() : TriggerSkill("nosniepan")
    {
        events << AskForPeaches;
        frequency = Limited;
        limit_mark = "@nosnirvana";
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *target, QVariant &data, ServerPlayer * &) const
    {
        if (TriggerSkill::triggerable(target) && target->getMark("@nosnirvana") > 0) {
            DyingStruct dying_data = data.value<DyingStruct>();

            if (target->getHp() > 0)
                return QStringList();

            if (dying_data.who != target)
                return QStringList();
            return nameList();
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *pangtong, QVariant &, ServerPlayer *) const
    {
        if (pangtong->askForSkillInvoke(this)) {
            pangtong->broadcastSkillInvoke(objectName());

            room->removePlayerMark(pangtong, "@nosnirvana");

            pangtong->throwAllHandCardsAndEquips();
            QList<const Card *> tricks = pangtong->getJudgingArea();
            foreach (const Card *trick, tricks) {
                CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, pangtong->objectName());
                room->throwCard(trick, reason, NULL);
            }

            if (pangtong->isChained())
                room->setPlayerProperty(pangtong, "chained", false);

            if (!pangtong->faceUp())
                pangtong->turnOver();

            pangtong->drawCards(3, objectName());
            room->recover(pangtong, RecoverStruct(pangtong, NULL, 3 - pangtong->getHp()));
        }

        return false;
    }
};

class NosHuoji : public OneCardViewAsSkill
{
public:
    NosHuoji() : OneCardViewAsSkill("noshuoji")
    {
        filter_pattern = ".|red|.|hand";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        FireAttack *fire_attack = new FireAttack(originalCard->getSuit(), originalCard->getNumber());
        fire_attack->addSubcard(originalCard->getId());
        fire_attack->setSkillName(objectName());
        return fire_attack;
    }
};

class NosKanpo : public OneCardViewAsSkill
{
public:
    NosKanpo() : OneCardViewAsSkill("noskanpo")
    {
        filter_pattern = ".|black|.|hand";
        response_pattern = "nullification";
        response_or_use = true;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Card *ncard = new Nullification(originalCard->getSuit(), originalCard->getNumber());
        ncard->addSubcard(originalCard);
        ncard->setSkillName(objectName());
        return ncard;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        return !player->isNude() || !player->getHandPile().isEmpty();
    }
};

NostalWindPackage::NostalWindPackage()
    : Package("nostal_wind")
{
    General *nos_caoren = new General(this, "nos_caoren", "wei");
    nos_caoren->addSkill(new NosJushou);

    General *nos_zhoutai = new General(this, "nos_zhoutai", "wu");
    nos_zhoutai->addSkill(new NosBuqu);
    nos_zhoutai->addSkill(new NosBuquRemove);
    nos_zhoutai->addSkill(new NosBuquClear);
    related_skills.insertMulti("nosbuqu", "#nosbuqu-remove");
    related_skills.insertMulti("nosbuqu", "#nosbuqu-clear");

    General *nos_zhangjiao = new General(this, "nos_zhangjiao$", "qun", 3);
    nos_zhangjiao->addSkill(new NosLeiji);
    nos_zhangjiao->addSkill("nolguidao");
    nos_zhangjiao->addSkill("huangtian");

    General *nos_yuji = new General(this, "nos_yuji", "qun", 3);
    nos_yuji->addSkill(new NosGuhuo);

    General *nos_xiahouyuan = new General(this, "nos_xiahouyuan", "wei", 4, true, true);
    nos_xiahouyuan->addSkill(new NosShensu);

    General *nos_huangzhong = new General(this, "nos_huangzhong", "shu", 4, true, true);
    nos_huangzhong->addSkill(new NosLiegong);

    General *nos_weiyan = new General(this, "nos_weiyan", "shu", 4, true, true);
    nos_weiyan->addSkill(new NosKuanggu);

    General *nos_xiaoqiao = new General(this, "nos_xiaoqiao", "wu", 3, false, true);
    nos_xiaoqiao->addSkill(new NosTianxiang);
    nos_xiaoqiao->addSkill("nolhongyan");

    addMetaObject<NosGuhuoCard>();
    addMetaObject<NosShensuCard>();
    addMetaObject<NosTianxiangCard>();
}

NostalFirePackage::NostalFirePackage()
    : Package("nostal_fire")
{
    General *nos_dianwei = new General(this, "nos_dianwei", "wei", 4, true, true);
    nos_dianwei->addSkill(new NosQiangxi);

    General *nos_xunyu = new General(this, "nos_xunyu", "wei", 3, true, true);
    nos_xunyu->addSkill("quhu");
    nos_xunyu->addSkill(new NosJieming);

    General *nos_pangtong = new General(this, "nos_pangtong", "shu", 3, true, true);
    nos_pangtong->addSkill(new NosLianhuan);
    nos_pangtong->addSkill(new NosNiepan);

    General *nos_wolong = new General(this, "nos_wolong", "shu", 3, true, true);
    nos_wolong->addSkill("bazhen");
    nos_wolong->addSkill(new NosHuoji);
    nos_wolong->addSkill(new NosKanpo);

    General *nos_taishici = new General(this, "nos_taishici", "wu", 4, true, true);
    nos_taishici->addSkill("tianyi");

    addMetaObject<NosQiangxiCard>();
}

ADD_PACKAGE(NostalWind)
ADD_PACKAGE(NostalFire)
