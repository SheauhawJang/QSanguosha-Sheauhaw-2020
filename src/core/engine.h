#ifndef _ENGINE_H
#define _ENGINE_H

#include "room-state.h"
#include "card.h"
#include "general.h"
#include "skill.h"
#include "package.h"
#include "exppattern.h"
#include "protocol.h"
#include "util.h"

#include <QHash>
#include <QStringList>
#include <QMetaObject>
#include <QThread>
#include <QList>
#include <QMutex>

class AI;
class Scenario;
class LuaBasicCard;
class LuaTrickCard;
class LuaWeapon;
class LuaArmor;
class LuaTreasure;

struct lua_State;

class Engine : public QObject
{
    Q_OBJECT

public:
    Engine(bool isManualMode = false);
    ~Engine();

    void addTranslationEntry(const char *key, const char *value);
    QString translate(const QString &to_translate) const;
    lua_State *getLuaState() const;

    int getMiniSceneCounts();

    void addPackage(Package *package);
    void addBanPackage(const QString &package_name);
    QList<const Package *> getPackages() const;
    QStringList getBanPackages() const;
    QMultiMap<QString, QString> getSpConvertPairs() const;
    Card *cloneCard(const Card *card) const;
    Card *cloneCard(const QString &name, Card::Suit suit = Card::SuitToBeDecided, int number = -1, const QStringList &flags = QStringList()) const;
    SkillCard *cloneSkillCard(const QString &name) const;
    QString getVersionNumber() const;
    QString getVersion() const;
    QString getVersionName() const;
    QString getMODName() const;
    QStringList getExtensions() const;
    QStringList getKingdoms() const;
    QColor getKingdomColor(const QString &kingdom) const;
    QMap<QString, QColor> getSkillTypeColorMap() const;
    QStringList getChattingEasyTexts() const;
    QString getSetupString() const;

    QMap<QString, QString> getAvailableModes() const;
    QString getModeName(const QString &mode) const;
    int getPlayerCount(const QString &mode) const;
    QString getRoles(const QString &mode) const;
    QStringList getRoleList(const QString &mode) const;
    int getRoleIndex() const;

    const CardPattern *getPattern(const QString &name) const;
    bool matchExpPattern(const QString &pattern, const Player *player, const Card *card) const;
    Card::HandlingMethod getCardHandlingMethod(const QString &method_name) const;
    QList<const Skill *> getRelatedSkills(const QString &skill_name) const;
    const Skill *getMainSkill(const QString &skill_name) const;

    QStringList getModScenarioNames() const;
    void addScenario(Scenario *scenario);
    const Scenario *getScenario(const QString &name) const;
    void addPackage(const QString &name);

    const General *getGeneral(const QString &name) const;
    int getGeneralCount(bool include_banned = false, const QString &kingdom = QString()) const;
    const Skill *getSkill(const QString &skill_name) const;
    const Skill *getSkill(const EquipCard *card) const;
    QStringList getSkillNames() const;
    const TriggerSkill *getTriggerSkill(const QString &skill_name) const;
    const ViewAsSkill *getViewAsSkill(const QString &skill_name) const;
    QList<const DistanceSkill *> getDistanceSkills() const;
    QList<const MaxCardsSkill *> getMaxCardsSkills() const;
    QList<const TargetModSkill *> getTargetModSkills() const;
    QList<const InvaliditySkill *> getInvaliditySkills() const;
    QList<const TriggerSkill *> getGlobalTriggerSkills() const;
    QList<const AttackRangeSkill *> getAttackRangeSkills() const;
    void addSkills(const QList<const Skill *> &skills);

    int getCardCount() const;
    const Card *getEngineCard(int cardId) const;
    // @todo: consider making this const Card *
    Card *getCard(int cardId);
    WrappedCard *getWrappedCard(int cardId);

    QStringList getLords(bool contain_banned = false) const;
    QStringList getRandomLords() const;
    QStringList getRandomFemaleLords(bool zuoci = true) const;
    QStringList getRandomGenerals(int count = 0, const QSet<QString> &ban_set = QSet<QString>(), const QString &kingdom = QString()) const;
    QStringList getRandomFemaleGenerals(int count = 0, const QSet<QString> &ban_set = QSet<QString>(), const QString &kingdom = QString(), bool zuoci = true) const;
    QList<int> getRandomCards() const;
    QString getRandomGeneralName() const;
    QStringList getLimitedGeneralNames(const QString &kingdom = QString()) const;
    inline QList<const General *> getAllGenerals() const
    {
        return findChildren<const General *>();
    }

    void playSystemAudioEffect(const QString &name, bool superpose = true) const;
    void playAudioEffect(const QString &filename, bool superpose = true) const;
    void playSkillAudioEffect(const QString &skill_name, int index, bool superpose = true) const;
    QStringList findSkillAudioFileNames(const QString &skillName, const QString &generalName ,int skinId = 0) const;

    const ProhibitSkill *isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &others = QList<const Player *>()) const;
    const FixCardSkill *isCardFixed(const Player *from, const Player *to, const QString &flags, Card::HandlingMethod method) const;
    const CardLimitedSkill *isCardLimited(const Player *player, const Card *card, Card::HandlingMethod method) const;
    const HideCardSkill *isCardHided(const Player *player, const Card *card) const;
    const ViewHasSkill *ViewHas(const Player *player, const QString &skill_name, const QString &flag) const;
    int correctDistance(const Player *from, const Player *to) const;
    int correctMaxCards(const Player *target, bool fixed = false) const;
    int correctCardTarget(const TargetModSkill::ModType type, const Player *from, const Card *card, const Player *to = NULL) const;
    bool correctSkillValidity(const Player *player, const Skill *skill) const;
    int correctAttackRange(const Player *target, bool include_weapon = true, bool fixed = false) const;
    const StatusAbilitySkill *turnOverSkill(const Player *target) const;
    const StatusAbilitySkill *setChainSkill(const Player *target, bool setting) const;
    const StatusAbilitySkill *pindianProhibitSkill(const Player *to, const Player *from) const;
    QList<const StatusAbilitySkill *> turnDelayedTrickSkills(const Player *player, const Card *card) const;
    bool correctCanTurnOver(const Player *target) const;
    bool correctCanSetChain(const Player *target, bool setting) const;
    bool correctCanPindian(const Player *to, const Player *from) const;
    bool correctTurnDelayedTrickResult(const Player *player, const Card *card) const;

    void registerRoom(QObject *room);
    void unregisterRoom();
    QObject *currentRoomObject();
    Room *currentRoom();
    RoomState *currentRoomState();

    QString getCurrentCardUsePattern();
    CardUseStruct::CardUseReason getCurrentCardUseReason();

    bool isGeneralHidden(const QString &general_name) const;
    QStringList getConvertGenerals(const QString &general_name) const;
    QString getMainGeneral(const QString &name) const;
    QStringList getMainGenerals(const QStringList &general_names) const;
    void setExtraGeneralsBan();
    QStringList getExtraGeneralsBan() const;

private:
    void _loadMiniScenarios();
    void _loadModScenarios();

    QMutex m_mutex;
    QHash<QString, QString> translations;
    QHash<QString, const General *> generals;
    QHash<QString, const QMetaObject *> metaobjects;
    QHash<QString, QString> className2objectName;
    QHash<QString, QString> objectName2className;
    QHash<QString, const Skill *> skills;
    QHash<QThread *, QObject *> m_rooms;
    QMap<QString, QString> modes;
    QMultiMap<QString, QString> related_skills;
    mutable QMap<QString, const CardPattern *> patterns;

    // special skills
    QList<const ProhibitSkill *> prohibit_skills;
    QList<const FixCardSkill *> fixcard_skills;
    QList<const CardLimitedSkill *> limitedcard_skills;
    QList<const HideCardSkill *> hidecard_skills;
    QList<const ViewHasSkill *> viewhas_skills;
    QList<const DistanceSkill *> distance_skills;
    QList<const MaxCardsSkill *> maxcards_skills;
    QList<const TargetModSkill *> targetmod_skills;
    QList<const InvaliditySkill *> invalidity_skills;
    QList<const TriggerSkill *> global_trigger_skills;
    QList<const AttackRangeSkill *> attack_range_skills;
    QList<const StatusAbilitySkill *> status_ability_skills;

    QList<Card *> cards;
    QStringList lord_list;
    QSet<QString> ban_package;
    QHash<QString, Scenario *> m_scenarios;
    QHash<QString, Scenario *> m_miniScenes;
    Scenario *m_customScene;

    lua_State *lua;

    QHash<QString, QString> luaBasicCard_className2objectName;
    QHash<QString, const LuaBasicCard *> luaBasicCards;
    QHash<QString, QString> luaTrickCard_className2objectName;
    QHash<QString, const LuaTrickCard *> luaTrickCards;
    QHash<QString, QString> luaWeapon_className2objectName;
    QHash<QString, const LuaWeapon*> luaWeapons;
    QHash<QString, QString> luaArmor_className2objectName;
    QHash<QString, const LuaArmor *> luaArmors;
    QHash<QString, QString> luaTreasure_className2objectName;
    QHash<QString, const LuaTreasure *> luaTreasures;

    QMultiMap<QString, QString> sp_convert_pairs;
    QStringList extra_hidden_generals;
    QStringList removed_hidden_generals;
    QStringList extra_default_lords;
    QStringList removed_default_lords;

};

static inline QVariant GetConfigFromLuaState(lua_State *L, const char *key)
{
    return GetValueFromLuaState(L, "config", key);
}

extern Engine *Sanguosha;

#endif

