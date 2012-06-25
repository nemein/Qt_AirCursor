/*
    Air Cursor library for Qt applications using Kinect
    Copyright (C) 2012 Tuomas Haapala, Nemein

    ---

    Part of the game example.

    This class implements the simple yet gorgeous grabbable buttons.
*/

#ifndef BUTTON_H
#define BUTTON_H

#include <QGraphicsObject>

class Button : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit Button(QString text, QPoint position, QPointF size, QGraphicsObject *parent = 0);
    ~Button();

    QRectF boundingRect() const;
    QPainterPath shape() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
            QWidget *widget);
    void activate();

signals:
    void activated();

    
public slots:

private:
    QString m_text;
    QPointF m_size;

    
};

#endif // BUTTON_H
