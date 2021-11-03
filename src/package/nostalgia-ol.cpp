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
#include "nostalgia-ol.h"

class NosOLBaonue : public TriggerSkill
{
public:
    NosOLBaonue() : TriggerSkill("nosolbaonue$")
    {
        events << Damage;
        view_as_skill = new dummyVS;
    }

    virtual TriggerList triggerable(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        TriggerList skill_list;
        if (player->isAlive() && player->getKingdom() == "qun") {
            foreach (ServerPlayer *dongzhuo, room->getOtherPlayers(player)) {
                if (dongzhuo->hasLordSkill(objectName()))
                    skill_list.insert(dongzhuo, QStringList("nosolbaonue!"));
            }
        }
        return skill_list;
    }

    virtual bool effect(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data, ServerPlayer *dongzhuo) const
    {
        if (room->askForChoice(player, objectName(), "yes+no", data, "@nosolbaonue-to:" + dongzhuo->objectName()) == "yes") {
            LogMessage log;
            log.type = "#InvokeOthersSkill";
            log.from = player;
            log.to << dongzhuo;
            log.arg = objectName();
            room->sendLog(log);
            room->notifySkillInvoked(dongzhuo, objectName());
            dongzhuo->broadcastSkillInvoke(objectName());

            JudgeStruct judge;
            judge.pattern = ".|spade";
            judge.good = true;
            judge.reason = objectName();
            judge.who = dongzhuo;

            room->judge(judge);

            if (judge.isGood())
                room->recover(dongzhuo, RecoverStruct(player));
        }
        return false;
    }
};

NostalOLPackage::NostalOLPackage()
    : Package("nostal_ol")
{
    General *dongzhuo = new General(this, "nos_ol_dongzhuo", "qun", 8, true, true);
    dongzhuo->addSkill("nosjiuchi");
    dongzhuo->addSkill("roulin");
    dongzhuo->addSkill("benghuai");
    dongzhuo->addSkill(new NosOLBaonue);
}

ADD_PACKAGE(NostalOL)
