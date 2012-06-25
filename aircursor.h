/*
    Air Cursor library for Qt applications using Kinect
    Copyright (C) 2012 Tuomas Haapala, Nemein
*/

#ifndef AIRCURSOR_H
#define AIRCURSOR_H

#include <QThread>
#include <QMutex>
#include <QImage>
#include <iostream>

#include <XnOpenNI.h>
#include <XnVNite.h>

#include <cv.h>

class AirCursor : public QThread
{
    Q_OBJECT
public:

    explicit AirCursor(QObject *parent = 0);
    ~AirCursor();

    bool init(bool makeDebugImage = false);

    virtual void run();
    void stop();

signals:
    // most of these signals are straight equivalents of openni/nite callbacks

    // emitted when hand tracking starts/stops
    void handCreate(qreal x, qreal y, qreal z, qreal time);
    void handDestroy(qreal time);

    // emitted when full focus gesture is detected
    void gestureRecognized(QString gestureStr);

    // emitted when partial focus gesture is detected
    void gestureProcess(QString gestureStr);

    // emitted when hand tracking session starts/stops
    void sessionStart();
    void sessionEnd();

    // emitted when hand position is updated
    void handUpdate(qreal x, qreal y, qreal z, qreal time, bool grab);

    // emitted when push gesture is detected
    void push(qreal x, qreal y, qreal z, qreal velocity, qreal angle);

    // emitted when grab state changes
    void grab(qreal x, qreal y, qreal z);
    void grabRelease(qreal x, qreal y, qreal z);

    // emitted when hand is too near/far
    void handTooClose();
    void handTooFar();

    // emitted when debug image is updated
    void debugUpdate(QImage image, QList<QString> strings);

    // emitted when swipe gesture is detected
    void swipeUp(qreal velocity, qreal angle);
    void swipeDown(qreal velocity, qreal angle);
    void swipeLeft(qreal velocity, qreal angle);
    void swipeRight(qreal velocity, qreal angle);

private:

    void analyzeGrab();
    void updateState();
    void newHandPoint(qreal x, qreal y, qreal z);

    // callbacks
    static void XN_CALLBACK_TYPE gestureRecognizedCB(xn::GestureGenerator& generator,
                                const XnChar* strGesture,
                                const XnPoint3D* pIDPosition,
                                const XnPoint3D* pEndPosition, void* pCookie);
    static void XN_CALLBACK_TYPE gestureProcessCB(xn::GestureGenerator& generator,
                                const XnChar* strGesture,
                                const XnPoint3D* pPosition,
                                XnFloat fProgress,
                                void* pCookie);
    static void XN_CALLBACK_TYPE handCreateCB(xn::HandsGenerator& generator,
                                XnUserID nId, const XnPoint3D* pPosition,
                                XnFloat fTime, void* pCookie);
    static void XN_CALLBACK_TYPE handUpdateCB(xn::HandsGenerator& generator,
                                XnUserID nId, const XnPoint3D* pPosition,
                                XnFloat fTime, void* pCookie);
    static void XN_CALLBACK_TYPE handDestroyCB(xn::HandsGenerator& generator,
                                XnUserID nId, XnFloat fTime,
                                void* pCookie);
    static void XN_CALLBACK_TYPE sessionStartCB(const XnPoint3D& pFocus, void* UserCxt);

    static void XN_CALLBACK_TYPE sessionEndCB(void* UserCxt);

    static void XN_CALLBACK_TYPE pushCB(XnFloat fVelocity, XnFloat fAngle, void *UserCxt);

    static void XN_CALLBACK_TYPE swipeUpCB(XnFloat fVelocity, XnFloat fAngle, void* cxt);
    static void XN_CALLBACK_TYPE swipeDownCB(XnFloat fVelocity, XnFloat fAngle, void* cxt);
    static void XN_CALLBACK_TYPE swipeLeftCB(XnFloat fVelocity, XnFloat fAngle, void* cxt);
    static void XN_CALLBACK_TYPE swipeRightCB(XnFloat fVelocity, XnFloat fAngle, void* cxt);

    QList<XnPoint3D> m_handPoints;

    xn::Context m_context;

    xn::GestureGenerator m_gestureGenerator;
    xn::HandsGenerator m_handsGenerator;

    XnVSessionManager m_sessionManager;

    xn::DepthGenerator m_depthGenerator;
    XnVPushDetector m_pushDetector;
    XnVSwipeDetector m_swipeDetector;

    bool m_init;

    XnDepthPixel* m_depthMap;

    bool m_grabbing;

    int m_grabCounter;

    CvMemStorage* m_cvMemStorage;

    IplImage* m_iplDepthMap;
    IplImage* m_iplDebugImage;

    bool m_quit;

    XnPoint3D m_handPosRealWorld;
    XnPoint3D m_handPosProjected;
    XnPoint3D m_handPosSmooth;

    QImage* m_debugImage;

    bool m_debugImageEnabled;

    XnPoint3D m_grabStarted;

    bool m_grabDetected;
    bool m_currentGrab;
    qreal m_runningGrab;
};

#endif // AIRCURSOR_H
