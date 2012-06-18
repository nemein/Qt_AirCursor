// main game logic class

#ifndef GAME_H
#define GAME_H

#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QTimer>
#include "item.h"

class Game : public QObject
{
    Q_OBJECT
public:
    explicit Game(QSize size, QObject *parent = 0);
    ~Game();


signals:

public slots:
    void start();

    void handCreate();
    void handDestroy();

    void handUpdate(qreal x, qreal y, qreal z);
    void grab(qreal x, qreal y);
    void grabRelease(qreal x, qreal y);

    void pointIncrease();
    void gameOver();

private slots:
    void speedIncrease();
    void itemSpawn();

private:

    QGraphicsTextItem* addTextItem(QString text, QPoint position, QColor color, quint16 fontSize);
    void mapKinectToScene(qreal& x, qreal& y);

    QSize m_size;

    QGraphicsScene* m_gameScene;
    QGraphicsScene* m_menuScene;
    QGraphicsView* m_view;

    QPixmap* m_mousePixmap;
    QPixmap* m_joystickPixmap;
    QPixmap* m_tabletPixmap;
    QPixmap* m_trashbinPixmap;

    QPixmap* m_cursorOpenPixmap;
    QPixmap* m_cursorClosedPixmap;

    QGraphicsPixmapItem* m_cursor;
    QPointF m_lastCursorPos;

    bool m_gameInProgress;

    QTimer m_gameTimer;
    QTimer m_itemSpawnTimer;
    QTimer m_speedIncreaseTimer;

    quint16 m_itemSpawnInterval;
    qreal m_currentSpeed;
    Item* m_grabbedItem;

    quint16 m_points;

    QGraphicsTextItem* m_pointsText;
    QGraphicsTextItem* m_kinectStatusText;

};

#endif // GAME_H
