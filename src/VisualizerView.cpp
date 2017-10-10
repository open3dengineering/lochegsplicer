/*
 * LocheGSplicer
 * Copyright (C) 2012 Jeff P. Houde (Lochemage)
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
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


#include <VisualizerView.h>
#include <GCodeObject.h>

#include <math.h>
#include <assert.h>

////////////////////////////////////////////////////////////////////////////////
#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

#include <QtGui>
#include <QtOpenGL>

////////////////////////////////////////////////////////////////////////////////

#if defined(_WIN32)
#include <glext.h>
#define glCreateProgram ((PFNGLCREATEPROGRAMPROC)	wglGetProcAddress("glCreateProgram"))
#define glCreateShader  ((PFNGLCREATESHADERPROC)	wglGetProcAddress("glCreateShader"))
#define glShaderSource  ((PFNGLSHADERSOURCEPROC)	wglGetProcAddress("glShaderSource"))
#define glCompileShader ((PFNGLCOMPILESHADERPROC)	wglGetProcAddress("glCompileShader"))
#define glAttachShader  ((PFNGLATTACHSHADERPROC)	wglGetProcAddress("glAttachShader"))
#define glLinkProgram   ((PFNGLLINKPROGRAMPROC)		wglGetProcAddress("glLinkProgram"))
#define glUseProgram    ((PFNGLUSEPROGRAMPROC)		wglGetProcAddress("glUseProgram"))
#elif defined(__linux__)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>
#define glCreateProgram ((PFNGLCREATEPROGRAMPROC)	glXGetProcAddress((const GLubyte*)"glCreateProgram"))
#define glCreateShader  ((PFNGLCREATESHADERPROC)	glXGetProcAddress((const GLubyte*)"glCreateShader"))
#define glShaderSource  ((PFNGLSHADERSOURCEPROC)	glXGetProcAddress((const GLubyte*)"glShaderSource"))
#define glCompileShader ((PFNGLCOMPILESHADERPROC)	glXGetProcAddress((const GLubyte*)"glCompileShader"))
#define glAttachShader  ((PFNGLATTACHSHADERPROC)	glXGetProcAddress((const GLubyte*)"glAttachShader"))
#define glLinkProgram   ((PFNGLLINKPROGRAMPROC)		glXGetProcAddress((const GLubyte*)"glLinkProgram"))
#define glUseProgram    ((PFNGLUSEPROGRAMPROC)		glXGetProcAddress((const GLubyte*)"glUseProgram"))
#elif defined(APPLE)
#include <glext.h>
#else
#error("Platform not supported")
#endif

////////////////////////////////////////////////////////////////////////////////
VisualizerView::VisualizerView(const PreferenceData& prefs)
   : QGLWidget(QGLFormat(QGL::SampleBuffers), NULL)
   , mUpdateTimer(NULL)
   , mPrefs(prefs)
   , mLayerDrawHeight(0.0)
{
   for (int axis = 0; axis < AXIS_NUM_NO_E; ++axis)
   {
      mCameraRot[axis] = 0.0;
      mCameraRotTarget[axis] = 0.0;
      mCameraTrans[axis] = 0.0;
      mCameraTransTarget[axis] = 0.0;
   }

   mCameraZoom = -300.0;
   mCameraZoomTarget = -300.0;

   mCameraTrans[X] = mPrefs.platformWidth / 2.0;
   mCameraTrans[Y] = mPrefs.platformHeight / 2.0;
   mCameraTransTarget[X] = mCameraTrans[X];
   mCameraTransTarget[Y] = mCameraTrans[Y];

   mCameraRot[X] = -45.0;
   mCameraRotTarget[X] = mCameraRot[X];
   mCameraRot[Z] = 25.0;
   mCameraRotTarget[Z] = mCameraRot[Z];

   mUpdateTimer = new QTimer(this);
   connect(mUpdateTimer, SIGNAL(timeout()), this, SLOT(updateTick()));
   mUpdateTimer->start(100);
}

////////////////////////////////////////////////////////////////////////////////
VisualizerView::~VisualizerView()
{
   clearObjects();
   mUpdateTimer->stop();
   delete mUpdateTimer;

   glUseProgram(0);
}

////////////////////////////////////////////////////////////////////////////////
bool VisualizerView::addObject(GCodeObject* object)
{
   VisualizerObjectData objectData;
   objectData.object = object;

   int maxProgress = object->getLayerCount() - 1;
   if (mPrefs.useDisplayLists)
   {
      maxProgress *= 3;
   }
   else
   {
      maxProgress *= 2;
   }

   QProgressDialog progressDialog("Generating Geometry...", 0, 0, maxProgress, this);
   progressDialog.setWindowModality(Qt::WindowModal);
   progressDialog.setFixedSize(progressDialog.sizeHint());
   progressDialog.show();

   if (!generateGeometry(objectData, progressDialog))
   {
      freeBuffers(objectData);
      return false;
   }

   // If we are using display lists, then generate our display lists.
   if (mPrefs.useDisplayLists)
   {
      if (!genObject(objectData, progressDialog))
      {
         return false;
      }
   }

   mObjectList.push_back(objectData);

   updateGL();

   return true;
}

////////////////////////////////////////////////////////////////////////////////
void VisualizerView::removeObject(GCodeObject* object)
{
   if (!object)
   {
      return;
   }

   int objectCount = (int)mObjectList.size();
   for (int objectIndex = 0; objectIndex < objectCount; ++objectIndex)
   {
      VisualizerObjectData& data = mObjectList[objectIndex];
      if (data.object == object)
      {
         freeBuffers(data);
         mObjectList.erase(mObjectList.begin() + objectIndex);
         break;
      }
   }

   updateGL();
}

////////////////////////////////////////////////////////////////////////////////
void VisualizerView::clearObjects()
{
   int objectCount = (int)mObjectList.size();
   for (int objectIndex = 0; objectIndex < objectCount; ++objectIndex)
   {
      VisualizerObjectData& data = mObjectList[objectIndex];
      freeBuffers(data);
   }
   mObjectList.clear();

   updateGL();
}

////////////////////////////////////////////////////////////////////////////////
void VisualizerView::setShaderEnabled(bool enabled)
{
   if (enabled)
   {
      glUseProgram(mShaderProgram);
   }
   else
   {
      glUseProgram(0);
   }
}

////////////////////////////////////////////////////////////////////////////////
QSize VisualizerView::minimumSizeHint() const
{
   return QSize(50, 50);
}

////////////////////////////////////////////////////////////////////////////////
QSize VisualizerView::sizeHint() const
{
   return QSize(400, 400);
}

////////////////////////////////////////////////////////////////////////////////
void VisualizerView::setLayerDrawHeight(double height)
{
   mLayerDrawHeight = height;
   updateGL();
}

////////////////////////////////////////////////////////////////////////////////
bool VisualizerView::regenerateGeometry()
{
   bool result = true;

   // Count the total number of layers that we are generating so we can
   // properly estimate our progress bar.
   int maxProgress = 0;
   int objectCount = (int)mObjectList.size();
   for (int objectIndex = 0; objectIndex < objectCount; ++objectIndex)
   {
      VisualizerObjectData& objectData = mObjectList[objectIndex];
      maxProgress += objectData.object->getLayerCount() - 1;
   }

   if (mPrefs.useDisplayLists)
   {
      maxProgress *= 3;
   }
   else
   {
      maxProgress *= 2;
   }

   QProgressDialog progressDialog("Generating Geometry...", 0, 0, maxProgress, this);
   progressDialog.setWindowModality(Qt::WindowModal);
   progressDialog.setFixedSize(progressDialog.sizeHint());
   progressDialog.show();

   for (int objectIndex = 0; objectIndex < objectCount; ++objectIndex)
   {
      VisualizerObjectData& objectData = mObjectList[objectIndex];
      result &= generateGeometry(objectData, progressDialog);

      // If we are using display lists, then generate our display lists.
      if (mPrefs.useDisplayLists)
      {
         result &= genObject(objectData, progressDialog);
      }
   }

   updateGL();

   return result;
}

////////////////////////////////////////////////////////////////////////////////
const QString& VisualizerView::getError() const
{
   return mError;
}

////////////////////////////////////////////////////////////////////////////////
void VisualizerView::onBackgroundColorChanged(const QColor& color)
{
   qglClearColor(color);
   updateGL();
}

////////////////////////////////////////////////////////////////////////////////
void VisualizerView::setXTranslation(double pos)
{
   mCameraTransTarget[X] = pos;
   emit xTranslationChanged(pos);
   updateGL();
}

////////////////////////////////////////////////////////////////////////////////
void VisualizerView::setYTranslation(double pos)
{
   mCameraTransTarget[Y] = pos;
   emit yTranslationChanged(pos);
   updateGL();
}

////////////////////////////////////////////////////////////////////////////////
void VisualizerView::setZTranslation(double pos)
{
   mCameraTransTarget[Z] = pos;
   emit zTranslationChanged(pos);
   updateGL();
}

////////////////////////////////////////////////////////////////////////////////
static double getNormalizeAngle(double angle)
{
   double result = angle;
   while (result < 0.0)
      result += 360.0;
   while (result > 360.0)
      result -= 360.0;
   return result;
}

////////////////////////////////////////////////////////////////////////////////
void VisualizerView::setXRotation(double angle)
{
   if (angle != mCameraRotTarget[X])
   {
      mCameraRotTarget[X] = angle;
      emit xRotationChanged(angle);
      updateGL();
   }
}

////////////////////////////////////////////////////////////////////////////////
void VisualizerView::setYRotation(double angle)
{
   if (angle != mCameraRotTarget[Y])
   {
      mCameraRotTarget[Y] = angle;
      emit yRotationChanged(angle);
      updateGL();
   }
}

////////////////////////////////////////////////////////////////////////////////
void VisualizerView::setZRotation(double angle)
{
   if (angle != mCameraRotTarget[Z])
   {
      mCameraRotTarget[Z] = angle;
      emit zRotationChanged(angle);
      updateGL();
   }
}

////////////////////////////////////////////////////////////////////////////////
void VisualizerView::setZoom(double zoom)
{
   if (zoom > -1.0) zoom = -1.0f;

   mCameraZoomTarget = zoom;
   emit zoomChanged(zoom);
   updateGL();
}

////////////////////////////////////////////////////////////////////////////////
void VisualizerView::updateTick()
{
   if (updateCamera())
   {
      updateGL();
   }
}

////////////////////////////////////////////////////////////////////////////////
void VisualizerView::initializeGL()
{
   qglClearColor(mPrefs.backgroundColor);

   glEnable(GL_DEPTH_TEST);
   glEnable(GL_CULL_FACE);
   glEnable(GL_MULTISAMPLE);
   glEnable(GL_COLOR_MATERIAL);
   glEnable(GL_POLYGON_SMOOTH);
   glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
   glHint(GL_POLYGON_SMOOTH, GL_NICEST);

   static GLfloat diffuseLight[4]  = {1.0f, 1.0f, 1.0f, 1.0f};
   static GLfloat ambientLight[4]  = {0.0f, 0.0f, 0.0f, 1.0f};
   static GLfloat lightPosition[4] = {0.3f, 0.7f, 0.3f, 1.0f};

   glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
   glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
   glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

   // Setup our vertex shader.
   const static GLchar* vertCode = \
      "varying vec3 vertex_light_position;" \
      "varying vec3 vertex_normal;" \
      "void main()\n" \
      "{\n" \
      "   vertex_normal = normalize(gl_NormalMatrix * gl_Normal);\n" \
      "   vertex_light_position = normalize(gl_LightSource[0].position.xyz);\n" \
      "   gl_FrontColor = gl_Color;\n" \
      "   gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n" \
      "}\n";

   const static GLchar* fragCode = \
      "varying vec3 vertex_light_position;\n" \
      "varying vec3 vertex_normal;\n" \
      "void main()\n" \
      "{\n" \
      "   float diffuse_value = max(dot(vertex_normal, vertex_light_position), 0.0);\n" \
      "   gl_FragColor = gl_Color * diffuse_value;\n" \
      "}\n";

   mShaderProgram = glCreateProgram();
   GLint vert = glCreateShader(GL_VERTEX_SHADER);
   GLint frag = glCreateShader(GL_FRAGMENT_SHADER);

   glShaderSource(vert, 1, &vertCode, 0);
   glShaderSource(frag, 1, &fragCode, 0);
   glCompileShader(vert);
   glCompileShader(frag);
   glAttachShader(mShaderProgram, vert);
   glAttachShader(mShaderProgram, frag);
   glLinkProgram(mShaderProgram);
}

////////////////////////////////////////////////////////////////////////////////
bool VisualizerView::updateCamera()
{
   bool changed = false;
   double dist = 0.0;

   // Update camera translation towards target.
   for (int axis = 0; axis < AXIS_NUM_NO_E; ++axis)
   {
      dist = mCameraRotTarget[axis] - mCameraRot[axis];
      if (dist) changed = true;
      mCameraRot[axis] += dist / 4.0;

      dist = (mCameraTransTarget[axis] - mCameraTrans[axis]) / 2.0;
      if (dist) changed = true;
      mCameraTrans[axis] += dist;
   }
   dist = (mCameraZoomTarget - mCameraZoom) / 2.0;
   if (dist) changed = true;
   mCameraZoom += dist;

   return changed;
}

////////////////////////////////////////////////////////////////////////////////
void VisualizerView::paintGL()
{
   updateCamera();

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, mCameraZoom);

   glPushMatrix();

   // Apply camera rotation.
   glRotatef(mCameraRot[X], 1.0, 0.0, 0.0);
   glRotatef(mCameraRot[Y], 0.0, 1.0, 0.0);
   glRotatef(mCameraRot[Z], 0.0, 0.0, 1.0);

   // Apply camera translation.
   glTranslatef(-mCameraTrans[X], -mCameraTrans[Y], -mCameraTrans[Z]);

   int objectCount = (int)mObjectList.size();
   for (int objectIndex = 0; objectIndex < objectCount; ++objectIndex)
   {
      if (mPrefs.useDisplayLists)
      {
         callObject(mObjectList[objectIndex]);
      }
      else
      {
         drawObject(mObjectList[objectIndex]);
      }
   }

   drawPlatform();
   glPopMatrix();
}

////////////////////////////////////////////////////////////////////////////////
void VisualizerView::resizeGL(int width, int height)
{
   glViewport(0, 0, width, height);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   //glOrtho(-0.5, +0.5, -0.5, +0.5, 4.0, 15.0);

   // Set up perspective view.
   GLdouble xmin, xmax, ymin, ymax, aspect;

   aspect = width/(double)height;
   ymax = 0.1f * tan( 45.0f * M_PI / 360.0 );
   ymin = -ymax;
   xmin = ymin * aspect;
   xmax = ymax * aspect;

   glFrustum(xmin, xmax, ymin, ymax, 0.1, 1000.0);

   glMatrixMode(GL_MODELVIEW);
}

////////////////////////////////////////////////////////////////////////////////
void VisualizerView::mousePressEvent(QMouseEvent *event)
{
   mLastPos = event->pos();

   if (event->buttons() & Qt::RightButton)
   {
      double angle = getNormalizeAngle(mCameraRot[X]);
      if ((angle < -90 && angle > -270) ||
         (angle > 90 && angle < 270))
      {
         mCameraRotDirection = -1.0;
      }
      else
      {
         mCameraRotDirection = 1.0;
      }

      if (mCameraRotTarget[X] < -90 && mCameraRotTarget[X] >= -165)
      {
         mCameraRotDirection *= -1.0;
      }

      // Flip it yet again if the mouse is on the top half of the window.
      if ((mCameraRotTarget[X] > -15 || mCameraRotTarget[X] < -165) &&
         event->pos().y() < height() / 2)
      {
         mCameraRotDirection *= -1.0;
      }
   }
}

////////////////////////////////////////////////////////////////////////////////
void VisualizerView::mouseMoveEvent(QMouseEvent *event)
{
   int dx = event->x() - mLastPos.x();
   int dy = event->y() - mLastPos.y();

   if (event->buttons() & Qt::LeftButton)
   {
      QMatrix4x4 mat;
      QVector3D pos;

      pos.setX(0.001 * -dx * -mCameraZoom);
      pos.setY(0.001 * dy * -mCameraZoom);

      mat.rotate(mCameraRotTarget[X], 1.0, 0.0, 0.0);
      pos = pos * mat;

      mat.setToIdentity();
      mat.rotate(mCameraRotTarget[Z], 0.0, 0.0, 1.0);
      pos = pos * mat;

      setXTranslation(mCameraTransTarget[X] + pos.x());
      setYTranslation(mCameraTransTarget[Y] + pos.y());
      setZTranslation(mCameraTransTarget[Z] + pos.z());
   }
   if (event->buttons() & Qt::RightButton)
   {
      setZRotation(mCameraRotTarget[Z] + (dx * mCameraRotDirection * 0.5));
      setXRotation(mCameraRotTarget[X] + (dy * 0.5));
   }
   mLastPos = event->pos();
}

////////////////////////////////////////////////////////////////////////////////
void VisualizerView::wheelEvent(QWheelEvent* event)
{
   setZoom(mCameraZoomTarget + (-mCameraZoomTarget * event->delta() * 0.001));
}

////////////////////////////////////////////////////////////////////////////////
void VisualizerView::drawPlatform()
{
   QColor color = Qt::lightGray;

   // Platform grid.
   glLineWidth(1.0f);
   glBegin(GL_LINES);
   {
      glColor4f(color.redF(), color.greenF(), color.blueF(), 1.0f);

      for (double y = ((mPrefs.platformHeight / 20.0) - floor(mPrefs.platformHeight / 20.0)) * 10.0;
         y <= mPrefs.platformHeight; y += 10.0)
      {
         glVertex3d(0.0, y, 0.0);
         glVertex3d(mPrefs.platformWidth, y, 0.0);
      }

      for (double x = ((mPrefs.platformWidth / 20.0) - floor(mPrefs.platformWidth / 20.0)) * 10.0;
         x <= mPrefs.platformWidth; x += 10.0)
      {
         glVertex3d(x, 0.0, 0.0);
         glVertex3d(x, mPrefs.platformHeight, 0.0);
      }
   }
   glEnd();

   // Draw the grid border.
   color = Qt::white;
   glLineWidth(5.0f);
   glBegin(GL_LINES);
   {
      glColor4f(color.redF(), color.greenF(), color.blueF(), 1.0f);

      glVertex3d(0.0, 0.0, 0.0);
      glVertex3d(0.0, mPrefs.platformHeight, 0.0);

      glVertex3d(0.0, mPrefs.platformHeight, 0.0);
      glVertex3d(mPrefs.platformWidth, mPrefs.platformHeight, 0.0);

      glVertex3d(mPrefs.platformWidth, mPrefs.platformHeight, 0.0);
      glVertex3d(mPrefs.platformWidth, 0.0, 0.0);
   }
   glEnd();

   color = Qt::yellow;
   glLineWidth(8.0f);
   glBegin(GL_LINES);
   {
      glColor4f(color.redF(), color.greenF(), color.blueF(), 1.0f);

      glVertex3d(0.0, 0.0, 0.0);
      glVertex3d(mPrefs.platformWidth, 0.0, 0.0);
   }
   glEnd();

   glPopAttrib();
}

////////////////////////////////////////////////////////////////////////////////
bool VisualizerView::genObject(VisualizerObjectData& object, QProgressDialog& progressDialog)
{
   glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
   glPushAttrib(GL_LIGHTING_BIT);

   switch (mPrefs.drawQuality)
   {
   case DRAW_QUALITY_LOW:
      {
         glEnableClientState(GL_VERTEX_ARRAY);

         glLineWidth(1.0f);

         int layerCount = (int)object.layers.size();
         for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex)
         {
            VisualizerBufferData& buffer = object.layers[layerIndex];

            buffer.displayListIndex = glGenLists(1);

            if (buffer.displayListIndex == GL_INVALID_VALUE)
            {
               mError = "Failed to generate display lists.";
               return false;
            }

            glNewList(buffer.displayListIndex, GL_COMPILE);
            glVertexPointer(3, GL_DOUBLE, 0, buffer.vertexBuffer);
            glDrawArrays(GL_LINES, 0, buffer.vertexCount);
            glEndList();

            // Increment our progress bar.
            progressDialog.setValue(progressDialog.value() + 1 + mPrefs.layerSkipSize);
         }
      }
      break;

   case DRAW_QUALITY_MED:
      {
         glEnable(GL_SMOOTH);
         glShadeModel(GL_SMOOTH);

         glEnable(GL_LIGHTING);
         glEnable(GL_LIGHT0);

         glEnableClientState(GL_VERTEX_ARRAY);
         glEnableClientState(GL_NORMAL_ARRAY);

         int layerCount = (int)object.layers.size();
         for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex)
         {
            VisualizerBufferData& buffer = object.layers[layerIndex];

            buffer.displayListIndex = glGenLists(1);

            if (buffer.displayListIndex == GL_INVALID_VALUE)
            {
               mError = "Failed to generate display lists.";
               return false;
            }

            glNewList(buffer.displayListIndex, GL_COMPILE);
            glVertexPointer(3, GL_DOUBLE, 0, buffer.vertexBuffer);
            glNormalPointer(GL_DOUBLE, 0, buffer.normalBuffer);
            glDrawElements(GL_QUADS, buffer.quadCount * 4, GL_UNSIGNED_INT, buffer.indexBuffer);
            glEndList();

            // Increment our progress bar.
            progressDialog.setValue(progressDialog.value() + 1 + mPrefs.layerSkipSize);
         }
      }
      break;

   case DRAW_QUALITY_HIGH:
      {
         glEnable(GL_SMOOTH);
         glShadeModel(GL_SMOOTH);

         glEnable(GL_LIGHTING);
         glEnable(GL_LIGHT0);

         glEnableClientState(GL_VERTEX_ARRAY);
         glEnableClientState(GL_NORMAL_ARRAY);

         int layerCount = (int)object.layers.size();
         for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex)
         {
            VisualizerBufferData& buffer = object.layers[layerIndex];

            buffer.displayListIndex = glGenLists(1);

            if (buffer.displayListIndex == GL_INVALID_VALUE)
            {
               mError = "Failed to generate display lists.";
               return false;
            }

            glNewList(buffer.displayListIndex, GL_COMPILE);
            glVertexPointer(3, GL_DOUBLE, 0, buffer.vertexBuffer);
            glNormalPointer(GL_DOUBLE, 0, buffer.normalBuffer);
            glDrawArrays(GL_QUADS, 0, buffer.vertexCount);
            glEndList();

            // Increment our progress bar.
            progressDialog.setValue(progressDialog.value() + 1 + mPrefs.layerSkipSize);
         }
      }
      break;
   }

   glPopAttrib();
   glPopClientAttrib();

   return true;
}

////////////////////////////////////////////////////////////////////////////////
void VisualizerView::callObject(const VisualizerObjectData& object)
{
   glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
   glPushAttrib(GL_LIGHTING_BIT);
   glPushMatrix();

   const double* offset = object.object->getOffsetPos();
   glTranslated(offset[0], offset[1], offset[2]);

   int extruderIndex = object.object->getExtruder();
   if (extruderIndex < 0 || extruderIndex >= (int)mPrefs.extruderList.size())
   {
      // Default to extruder index 0 if our desired index is out of bounds.
      extruderIndex = 0;
   }

   glColor4d(mPrefs.extruderList[extruderIndex].color.redF(),
             mPrefs.extruderList[extruderIndex].color.greenF(),
             mPrefs.extruderList[extruderIndex].color.blueF(),
             1.0);


   switch (mPrefs.drawQuality)
   {
   case DRAW_QUALITY_LOW:
      {
         glEnableClientState(GL_VERTEX_ARRAY);

         glLineWidth(1.0f);

         int layerCount = (int)object.layers.size();
         for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex)
         {
            const VisualizerBufferData& buffer = object.layers[layerIndex];

            if (buffer.height + object.object->getOffsetPos()[Z] <= mLayerDrawHeight)
            {
               // If this layer is at the top, render it with a slightly darker color.
               if (layerIndex < layerCount - 1 && buffer.height + object.object->getOffsetPos()[Z] > mLayerDrawHeight - object.object->getAverageLayerHeight())
               {
                  QColor darker = mPrefs.extruderList[extruderIndex].color.dark();
                  glColor4d(darker.redF(), darker.greenF(), darker.blueF(), 1.0);
               }

               glCallList(buffer.displayListIndex);
            }
            else
            {
               break;
            }
         }
      }
      break;

   case DRAW_QUALITY_MED:
      {
         glEnable(GL_SMOOTH);
         glShadeModel(GL_SMOOTH);

         glEnable(GL_LIGHTING);
         glEnable(GL_LIGHT0);

         glEnableClientState(GL_VERTEX_ARRAY);
         glEnableClientState(GL_NORMAL_ARRAY);

         int layerCount = (int)object.layers.size();
         for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex)
         {
            const VisualizerBufferData& buffer = object.layers[layerIndex];

            if (buffer.height + object.object->getOffsetPos()[Z] <= mLayerDrawHeight)
            {
               // If this layer is at the top, render it with a slightly darker color.
               if (layerIndex < layerCount - 1 && buffer.height + object.object->getOffsetPos()[Z] > mLayerDrawHeight - object.object->getAverageLayerHeight())
               {
                  QColor darker = mPrefs.extruderList[extruderIndex].color.dark();
                  glColor4d(darker.redF(), darker.greenF(), darker.blueF(), 1.0);
               }

               glCallList(buffer.displayListIndex);
            }
            else
            {
               break;
            }
         }
      }
      break;

   case DRAW_QUALITY_HIGH:
      {
         glEnable(GL_SMOOTH);
         glShadeModel(GL_SMOOTH);

         glEnable(GL_LIGHTING);
         glEnable(GL_LIGHT0);

         glEnableClientState(GL_VERTEX_ARRAY);
         glEnableClientState(GL_NORMAL_ARRAY);

         int layerCount = (int)object.layers.size();
         for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex)
         {
            const VisualizerBufferData& buffer = object.layers[layerIndex];

            if (buffer.height + object.object->getOffsetPos()[Z] <= mLayerDrawHeight)
            {
               // If this layer is at the top, render it with a slightly darker color.
               if (layerIndex < layerCount - 1 && buffer.height + object.object->getOffsetPos()[Z] > mLayerDrawHeight - object.object->getAverageLayerHeight())
               {
                  QColor darker = mPrefs.extruderList[extruderIndex].color.dark();
                  glColor4d(darker.redF(), darker.greenF(), darker.blueF(), 1.0);
               }

               glCallList(buffer.displayListIndex);
            }
            else
            {
               break;
            }
         }
      }
      break;
   }

   glPopMatrix();
   glPopAttrib();
   glPopClientAttrib();
}

////////////////////////////////////////////////////////////////////////////////
void VisualizerView::drawObject(const VisualizerObjectData& object)
{
   glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
   glPushAttrib(GL_LIGHTING_BIT);
   glPushMatrix();

   const double* offset = object.object->getOffsetPos();
   glTranslated(offset[0], offset[1], offset[2]);

   int extruderIndex = object.object->getExtruder();
   if (extruderIndex < 0 || extruderIndex >= (int)mPrefs.extruderList.size())
   {
      // Default to extruder index 0 if our desired index is out of bounds.
      extruderIndex = 0;
   }

   glColor4d(mPrefs.extruderList[extruderIndex].color.redF(),
      mPrefs.extruderList[extruderIndex].color.greenF(),
      mPrefs.extruderList[extruderIndex].color.blueF(),
      1.0);

   switch (mPrefs.drawQuality)
   {
   case DRAW_QUALITY_LOW:
      {
         glEnableClientState(GL_VERTEX_ARRAY);

         glLineWidth(1.0f);

         int layerCount = (int)object.layers.size();
         for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex)
         {
            const VisualizerBufferData& buffer = object.layers[layerIndex];

            if (buffer.height + object.object->getOffsetPos()[Z] <= mLayerDrawHeight)
            {
               // If this layer is at the top, render it with a slightly darker color.
               if (layerIndex < layerCount - 1 && buffer.height + object.object->getOffsetPos()[Z] > mLayerDrawHeight - object.object->getAverageLayerHeight())
               {
                  QColor darker = mPrefs.extruderList[extruderIndex].color.dark();
                  glColor4d(darker.redF(), darker.greenF(), darker.blueF(), 1.0);
               }
               glVertexPointer(3, GL_DOUBLE, 0, buffer.vertexBuffer);
               glDrawArrays(GL_LINES, 0, buffer.vertexCount);
            }
            else
            {
               break;
            }
         }
      }
      break;

   case DRAW_QUALITY_MED:
      {
         glEnable(GL_SMOOTH);
         glShadeModel(GL_SMOOTH);

         glEnable(GL_LIGHTING);
         glEnable(GL_LIGHT0);

         glEnableClientState(GL_VERTEX_ARRAY);
         glEnableClientState(GL_NORMAL_ARRAY);

         int layerCount = (int)object.layers.size();
         for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex)
         {
            const VisualizerBufferData& buffer = object.layers[layerIndex];

            if (buffer.height + object.object->getOffsetPos()[Z] <= mLayerDrawHeight)
            {
               // If this layer is at the top, render it with a slightly darker color.
               if (layerIndex < layerCount - 1 && buffer.height + object.object->getOffsetPos()[Z] > mLayerDrawHeight - object.object->getAverageLayerHeight())
               {
                  QColor darker = mPrefs.extruderList[extruderIndex].color.dark();
                  glColor4d(darker.redF(), darker.greenF(), darker.blueF(), 1.0);
               }
               glVertexPointer(3, GL_DOUBLE, 0, buffer.vertexBuffer);
               glNormalPointer(GL_DOUBLE, 0, buffer.normalBuffer);
               glDrawElements(GL_QUADS, buffer.quadCount * 4, GL_UNSIGNED_INT, buffer.indexBuffer);
            }
            else
            {
               break;
            }
         }
      }
      break;

   case DRAW_QUALITY_HIGH:
      {
         glEnable(GL_SMOOTH);
         glShadeModel(GL_SMOOTH);

         glEnable(GL_LIGHTING);
         glEnable(GL_LIGHT0);

         glEnableClientState(GL_VERTEX_ARRAY);
         glEnableClientState(GL_NORMAL_ARRAY);

         int layerCount = (int)object.layers.size();
         for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex)
         {
            const VisualizerBufferData& buffer = object.layers[layerIndex];

            if (buffer.height + object.object->getOffsetPos()[Z] <= mLayerDrawHeight)
            {
               // If this layer is at the top, render it with a slightly darker color.
               if (layerIndex < layerCount - 1 && buffer.height + object.object->getOffsetPos()[Z] > mLayerDrawHeight - object.object->getAverageLayerHeight())
               {
                  QColor darker = mPrefs.extruderList[extruderIndex].color.dark();
                  glColor4d(darker.redF(), darker.greenF(), darker.blueF(), 1.0);
               }
               glVertexPointer(3, GL_DOUBLE, 0, buffer.vertexBuffer);
               glNormalPointer(GL_DOUBLE, 0, buffer.normalBuffer);
               glDrawArrays(GL_QUADS, 0, buffer.vertexCount);
            }
            else
            {
               break;
            }
         }
      }
      break;
   }

   glPopMatrix();
   glPopAttrib();
   glPopClientAttrib();
}

////////////////////////////////////////////////////////////////////////////////
bool VisualizerView::generateGeometry(VisualizerObjectData& data, QProgressDialog& progressDialog)
{
   if (!data.object)
   {
      mError = "No object to generate geometry for.";
      return false;
   }

   freeBuffers(data);

   // Before we can allocate memory for our vertex buffers, we first need to
   // determine exactly how many vertices we need.
   int skipCount = mPrefs.layerSkipSize + 1;
   double lastE = 0.0;
   int levelCount = data.object->getLayerCount();
   for (int levelIndex = 1; levelIndex < levelCount; ++levelIndex)
   {
      const LayerData& layerData = data.object->getLayer(levelIndex);

      // Skip layers if necessary.
      if (mPrefs.layerSkipSize > 0)
      {
         skipCount++;

         if (skipCount <= mPrefs.layerSkipSize)
         {
            continue;
         }

         skipCount = 0;
      }

      VisualizerBufferData buffer;
      buffer.height = layerData.height;

      const std::vector<GCodeCommand>& codes = layerData.codes;

      int codeCount = (int)codes.size();
      for (int codeIndex = 0; codeIndex < codeCount; ++codeIndex)
      {
         const GCodeCommand& code = codes[codeIndex];

         if (code.hasAxis)
         {
            // We only draw a line segment if we are extruding filament on this line.
            if (code.axisValue[E] != 0.0 && lastE + code.axisValue[E] > 0.0)
            {
               switch (mPrefs.drawQuality)
               {
               case DRAW_QUALITY_LOW:
                  {
                     // The line segment will consist of a single line between two points.
                     buffer.vertexCount += 2;
                  }
                  break;
               case DRAW_QUALITY_MED:
                  {
                     // The line segment will consist of 8 points that
                     // form together to make two flat quads.
                     buffer.vertexCount += 8;
                     buffer.quadCount += 4;
                  }
                  break;
               case DRAW_QUALITY_HIGH:
                  {
                     // The line segment will consist of four quads, each
                     // using their own four points so they can have their
                     // own normal values.
                     buffer.vertexCount += 4 * 4;
                  }
                  break;
               }
            }

            lastE += code.axisValue[E];
            if (lastE > 0.0)
            {
               lastE = 0.0;
            }
         }
      }

      // Increment our progress bar.
      progressDialog.setValue(progressDialog.value() + 1 + mPrefs.layerSkipSize);

      data.layers.push_back(buffer);
   }

   if (data.layers.size() > 0)
   {
      skipCount = mPrefs.layerSkipSize + 1;

      double radius = data.object->getAverageLayerHeight() * 0.5;
      QVector3D up = QVector3D(0.0, 0.0, 1.0);

      double lastPos[AXIS_NUM] = {0.0,};

      // Now fill in our newly allocated buffer space.
      int bufferIndex = 0;

      for (int levelIndex = 1; levelIndex < levelCount; ++levelIndex)
      {
         const LayerData& layerData = data.object->getLayer(levelIndex);

         // Skip layers if necessary.
         if (mPrefs.layerSkipSize > 0)
         {
            skipCount++;

            if (skipCount <= mPrefs.layerSkipSize)
            {
               continue;
            }

            skipCount = 0;
         }

         if (bufferIndex >= (int)data.layers.size())
         {
            mError = "Failed to configure enough layers.";
            return false;
         }

         VisualizerBufferData& buffer = data.layers[bufferIndex++];

         // Allocate memory for this layer.
         try
         {
            switch (mPrefs.drawQuality)
            {
            case DRAW_QUALITY_MED:
               {
                  buffer.indexBuffer = new unsigned int[buffer.quadCount * 4];
               }
            case DRAW_QUALITY_HIGH:
               {
                  buffer.normalBuffer = new double[buffer.vertexCount * 3];
               }
            case DRAW_QUALITY_LOW:
               {
                  buffer.vertexBuffer = new double[buffer.vertexCount * 3];
               }
            }
         }
         catch (...)
         {
            mError = "Memory allocation error.";
            data.layers.erase(data.layers.begin() + bufferIndex - 1, data.layers.end());
            return false;
         }

         int pointIndex = 0;
         int normalIndex = 0;
         int quadIndex = 0;

         const std::vector<GCodeCommand>& codes = layerData.codes;
         int codeCount = (int)codes.size();
         for (int codeIndex = 0; codeIndex < codeCount; ++codeIndex)
         {
            const GCodeCommand& code = codes[codeIndex];

            if (code.hasAxis)
            {
               // We only draw a line segment if we are extruding filament on this line.
               if (code.axisValue[E] != 0.0 && lastPos[E] + code.axisValue[E] > 0.0)
               {
                  QVector3D p1 = QVector3D(lastPos[X], lastPos[Y], lastPos[Z]);
                  QVector3D p2 = QVector3D(code.axisValue[X], code.axisValue[Y], code.axisValue[Z]);

                  switch (mPrefs.drawQuality)
                  {
                  case DRAW_QUALITY_LOW:
                     {
                        addGeometryPoint(&buffer.vertexBuffer[pointIndex], pointIndex, p1);
                        addGeometryPoint(&buffer.vertexBuffer[pointIndex], pointIndex, p2);
                     }
                     break;

                  case DRAW_QUALITY_MED:
                     {
                        // Set up a rotation matrix
                        QMatrix4x4 rot;
                        rot.lookAt(p2 - p1, QVector3D(0.0, 0.0, 0.0), up);

                        QVector3D right;
                        right.setX(1.0);
                        right = right * rot;

                        QVector3D vec = (p2 - p1).normalized();
                        QVector3D norm;

                        int vertexIndex = pointIndex / 3;

                        // Generate our quads.
                        // Left
                        buffer.indexBuffer[quadIndex + 0] = vertexIndex + POINT_FIRST_TOP_LEFT;
                        buffer.indexBuffer[quadIndex + 1] = vertexIndex + POINT_FIRST_BOT_LEFT;
                        buffer.indexBuffer[quadIndex + 2] = vertexIndex + POINT_SECOND_BOT_LEFT;
                        buffer.indexBuffer[quadIndex + 3] = vertexIndex + POINT_SECOND_TOP_LEFT;
                        quadIndex += 4;

                        // Top
                        buffer.indexBuffer[quadIndex + 0] = vertexIndex + POINT_FIRST_TOP_RIGHT;
                        buffer.indexBuffer[quadIndex + 1] = vertexIndex + POINT_FIRST_TOP_LEFT;
                        buffer.indexBuffer[quadIndex + 2] = vertexIndex + POINT_SECOND_TOP_LEFT;
                        buffer.indexBuffer[quadIndex + 3] = vertexIndex + POINT_SECOND_TOP_RIGHT;
                        quadIndex += 4;

                        // Right
                        buffer.indexBuffer[quadIndex + 0] = vertexIndex + POINT_FIRST_BOT_RIGHT;
                        buffer.indexBuffer[quadIndex + 1] = vertexIndex + POINT_FIRST_TOP_RIGHT;
                        buffer.indexBuffer[quadIndex + 2] = vertexIndex + POINT_SECOND_TOP_RIGHT;
                        buffer.indexBuffer[quadIndex + 3] = vertexIndex + POINT_SECOND_BOT_RIGHT;
                        quadIndex += 4;

                        // Bottom
                        buffer.indexBuffer[quadIndex + 0] = vertexIndex + POINT_FIRST_BOT_LEFT;
                        buffer.indexBuffer[quadIndex + 1] = vertexIndex + POINT_FIRST_BOT_RIGHT;
                        buffer.indexBuffer[quadIndex + 2] = vertexIndex + POINT_SECOND_BOT_RIGHT;
                        buffer.indexBuffer[quadIndex + 3] = vertexIndex + POINT_SECOND_BOT_LEFT;
                        quadIndex += 4;

                        // Generate our 8 vertices for this rectangle segment.
                        addGeometryPoint(&buffer.vertexBuffer[pointIndex], pointIndex, p1 + up * radius - right * radius - vec * radius);
                        addGeometryPoint(&buffer.vertexBuffer[pointIndex], pointIndex, p1 + up * radius + right * radius - vec * radius);
                        addGeometryPoint(&buffer.vertexBuffer[pointIndex], pointIndex, p1 - up * radius - right * radius - vec * radius);
                        addGeometryPoint(&buffer.vertexBuffer[pointIndex], pointIndex, p1 - up * radius + right * radius - vec * radius);

                        addGeometryPoint(&buffer.vertexBuffer[pointIndex], pointIndex, p2 + up * radius * 0.95 - right * radius + vec * radius);
                        addGeometryPoint(&buffer.vertexBuffer[pointIndex], pointIndex, p2 + up * radius * 0.95 + right * radius + vec * radius);
                        addGeometryPoint(&buffer.vertexBuffer[pointIndex], pointIndex, p2 - up * radius * 0.95 - right * radius + vec * radius);
                        addGeometryPoint(&buffer.vertexBuffer[pointIndex], pointIndex, p2 - up * radius * 0.95 + right * radius + vec * radius);

                        // Generate our 8 vertex normal values.
                        addGeometryPoint(&buffer.normalBuffer[normalIndex], normalIndex, (up - right - vec).normalized());
                        addGeometryPoint(&buffer.normalBuffer[normalIndex], normalIndex, (up + right - vec).normalized());
                        addGeometryPoint(&buffer.normalBuffer[normalIndex], normalIndex, (-up - right - vec).normalized());
                        addGeometryPoint(&buffer.normalBuffer[normalIndex], normalIndex, (-up + right - vec).normalized());

                        addGeometryPoint(&buffer.normalBuffer[normalIndex], normalIndex, (up - right + vec).normalized());
                        addGeometryPoint(&buffer.normalBuffer[normalIndex], normalIndex, (up + right + vec).normalized());
                        addGeometryPoint(&buffer.normalBuffer[normalIndex], normalIndex, (-up - right + vec).normalized());
                        addGeometryPoint(&buffer.normalBuffer[normalIndex], normalIndex, (-up + right + vec).normalized());
                     }
                     break;

                  case DRAW_QUALITY_HIGH:
                     {
                        // Set up a rotation matrix
                        QMatrix4x4 rot;
                        rot.lookAt(p2 - p1, QVector3D(0.0, 0.0, 0.0), up);

                        QVector3D right;
                        right.setX(1.0);
                        right = right * rot;

                        QVector3D vec = (p2 - p1).normalized();
                        QVector3D norm;

                        int vertexIndex = pointIndex / 3;

                        // Left.
                        addGeometryPoint(&buffer.vertexBuffer[pointIndex], pointIndex, p1 + up * radius - right * radius - vec * radius);
                        addGeometryPoint(&buffer.vertexBuffer[pointIndex], pointIndex, p1 - up * radius - right * radius - vec * radius);
                        addGeometryPoint(&buffer.vertexBuffer[pointIndex], pointIndex, p2 - up * radius * 0.95 - right * radius + vec * radius);
                        addGeometryPoint(&buffer.vertexBuffer[pointIndex], pointIndex, p2 + up * radius * 0.95 - right * radius + vec * radius);

                        addGeometryPoint(&buffer.normalBuffer[normalIndex], normalIndex, -right);
                        addGeometryPoint(&buffer.normalBuffer[normalIndex], normalIndex, -right);
                        addGeometryPoint(&buffer.normalBuffer[normalIndex], normalIndex, -right);
                        addGeometryPoint(&buffer.normalBuffer[normalIndex], normalIndex, -right);

                        // Top.
                        addGeometryPoint(&buffer.vertexBuffer[pointIndex], pointIndex, p1 + up * radius + right * radius - vec * radius);
                        addGeometryPoint(&buffer.vertexBuffer[pointIndex], pointIndex, p1 + up * radius - right * radius - vec * radius);
                        addGeometryPoint(&buffer.vertexBuffer[pointIndex], pointIndex, p2 + up * radius * 0.95 - right * radius + vec * radius);
                        addGeometryPoint(&buffer.vertexBuffer[pointIndex], pointIndex, p2 + up * radius * 0.95 + right * radius + vec * radius);

                        addGeometryPoint(&buffer.normalBuffer[normalIndex], normalIndex, up);
                        addGeometryPoint(&buffer.normalBuffer[normalIndex], normalIndex, up);
                        addGeometryPoint(&buffer.normalBuffer[normalIndex], normalIndex, up);
                        addGeometryPoint(&buffer.normalBuffer[normalIndex], normalIndex, up);

                        // Right.
                        addGeometryPoint(&buffer.vertexBuffer[pointIndex], pointIndex, p1 - up * radius + right * radius - vec * radius);
                        addGeometryPoint(&buffer.vertexBuffer[pointIndex], pointIndex, p1 + up * radius + right * radius - vec * radius);
                        addGeometryPoint(&buffer.vertexBuffer[pointIndex], pointIndex, p2 + up * radius * 0.95 + right * radius + vec * radius);
                        addGeometryPoint(&buffer.vertexBuffer[pointIndex], pointIndex, p2 - up * radius * 0.95 + right * radius + vec * radius);

                        addGeometryPoint(&buffer.normalBuffer[normalIndex], normalIndex, right);
                        addGeometryPoint(&buffer.normalBuffer[normalIndex], normalIndex, right);
                        addGeometryPoint(&buffer.normalBuffer[normalIndex], normalIndex, right);
                        addGeometryPoint(&buffer.normalBuffer[normalIndex], normalIndex, right);

                        // Bottom.
                        addGeometryPoint(&buffer.vertexBuffer[pointIndex], pointIndex, p1 - up * radius - right * radius - vec * radius);
                        addGeometryPoint(&buffer.vertexBuffer[pointIndex], pointIndex, p1 - up * radius + right * radius - vec * radius);
                        addGeometryPoint(&buffer.vertexBuffer[pointIndex], pointIndex, p2 - up * radius * 0.95 + right * radius + vec * radius);
                        addGeometryPoint(&buffer.vertexBuffer[pointIndex], pointIndex, p2 - up * radius * 0.95 - right * radius + vec * radius);

                        addGeometryPoint(&buffer.normalBuffer[normalIndex], normalIndex, -up);
                        addGeometryPoint(&buffer.normalBuffer[normalIndex], normalIndex, -up);
                        addGeometryPoint(&buffer.normalBuffer[normalIndex], normalIndex, -up);
                        addGeometryPoint(&buffer.normalBuffer[normalIndex], normalIndex, -up);
                     }
                     break;
                  }
               }

               for (int axis = 0; axis < AXIS_NUM_NO_E; ++axis)
               {
                  lastPos[axis] = code.axisValue[axis];
               }

               lastPos[E] += code.axisValue[E];
               if (lastPos[E] > 0.0)
               {
                  lastPos[E] = 0.0;
               }
            }
         }

         // Increment our progress bar.
         progressDialog.setValue(progressDialog.value() + 1 + mPrefs.layerSkipSize);

         // Just a simple check to make sure we actually used the proper number of vertices.
         if (buffer.vertexCount * 3 != pointIndex ||
             (mPrefs.drawQuality != DRAW_QUALITY_LOW &&
             (buffer.vertexCount * 3 != normalIndex ||
             buffer.quadCount * 4 != quadIndex)))
         {
            mError = "Checksum failure with geometry generation.";
            return false;
         }
      }
   }

   return true;
}

////////////////////////////////////////////////////////////////////////////////
void VisualizerView::addGeometryPoint(double* buffer, int& index, const QVector3D& point)
{
   if (!buffer)
   {
      return;
   }

   buffer[0] = point.x();
   buffer[1] = point.y();
   buffer[2] = point.z();

   index += 3;
}

////////////////////////////////////////////////////////////////////////////////
void VisualizerView::freeBuffers(VisualizerObjectData& data)
{
   int layerCount = (int)data.layers.size();
   for (int layerIndex = 0; layerIndex < layerCount; ++layerIndex)
   {
      VisualizerBufferData& buffer = data.layers[layerIndex];

      if (buffer.displayListIndex != GL_INVALID_VALUE)
      {
         glDeleteLists(buffer.displayListIndex, 1);
      }

      buffer.free();
   }

   data.layers.clear();
}

////////////////////////////////////////////////////////////////////////////////
