// class representing grabbable items

#ifndef ITEM_H
#define ITEM_H

#include <QGraphicsObject>
#include <QPainter>

class Item : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit Item(QPixmap* pixmap, QPointF size, QGraphicsObject *parent = 0);
    ~Item();

    void setSpeed(qreal speed);
    void setVelocity(QPointF velocity);
    void setTrashBoundary(qreal trashBoundary);

    QPointF getSize();

    void grab();
    void grabRelease();

    QRectF boundingRect() const;
    QPainterPath shape() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
            QWidget *widget);

signals:
    void wentToTrash();
    void escape();

protected:
    void advance(int step);

private:
    QPixmap* m_pixmap;
    QPointF m_size;

    qreal m_zCoord;
    qreal m_speed;
    QPointF m_velocity;
    qreal m_zVelocity;
    qreal m_targetAngle;
    bool m_onGround;
    bool m_grabbed;

    qreal m_trashBoundary;
};

#endif // ITEM_H
