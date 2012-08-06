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


#include <GCodeSplicer.h>
#include <GCodeParser.h>
#include <GCodeObject.h>

#include <QtGui/QtGui>

////////////////////////////////////////////////////////////////////////////////
GCodeSplicer::GCodeSplicer(const PreferenceData& prefs)
   : mPrefs(prefs)
{
}

////////////////////////////////////////////////////////////////////////////////
GCodeSplicer::~GCodeSplicer()
{
}

////////////////////////////////////////////////////////////////////////////////
bool GCodeSplicer::addObject(const GCodeObject* object)
{
   if (!object)
   {
      return false;
   }

   // First make sure we don't add the same object twice.
   int count = (int)mObjectList.size();
   for (int index = 0; index < count; ++index)
   {
      if (object == mObjectList[index])
      {
         return false;
      }
   }

   // Now insert the new object into the list, sorted by extruder index.
   for (int index = 0; index < count; ++index)
   {
      const GCodeObject* other = mObjectList[index];
      if (other && other->getExtruder() > object->getExtruder())
      {
         mObjectList.insert(mObjectList.begin() + index, object);
         return true;
      }
   }

   // If we get here, it means the object has not been entered into a
   // sorted position into the list.  This can be either because
   // the list was empty or the new object extruder value was higher
   // than everything already in the list.  Therefore, any objects
   // that have not been inserted yet go to the end.
   mObjectList.push_back(object);

   return true;
}

////////////////////////////////////////////////////////////////////////////////
bool GCodeSplicer::build(const QString& fileName, QWidget* parent)
{
   if (mObjectList.empty())
   {
      mError = "No objects to export.";
      return false;
   }

   QFile file;
   file.setFileName(fileName);
   if (!file.open(QIODevice::WriteOnly))
   {
      mError = "Could not open file \'" + fileName + "\' for writing.";
      return false;
   }

   QProgressDialog progressDialog("Splicing...", "Cancel", 0, 100, parent);
   progressDialog.setWindowModality(Qt::WindowModal);
   progressDialog.setFixedSize(progressDialog.sizeHint());
   progressDialog.show();

   if (!buildHeader(file))
   {
      mError = "Failed to build header codes for splice.";
      return false;
   }

   double currentPos[AXIS_NUM] = {0.0,};

   double currentLayerHeight = 0.0;
   int lastExtruder = 0;
   int layerIndex = 1;
   bool initExtruders = true;

   progressDialog.setMaximum(getTotalLayerCount());

   // Iterate through each layer.
   while (getNextLayer(currentLayerHeight, currentLayerHeight))
   {
      if (mPrefs.exportComments)
      {
         file.write("; ++++++++++++++++++++++++++++++++++++++\n; Begin Layer ");
         file.write(QString::number(layerIndex).toAscii());
         file.write(" with height = ");
         file.write(QString::number(currentLayerHeight).toAscii());
         file.write("\n; ++++++++++++++++++++++++++++++++++++++\n");
      }

      int currentExtruder = lastExtruder;
      double offset[AXIS_NUM] = {0,};

      file.write("G92 E0");
      if (mPrefs.exportComments) file.write("; Reset extrusion");
      file.write("\n");
      offset[E] = 0.0;

      // Iterate through each extruder.  We try to start with the last
      // extruder we used previously in an attempt to reduce the total
      // number of extruder changes done throughout the print.
      for (int extruderIndex = 0; extruderIndex < (int)mPrefs.extruderList.size(); ++extruderIndex)
      {
         int objectCount = (int)mObjectList.size();
         for (int objectIndex = 0; objectIndex < objectCount; ++objectIndex)
         {
            const GCodeObject* object = mObjectList[objectIndex];
            if (object && (initExtruders || object->getExtruder() == currentExtruder))
            {
               // If we are still waiting to initialize our extruders,
               // we need to set them up to be idle except for the
               // one we are starting the print with.
               if (initExtruders)
               {
                  initExtruders = false;
                  currentExtruder = object->getExtruder();

                  if (!buildExtruderInit(file, currentExtruder))
                  {
                     mError = "Failed to build extruder initialization code.";
                     return false;
                  }
               }
               std::vector<GCodeCommand> layer;
               object->getLayerAtHeight(layer, currentLayerHeight);

               // If we found some codes for this layer using our current extruder...
               if (!layer.empty())
               {
                  // Begin by processing the extruder change if necessary.
                  if (lastExtruder != currentExtruder)
                  {
                     if (!buildExtruderSwap(file, lastExtruder, currentExtruder, offset[E]))
                     {
                        mError = "Failed to build extruder swap code.";
                        return false;
                     }

                     lastExtruder = currentExtruder;
                  }

                  // Setup the offset based on the offset of the current extruder
                  // and the offset position to place the object.
                  for (int axis = 0; axis < AXIS_NUM_NO_E; ++axis)
                  {
                     offset[axis] = object->getOffsetPos()[axis] + mPrefs.extruderList[lastExtruder].offset[axis];
                  }

                  int codeCount = (int)layer.size();
                  for (int codeIndex = 0; codeIndex < codeCount; ++codeIndex)
                  {
                     const GCodeCommand& code = layer[codeIndex];

                     if (code.type == GCODE_EXTRUDER_MOVEMENT0 ||
                         code.type == GCODE_EXTRUDER_MOVEMENT1)
                     {
                        buildExtruderMovement(file, code, currentExtruder, offset, currentPos);
                     }
                     // Commands to skip.
                     else if (code.type == GCODE_HOME ||
                        code.type == MCODE_DISABLE_STEPPERS)
                     {
                        continue;
                     }
                     else
                     {
                        file.write(code.command.toAscii());
                     }

                     if (mPrefs.exportComments) file.write(code.comment.toAscii());
                     file.write("\n");
                  }
               }
            }
         }

         // Iterate to the next extruder.
         currentExtruder++;
         if (currentExtruder >= (int)mPrefs.extruderList.size())
         {
            currentExtruder = 0;
         }
      }
   
      layerIndex++;

      progressDialog.setValue(layerIndex);
      if (progressDialog.wasCanceled())
      {
         mError = "";
         file.close();
         return false;
      }
   }

   // Now cool down all of our extruders and disable motors on the printer.
   int extruderCount = (int)mPrefs.extruderList.size();
   for (int extruderIndex = 0; extruderIndex < extruderCount; ++extruderIndex)
   {
      const ExtruderData& extruder = mPrefs.extruderList[extruderIndex];

      file.write("T");
      file.write(QString::number(extruderIndex).toAscii());
      file.write("\n");

      file.write("M104 S0\n");
   }

   file.write("M84");
   if (mPrefs.exportComments) file.write("; Disable motors");
   file.write("\n");

   // Now supply the custom end code.
   if (!mPrefs.postfixCode.isEmpty())
   {
      file.write(mPrefs.postfixCode.toAscii());
      file.write("\n");
   }

   file.close();

   return true;
}

////////////////////////////////////////////////////////////////////////////////
bool GCodeSplicer::buildHeader(QFile& file)
{
   file.write("; Spliced using LocheGSplicer ");
   file.write(VERSION.toAscii());
   file.write("\n");

   // Start by assembling the initialization code.  Start with
   // the header code from the first object, then include our
   // custom header code, and last include our final settings.
   const GCodeObject* object = mObjectList[0];
   if (object && object->getLayerCount())
   {
      const std::vector<GCodeCommand>& header = object->getLayer(0).codes;
      int count = (int)header.size();
      for (int index = 0; index < count; ++index)
      {
         const GCodeCommand& code = header[index];

         // We only care about certain codes.
         if (code.type == GCODE_COMMENT ||
            code.type == GCODE_DWELL ||
            code.type == GCODE_HOME ||
            code.type == MCODE_FAN_ENABLE ||
            code.type == MCODE_FAN_DISABLE)
         {
            file.write(code.command.toAscii());

            if (mPrefs.exportComments && !code.comment.isEmpty())
            {
               file.write(code.comment.toAscii());
            }
            file.write("\n");
         }
      }
   }

   if (!mPrefs.prefixCode.isEmpty())
   {
      file.write(mPrefs.prefixCode.toAscii());
      file.write("\n");
   }

   file.write("G21");
   if (mPrefs.exportComments) file.write("; Set units to millimeters");
   file.write("\n");

   // Now append our constant header codes.
   if (mPrefs.exportAbsoluteMode)
   {
      file.write("G90");
      if (mPrefs.exportComments) file.write("; Use absolute coordinates");
   }
   else
   {
      file.write("G91");
      if (mPrefs.exportComments) file.write("; Use relative coordinates");
   }
   file.write("\n");

   if (mPrefs.exportAbsoluteEMode)
   {
      file.write("M82");
      if (mPrefs.exportComments) file.write("; Use absolute E coordinates");
   }
   else
   {
      file.write("M83");
      if (mPrefs.exportComments) file.write("; Use relative E coordinates");
   }
   file.write("\n");
   return true;
}

////////////////////////////////////////////////////////////////////////////////
bool GCodeSplicer::buildExtruderInit(QFile& file, int currentExtruder)
{
   // Set all extruders to print temp first so we can
   // retract them.  Then put the all of them to idle
   // except the one we are going to print with.
   if (mPrefs.exportComments)
   {
      file.write("; Do some initialization on our extruders by retracting\n");
      file.write("; our idle ones and setting our initial temperatures\n");
   }

   // Start with a non-waiting temp change so we can quickly
   // apply this to all of the extruders at once.
   int extruderCount = (int)mPrefs.extruderList.size();
   for (int extruderIndex = 0; extruderIndex < extruderCount; ++extruderIndex)
   {
      const ExtruderData& extruder = mPrefs.extruderList[extruderIndex];

      file.write("T");
      file.write(QString::number(extruderIndex).toAscii());
      file.write("\n");

      file.write("M104 S");
      file.write(QString::number(extruder.printTemp).toAscii());
      file.write("\n");
   }

   // Once all of the extruders are set to heat up, let's wait
   // for them to finish heating up.
   for (int extruderIndex = 0; extruderIndex < extruderCount; ++extruderIndex)
   {
      const ExtruderData& extruder = mPrefs.extruderList[extruderIndex];

      file.write("T");
      file.write(QString::number(extruderIndex).toAscii());
      file.write("\n");

      file.write("M109 S");
      file.write(QString::number(extruder.printTemp).toAscii());
      file.write("\n");
   }

   // Once the extruders reach their temperatures, we can
   // retract them.
   for (int extruderIndex = 0; extruderIndex < extruderCount; ++extruderIndex)
   {
      const ExtruderData& extruder = mPrefs.extruderList[extruderIndex];

      if (extruderIndex != currentExtruder)
      {
         file.write("T");
         file.write(QString::number(extruderIndex).toAscii());
         file.write("\n");

         file.write("G1 F");
         file.write(QString::number(extruder.retractSpeed * 60.0).toAscii());
         file.write("\n");

         file.write("G1 E");
         file.write(QString::number(-extruder.retraction * extruder.flow).toAscii());
         file.write("\n");
      }
   }

   // Return all idle extruders to their idle temperatures.
   for (int extruderIndex = 0; extruderIndex < extruderCount; ++extruderIndex)
   {
      const ExtruderData& extruder = mPrefs.extruderList[extruderIndex];

      if (extruderIndex != currentExtruder &&
         extruder.idleTemp != extruder.printTemp)
      {
         file.write("T");
         file.write(QString::number(extruderIndex).toAscii());
         file.write("\n");

         file.write("M104 S");
         file.write(QString::number(extruder.idleTemp).toAscii());
         file.write("\n");
      }
   }

   // Now set it to our first extruder and begin printing.
   file.write("T");
   file.write(QString::number(currentExtruder).toAscii());
   file.write("\n");
   return true;
}

////////////////////////////////////////////////////////////////////////////////
bool GCodeSplicer::buildExtruderSwap(QFile& file, int lastExtruder, int currentExtruder, double& extrusionValue)
{
   if (mPrefs.exportComments)
   {
      file.write("; ++++++++++++++++++++++++++++++++++++++\n; Swap from extruder ");
      file.write(QString::number(lastExtruder).toAscii());
      file.write(" to ");
      file.write(QString::number(currentExtruder).toAscii());
      file.write("\n; ++++++++++++++++++++++++++++++++++++++\n");
   }

   const ExtruderData& oldExtruder = mPrefs.extruderList[lastExtruder];
   const ExtruderData& newExtruder = mPrefs.extruderList[currentExtruder];

   // Start by retracting the old extruder if necessary.
   if (oldExtruder.retraction > 0)
   {
      file.write("G1 F");
      file.write(QString::number(oldExtruder.retractSpeed * 60.0).toAscii());
      file.write("\n");

      file.write("G1 E");
      file.write(QString::number(extrusionValue - oldExtruder.retraction * oldExtruder.flow).toAscii());
      if (mPrefs.exportAbsoluteEMode)
      {
         extrusionValue -= oldExtruder.retraction * oldExtruder.flow;
      }
      if (mPrefs.exportComments) file.write("; Retract the old extruder");
      file.write("\n");
   }

   // TODO: Wipe excess from the old extruder if necessary.

   // Set the old extruder to idle temperature if necessary.
   if (oldExtruder.idleTemp > 0.0 && oldExtruder.idleTemp != oldExtruder.printTemp)
   {
      file.write("M104 S");
      file.write(QString::number(oldExtruder.idleTemp).toAscii());
      if (mPrefs.exportComments) file.write("; Set the old extruder to idle temp");
      file.write("\n");
   }

   // Swap extruders.
   file.write("T");
   file.write(QString::number(currentExtruder).toAscii());
   if (mPrefs.exportComments) file.write("; Perform the extruder swap");
   file.write("\n");

   // Set the new extruder to print temperature if necessary.
   if (newExtruder.printTemp > 0.0 && newExtruder.idleTemp != newExtruder.printTemp)
   {
      file.write("M109 S");
      file.write(QString::number(newExtruder.printTemp).toAscii());
      if (mPrefs.exportComments) file.write("; Set the new extruder to print temp");
      file.write("\n");
   }

   // Prime the extruder if necessary.
   if (newExtruder.primer * newExtruder.flow > 0 || newExtruder.retraction * newExtruder.flow > 0)
   {
      double primer = newExtruder.primer;
      if (newExtruder.primer <= 0.0)
      {
         primer = newExtruder.retraction;
      }

      primer *= newExtruder.flow;

      file.write("G1 F");
      file.write(QString::number(newExtruder.retractSpeed * 60.0).toAscii());
      file.write("\n");

      file.write("G1 E");
      file.write(QString::number(extrusionValue + primer).toAscii());
      if (mPrefs.exportAbsoluteEMode)
      {
         extrusionValue += primer;
      }
      if (mPrefs.exportComments) file.write("; Prime the new extruder");
      file.write("\n");
   }

   file.write("G92 E0");
   if (mPrefs.exportComments) file.write("; Reset extrusion");
   file.write("\n");
   extrusionValue = 0.0;

   file.write("G1 F");
   file.write(QString::number(oldExtruder.travelSpeed * 60.0).toAscii());
   file.write("\n");

   if (!mPrefs.swapCode.isEmpty())
   {
      file.write(mPrefs.swapCode.toAscii());
      file.write("\n");
   }
   return true;
}

////////////////////////////////////////////////////////////////////////////////
bool GCodeSplicer::buildExtruderMovement(QFile& file, const GCodeCommand& code, int currentExtruder, double* offset, double* currentPos)
{
   QString output;
   if (code.type == GCODE_EXTRUDER_MOVEMENT0) output = "G0 ";
   else                                       output = "G1 ";

   bool hasChanged = false;
   for (int axis = 0; axis < AXIS_NUM; ++axis)
   {
      // Only export this axis if it has changed, or if we
      // have the preference to re-export duplicate axes.
      if (mPrefs.exportAllAxes ||
         (axis != E && code.axisValue[axis] != currentPos[axis]) ||
         (axis == E && code.axisValue[axis] != 0.0))
      {
         output += QString(AXIS_NAME[axis]).toAscii();

         double value = code.axisValue[axis];
         if (axis == E)
         {
            // Offset the extrusion value by our extruders flow ratio.
            value *= mPrefs.extruderList[currentExtruder].flow;
         }
         value += offset[axis];
         if (axis == E && mPrefs.exportAbsoluteEMode)
         {
            offset[E] = value;
         }

         output += QString::number(value).toAscii() + " ";
         hasChanged = true;
      }

      currentPos[axis] = code.axisValue[axis];
   }

   if (code.hasF)
   {
      output += "F" + QString::number(code.f).toAscii();
      hasChanged = true;
   }

   if (hasChanged)
   {
      file.write(output.toAscii());
   }

   return true;
}

#ifdef BUILD_DEBUG_CONTROLS
////////////////////////////////////////////////////////////////////////////////
bool GCodeSplicer::debugBuildLayerData(const QString& fileName)
{
   if (mObjectList.empty())
   {
      mError = "No objects to export.";
      return false;
   }

   QFile file;
   file.setFileName(fileName);
   if (!file.open(QIODevice::WriteOnly))
   {
      mError = "Could not open file \'" + fileName + "\' for writing.";
      return false;
   }

   double extrusionOffset = 0.0;
   double currentPos[AXIS_NUM] = {0.0,};

   const GCodeObject* object = mObjectList[0];
   if (object)
   {
      int levelCount = object->getLayerCount();
      for (int levelIndex = 0; levelIndex < levelCount; ++levelIndex)
      {
         const LayerData& layer = object->getLayer(levelIndex);

         file.write("; ++++++++++++++++++++++++++++++++++++++\n; Begin Layer ");
         file.write(QString::number(levelIndex).toAscii());
         file.write(" with height = ");
         file.write(QString::number(layer.height).toAscii());
         file.write("\n; ++++++++++++++++++++++++++++++++++++++\n");

         if (levelIndex > 0)
         {
            file.write("G92 E0; Reset extruder position\n");
            extrusionOffset = 0.0;
         }

         int codeCount = (int)layer.codes.size();
         for (int codeIndex = 0; codeIndex < codeCount; ++codeIndex)
         {
            const GCodeCommand& code = layer.codes[codeIndex];

            if (code.type == GCODE_EXTRUDER_MOVEMENT0 ||
               code.type == GCODE_EXTRUDER_MOVEMENT1)
            {
               QString output;
               if (code.type == GCODE_EXTRUDER_MOVEMENT0) output = "G0 ";
               else                                       output = "G1 ";

               bool hasChanged = false;
               for (int axis = 0; axis < AXIS_NUM; ++axis)
               {
                  // Only export this axis if it has changed, or if we
                  // have the preference to re-export duplicate axes.
                  if (mPrefs.exportAllAxes ||
                     (axis != E && code.axisValue[axis] != currentPos[axis]) ||
                     (axis == E && code.axisValue[axis] != 0.0))
                  {
                     output += QString(AXIS_NAME[axis]).toAscii();

                     double value = code.axisValue[axis];

                     if (axis == E)
                     {
                        value += extrusionOffset;
                        extrusionOffset = value;
                     }
                     output += QString::number(value).toAscii() + " ";
                     hasChanged = true;
                  }

                  currentPos[axis] = code.axisValue[axis];
               }

               if (code.hasF)
               {
                  output += "F" + QString::number(code.f).toAscii();
                  hasChanged = true;
               }

               if (hasChanged)
               {
                  file.write(output.toAscii());
               }
            }
            else
            {
               file.write(code.command.toAscii());
            }
            file.write(code.comment.toAscii());
            file.write("\n");
         }
      }
   }
	
   return true;
}
#endif

////////////////////////////////////////////////////////////////////////////////
const QString& GCodeSplicer::getError() const
{
   return mError;
}

////////////////////////////////////////////////////////////////////////////////
int GCodeSplicer::getTotalLayerCount()
{
   int layerCount = 0;
   double height = 0.0;

   while(getNextLayer(height, height))
   {
      layerCount++;
   }

   return layerCount;
}

////////////////////////////////////////////////////////////////////////////////
bool GCodeSplicer::getNextLayer(double height, double& outHeight)
{
   double lowestHeight = 0.0;

   // Determine the next layer by getting the next
   // highest layer between all of the objects.
   int count = (int)mObjectList.size();
   for (int index = 0; index < count; ++index)
   {
      const GCodeObject* object = mObjectList[index];
      if (object)
      {
         const LayerData* data = NULL;
         if (object->getLayerAboveHeight(data, height) && data)
         {
            if (lowestHeight == 0.0 || lowestHeight > data->height + object->getOffsetPos()[Z])
            {
               lowestHeight = data->height + object->getOffsetPos()[Z];
            }
         }
      }
   }

   if (lowestHeight > 0.0)
   {
      outHeight = lowestHeight;
      return true;
   }
   return false;
}

////////////////////////////////////////////////////////////////////////////////
