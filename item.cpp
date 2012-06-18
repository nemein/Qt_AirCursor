#include "item.h"
#include <QDebug>
#include <QVector>
#include <QGraphicsScene>

Item::Item(QPixmap* pixmap, QPointF size, QGraphicsObject *parent) :
    QGraphicsObject(parent),
    m_pixmap(pixmap),
    m_size(size),
    m_zCoord(0.0),
    m_speed(0.0),
    m_velocity(QPointF(0.0, 0.0)),
    m_zVelocity(0.0),
    m_targetAngle(0.0),
    m_onGround(false),
    m_grabbed(false),
    m_trashBoundary(0.0)
{

}

Item::~Item()
{

}

void Item::setSpeed(qreal speed)
{
    m_speed = speed;
}

void Item::setTrashBoundary(qreal trashBoundary)
{
    m_trashBoundary = trashBoundary;
}

QPointF Item::getSize()
{
    return m_size;
}

QRectF Item::boundingRect() const
{
    // make bounding rect bigger than object so that it is easier to grab
    return QRectF((-m_size.x() / 2.0) * 2.0, (-m_size.y() / 2.0) * 2.0, m_size.x() * 2.0, m_size.y() * 2.0);
}

QPainterPath Item::shape() const
{
    QPainterPath path;
    path.addRect((-m_size.x() / 2.0) * 2.0, (-m_size.y() / 2.0) * 2.0, m_size.x() * 2.0, m_size.y() * 2.0);
    return path;
}
void Item::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
        QWidget* widget)
{
    qreal zCoeff = 5.0 / (5.0 - m_zCoord); //coeff = 1.0f;

    QRect target(-m_size.x() / 2 * zCoeff, -m_size.y() / 2 * zCoeff, m_size.x() * zCoeff, m_size.y() * zCoeff);
    painter->drawPixmap(target, *m_pixmap);

}

void Item::advance(int step)
{
    if (!step) return;

    if (m_grabbed)
    {
        // item is grabbed, struggle a bit
        setRotation(rotation() + (qreal)qrand()/(qreal)RAND_MAX * 20 - 10);
        return;
    }

    // update z coordinate
    if (!m_onGround && !m_grabbed)
    {
        m_zCoord += m_zVelocity;

        if (m_zCoord > 0.1f)
        {
            // item is in the air
            m_zVelocity -= 0.05;

            // pseudo air resistance
            m_zVelocity = m_zVelocity * 0.90;
        }
        else
        {
            // item hits ground
            if (qAbs(m_zVelocity) > 0.2)
            {
                // enough speed to bounce
                m_zVelocity = -m_zVelocity;
            }
            else
            {
                // item stays at ground
                m_zCoord = 0.0;
                m_zVelocity = 0.0;
                m_onGround = true;
                setZValue(0.0f);

            }
        }
    }

    // turn toward target angle
    qreal angleDiff = m_targetAngle - rotation();
    qreal angleStep = m_speed * 0.5;

    if (angleDiff < 0) angleStep = -angleStep;
    if (abs(angleDiff) > angleStep)
    {
        setRotation(rotation() + angleStep);
    }
    else
    {
        // target angle reached, create a new one
        setRotation(m_targetAngle);
        m_targetAngle = ((qreal)qrand()/(qreal)RAND_MAX) * 120.0 - 60.0;

    }

    // check if item hits trashbin
    if (pos().x() < scene()->sceneRect().left() + m_trashBoundary)
    {
        if (m_zCoord < 0.1)
        {
           // item hits trash boundary too slow, bounce it back
           m_targetAngle = ((qreal)qrand()/(qreal)RAND_MAX) * 60 + 45;
           m_velocity.setX(-m_velocity.x());
        }
        else
        {

           emit wentToTrash();
           deleteLater();
        }
    }
    else if (pos().x() > scene()->sceneRect().right() - m_trashBoundary)
    {
        if (m_zCoord < 0.1)
        {
           // item hits trash boundary too slow, bounce it back
           m_targetAngle = ((qreal)qrand()/(qreal)RAND_MAX) * 60 - 45;
           m_velocity.setX(-m_velocity.x());
        }
        else
        {
           emit wentToTrash();
           deleteLater();
        }
    }

    // go forward
    if (m_onGround) setPos(mapToParent(0, -m_speed));

    // check if item is past upper boundary of the screen
    if (pos().y() < scene()->sceneRect().top())
    {
        emit escape();
        deleteLater();
    }
}

void Item::grab()
{
    m_zCoord = 2.0;
    m_grabbed = true;
    m_onGround = false;
    setZValue(1.0f);
}

void Item::grabRelease()
{
    m_grabbed = false;
}

void Item::setVelocity(QPointF velocity)
{
    m_velocity = velocity;
}


