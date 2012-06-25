/*
    Air Cursor library for Qt applications using Kinect
    Copyright (C) 2012 Tuomas Haapala, Nemein

    ---

    Part of the game example.

    This class handles the main game functionality.
*/

#include "game.h"
#include "button.h"

#include <QCoreApplication>
#include <QTime>

const quint16 FPS = 30;

// determine how much hand needs to move in mm to reach screen boundaries
const qreal KINECT_X_RANGE = 250;
const qreal KINECT_Y_RANGE = 150;

// warning boundaries
const quint16 KINECT_TOO_NEAR = 750;
const quint16 KINECT_TOO_FAR = 1750;

// button size in percentages of screen size
const qreal BUTTON_X_SIZE = 0.20;
const qreal BUTTON_Y_SIZE = 0.30;

// item sizes relative to screen height
const qreal MOUSE_SIZE = 0.20;
const qreal JOYSTICK_SIZE = 0.30;
const qreal TABLET_SIZE = 0.35;

// values determining game speed increase
const qint16 START_ITEM_SPAWN_INTERVAL = 3000;
const qint16 ITEM_SPAWN_INTERVAL_STEP = 150;
const qreal START_SPEED = 2.0;
const qreal SPEED_STEP = 0.25;

// size of trashbin area, percent of screen width
const qreal TRASH_BOUNDARY = 0.10;

const QString KINECT_NOT_IN_SESSION_TEXT = "Wave your hand to start Kinect controlling.";
const QString KINECT_IN_SESSION_TEXT = "Kinect tracking.";
const QString KINECT_TOO_NEAR_TEXT = "Hand too near, please move it back a bit.";
const QString KINECT_TOO_FAR_TEXT = "Hand too far, please move it forwards a bit.";

Game::Game(QSize size, QObject *parent) :
    QObject(parent),
    m_size(size),
    m_gameScene(0),
    m_menuScene(0),
    m_view(0),
    m_gameInProgress(0),
    m_itemSpawnInterval(START_ITEM_SPAWN_INTERVAL),
    m_currentSpeed(START_SPEED),
    m_points(0),
    m_grabbedItem(0)
{
    m_mousePixmap = new QPixmap("img/mouse.png");
    m_joystickPixmap = new QPixmap("img/joystick.png");
    m_tabletPixmap = new QPixmap("img/tablet.png");
    m_trashbinPixmap = new QPixmap("img/trashbin.png");

    qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));

    // create game scene
    m_gameScene = new QGraphicsScene(this);
    m_gameScene->setSceneRect(-m_size.width() / 2, -m_size.height() / 2, m_size.width(), m_size.height());

    // create view
    m_view = new QGraphicsView(m_gameScene);
    m_view->setBackgroundBrush(QBrush(Qt::black));
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setCacheMode(QGraphicsView::CacheBackground);
    m_view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    m_view->showFullScreen();
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->show();

    // connect timers
    QObject::connect(&m_speedIncreaseTimer, SIGNAL(timeout()), this, SLOT(speedIncrease()));
    QObject::connect(&m_itemSpawnTimer, SIGNAL(timeout()), this, SLOT(itemSpawn()));
    QObject::connect(&m_gameTimer, SIGNAL(timeout()), m_gameScene, SLOT(advance()));

    // texts at the beginning
    QString str("GRAB TO THE FUTURE\n\nGrab ancient control devices and throw them in trash bin before they escape and degenerate the world!");

    // shadowed text done stupidly
    addTextItem(str, QPoint(0, -m_size.height() / 2 + 2), QColor(Qt::darkRed), 32);
    addTextItem(str, QPoint(2, -m_size.height() / 2 + 4), QColor(Qt::red), 32);

    // create cursor
    m_cursorOpenPixmap = new QPixmap("img/hand_cursor_open.png");
    m_cursorClosedPixmap = new QPixmap("img/hand_cursor_closed.png");
    m_cursor = new QGraphicsPixmapItem(*m_cursorOpenPixmap);
    m_cursor->setOffset(-m_cursor->boundingRect().width() / 2, -m_cursor->boundingRect().height() / 2);
    m_cursor->setScale(0.25);
    m_cursor->setVisible(false);
    m_cursor->setZValue(3000);
    m_gameScene->addItem(m_cursor);

    // create start screen buttons
    Button* button = new Button("START", QPoint(-m_size.width() / 4, m_size.height() / 8), QPointF(BUTTON_X_SIZE * m_size.width(), BUTTON_Y_SIZE * m_size.height()));
    QObject::connect(button, SIGNAL(activated()), this, SLOT(start()));
    m_gameScene->addItem(button);
    button = new Button("QUIT", QPoint(m_size.width() / 4, m_size.height() / 8), QPointF(BUTTON_X_SIZE * m_size.width(), BUTTON_Y_SIZE * m_size.height()));
    QObject::connect(button, SIGNAL(activated()), QCoreApplication::instance(), SLOT(quit()));
    m_gameScene->addItem(button);

    // create kinect status text
    m_kinectStatusText = addTextItem(KINECT_NOT_IN_SESSION_TEXT, QPoint(0, m_size.height() / 2 - 50), QColor(Qt::red), 16);
}

Game::~Game()
{
    delete m_mousePixmap;
    delete m_joystickPixmap;
    delete m_trashbinPixmap;
    delete m_cursorOpenPixmap;
    delete m_cursorClosedPixmap;
}

void Game::start()
{
    m_gameScene->clear();

    // add trashbin images
    QGraphicsPixmapItem* pi = new QGraphicsPixmapItem(*m_trashbinPixmap);
    pi->setPos(-m_size.width() / 2, -m_size.height() / 2);
    m_gameScene->addItem(pi);
    pi = new QGraphicsPixmapItem(*m_trashbinPixmap);
    pi->rotate(180.0);
    pi->setPos(m_size.width() / 2, m_size.height() / 2);
    m_gameScene->addItem(pi);

    // add cursor
    m_cursor = new QGraphicsPixmapItem(*m_cursorOpenPixmap);
    m_cursor->setOffset(-m_cursor->boundingRect().width() / 2, -m_cursor->boundingRect().height() / 2);
    m_cursor->setScale(0.25);
    m_cursor->setZValue(1000);
    m_gameScene->addItem(m_cursor);

    // add kinect status text
    m_kinectStatusText = addTextItem(KINECT_NOT_IN_SESSION_TEXT, QPoint(0, m_size.height() / 2 - 50), QColor(Qt::red), 16);

    // start timers
    m_speedIncreaseTimer.start(1000);
    m_itemSpawnTimer.start(m_itemSpawnInterval);
    m_gameTimer.start(1000.0 / FPS);

    // texts at the top
    //addTextItem("GRAB TO THE FUTURE", QPoint(-m_size.width() / 4, -m_size.height() / 2 + 2), QColor(Qt::darkRed), 32);
    //addTextItem("GRAB TO THE FUTURE", QPoint(-m_size.width() / 4 + 2, -m_size.height() / 2 + 4), QColor(Qt::red), 32);
    m_pointsText = addTextItem("Points: 0", QPoint(0, -m_size.height() / 2 + 2), QColor(Qt::red), 32);

    // reset game values
    m_itemSpawnInterval = START_ITEM_SPAWN_INTERVAL;
    m_currentSpeed = START_SPEED;
    m_points = 0;

    m_gameInProgress = true;
}

// called when kinect hand tracking session starts
void Game::handCreate()
{
    m_cursor->setVisible(true);

    m_kinectStatusText->setPlainText(KINECT_IN_SESSION_TEXT);
    m_kinectStatusText->setDefaultTextColor(QColor(Qt::green));

}

// called when kinect hand tracking session ends
void Game::handDestroy()
{
    m_cursor->setVisible(false);

    if (m_grabbedItem)
    {
        m_grabbedItem->grabRelease();
        m_grabbedItem = 0;
    }

    m_kinectStatusText->setPlainText(KINECT_NOT_IN_SESSION_TEXT);
    m_kinectStatusText->setDefaultTextColor(QColor(Qt::red));

}

// called when kinect is updated while tracking hand
void Game::handUpdate(qreal x, qreal y, qreal z)
{
    if (z < KINECT_TOO_NEAR)
    {
        m_kinectStatusText->setPlainText(KINECT_TOO_NEAR_TEXT);
        m_kinectStatusText->setDefaultTextColor(QColor(Qt::yellow));
    }
    else if (z > KINECT_TOO_FAR)
    {
        m_kinectStatusText->setPlainText(KINECT_TOO_FAR_TEXT);
        m_kinectStatusText->setDefaultTextColor(QColor(Qt::yellow));
    }
    else
    {
        m_kinectStatusText->setPlainText(KINECT_IN_SESSION_TEXT);
        m_kinectStatusText->setDefaultTextColor(QColor(Qt::green));
    }

    // convert kinect coordinates to scene coordinates
    mapKinectToScene(x, y);

    m_cursor->setPos(x, y);

    // if an item is grabbed update it's location also
    if (m_gameInProgress && m_grabbedItem)
    {
        m_lastCursorPos = m_grabbedItem->scenePos();

        // keep item inside screen
        QPointF newPoint = QPointF(x, y);
        if (newPoint.x() < -m_size.width() / 2) {
            newPoint.setX(-m_size.width() / 2);
        }
        if (newPoint.x() > m_size.width() / 2) {
            newPoint.setX(m_size.width() / 2);
        }
        m_grabbedItem->setPos(newPoint);
    }
}

// called when grab gesture is detected
void Game::grab(qreal x, qreal y)
{
    mapKinectToScene(x, y);

    m_cursor->setPixmap(*m_cursorClosedPixmap);
    m_cursor->setOffset(-m_cursor->boundingRect().width() / 2, -m_cursor->boundingRect().height() / 2);
    m_cursor->setSelected(false);

    QList<QGraphicsItem*> items = m_gameScene->items(QPointF(x, y));

    if (!m_gameInProgress)
    {
        // game isn't running check if buttons are hit
        for (unsigned int i = 0; i < items.size(); i++)
        {
            Button* but;
            but = dynamic_cast<Button*>(items.at(i));
            if (but) {
                but->activate();
                return;
            }
        }
    }
    else
    {
        // game is running check if items are hit
        for (unsigned int i = 0; i < items.size(); i++)
        {
            Item* item;
            item = dynamic_cast<Item*>(items.at(i));
            if (item)
            {
                item->grab();
                m_grabbedItem = item;
                item->setPos(x, y);
                return;
            }
            else
            {
                if (m_grabbedItem) m_grabbedItem->grabRelease();
                m_grabbedItem = 0;
            }
        }
    }
}

// called when grab release is detected
void Game::grabRelease(qreal x, qreal y)
{
    mapKinectToScene(x, y);

    m_cursor->setPixmap(*m_cursorOpenPixmap);
    m_cursor->setOffset(-m_cursor->boundingRect().width() / 2, -m_cursor->boundingRect().height() / 2);

    if (m_grabbedItem)
    {
        QPointF newPoint = QPointF(x, y);
        if (newPoint.x() < -m_size.width() / 2) newPoint.setX(-m_size.width() / 2);
        if (newPoint.x() > m_size.width() / 2) newPoint.setX(m_size.width() / 2);

        m_grabbedItem->grabRelease();

        // set item velocity based on two last cursor points
        QPointF vel = m_lastCursorPos - newPoint;
        m_grabbedItem->setVelocity(-vel * 0.5);

    }
    m_grabbedItem = 0;
}


void Game::speedIncrease()
{
    m_currentSpeed += SPEED_STEP;
}

void Game::itemSpawn()
{
    if (!m_gameInProgress) return;

    // create a new item to the scene
    Item* newItem = 0;
    switch (qrand() % 3)
    {
        case 0:
            newItem = new Item(m_mousePixmap, QPointF(MOUSE_SIZE * m_size.height(), MOUSE_SIZE * m_size.height()));
            break;
        case 1:
            newItem = new Item(m_joystickPixmap, QPointF(JOYSTICK_SIZE * m_size.height(), JOYSTICK_SIZE * m_size.height()));
            break;
        case 2:
            newItem = new Item(m_tabletPixmap, QPointF(TABLET_SIZE * m_size.height() * 1.2, TABLET_SIZE * m_size.height()));
            break;
        default:
            break;
    }
    qreal trashBoundaryInPixels = TRASH_BOUNDARY * m_size.width();

    // set item's x position at random pos between trash bins
    qreal xPos = ((qreal)qrand()/(qreal)RAND_MAX) * (m_gameScene->sceneRect().width() - trashBoundaryInPixels * 2.0) - (m_gameScene->width() / 2.0 - trashBoundaryInPixels);

    // set item's y position to screen bottom
    qreal yPos = m_gameScene->sceneRect().bottom() + newItem->getSize().y() / 2;

    newItem->setPos(xPos, yPos);
    newItem->setSpeed(m_currentSpeed);
    newItem->setTrashBoundary(trashBoundaryInPixels);

    QObject::connect(newItem, SIGNAL(wentToTrash()), this, SLOT(pointIncrease()));
    QObject::connect(newItem, SIGNAL(escape()), this, SLOT(gameOver()));

    m_gameScene->addItem(newItem);

    // make items birth faster
    m_itemSpawnInterval -= ITEM_SPAWN_INTERVAL_STEP;
    if (m_itemSpawnInterval < 1000) m_itemSpawnInterval = 1000;

    m_itemSpawnTimer.start(m_itemSpawnInterval);
}

// adds text item to the scene with center given in param position
QGraphicsTextItem* Game::addTextItem(QString text, QPoint position, QColor color, quint16 fontSize)
{
    QGraphicsTextItem* textItem = new QGraphicsTextItem;
    QFont font("Courier", fontSize, QFont::Bold, true);
    textItem->setDefaultTextColor(color);
    textItem->setFont(font);
    textItem->setTextInteractionFlags(Qt::NoTextInteraction);
    textItem->setPlainText(text);
    QFontMetrics fm(font);
    int width = fm.width(text);
    if (width < 800)
    {
        position.setX(position.x() - width / 2);
    }
    else
    {
        position.setX(position.x() - 400);
    }
    textItem->setPos(position);
    textItem->setTextWidth(800);
    textItem->setZValue(10);
    m_gameScene->addItem(textItem);
    return textItem;
}

void Game::mapKinectToScene(qreal& x, qreal& y)
{
    x = (m_size.width() / 2) * (x / KINECT_X_RANGE);
    y = -(m_size.height() / 2) * (y / KINECT_Y_RANGE);
}

void Game::pointIncrease()
{
    m_points++;
    m_pointsText->setPlainText(QString("Points: ") + QString::number(m_points));
}

void Game::gameOver()
{
    m_speedIncreaseTimer.stop();
    m_itemSpawnTimer.stop();
    m_gameTimer.stop();

    m_gameInProgress = false;
    if (m_grabbedItem)
    {
        m_grabbedItem->grabRelease();
        m_grabbedItem = 0;
    }

    // darken scene
    QGraphicsRectItem* ri = new QGraphicsRectItem(QRectF(-m_size.width() / 2, -m_size.height() / 2, m_size.width(), m_size.height()));
    ri->setBrush(QColor(0, 0, 0, 196));
    ri->setZValue(1000);
    m_gameScene->addItem(ri);

    m_cursor->setZValue(3000);

    // add end texts
    QGraphicsTextItem* text = addTextItem("GAME OVER", QPoint(0, -m_size.height() * 0.375), QColor(Qt::darkRed), 64);
    text->setZValue(2000);
    text = addTextItem("GAME OVER", QPoint(2, -m_size.height() * 0.375 + 2), QColor(Qt::red), 64);
    text->setZValue(2000);
    text = addTextItem("Points: " + QString::number(m_points), QPoint(0, -m_size.height() * 0.25), QColor(Qt::red), 32);
    text->setZValue(2000);

    // add buttons
    Button* button = new Button("RESTART", QPoint(-m_size.width() / 4, m_size.height() / 8), QPointF(BUTTON_X_SIZE * m_size.width(), BUTTON_Y_SIZE * m_size.height()));
    m_gameScene->addItem(button);
    QObject::connect(button, SIGNAL(activated()), this, SLOT(start()));
    button = new Button("QUIT", QPoint(m_size.width() / 4, m_size.height() / 8), QPointF(BUTTON_X_SIZE * m_size.width(), BUTTON_Y_SIZE * m_size.height()));
    m_gameScene->addItem(button);
    QObject::connect(button, SIGNAL(activated()), QCoreApplication::instance(), SLOT(quit()));
}


