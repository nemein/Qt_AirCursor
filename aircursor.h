/*
    Air Cursor library for Qt applications using Kinect
    Copyright (C) 2012 Tuomas Haapala, Nemein
*/

#ifndef AIRCURSOR_H
#define AIRCURSOR_H

#include <QThread>
#include <QMutex>
#include <QImage>
#include <QSettings>
#include <iostream>

#include <XnOpenNI.h>
#include <XnVNite.h>

#include <cv.h>

class AirCursor : public QThread
{
    Q_OBJECT
public:

    // singleton pattern
    static AirCursor* instance()
    {
        if (!m_instance)
        {
            static QMutex mutex;
            mutex.lock();

            if (!m_instance)
                m_instance = new AirCursor();

            mutex.unlock();
        }

        return m_instance;
    }

    static void drop()
    {
        static QMutex mutex;
        mutex.lock();
        delete m_instance;
        m_instance = 0;
        mutex.unlock();
    }

    ~AirCursor();

    bool init(bool makeDebugImage = false);

    virtual void run();

signals:

    void handCreate(qreal x, qreal y, qreal z, qreal time);
    void handDestroy(qreal time);

    void gestureRecognized(QString gestureStr);
    void gestureProcess(QString gestureStr);

    void sessionStart();
    void sessionEnd();

    void handUpdate(qreal x, qreal y, qreal z, qreal time, bool grab);
    void push(qreal x, qreal y, qreal z, qreal velocity, qreal angle);

    void grab(qreal x, qreal y, qreal z);
    void grabRelease(qreal x, qreal y, qreal z);

    void handTooClose();
    void handTooFar();

    void debugImageUpdate(QImage image);

    void swipeUp(qreal velocity, qreal angle);
    void swipeDown(qreal velocity, qreal angle);
    void swipeLeft(qreal velocity, qreal angle);
    void swipeRight(qreal velocity, qreal angle);

public slots:
    void quit();

    void stopRunning();

private:

    explicit AirCursor(QObject *parent = 0);

    void analyzeGrab();
    void updateState();
    void newHandPoint(qreal x, qreal y, qreal z);

    QList<XnPoint3D> m_handPoints;

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

    static AirCursor* m_instance;

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
