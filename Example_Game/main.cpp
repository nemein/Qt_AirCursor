/*
    Air Cursor library for Qt applications using Kinect
    Copyright (C) 2012 Tuomas Haapala, Nemein
    
    ---

    A simple game demonstrating how Air Cursor can be used in
    hand tracking and grabbing.
    
    Mouse pic by lunik, from openclipart.org
    Joystick pic by brunurb, from openclipart.org
    Tablet pic by ARBY JONES, from clker.com
*/

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
    AirCursor ac;
    std::cout << "Initializing Kinect... " << std::flush;
    if (!ac.init())
    {
        return 0;
    }
    std::cout << "ok" << std::endl;

    // connect signals from air cursor
    QObject::connect(&ac, SIGNAL(handCreate(qreal,qreal,qreal,qreal)), &game, SLOT(handCreate()));
    QObject::connect(&ac, SIGNAL(handDestroy(qreal)), &game, SLOT(handDestroy()));
    QObject::connect(&ac, SIGNAL(handUpdate(qreal,qreal,qreal,qreal,bool)), &game, SLOT(handUpdate(qreal, qreal, qreal)));
    QObject::connect(&ac, SIGNAL(grab(qreal,qreal,qreal)), &game, SLOT(grab(qreal, qreal)));
    QObject::connect(&ac, SIGNAL(grabRelease(qreal,qreal,qreal)), &game, SLOT(grabRelease(qreal,qreal)));

    // for air cursor to work, start needs to be called first
    ac.start();

    return app.exec();
}
