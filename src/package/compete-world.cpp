#include "compete-world.h"
#include "client.h"
#include "engine.h"
#include "general.h"
#include "room.h"



Demobilizing::Demobilizing(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("demobilizing");
}

bool Demobilizing::targetRated(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    return targets.isEmpty() && to_select->hasEquip();
}

void Demobilizing::onEffect(const CardEffectStruct &effect) const
{
    QList<const Card *> equips = effect.to->getEquips();
    if (equips.isEmpty()) return;
    DummyCard *card = new DummyCard;
    foreach (const Card *equip, equips) {
        card->addSubcard(equip);
    }
    if (card->subcardsLength() > 0)
        effect.to->obtainCard(card);
}

class FloweringTreeDiscard : public ViewAsSkill
{
public:
    FloweringTreeDiscard() : ViewAsSkill("floweringtree_discard")
    {
        response_pattern = "@@floweringtree_discard";
    }


    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.length() < 2 && !Self->isJilei(to_select);
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (!cards.isEmpty()) {
            DummyCard *discard = new DummyCard;
            discard->addSubcards(cards);
            return discard;
        }

        return NULL;
    }
};

FloweringTree::FloweringTree(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("flowering_tree");
    target_fixed = true;
}

bool FloweringTree::targetRated(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.isEmpty();
}

QList<ServerPlayer *> FloweringTree::defaultTargets(Room *, ServerPlayer *source) const
{
    return QList<ServerPlayer *>() << source;
}

bool FloweringTree::isAvailable(const Player *player) const
{
    return !player->isProhibited(player, this) && TrickCard::isAvailable(player);
}

void FloweringTree::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    const Card *card = room->askForCard(effect.to, "@@floweringtree_discard", "@floweringtree-discard", QVariant(), Card::MethodNone);
    if (card->subcardsLength() > 0) {
        int x = card->subcardsLength();
        QList<int> to_discard = card->getSubcards();
        foreach (int id, to_discard) {
            if (Sanguosha->getCard(id)->getTypeId() == Card::TypeEquip) {
                x++;
                break;
            }
        }
        CardMoveReason mreason(CardMoveReason::S_REASON_THROW, effect.to->objectName(), QString(), objectName(), QString());
        room->throwCard(card, mreason, effect.to);
        effect.to->drawCards(x, objectName());
    }
}




BrokenHalberd::BrokenHalberd(Suit suit, int number)
    : Weapon(suit, number, 0)
{
    setObjectName("broken_halberd");
}



class WomenDressSkill : public ArmorSkill
{
public:
    WomenDressSkill() : ArmorSkill("women_dress")
    {
        events << CardsMoveOneTime;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (triggerEvent == CardsMoveOneTime && player->getMark("Armor_Nullified") == 0) {
            QVariantList move_datas = data.toList();
            foreach(QVariant move_data, move_datas) {
                CardsMoveOneTimeStruct move = move_data.value<CardsMoveOneTimeStruct>();
                QString source_name = move.reason.m_playerId;
                ServerPlayer *source = room->findPlayer(source_name);
                if (source && source->ingoreArmor(player)) continue;

                if (move.from == player && move.from_places.contains(Player::PlaceEquip)) {
                    for (int i = 0; i < move.card_ids.size(); i++) {
                        if (move.from_places[i] != Player::PlaceEquip) continue;
                        const Card *card = Sanguosha->getEngineCard(move.card_ids[i]);
                        if (card->objectName() == objectName()) {
                            return nameList();
                        }
                    }
                }
                if (move.to == player && move.to_place == Player::PlaceEquip) {
                    for (int i = 0; i < move.card_ids.size(); i++) {
                        const Card *card = Sanguosha->getEngineCard(move.card_ids[i]);
                        if (card->objectName() == objectName()) {
                            return nameList();
                        }
                    }
                }

            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName(), false);
        room->setEmotion(player, "armor/women_dress");

        room->askForDiscard(player, "women_dress", 1, 1, false, true);
        return false;
    }
};

WomenDress::WomenDress(Suit suit, int number)
    : Armor(suit, number)
{
    setObjectName("women_dress");
}














class Falu : public TriggerSkill
{
public:
    Falu() : TriggerSkill("falu")
    {
        events << TurnStart << CardsMoveOneTime;
        frequency = Compulsory;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if ((triggerEvent == TurnStart && room->getTag("FirstRound").toBool())) {
            QList<ServerPlayer *> zhangqiyings = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *zhangqiying, zhangqiyings)
                skill_list.insert(zhangqiying, nameList());

        } else if (triggerEvent == CardsMoveOneTime && TriggerSkill::triggerable(player)) {
            QVariantList move_datas = data.toList();
            foreach(QVariant move_data, move_datas) {
                CardsMoveOneTimeStruct move = move_data.value<CardsMoveOneTimeStruct>();
                if (move.from == player && move.to_place == Player::DiscardPile
                        && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                    QStringList names;
                    names << "@ziwei" << "@houtu" << "@yuqing" << "@gouchen";
                    for (int i = 0; i < move.card_ids.length(); i++) {
                        int id = move.card_ids.at(i);
                        if (move.from_places[i] == Player::PlaceHand || move.from_places[i] == Player::PlaceEquip) {
                            int suit = (int) Sanguosha->getCard(id)->getSuit();
                            if (player->getMark(names.at(suit)) == 0) {
                                skill_list.insert(player, nameList());
                                return skill_list;
                            }
                        }
                    }
                }
            }
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *player) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        player->broadcastSkillInvoke(objectName());

        QStringList names, addmarks;
        names << "@ziwei" << "@houtu" << "@yuqing" << "@gouchen";

        if (triggerEvent == TurnStart)
            addmarks = names;
        else {
            QVariantList move_datas = data.toList();
            foreach(QVariant move_data, move_datas) {
                CardsMoveOneTimeStruct move = move_data.value<CardsMoveOneTimeStruct>();
                if (move.from == player && move.to_place == Player::DiscardPile
                        && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                    for (int i = 0; i < move.card_ids.length(); i++) {
                        int id = move.card_ids.at(i);
                        if (move.from_places[i] == Player::PlaceHand || move.from_places[i] == Player::PlaceEquip) {
                            int suit = (int) Sanguosha->getCard(id)->getSuit();
                            QString mark_name = names.at(suit);
                            if (player->getMark(mark_name) == 0 && !addmarks.contains(mark_name))
                                addmarks << mark_name;
                        }
                    }
                }
            }
        }

        foreach (QString mark_name, names) {
            if (addmarks.contains(mark_name))
                player->gainMark(mark_name);
        }
        return false;
    }
};

class ZhenyiViewAsSkill : public OneCardViewAsSkill
{
public:
    ZhenyiViewAsSkill() : OneCardViewAsSkill("zhenyi")
    {
        filter_pattern = ".|.|.|hand";
        response_or_use = true;
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern.contains("peach") && player->getHp() < 1 && player->getMark("@houtu") > 0;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Peach *peach = new Peach(originalCard->getSuit(), originalCard->getNumber());
        peach->addSubcard(originalCard->getId());
        peach->setSkillName(objectName());
        return peach;
    }
};

class Zhenyi : public TriggerSkill
{
public:
    Zhenyi() : TriggerSkill("zhenyi")
    {
        events << PreCardUsed << AskForRetrial << DamageCaused << Damaged;
        view_as_skill = new ZhenyiViewAsSkill;
    }

    virtual void record(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Peach") && use.card->getSkillName() == objectName())
                player->loseMark("@houtu");
        }
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == AskForRetrial) {
            if (player->getMark("@ziwei") > 0) return nameList();
        } else if (triggerEvent == DamageCaused) {
            if (player->getMark("@yuqing") > 0) return nameList();
        } else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.nature != DamageStruct::Normal && player->getMark("@gouchen") > 0) return nameList();
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == AskForRetrial) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (player->askForSkillInvoke(objectName(), data)) {
                player->broadcastSkillInvoke(objectName());
                player->loseMark("@ziwei");
                QString choice = room->askForChoice(player, objectName(), "spade+heart", data, "@zhenyi-choose:"+judge->who->objectName()+"::"+judge->reason);
                if (choice == "heart") {
                    room->setCardFlag(judge->card, "ZhenyiChangeToHeart");
                    room->acquireSkill(judge->who, "#zhenyi-judge", false);
                    QList<const Card *> cards;
                    cards << judge->card;
                    room->filterCards(judge->who, cards, false);
                    judge->updateResult();
                } else if (choice == "spade") {
                    room->setCardFlag(judge->card, "ZhenyiChangeToSpade");
                    room->acquireSkill(judge->who, "#zhenyi-judge", false);
                    QList<const Card *> cards;
                    cards << judge->card;
                    room->filterCards(judge->who, cards, false);
                    judge->updateResult();
                }
            }
        } else if (triggerEvent == DamageCaused) {
            if (player->askForSkillInvoke(objectName(), data)) {
                player->broadcastSkillInvoke(objectName());
                player->loseMark("@yuqing");
                JudgeStruct judge;
                judge.reason = objectName();
                judge.pattern = ".|black";
                judge.who = player;
                room->judge(judge);
                if (judge.isGood()) {
                    DamageStruct damage = data.value<DamageStruct>();
                    damage.damage++;
                    data = QVariant::fromValue(damage);
                }
            }
        } else if (triggerEvent == Damaged) {
            if (player->askForSkillInvoke(objectName(), data)) {
                player->broadcastSkillInvoke(objectName());
                player->loseMark("@gouchen");

                QList<Card::CardType> types;
                types << Card::TypeBasic << Card::TypeTrick << Card::TypeEquip;

                foreach (Card::CardType type, types) {
                    if (player->isDead()) break;
                    QList<int> cards;
                    foreach (int card_id, room->getDrawPile()) {
                        if (type == Sanguosha->getCard(card_id)->getTypeId())
                            cards << card_id;
                    }
                    if (!cards.isEmpty()){
                        int index = qrand() % cards.length();
                        int id = cards.at(index);
                        player->obtainCard(Sanguosha->getCard(id), false);
                    }
                }
            }
        }
        return false;
    }
};

class ZhenyiJudge : public FilterSkill
{
public:
    ZhenyiJudge() : FilterSkill("#zhenyi-judge")
    {

    }

    bool viewFilter(const Card *to_select) const
    {
        return (to_select->hasFlag("ZhenyiChangeToHeart") || to_select->hasFlag("ZhenyiChangeToSpade")) &&
                Sanguosha->currentRoom()->getCardPlace(to_select->getEffectiveId()) == Player::PlaceJudge;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        WrappedCard *new_card = Sanguosha->getWrappedCard(originalCard->getEffectiveId());
        new_card->setSkillName("zhenyi");
        if (originalCard->hasFlag("ZhenyiChangeToHeart"))
            new_card->setSuit(Card::Heart);
        else if (originalCard->hasFlag("ZhenyiChangeToSpade"))
            new_card->setSuit(Card::Spade);
        new_card->setNumber(5);
        new_card->setModified(true);
        return new_card;
    }
};

class Dianhua : public PhaseChangeSkill
{
public:
    Dianhua() : PhaseChangeSkill("dianhua")
    {
        frequency = Frequent;
    }

    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *target, QVariant &, ServerPlayer* &) const
    {
        if (PhaseChangeSkill::triggerable(target) && getGuanxingNum(target) > 0
                && (target->getPhase() == Player::Start || target->getPhase() == Player::Finish))
            return nameList();
        return QStringList();
    }

    virtual bool onPhaseChange(ServerPlayer *zhuge) const
    {
        if (zhuge->askForSkillInvoke(this)) {
            zhuge->broadcastSkillInvoke(objectName());
            Room *room = zhuge->getRoom();
            QList<int> guanxing = room->getNCards(getGuanxingNum(zhuge));

            LogMessage log;
            log.type = "$ViewDrawPile";
            log.from = zhuge;
            log.card_str = IntList2StringList(guanxing).join("+");
            room->sendLog(log, zhuge);

            room->askForGuanxing(zhuge, guanxing, Room::GuanxingUpOnly);
        }

        return false;
    }

    virtual int getGuanxingNum(ServerPlayer *target) const
    {
        return target->getMark("@ziwei") + target->getMark("@houtu") + target->getMark("@yuqing") + target->getMark("@gouchen");
    }
};



























QianxiniCard::QianxiniCard()
{
    will_throw = false;
}

void QianxiniCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    QList<int> cards = getSubcards();

    qShuffle(cards);

    CardMoveReason reason(CardMoveReason::S_REASON_PUT, source->objectName(), "qianxini", QString());
    CardsMoveOneTimeStruct moveOneTime;

    moveOneTime.card_ids = cards;
    moveOneTime.from_places << Player::PlaceHand;
    moveOneTime.to_place = Player::DrawPile;
    moveOneTime.reason = reason;
    moveOneTime.from = source;
    moveOneTime.to = NULL;
    moveOneTime.is_last_handcard = true;
    foreach (int id, cards) {
        moveOneTime.open << false;
        const Card *card = Sanguosha->getCard(id);
        moveOneTime.cards << card->toRealString();
    }


    CardsMoveStruct move(cards, source, NULL, Player::PlaceHand, Player::DrawPile, reason);
    move.from_player_name = source->objectName();
    move.open = false;
    move.is_open_pile = false;

    move.is_last_handcard = true;
    foreach (const Card *c, source->getHandcards()) {
        if (!cards.contains(c->getEffectiveId())) {
            move.is_last_handcard = false;
            moveOneTime.is_last_handcard = false;
            break;
        }
    }


    QVariantList pre_move_datas;

    QVariant data = QVariant::fromValue(moveOneTime);
    pre_move_datas << data;


    foreach (ServerPlayer *player, room->getAllPlayers(true)) {
        QVariant data = QVariant::fromValue(pre_move_datas);
        room->getThread()->trigger(BeforeCardsMove, room, player, data);

    }


    QList<CardsMoveStruct> cards_moves;
    cards_moves << move;
    room->notifyMoveCards(true, cards_moves, false);



    for (int ii = 0; ii < cards.length(); ii++) {
        const Card *card = Sanguosha->getCard(cards.at(ii));
        source->removeCard(card, Player::PlaceHand);
    }

    int alive_num = room->alivePlayerCount();

    for (int ii = 0; ii < cards.length(); ii++) {

        room->getDrawPile().insert((ii+1)*alive_num-1 , cards.at(ii));

        room->setCardMapping(cards.at(ii), NULL, Player::DrawPile);
    }

    room->notifyMoveCards(false, cards_moves, false);

    QVariantList move_datas;

    QVariant data2 = QVariant::fromValue(moveOneTime);
    move_datas << data2;

    foreach (ServerPlayer *player, room->getAllPlayers(true)) {
        QVariant data = QVariant::fromValue(move_datas);
        room->getThread()->trigger(PreCardsMoveOneTime, room, player, data);
    }
    foreach (ServerPlayer *player, room->getAllPlayers(true)) {
        QVariant data = QVariant::fromValue(move_datas);
        room->getThread()->trigger(CardsMoveOneTime, room, player, data);
    }

//    for (int ii = 0; ii < room->getDrawPile().length(); ii++) {

//        int id = room->getDrawPile().at(ii);
//        if (cards.contains(id)) {

//            LogMessage log;
//            log.type = "#Qianxinlog";
//            log.card_str = Sanguosha->getCard(id)->toString();
//            log.arg = QString::number(ii+1);
//            room->sendLog(log);

//        }
//    }

    ServerPlayer *target = targets.first();

    QVariantList idlist;

    foreach (int id, cards) {
        if (room->getCardPlace(id) == Player::DrawPile) {
            idlist << id;
        }
    }

    if (idlist.isEmpty()) return;

    room->addPlayerMark(target, "@qianxini_target");
    room->setPlayerMark(source, "#qianxini_num", idlist.length());

    source->tag["QianxiniCards"] = idlist;
    source->tag["QianxiniTarget"] = QVariant::fromValue(target);
}

class QianxiniViewAsSkill : public ViewAsSkill
{
public:
    QianxiniViewAsSkill() : ViewAsSkill("qianxini")
    {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        int x = Self->getMark("drawpile_length");
        int y = Self->getAliveSiblings().length();

        return x >= (selected.length()+1) * y && !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return NULL;

        QianxiniCard *qianxin_card = new QianxiniCard;
        qianxin_card->addSubcards(cards);
        qianxin_card->setSkillName(objectName());
        return qianxin_card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("QianxiniCard") && player->getMark("#qianxini_num")==0;
    }
};

class Qianxini : public TriggerSkill
{
public:
    Qianxini() : TriggerSkill("qianxini")
    {
        events << ViewDrawPileCard << PreCardsMoveOneTime << PlayCard << EventPhaseStart;
        view_as_skill = new QianxiniViewAsSkill;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (triggerEvent == ViewDrawPileCard) {

            QList<ServerPlayer *> all_players = room->getAllPlayers();
            foreach (ServerPlayer *zhanggong, all_players) {
                QVariantList carddatas = zhanggong->tag["QianxiniCards"].toList();

                QVariantList idlist;

                foreach (QVariant card_data, carddatas) {
                    int card_id = card_data.toInt();
                    if (room->getDrawPile().contains(card_id)) {
                        idlist << card_data;
                    }
                }

                room->setPlayerMark(zhanggong, "#qianxini_num", idlist.length());

                zhanggong->tag["QianxiniCards"] = idlist;

                QVariantList carddatas2 = zhanggong->tag["QianxiniReceivedCards"].toList();

                if (idlist.isEmpty() && carddatas2.isEmpty()) {
                    ServerPlayer *target = zhanggong->tag["QianxiniTarget"].value<ServerPlayer *>();
                    zhanggong->tag.remove("QianxiniTarget");
                    if (target)
                        room->removePlayerMark(target, "@qianxini_target");
                }

            }

        } else if (triggerEvent == PreCardsMoveOneTime) {
            QVariantList carddatas2 = player->tag["QianxiniReceivedCards"].toList();
            QVariantList idlist2 = carddatas2;

            ServerPlayer *target = player->tag["QianxiniTarget"].value<ServerPlayer *>();

            foreach (QVariant card_data, carddatas2) {
                int card_id = card_data.toInt();
                if (room->getCardOwner(card_id) != target || room->getCardPlace(card_id) != Player::PlaceHand) {
                    idlist2.removeOne(card_data);
                }
            }

            if (idlist2.isEmpty() && target)
                room->removePlayerTip(target, "#qianxini_received");

            QVariantList carddatas = player->tag["QianxiniCards"].toList();

            QVariantList idlist;
            QList<int> ids;

            foreach (QVariant card_data, carddatas) {
                int card_id = card_data.toInt();
                if (room->getCardPlace(card_id) == Player::DrawPile) {
                    idlist << card_data;
                }
                if (room->getCardOwner(card_id) == target && room->getCardPlace(card_id) == Player::PlaceHand
                        && target->getPhase() != Player::NotActive) {
                    room->addPlayerTip(target, "#qianxini_received");
                    idlist2 << card_data;
                    ids << card_id;
                }
            }

            if (!ids.isEmpty()) {
                QList<ServerPlayer *> all_players = room->getAllPlayers(true);
                all_players.removeOne(target);
                LogMessage log;
                log.type = "#QianxiniReceived";
                log.from = target;
                log.arg = QString::number(ids.length());
                room->sendLog(log, all_players);

                LogMessage b;
                b.type = "$QianxiniReceived";
                b.from = target;
                b.card_str = IntList2StringList(ids).join("+");
                room->doNotify(target, QSanProtocol::S_COMMAND_LOG_SKILL, b.toVariant());
            }

            room->setPlayerMark(player, "#qianxini_num", idlist.length());

            player->tag["QianxiniCards"] = idlist;
            player->tag["QianxiniReceivedCards"] = idlist2;

            if (idlist.isEmpty() && idlist2.isEmpty()) {
                player->tag.remove("QianxiniTarget");
                if (target)
                    room->removePlayerMark(target, "@qianxini_target");
            }

        } else if (triggerEvent == PlayCard && TriggerSkill::triggerable(player)) {
            room->setPlayerMark(player, "drawpile_length", room->getDrawPile().length());


        } else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::NotActive) {
            room->removePlayerTip(player, "#qianxini_received");

            QList<ServerPlayer *> all_players = room->getAllPlayers();
            foreach (ServerPlayer *zhanggong, all_players) {
                zhanggong->tag.remove("QianxiniReceivedCards");
                QVariantList carddatas = zhanggong->tag["QianxiniCards"].toList();
                if (carddatas.isEmpty()) {
                    ServerPlayer *target = zhanggong->tag["QianxiniTarget"].value<ServerPlayer *>();
                    zhanggong->tag.remove("QianxiniTarget");
                    if (target)
                        room->removePlayerMark(target, "@qianxini_target");
                }
            }

        }
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Discard && player->isAlive()) {
            QList<ServerPlayer *> all_players = room->getAllPlayers();
            foreach (ServerPlayer *zhanggong, all_players) {
                ServerPlayer *target = zhanggong->tag["QianxiniTarget"].value<ServerPlayer *>();
                QVariantList carddatas = zhanggong->tag["QianxiniReceivedCards"].toList();
                if (target == player && !carddatas.isEmpty()) {
                    skill_list.insert(zhanggong, QStringList("qianxini!"));
                }
            }
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *target, QVariant &, ServerPlayer *zhanggong) const
    {
        room->sendCompulsoryTriggerLog(zhanggong, objectName());
        zhanggong->broadcastSkillInvoke(objectName());
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, zhanggong->objectName(), target->objectName());
        QStringList choicelist;
        choicelist << "maxcards";
        if (zhanggong->getHandcardNum() < 4)
            choicelist << "draw";
        QString choice = room->askForChoice(target, objectName(), choicelist.join("+"), QVariant(),
                                            "@qianxini-choice:"+zhanggong->objectName(), "draw+maxcards");

        if (choice == "draw" && zhanggong->getHandcardNum() < 4) {
            zhanggong->drawCards(4-zhanggong->getHandcardNum(), objectName());
        } else {
            if (target->getMaxCards() > 0)
                room->addPlayerMark(target, "Global_MaxcardsDecrease", 2);
        }
        return false;
    }
};

class ZhenxingSelect : public OneCardViewAsSkill
{
public:
    ZhenxingSelect() : OneCardViewAsSkill("zhenxing_select")
    {
        expand_pile = "#zhenxing",
        response_pattern = "@@zhenxing_select";
    }

    bool viewFilter(const Card *to_select) const
    {
        QList<int> cards = Self->getPile(expand_pile);

        bool cheak = false;

        foreach (int id, cards) {
            const Card *card = Sanguosha->getCard(id);
            if (card->sameSuitWith(to_select)) {
                cheak = !cheak;
                if (!cheak) break;
            }
        }

        return cheak && cards.contains(to_select->getEffectiveId());

    }

    const Card *viewAs(const Card *originalCard) const
    {
        return originalCard;
    }
};
class Zhenxing : public TriggerSkill
{
public:
    Zhenxing() : TriggerSkill("zhenxing")
    {
        events << EventPhaseStart << Damaged;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *target, QVariant &, ServerPlayer* &) const
    {
        if (TriggerSkill::triggerable(target) && (triggerEvent == Damaged || target->getPhase() == Player::Finish))
            return nameList();
        return QStringList();
    }

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(this)) {
            player->broadcastSkillInvoke(objectName());

            int x = room->askForChoice(player, objectName(), "1+2+3", QVariant(), "@zhenxing-choose").toInt();

            QList<int> ids = room->getNCards(x);
            room->notifyMoveToPile(player, ids, "zhenxing", Player::PlaceTable, true, true);
            const Card *card = room->askForCard(player, "@@zhenxing_select", "@zhenxing-select", QVariant(), Card::MethodNone);
            room->notifyMoveToPile(player, ids, "zhenxing", Player::PlaceTable, false, false);

            room->returnToTopDrawPile(ids);

            if (card) {
                ids.removeOne(card->getId());
                player->obtainCard(card);
            }

            if (!ids.isEmpty()) {
                DummyCard *dummy = new DummyCard(ids);
                CardMoveReason reason(CardMoveReason::S_REASON_PUT, player->objectName(), objectName(), QString());
                room->throwCard(dummy, reason, NULL);
                delete dummy;
            }
        }
        return false;
    }
};

FuhaiCard::FuhaiCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool FuhaiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    QList<const Player *> players = Self->getAliveSiblings();
    players.append(Self);

    int x = Self->getSeat();
    int x1 = x+1,x2 = x-1;
    if (x == 1)
        x2 = players.length();
    if (x == players.length())
        x1 = 1;

    return targets.isEmpty() && (to_select->getSeat() == x1 || to_select->getSeat() == x2) && !to_select->isKongcheng() && !to_select->hasFlag("fuhaiTargeted");
}

void FuhaiCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *weizhu = effect.from;
    ServerPlayer *first = effect.to;
    Room *room = weizhu->getRoom();

    QList<ServerPlayer *> players = room->getAlivePlayers();

    bool start = false;
    QList<ServerPlayer *> fuhai_targets;
    foreach (ServerPlayer *p, players) {
        if (p == weizhu || start) {
            start = true;
            fuhai_targets << p;
        }
    }
    foreach (ServerPlayer *p, players) {
        if (p == weizhu) break;
        fuhai_targets << p;
    }
    fuhai_targets.removeOne(weizhu);

    if (first != fuhai_targets.first()) {
        QList<ServerPlayer *> _targets;
        for (int i = fuhai_targets.length()-1; i >= 0; i--) {
            _targets << fuhai_targets[i];
        }
        fuhai_targets = _targets;
    }

    foreach (ServerPlayer *target, fuhai_targets) {
        if (target->isDead()) continue;
        if (weizhu->isDead() || weizhu->isKongcheng() || target->isKongcheng() || target->hasFlag("fuhaiTargeted")) break;

        room->addPlayerMark(weizhu, "fuhaiTimes");
        room->setPlayerFlag(target, "fuhaiTargeted");

        const Card *card1 = NULL;
        if (start) {
            start = false;
            card1 = this;
        } else {
            card1 = room->askForCard(weizhu, ".!", "@fuhai-show1::"+target->objectName(), QVariant(), Card::MethodNone);
            if (card1 == NULL)
                card1 = weizhu->getHandcards().first();

            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, weizhu->objectName(), target->objectName());
        }

        room->showCard(weizhu, card1->getEffectiveId());

        const Card *card2 = room->askForCard(target, ".!", "@fuhai-show2:"+weizhu->objectName(), QVariant(), Card::MethodNone);

        if (card2 == NULL)
            card2 = target->getHandcards().first();

        room->showCard(target, card2->getEffectiveId());
        room->getThread()->delay(3000);

        if (card1->getNumber() < card2->getNumber()) {
            room->throwCard(card2, target);
            int x = weizhu->getMark("fuhaiTimes");
            weizhu->drawCards(x, objectName());
            target->drawCards(x, objectName());
            room->setPlayerFlag(weizhu, "fuhaiStop");
            break;
        } else
            room->throwCard(card1, weizhu);
    }
}

class FuhaiViewAsSkill : public OneCardViewAsSkill
{
public:
    FuhaiViewAsSkill() : OneCardViewAsSkill("fuhai")
    {
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *weizhu) const
    {
        return !weizhu->hasFlag("fuhaiStop");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        FuhaiCard *card = new FuhaiCard;
        card->addSubcard(originalCard);
        card->setSkillName(objectName());
        return card;
    }
};

class Fuhai : public TriggerSkill
{
public:
    Fuhai() : TriggerSkill("fuhai")
    {
        events << EventPhaseChanging;
        view_as_skill = new FuhaiViewAsSkill;
    }

    bool triggerable(const ServerPlayer *) const
    {
        return false;
    }

    virtual void record(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (data.value<PhaseChangeStruct>().from == Player::Play) {
            room->setPlayerMark(player, "fuhaiTimes", 0);
            room->setPlayerFlag(player, "-fuhaiStop");
            QList<ServerPlayer *> players = room->getAlivePlayers();
            foreach (ServerPlayer *p, players) {
                room->setPlayerFlag(p, "-fuhaiTargeted");
            }
        }
    }
};



class TunanUseViewAsSkill : public OneCardViewAsSkill
{
public:
    TunanUseViewAsSkill() : OneCardViewAsSkill("tunan_use")
    {
        expand_pile = "#tunan";
    }

    bool viewFilter(const Card *to_select) const
    {
        return Self->getPile(expand_pile).contains(to_select->getEffectiveId());
    }

    const Card *viewAs(const Card *originalCard) const
    {
        QString choice = Self->tag["tunan_use"].toString();
        if (choice == "use") {
            originalCard->setFlags("Global_NoDistanceChecking");
            return originalCard;
        } else if (choice == "slash") {
            Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
            slash->addSubcard(originalCard);
            slash->setSkillName("_tunan");
            return slash;
        }
        return NULL;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern.startsWith("@@tunan_use");
    }

};

class TunanUse : public TriggerSkill
{
public:
    TunanUse() : TriggerSkill("tunan_use")
    {
        view_as_skill = new TunanUseViewAsSkill;
    }

    bool triggerable(const ServerPlayer *) const
    {
        return false;
    }

    QString getSelectBox() const
    {
        return "use+slash";
    }

    bool buttonEnabled(const QString &button_name, const QList<const Card *> &, const QList<const Player *> &) const
    {
        QStringList tunan_choices = Self->property("tunan_choices").toString().split("+");
        return tunan_choices.contains(button_name);
    }
};

TunanCard::TunanCard()
{
}

void TunanCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *target = effect.from;
    //ServerPlayer *target = effect.to;
    Room *room = target->getRoom();


    QList<ServerPlayer *> _guojia;
    _guojia.append(target);

    QList<int> yiji_cards = room->getNCards(1, false);
    room->returnToTopDrawPile(yiji_cards);

    int id = yiji_cards.first();
    const Card *card = Sanguosha->getCard(id);

    Slash *slash = new Slash(card->getSuit(), card->getNumber());
    slash->addSubcard(id);
    slash->setSkillName("_tunan");

    QStringList choicelist;

    QList<ServerPlayer *> tos1, tos2;


    if (card->isAvailable(target)) {
        tos1 = target->getCardDefautTargets(card, false);
        if (card->targetFixed() || !tos1.isEmpty())
            choicelist << "use";
    }
    if (slash->isAvailable(target)) {
        tos2 = target->getCardDefautTargets(slash, true);
        if (!tos2.isEmpty())
            choicelist << "slash";
    }

    QString pattern = "@@tunan_use!";
    QString prompt = "@tunan-use:::" + card->objectName();
    if (choicelist.isEmpty()) {
        pattern.chop(1);
        prompt = "@tunan-cancel:::" + card->objectName();
    }
    room->notifyMoveToPile(target, yiji_cards, "tunan", Player::PlaceTable, true, true);
    room->setPlayerProperty(target, "tunan_choices", choicelist.join("+"));
    bool notuse = room->askForUseCard(target, pattern, prompt, QVariant(), Card::MethodUse, false) == NULL;
    room->notifyMoveToPile(target, yiji_cards, "tunan", Player::PlaceTable, false, false);
    room->setPlayerProperty(target, "tunan_choices", QVariant());
    if (!choicelist.isEmpty() && notuse) {
        if (choicelist.contains("slash"))
            room->useCard(CardUseStruct(slash, target, tos2));
        if (choicelist.contains("use") && !card->isKindOf("Collateral") && !card->isKindOf("BeatAnother"))
            room->useCard(CardUseStruct(card, target, tos1));
    }
}

class Tunan : public ZeroCardViewAsSkill
{
public:
    Tunan() : ZeroCardViewAsSkill("tunan")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("TunanCard");
    }

    virtual const Card *viewAs() const
    {
        return new TunanCard;
    }
};

class Bijing : public TriggerSkill
{
public:
    Bijing() : TriggerSkill("bijing")
    {
        events << EventPhaseStart << PreCardsMoveOneTime;
        frequency = Compulsory;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == PreCardsMoveOneTime) {
            const Card *card = player->tag["bijingCard"].value<const Card *>();
            if (card == NULL) return;
            QVariantList move_datas = data.toList();
            foreach(QVariant move_data, move_datas) {
                CardsMoveOneTimeStruct move = move_data.value<CardsMoveOneTimeStruct>();
                if (player != move.from) return;
                int id = card->getEffectiveId();
                if (move.card_ids.contains(id) && move.from_places[move.card_ids.indexOf(id)] == Player::PlaceHand) {
                    player->tag.remove("bijingCard");
                    if (room->getCurrent() && room->getCurrent()->getPhase() != Player::NotActive)
                        room->setPlayerFlag(player, "bijingLostCard");
                    return;
                }
            }
        }
    }
    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (triggerEvent == EventPhaseStart) {
            switch (player->getPhase()) {
            case Player::Start: {
                const Card *card = player->tag["bijingCard"].value<const Card *>();
                if (card) {
                    int id = card->getEffectiveId();
                    if (room->getCardOwner(id) == player && room->getCardPlace(id) == Player::PlaceHand)
                        skill_list.insert(player, nameList());
                }
                break;
            }
            case Player::Discard: {
                QList<ServerPlayer *> players = room->getOtherPlayers(player);
                foreach (ServerPlayer *p, players) {
                    if (p->hasFlag("bijingLostCard"))
                        skill_list.insert(p, nameList());
                }
                break;
            }
            case Player::Finish: {
                if (TriggerSkill::triggerable(player) && !player->isKongcheng())
                    skill_list.insert(player, nameList());
                break;
            }
            default: break;
            }
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        switch (player->getPhase()) {
        case Player::Start: {
            const Card *card = player->tag["bijingCard"].value<const Card *>();
            if (card)
                room->throwCard(card->getEffectiveId(), player);
            break;
        }
        case Player::Discard: {
            room->askForDiscard(player, objectName(), 2, 2, false, true);
            break;
        }
        case Player::Finish: {

            const Card *card = room->askForCard(player, ".", "@bijing-invoke", data, Card::MethodNone);
            if (card) {
                player->broadcastSkillInvoke(objectName());
                room->notifySkillInvoked(player, objectName());
                LogMessage log;
                log.from = player;
                log.type = "#InvokeSkill";
                log.arg = objectName();
                room->sendLog(log);
                player->tag["bijingCard"] = QVariant::fromValue(card);
            }

            break;
        }
        default: break;
        }
        return false;
    }
};

class Zongkui : public TriggerSkill
{
public:
    Zongkui() : TriggerSkill("zongkui")
    {
        events << TurnStart;
        frequency = Compulsory;
    }

    virtual TriggerList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (room->getTag("TurnFirstRound").toBool()) {
            QList<ServerPlayer *> beimihus = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *beimihu, beimihus) {
                QList<ServerPlayer *> targets, allplayers = room->getOtherPlayers(beimihu);
                int min_hp = allplayers.first()->getHp();
                foreach (ServerPlayer *p, allplayers) {
                    if (p->getHp() < min_hp) {
                        targets.clear();
                        min_hp = p->getHp();
                    }
                    if (p->getHp() == min_hp && p->getMark("@puppet") == 0)
                        targets << p;
                }
                if (!targets.isEmpty())
                    skill_list.insert(beimihu, QStringList("zongkui!"));
            }
        }
        if (TriggerSkill::triggerable(player) && !skill_list.contains(player)) {
            QList<ServerPlayer *> allplayers = room->getOtherPlayers(player);
            foreach (ServerPlayer *p, allplayers) {
                if (p->getMark("@puppet") == 0) {
                    skill_list.insert(player, QStringList("zongkui"));
                }
            }
        }

        return skill_list;
    }

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &, ServerPlayer *beimihu) const
    {
        if (room->getTag("TurnFirstRound").toBool()) {
            QList<ServerPlayer *> targets, allplayers = room->getOtherPlayers(beimihu);
            int min_hp = allplayers.first()->getHp();
            foreach (ServerPlayer *p, allplayers) {
                if (p->getHp() < min_hp) {
                    targets.clear();
                    min_hp = p->getHp();
                }
                if (p->getHp() == min_hp && p->getMark("@puppet") == 0)
                    targets << p;
            }
            if (!targets.isEmpty()) {
                room->sendCompulsoryTriggerLog(beimihu, objectName());
                beimihu->broadcastSkillInvoke(objectName());
                ServerPlayer *target = room->askForPlayerChosen(beimihu, targets, objectName(), "@zongkui2-invoke");
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, beimihu->objectName(), target->objectName());
                target->gainMark("@puppet");
            }
        }
        if (beimihu == player && TriggerSkill::triggerable(beimihu)) {
            QList<ServerPlayer *> targets, allplayers = room->getOtherPlayers(beimihu);
            foreach (ServerPlayer *p, allplayers) {
                if (p->getMark("@puppet") == 0)
                    targets << p;
            }
            if (targets.isEmpty()) return false;

            ServerPlayer *target = room->askForPlayerChosen(beimihu, targets, objectName(), "@zongkui-invoke", true, true);
            if (target) {
                beimihu->broadcastSkillInvoke(objectName());
                target->gainMark("@puppet");
            }
        }

        return false;
    }
};

class Guju : public TriggerSkill
{
public:
    Guju() : TriggerSkill("guju")
    {
        events << Damaged;
        frequency = Compulsory;
    }

    virtual TriggerList triggerable(TriggerEvent , Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (player->isAlive() && player->getMark("@puppet") > 0) {
            QList<ServerPlayer *> beimihus = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *beimihu, beimihus)
                skill_list.insert(beimihu, nameList());
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *, QVariant &, ServerPlayer *beimihu) const
    {
        room->sendCompulsoryTriggerLog(beimihu, objectName());
        beimihu->broadcastSkillInvoke(objectName());

        room->addPlayerMark(beimihu, "#guju");
        beimihu->drawCards(1, objectName());
        return false;
    }
};

class Baijia : public PhaseChangeSkill
{
public:
    Baijia() : PhaseChangeSkill("baijia")
    {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target)
            && target->getMark("baijia") == 0
            && target->getPhase() == Player::Start
            && target->getMark("#guju") > 6;
    }

    virtual bool onPhaseChange(ServerPlayer *beimihu) const
    {
        Room *room = beimihu->getRoom();
        room->sendCompulsoryTriggerLog(beimihu, objectName());

        beimihu->broadcastSkillInvoke(objectName());

        room->setPlayerMark(beimihu, objectName(), 1);
        if (room->changeMaxHpForAwakenSkill(beimihu, 1) && beimihu->getMark(objectName()) == 1) {
            room->recover(beimihu, RecoverStruct(beimihu));
            foreach (ServerPlayer *p, room->getOtherPlayers(beimihu)) {
                if (p->getMark("@puppet") == 0)
                    p->gainMark("@puppet");
            }
            room->handleAcquireDetachSkills(beimihu, "-guju|canshii");
        }
        return false;
    }
};

class Canshii : public TriggerSkill
{
public:
    Canshii() : TriggerSkill("canshii")
    {
        events << TargetChosed << TargetConfirming;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getTypeId() != Card::TypeBasic && !use.card->isNDTrick()) return QStringList();
        if (use.card->isKindOf("Collateral") || use.card->isKindOf("BeatAnother")) return QStringList();
        if (triggerEvent == TargetChosed) {
            QList<ServerPlayer *> available_targets = player->getUseExtraTargets(use, false);
            foreach (ServerPlayer *p, available_targets) {
                if (p->getMark("@puppet") > 0)
                    return nameList();
            }

        } else if (triggerEvent == TargetConfirming) {
            if (use.from && use.from->isAlive() && use.from->getMark("@puppet") > 0) {
                foreach (ServerPlayer *to, use.to)
                    if (to->isAlive() && to != player) return QStringList();

                return nameList();
            }

        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (triggerEvent == TargetChosed) {
            QList<ServerPlayer *> available_targets = player->getUseExtraTargets(use, false);
            foreach (ServerPlayer *p, available_targets) {
                if (p->getMark("@puppet") == 0) available_targets.removeOne(p);
            }

            if (available_targets.isEmpty()) return false;

            QList<ServerPlayer *> choosees = room->askForPlayersChosen(player, available_targets, objectName(), 0, 100,
                                                                       "@canshii-add:::" + use.card->objectName(), true);

            if (choosees.length() > 0) {
                player->broadcastSkillInvoke(objectName());
                room->sortByActionOrder(choosees);
                foreach (ServerPlayer *target, choosees) {
                    use.to.append(target);
                    room->removePlayerMark(target, "@puppet");
                }
            }

            room->sortByActionOrder(use.to);
            data = QVariant::fromValue(use);
            return false;


        } else if (triggerEvent == TargetConfirming) {
            if (player->askForSkillInvoke(objectName(), "prompt:::" + use.card->objectName())) {
                player->broadcastSkillInvoke(objectName());
                room->removePlayerMark(use.from, "@puppet");
                use.to.removeOne(player);
                data = QVariant::fromValue(use);
            }
        }
        return false;
    }
};


class Jilii : public TriggerSkill
{
public:
    Jilii() : TriggerSkill("jilii")
    {
        events << CardUsed << CardResponded;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (TriggerSkill::triggerable(player)) {
            const Card *card = NULL;
            if (triggerEvent == CardUsed)
                card = data.value<CardUseStruct>().card;
            else if (triggerEvent == CardResponded)
                card = data.value<CardResponseStruct>().m_card;
            if (card == NULL) return QStringList();

            if (card->getTypeId() != Card::TypeSkill && player->getCardUsedTimes(".")+player->getCardRespondedTimes(".") == player->getAttackRange()) {
                return nameList();
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer *) const
    {
        if (player->askForSkillInvoke(objectName())) {
            player->broadcastSkillInvoke(objectName());
            player->drawCards(player->getAttackRange(), objectName());
        }
        return false;
    }
};

class Jiedao : public TriggerSkill
{
public:
    Jiedao() : TriggerSkill("jiedao")
    {
        events << DamageCaused << Damage;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (triggerEvent == DamageCaused && TriggerSkill::triggerable(player) && player->isWounded()) {
            if (damage.flags.contains("FirstCauseDamge")) {
                return nameList();
            }
        } else if (triggerEvent == Damage && player->isAlive()) {
            if (damage.to && damage.to->isAlive()) {
                foreach (QString flag, damage.flags) {
                    if (flag.startsWith("JiedaoAdd"))
                        return QStringList("jiedao!");
                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (triggerEvent == DamageCaused) {
            if (player->askForSkillInvoke(objectName())) {
                player->broadcastSkillInvoke(objectName());
                QStringList draw_num;
                for (int i = 1; i <= player->getLostHp(); draw_num << QString::number(i++)) {}
                QString num_str = room->askForChoice(player, "jiedao_count", draw_num.join("+"), data, "@jiedao-count");
                damage.damage+=num_str.toInt();
                damage.flags << "JiedaoAdd"+num_str;
                data.setValue(damage);

            }
        } else if (triggerEvent == Damage) {
            int x = 0;
            foreach (QString flag, damage.flags) {
                if (flag.startsWith("JiedaoAdd")) {
                    x = flag.mid(9).toInt();
                    break;
                }
            }
            if (x > 0)
                room->askForDiscard(player, "jiedao", x, x, false, true, "@jiedao-discard");
        }

        return false;
    }
};

class Biaozhao : public TriggerSkill
{
public:
    Biaozhao() : TriggerSkill("biaozhao")
    {
        events << EventPhaseStart << CardsMoveOneTime;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer * &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Finish && !player->isNude()) {
                return nameList();
            } else if (player->getPhase() == Player::Start && !player->getPile("biao").isEmpty()) {
                return QStringList("biaozhao!");
            }
        } else if (triggerEvent == CardsMoveOneTime && !player->getPile("biao").isEmpty()) {
            const Card *biao = Sanguosha->getCard(player->getPile("biao").first());

            QVariantList move_datas = data.toList();
            foreach(QVariant move_data, move_datas) {
                CardsMoveOneTimeStruct move = move_data.value<CardsMoveOneTimeStruct>();
                if (move.to_place == Player::DiscardPile) {
                    foreach (QString card_str, move.cards) {
                        const Card *card = Card::Parse(card_str);
                        if (card->getSuit() == biao->getSuit() && card->getNumber() == biao->getNumber())
                            return QStringList("biaozhao!");
                    }

                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Finish && !player->isNude()) {
                const Card *card = room->askForCard(player, "..", "@biaozhao-invoke", data, Card::MethodNone);
                if (card) {
                    player->broadcastSkillInvoke(objectName());
                    room->notifySkillInvoked(player, objectName());
                    LogMessage log;
                    log.from = player;
                    log.type = "#InvokeSkill";
                    log.arg = objectName();
                    room->sendLog(log);
                    player->addToPile("biao", card->getSubcards(), true);

                }
            } else if (player->getPhase() == Player::Start && !player->getPile("biao").isEmpty()) {
                room->sendCompulsoryTriggerLog(player, objectName());
                player->broadcastSkillInvoke(objectName());
                player->clearOnePrivatePile("biao");
                ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@biaozhao-choose");
                room->recover(target, RecoverStruct(player));
                int x = 0;
                QList<ServerPlayer *> players = room->getAlivePlayers();
                foreach (ServerPlayer *p, players) {
                    x=qMax(x, p->getHandcardNum());
                }
                x-=target->getHandcardNum();
                if (x > 0)
                    target->drawCards(qMin(x, 5), objectName());
            }
        } else if (triggerEvent == CardsMoveOneTime && !player->getPile("biao").isEmpty()) {

            const Card *biao = Sanguosha->getCard(player->getPile("biao").first());
            ServerPlayer *target = NULL;

            QVariantList move_datas = data.toList();
            foreach(QVariant move_data, move_datas) {
                CardsMoveOneTimeStruct move = move_data.value<CardsMoveOneTimeStruct>();
                if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) != CardMoveReason::S_REASON_DISCARD) continue;
                if (move.to_place == Player::DiscardPile) {
                    foreach (QString card_str, move.cards) {
                        const Card *card = Card::Parse(card_str);
                        if (card->getSuit() == biao->getSuit() && card->getNumber() == biao->getNumber()) {
                            if (move.from && move.from->isAlive() && move.from != player) {
                                target = (ServerPlayer *)move.from;
                                break;
                            }
                        }
                    }
                }
            }

            room->sendCompulsoryTriggerLog(player, objectName());
            player->broadcastSkillInvoke(objectName());

            if (target == NULL)
                player->clearOnePrivatePile("biao");
            else {
                CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, target->objectName());
                room->obtainCard(target, biao, reason, true);
            }
            room->loseHp(player);
        }
        return false;
    }
};

class Yechou : public TriggerSkill
{
public:
    Yechou() : TriggerSkill("yechou")
    {
        events << Death << EventPhaseStart;
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart)
            room->removePlayerTip(player, "#yechou");
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (triggerEvent == Death) {
            if (player == NULL || !player->hasSkill(objectName())) return skill_list;
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player) return skill_list;
            QList<ServerPlayer *> players = room->getAlivePlayers();
            foreach (ServerPlayer *p, players) {
                if (p->getLostHp() > 1) {
                    skill_list.insert(player, nameList());
                    return skill_list;
                }
            }
        } else if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Finish) {
                QList<ServerPlayer *> players = room->getAlivePlayers();
                foreach (ServerPlayer *p, players) {
                    if (p->getMark("#yechou") > 0) {
                        skill_list.insert(p, nameList());
                    }
                }
            }
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &, ServerPlayer *target) const
    {
        if (triggerEvent == Death) {
            QList<ServerPlayer *> players = room->getAlivePlayers(), targets;
            foreach (ServerPlayer *p, players) {
                if (p->getLostHp() > 1)
                    targets << p;
            }
            if (targets.isEmpty()) return false;

            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@yechou-invoke", true, true);
            if (target == NULL) return false;
            player->broadcastSkillInvoke(objectName());

            room->addPlayerTip(target, "#yechou");

        } else if (triggerEvent == EventPhaseStart) {
            LogMessage log;
            log.type = "#SkillForce";
            log.from = target;
            log.arg = objectName();
            room->sendLog(log);
            room->loseHp(target);
        }
        return false;
    }
};




YanjiaoCard::YanjiaoCard()
{
}

void YanjiaoCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *source = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = target->getRoom();

    int x = 4+source->getMark("#yanjiao");
    room->setPlayerMark(source, "#yanjiao", 0);

    QList<int> cards = room->getNCards(x, false);

    CardsMoveStruct move(cards, NULL, Player::PlaceTable,
        CardMoveReason(CardMoveReason::S_REASON_TURNOVER, source->objectName(), "yanjiao", QString()));
    room->moveCardsAtomic(move, true);

    AskForMoveCardsStruct result = room->askForYanjiao(target, cards, true, "yanjiao", "");

    if (result.is_success) {
        if (!result.top.isEmpty()) {
            DummyCard *dummy = new DummyCard(result.top);
            room->obtainCard(source, dummy);
            delete dummy;
        }
        if (!result.bottom.isEmpty()) {
            DummyCard *dummy = new DummyCard(result.bottom);
            room->obtainCard(target, dummy);
            delete dummy;
        }
    }

    QList<int> cards_to_throw = room->getCardIdsOnTable(cards);
    if (!cards_to_throw.isEmpty()) {
        DummyCard *dummy = new DummyCard(cards_to_throw);
        CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, source->objectName(), "yanjiao", QString());
        room->throwCard(dummy, reason, NULL);
        delete dummy;
    }
}

class Yanjiao : public ZeroCardViewAsSkill
{
public:
    Yanjiao() : ZeroCardViewAsSkill("yanjiao")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("YanjiaoCard");
    }

    virtual const Card *viewAs() const
    {
        return new YanjiaoCard;
    }
};

class Xingshen : public MasochismSkill
{
public:
    Xingshen() : MasochismSkill("xingshen")
    {
    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &) const
    {
        Room *room = target->getRoom();
        if (target->askForSkillInvoke(this)){
            target->broadcastSkillInvoke(objectName());
            int x = 2;
            QList<ServerPlayer *> players1 = room->getOtherPlayers(target);
            foreach(ServerPlayer *p, players1) {
                if (target->getHandcardNum() > p->getHandcardNum()) {
                    x = 1;
                    break;
                }
            }
            target->drawCards(x, objectName());
            int y = 2;
            QList<ServerPlayer *> players2 = room->getOtherPlayers(target);
            foreach(ServerPlayer *p, players2) {
                if (target->getHp() > p->getHp()) {
                    y = 1;
                    break;
                }
            }
            int z = qMin(4 - target->getMark("#yanjiao"), y);
            if (z > 0)
                room->addPlayerMark(target, "#yanjiao", z);
        }
    }
};

class ShanjiaSlash : public ZeroCardViewAsSkill
{
public:
    ShanjiaSlash() : ZeroCardViewAsSkill("shanjia_slash")
    {
        response_pattern = "@@shanjia_slash";
    }

    const Card *viewAs() const
    {
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_shanjia");
        return slash;
    }
};

class Shanjia : public PhaseChangeSkill
{
public:
    Shanjia() : PhaseChangeSkill("shanjia")
    {
    }

    virtual QStringList triggerable(TriggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer* &) const
    {
        if (!PhaseChangeSkill::triggerable(player)) return QStringList();
        if (player->getPhase() != Player::Play) return QStringList();
        return nameList();
    }

    virtual bool onPhaseChange(ServerPlayer *caochun) const
    {
        Room *room = caochun->getRoom();
        if (caochun->askForSkillInvoke(this)){
            caochun->broadcastSkillInvoke(objectName());
            caochun->drawCards(3, objectName());

            int n = 3 - qMin(3, caochun->getMark("GlobalLostEquipcard"));

            if (n == 0 || !room->askForDiscard(caochun, objectName(), n, n ,false, true, "@shanjia-discard", ".", "^EquipCard")) {
                if (Slash::IsAvailable(caochun))
                    room->askForUseCard(caochun, "@@shanjia_slash", "@shanjia-slash", QVariant(), Card::MethodUse, false);
            }

        }
        return false;
    }
};

class Chijie : public TriggerSkill
{
public:
    Chijie() : TriggerSkill("chijie")
    {
        events << Damaged << CardFinished << CardEffected;
    }

    virtual TriggerList triggerable(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        TriggerList skill_list;
        if (triggerEvent == CardEffected) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (effect.card->hasFlag("chijieEffect")) {
                skill_list.insert(player, QStringList("chijie!"));
                return skill_list;
            }
        }else if (triggerEvent == Damaged && TriggerSkill::triggerable(player) && !player->hasFlag("chijieUsed")) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card) {
                skill_list.insert(player, nameList());
                return skill_list;
            }
        } else if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
//            if (use.card->tag["GlobalCardDamagedTag"].isNull() && room->isAllOnPlace(use.card, Player::PlaceTable)) {
//                QList<ServerPlayer *> xinpis = room->findPlayersBySkillName(objectName());
//                foreach (ServerPlayer *xinpi, xinpis)
//                    if (xinpi != player && use.to.contains(xinpi) && !xinpi->hasFlag("chijieUsed"))
//                        skill_list.insert(xinpi, nameList());
//                return skill_list;
//            }

            QVariantList damaged_datas = room->getTag("GlobalCardDamagedTag").toList();

            foreach(QVariant damaged_data, damaged_datas) {
                DamageStruct damage = damaged_data.value<DamageStruct>();
                if (damage.card && damage.card == use.card)
                    return skill_list;
            }
            QList<ServerPlayer *> xinpis = room->findPlayersBySkillName(objectName());
            foreach (ServerPlayer *xinpi, xinpis)
                if (xinpi != player && use.to.contains(xinpi) && !xinpi->hasFlag("chijieUsed"))
                    skill_list.insert(xinpi, nameList());
            return skill_list;
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data, ServerPlayer *player) const
    {
        if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && player->askForSkillInvoke(objectName(), QVariant("prompt1:::" + damage.card->objectName()))) {
                player->broadcastSkillInvoke(objectName());
                room->setPlayerFlag(player, "chijieUsed");

                room->setCardFlag(damage.card, "chijieEffect");

            }
        } else if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card && room->isAllOnPlace(use.card, Player::PlaceTable) &&
                    player->askForSkillInvoke(objectName(), QVariant("prompt2:::" + use.card->objectName()))) {
                player->broadcastSkillInvoke(objectName());
                room->setPlayerFlag(player, "chijieUsed");
                player->obtainCard(use.card);
            }
        } else if (triggerEvent == CardEffected) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            effect.nullified = true;
            data = QVariant::fromValue(effect);
        }
        return false;
    }
};


YinjuCard::YinjuCard()
{
}

void YinjuCard::onUse(Room *room, const CardUseStruct &card_use) const
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
    log.to = card_use.to;
    log.card_str = toString();
    room->sendLog(log);

    room->removePlayerMark(source, "@limit_yinju");

    thread->trigger(CardUsed, room, source, data);
    use = data.value<CardUseStruct>();
    thread->trigger(CardFinished, room, source, data);
}

void YinjuCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *source = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = source->getRoom();
    room->addPlayerTip(target, "#yinju");

    QStringList assignee_list = source->tag["YinjuTarget"].toStringList();
    assignee_list << target->objectName();
    source->tag["YinjuTarget"] = QVariant::fromValue(assignee_list);

}

class YinjuViewAsSkill : public ZeroCardViewAsSkill
{
public:
    YinjuViewAsSkill() : ZeroCardViewAsSkill("yinju")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@limit_yinju") > 0;
    }

    virtual const Card *viewAs() const
    {
        return new YinjuCard;
    }
};

class Yinju : public TriggerSkill
{
public:
    Yinju() : TriggerSkill("yinju")
    {
        events << Predamage << TargetSpecified << EventPhaseChanging;
        view_as_skill = new YinjuViewAsSkill;
        frequency = Limited;
        limit_mark = "@limit_yinju";
    }

    virtual void record(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging && data.value<PhaseChangeStruct>().from == Player::Play) {
            QList<ServerPlayer *> players = room->getAlivePlayers();
            foreach (ServerPlayer *p, players) {
                p->tag.remove("YinjuTarget");
                room->removePlayerTip(p, "#yinju");
            }
        }
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer * &ask_who) const
    {
        if (triggerEvent == Predamage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from == NULL || damage.from->isDead()) return QStringList();
            QStringList assignee_list = damage.from->tag["YinjuTarget"].toStringList();
            if (assignee_list.contains(player->objectName()) && player->isAlive() && player->getMark("#yinju") > 0){
                ask_who = damage.from;
                return nameList();
            }
        } else if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card != NULL && use.card->getTypeId() != Card::TypeSkill) {
                QStringList assignee_list = player->tag["YinjuTarget"].toStringList();
                ServerPlayer *to = use.to.at(use.index);
                if (to && to->isAlive() && assignee_list.contains(to->objectName()) && to->getMark("#yinju") > 0)
                    return nameList();
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *from) const
    {
        if (triggerEvent == Predamage) {
            LogMessage log;
            log.type = "#SkillForce";
            log.from = from;
            log.arg = objectName();
            room->sendLog(log);
            DamageStruct damage = data.value<DamageStruct>();
            room->preventDamage(damage);
            room->recover(player, RecoverStruct(from, NULL, damage.damage));
            return true;
        } else if (triggerEvent == TargetSpecified) {
            LogMessage log;
            log.type = "#SkillForce";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);
            player->drawCards(1, objectName());
        }
        return false;
    }
};


































JijieCard::JijieCard()
{
    target_fixed = true;
}

void JijieCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    QList<int> yiji_cards = room->getNCards(1, false, false);
    const Card *card = Sanguosha->getCard(yiji_cards.first());
    room->fillAG(yiji_cards, source);
    ServerPlayer *target = room->askForPlayerChosen(source, room->getAlivePlayers(), "jijie", "@jijie-choose:::"+card->objectName());
    room->clearAG(source);
    CardMoveReason reason(CardMoveReason::S_REASON_PREVIEWGIVE, source->objectName(), target->objectName(), "jijie", QString());
    room->obtainCard(target, card, reason, false);
}

class Jijie : public ZeroCardViewAsSkill
{
public:
    Jijie() : ZeroCardViewAsSkill("jijie")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("JijieCard");
    }

    virtual const Card *viewAs() const
    {
        return new JijieCard;
    }
};

class Jiyuan : public TriggerSkill
{
public:
    Jiyuan() : TriggerSkill("jiyuan")
    {
        events << CardsMoveOneTime << Dying;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data, ServerPlayer* &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == Dying) {
            DyingStruct dying_data = data.value<DyingStruct>();
            if (dying_data.who && dying_data.who->isAlive())
                return nameList();
        } else if (triggerEvent == CardsMoveOneTime) {
            QVariantList move_datas = data.toList();
            foreach(QVariant move_data, move_datas) {
                CardsMoveOneTimeStruct move = move_data.value<CardsMoveOneTimeStruct>();
                if ((move.reason.m_reason == CardMoveReason::S_REASON_GIVE || move.reason.m_reason == CardMoveReason::S_REASON_PREVIEWGIVE) &&
                        move.reason.m_playerId == player->objectName()) {
                    if (move.to && move.to != move.from && move.to != player && move.to_place == Player::PlaceHand && move.to->isAlive()) {
                        return nameList();
                    }
                }
            }
        }
        return QStringList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        if (triggerEvent == Dying) {
            DyingStruct dying_data = data.value<DyingStruct>();
            if (room->askForSkillInvoke(player, objectName(), QVariant::fromValue(dying_data.who))) {
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), dying_data.who->objectName());
                player->broadcastSkillInvoke(objectName());
                dying_data.who->drawCards(1, objectName());
            }
        } else if (triggerEvent == CardsMoveOneTime) {
            QVariantList move_datas = data.toList();
            QList<ServerPlayer *> targets;
            foreach(QVariant move_data, move_datas) {
                CardsMoveOneTimeStruct move = move_data.value<CardsMoveOneTimeStruct>();
                if ((move.reason.m_reason == CardMoveReason::S_REASON_GIVE || move.reason.m_reason == CardMoveReason::S_REASON_PREVIEWGIVE) &&
                        move.reason.m_playerId == player->objectName()) {
                    if (move.to && move.to != move.from && move.to != player && move.to_place == Player::PlaceHand && move.to->isAlive()) {
                        targets << (ServerPlayer *)move.to;
                    }
                }
            }
            room->sortByActionOrder(targets);
            foreach(ServerPlayer *target, targets) {
                if (!TriggerSkill::triggerable(player)) break;
                if (room->askForSkillInvoke(player, objectName(), QVariant::fromValue(target))) {
                    player->broadcastSkillInvoke(objectName());
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());
                    target->drawCards(1, objectName());
                }
            }
        }
        return false;
    }
};

SongshuCard::SongshuCard()
{
}

bool SongshuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && Self->canPindian(to_select);
}

void SongshuCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *source = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = source->getRoom();
    bool success = source->pindian(target, "songshu", NULL);
    if (success)
        room->addPlayerHistory(source, getClassName(), -1);
    else
        target->drawCards(2, "songshu");
}

class Songshu : public ZeroCardViewAsSkill
{
public:
    Songshu() : ZeroCardViewAsSkill("songshu")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("SongshuCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs() const
    {
        return new SongshuCard;
    }
};

class Sibian : public PhaseChangeSkill
{
public:
    Sibian() : PhaseChangeSkill("sibian")
    {
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Draw;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        if (room->askForSkillInvoke(player, objectName())) {
            player->broadcastSkillInvoke(objectName());

            QList<int> ids = room->getNCards(4, false);
            CardsMoveStruct move(ids, NULL, Player::PlaceTable,
                CardMoveReason(CardMoveReason::S_REASON_TURNOVER, player->objectName(), objectName(), QString()));
            room->moveCardsAtomic(move, true);

            QList<int> maxs, mins;
            int max = 0, min = 13;
            foreach (int id, ids) {
                int number = Sanguosha->getCard(id)->getNumber();
                if (number > max) {
                    max = number;
                    maxs.clear();
                }
                if (number == max)
                    maxs << id;
                if (number < min) {
                    min = number;
                    mins.clear();
                }
                if (number == min)
                    mins << id;
            }
            QList<int> to_get = maxs;
            foreach (int id, mins) {
                if (!to_get.contains(id))
                    to_get << id;
            }
            if (!to_get.isEmpty()) {
                DummyCard *dummy = new DummyCard(to_get);
                room->obtainCard(player, dummy);
                delete dummy;
            }

            QList<int> residue = room->getCardIdsOnTable(ids);
            if (!residue.isEmpty()) {
                if (to_get.length() == 2 && max - min < room->alivePlayerCount()) {
                    QList<ServerPlayer *> players = room->getOtherPlayers(player), vics;
                    int min_h = players.first()->getHandcardNum();
                    foreach (ServerPlayer *p, players) {
                        int number = p->getHandcardNum();
                        if (number < min_h) {
                            min_h = number;
                            vics.clear();
                        }
                        if (number == min_h)
                            vics << p;
                    }

                    ServerPlayer *vic = room->askForPlayerChosen(player, vics, objectName(), "@sibian-choose", true);
                    if (vic != NULL) {
                        DummyCard *dummy = new DummyCard(residue);
                        CardMoveReason reason(CardMoveReason::S_REASON_PREVIEWGIVE, player->objectName(), vic->objectName(), objectName(), QString());
                        room->obtainCard(vic, dummy, reason);
                        delete dummy;
                        return true;
                    }
                }
                DummyCard *dummy = new DummyCard(residue);
                CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, player->objectName(), objectName(), QString());
                room->throwCard(dummy, reason, NULL);
                delete dummy;
            }
            return true;
        }
        return false;
    }
};




class LisuDiscard : public ViewAsSkill
{
public:
    LisuDiscard() : ViewAsSkill("lisu_discard")
    {
        response_pattern = "@@lisu_discard";
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.length() < Self->getMark("LisuMaxDiscardNum") && !Self->isJilei(to_select) && !to_select->isEquipped();
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty()) return NULL;
        DummyCard *xt = new DummyCard;
        xt->addSubcards(cards);
        return xt;
    }
};

class Lixun : public TriggerSkill
{
public:
    Lixun() : TriggerSkill("lixun")
    {
        events << DamageInflicted << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual QStringList triggerable(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &, ServerPlayer * &) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() != Player::Play || player->getMark("#pearl") == 0)
                return QStringList();
        }
        return nameList();
    }

    virtual bool effect(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        room->sendCompulsoryTriggerLog(player, objectName());
        player->broadcastSkillInvoke(objectName());
        if (triggerEvent == DamageInflicted) {
            DamageStruct damage = data.value<DamageStruct>();
            room->preventDamage(damage);
            room->addPlayerMark(player, "#pearl",damage.damage);
            return true;
        } else if (triggerEvent == EventPhaseStart) {
            int x = player->getMark("#pearl");
            JudgeStruct judge;
            judge.pattern = ".";
            judge.who = player;
            judge.reason = objectName();
            judge.play_animation = false;

            for (int i = 1; i <= 13; i++)
                judge.patterns << QString(".|.|%1|.").arg(QString::number(i));

            room->judge(judge);

            QStringList factors = judge.pattern.split('|');
            if (factors.length() > 2) {
                QString num_str = factors.at(2);
                bool ok = false;
                int num = num_str.toInt(&ok);
                if (ok && num < x) {
                    room->setPlayerMark(player, "LisuMaxDiscardNum", x);
                    const Card *card = room->askForCard(player, "@@lisu_discard", "@lixun-discard:::" + QString::number(x),
                                                        data, Card::MethodNone);
                    room->setPlayerMark(player, "LisuMaxDiscardNum", 0);

                    int y = 0;
                    if (card != NULL) {
                        CardMoveReason mreason(CardMoveReason::S_REASON_THROW, player->objectName(), QString(), objectName(), QString());
                        room->throwCard(card, mreason, player);
                        y = card->subcardsLength();
                    }

                    int z = x - y;
                    if (z > 0)
                        room->loseHp(player, z);

                }
            }
        }
        return false;
    }
};

class KuizhuiGet : public ViewAsSkill
{
public:
    KuizhuiGet() : ViewAsSkill("kuizhui_get")
    {
        response_pattern = "@@kuizhui_get!";
        expand_pile = "#kuizhui";
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.length() < Self->getMark("KuizhuiNum") && Self->getPile(expand_pile).contains(to_select->getEffectiveId());
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == Self->getMark("KuizhuiNum")) {
            DummyCard *xt = new DummyCard;
            xt->addSubcards(cards);
            return xt;
        }
        return NULL;
    }
};

class Kuizhui : public TriggerSkill
{
public:
    Kuizhui() : TriggerSkill("kuizhui")
    {
        events << EventPhaseEnd;
    }

    virtual QStringList triggerable(TriggerEvent , Room *, ServerPlayer *player, QVariant &, ServerPlayer * &) const
    {
        if (player->getPhase() == Player::Play && TriggerSkill::triggerable(player))
            return nameList();
        return QStringList();
    }

    virtual bool effect(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *) const
    {
        QList<ServerPlayer *> mosts, otherplayers = room->getOtherPlayers(player);
        int most = -1;
        foreach (ServerPlayer *p, otherplayers) {
            int h = p->getHp();
            if (h > most) {
                mosts.clear();
                most = h;
                mosts << p;
            } else if (most == h)
                mosts << p;
        }
        ServerPlayer *target = room->askForPlayerChosen(player, mosts, objectName(), "@kuizhui-invoke", true, true);
        if (target != NULL) {
            player->broadcastSkillInvoke(objectName());
            player->fillHandCards(qMin(5, target->getHandcardNum()), objectName());
            room->fillAG(player->handCards(), target);
            room->setPlayerMark(target, "LisuMaxDiscardNum", player->getHandcardNum());
            const Card *card = room->askForCard(target, "@@lisu_discard", "@kuizhui-discard",
                                                data, Card::MethodNone);
            room->setPlayerMark(target, "LisuMaxDiscardNum", 0);
            room->clearAG(target);

            if (card != NULL) {
                CardMoveReason mreason(CardMoveReason::S_REASON_THROW, target->objectName(), QString(), objectName(), QString());
                room->throwCard(card, mreason, target);
                int y = card->subcardsLength();
                QList<int> handcards = player->handCards();
                QList<int> to_get;
                foreach (int id, handcards) {
                    to_get << id;
                    if (to_get.length() == y) break;
                }

                room->notifyMoveToPile(target, handcards, objectName(), Player::PlaceTable, true, true);
                room->setPlayerMark(target, "KuizhuiNum", y);
                const Card *card2 = room->askForCard(target, "@@kuizhui_get!", "@kuizhui-select:::"+QString::number(y),
                                                    data, Card::MethodNone);
                if (card2 != NULL)
                    to_get = card2->getSubcards();

                room->setPlayerMark(target, "KuizhuiNum", 0);
                room->notifyMoveToPile(target, handcards, objectName(), Player::PlaceTable, false, false);
                CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, target->objectName());
                DummyCard *dummy_card = new DummyCard(to_get);
                dummy_card->deleteLater();
                room->obtainCard(target, dummy_card, reason, false);
                if (to_get.length() > 1) {
                    QStringList choices;
                    if (player->getMark("#pearl") > 0)
                        choices << "remove";

                    QList<ServerPlayer *> victims, allplayers = room->getAlivePlayers();
                    foreach (ServerPlayer *p, allplayers) {
                        if (target->inMyAttackRange(p))
                            victims << p;
                    }
                    if (!victims.isEmpty())
                        choices << "damage";
                    if (choices.isEmpty()) return false;
                    QString choice = room->askForChoice(player, objectName(), choices.join("+"), data, QString(), "remove+damage");
                    if (choice == "remove") {
                        room->removePlayerMark(player, "#pearl");
                    } else if (choice == "damage") {
                        ServerPlayer *vic = room->askForPlayerChosen(player, victims, objectName(), "@kuizhui-choose");
                        room->damage(DamageStruct(objectName(), player, vic));
                    }
                }
            }
        }
        return false;
    }
};



CompeteWorldCardPackage::CompeteWorldCardPackage()
    : Package("CompeteWorldCard", Package::CardPack)
{
    QList<Card *> cards;


    cards << new Demobilizing(Card::Club, 3)
          << new Demobilizing(Card::Diamond, 3)
          << new FloweringTree(Card::Heart, 9)
          << new FloweringTree(Card::Heart, 11)
          << new FloweringTree(Card::Diamond, 9)
          << new BrokenHalberd << new WomenDress;

    foreach(Card *card, cards)
        card->setParent(this);

    skills << new FloweringTreeDiscard << new WomenDressSkill;
}

ADD_PACKAGE(CompeteWorldCard)


CompeteWorldPackage::CompeteWorldPackage()
    :Package("CompeteWorld")
{
    General *zhanggong = new General(this, "zhanggong", "wei", 3);
    zhanggong->addSkill(new Qianxini);
    zhanggong->addSkill(new Zhenxing);

    General *weiwenzhugezhi = new General(this, "weiwenzhugezhi", "wu", 4);
    weiwenzhugezhi->addSkill(new Fuhai);

    General *lvkai = new General(this, "lvkai", "shu", 3);
    lvkai->addSkill(new Tunan);
    lvkai->addSkill(new Bijing);

    General *beimihu = new General(this, "beimihu", "qun", 3, false);
    beimihu->addSkill(new Zongkui);
    beimihu->addSkill(new Guju);
    beimihu->addSkill(new DetachEffectSkill("guju", QString(), "#guju"));
    related_skills.insertMulti("guju", "#guju-clear");
    beimihu->addSkill(new Baijia);
    beimihu->addRelateSkill("canshii");

    General *zhangqiying = new General(this, "zhangqiying", "qun", 3, false);
    zhangqiying->addSkill(new Falu);
    zhangqiying->addSkill(new Zhenyi);
    zhangqiying->addSkill(new Dianhua);

    addMetaObject<QianxiniCard>();
    addMetaObject<TunanCard>();
    addMetaObject<FuhaiCard>();

    skills << new ZhenxingSelect << new TunanUse << new Canshii << new ZhenyiJudge;

}

ADD_PACKAGE(CompeteWorld)

SelfPlayChessPackage::SelfPlayChessPackage()
    :Package("SelfPlayChess")
{
    General *shamoke = new General(this, "shamoke", "shu");
    shamoke->addSkill(new Jilii);

    General *mangyachang = new General(this, "mangyachang", "qun");
    mangyachang->addSkill(new Jiedao);

    General *xugong = new General(this, "xugong", "wu", 3);
    xugong->addSkill(new Biaozhao);
    xugong->addSkill(new Yechou);

    General *zhangchangpu = new General(this, "zhangchangpu", "wei", 3, false);
    zhangchangpu->addSkill(new Yanjiao);
    zhangchangpu->addSkill(new Xingshen);

    General *caochun = new General(this, "caochun", "wei");
    caochun->addSkill(new Shanjia);

    addMetaObject<YanjiaoCard>();

    skills << new ShanjiaSlash;

}

ADD_PACKAGE(SelfPlayChess)

StrategyPackage::StrategyPackage()
    :Package("Strategy")
{
    General *xinpi = new General(this, "xinpi", "wei", 3);
    xinpi->addSkill(new Chijie);
    xinpi->addSkill(new Yinju);

    General *yijibo = new General(this, "yijibo", "shu", 3);
    yijibo->addSkill(new Jijie);
    yijibo->addSkill(new Jiyuan);

    General *zhangwen = new General(this, "zhangwen", "wu", 3);
    zhangwen->addSkill(new Songshu);
    zhangwen->addSkill(new Sibian);

    General *lisu = new General(this, "lisu", "qun", 2);
    lisu->addSkill(new Lixun);
    lisu->addSkill(new Kuizhui);

    addMetaObject<YinjuCard>();
    addMetaObject<JijieCard>();
    addMetaObject<SongshuCard>();

    skills << new LisuDiscard << new KuizhuiGet;
}

ADD_PACKAGE(Strategy)
