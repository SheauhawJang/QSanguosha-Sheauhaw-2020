#ifndef _GENERAL_CARD_CONTAINER_UI_H
#define _GENERAL_CARD_CONTAINER_UI_H

#include "carditem.h"
#include "player.h"
#include "qsan-selectable-item.h"
#include "skin-bank.h"
#include "timed-progressbar.h"
#include "magatamas-item.h"
#include "rolecombobox.h"

#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QMutex>

#include <qparallelanimationgroup.h>
#include <qgraphicseffect.h>
#include <qvariant.h>
#include <qlabel.h>

class GraphicsPixmapHoverItem;

class GenericCardContainer : public QGraphicsObject
{
    Q_OBJECT

public:
    inline GenericCardContainer()
    {
        _m_highestZ = 10000;
    }
    virtual QList<CardItem *> removeCardItems(const QList<int> &card_ids, const CardsMoveStruct &moveInfo) = 0;
    virtual void addCardItems(QList<CardItem *> &card_items, const CardsMoveStruct &moveInfo);
    virtual QList<CardItem *> cloneCardItems(QList<int> card_ids);

protected:
    // @return Whether the card items should be destroyed after animation
    virtual bool _addCardItems(QList<CardItem *> &card_items, const CardsMoveStruct &moveInfo) = 0;
    QList<CardItem *> _createCards(QList<int> card_ids);
    CardItem *_createCard(int card_id);
    void _disperseCards(QList<CardItem *> &cards, QRectF fillRegion, Qt::Alignment align, bool useHomePos, bool keepOrder, int index = -1);
    void _disperseCards(QList<CardItem *> &cards, QPointF center, bool useHomePos);
    void _playMoveCardsAnimation(QList<CardItem *> &cards, bool destroyCards, bool smoothTransition, int duration);
    int _m_highestZ;

protected slots:
    virtual void onAnimationFinished();

private slots:
    void _doUpdate();
    void _destroyCard();

private:
    static bool _horizontalPosLessThan(const CardItem *card1, const CardItem *card2);

signals:
    void animation_finished();
};

class PlayerCardContainer : public GenericCardContainer
{
    Q_OBJECT

public:
    PlayerCardContainer();
    virtual void showProgressBar(QSanProtocol::Countdown countdown, int max = 0);
    void hideProgressBar();
    void hideAvatars();
    const ClientPlayer *getPlayer() const;
    void setPlayer(ClientPlayer *player);
    inline int getVotes()
    {
        return _m_votesGot;
    }
    inline void setMaxVotes(int maxVotes)
    {
        _m_maxVotes = maxVotes;
    }
    // See _m_floatingArea for more information
    inline QRect getFloatingArea() const
    {
        return _m_floatingAreaRect;
    }
    inline void setSaveMeIcon(bool visible)
    {
        _m_saveMeIcon->setVisible(visible);
    }
    void setFloatingArea(QRect rect);

    // repaintAll is different from refresh in that it recreates all controls and is
    // very costly. Avoid calling this except for changing skins or only once during
    // the initialization. If you just want to update the information displayed, call
    // refresh instead.
    virtual void repaintAll();
    virtual void killPlayer();
    virtual void revivePlayer();
    virtual QGraphicsItem *getMouseClickReceiver() = 0;
    virtual void startHuaShen(QString generalName, QString skillName);
    virtual void stopHuaShen();
    virtual void updateAvatarTooltip();

    inline void hookMouseEvents();

    QPixmap paintByMask(QPixmap& source);

    bool canBeSelected();

    void stopHeroSkinChangingAnimation();

public slots:
    virtual void updateAvatar();
    void updateSmallAvatar();
    void updatePhase();
    void updateHp();
    void updateHandcardNum();
    void updateDrankState();
    virtual void updateDuanchang();
	void updatePile(const QString &pile_name);
    void updateCount(const QString &pile_name, int value);
	void updateTip(const QString &tag_name, bool add_in);
    void updateRole(const QString &role);

    virtual void sealEquipArea(const QString &area_name);
    virtual void unsealEquipArea(const QString &area_name);

    virtual void sealJudgeArea();
    virtual void unsealJudgeArea();

    void updateMarks();
    void updateVotes(bool need_select = true, bool display_1 = false);
    void updateReformState();
    void showDistance();
    virtual void showSeat();
    virtual void showPile();
    virtual void hidePile();
    virtual void refresh(bool killed = false);

    QPixmap getHeadAvatarIcon(const QString &generalName);
    QPixmap getDeputyAvatarIcon(const QString &generalName);

    inline GraphicsPixmapHoverItem *getHeadAvartarItem() const
    {
        return _m_avatarIcon;
    }
    inline GraphicsPixmapHoverItem *getDeputyAvartarItem() const
    {
        return _m_smallAvatarIcon;
    }

    static void _paintPixmap(QGraphicsPixmapItem *&item, const QRect &rect, const QPixmap &pixmap, QGraphicsItem *parent);

protected:
    // overrider parent functions
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    // initialization of _m_layout is compulsory for children classes.
    virtual QGraphicsItem *_getEquipParent() = 0;
    virtual QGraphicsItem *_getDelayedTrickParent() = 0;
    virtual QGraphicsItem *_getAvatarParent() = 0;
    virtual QGraphicsItem *_getMarkParent() = 0;
    virtual QGraphicsItem *_getPhaseParent() = 0;
    virtual QGraphicsItem *_getRoleComboBoxParent() = 0;
    virtual QGraphicsItem *_getPileParent() = 0;
    virtual QGraphicsItem *_getFocusFrameParent() = 0;
    virtual QGraphicsItem *_getProgressBarParent() = 0;
    virtual QGraphicsItem *_getDeathIconParent() = 0;
    virtual QString getResourceKeyName() = 0;

    void _createRoleComboBox();
    void _updateProgressBar(); // a dirty function used by the class itself only.
    void _updateDeathIcon();
    void _updateEquips();
    void _updateHorses();
    void _paintPixmap(QGraphicsPixmapItem *&item, const QRect &rect, const QString &key);
    void _paintPixmap(QGraphicsPixmapItem *&item, const QRect &rect, const QString &key, QGraphicsItem *parent);
    void _paintPixmap(QGraphicsPixmapItem *&item, const QRect &rect, const QPixmap &pixmap);
    void _clearPixmap(QGraphicsPixmapItem *item);
    QPixmap _getPixmap(const QString &key);
    QPixmap _getPixmap(const QString &key, const QString &arg);
    QPixmap _getEquipPixmap(const EquipCard *equip, bool has_treasure);
    QPixmap _getSealEquipPixmap(const QString &area_name, bool has_treasure);
    virtual void _adjustComponentZValues(bool killed = false);
    void _updateFloatingArea();
    // We use QList of cards instead of a single card as parameter here, just in case
    // we need to do group animation in the future.
    virtual void addEquips(QList<CardItem *> &equips);
    virtual QList<CardItem *> removeEquips(const QList<int> &cardIds);
    virtual void addDelayedTricks(QList<CardItem *> &judges);
    virtual QList<CardItem *> removeDelayedTricks(const QList<int> &cardIds);
    virtual void updateDelayedTricks();

    // This is a dirty but easy design, we require children class to call create controls after
    // everything specific to the children has been setup (such as the frames that we attach
    // the controls. Consider revise this in the future.
    void _createControls();
    void _layBetween(QGraphicsItem *middle, QGraphicsItem *item1, QGraphicsItem *item2);
    void _layUnder(QGraphicsItem *item);

    // layout
    const QSanRoomSkin::PlayerCardContainerLayout *_m_layout;
    QGraphicsRectItem *_m_avatarArea, *_m_smallAvatarArea;

    // icons;
    // painting large shadowtext every frame is very costly, so we use a
    // graphicsitem to cache the result
    QGraphicsPixmapItem *_m_avatarNameItem, *_m_smallAvatarNameItem;
    GraphicsPixmapHoverItem *_m_avatarIcon, *_m_smallAvatarIcon;
    QGraphicsPixmapItem *_m_backgroundFrame, *_m_circleItem;
    QGraphicsPixmapItem *_m_screenNameItem;
    QGraphicsPixmapItem *_m_chainIcon, *_m_faceTurnedIcon;
    QGraphicsPixmapItem *_m_handCardBg, *_m_handCardNumText;
    QGraphicsPixmapItem *_m_kingdomColorMaskIcon;
    QGraphicsPixmapItem *_m_deathIcon;
    QGraphicsPixmapItem *_m_actionIcon;
    QGraphicsPixmapItem *_m_kingdomIcon;
    QGraphicsPixmapItem *_m_saveMeIcon;
    QGraphicsPixmapItem *_m_phaseIcon;
    QGraphicsPixmapItem *_m_extraSkillText;
	QGraphicsTextItem *_m_markItem;
    QGraphicsPixmapItem *_m_selectedFrame;
    QMap<QString, QGraphicsProxyWidget *> _m_privatePiles;

    // The frame that is maintained by roomscene. Items in this area has positions
    // or contents that cannot be decided based on the information of PlayerCardContainer
    // alone. It is relative to other components in the roomscene. One use case is
    // phase area of dashboard;
    QRect _m_floatingAreaRect;
    QGraphicsPixmapItem *_m_floatingArea;

    QList<QGraphicsPixmapItem *> _m_judgeIcons;
    QList<CardItem *> _m_judgeCards;

    QGraphicsProxyWidget *_m_equipRegions[S_EQUIP_AREA_LENGTH];
    CardItem *_m_equipCards[S_EQUIP_AREA_LENGTH];
    QLabel *_m_equipLabel[S_EQUIP_AREA_LENGTH];
    QParallelAnimationGroup *_m_equipAnim[S_EQUIP_AREA_LENGTH];
    QMutex _mutexEquipAnim;

    // controls
    MagatamasBoxItem *_m_hpBox;
    RoleComboBox *_m_roleComboBox;
    QSanCommandProgressBar *_m_progressBar;
    QGraphicsProxyWidget *_m_progressBarItem;

    // in order to apply different graphics effect;
    QGraphicsPixmapItem *_m_groupMain;
    QGraphicsPixmapItem *_m_groupDeath;

    // now, logic
    ClientPlayer *m_player;

    // The following stuffs for mulitple votes required for yeyan
    int _m_votesGot, _m_maxVotes;
    QGraphicsPixmapItem *_m_votesItem;

    // The following stuffs for showing distance
    QGraphicsPixmapItem *_m_distanceItem;

    QGraphicsPixmapItem *_m_seatItem;

    // animations
    QAbstractAnimation *_m_huashenAnimation;
    QGraphicsItem *_m_huashenItem;
    QString _m_huashenGeneralName;
    QString _m_huashenSkillName;

protected slots:
    virtual void _onEquipSelectChanged();

private:
    bool _startLaying();
    void clearVotes();
    int _lastZ;
    bool _allZAdjusted;

signals:
    void selected_changed();
    void enable_changed();
    void add_equip_skill(const Skill *skill, bool from_left);
    void remove_equip_skill(const QString &skill_name);
};

#endif

