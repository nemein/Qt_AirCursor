// simple button for grabbing

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
