#include "engine.h"
#include "card.h"
#include "client.h"
#include "ai.h"
#include "settings.h"
#include "scenario.h"
#include "lua.hpp"
#include "banpair.h"
#include "audio.h"
#include "protocol.h"
#include "structs.h"
#include "lua-wrapper.h"
#include "room-state.h"

#include "guandu-scenario.h"
#include "couple-scenario.h"
#include "boss-mode-scenario.h"
#include "zombie-scenario.h"
#include "fancheng-scenario.h"

#include <QFile>
#include <QTextCodec>
#include <QTextStream>
#include <QStringList>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QApplication>
#include <scenario.h>
#include <miniscenarios.h>

Engine *Sanguosha = NULL;

int Engine::getMiniSceneCounts()
{
    return m_miniScenes.size();
}

void Engine::_loadMiniScenarios()
{
    static bool loaded = false;
    if (loaded) return;
    int i = 1;
    while (true) {
        if (!QFile::exists(QString("etc/customScenes/%1.txt").arg(QString::number(i))))
            break;

        QString sceneKey = QString(MiniScene::S_KEY_MINISCENE).arg(QString::number(i));
        m_miniScenes[sceneKey] = new LoadedScenario(QString::number(i));
        i++;
    }
    loaded = true;
}

void Engine::_loadModScenarios()
{
    addScenario(new GuanduScenario());
    addScenario(new CoupleScenario());
    addScenario(new FanchengScenario());
    addScenario(new ZombieScenario());
    addScenario(new ImpasseScenario());
}

void Engine::addPackage(const QString &name)
{
    Package *pack = PackageAdder::packages()[name];
    if (pack)
        addPackage(pack);
    else
        qWarning("Package %s cannot be loaded!", qPrintable(name));
}

struct ManualSkill
{
    ManualSkill(const Skill *skill)
        : skill(skill),
          baseName(skill->objectName().split("_").last())
    {
        static const QString prefixes[] = { "boss", "gd", "jg", "jsp", "kof", "neo", "nos", "ol", "sp", "tw", "vs", "yt", "diy" };

        for (int i = 0; i < (sizeof(prefixes) / sizeof(QString)); ++i) {
            QString prefix = prefixes[i];
            if (baseName.startsWith(prefix))
                baseName.remove(0, prefix.length());
        }

        QTextCodec *codec = QTextCodec::codecForName("GBK");
        translatedBytes = codec->fromUnicode(Sanguosha->translate(skill->objectName()));

        printf("%s:%d", skill->objectName().toLocal8Bit().constData(), translatedBytes.length());
    }

    const Skill *skill;
    QString baseName;
    QByteArray translatedBytes;
    QList<const General *> relatedGenerals;
};

static bool nameLessThan(const ManualSkill *skill1, const ManualSkill *skill2)
{
    return skill1->baseName < skill2->baseName;
}

static bool translatedNameLessThan(const ManualSkill *skill1, const ManualSkill *skill2)
{
    return skill1->translatedBytes < skill2->translatedBytes;
}

class ManualSkillList
{
public:
    ManualSkillList()
    {

    }

    ~ManualSkillList()
    {
        foreach (ManualSkill *manualSkill, m_skills)
            delete manualSkill;
    }

    void insert(const Skill *skill, const General *owner)
    {
        bool exist = false;
        foreach (ManualSkill *manualSkill, m_skills) {
            if (skill == manualSkill->skill) {
                exist = true;
                manualSkill->relatedGenerals << owner;
            }
        }

        if (!exist) {
            ManualSkill *manualSkill = new ManualSkill(skill);
            manualSkill->relatedGenerals << owner;
            m_skills << manualSkill;
        }
    }

    void insert(QList<const Skill *>skills, const General *owner)
    {
        foreach (const Skill *skill, skills)
            insert(skill, owner);
    }

    void insert(ManualSkill *skill)
    {
        m_skills << skill;
    }

    void clear()
    {
        m_skills.clear();
    }

    bool isEmpty() const
    {
        return m_skills.isEmpty();
    }

    void sortByName()
    {
        std::sort(m_skills.begin(), m_skills.end(), nameLessThan);
    }

    void sortByTranslatedName(QList<ManualSkill *>::iterator begin, QList<ManualSkill *>::iterator end)
    {
        std::sort(begin, end, translatedNameLessThan);
    }

    QList<ManualSkill *>::iterator begin()
    {
        return m_skills.begin();
    }

    QList<ManualSkill *>::iterator end()
    {
        return m_skills.end();
    }

    QString join(const QString &sep)
    {
        QStringList baseNames;
        foreach (ManualSkill *skill, m_skills)
            baseNames << Sanguosha->translate(skill->skill->objectName());

        return baseNames.join(sep);
    }

private:
    QList<ManualSkill *> m_skills;
};

Engine::Engine(bool isManualMode)
{
    Sanguosha = this;

    lua = CreateLuaState();
    if (!DoLuaScript(lua, "lua/config.lua")) exit(1);

    QStringList stringlist_sp_convert = GetConfigFromLuaState(lua, "convert_pairs").toStringList();
    foreach (QString cv_pair, stringlist_sp_convert) {
        QStringList pairs = cv_pair.split("->");
        QStringList cv_to = pairs.at(1).split("|");
        foreach (QString to, cv_to)
            sp_convert_pairs.insertMulti(pairs.at(0), to);
    }

    extra_hidden_generals = GetConfigFromLuaState(lua, "extra_hidden_generals").toStringList();
    removed_hidden_generals = GetConfigFromLuaState(lua, "removed_hidden_generals").toStringList();
    extra_default_lords = GetConfigFromLuaState(lua, "extra_default_lords").toStringList();
    removed_default_lords = GetConfigFromLuaState(lua, "removed_default_lords").toStringList();


    QStringList package_names = GetConfigFromLuaState(lua, "package_names").toStringList();
    foreach (QString name, package_names)
        addPackage(name);

    _loadMiniScenarios();
    _loadModScenarios();
    m_customScene = new CustomScenario;

    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(deleteLater()));

    if (!DoLuaScript(lua, "lua/sanguosha.lua")) exit(1);

    if (isManualMode) {
        ManualSkillList allSkills;
        foreach (const General *general, getAllGenerals()) {
            allSkills.insert(general->getVisibleSkillList(), general);

            foreach (const QString &skillName, general->getRelatedSkillNames()) {
                const Skill *skill = getSkill(skillName);
                if (skill != NULL && skill->isVisible())
                    allSkills.insert(skill, general);
            }
        }

        allSkills.sortByName();

        QList<ManualSkill *>::iterator j = allSkills.begin();
        QList<ManualSkill *>::iterator i = j;
        for (char c = 'a'; c <= 'z'; ++c) {
            while (j != allSkills.end()) {
                if (!(*j)->baseName.startsWith(c))
                    break;
                else
                    ++j;
            }
            if (j - i > 1)
                allSkills.sortByTranslatedName(i, j);

            i = j;
        }

        QDir dir("manual");
        if (!dir.exists())
            QDir::current().mkdir("manual");


        Config.setValue("AutoSkillTypeColorReplacement", false);
        Config.setValue("AutoSuitReplacement", false);

        QList<ManualSkill *>::iterator iter = allSkills.begin();
        for (char c = 'a'; c <= 'z'; ++c) {
            QChar upper = QChar(c).toUpper();
            QFile file(QString("manual/Chapter%1.lua").arg(upper));
            if (file.open(QFile::WriteOnly | QFile::Truncate)) {
                QTextStream stream(&file);
                stream.setCodec(QTextCodec::codecForName("UTF-8"));

                ManualSkillList list;
                while (iter != allSkills.end()) {
                    if ((*iter)->baseName.startsWith(c)) {
                        list.insert(*iter);
                        ++iter;
                    } else {
                        break;
                    }
                }

                QString info;
                if (list.isEmpty())
                    info = translate("Manual_Empty");
                else
                    info = translate("Manual_Index") + list.join(" ");

                stream << translate("Manual_Head").arg(upper).arg(info)
                       .arg(getVersion())
                       << endl;

                for (QList<ManualSkill *>::iterator it = list.begin();
                        it < list.end(); ++it) {
                    ManualSkill *skill = *it;
                    QStringList generals;

                    foreach(const General *general, skill->relatedGenerals) {
                        generals << QString("%1-%2")
                                 .arg(translate(general->getPackage()))
                                 .arg(general->getBriefName());
                    }
                    stream << translate("Manual_Skill")
                           .arg(translate(skill->skill->objectName()))
                           .arg(generals.join(" "))
                           .arg(skill->skill->getDescription())
                           << endl << endl;
                }

                list.clear();
                file.close();
            }
        }
        return;
    }

    // available game modes
    modes["02p"] = tr("2 players");
    //modes["02pbb"] = tr("2 players (using blance beam)");
    modes["02_1v1"] = tr("2 players (KOF style)");
    modes["03p"] = tr("3 players");
    modes["03_1v2"] = tr("3 players (Dou Di Zhu)");
    modes["04p"] = tr("4 players");
    modes["04_1v3"] = tr("4 players (Hulao Pass)");
    modes["04_boss"] = tr("4 players(Boss)");
    modes["04_year"] = tr("4 players (Year Boss)");
    modes["05p"] = tr("5 players");
    modes["05_zhfd"] = tr("5 players (Attack Dongzhuo)");
    modes["06p"] = tr("6 players");
    modes["06pd"] = tr("6 players (2 renegades)");
    modes["06_3v3"] = tr("6 players (3v3)");
    modes["06_XMode"] = tr("6 players (XMode)");
    //modes["06_swzs"] = tr("6 players (Gods Return)");
    modes["07p"] = tr("7 players");
    modes["08p"] = tr("8 players");
    modes["08pd"] = tr("8 players (2 renegades)");
    modes["08pz"] = tr("8 players (0 renegade)");
    modes["08_defense"] = tr("8 players (JianGe Defense)");
    modes["08_zdyj"] = tr("8 players (Best Loyalist)");
    modes["08_hongyan"] = tr("8 players (Hongyan Races)");
    modes["08_dragonboat"] = tr("8 players (Dragon Bost Races)");
    modes["09p"] = tr("9 players");
    modes["10pd"] = tr("10 players");
    modes["10p"] = tr("10 players (1 renegade)");
    modes["10pz"] = tr("10 players (0 renegade)");
    modes["11p"] = tr("11 players");
    modes["12pd"] = tr("12 players");
    modes["12p"] = tr("12 players (1 renegade)");
    modes["12pz"] = tr("12 players (0 renegade)");

    foreach (const Skill *skill, skills.values()) {
        Skill *mutable_skill = const_cast<Skill *>(skill);
        mutable_skill->initMediaSource();
    }
    if (!DoLuaScript(lua, "lua/func.lua")) exit(1);
}

lua_State *Engine::getLuaState() const
{
    return lua;
}

void Engine::addTranslationEntry(const char *key, const char *value)
{
    translations.insert(key, QString::fromUtf8(value));
}

Engine::~Engine()
{
    lua_close(lua);
    delete m_customScene;
#ifdef AUDIO_SUPPORT
    Audio::quit();
#endif
}

QStringList Engine::getModScenarioNames() const
{
    return m_scenarios.keys();
}

void Engine::addScenario(Scenario *scenario)
{
    QString key = scenario->objectName();
    m_scenarios[key] = scenario;
    addPackage(scenario);
}

const Scenario *Engine::getScenario(const QString &name) const
{
    if (m_scenarios.contains(name))
        return m_scenarios[name];
    else if (m_miniScenes.contains(name))
        return m_miniScenes[name];
    else if (name == "custom_scenario")
        return m_customScene;
    else return NULL;
}

void Engine::addSkills(const QList<const Skill *> &all_skills)
{
    foreach (const Skill *skill, all_skills) {
        if (!skill) {
            QMessageBox::warning(NULL, "", tr("The engine tries to add an invalid skill"));
            continue;
        }
        if (skills.contains(skill->objectName()))
            QMessageBox::warning(NULL, "", tr("Duplicated skill : %1").arg(skill->objectName()));

        skills.insert(skill->objectName(), skill);

        if (skill->inherits("ProhibitSkill"))
            prohibit_skills << qobject_cast<const ProhibitSkill *>(skill);
        else if (skill->inherits("FixCardSkill"))
            fixcard_skills << qobject_cast<const FixCardSkill *>(skill);
        else if (skill->inherits("CardLimitedSkill"))
            limitedcard_skills << qobject_cast<const CardLimitedSkill *>(skill);
        else if (skill->inherits("HideCardSkill"))
            hidecard_skills << qobject_cast<const HideCardSkill *>(skill);
        else if (skill->inherits("ViewHasSkill"))
            viewhas_skills << qobject_cast<const ViewHasSkill *>(skill);
        else if (skill->inherits("DistanceSkill"))
            distance_skills << qobject_cast<const DistanceSkill *>(skill);
        else if (skill->inherits("MaxCardsSkill"))
            maxcards_skills << qobject_cast<const MaxCardsSkill *>(skill);
        else if (skill->inherits("TargetModSkill"))
            targetmod_skills << qobject_cast<const TargetModSkill *>(skill);
        else if (skill->inherits("InvaliditySkill"))
            invalidity_skills << qobject_cast<const InvaliditySkill *>(skill);
        else if (skill->inherits("AttackRangeSkill"))
            attack_range_skills << qobject_cast<const AttackRangeSkill *>(skill);
        else if (skill->inherits("StatusAbilitySkill"))
            status_ability_skills << qobject_cast<const StatusAbilitySkill *>(skill);
        else if (skill->inherits("TriggerSkill")) {
            const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(skill);
            if (trigger_skill && trigger_skill->isGlobal())
                global_trigger_skills << trigger_skill;
        }
    }
}

QList<const DistanceSkill *> Engine::getDistanceSkills() const
{
    return distance_skills;
}

QList<const MaxCardsSkill *> Engine::getMaxCardsSkills() const
{
    return maxcards_skills;
}

QList<const TargetModSkill *> Engine::getTargetModSkills() const
{
    return targetmod_skills;
}

QList<const InvaliditySkill *> Engine::getInvaliditySkills() const
{
    return invalidity_skills;
}

QList<const TriggerSkill *> Engine::getGlobalTriggerSkills() const
{
    return global_trigger_skills;
}

QList<const AttackRangeSkill *> Engine::getAttackRangeSkills() const
{
    return attack_range_skills;
}

QList<const StatusAbilitySkill *> Engine::getStatusAbilitySkills() const
{
    return status_ability_skills;
}

void Engine::addPackage(Package *package)
{
    if (findChild<const Package *>(package->objectName()))
        return;

    package->setParent(this);
    sp_convert_pairs.unite(package->getConvertPairs());
    patterns.unite(package->getPatterns());
    related_skills.unite(package->getRelatedSkills());

    QList<Card *> all_cards = package->findChildren<Card *>();
    foreach (Card *card, all_cards) {
        card->setId(cards.length());
        cards << card;

        if (card->isKindOf("LuaBasicCard")) {
            const LuaBasicCard *lcard = qobject_cast<const LuaBasicCard *>(card);
            Q_ASSERT(lcard != NULL);
            luaBasicCard_className2objectName.insert(lcard->getClassName(), lcard->objectName());
            if (!luaBasicCards.keys().contains(lcard->getClassName()))
                luaBasicCards.insert(lcard->getClassName(), lcard->clone());
        } else if (card->isKindOf("LuaTrickCard")) {
            const LuaTrickCard *lcard = qobject_cast<const LuaTrickCard *>(card);
            Q_ASSERT(lcard != NULL);
            luaTrickCard_className2objectName.insert(lcard->getClassName(), lcard->objectName());
            if (!luaTrickCards.keys().contains(lcard->getClassName()))
                luaTrickCards.insert(lcard->getClassName(), lcard->clone());
        } else if (card->isKindOf("LuaWeapon")) {
            const LuaWeapon *lcard = qobject_cast<const LuaWeapon *>(card);
            Q_ASSERT(lcard != NULL);
            luaWeapon_className2objectName.insert(lcard->getClassName(), lcard->objectName());
            if (!luaWeapons.keys().contains(lcard->getClassName()))
                luaWeapons.insert(lcard->getClassName(), lcard->clone());
        } else if (card->isKindOf("LuaArmor")) {
            const LuaArmor *lcard = qobject_cast<const LuaArmor *>(card);
            Q_ASSERT(lcard != NULL);
            luaArmor_className2objectName.insert(lcard->getClassName(), lcard->objectName());
            if (!luaArmors.keys().contains(lcard->getClassName()))
                luaArmors.insert(lcard->getClassName(), lcard->clone());
        } else if (card->isKindOf("LuaTreasure")) {
            const LuaTreasure *lcard = qobject_cast<const LuaTreasure *>(card);
            Q_ASSERT(lcard != NULL);
            luaTreasure_className2objectName.insert(lcard->getClassName(), lcard->objectName());
            if (!luaTreasures.keys().contains(lcard->getClassName()))
                luaTreasures.insert(lcard->getClassName(), lcard->clone());
        } else {
            QString class_name = card->metaObject()->className();
            metaobjects.insert(class_name, card->metaObject());
            className2objectName.insert(class_name, card->objectName());
            objectName2className.insert(card->objectName(), class_name);
        }
    }

    addSkills(package->getSkills());

    QList<General *> all_generals = package->findChildren<General *>();
    foreach (General *general, all_generals) {
        addSkills(general->findChildren<const Skill *>());
        foreach (QString skill_name, general->getExtraSkillSet()) {
            if (skill_name.startsWith("#")) continue;
            foreach(const Skill *related, getRelatedSkills(skill_name))
                general->addSkill(related->objectName());
        }
        generals.insert(general->objectName(), general);
        if (isGeneralHidden(general->objectName())) continue;
        if ((general->isLord() && !removed_default_lords.contains(general->objectName()))
                || extra_default_lords.contains(general->objectName()))
            lord_list << general->objectName();
    }

    QList<const QMetaObject *> metas = package->getMetaObjects();
    foreach(const QMetaObject *meta, metas)
        metaobjects.insert(meta->className(), meta);
}

void Engine::addBanPackage(const QString &package_name)
{
    ban_package.insert(package_name);
}

QStringList Engine::getBanPackages() const
{
    if (qApp->arguments().contains("-server"))
        return Config.BanPackages;
    else
        return ban_package.toList();
}

QMultiMap<QString, QString> Engine::getSpConvertPairs() const
{
    return sp_convert_pairs;
}

QList<const Package *> Engine::getPackages() const
{
    return findChildren<const Package *>();
}

QString Engine::translate(const QString &to_translate) const
{
    QStringList list = to_translate.split("\\");
    QString res;
    foreach(QString str, list)
        res.append(translations.value(str, str));
    return res;
}

int Engine::getRoleIndex() const
{
    if (ServerInfo.GameMode == "06_3v3" || ServerInfo.GameMode == "06_XMode") {
        return 4;
    } else if (ServerInfo.EnableHegemony) {
        return 5;
    } else
        return 1;
}

const CardPattern *Engine::getPattern(const QString &name) const
{
    const CardPattern *ptn = patterns.value(name, NULL);
    if (ptn) return ptn;

    ExpPattern *expptn = new ExpPattern(name);
    patterns.insert(name, expptn);
    return expptn;
}

bool Engine::matchExpPattern(const QString &pattern, const Player *player, const Card *card) const
{
    ExpPattern p(pattern);
    return p.match(player, card);
}

Card::HandlingMethod Engine::getCardHandlingMethod(const QString &method_name) const
{
    if (method_name == "use")
        return Card::MethodUse;
    else if (method_name == "response")
        return Card::MethodResponse;
    else if (method_name == "discard")
        return Card::MethodDiscard;
    else if (method_name == "recast")
        return Card::MethodRecast;
    else if (method_name == "pindian")
        return Card::MethodPindian;
    else {
        Q_ASSERT(false);
        return Card::MethodNone;
    }
}

QList<const Skill *> Engine::getRelatedSkills(const QString &skill_name) const
{
    QList<const Skill *> skills;
    foreach(QString name, related_skills.values(skill_name))
        skills << getSkill(name);

    return skills;
}

const Skill *Engine::getMainSkill(const QString &skill_name) const
{
    const Skill *skill = getSkill(skill_name);
    if (!skill || skill->isVisible() || related_skills.keys().contains(skill_name)) return skill;
    foreach (QString key, related_skills.keys()) {
        foreach(QString name, related_skills.values(key))
            if (name == skill_name) return getSkill(key);
    }
    return skill;
}

const General *Engine::getGeneral(const QString &name) const
{
    return generals.value(name, NULL);
}

int Engine::getGeneralCount(bool include_banned, const QString &kingdom) const
{
    int total = 0;
    QHashIterator<QString, const General *> itor(generals);
    while (itor.hasNext()) {
        bool isBanned = false;
        itor.next();
        const General *general = itor.value();
        if ((!kingdom.isEmpty() && general->getKingdom() != kingdom)
                || isGeneralHidden(general->objectName()))
            continue;

        if (getBanPackages().contains(general->getPackage()))
            isBanned = true;
        else {
            QStringList ban_list = getExtraGeneralsBan();
            if (ban_list.contains(general->objectName()))
                isBanned = true;
            else if (ServerInfo.Enable2ndGeneral && BanPair::isBanned(general->objectName()))
                isBanned = true;
        }
        if (include_banned || !isBanned)
            total++;
    }

    // special case for neo standard package
    if (getBanPackages().contains("standard") && !getBanPackages().contains("nostal_standard")) {
        if (kingdom.isEmpty() || kingdom == "wei")
            ++total; // zhenji
        if (kingdom.isEmpty() || kingdom == "shu")
            ++total; // zhugeliang
        if (kingdom.isEmpty() || kingdom == "wu")
            total += 2; // suanquan && sunshangxiang
    }

    return total;
}

void Engine::registerRoom(QObject *room)
{
    m_mutex.lock();
    m_rooms[QThread::currentThread()] = room;
    m_mutex.unlock();
}

void Engine::unregisterRoom()
{
    m_mutex.lock();
    m_rooms.remove(QThread::currentThread());
    m_mutex.unlock();
}

QObject *Engine::currentRoomObject()
{
    QObject *room = NULL;
    m_mutex.lock();
    room = m_rooms[QThread::currentThread()];
    Q_ASSERT(room);
    m_mutex.unlock();
    return room;
}

Room *Engine::currentRoom()
{
    QObject *roomObject = currentRoomObject();
    Room *room = qobject_cast<Room *>(roomObject);
    Q_ASSERT(room != NULL);
    return room;
}

RoomState *Engine::currentRoomState()
{
    QObject *roomObject = currentRoomObject();
    Room *room = qobject_cast<Room *>(roomObject);
    if (room != NULL) {
        return room->getRoomState();
    } else {
        Client *client = qobject_cast<Client *>(roomObject);
        Q_ASSERT(client != NULL);
        return client->getRoomState();
    }
}

QString Engine::getCurrentCardUsePattern()
{
    return currentRoomState()->getCurrentCardUsePattern();
}

CardUseStruct::CardUseReason Engine::getCurrentCardUseReason()
{
    return currentRoomState()->getCurrentCardUseReason();
}

bool Engine::isGeneralHidden(const QString &general_name) const
{
    const General *general = getGeneral(general_name);
    if (!general) return true;
    return (general->isHidden() && !removed_hidden_generals.contains(general_name))
           || extra_hidden_generals.contains(general_name);
}

QStringList Engine::getConvertGenerals(const QString &name) const
{
    if (!getGeneral(name)) return QStringList();
    QStringList generals;
    QMultiMap<QString, QString> convert_pairs = getSpConvertPairs();
    foreach (const QString &_name, convert_pairs.values(name)) {
        if (!getGeneral(_name)) continue;
        generals << _name;
    }

    return generals;
}

QString Engine::getMainGeneral(const QString &name) const
{
    if (!getGeneral(name)) return QString();
    QMultiMap<QString, QString> convert_pairs = getSpConvertPairs();
    if (convert_pairs.values().contains(name))
        return convert_pairs.key(name, NULL);
    return name;
}

QStringList Engine::getMainGenerals(const QStringList &general_names) const
{
    QStringList main_generals;
    for (int i = 0; i < general_names.length(); i++) {
        QString general_name = general_names.at(i);
        QString main_name = getMainGeneral(general_name);
        if (!main_generals.contains(main_name))
            main_generals << main_name;
    }
    return main_generals;
}

void Engine::setExtraGeneralsBan()
{
    QStringList ban_list;
    if (Config.EnableBasara)
        ban_list.append(Config.value("Banlist/Basara", "").toStringList());
    if (Config.EnableHegemony)
        ban_list.append(Config.value("Banlist/Hegemony", "").toStringList());

    if (ServerInfo.GameMode == "04_boss")
        ban_list.append(Config.value("Banlist/BossMode", "").toStringList());
    else if (ServerInfo.GameMode == "04_year")
        ban_list.append(Config.value("Banlist/YearBoss", "").toStringList());
    else if (ServerInfo.GameMode == "05_zhfd")
        ban_list.append(Config.value("Banlist/AttackDong", "").toStringList());
    else if (ServerInfo.GameMode == "06_swzs")
        ban_list.append(Config.value("Banlist/GodsReturn", "").toStringList());
    else if (ServerInfo.GameMode == "08_zdyj")
        ban_list.append(Config.value("Banlist/BestLoyalist", "").toStringList());
    else if (ServerInfo.GameMode == "08_dragonboat")
        ban_list.append(Config.value("Banlist/DragonBoat", "").toStringList());
    else if (ServerInfo.GameMode == "02_1v1")
        ban_list.append(Config.value("Banlist/1v1", "").toStringList());
    else if (Config.GameMode == "zombie_mode")
        ban_list.append(Config.value("Banlist/Zombie").toStringList());
    else if (isNormalGameMode(ServerInfo.GameMode)
             || ServerInfo.GameMode.contains("_mini_")
             || ServerInfo.GameMode == "custom_scenario")
        ban_list.append(Config.value("Banlist/Roles", "").toStringList());

    Config.setValue("Banlist/Generals", ban_list);
}

QStringList Engine::getExtraGeneralsBan() const
{
    return Config.value("Banlist/Generals", "").toStringList();
}

WrappedCard *Engine::getWrappedCard(int cardId)
{
    Card *card = getCard(cardId);
    WrappedCard *wrappedCard = qobject_cast<WrappedCard *>(card);
    Q_ASSERT(wrappedCard != NULL && wrappedCard->getId() == cardId);
    return wrappedCard;
}

Card *Engine::getCard(int cardId)
{
    Card *card = NULL;
    if (cardId < 0 || cardId >= cards.length())
        return NULL;
    QObject *room = currentRoomObject();
    Q_ASSERT(room);
    Room *serverRoom = qobject_cast<Room *>(room);
    if (serverRoom != NULL) {
        card = serverRoom->getCard(cardId);
    } else {
        Client *clientRoom = qobject_cast<Client *>(room);
        Q_ASSERT(clientRoom != NULL);
        card = clientRoom->getCard(cardId);
    }
    Q_ASSERT(card);
    return card;
}

const Card *Engine::getEngineCard(int cardId) const
{
    if (cardId == Card::S_UNKNOWN_CARD_ID)
        return NULL;
    else if (cardId < 0 || cardId >= cards.length()) {
        Q_ASSERT(!(cardId < 0 || cardId >= cards.length()));
        return NULL;
    } else {
        Q_ASSERT(cards[cardId] != NULL);
        return cards[cardId];
    }
}

Card *Engine::cloneCard(const Card *card) const
{
    QString name = card->getClassName();
    Card *result = cloneCard(name, card->getSuit(), card->getNumber(), card->getFlags());
    if (result == NULL)
        return NULL;
    result->setId(card->getEffectiveId());
    result->setSkillName(card->getSkillName(false));
    result->setObjectName(card->objectName());
    return result;
}

Card *Engine::cloneCard(const QString &name, Card::Suit suit, int number, const QStringList &flags) const
{
    Card *card = NULL;
    if (luaBasicCard_className2objectName.keys().contains(name)) {
        const LuaBasicCard *lcard = luaBasicCards.value(name, NULL);
        if (!lcard) return NULL;
        card = lcard->clone(suit, number);
    } else if (luaBasicCard_className2objectName.values().contains(name)) {
        QString class_name = luaBasicCard_className2objectName.key(name, name);
        const LuaBasicCard *lcard = luaBasicCards.value(class_name, NULL);
        if (!lcard) return NULL;
        card = lcard->clone(suit, number);
    } else if (luaTrickCard_className2objectName.keys().contains(name)) {
        const LuaTrickCard *lcard = luaTrickCards.value(name, NULL);
        if (!lcard) return NULL;
        card = lcard->clone(suit, number);
    } else if (luaTrickCard_className2objectName.values().contains(name)) {
        QString class_name = luaTrickCard_className2objectName.key(name, name);
        const LuaTrickCard *lcard = luaTrickCards.value(class_name, NULL);
        if (!lcard) return NULL;
        card = lcard->clone(suit, number);
    } else if (luaWeapon_className2objectName.keys().contains(name)) {
        const LuaWeapon *lcard = luaWeapons.value(name, NULL);
        if (!lcard) return NULL;
        card = lcard->clone(suit, number);
    } else if (luaWeapon_className2objectName.values().contains(name)) {
        QString class_name = luaWeapon_className2objectName.key(name, name);
        const LuaWeapon *lcard = luaWeapons.value(class_name, NULL);
        if (!lcard) return NULL;
        card = lcard->clone(suit, number);
    } else if (luaArmor_className2objectName.keys().contains(name)) {
        const LuaArmor *lcard = luaArmors.value(name, NULL);
        if (!lcard) return NULL;
        card = lcard->clone(suit, number);
    } else if (luaArmor_className2objectName.values().contains(name)) {
        QString class_name = luaArmor_className2objectName.key(name, name);
        const LuaArmor *lcard = luaArmors.value(class_name, NULL);
        if (!lcard) return NULL;
        card = lcard->clone(suit, number);
    } else if (luaTreasure_className2objectName.keys().contains(name)) {
        const LuaTreasure *lcard = luaTreasures.value(name, NULL);
        if (!lcard) return NULL;
        card = lcard->clone(suit, number);
    } else if (luaTreasure_className2objectName.values().contains(name)) {
        QString class_name = luaTreasure_className2objectName.key(name, name);
        const LuaTreasure *lcard = luaTreasures.value(class_name, NULL);
        if (!lcard) return NULL;
        card = lcard->clone(suit, number);
    } else {
        const QMetaObject *meta = metaobjects.value(name, NULL);
        if (meta == NULL)
            meta = metaobjects.value(objectName2className.value(name, QString()), NULL);
        if (meta) {
            QObject *card_obj = meta->newInstance(Q_ARG(Card::Suit, suit), Q_ARG(int, number));
            card_obj->setObjectName(className2objectName.value(name, name));
            card = qobject_cast<Card *>(card_obj);
        }
    }
    if (!card) return NULL;
    card->clearFlags();
    if (!flags.isEmpty()) {
        foreach(QString flag, flags)
            card->setFlags(flag);
    }
    return card;
}

SkillCard *Engine::cloneSkillCard(const QString &name) const
{
    const QMetaObject *meta = metaobjects.value(name, NULL);
    if (meta) {
        QObject *card_obj = meta->newInstance();
        SkillCard *card = qobject_cast<SkillCard *>(card_obj);
        return card;
    } else
        return NULL;
}

#ifndef USE_BUILDBOT
QString Engine::getVersionNumber() const
{
    return "20210205";
}
#endif

QString Engine::getVersion() const
{
    QString version_number = getVersionNumber();
    QString mod_name = getMODName();
    if (mod_name == "official")
        return version_number;
    else
        return QString("%1:%2").arg(version_number).arg(mod_name);
}

QString Engine::getVersionName() const
{
    return "V2";
}

QString Engine::getMODName() const
{
    return "Sheauhaw";
}

QStringList Engine::getExtensions() const
{
    QStringList extensions;
    QList<const Package *> packages = findChildren<const Package *>();
    foreach (const Package *package, packages) {
        if (package->inherits("Scenario"))
            continue;

        extensions << package->objectName();
    }
    return extensions;
}

QStringList Engine::getKingdoms() const
{
    static QStringList kingdoms;
    if (kingdoms.isEmpty())
        kingdoms = GetConfigFromLuaState(lua, "kingdoms").toStringList();

    return kingdoms;
}

QColor Engine::getKingdomColor(const QString &kingdom) const
{
    static QMap<QString, QColor> color_map;
    if (color_map.isEmpty()) {
        QVariantMap map = GetValueFromLuaState(lua, "config", "kingdom_colors").toMap();
        QMapIterator<QString, QVariant> itor(map);
        while (itor.hasNext()) {
            itor.next();
            QColor color(itor.value().toString());
            if (!color.isValid()) {
                qWarning("Invalid color for kingdom %s", qPrintable(itor.key()));
                color = QColor(128, 128, 128);
            }
            color_map[itor.key()] = color;
        }
        Q_ASSERT(!color_map.isEmpty());
    }

    return color_map.value(kingdom);
}

QMap<QString, QColor> Engine::getSkillTypeColorMap() const
{
    static QMap<QString, QColor> color_map;
    if (color_map.isEmpty()) {
        QVariantMap map = GetValueFromLuaState(lua, "config", "skill_type_colors").toMap();
        QMapIterator<QString, QVariant> itor(map);
        while (itor.hasNext()) {
            itor.next();
            QColor color(itor.value().toString());
            if (!color.isValid()) {
                qWarning("Invalid color for skill type %s", qPrintable(itor.key()));
                color = QColor(128, 128, 128);
            }
            color_map[itor.key()] = color;
        }
        Q_ASSERT(!color_map.isEmpty());
    }

    return color_map;
}

QStringList Engine::getChattingEasyTexts() const
{
    static QStringList easy_texts;
    if (easy_texts.isEmpty())
        easy_texts = GetConfigFromLuaState(lua, "easy_text").toStringList();

    return easy_texts;
}

QString Engine::getSetupString() const
{
    int timeout = Config.OperationNoLimit ? 0 : Config.OperationTimeout;
    QString flags;
    if (Config.RandomSeat)
        flags.append("R");
    if (Config.EnableCheat)
        flags.append("C");
    if (Config.EnableCheat && Config.FreeChoose)
        flags.append("F");
    if (Config.Enable2ndGeneral)
        flags.append("S");
    if (Config.EnableSame)
        flags.append("T");
    if (Config.EnableBasara)
        flags.append("B");
    if (Config.EnableHegemony)
        flags.append("H");
    if (Config.EnableAI)
        flags.append("A");
    if (Config.DisableChat)
        flags.append("M");
    if (Config.EnableTriggerOrder)
        flags.append("O");

    if (Config.MaxHpScheme == 1)
        flags.append("1");
    else if (Config.MaxHpScheme == 2)
        flags.append("2");
    else if (Config.MaxHpScheme == 3)
        flags.append("3");
    else if (Config.MaxHpScheme == 0) {
        char c = Config.Scheme0Subtraction + 5 + 'a'; // from -5 to 12
        flags.append(c);
    }

    QString server_name = Config.ServerName.toUtf8().toBase64();
    QStringList setup_items;
    QString mode = Config.GameMode;
    if (mode == "02_1v1")
        mode = mode + Config.value("1v1/Rule", "2013").toString();
    else if (mode == "06_3v3")
        mode = mode + Config.value("3v3/OfficialRule", "2013").toString();
    QList<int> cards = Sanguosha->getRandomCards();
    QStringList card_string;
    foreach (int card, cards)
        card_string.append(QString::number(card));

    setup_items << server_name
                << mode
                << QString::number(timeout)
                << QString::number(Config.NullificationCountDown)
                << Sanguosha->getBanPackages().join("+")
                << flags
                << QString::number(Config.GeneralLevel)
                << card_string.join("+");

    return setup_items.join(":");
}

QMap<QString, QString> Engine::getAvailableModes() const
{
    return modes;
}

QString Engine::getModeName(const QString &mode) const
{
    if (modes.contains(mode))
        return modes.value(mode);
    else
        return tr("%1 [Scenario mode]").arg(translate(mode));
}

int Engine::getPlayerCount(const QString &mode) const
{
    if (mode == "05_zhfd" && Config.value("zhfd/Mode", "NormalMode").toString() == "BossMode")
        return 8;
    if (mode == "04_year" && Config.value("year/Mode", "2018").toString() == "2018")
        return 6;
    if (mode == "04_year" && Config.value("year/Mode", "2018").toString() == "2019G")
        return 8;
    if (mode == "04_year" && Config.value("year/Mode", "2018").toString() == "2019Y")
        return 6;
    if (modes.contains(mode) || isNormalGameMode(mode)) { // hidden pz settings?
        QRegExp rx("(\\d+)");
        int index = rx.indexIn(mode);
        if (index != -1)
            return rx.capturedTexts().first().toInt();
    } else {
        // scenario mode
        const Scenario *scenario = getScenario(mode);
        Q_ASSERT(scenario);
        return scenario->getPlayerCount();
    }

    return -1;
}

QString Engine::getRoles(const QString &mode) const
{
    int n = getPlayerCount(mode);

    if (mode == "02_1v1") {
        return "ZN";
    } else if (mode == "03_1v2") {
        return "ZFF";
    } else if (mode == "04_1v3" || mode == "04_boss") {
        return "ZFFF";
    } else if (mode == "04_year") {
        if (Config.value("year/Mode", "2018").toString() == "2018")
            return "FCFCFC";
        if (Config.value("year/Mode", "2018").toString() == "2019G")
            return "FFCFCCFC";
        if (Config.value("year/Mode", "2018").toString() == "2019Y")
            return "CFFCFF";
    } else if (mode == "05_zhfd") {
        if (Config.value("zhfd/Mode", "NormalMode").toString() == "BossMode")
            return "ZCFCFCFC";
        return "CZCFF";
    } else if (mode == "06_swzs") {
        return "CZCFFF";
    } else if (mode == "08_defense") {
        return "FFFFCCCC";
    } else if (mode == "08_zdyj") {
        return "ZCCFFFFN";
    } else if (mode == "08_hongyan") {
        return "ZCCFFFFN";
    } else if (mode == "08_dragonboat") {
        return "EESSUUQQ";
    }

    if (modes.contains(mode) || isNormalGameMode(mode)) { // hidden pz settings?
        static const char *table1[] = {
            "",
            "",

            "ZF", // 2
            "ZFN", // 3
            "ZNFF", // 4
            "ZCFFN", // 5
            "ZCFFFN", // 6
            "ZCCFFFN", // 7
            "ZCCFFFFN", // 8
            "ZCCCFFFFN", // 9
            "ZCCCFFFFFN", // 10
            "ZCCCCFFFFFN", // 11
            "ZCCCCFFFFFFN", // 12
        };

        static const char *table2[] = {
            "",
            "",

            "ZF", // 2
            "ZFN", // 3
            "ZNFF", // 4
            "ZCFFN", // 5
            "ZCFFNN", // 6
            "ZCCFFFN", // 7
            "ZCCFFFNN", // 8
            "ZCCCFFFFN", // 9
            "ZCCCFFFFNN", // 10
            "ZCCCFFFFFNN", // 11
            "ZCCCCFFFFFNN", // 12
        };

        const char **table = mode.endsWith("d") ? table2 : table1;
        QString rolechar = table[n];
        if (mode.endsWith("z"))
            rolechar.replace("N", "C");
        else if (Config.EnableHegemony) {
            rolechar.replace("F", "N");
            rolechar.replace("C", "N");
        }

        return rolechar;
    } else if (mode.startsWith("@")) {
        if (n == 8)
            return "ZCCCNFFF";
        else if (n == 6)
            return "ZCCNFF";
    } else {
        const Scenario *scenario = getScenario(mode);
        if (scenario)
            return scenario->getRoles();
    }
    return QString();
}

QStringList Engine::getRoleList(const QString &mode) const
{
    QString roles = getRoles(mode);

    QStringList role_list;
    for (int i = 0; roles[i] != '\0'; i++) {
        QString role;
        switch (roles[i].toLatin1()) {
        case 'Z':
            role = "lord";
            break;
        case 'C':
            role = "loyalist";
            break;
        case 'N':
            role = "renegade";
            break;
        case 'F':
            role = "rebel";
            break;
        case 'E':
            role = "dragon_wei";
            break;
        case 'S':
            role = "dragon_shu";
            break;
        case 'U':
            role = "dragon_wu";
            break;
        case 'Q':
            role = "dragon_qun";
            break;
        }
        role_list << role;
    }

    return role_list;
}

int Engine::getCardCount() const
{
    return cards.length();
}

QStringList Engine::getLords(bool contain_banned) const
{
    QStringList lords;
    QStringList general_names = getLimitedGeneralNames();

    // add intrinsic lord
    foreach (QString lord, lord_list) {
        if (!general_names.contains(lord))
            continue;
        if (!contain_banned) {
            if (ServerInfo.GameMode.endsWith("p")
                    || ServerInfo.GameMode.endsWith("pd")
                    || ServerInfo.GameMode.endsWith("pz")
                    || ServerInfo.GameMode.contains("_mini_")
                    || ServerInfo.GameMode == "custom_scenario")
                if (getExtraGeneralsBan().contains(lord))
                    continue;
            if (Config.Enable2ndGeneral && BanPair::isBanned(lord))
                continue;
        }
        lords << lord;
    }

    return lords;
}

QStringList Engine::getRandomLords() const
{
    QStringList banlist_ban = getExtraGeneralsBan();

    QStringList lords;

    foreach (QString alord, getLords()) {
        if (banlist_ban.contains(alord)) continue;
        lords << alord;
    }

    int lord_num = Config.value("LordMaxChoice", -1).toInt();
    if (lord_num != -1 && lord_num < lords.length()) {
        int to_remove = lords.length() - lord_num;
        for (int i = 0; i < to_remove; i++) {
            lords.removeAt(qrand() % lords.length());
        }
    }

    QStringList nonlord_list, all_nonlord_list;
    foreach (QString nonlord, generals.keys()) {
        if (isGeneralHidden(nonlord) || lord_list.contains(nonlord)) continue;
        const General *general = generals.value(nonlord);
        if (getBanPackages().contains(general->getPackage()))
            continue;
        if (Config.Enable2ndGeneral && BanPair::isBanned(general->objectName()))
            continue;
        if (banlist_ban.contains(general->objectName()))
            continue;

        QString main_name = getMainGeneral(nonlord);
        all_nonlord_list << nonlord;
        if (!nonlord_list.contains(main_name))
            nonlord_list << main_name;
    }

    qShuffle(nonlord_list);

    int extra = Config.value("NonLordMaxChoice", 2).toInt();
    if (lord_num == 0 && extra == 0)
        extra = 1;
    for (int i = 0; i < extra; i++) {
        QString name = nonlord_list.at(i);
        if (all_nonlord_list.contains(name))
            lords << name;
        foreach (QString sp, getConvertGenerals(name)) {
            if (all_nonlord_list.contains(sp))
                lords << sp;
        }
        if (i == nonlord_list.length() - 1) break;
    }
    return lords;
}

QStringList Engine::getRandomFemaleLords(bool zuoci) const
{
    QStringList banlist_ban = getExtraGeneralsBan();

    QStringList lords;

    QStringList all_maf_lords = Sanguosha->getLords();
    QStringList all_lords;
    foreach (QString name, all_maf_lords) {
        const General *general = Sanguosha->getGeneral(name);
        if (general && general->isFemale())
            all_lords << general->objectName();
        else if (general && name.contains("zuoci") && zuoci)
            all_lords << name;
    }

    foreach (QString alord, all_lords) {
        if (banlist_ban.contains(alord)) continue;
        lords << alord;
    }

    int lord_num = Config.value("LordMaxChoice", -1).toInt();
    if (lord_num != -1 && lord_num < lords.length()) {
        int to_remove = lords.length() - lord_num;
        for (int i = 0; i < to_remove; i++) {
            lords.removeAt(qrand() % lords.length());
        }
    }

    QStringList nonlord_list, all_nonlord_list;
    foreach (QString nonlord, generals.keys()) {
        if (isGeneralHidden(nonlord) || lord_list.contains(nonlord)) continue;
        const General *general = generals.value(nonlord);
        if (getBanPackages().contains(general->getPackage()))
            continue;
        if (Config.Enable2ndGeneral && BanPair::isBanned(general->objectName()))
            continue;
        if (banlist_ban.contains(general->objectName()))
            continue;
        //const General *general = Sanguosha->getGeneral(nonlord);
        if (!general || !(general->isFemale() || (nonlord.contains("zuoci") && zuoci)))
            continue;
        QString main_name = getMainGeneral(nonlord);
        all_nonlord_list << nonlord;
        if (!nonlord_list.contains(main_name))
            nonlord_list << main_name;
    }

    qShuffle(nonlord_list);

    int extra = Config.value("NonLordMaxChoice", 2).toInt();
    if (lord_num == 0 && extra == 0)
        extra = 1;
    for (int i = 0; i < extra; i++) {
        QString name = nonlord_list.at(i);
        if (all_nonlord_list.contains(name))
            lords << name;
        foreach (QString sp, getConvertGenerals(name)) {
            if (all_nonlord_list.contains(sp))
                lords << sp;
        }
        if (i == nonlord_list.length() - 1) break;
    }
    return lords;
}

QStringList Engine::getLimitedGeneralNames(const QString &kingdom) const
{
    QStringList general_names;
    QHashIterator<QString, const General *> itor(generals);
    while (itor.hasNext()) {
        itor.next();
        const General *gen = itor.value();
        if ((kingdom.isEmpty() || gen->getKingdom() == kingdom)
                && !isGeneralHidden(gen->objectName()) && !getBanPackages().contains(gen->getPackage()))
            general_names << itor.key();
    }

    // special case for neo standard package
    if (getBanPackages().contains("standard") && !getBanPackages().contains("nostal_standard")) {
        if (kingdom.isEmpty() || kingdom == "wei")
            general_names << "zhenji";
        if (kingdom.isEmpty() || kingdom == "shu")
            general_names << "zhugeliang";
        if (kingdom.isEmpty() || kingdom == "wu")
            general_names << "sunquan" << "sunshangxiang";
    }

    return general_names;
}

QStringList Engine::getRandomGenerals(int count, const QSet<QString> &ban_set, const QString &kingdom) const
{
    QStringList all_generals = getLimitedGeneralNames(kingdom);

    QSet<QString> general_set = all_generals.toSet();

    QStringList banned_generals = getExtraGeneralsBan();
    general_set.subtract(banned_generals.toSet());

    all_generals = general_set.subtract(ban_set).toList();

    if (count < 1) {
        // shuffle them
        qShuffle(all_generals);
        return all_generals;
    }

    QStringList main_generals = getMainGenerals(all_generals);

    Q_ASSERT(main_generals.count() >= count);

    qShuffle(main_generals);

    QStringList main_list = main_generals.mid(0, count);
    Q_ASSERT(main_list.count() == count);

    QStringList general_list;
    foreach (QString name, main_list) {
        if (all_generals.contains(name))
            general_list << name;
        foreach (QString sp, getConvertGenerals(name)) {
            if (all_generals.contains(sp))
                general_list << sp;
        }
    }
    return general_list;
}

QStringList Engine::getRandomFemaleGenerals(int count, const QSet<QString> &ban_set, const QString &kingdom, bool zuoci) const
{
    QStringList all_maf_generals = getLimitedGeneralNames(kingdom);
    QStringList all_generals;
    foreach (QString name, all_maf_generals) {
        const General *general = Sanguosha->getGeneral(name);
        if (general && general->isFemale())
            all_generals << general->objectName();
        else if (general && name.contains("zuoci") && zuoci)
            all_generals << name;
    }

    QSet<QString> general_set = all_generals.toSet();

    QStringList banned_generals = getExtraGeneralsBan();
    general_set.subtract(banned_generals.toSet());

    all_generals = general_set.subtract(ban_set).toList();

    if (count < 1) {
        // shuffle them
        qShuffle(all_generals);
        return all_generals;
    }

    QStringList main_generals = getMainGenerals(all_generals);

    Q_ASSERT(main_generals.count() >= count);

    qShuffle(main_generals);

    QStringList main_list = main_generals.mid(0, count);
    Q_ASSERT(main_list.count() == count);

    QStringList general_list;
    foreach (QString name, main_list) {
        if (all_generals.contains(name))
            general_list << name;
        foreach (QString sp, getConvertGenerals(name)) {
            if (all_generals.contains(sp))
                general_list << sp;
        }
    }
    return general_list;
}

QList<int> Engine::getRandomCards() const
{
    bool exclude_disaters = false, using_2012_3v3 = false, using_2013_3v3 = false, include_zdyj = false,
         include_dragonboat = false, include_swzs = false, include_year_18 = false, include_year_19 = false;
    QStringList extra_ban = QStringList();

    if (Config.GameMode == "06_3v3") {
        using_2012_3v3 = (Config.value("3v3/OfficialRule", "2013").toString() == "2012");
        using_2013_3v3 = (Config.value("3v3/OfficialRule", "2013").toString() == "2013");
        exclude_disaters = !Config.value("3v3/UsingExtension", false).toBool() || Config.value("3v3/ExcludeDisasters", true).toBool();
    }

    if (Config.GameMode == "04_1v3")
        exclude_disaters = true;

    if (Config.GameMode == "08_zdyj") {
        include_zdyj = true;
        extra_ban << Config.BestLoyalistSets["cards_ban"];
        if (Config.value("zdyj/Rule", "2017").toString() != "2017")
            extra_ban << Config.BestLoyalistSets["cards_ban_old"];
        else
            extra_ban << Config.BestLoyalistSets["cards_ban_new"];
    }

    if (Config.GameMode == "08_dragonboat") {
        include_dragonboat = true;
        exclude_disaters = true;
        extra_ban << Config.DragonBoatBanC["cards"];
    }

    if (Config.GameMode == "04_year") {
        if (Config.value("year/Mode", "2018").toString() == "2018") {
            include_year_18 = true;
            exclude_disaters = false;
            extra_ban << Config.YearBossBanC["cards"];
        }
        if (Config.value("year/Mode", "2018").toString().contains("2019")) {
            include_year_19 = true;
            exclude_disaters = false;
        }
    }

    if (Config.GameMode == "06_swzs") {
        include_swzs = true;
        extra_ban << Config.GodsReturnBanC["cards"];
    }

    QList<int> list;
    foreach (Card *card, cards) {
        card->clearFlags();

        QStringList banned_patterns = Config.value("Banlist/Cards").toStringList();
        bool removed = false;
        foreach (QString banned_pattern, banned_patterns) {
            if (matchExpPattern(banned_pattern, NULL, card)) {
                removed = true;
                break;
            }
        }
        if (!removed && extra_ban.contains(card->objectName()))
            removed = true;
        if (removed)
            continue;

        if (exclude_disaters && card->isKindOf("Disaster"))
            continue;

        if (card->getPackage() == "New3v3Card" && (using_2012_3v3 || using_2013_3v3))
            list << card->getId();
        else if (card->getPackage() == "New3v3_2013Card" && using_2013_3v3)
            list << card->getId();
        else if (card->getPackage() == "BestLoyalistCard" && include_zdyj)
            list << card->getId();
        else if (card->getPackage() == "DragonBoatCard" && include_dragonboat)
            list << card->getId();
        else if (card->getPackage() == "YearBoss2018Card" && include_year_18)
            list << card->getId();
        else if (card->getPackage() == "YearBoss2019Card" && include_year_19)
            list << card->getId();
        else if (card->getPackage() == "GodsReturnCard" && include_swzs)
            list << card->getId();

        if (Config.GameMode == "02_1v1" && !Config.value("1v1/UsingCardExtension", false).toBool()) {
            if (card->getPackage() == "New1v1Card")
                list << card->getId();
            continue;
        }

        if (Config.GameMode == "06_3v3" && !Config.value("3v3/UsingExtension", false).toBool()
                && card->getPackage() != "standard_cards" && card->getPackage() != "standard_ex_cards")
            continue;
        if (!getBanPackages().contains(card->getPackage()))
            list << card->getId();
    }
    if (using_2012_3v3 || using_2013_3v3)
        list.removeOne(98);
    if (using_2013_3v3) {
        list.removeOne(53);
        list.removeOne(54);
    }

    qShuffle(list);

    return list;
}

QString Engine::getRandomGeneralName() const
{
    return generals.keys().at(qrand() % generals.size());
}

void Engine::playSystemAudioEffect(const QString &name, bool superpose) const
{
    playAudioEffect(QString("audio/system/%1.ogg").arg(name), superpose);
}

void Engine::playAudioEffect(const QString &filename, bool superpose) const
{
#ifdef AUDIO_SUPPORT
    if (!Config.EnableEffects)
        return;
    if (filename.isNull())
        return;

    Audio::play(filename, superpose);
#endif
}

void Engine::playSkillAudioEffect(const QString &skill_name, int index, bool superpose) const
{
    const Skill *skill = skills.value(skill_name, NULL);
    if (skill)
        skill->playAudioEffect(index, superpose);
}

QStringList Engine::findSkillAudioFileNames(const QString &skillName, const QString &generalName, int skinId) const
{
    QStringList fileNames;

    QString eventName = generalName + "-" + skillName;

    if (!generalName.isEmpty() && Sanguosha->getGeneral(generalName)) {
        if (skinId > 0) {
            for (int i = 1;; i++) {
                QString effect_file = QString("hero-skin/%1/%2/%3%4.ogg")
                                      .arg(generalName).arg(QString::number(skinId))
                                      .arg(skillName).arg(QString::number(i));
                if (QFile::exists(effect_file))
                    fileNames << effect_file;
                else
                    break;
            }

            if (fileNames.isEmpty()) {
                QString effect_file = QString("hero-skin/%1/%2/%3.ogg")
                                      .arg(generalName).arg(QString::number(skinId)).arg(skillName);
                if (QFile::exists(effect_file))
                    fileNames << effect_file;
            }
        }

        if (fileNames.isEmpty()) {
            for (int i = 1;; i++) {
                QString effect_file = QString("audio/skill/%1%2.ogg").arg(eventName).arg(QString::number(i));
                if (QFile::exists(effect_file))
                    fileNames << effect_file;
                else
                    break;
            }
        }

        if (fileNames.isEmpty()) {
            QString effect_file = QString("audio/skill/%1.ogg").arg(eventName);
            if (QFile::exists(effect_file))
                fileNames << effect_file;
        }
    }

    if (fileNames.isEmpty()) {
        const Skill *skill = Sanguosha->getSkill(skillName);
        if (skill) {
            QString owner_name = skill->getOwner();
            if (generalName.isEmpty() || owner_name.isEmpty() || owner_name == generalName || generalName.contains("_" + owner_name))
                fileNames = skill->getSources();
        }
    }
    return fileNames;
}

const Skill *Engine::getSkill(const QString &skill_name) const
{
    return skills.value(skill_name, NULL);
}

const Skill *Engine::getSkill(const EquipCard *equip) const
{
    if (equip == NULL) return NULL;

    return getSkill(equip->objectName());
}

QStringList Engine::getSkillNames() const
{
    return skills.keys();
}

const TriggerSkill *Engine::getTriggerSkill(const QString &skill_name) const
{
    const Skill *skill = getSkill(skill_name);
    if (skill)
        return qobject_cast<const TriggerSkill *>(skill);
    else
        return NULL;
}

const ViewAsSkill *Engine::getViewAsSkill(const QString &skill_name) const
{
    const Skill *skill = getSkill(skill_name);
    if (skill == NULL)
        return NULL;

    if (skill->inherits("ViewAsSkill"))
        return qobject_cast<const ViewAsSkill *>(skill);
    else if (skill->inherits("TriggerSkill")) {
        const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(skill);
        return trigger_skill->getViewAsSkill();
    } else
        return NULL;
}

const ProhibitSkill *Engine::isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &others) const
{
    if (card->isKindOf("SingleTargetTrick") && !others.isEmpty()) return NULL;

    foreach (const ProhibitSkill *skill, prohibit_skills) {
        if (skill->isProhibited(from, to, card, others))
            return skill;
    }

    return NULL;
}

const FixCardSkill *Engine::isCardFixed(const Player *from, const Player *to, const QString &flags, Card::HandlingMethod method) const
{
    foreach (const FixCardSkill *skill, fixcard_skills) {
        if (skill->isCardFixed(from, to, flags, method))
            return skill;
    }

    return NULL;
}

const CardLimitedSkill *Engine::isCardLimited(const Player *player, const Card *card, Card::HandlingMethod method) const
{
    foreach (const CardLimitedSkill *skill, limitedcard_skills) {
        if (skill->isCardLimited(player, card, method))
            return skill;
    }

    return NULL;
}

const HideCardSkill *Engine::isCardHided(const Player *player, const Card *card) const
{
    foreach (const HideCardSkill *skill, hidecard_skills) {
        if (skill->isCardHided(player, card))
            return skill;
    }

    return NULL;
}

const ViewHasSkill *Engine::ViewHas(const Player *player, const QString &skill_name, const QString &flag) const
{
    foreach (const ViewHasSkill *skill, viewhas_skills) {
        if (skill->ViewHas(player, skill_name, flag))
            return skill;
    }

    return NULL;
}

int Engine::correctDistance(const Player *from, const Player *to) const
{
    int correct = 0;

    foreach (const DistanceSkill *skill, distance_skills) {
        correct += skill->getCorrect(from, to);
    }

    return correct;
}

int Engine::correctFixedDistance(const Player *from, const Player *to) const
{
    int min = -1;

    foreach (const DistanceSkill *skill, distance_skills) {
        int fix = skill->getFixed(from, to);
        if (fix > 0)
            min = min <= 0 ? fix : qMin(fix, min);
    }

    return min;
}

int Engine::correctMaxCards(const Player *target, bool fixed) const
{
    if (fixed) {
        int max = -1;
        foreach (const MaxCardsSkill *skill, maxcards_skills) {
            int f = skill->getFixed(target);
            if (f > max) max = f;
        }
        return max;
    } else {
        int extra = 0;
        foreach(const MaxCardsSkill *skill, maxcards_skills)
            extra += skill->getExtra(target);
        return extra;
    }
    return 0;
}

int Engine::correctCardTarget(const TargetModSkill::ModType type, const Player *from, const Card *card, const Player *to) const
{
    int x = 0;

    if (type == TargetModSkill::Residue) {
        foreach (const TargetModSkill *skill, targetmod_skills) {
            ExpPattern p(skill->getPattern());
            if (p.match(from, card)) {
                int residue = skill->getResidueNum(from, card, to);
                if (residue >= 998) return residue;
                x += residue;
            }
        }
    } else if (type == TargetModSkill::DistanceLimit) {
        foreach (const TargetModSkill *skill, targetmod_skills) {
            ExpPattern p(skill->getPattern());
            if (p.match(from, card)) {
                int distance_limit = skill->getDistanceLimit(from, card, to);
                if (distance_limit >= 998) return distance_limit;
                x += distance_limit;
            }
        }
    } else if (type == TargetModSkill::ExtraTarget) {
        foreach (const TargetModSkill *skill, targetmod_skills) {
            ExpPattern p(skill->getPattern());
            if (p.match(from, card)) {
                x += skill->getExtraTargetNum(from, card);
            }
        }
    }

    return x;
}

bool Engine::correctSkillValidity(const Player *player, const Skill *skill) const
{
    foreach (const InvaliditySkill *is, invalidity_skills) {
        if (!is->isSkillValid(player, skill))
            return false;
    }
    return true;
}

int Engine::correctAttackRange(const Player *target, bool include_weapon, bool fixed) const
{
    int extra = 0;

    foreach (const AttackRangeSkill *skill, attack_range_skills) {
        if (fixed) {
            int f = skill->getFixed(target, include_weapon);
            if (f > extra)
                extra = f;
        } else {
            extra += skill->getExtra(target, include_weapon);
        }
    }

    return extra;
}

const StatusAbilitySkill *Engine::turnOverSkill(const Player *target) const
{
    foreach (const StatusAbilitySkill *skill, status_ability_skills) {
        if (!skill->canTurnOver(target))
            return skill;
    }
    return NULL;
}

const StatusAbilitySkill *Engine::setChainSkill(const Player *target, bool setting) const
{
    foreach (const StatusAbilitySkill *skill, status_ability_skills) {
        if (!skill->canSetChain(target, setting))
            return skill;
    }
    return NULL;
}

const StatusAbilitySkill *Engine::pindianProhibitSkill(const Player *to, const Player *from) const
{
    foreach (const StatusAbilitySkill *skill, status_ability_skills) {
        if (!skill->canPindian(to, from))
            return skill;
    }
    return NULL;
}

QList<const StatusAbilitySkill *> Engine::turnDelayedTrickSkills(const Player *player, const Card *card) const
{
    QList<const StatusAbilitySkill *> skills;
    foreach (const StatusAbilitySkill *skill, status_ability_skills) {
        if (skill->turnDelayedTrickResult(player, card))
            skills.append(skill);
    }
    return skills;
}

bool Engine::correctCanTurnOver(const Player *target) const
{
    return turnOverSkill(target) == NULL;
}

bool Engine::correctCanSetChain(const Player *target, bool setting) const
{
    return setChainSkill(target, setting) == NULL;
}

bool Engine::correctCanPindian(const Player *to, const Player *from) const
{
    return pindianProhibitSkill(to, from) == NULL;
}

bool Engine::correctTurnDelayedTrickResult(const Player *player, const Card *card) const
{
    return turnDelayedTrickSkills(player, card).size() & 1;
}
