//=============================================================================
//  Cam
//  Webcam client
//
//  Copyright (C) 2016 Werner Schweer
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License version 2
//  as published by the Free Software Foundation and appearing in
//  the file LICENCE.GPL
//=============================================================================

#ifndef __MJPEG_H__
#define __MJPEG_H__

#include <QImageIOHandler>
#include <QImageIOPlugin>

struct AVCodec;
struct SwsContext;
struct AVCodecContext;

//---------------------------------------------------------
//   MjpegImageIOHandler
//---------------------------------------------------------

class MjpegImageIOHandler : public QImageIOHandler {
      AVCodec* codec;
      AVCodecContext* c;
      SwsContext* imgConvertCtx { 0 };

   public:
      MjpegImageIOHandler();
      virtual ~MjpegImageIOHandler();
      virtual bool canRead() const { return true; }
      virtual bool read(QImage*);
      };

//---------------------------------------------------------
//   MjpegImageIOPlugin
//---------------------------------------------------------

class MjpegImageIOPlugin : public QImageIOPlugin {
      Q_OBJECT
      Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QImageIOHandlerFactoryInterface" FILE "mjpeg.json")

   public:
      Capabilities capabilities(QIODevice*, const QByteArray& ba) const override;
      QImageIOHandler* create(QIODevice* device, const QByteArray& format = QByteArray()) const override;
      };

#endif

