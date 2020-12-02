#include "general.h"
#include "engine.h"
#include "skill.h"
#include "package.h"
#include "client.h"
#include "settings.h"
#include "skin-bank.h"

#include <QSize>
#include <QFile>
#include <QMessageBox>

General::General(Package *package, const QString &name, const QString &kingdom,
    int max_hp, bool male, bool hidden, bool never_shown, int hp)
    : QObject(package), kingdom(kingdom), max_hp(max_hp), hp(hp), gender(male ? Male : Female),
    hidden(hidden), never_shown(never_shown), skin_count(-1)
{
    static QChar lord_symbol('$');
    if (name.endsWith(lord_symbol)) {
        QString copy = name;
        copy.remove(lord_symbol);
        lord = true;
        setObjectName(copy);
    } else {
        lord = false;
        setObjectName(name);
    }
}

int General::getMaxHp() const
{
    return max_hp;
}

int General::getHp() const
{
    if (hp == -1)
        return max_hp;
    return hp;
}

QString General::getKingdom() const
{
    return kingdom;
}

bool General::isMale() const
{
    return gender == Male;
}

bool General::isFemale() const
{
    return gender == Female;
}

bool General::isNeuter() const
{
    return gender == Neuter;
}

void General::setGender(Gender gender)
{
    this->gender = gender;
}

General::Gender General::getGender() const
{
    return gender;
}

bool General::isLord() const
{
    return lord;
}

bool General::isHidden() const
{
    return hidden;
}

bool General::isTotallyHidden() const
{
    return never_shown;
}

void General::addSkill(Skill *skill)
{
    if (!skill) {
        QMessageBox::warning(NULL, "", tr("Invalid skill added to general %1").arg(objectName()));
        return;
    }
    if (!skillname_list.contains(skill->objectName())) {
        skill->setParent(this);
        if (skill->getOwner().isEmpty())
            skill->setOwner(objectName());
        skillname_list << skill->objectName();
    }
}

void General::addSkill(const QString &skill_name)
{
    if (!skillname_list.contains(skill_name)) {
        extra_set.insert(skill_name);
        skillname_list << skill_name;
    }
}

bool General::hasSkill(const QString &skill_name) const
{
    return skillname_list.contains(skill_name);
}

QList<const Skill *> General::getSkillList() const
{
    QList<const Skill *> skills;
    foreach (QString skill_name, skillname_list) {
		if (ServerInfo.DuringGame) {
			if (ServerInfo.GameMode == "02_1v1" && ServerInfo.GameRuleMode != "Classical") {
				if (skill_name == "mashu")
					skill_name = "xiaoxi";
                if (skill_name == "juxiang" || skill_name == "huoshou")
                    skill_name = "kofmanyi";
            }
        }
        const Skill *skill = Sanguosha->getSkill(skill_name);
		skills << skill;
    }
    return skills;
}

QList<const Skill *> General::getVisibleSkillList() const
{
    QList<const Skill *> skills;
    foreach (const Skill *skill, getSkillList()) {
        if (skill->isVisible())
            skills << skill;
    }

    return skills;
}

QSet<const Skill *> General::getVisibleSkills() const
{
    return getVisibleSkillList().toSet();
}

QSet<const TriggerSkill *> General::getTriggerSkills() const
{
    QSet<const TriggerSkill *> skills;
    foreach (QString skill_name, skillname_list) {
        const TriggerSkill *skill = Sanguosha->getTriggerSkill(skill_name);
        if (skill)
            skills << skill;
    }
    return skills;
}

void General::addRelateSkill(const QString &skill_name)
{
    related_skills << skill_name;
}

QStringList General::getRelatedSkillNames() const
{
    return related_skills;
}

QString General::getPackage() const
{
    QObject *p = parent();
    if (p)
        return p->objectName();
    else
        return QString(); // avoid null pointer exception;
}

QString General::getSkillDescription(bool include_name) const
{
    QString description;

    foreach (const Skill *skill, getVisibleSkillList()) {
        QString skill_name = Sanguosha->translate(skill->objectName());
        QString desc = skill->getDescription();
        desc.replace("\n", "<br/>");
        description.append(QString("<b>%1</b>: %2 <br/> <br/>").arg(skill_name).arg(desc));
    }

    if (include_name) {
        QString color_str = Sanguosha->getKingdomColor(kingdom).name();
        QString name = QString("<font color=%1><b>%2</b></font>     ").arg(color_str).arg(Sanguosha->translate(objectName()));
        name.prepend(QString("<img src='image/kingdom/icon/%1.png'/>    ").arg(kingdom));
		if (max_hp > 8){
            if (hp == -1)
                name.append("<img src='image/system/magatamas/hp.png' height = 12/>×" + QString::number(max_hp));
            else
                name.append("<img src='image/system/magatamas/hp.png' height = 12/>×" + QString::number(hp) + "/" + QString::number(max_hp));
		} else {
            for (int i = 0; i < max_hp; i++) {
                if (hp > 0 && i >= hp)
                    name.append("<img src='image/system/magatamas/_hp.png' height = 12/>");
                else
                    name.append("<img src='image/system/magatamas/hp.png' height = 12/>");
            }
		}

        QString gender("  <img src='image/gender/%1.png' height=17 />");
        if (isMale())
            name.append(gender.arg("male"));
        else if (isFemale())
            name.append(gender.arg("female"));

        name.append("<br/> <br/>");
        description.prepend(name);
    }

    return description;
}

QString General::getBriefName() const
{
    QString name = Sanguosha->translate("&" + objectName());
    if (name.startsWith("&"))
        name = Sanguosha->translate(objectName());

    return name;
}

void General::lastWord(int skinId) const
{
    QString fileName;
    if (skinId < 0)
        skinId = Config.value(QString("HeroSkin/%1").arg(objectName()), 0).toInt();
    if (skinId == 0) {
        fileName = QString("audio/death/%1.ogg").arg(objectName());
        if (!QFile::exists(fileName)) {
            QStringList origin_generals = objectName().split("_");
            if (origin_generals.length() > 1) {
                fileName = QString("audio/death/%1.ogg").arg(origin_generals.last());
            }
        }
    } else {
        fileName = QString("hero-skin/%1/%2/death.ogg").arg(objectName()).arg(skinId);
        if (!QFile::exists(fileName)) {
            QStringList origin_generals = objectName().split("_");
            if (origin_generals.length() > 1) {
                fileName = QString("hero-skin/%1/%2/death.ogg").arg(origin_generals.last()).arg(skinId);
            }
        }
        if (!QFile::exists(fileName)) {
            fileName = QString("audio/death/%1.ogg").arg(objectName());
            if (!QFile::exists(fileName)) {
                QStringList origin_generals = objectName().split("_");
                if (origin_generals.length() > 1) {
                    fileName = QString("audio/death/%1.ogg").arg(origin_generals.last());
                }
            }
        }
    }

    Sanguosha->playAudioEffect(fileName, false);
}

void General::tryLoadingSkinTranslation(const int skinId) const
{
    if (translated_skins.contains(skinId))
        return;

    const QString file = QString("hero-skin/%1/%2/%3.lua").arg(objectName()).arg(skinId).arg(Config.value("Language", "zh_CN").toString());

    if (QFile::exists(file)) {
        Sanguosha->setProperty("CurrentSkinId", skinId);
        DoLuaScript(Sanguosha->getLuaState(), file.toLatin1().constData());
    }

    translated_skins << skinId;
}

int General::skinCount() const
{
    if (skin_count != -1)
        return skin_count;

    forever {
        if (!G_ROOM_SKIN.generalHasSkin(objectName(), (++skin_count) + 1))
        return skin_count;
    }
}

QString General::getTitle(const int skinId) const
{
    if (skinId == 0)
        return tr("classic figure");
    QString title;
    const QString id = QString::number(skinId);
    if (skinId == 0)
        title = Sanguosha->translate("#" + objectName());
    else
        title = Sanguosha->translate("#" + id + objectName());

    if (title.startsWith("#")) {
        if (objectName().contains("_")) {
            const QString generalName = objectName().split("_").last();
            if (skinId == 0) {
                title = Sanguosha->translate(("#") + generalName);
            } else {
                title = Sanguosha->translate(("#") + id + generalName);
                if (title.startsWith("#"))
                    title = Sanguosha->translate(("#") + generalName);
            }
        } else if (skinId != 0) {
            title = Sanguosha->translate("#" + objectName());
        }
    }
    return title;
}
