#ifndef _SETTINGS_H
#define _SETTINGS_H

#include "protocol.h"
#include <QSettings>
#include <QFont>
#include <QRectF>
#include <QPixmap>
#include <QBrush>

class Settings : public QSettings
{
    Q_OBJECT

public:
    explicit Settings();
    void init();

    const QRectF Rect;
    QFont BigFont;
    QFont SmallFont;
    QFont TinyFont;

    QFont AppFont;
    QFont UIFont;
    QColor TextEditColor;
	QColor ToolTipBackgroundColor;

    // server side
    QString ServerName;
    int CountDownSeconds;
    int NullificationCountDown;
    int GeneralLevel;
    bool EnableTriggerOrder;
    bool EnableMinimizeDialog;
    QString GameMode;
    QStringList BanPackages;
    bool RandomSeat;
    bool EnableCheat;
    bool FreeChoose;
    bool ForbidSIMC;
    bool DisableChat;
    bool FreeAssignSelf;
    bool Enable2ndGeneral;
    bool EnableSame;
    bool EnableBasara;
    bool EnableHegemony;
    int MaxHpScheme;
    int Scheme0Subtraction;
    bool PreventAwakenBelow3;
    QString Address;
    bool EnableAI;
    int AIDelay;
    int OriginAIDelay;
    bool AlterAIDelayAD;
    int AIDelayAD;
    bool SurrenderAtDeath;
    bool EnableLuckCard;
    ushort ServerPort;
    bool DisableLua;

    QStringList BossGenerals;
    int BossLevel;
    QStringList BossEndlessSkills;

    QMap<QString, QString> JianGeDefenseKingdoms;
    QMap<QString, QStringList> JianGeDefenseMachine;
    QMap<QString, QStringList> JianGeDefenseSoul;

    QMap<QString, QStringList> BestLoyalistSets;
    QMap<QString, QStringList> DragonBoatBanC;
    QMap<QString, QStringList> GodsReturnBanC;
    QMap<QString, QStringList> YearBossBanC;
    QMap<QString, QStringList> CardsBan;

    // client side
    QString HostAddress;
    QString UserName;
    QString UserAvatar;
    QStringList HistoryIPs;
    ushort DetectorPort;
    int MaxCards;

    bool EnableHotKey;
    bool NeverNullifyMyTrick;
    bool EnableAutoTarget;
    bool EnableIntellectualSelection;
    bool EnableDoubleClick;
    bool EnableSuperDrag;
    bool EnableAutoBackgroundChange;
    int OperationTimeout;
    bool OperationNoLimit;
    bool EnableEffects;
    bool EnableLastWord;
    bool EnableBgMusic;
    float BGMVolume;
    float EffectVolume;

    QString BackgroundImage;
    int BubbleChatBoxKeepTime;

    // consts
    static const int S_SURRENDER_REQUEST_MIN_INTERVAL;
    static const int S_PROGRESS_BAR_UPDATE_INTERVAL;
    static const int S_SERVER_TIMEOUT_GRACIOUS_PERIOD;
    static const int S_MOVE_CARD_ANIMATION_DURATION;
    static const int S_JUDGE_ANIMATION_DURATION;
    static const int S_JUDGE_LONG_DELAY;
};

extern Settings Config;

#endif

