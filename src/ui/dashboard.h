#ifndef _DASHBOARD_H
#define _DASHBOARD_H

#include "qsan-selectable-item.h"
#include "qsanbutton.h"
#include "carditem.h"
#include "player.h"
#include "skill.h"
#include "protocol.h"
#include "timed-progressbar.h"
#include "generic-cardcontainer-ui.h"
#include "pixmapanimation.h"
#include "sprite.h"
#include "util.h"

#include <QPushButton>
#include <QComboBox>
#include <QGraphicsLinearLayout>
#include <QLineEdit>
#include <QMutex>
#include <QPropertyAnimation>
#include <QGraphicsProxyWidget>

class HeroSkinContainer;
class GraphicsPixmapHoverItem;

class Dashboard : public PlayerCardContainer
{
    Q_OBJECT
    Q_ENUMS(SortType)

public:
    enum SortType
    {
        ByType, BySuit, ByNumber
    };

    Dashboard(QGraphicsPixmapItem *button_widget);
    virtual QRectF boundingRect() const;
    void setWidth(int width);
    int getMiddleWidth();
    inline QRectF getAvatarArea()
    {
        QRectF rect;
        rect.setSize(_dlayout->m_avatarArea.size());
        QPointF topLeft = mapFromItem(_getAvatarParent(), _dlayout->m_avatarArea.topLeft());
        rect.moveTopLeft(topLeft);
        return rect;
    }

    void hideControlButtons();
    void showControlButtons();
    virtual void showProgressBar(QSanProtocol::Countdown countdown, int max = 0);

    QRectF getProgressBarSceneBoundingRect() const {
        return _m_progressBarItem->sceneBoundingRect();
    }

    QSanSkillButton *removeSkillButton(const QString &skillName);
    QSanSkillButton *addSkillButton(const QString &skillName);
    bool isAvatarUnderMouse();

    void highlightEquip(QString skillName, bool hightlight);
    void clearHighlightEquip();

    void setTrust(bool trust);
    virtual void killPlayer();
    virtual void revivePlayer();
    void selectCard(const QString &pattern, bool forward = true, bool multiple = false);
    void selectEquip(int position);
    void selectOnlyCard(bool need_only = false);
    void useSelected();
    const Card *getSelected() const;
    void unselectAll(const CardItem *except = NULL, bool update_skill = true);
    void hideAvatar();

    void disableAllCards();
    void enableCards();
    void enableAllCards();

    void adjustCards(bool playAnimation = true);
    void adjustCards(QList<CardItem *> &cards, bool playAnimation = true, int animation_type = -1);

    virtual QGraphicsItem *getMouseClickReceiver();

    QList<CardItem *> removeCardItems(const QList<int> &card_ids, const CardsMoveStruct &moveInfo);
    virtual QList<CardItem *> cloneCardItems(QList<int> card_ids);

    // pending operations
    void startPending(const ViewAsSkill *skill);
    void stopPending();
    void updatePending();
    void clearPendings();

    void externalPending(CardItem *item);

    inline void addPending(CardItem *item)
    {
        pendings << item;
    }
    inline QList<CardItem *> getPendings() const
    {
        return pendings;
    }
    inline bool hasHandCard(CardItem *item) const
    {
        return m_handCards.contains(item);
    }

    const ViewAsSkill *currentSkill() const;
    const Card *pendingCard() const;

    void expandGuhuoCards(const QString &guhuo_type);
    void retractGuhuoCards();
    void expandPileCards(const QString &pile_name, bool prepend = true);
    void retractPileCards(const QString &pile_name);
    void updateHandPile(const QString &pile_name, bool add, QList<int> card_ids);
    void retractAllSkillPileCards();
    inline QStringList getPileExpanded() const 
    { 
        return _m_pile_expanded.keys(); 
    } 

    void selectCard(CardItem *item, bool isSelected);

    int getButtonWidgetWidth() const;
    int getTextureWidth() const;

    int width();
    int height();

    virtual void repaintAll();
    int middleFrameAndRightFrameHeightDiff() const {
        return m_middleFrameAndRightFrameHeightDiff;
    }
    void showNullificationButton();
    void hideNullificationButton();

    static const int S_PENDING_OFFSET_Y = -25;

    inline void updateSkillButton()
    {
        if (_m_skillDock)
            _m_skillDock->update();
    }

    void updateSkillButton(const QString &skillName);

    void showSeat();

    inline QRectF getAvatarAreaSceneBoundingRect() const
    {
        return _m_rightFrame->sceneBoundingRect();
    }

public slots:
    virtual void updateAvatar();

    void sortCards();
    void beginSorting();
    void reverseSelection();
    void cancelNullification();
    void skillButtonActivated();
    void skillButtonDeactivated();
    void selectAll();
    void controlNullificationButton(bool show);

protected:
    void _createExtraButtons();
    virtual void _adjustComponentZValues(bool killed = false);
    virtual void addHandCards(QList<CardItem *> &cards);
    virtual QList<CardItem *> removeHandCards(const QList<int> &cardIds);

    // initialization of _m_layout is compulsory for children classes.
    inline virtual QGraphicsItem *_getEquipParent()
    {
        return _m_leftFrame;
    }
    inline virtual QGraphicsItem *_getDelayedTrickParent()
    {
        return _m_leftFrame;
    }
    inline virtual QGraphicsItem *_getAvatarParent()
    {
        return _m_rightFrame;
    }
    inline virtual QGraphicsItem *_getMarkParent()
    {
        return _m_floatingArea;
    }
    inline virtual QGraphicsItem *_getPhaseParent()
    {
        return _m_floatingArea;
    }
    inline virtual QGraphicsItem *_getRoleComboBoxParent()
    {
        return _m_rightFrame;
    }
    inline virtual QGraphicsItem *_getPileParent()
    {
        return _m_rightFrame;
    }
    inline virtual QGraphicsItem *_getProgressBarParent()
    {
        return _m_floatingArea;
    }
    inline virtual QGraphicsItem *_getFocusFrameParent()
    {
        return _m_rightFrame;
    }
    inline virtual QGraphicsItem *_getDeathIconParent()
    {
        return _m_middleFrame;
    }
    inline virtual QString getResourceKeyName()
    {
        return QSanRoomSkin::S_SKIN_KEY_DASHBOARD;
    }

    bool _addCardItems(QList<CardItem *> &card_items, const CardsMoveStruct &moveInfo);
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent);
    void _addHandCard(CardItem *card_item, int index = 0, const QString &footnote = QString());
    void _adjustCards();
    void _adjustCards(const QList<CardItem *> &list, int y);

    int _m_width;
    // sync objects
    QMutex m_mutex;
    QMutex m_mutexEnableCards;

    QSanButton *m_trustButton;
    QSanButton *m_btnReverseSelection;
    QSanButton *m_btnSortHandcard;
    QSanButton *m_btnNoNullification;
    QGraphicsPixmapItem *_m_leftFrame, *_m_middleFrame, *_m_rightFrame, *_m_flowerFrame, *_m_treeFrame;
    // we can not draw bg directly _m_rightFrame because then it will always be
    // under avatar (since it's avatar's parent).
    //QGraphicsPixmapItem *_m_rightFrameBg;
    QGraphicsPixmapItem *button_widget;

    CardItem *selected;
    QList<CardItem *> m_handCards, m_leftCards, m_middleCards, m_rightCards;

    QGraphicsPathItem *trusting_item;
    QGraphicsSimpleTextItem *trusting_text;

    QSanInvokeSkillDock* _m_skillDock;
    const QSanRoomSkin::DashboardLayout *_dlayout;

    //for animated effects
    EffectAnimation *animations;

    // for parts creation
    void _createLeft();
    void _createRight();
    void _createMiddle();
    void _updateFrames();

    void _paintLeftFrame();
    void _paintMiddleFrame(const QRect &rect);
    void _paintRightFrame();
    // for pendings
    QList<CardItem *> pendings;
    const Card *pending_card;
    const ViewAsSkill *view_as_skill;
    const FilterSkill *filter;
    QMap<QString, QList<int> > _m_pile_expanded;
    QList<CardItem *> _m_guhuo_expanded;

    // for equip skill/selections
    PixmapAnimation *_m_equipBorders[S_EQUIP_AREA_LENGTH];
    QSanSkillButton *_m_equipSkillBtns[S_EQUIP_AREA_LENGTH];
    bool _m_isEquipsAnimOn[S_EQUIP_AREA_LENGTH];

    void _createEquipBorderAnimations();
    void _setEquipBorderAnimation(int index, bool turnOn);

    void drawEquip(QPainter *painter, const CardItem *equip, int order);
    void setSelectedItem(CardItem *card_item);

    QMenu *_m_sort_menu;

    QSanButton *m_changeHeadHeroSkinButton;
    QSanButton *m_changeDeputyHeroSkinButton;
    HeroSkinContainer *m_headHeroSkinContainer;
    HeroSkinContainer *m_deputyHeroSkinContainer;

    int m_middleFrameAndRightFrameHeightDiff;

private:
    void showHeroSkinListHelper(const General *general, HeroSkinContainer * &heroSkinContainer);

    QPointF getHeroSkinContainerPosition() const;

protected slots:
    virtual void _onEquipSelectChanged();

private slots:
    void onCardItemClicked();
    void onCardItemDoubleClicked();
    void onCardItemThrown();
    void onCardItemHover();
    void onCardItemLeaveHover();
    void onMarkChanged();

    void updateTrustButton();

    void showHeroSkinList();
    void heroSkinButtonMouseOutsideClicked();

    void onAvatarHoverEnter();
    void onAvatarHoverLeave();
    void onSkinChangingStart();
    void onSkinChangingFinished();

signals:
    void card_selected(const Card *card);
    void card_to_use();
    void progressBarTimedOut();
};

#endif

