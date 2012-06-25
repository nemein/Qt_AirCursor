/*
    Air Cursor library for Qt applications using Kinect
    Copyright (C) 2012 Tuomas Haapala, Nemein

    ---

    Part of the debug view example.

    A simple view widget.
    Draws given QImage and QStrings on the screen.
*/

#include "debugview.h"
#include <QPainter>

DebugView::DebugView(QWidget *parent)
    : QWidget(parent)
{
    // with aircursor the size should always be 640x480
    setFixedSize(640, 480);

    m_strings.push_back(QString("Wave your hand to start"));
}

DebugView::~DebugView()
{   
}

void DebugView::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    // first draw the actual image
    painter.drawImage(0, 0, m_image);

    // draw debug strings starting from bottom
    int y = 480 - 40;
    for (unsigned int i = 0; i < m_strings.size(); i++)
    {
        QPainterPath path;
        QFont font("arial", 30);
        font.setBold(true);
        path.addText(10, y, font, m_strings.at(i));

        // text outline
        painter.setPen(QPen(Qt::darkBlue, 2));

        painter.setBrush(QBrush(Qt::blue));
        painter.drawPath(path);
        y -= 40;
    }
}

void DebugView::debugUpdate(QImage image, QList<QString> strings)
{  
    m_image = image;
    m_strings = strings;
    update();
}
