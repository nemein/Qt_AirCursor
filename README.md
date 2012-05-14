Air Cursor library for Qt applications using Kinect
===================================================

This library allows natural user interactions in Qt applications using a [Kinect](http://en.wikipedia.org/wiki/Kinect) as the input device. Users can manipulate content on the screen by moving their hand around and making _grab gestures_ by closing their hand. This corresponds roughly to traditional mouse movements and click-and-hold events.

You can see an example application using this library in action in [a YouTube video](http://youtu.be/dxkpSzl-SLg).

This library is available under the LGPL license and has been developed by [Nemein](http://nemein.com) as part of the EU-funded [SmarcoS project](http://smarcos-project.eu/).

## Dependencies

* [OpenCV](http://opencv.willowgarage.com/wiki/) - computer vision library
* [OpenNI](http://www.openni.org/) - natural interaction library
* [PrimeSense NITE](http://www.primesense.com/technology/nite3) - gesture and skeleton tracking for OpenNI, free for commercial usage but [requires an EULA](https://groups.google.com/d/msg/openni-dev/bXef5gGLHyI/UV7C2-QsabYJ)
* [PrimeSense Kinect Sensor](https://github.com/avin2/SensorKinect) - Kinect drivers for OpenNI

See [Kinect on Ubuntu with OpenNI](http://www.20papercups.net/programming/kinect-on-ubuntu-with-openni/) for a tutorial on how these pieces fit together.
