#include "general.h"
#include "standard.h"
#include "skill.h"
#include "engine.h"
#include "client.h"
#include "serverplayer.h"
#include "room.h"
#include "standard-skillcards.h"
#include "ai.h"
#include "settings.h"
#include "sp.h"
#include "wind.h"
#include "god.h"
#include "maneuvering.h"
#include "json.h"
#include "clientplayer.h"
#include "clientstruct.h"
#include "util.h"
#include "wrapped-card.h"
#include "roomthread.h"
#include "nostalgia-limit.h"

class NJieJianxiong : public MasochismSkill
{
public:
    NJieJianxiong() : MasochismSkill("njiejianxiong")
    {
    }

    void onDamaged(ServerPlayer *caocao, const DamageStruct &damage) const
    {
        Room *room = caocao->getRoom();
        QVariant data = QVariant::fromValue(damage);
        QStringList choices;
        choices << "draw";

        const Card *card = damage.card;
        if (card) {
            QList<int> ids;
            if (card->isVirtualCard())
                ids = card->getSubcards();
            else
                ids << card->getEffectiveId();
            if (ids.length() > 0) {
                bool all_place_table = true;
                foreach (int id, ids) {
                    if (room->getCardPlace(id) != Player::PlaceTable) {
                        all_place_table = false;
                        break;
                    }
                }
                if (all_place_table) choices.append("obtain");
            }
        }
        choices << "cancel";
        QString choice = room->askForChoice(caocao, objectName(), choices.join("+"), data);
        if (choice != "cancel") {
            LogMessage log;
            log.type = "#InvokeSkill";
            log.from = caocao;
            log.arg = objectName();
            room->sendLog(log);

            room->notifySkillInvoked(caocao, objectName());
            room->broadcastSkillInvoke(objectName());
            if (choice == "obtain")
                caocao->obtainCard(card);
            else
                caocao->drawCards(1, objectName());
        }
    }
};

class NJieQingjian : public TriggerSkill
{
public:
    NJieQingjian() : TriggerSkill("njieqingjian")
    {
        events << CardsMoveOneTime;
    }

    QStringList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *&) const
    {
        if (!TriggerSkill::triggerable(player)) return QStringList();
        foreach (QVariant qvar, data.toList()) {
            CardsMoveOneTimeStruct move = qvar.value<CardsMoveOneTimeStruct>();
            if (!room->getTag("FirstRound").toBool() && player->getPhase() != Player::Draw && move.to == player && move.to_place == Player::PlaceHand) {
                foreach (int id, move.card_ids) {
                    if (room->getCardOwner(id) == player && room->getCardPlace(id) == Player::PlaceHand)
                        return nameList();
                }
            }
        }
        return QStringList();
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        QList<int> ids;
        foreach (QVariant qvar, data.toList()) {
            CardsMoveOneTimeStruct move = qvar.value<CardsMoveOneTimeStruct>();
            if (!room->getTag("FirstRound").toBool() && player->getPhase() != Player::Draw && move.to == player && move.to_place == Player::PlaceHand) {
                foreach (int id, move.card_ids) {
                    if (room->getCardOwner(id) == player && room->getCardPlace(id) == Player::PlaceHand)
                        ids << id;
                }
            }
        }
        if (ids.isEmpty())
            return false;
        while (room->askForYiji(player, ids, objectName(), false, false, true, -1, QList<ServerPlayer *>(), CardMoveReason(), "@njieqingjian-distribute", QString(), true)) {
            if (player->isDead()) return false;
        }
        return false;
    }
};

NostalLimitationBrokenPackage::NostalLimitationBrokenPackage()
    : Package("nostal_limitation_broken")
{
    General *njie_caocao = new General(this, "njie_caocao$", "wei", 4, true, true);
    njie_caocao->addSkill(new NJieJianxiong);
    njie_caocao->addSkill("hujia");

    General *njie_xiahoudun = new General(this, "njie_xiahoudun", "wei", 4, true, true);
    njie_xiahoudun->addSkill("ganglie");
    njie_xiahoudun->addSkill(new NJieQingjian);
}

ADD_PACKAGE(NostalLimitationBroken)
