#ifndef _ROOM_SCENE_H
#define _ROOM_SCENE_H

#include "photo.h"
#include "dashboard.h"
#include "table-pile.h"
#include "card.h"
#include "client.h"
#include "aux-skills.h"
#include "clientlogbox.h"
#include "chatwidget.h"
#include "skin-bank.h"
#include "sprite.h"
#include "qsanbutton.h"

class Window;
class Button;
class CardContainer;
class GuanxingBox;
class CardChooseBox;
class YanjiaoBox;
class PindianBox;
class QSanButton;
class QGroupBox;
class BubbleChatBox;
class ChooseGeneralBox;
class ChooseOptionsBox;
class PlayerCardBox;
struct RoomLayout;

#include <QGraphicsScene>
#include <QTableWidget>
#include <QMainWindow>
#include <QTextEdit>
#include <QSpinBox>
#include <QDialog>
#include <QGraphicsWidget>
#include <QGraphicsProxyWidget>
#include <QThread>
#include <QHBoxLayout>
#include <QMutex>
#include <QStack>
#ifndef Q_OS_WINRT
#include <QDeclarativeEngine>
#include <QDeclarativeContext>
#include <QDeclarativeComponent>
#endif
class ScriptExecutor : public QDialog
{
    Q_OBJECT

public:
    ScriptExecutor(QWidget *parent);

public slots:
    void doScript();
};

class DeathNoteDialog : public QDialog
{
    Q_OBJECT

public:
    DeathNoteDialog(QWidget *parent);

protected:
    virtual void accept();

private:
    QComboBox *killer, *victim;
};

class DamageMakerDialog : public QDialog
{
    Q_OBJECT

public:
    DamageMakerDialog(QWidget *parent);

protected:
    virtual void accept();

private:
    QComboBox *damage_source;
    QComboBox *damage_target;
    QComboBox *damage_nature;
    QSpinBox *damage_point;

    void fillComboBox(QComboBox *ComboBox);

private slots:
    void disableSource();
};

class KOFOrderBox : public QGraphicsPixmapItem
{
public:
    KOFOrderBox(bool self, QGraphicsScene *scene);
    void revealGeneral(const QString &name);
    void killPlayer(const QString &general_name);

private:
    QSanSelectableItem *avatars[3];
    int revealed;
};

class ReplayerControlBar : public QGraphicsObject
{
    Q_OBJECT

public:
    ReplayerControlBar(Dashboard *dashboard);
    static QString FormatTime(int secs);
    virtual QRectF boundingRect() const;

public slots:
    void setTime(int secs);
    void setSpeed(qreal speed);

protected:
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    static const int S_BUTTON_GAP = 3;
    static const int S_BUTTON_WIDTH = 25;
    static const int S_BUTTON_HEIGHT = 21;

private:
    QLabel *time_label;
    QString duration_str;
    qreal speed;
};

class RoomScene : public QGraphicsScene
{
    Q_OBJECT

public:
    RoomScene(QMainWindow *main_window);
    ~RoomScene();
    void changeTextEditBackground();
    void adjustItems();
    void showIndicator(const QString &from, const QString &to, int secs = 0);
    static void FillPlayerNames(QComboBox *ComboBox, bool add_none);
    void updateTable();
    void updateVolumeConfig();
    void redrawDashboardButtons();
    void deletePromptInfoItem();
    inline QMainWindow *mainWindow()
    {
        return main_window;
    }

    inline bool isCancelButtonEnabled() const
    {
        return cancel_button != NULL && cancel_button->isEnabled();
    }
    void stopHeroSkinChangingAnimations();

    void addHeroSkinContainer(HeroSkinContainer *heroSkinContainer);
    HeroSkinContainer *findHeroSkinContainer(const QString &generalName) const;

    Dashboard *getDasboard() const;

    GuhuoBox *current_guhuo_box;
    SelectBox *current_select_box;

    QMap<QString, GuhuoBox *> getGuhuoItems() const
    {
        return guhuo_items;
    }

    bool m_skillButtonSank;
    void setTurn(int number);

    void filterCardContainer(const ViewAsSkill *skill);


public slots:
    void addPlayer(ClientPlayer *player);
    void removePlayer(const QString &player_name);
    void loseCards(int moveId, QList<CardsMoveStruct> moves);
    void getCards(int moveId, QList<CardsMoveStruct> moves);
    void updateHandPile(const QString &pile_name, bool add, QList<int> card_ids);
    void keepLoseCardLog(const CardsMoveStruct &move);
    void keepGetCardLog(const CardsMoveStruct &move);
    // choice dialog
    void chooseGeneral(const QStringList &generals, bool single_result, const QString &reason, bool convert_enabled);
    void chooseSuit(const QStringList &suits);
    void chooseCard(const ClientPlayer *playerName, const QString &flags, const QString &reason,
        bool handcard_visible, Card::HandlingMethod method, QList<int> disabled_ids, QList<int> handcards, int min_num, int max_num);
    void chooseKingdom(const QStringList &kingdoms);
    void chooseOption(const QString &skillName, const QStringList &options, const QStringList &all_options);
    void chooseOrder(QSanProtocol::Game3v3ChooseOrderCommand reason);
    void chooseRole(const QString &scheme, const QStringList &roles);
    void chooseDirection();

    void bringToFront(QGraphicsItem *item);
    void arrangeSeats(const QList<const ClientPlayer *> &seats);
    void toggleDiscards();
    void enableTargetsAfterGuhuo();
    void enableTargetsAfterSelect();
    void enableTargets(const Card *card);
    void useSelectedCard();
    void updateStatus(Client::Status oldStatus, Client::Status newStatus);
    void cardMovedinCardchooseBox(const bool enable);
    void playPindianSuccess(int type, int index);
    void killPlayer(const QString &who);
    void revivePlayer(const QString &who);
    void showServerInformation();
    void surrender();
    void saveReplayRecord();
    void makeDamage();
    void makeKilling();
    void makeReviving();
    void doScript();
    void viewGenerals(const QString &reason, const QStringList &names);

    void handleGameEvent(const QVariant &arg);

    void doOkButton();
    void doCancelButton();
    void doDiscardButton();
	
	void setOkButton(bool state);
    void setCancelButton(bool state);

    inline QPointF tableCenterPos()
    {
        return m_tableCenterPos;
    }

    void doGongxin(const QList<int> &card_ids, bool enable_heart, QList<int> enabled_ids);

    void showPile(const QList<int> &card_ids, const QString &nam, const ClientPlayer *target);
    QString getCurrentShownPileName();
    void hidePile();

    void setChatBoxVisibleSlot();
    void pause();
    void trust();

    void addRobot();
    void doAddRobotAction();
    void fillRobots();

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);
    //this method causes crashes
    virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
    QMutex m_roomMutex;
    QMutex m_zValueMutex;

private:
    void _getSceneSizes(QSize &minSize, QSize &maxSize);
    bool _shouldIgnoreDisplayMove(CardsMoveStruct &movement);
    bool _processCardsMove(CardsMoveStruct &move, bool isLost);
    bool _m_isInDragAndUseMode;
    bool _m_superDragStarted;
    const QSanRoomSkin::RoomLayout *_m_roomLayout;
    const QSanRoomSkin::PhotoLayout *_m_photoLayout;
    const QSanRoomSkin::CommonLayout *_m_commonLayout;
    const QSanRoomSkin* _m_roomSkin;
    QGraphicsItem *_m_last_front_item;
    double _m_last_front_ZValue;
    GenericCardContainer *_getGenericCardContainer(Player::Place place, Player *player);
    QMap<int, QList<QList<CardItem *> > > _m_cardsMoveStash;
    Button *add_robot, *start_game, *return_to_main_menu;
    QList<Photo *> photos;
    QMap<QString, Photo *> name2photo;
    Dashboard *dashboard;
    TablePile *m_tablePile;
    QMainWindow *main_window;
    QSanButton *ok_button, *cancel_button, *discard_button;
    QMenu *miscellaneous_menu, *change_general_menu;
    QGraphicsItem *control_panel;
    QMap<PlayerCardContainer *, const ClientPlayer *> item2player;
    QDialog *m_choiceDialog; // Dialog for choosing generals, suits, card/equip, or kingdoms

    QGraphicsRectItem *pausing_item;
    QGraphicsSimpleTextItem *pausing_text;

    QList<QGraphicsPixmapItem *> role_items;
    CardContainer *m_cardContainer;
    CardContainer *pileContainer;

    QList<QSanSkillButton *> m_skillButtons;

    ResponseSkill *response_skill;
    ShowOrPindianSkill *showorpindian_skill;
    DiscardSkill *discard_skill;
    NosYijiViewAsSkill *yiji_skill;
    ChoosePlayerSkill *choose_skill;

    QList<const Player *> selected_targets;

    GuanxingBox *m_guanxingBox;

    CardChooseBox *m_cardchooseBox;

    YanjiaoBox *m_yanjiaoBox;

    PindianBox *m_pindianBox;

    ChooseGeneralBox *m_chooseGeneralBox;

    ChooseOptionsBox *m_chooseOptionsBox;

    QGraphicsPixmapItem *m_fullemotion;

    PlayerCardBox *m_playerCardBox;

    QList<CardItem *> gongxin_items;

    QMap<QString, GuhuoBox *> guhuo_items;

    ClientLogBox *log_box;
    QTextEdit *chat_box;
    QLineEdit *chat_edit;
    QGraphicsProxyWidget *chat_box_widget;
    QGraphicsProxyWidget *log_box_widget;
    QGraphicsProxyWidget *chat_edit_widget;
    QGraphicsTextItem *prompt_box_widget;
    ChatWidget *chat_widget;
    QPixmap m_rolesBoxBackground;
    QGraphicsPixmapItem *m_rolesBox;
    QGraphicsTextItem *m_pileCardNumInfoTextBox;
    QGraphicsSimpleTextItem *turn_box;

    QGraphicsPixmapItem *m_tableBg;
    QPixmap m_tableBgPixmap;
    QPixmap m_tableBgPixmapOrig;
    int m_tablew;
    int m_tableh;

    QMenu *m_add_robot_menu;

    QMap<QString, BubbleChatBox *> bubbleChatBoxes;

    // for 3v3 & 1v1 mode
    QSanSelectableItem *selector_box;
    QList<CardItem *> general_items, up_generals, down_generals;
    CardItem *to_change;
    QList<QGraphicsRectItem *> arrange_rects;
    QList<CardItem *> arrange_items;
    Button *arrange_button;
    KOFOrderBox *enemy_box, *self_box;
    QPointF m_tableCenterPos;
    ReplayerControlBar *m_replayControl;

    struct _MoveCardsClassifier
    {
        inline _MoveCardsClassifier(const CardsMoveStruct &move)
        {
            m_card_ids = move.card_ids;
        }
        inline bool operator ==(const _MoveCardsClassifier &other) const
        {
            return m_card_ids == other.m_card_ids;
        }
        inline bool operator <(const _MoveCardsClassifier &other) const
        {
            return m_card_ids.first() < other.m_card_ids.first();
        }
        QList<int> m_card_ids;
    };

    QMap<_MoveCardsClassifier, CardsMoveStruct> m_move_cache;

    // @todo: this function shouldn't be here. But it's here anyway, before someone find a better
    // home for it.
    QString _translateMovement(const CardsMoveStruct &move);

    void useCard(const Card *card);
    void fillTable(QTableWidget *table, const QList<const ClientPlayer *> &players);
    void chooseSkillButton();

    void selectTarget(int order, bool multiple);
    void selectNextTarget(bool multiple);
    void unselectAllTargets(const QGraphicsItem *except = NULL);
    void updateTargetsEnablity(const Card *card = NULL);

    void callViewAsSkill();
    void cancelViewAsSkill();

    void freeze();
    void addRestartButton(QDialog *dialog);
    QGraphicsPixmapItem *createDashboardButtons();
    void createReplayControlBar();

    void fillGenerals1v1(const QStringList &names);
    void fillGenerals3v3(const QStringList &names);

    void setChatBoxVisible(bool show);
    QRect getBubbleChatBoxShowArea(const QString &who) const;

    // animation related functions
    typedef void (RoomScene::*AnimationFunc)(const QString &, const QStringList &);
    QGraphicsObject *getAnimationObject(const QString &name) const;

    void doMovingAnimation(const QString &name, const QStringList &args);
    void doAppearingAnimation(const QString &name, const QStringList &args);
    void doLightboxAnimation(const QString &name, const QStringList &args);
    void doHuashen(const QString &name, const QStringList &args);
    void doIndicate(const QString &name, const QStringList &args);
    EffectAnimation *animations;
    bool pindian_success;

    // re-layout attempts
    bool game_started;
    void _dispersePhotos(QList<Photo *> &photos, QRectF disperseRegion, Qt::Orientation orientation, Qt::Alignment align);

    void _cancelAllFocus();
    // for miniscenes
    int _m_currentStage;

    QRectF _m_infoPlane;

    bool _m_bgEnabled;
    QString _m_bgMusicPath;

#ifndef Q_OS_WINRT
    // for animation effects
    QDeclarativeEngine *_m_animationEngine;
    QDeclarativeContext *_m_animationContext;
    QDeclarativeComponent *_m_animationComponent;
#endif

    QSet<HeroSkinContainer *> m_heroSkinContainers;

private slots:
    void fillCards(const QList<int> &card_ids, const QList<int> &disabled_ids = QList<int>());
    void askAG(const QString &player_name, const QString &reason);
    void updateSkillButtons(bool isPrepare = false);
    void acquireSkill(const ClientPlayer *player, const QString &skill_name);
    void updateSelectedTargets();
    void onSkillActivated();
    void onSkillDialogActivated();
    void onSkillDeactivated();
    void doTimeout();
    void startInXs();
    void hideAvatars();
    void changeHp(const QString &who, int delta, DamageStruct::Nature nature, bool losthp);
    void changeMaxHp(const QString &who, int delta);
    void moveFocus(const QStringList &who, QSanProtocol::Countdown);
    void setEmotion(const QString &who, const QString &emotion);
    void setFullEmotion(const QString &emotion, const int &dx, const int &dy);
    void playRoomAudio(const QString &path, bool superpose = true);
    void showSkillInvocation(const QString &who, const QString &skill_name);
    void doAnimation(int name, const QStringList &args);
    void showOwnerButtons(bool owner);
    void showPlayerCards();
    void updateRolesBox();
    void updateRoles(const QString &roles);
    void addSkillButton(const Skill *skill);
	QSanSkillButton *findSkillButtonByName(const QString &skill_name);

    void resetPiles();
    void removeLightBox();

    void showCard(const QString &player_name, QList<int> card_ids);
    void moveCardToPile(const QString &player_name, QList<int> card_ids, const QString &skill_name);
    void showDummy(const CardMoveReason &reason);
    void viewDistance();

    void speak();

    void onGameStart();
    void onGameOver();
    void onStandoff();

    void appendChatEdit(QString txt);
    void showBubbleChatBox(const QString &who, const QString &words);

    //animations
    void onEnabledChange();

    void takeAmazingGrace(ClientPlayer *taker, int card_id, bool move_cards);

    void attachSkill(const QString &skill_name);
    void detachSkill(const QString &skill_name);
    void updateSkill(const QString &skill_name);

    void startAssign();

    // 3v3 mode & 1v1 mode
    void fillGenerals(const QStringList &names);
    void takeGeneral(const QString &who, const QString &name, const QString &rule);
    void recoverGeneral(int index, const QString &name);
    void startGeneralSelection();
    void selectGeneral();
    void startArrange(const QString &to_arrange);
    void toggleArrange();
    void finishArrange();
    void changeGeneral(const QString &general);
    void revealGeneral(bool self, const QString &general);

signals:
    void restart();
    void return_to_start();
    void game_over_dialog_rejected();
};

class PromptInfoItem : public QGraphicsTextItem
{
public:
    explicit PromptInfoItem(QGraphicsItem *parent = 0);

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);

};

extern RoomScene *RoomSceneInstance;

#endif

