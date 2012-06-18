#include "button.h"
#include <QPainter>

Button::Button(QString text, QPoint position, QPointF size, QGraphicsObject *parent) :
    QGraphicsObject(parent),
    m_text(text)
{
    m_size = size;
    setZValue(2000);
    setPos(position);
}

Button::~Button()
{

}

QRectF Button::boundingRect() const
{
    return QRectF(-m_size.x() / 2.0, -m_size.y() / 2.0, m_size.x(), m_size.y());
}

QPainterPath Button::shape() const
{
    QPainterPath path;
    path.addRect(-m_size.x() / 2.0, -m_size.y() / 2.0, m_size.x(), m_size.y());
    return path;
}

void Button::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                   QWidget *widget)
{
    // draw a very cool shadow first
    painter->setBrush(QBrush(Qt::darkGray));
    QRectF rect = boundingRect();
    painter->drawRoundedRect(rect, 10, 10);

    rect.moveCenter(QPoint(-7, -7));
    painter->setBrush(QBrush(Qt::gray));
    painter->drawRoundedRect(rect, 10, 10);

    // add text to the center
    QFont font("Courier", 36, QFont::Bold, true);
    painter->setFont(font);
    QFontMetrics fm(font);
    int width = fm.width(m_text);
    painter->drawText(- width / 2, 0, m_text);
}

void Button::activate()
{
    emit activated();
}
