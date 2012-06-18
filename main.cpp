// small game demonstrating how air cursor can be used.

#include <QtGui/QApplication>
#include <QDesktopWidget>
#include <iostream>

#include "game.h"
#include "aircursor.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // new full screen sized game instance
    Game game(QApplication::desktop()->screenGeometry().size());

    // init air cursor
    AirCursor* ac = AirCursor::instance();
    std::cout << "Initializing Kinect... " << std::flush;
    if (!ac->init())
    {
        return 0;
    }
    std::cout << "ok" << std::endl;

    // connect signals
    QObject::connect(ac, SIGNAL(handCreate(qreal,qreal,qreal,qreal)), &game, SLOT(handCreate()));
    QObject::connect(ac, SIGNAL(handDestroy(qreal)), &game, SLOT(handDestroy()));
    QObject::connect(ac, SIGNAL(handUpdate(qreal,qreal,qreal,qreal,bool)), &game, SLOT(handUpdate(qreal, qreal, qreal)));
    QObject::connect(ac, SIGNAL(grab(qreal,qreal,qreal)), &game, SLOT(grab(qreal, qreal)));
    QObject::connect(ac, SIGNAL(grabRelease(qreal,qreal,qreal)), &game, SLOT(grabRelease(qreal,qreal)));

    // for air cursor to work start needs to be called
    ac->start();

    return app.exec();
}
