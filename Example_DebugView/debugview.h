/*
    Air Cursor library for Qt applications using Kinect
    Copyright (C) 2012 Tuomas Haapala, Nemein

    ---

    Part of the debug view example.

    A simple view widget.
    Draws given QImage and QStrings on the screen.
*/

#ifndef DEBUGVIEW_H
#define DEBUGVIEW_H

#include <QtGui/QWidget>
#include <QImage>

class DebugView : public QWidget
{
    Q_OBJECT
    
public:
    DebugView(QWidget *parent = 0);
    ~DebugView();

    void paintEvent(QPaintEvent *);

public slots:

    // called when air cursor's debug image is updated
    void debugUpdate(QImage image, QList<QString> strings);

private:
    QImage m_image;
    QList<QString> m_strings;
};

#endif // DEBUGVIEW_H
