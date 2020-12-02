/********************************************************************
    Copyright (c) 2013-2015 - Mogara

    This file is part of QSanguosha-Hegemony.

    This game is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 3.0
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    See the LICENSE file for more details.

    Mogara
    *********************************************************************/

#include "yanjiaobox.h"
#include "engine.h"
#include "skin-bank.h"
#include "choosegeneraldialog.h"
#include "banpair.h"
#include "button.h"
#include "client.h"
#include "clientplayer.h"

#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsProxyWidget>

YanjiaoBox::YanjiaoBox()
    : itemCount(0), single_result(false), m_viewOnly(false),
    confirm(new Button(tr("confirm"), 0.6)), cancel(new Button(tr("cancel"), 0.6)),
    progress_bar(NULL)
{
    confirm->setEnabled(ClientInstance->getReplayer());
    confirm->setParentItem(this);
    confirm->setObjectName("confirm");
    connect(confirm, &Button::clicked, this, &YanjiaoBox::reply);

    cancel->setEnabled(ClientInstance->getReplayer());
    cancel->setParentItem(this);
    cancel->setObjectName("cancel");
    connect(cancel, &Button::clicked, this, &YanjiaoBox::reply);

}

QRectF YanjiaoBox::boundingRect() const
{
    const int card_width = G_COMMON_LAYOUT.m_cardNormalWidth;
    const int card_height = G_COMMON_LAYOUT.m_cardNormalHeight;

    int width = (card_width + cardInterval) * itemCount - cardInterval + 70+40;
    int height = card_height*3 + cardInterval*2 + 90 + 16;

    if (ServerInfo.OperationTimeout != 0)
        height += 24;

    return QRectF(0, 0, width, height);
}

void YanjiaoBox::doYanjiaoChoose(const QList<int> &cards, const QString &reason, const QString &func)
{
    if (cards.isEmpty()) {
        clear();
        return;
    }

    zhuge.clear();//self
    this->reason = reason;
    this->func = func;
    this->moverestricted = false;
    buttonstate = ClientInstance->m_isDiscardActionRefusable;
    firstItems.clear();
    upItems.clear();
    downItems.clear();

    foreach (int cardId, cards) {
        CardItem *cardItem = new CardItem(Sanguosha->getCard(cardId));
        cardItem->setAutoBack(false);
        cardItem->setFlag(QGraphicsItem::ItemIsFocusable);

        connect(cardItem, &CardItem::released, this, &YanjiaoBox::onItemReleased);

        firstItems << cardItem;
        cardItem->setParentItem(this);
        if (cardId == -1) noneoperator = true;
    }


    itemCount = firstItems.length();

    int cardWidth = G_COMMON_LAYOUT.m_cardNormalWidth;

    prepareGeometryChange();
    GraphicsBox::moveToCenter(this);
    show();

    for (int i = 0; i < firstItems.length(); i++) {
        CardItem *cardItem = firstItems.at(i);

        QPointF pos;
        int X, Y;

        X = (45 + (cardWidth + cardInterval) * i);
        Y = (45);

        pos.setX(X);
        pos.setY(Y);
        cardItem->resetTransform();
        cardItem->setOuterGlowEffectEnabled(true);
        cardItem->setPos(45, 45);
        cardItem->setHomePos(pos);
        cardItem->goBack(true, true, -1, 400);
    }

    if (this->moverestricted && !noneoperator) {
        foreach (CardItem *card, firstItems)
            card->setEnabled(true);
    }

    confirm->setPos(boundingRect().center().x() - confirm->boundingRect().width() / 2-80, boundingRect().height() - ((ServerInfo.OperationTimeout == 0) ? 40 : 60));
    confirm->show();
    confirm->setEnabled(false);

    cancel->setPos(boundingRect().center().x() - cancel->boundingRect().width() / 2+80, boundingRect().height() - ((ServerInfo.OperationTimeout == 0) ? 40 : 60));
    cancel->show();
    cancel->setEnabled(true);

    if (ServerInfo.OperationTimeout != 0) {
        if (!progress_bar) {
            progress_bar = new QSanCommandProgressBar();
            progress_bar->setMinimumWidth(200);
            progress_bar->setMaximumHeight(12);
            progress_bar->setTimerEnabled(true);
            progress_bar_item = new QGraphicsProxyWidget(this);
            progress_bar_item->setWidget(progress_bar);
            progress_bar_item->setPos(boundingRect().center().x() - progress_bar_item->boundingRect().width() / 2, boundingRect().height() - 20);
            connect(progress_bar, &QSanCommandProgressBar::timedOut, this, &YanjiaoBox::reply);
        }
        progress_bar->setCountdown(QSanProtocol::S_COMMAND_SKILL_YANJIAO);
        progress_bar->show();
    }
}

void YanjiaoBox::onItemReleased()
{
    CardItem *item = qobject_cast<CardItem *>(sender());
    if (item == NULL) return;

    if (firstItems.contains(item))
        firstItems.removeOne(item);
    if (upItems.contains(item))
        upItems.removeOne(item);
    if (downItems.contains(item))
        downItems.removeOne(item);

    const int cardWidth = G_COMMON_LAYOUT.m_cardNormalWidth;
    const int cardHeight = G_COMMON_LAYOUT.m_cardNormalHeight;

    bool toOneRow = (item->y() + cardHeight / 2 <= 45 + cardHeight);
    bool toSecondRow = (item->y() + cardHeight / 2 <= 45 + cardHeight*2 + cardInterval);

    QList<CardItem *> *to_items = toOneRow ? &firstItems : (toSecondRow ? &upItems : &downItems);

    int c = (item->x() + item->boundingRect().width() / 2 - 45) / cardWidth;
    c = qBound(0, c, to_items->length());
    to_items->insert(c, item);

    adjust();
}

void YanjiaoBox::adjust()
{
    const int cardWidth = G_COMMON_LAYOUT.m_cardNormalWidth;
    const int card_height = G_COMMON_LAYOUT.m_cardNormalHeight;


    for (int i = 0; i < firstItems.length(); i++) {
        QPointF pos;
        pos.setX(45 + (cardWidth + cardInterval) * i);
        pos.setY(45);
        firstItems.at(i)->setHomePos(pos);
        firstItems.at(i)->goBack(true, true, -1, 400);
    }

    for (int i = 0; i < upItems.length(); i++) {
        QPointF pos;
        pos.setX(45 + (cardWidth + cardInterval) * i);
        pos.setY(45 + (card_height + cardInterval));
        upItems.at(i)->setHomePos(pos);
        upItems.at(i)->goBack(true, true, -1, 400);
    }

    for (int i = 0; i < downItems.length(); i++) {
        QPointF pos;
        pos.setX(45 + (cardWidth + cardInterval) * i);
        pos.setY(45 + (card_height + cardInterval) * 2);
        downItems.at(i)->setHomePos(pos);
        downItems.at(i)->goBack(true, true, -1, 400);
    }

    int point1 = 0, point2 = 0;
    foreach (CardItem *item, upItems) {
        int id = item->getId();
        point1 += Sanguosha->getCard(id)->getNumber();
    }
    foreach (CardItem *item, downItems) {
        int id = item->getId();
        point2 += Sanguosha->getCard(id)->getNumber();
    }

    confirm->setEnabled(point1 == point2 && point1 != 0);

}

void YanjiaoBox::clear()
{
    foreach(CardItem *card_item, firstItems)
        card_item->deleteLater();
    foreach(CardItem *card_item, upItems)
        card_item->deleteLater();
    foreach(CardItem *card_item, downItems)
        card_item->deleteLater();

    firstItems.clear();
    upItems.clear();
    downItems.clear();

    if (progress_bar != NULL) {
        progress_bar->hide();
        progress_bar->deleteLater();
        progress_bar = NULL;
    }

    prepareGeometryChange();
    hide();
}

void YanjiaoBox::reply()
{
    QList<int> up_cards, down_cards;
    Button *button = qobject_cast<Button *>(sender());
    if (button && button->objectName() == "confirm") {
        foreach(CardItem *card_item, upItems)
            up_cards << card_item->getCard()->getId();

        foreach(CardItem *card_item, downItems)
            down_cards << card_item->getCard()->getId();

    }
    ClientInstance->onPlayerReplyYanjiao(up_cards, down_cards);
    clear();
}

void YanjiaoBox::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    QString title = QString("%1: %2").arg(Sanguosha->translate(reason)).arg(Sanguosha->translate("@" + reason));
    if (zhuge.isEmpty()) {
        GraphicsBox::paintGraphicsBoxStyle(painter, title, boundingRect());
    } else {
        QString playerName = ClientInstance->getPlayerName(zhuge);
        GraphicsBox::paintGraphicsBoxStyle(painter, tr("%1: %2 is Choosing").arg(Sanguosha->translate(reason)).arg(playerName), boundingRect());
    }

    const int card_width = G_COMMON_LAYOUT.m_cardNormalWidth;
    const int card_height = G_COMMON_LAYOUT.m_cardNormalHeight;

    for (int i = 0; i < itemCount; ++i) {

        int x = 45 + (card_width + cardInterval) * i;
        int y = 45;

        QRect top_rect(x, y, card_width, card_height);
        painter->drawPixmap(top_rect, G_ROOM_SKIN.getPixmap(QSanRoomSkin::S_SKIN_KEY_CHOOSE_GENERAL_BOX_DEST_SEAT));

    }

    int point1 = 0;
    foreach (CardItem *item, firstItems) {
        int id = item->getId();
        point1 += Sanguosha->getCard(id)->getNumber();
    }

    QRect point_rect1(30 + (card_width + cardInterval) * itemCount, 45, 100, card_height);
    G_COMMON_LAYOUT.graphicsBoxTitleFont.paintText(painter, point_rect1, Qt::AlignCenter, QString::number(point1));

    QString description2 = Sanguosha->translate("zhangchangpu");
    QRect up_rect(15, 45 + (card_height + cardInterval), 20, card_height);
    G_COMMON_LAYOUT.playerCardBoxPlaceNameText.paintText(painter, up_rect, Qt::AlignCenter, description2);

    for (int i = 0; i < itemCount; ++i) {

        int x = (45 + (card_width + cardInterval) * i);
        int y = (45 + (card_height + cardInterval));

        QRect bottom_rect(x, y, card_width, card_height);
        painter->drawPixmap(bottom_rect, G_ROOM_SKIN.getPixmap(QSanRoomSkin::S_SKIN_KEY_CHOOSE_GENERAL_BOX_DEST_SEAT));
    }

    int point2 = 0;
    foreach (CardItem *item, upItems) {
        int id = item->getId();
        point2 += Sanguosha->getCard(id)->getNumber();
    }

    QRect point_rect2(30 + (card_width + cardInterval) * itemCount, 45 + (card_height + cardInterval), 100, card_height);
    G_COMMON_LAYOUT.graphicsBoxTitleFont.paintText(painter, point_rect2, Qt::AlignCenter, QString::number(point2));

    QString description3 = Sanguosha->translate("self");
    QRect down_rect(15, 45 + (card_height + cardInterval) * 2, 20, card_height);
    G_COMMON_LAYOUT.playerCardBoxPlaceNameText.paintText(painter, down_rect, Qt::AlignCenter, description3);

    for (int i = 0; i < itemCount; ++i) {

        int x = (45 + (card_width + cardInterval) * i);
        int y = (45 + (card_height + cardInterval) * 2);

        QRect bottom_rect(x, y, card_width, card_height);
        painter->drawPixmap(bottom_rect, G_ROOM_SKIN.getPixmap(QSanRoomSkin::S_SKIN_KEY_CHOOSE_GENERAL_BOX_DEST_SEAT));
    }


    int point3 = 0;
    foreach (CardItem *item, downItems) {
        int id = item->getId();
        point3 += Sanguosha->getCard(id)->getNumber();
    }

    QRect point_rect3(30 + (card_width + cardInterval) * itemCount, 45 + (card_height + cardInterval) * 2, 100, card_height);
    G_COMMON_LAYOUT.graphicsBoxTitleFont.paintText(painter, point_rect3, Qt::AlignCenter, QString::number(point3));






}
