/*
    Air Cursor library for Qt applications using Kinect
    Copyright (C) 2012 Tuomas Haapala, Nemein
    
    Quick instructions:
    
    1. Make sure dependencies (OpenNI, Nite, OpenCV) are included in your .pro file
    2. Add aircursor.h and aircursor.cpp to your project
    3. Instantiate AirCursor class in your code
    4. Call AirCursor::init()
    5. Connect AirCursor signals to your QObjects
    6. Call AirCursor::start()
*/

#include <QMetaType>
#include "aircursor.h"

// how much running grab value is affected by new values
const qreal GRAB_SMOOTHING_FACTOR = 0.5;

// how much running grab value needs to shift before grab status is changed
const qreal GRAB_STATE_CHANGE_THRESHOLD = 0.1;

// how many raw positions are used to calculate current hand position
const quint8 NUM_OF_SMOOTHING_POINTS = 5;

const int DEPTH_MAP_SIZE_X = 640;
const int DEPTH_MAP_SIZE_Y = 480;

// how much further away (z distance) points are included to be part of user's hand
// TODO change this to be in mm.
const int DEPTH_THRESHOLD = 10;

// min size for defects to be counted
const int DEFECT_MIN_SIZE = 25;

// maximum number of defects that is allowed for grabbing hand
const int GRAB_MAX_DEFECTS = 0;

// allowed depth range in millimeters.
const int NEAR_CLIPPING_DISTANCE = 500;
const int FAR_CLIPPING_DISTANCE = 2000;

// distances whose crossing will emit warning signal
const int NEAR_WARNING_DISTANCE = 700;
const int FAR_WARNING_DISTANCE = 1700;

// region of interest rectangle's size measured from current hand position.
// ideally should contain whole hand and nothing more
const int HAND_ROI_SIZE_LEFT = 110;
const int HAND_ROI_SIZE_RIGHT = 110;
const int HAND_ROI_SIZE_UP = 100;
const int HAND_ROI_SIZE_DOWN = 150;

// min size for contours to be counted in
const int CONTOUR_MIN_SIZE = 1000;

const QString SETTINGS_FILENAME = "aircursor.ini";

AirCursor::AirCursor(QObject *parent) :
    QThread(parent),
    m_grabbing(false),
    m_init(false),
    m_quit(false),
    m_iplDepthMap(0),
    m_iplDebugImage(0),
    m_debugImage(0),
    m_debugImageEnabled(false),
    m_grabCounter(0),
    m_grabDetected(false),
    m_currentGrab(false),
    m_runningGrab(0.0f)
{
    // this is needed so that QImage can be used as a parameter with queued signals
    qRegisterMetaType<QImage>("QImage");
}

AirCursor::~AirCursor()
{
    stop();
    while (isRunning());

    if (m_debugImage)
    {
        delete m_debugImage;
        m_debugImage = 0;
    }
    if (m_iplDepthMap)
    {
        cvReleaseImage(&m_iplDepthMap);
        m_iplDepthMap = 0;
    }
    if (m_iplDebugImage)
    {
        cvReleaseImage(&m_iplDebugImage);
        m_iplDebugImage = 0;
    }
    if (m_cvMemStorage)
    {
        cvReleaseMemStorage(&m_cvMemStorage);
        m_cvMemStorage = 0;
    }

    m_depthGenerator.Release();
    m_handsGenerator.Release();
    m_gestureGenerator.Release();
    m_context.Release();

    quit();
    wait();
}

void XN_CALLBACK_TYPE AirCursor::gestureRecognizedCB(xn::GestureGenerator& generator,
                   const XnChar* strGesture,
                   const XnPoint3D* pIDPosition,
                   const XnPoint3D* pEndPosition, void* pCookie)
{
    AirCursor* ac = (AirCursor*)pCookie;
    ac->emit gestureRecognized(QString(strGesture));
}

void XN_CALLBACK_TYPE AirCursor::gestureProcessCB(xn::GestureGenerator& generator,
                const XnChar* strGesture,
                const XnPoint3D* pPosition,
                XnFloat fProgress,
                void* pCookie)
{
    AirCursor* ac = (AirCursor*)pCookie;
    ac->emit gestureProcess(QString(strGesture));
}

void XN_CALLBACK_TYPE AirCursor::handCreateCB(xn::HandsGenerator& generator,
            XnUserID nId, const XnPoint3D* pPosition,
            XnFloat fTime, void* pCookie)
{
    AirCursor* ac = (AirCursor*)pCookie;
    ac->emit handCreate(pPosition->X, pPosition->Y, pPosition->Z, fTime);
}

void XN_CALLBACK_TYPE AirCursor::handUpdateCB(xn::HandsGenerator& generator,
            XnUserID nId, const XnPoint3D* pPosition,
            XnFloat fTime, void* pCookie)
{

    AirCursor* ac = (AirCursor*)pCookie;
    ac->m_handPosRealWorld = *pPosition;
    ac->m_depthGenerator.ConvertRealWorldToProjective(1, pPosition, &(ac->m_handPosProjected));
    ac->newHandPoint(pPosition->X, pPosition->Y, pPosition->Z);

    ac->analyzeGrab();
    ac->updateState();
    //emit ac->handUpdate(pPosition->X, pPosition->Y, pPosition->Z, fTime, ac->m_grabbing);
    emit ac->handUpdate(ac->m_handPosSmooth.X, ac->m_handPosSmooth.Y, ac->m_handPosSmooth.Z, fTime, ac->m_grabbing);

    if (ac->m_handPosRealWorld.Z < NEAR_WARNING_DISTANCE)
    {
        emit ac->handTooClose();
    }
    else if (ac->m_handPosRealWorld.Z > FAR_WARNING_DISTANCE)
    {
        emit ac->handTooFar();
    }
}

void XN_CALLBACK_TYPE AirCursor::handDestroyCB(xn::HandsGenerator& generator,
             XnUserID nId, XnFloat fTime,
             void* pCookie)
{
    AirCursor* ac = (AirCursor*)pCookie;
    emit ac->handDestroy(fTime);
    //std::cout << "hand destroy frame: " << ac->m_frame << std::endl;
}

void XN_CALLBACK_TYPE AirCursor::sessionStartCB(const XnPoint3D& pFocus, void* UserCxt)
{
    AirCursor* ac = (AirCursor*)UserCxt;
    //std::cout << "session start frame: " << ac->m_frame << std::endl;
    emit ac->sessionStart();
}
void XN_CALLBACK_TYPE AirCursor::sessionEndCB(void* UserCxt)
{
    AirCursor* ac = (AirCursor*)UserCxt;
    emit ac->sessionEnd();
    //std::cout << "session ended frame: " << ac->m_frame << std::endl;
}

void XN_CALLBACK_TYPE AirCursor::pushCB(XnFloat fVelocity, XnFloat fAngle, void *UserCxt)
{
    AirCursor* ac = (AirCursor*)UserCxt;
    emit ac->push(ac->m_handPosRealWorld.X, ac->m_handPosRealWorld.Y, ac->m_handPosRealWorld.Z, fVelocity, fAngle);
}

void XN_CALLBACK_TYPE AirCursor::swipeUpCB(XnFloat fVelocity, XnFloat fAngle, void* cxt)
{
    //std::cout << "Swipe UP velocity: " << fVelocity << " angle: " << fAngle << std::endl;
    AirCursor* ac = (AirCursor*)cxt;
    ac->emit swipeUp(fVelocity, fAngle);
}
void XN_CALLBACK_TYPE AirCursor::swipeDownCB(XnFloat fVelocity, XnFloat fAngle, void* cxt)
{
    //std::cout << "Swipe DOWN velocity: " << fVelocity << " angle: " << fAngle << std::endl;
    AirCursor* ac = (AirCursor*)cxt;
    ac->emit swipeDown(fVelocity, fAngle);
}
void XN_CALLBACK_TYPE AirCursor::swipeLeftCB(XnFloat fVelocity, XnFloat fAngle, void* cxt)
{
    //std::cout << "Swipe LEFT velocity: " << fVelocity << " angle: " << fAngle << std::endl;
    AirCursor* ac = (AirCursor*)cxt;
    ac->emit swipeLeft(fVelocity, fAngle);
}
void XN_CALLBACK_TYPE AirCursor::swipeRightCB(XnFloat fVelocity, XnFloat fAngle, void* cxt)
{
    //std::cout << "Swipe RIGHT velocity: " << fVelocity << " angle: " << fAngle << std::endl;
    AirCursor* ac = (AirCursor*)cxt;
    ac->emit swipeRight(fVelocity, fAngle);
}

bool AirCursor::init(bool makeDebugImage)
{
    if (m_init) return true;

    m_debugImageEnabled = makeDebugImage;

    XnStatus rc = XN_STATUS_OK;

    // init OpenNI context
    rc = m_context.Init();
    m_context.SetGlobalMirror(true);
    if (rc != XN_STATUS_OK)
    {
        std::cout << "ERROR: init failed: " << xnGetStatusString(rc) << std::endl;
        return false;
    }

    // create a DepthGenerator node
    rc = m_depthGenerator.Create(m_context);
    if (rc != XN_STATUS_OK)
    {
        std::cout << "node creation failed: " << xnGetStatusString(rc) << std::endl;
        return false;
    }

    // create the gesture and hands generators
    rc = m_gestureGenerator.Create(m_context);
    if (rc != XN_STATUS_OK)
    {
        std::cout << "gesture generator creation failed: " << xnGetStatusString(rc) << std::endl;
        return false;
    }

    rc = m_handsGenerator.Create(m_context);
    if (rc != XN_STATUS_OK)
    {
        std::cout << "hands generator creation failed: " << xnGetStatusString(rc) << std::endl;
        return false;
    }

    // register to callbacks
    XnCallbackHandle h1, h2;
    m_gestureGenerator.RegisterGestureCallbacks(gestureRecognizedCB, gestureProcessCB, this, h1);
    m_handsGenerator.RegisterHandCallbacks(handCreateCB, handUpdateCB, handDestroyCB, this, h2);

    // init session manager
    rc = m_sessionManager.Initialize(&m_context, "Wave,Click", NULL);
    if (rc != XN_STATUS_OK)
    {
        std::cout << "session manager init failed: " << xnGetStatusString(rc) << std::endl;
        return false;
    }

    // register to session callbacks
    m_sessionManager.RegisterSession(this, &sessionStartCB, &sessionEndCB);

    // start generating data
    rc = m_context.StartGeneratingAll();
    if (rc != XN_STATUS_OK)
    {
        std::cout << "data generating start failed: " << xnGetStatusString(rc) << std::endl;
        return false;
    }

    m_pushDetector.RegisterPush(this, pushCB);
    m_sessionManager.AddListener(&m_pushDetector);

    m_swipeDetector.RegisterSwipeUp(this, &swipeUpCB);
    m_swipeDetector.RegisterSwipeDown(this, &swipeDownCB);
    m_swipeDetector.RegisterSwipeLeft(this, &swipeLeftCB);
    m_swipeDetector.RegisterSwipeRight(this, &swipeRightCB);
    m_sessionManager.AddListener(&m_swipeDetector);

    // 8bit depth map
    m_iplDepthMap = cvCreateImage(cvSize(DEPTH_MAP_SIZE_X, DEPTH_MAP_SIZE_Y), IPL_DEPTH_8U, 1);

    // opencv mem storage
    m_cvMemStorage = cvCreateMemStorage(0);

    if (m_debugImageEnabled)
    {
        // 24bit rgb888 debug image
        m_iplDebugImage = cvCreateImage(cvSize(DEPTH_MAP_SIZE_X, DEPTH_MAP_SIZE_Y), IPL_DEPTH_8U, 3);

        // Same debug image as a QImage
        m_debugImage = new QImage(DEPTH_MAP_SIZE_X, DEPTH_MAP_SIZE_Y, QImage::Format_RGB888);
    }

    m_init = true;
    return true;
}

void AirCursor::run()
{
    if (!m_init)
    {
        m_context.Shutdown();
        return;
    }

    XnStatus rc = XN_STATUS_OK;

    bool quit = false;
    while (!quit)
    {

        // Wait for new data to be available
        rc = m_context.WaitOneUpdateAll(m_depthGenerator);
        if (rc != XN_STATUS_OK)
        {
            std::cout << "Failed updating data: " << xnGetStatusString(rc) << std::endl;
            break;
        }

        m_sessionManager.Update(&m_context);

        static QMutex mutex;
        mutex.lock();
        quit = m_quit;
        mutex.unlock();
    }
}

void AirCursor::stop()
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    m_quit = true;
}

void AirCursor::analyzeGrab()
{
    cvClearMemStorage(m_cvMemStorage);

    // get current depth map from Kinect
    const XnDepthPixel* depthMap = m_depthGenerator.GetDepthMap();

    // convert 16bit openNI depth map to 8bit IplImage used in opencv processing
    int origDepthIndex = 0;
    char* depthPtr = m_iplDepthMap->imageData;
    char* debugPtr = 0;
    if (m_debugImageEnabled) debugPtr = m_iplDebugImage->imageData;
    for (unsigned int y = 0; y < DEPTH_MAP_SIZE_Y; y++)
    {
        for (unsigned int x = 0; x < DEPTH_MAP_SIZE_X; x++)
        {
            // get current depth value from original depth map
            short depth = depthMap[origDepthIndex];

            // check that current value is in the allowed range determined by clipping distances,
            // and if it is map it to range 0 - 255 so that 255 is the closest value
            unsigned char pixel = 0;
            if (depth >= NEAR_CLIPPING_DISTANCE && depth <= FAR_CLIPPING_DISTANCE)
            {
               depth -= NEAR_CLIPPING_DISTANCE;
                    pixel = 255 - (255.0f * ((float)depth / (FAR_CLIPPING_DISTANCE - NEAR_CLIPPING_DISTANCE)));
            }
            else {
                pixel = 0;
            }
            m_iplDepthMap->imageData[y * m_iplDepthMap->widthStep + x] = pixel;
            *depthPtr = pixel;

            if (m_debugImageEnabled)
            {
                // init debug image with the same depth map
                *(debugPtr + 0) = pixel;
                *(debugPtr + 1) = pixel;
                *(debugPtr + 2) = pixel;
                debugPtr += 3;
            }
            origDepthIndex++;
            depthPtr++;
        }
    }

    // calculate region of interest corner points in real world coordinates
    XnPoint3D rwPoint1 = m_handPosRealWorld;
    rwPoint1.X -= HAND_ROI_SIZE_LEFT;
    rwPoint1.Y += HAND_ROI_SIZE_UP;
    XnPoint3D rwPoint2 = m_handPosRealWorld;
    rwPoint2.X += HAND_ROI_SIZE_RIGHT;
    rwPoint2.Y -= HAND_ROI_SIZE_DOWN;

    // convert corner points to projective coordinates
    XnPoint3D projPoint1, projPoint2;
    m_depthGenerator.ConvertRealWorldToProjective(1, &rwPoint1, &projPoint1);
    m_depthGenerator.ConvertRealWorldToProjective(1, &rwPoint2, &projPoint2);

    // round projected corner points to ints and clip them against the depth map
    int ROItopLeftX = qRound(projPoint1.X); int ROItopLeftY = qRound(projPoint1.Y);
    int ROIbottomRightX = qRound(projPoint2.X); int ROIbottomRightY = qRound(projPoint2.Y);
    if (ROItopLeftX < 0) ROItopLeftX = 0; else if (ROItopLeftX > DEPTH_MAP_SIZE_X - 1) ROItopLeftX = DEPTH_MAP_SIZE_X - 1;
    if (ROItopLeftY < 0) ROItopLeftY = 0; else if (ROItopLeftY > DEPTH_MAP_SIZE_Y - 1) ROItopLeftY = DEPTH_MAP_SIZE_Y - 1;
    if (ROIbottomRightX < 0) ROIbottomRightX = 0; else if (ROIbottomRightX > DEPTH_MAP_SIZE_X - 1) ROIbottomRightX = DEPTH_MAP_SIZE_X - 1;
    if (ROIbottomRightY < 0) ROIbottomRightY = 0; else if (ROIbottomRightY > DEPTH_MAP_SIZE_Y - 1) ROIbottomRightY = DEPTH_MAP_SIZE_Y - 1;

    // set region of interest
    CvRect rect = cvRect(ROItopLeftX, ROItopLeftY, ROIbottomRightX - ROItopLeftX, ROIbottomRightY - ROItopLeftY);
    if(rect.height > 0 && rect.width > 0)
    {
        cvSetImageROI(m_iplDepthMap, rect);
        if (m_debugImageEnabled) cvSetImageROI(m_iplDebugImage, rect);
    }

    // use depth threshold to isolate hand
    // as a center point of thresholding, it seems that it's better to use a point bit below
    // the point Nite gives as the hand point
    XnPoint3D rwThresholdPoint = m_handPosRealWorld; rwThresholdPoint.Y -= 30;
    XnPoint3D projThresholdPoint;
    m_depthGenerator.ConvertRealWorldToProjective(1, &rwThresholdPoint, &projThresholdPoint);
    int lowerBound = (unsigned char)m_iplDepthMap->imageData[(int)projThresholdPoint.Y * DEPTH_MAP_SIZE_X + (int)projThresholdPoint.X] - DEPTH_THRESHOLD;
    if (lowerBound < 0) lowerBound = 0;
    cvThreshold( m_iplDepthMap, m_iplDepthMap, lowerBound, 255, CV_THRESH_BINARY );

    // color used for drawing the hand in the debug image, green for normal and red for grab.
    // color lags one frame from actual grab status but in practice that shouldn't be too big of a problem
    int rCol, gCol, bCol;
    if(m_grabbing) {
        rCol = 255; gCol = 0; bCol = 0;
    }
    else {
        rCol = 0; gCol = 255; bCol = 0;
    }

    // go through the ROI and paint hand on debug image with current grab status color
    if (m_debugImageEnabled)
    {
        // index of first pixel in the ROI
        int startIndex = ROItopLeftY * m_iplDepthMap->widthStep + ROItopLeftX;

        depthPtr = &(m_iplDepthMap->imageData[startIndex]);
        debugPtr = &(m_iplDebugImage->imageData[startIndex * 3]);

        // how much index needs to increase when moving to next line
        int vertInc = m_iplDepthMap->widthStep - (ROIbottomRightX - ROItopLeftX);

        for (int y = ROItopLeftY; y < ROIbottomRightY; y++)
        {
            for (int x = ROItopLeftX; x < ROIbottomRightX; x++)
            {
                if((unsigned char)*depthPtr > 0)
                {
                    *(debugPtr + 0) = rCol / 2;
                    *(debugPtr + 1) = gCol / 2;
                    *(debugPtr + 2) = bCol / 2;
                }

                // next pixel
                depthPtr++;
                debugPtr += 3;
            }

            // next line
            depthPtr += vertInc;
            debugPtr += vertInc * 3;
        }
    }

    // find contours in the hand and draw them on debug image
    CvSeq* contours = 0;
    cvFindContours(m_iplDepthMap, m_cvMemStorage, &contours, sizeof(CvContour));
    if (m_debugImageEnabled)
    {
        if(contours)
        {
            cvDrawContours(m_iplDebugImage, contours, cvScalar(rCol, gCol , bCol), cvScalar(rCol, gCol, bCol), 1);
        }
    }

    // go through contours and search for the biggest one
    CvSeq* biggestContour = 0;
    double biggestArea = 0.0f;
    for(CvSeq* currCont = contours; currCont != 0; currCont = currCont->h_next)
    {
        // ignore small contours which are most likely caused by artifacts
        double currArea = cvContourArea(currCont);
        if(currArea < CONTOUR_MIN_SIZE) continue;

        if(!biggestContour || currArea > biggestArea) {
            biggestContour = currCont;
            biggestArea = currArea;
        }
    }

    int numOfValidDefects = 0;

    if(biggestContour)
    {
        // calculate convex hull of the biggest contour found which is hopefully the hand
        CvSeq* hulls = cvConvexHull2(biggestContour, m_cvMemStorage, CV_CLOCKWISE, 0);

        if (m_debugImageEnabled)
        {
            // calculate convex hull and return it in a different form.
            // only required for drawing
            CvSeq* hulls2 = cvConvexHull2(biggestContour, m_cvMemStorage, CV_CLOCKWISE, 1);

            // draw the convex hull
            cvDrawContours(m_iplDebugImage, hulls2, cvScalar(rCol, gCol , bCol), cvScalar(rCol, gCol, bCol), 1);
        }

        // calculate convexity defects of hand's convex hull
        CvSeq* defects = cvConvexityDefects(biggestContour, hulls, m_cvMemStorage);

        int numOfDefects = defects->total;

        if (numOfDefects > 0)
        {
            // calculate defect min size in projective coordinates.
            // this is done using a vector from current hand position to a point DEFECT_MIN_SIZE amount above it.
            // that vector is converted to projective coordinates and it's length is calculated.
            XnPoint3D rwTempPoint = m_handPosRealWorld;
            rwTempPoint.Y += DEFECT_MIN_SIZE;
            XnPoint3D projTempPoint;
            m_depthGenerator.ConvertRealWorldToProjective(1, &rwTempPoint, &projTempPoint);
            int defectMinSizeProj = m_handPosProjected.Y - projTempPoint.Y;

            // convert opencv seq to array
            CvConvexityDefect* defectArray;defectArray = (CvConvexityDefect*)malloc(sizeof(CvConvexityDefect) * numOfDefects);
            cvCvtSeqToArray(defects, defectArray, CV_WHOLE_SEQ);

            for(int i = 0; i < numOfDefects; i++)
            {
                // ignore too small defects
                if((defectArray[i].depth) < defectMinSizeProj)
                {
                   continue;
                }

               numOfValidDefects++;

               if (m_debugImageEnabled)
               {
                   // draw blue point to defect
                   cvCircle(m_iplDebugImage, *(defectArray[i].depth_point), 5, cvScalar(0, 0, 255), -1);
                   cvCircle(m_iplDebugImage, *(defectArray[i].start), 5, cvScalar(0, 0, 255), -1);
                   cvCircle(m_iplDebugImage, *(defectArray[i].end), 5, cvScalar(0, 0, 255), -1);
               }
            }

            free(defectArray);
        }
    }

    if (m_debugImageEnabled)
    {
        cvResetImageROI(m_iplDebugImage);

        // draw white dot on current hand position
        cvCircle(m_iplDebugImage, cvPoint(m_handPosProjected.X, m_handPosProjected.Y), 5, cvScalar(255, 255, 255), -1);

        // draw gray dot on current center of threshold position
        //cvCircle(m_iplDebugImage, cvPoint(projThresholdPoint.X, projThresholdPoint.Y), 5, cvScalar(127, 127, 127), -1);

        // draw ROI with green
        //cvRectangle(m_iplDebugImage, cvPoint(ROItopLeftX, ROItopLeftY), cvPoint(ROIbottomRightX, ROIbottomRightY), cvScalar(0, 255, 0));
    }

    // determine current grab status based on defect count
    if(numOfValidDefects <= GRAB_MAX_DEFECTS)
    {
        m_currentGrab = true;
    }
    else
    {
        m_currentGrab = false;
    }

    if (m_debugImageEnabled)
    {
        // debug strings
        QList<QString> debugStrings;
        debugStrings.push_back(QString("hand distance: " + QString::number(m_handPosRealWorld.Z) + " mm").toStdString().c_str());
        debugStrings.push_back(QString("defects: " + QString::number(numOfValidDefects)).toStdString().c_str());

        // convert iplDebugImage to QImage
        char* scanLinePtr = m_iplDebugImage->imageData;
        for (int y = 0;y < DEPTH_MAP_SIZE_Y; y++) {
            memcpy(m_debugImage->scanLine(y), scanLinePtr, DEPTH_MAP_SIZE_X * 3);
            scanLinePtr += DEPTH_MAP_SIZE_X * 3;
        }

        emit debugUpdate(*m_debugImage, debugStrings);
    }
}

// update grab state based on running grab value
void AirCursor::updateState()
{
    m_runningGrab = GRAB_SMOOTHING_FACTOR * m_runningGrab + (1.0 - GRAB_SMOOTHING_FACTOR) * (float)m_currentGrab;

    if (!m_grabbing)
    {
        if (m_runningGrab > (0.5 + GRAB_STATE_CHANGE_THRESHOLD))
        {
            m_grabbing = true;
            emit grab(m_handPosRealWorld.X, m_handPosRealWorld.Y, m_handPosRealWorld.Z);
        }
    }
    else
    {
        if (m_runningGrab < (0.5 - GRAB_STATE_CHANGE_THRESHOLD))
        {
            m_grabbing = false;
            emit grabRelease(m_handPosRealWorld.X, m_handPosRealWorld.Y, m_handPosRealWorld.Z);
        }
    }
}

// add new raw hand position and update smoothed position
void AirCursor::newHandPoint(qreal x, qreal y, qreal z)
{
    XnPoint3D hp;
    hp.X = x; hp.Y = y; hp.Z = z;
    while (m_handPoints.size() > NUM_OF_SMOOTHING_POINTS - 1) m_handPoints.pop_front();
    m_handPoints.push_back(hp);

    XnPoint3D cumulHp;
    cumulHp.X = cumulHp.Y = cumulHp.Z = 0;
    for (unsigned int i = 0; i < m_handPoints.size(); i++)
    {
        cumulHp.X += m_handPoints[i].X;
        cumulHp.Y += m_handPoints[i].Y;
        cumulHp.Z += m_handPoints[i].Z;
    }

    m_handPosSmooth.X = cumulHp.X / m_handPoints.size();
    m_handPosSmooth.Y = cumulHp.Y / m_handPoints.size();
    m_handPosSmooth.Z = cumulHp.Z / m_handPoints.size();
}






