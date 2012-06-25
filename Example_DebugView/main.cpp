/*
    Air Cursor library for Qt applications using Kinect
    Copyright (C) 2012 Tuomas Haapala, Nemein

    ---

    This example shows how to display the debug image
    Air Cursor provides.
*/

#include <QtGui/QApplication>

#include "debugview.h"
#include "aircursor.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    DebugView view;
    view.show();

    // init air cursor with debug image creation (true)
    AirCursor ac;
    std::cout << "Initializing Kinect... " << std::flush;
    if (!ac.init(true))
    {
        return 0;
    }
    std::cout << "ok" << std::endl;

    // connect debug update signal from air cursor
    QObject::connect(&ac, SIGNAL(debugUpdate(QImage, QList<QString>)), &view, SLOT(debugUpdate(QImage, QList<QString>)), Qt::BlockingQueuedConnection);

    // for air cursor to work, start needs to be called first
    ac.start();

    return app.exec();
}
