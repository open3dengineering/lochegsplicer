/*
 * LocheGSplicer
 * Copyright (C) 2012 Jeff P. Houde (Lochemage)
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */


#ifndef VISUALIZER_VIEW_H
#define VISUALIZER_VIEW_H

#include <QGLWidget>

class QtLogo;

class VisualizerView : public QGLWidget
{
   Q_OBJECT

public:
   VisualizerView(QWidget *parent = 0);
   virtual ~VisualizerView();

   QSize minimumSizeHint() const;
   QSize sizeHint() const;

public slots:
   void setXRotation(int angle);
   void setYRotation(int angle);
   void setZRotation(int angle);

signals:
   void xRotationChanged(int angle);
   void yRotationChanged(int angle);
   void zRotationChanged(int angle);

protected:
   void initializeGL();
   void paintGL();
   void resizeGL(int width, int height);
   void mousePressEvent(QMouseEvent *event);
   void mouseMoveEvent(QMouseEvent *event);

private:
   QtLogo *mLogo;
   int mRotX;
   int mRotY;
   int mRotZ;
   QPoint mLastPos;
   QColor mGreenColor;
   QColor mPurpleColor;
};

#endif // VISUALIZER_VIEW_H
