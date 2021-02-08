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

#ifndef _YANJIAO_BOX_H
#define _YANJIAO_BOX_H

#include "carditem.h"
#include "timed-progressbar.h"
#include "graphicsbox.h"

class Button;
class QGraphicsDropShadowEffect;

class YanjiaoBox : public GraphicsBox
{
    Q_OBJECT

public:
    explicit YanjiaoBox();

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    QRectF boundingRect() const;
    void clear();

public slots:
    void doYanjiaoChoose(const QList<int> &cards, const QString &reason, const QString &pattern);
    void reply();

private:
    QList<CardItem *> firstItems, upItems, downItems;
    QString reason;
    QString func;
    bool moverestricted;
    bool buttonstate;
    bool noneoperator;
    QString zhuge;

    int itemCount;

    static const int cardInterval = 3;



    bool single_result;
    bool m_viewOnly;

    static const int top_dark_bar = 27;
    static const int top_blank_width = 42;
    static const int bottom_blank_width = 68;
    static const int card_bottom_to_split_line = 23;
    static const int card_to_center_line = 5;
    static const int lord_to_card_center_line = 20;
    static const int left_blank_width = 37;
    static const int split_line_to_card_seat = 15;


    //data index
    static const int S_DATA_INITIAL_HOME_POS = 9527;

    Button *confirm, *cancel;
    QGraphicsProxyWidget *progress_bar_item;
    QSanCommandProgressBar *progress_bar;

    void adjust();

private slots:
    void onItemReleased();
};

#endif // _YANJIAO_BOX_H
