Sorry guys, production on this project has slowed to a crawl.  I'm just not
getting very much interest in it, not many people are actually using a dual
extruder machine.  Until people begin to express their interest in this
project, I have gone on to other things.

Also, if there are any particular things from this project that you'd like
to use in your own then feel free, I hope it proves some use to you!

Help Wanted:
------------

 -In need of programmers that can help with the build process
  for Mac and Linux operating systems.


Licensing:
----------

Copyright (C) 2012 Jeff P. Houde (Lochemage)

This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free
Software Foundation; either version 3 of the License, or (at your option)
any later version.

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307 USA


Introduction:
-------------

LocheGSplicer is primarily designed to splice multiple gcode files into a
single file meant for multiple extruder printing.  However, it can also be
used as a high-res gcode visualizer as well as a single extrusion plater.


Building from Source:
---------------------
Download the source here: https://bitbucket.org/Lochemage/lochegsplicer

LocheGSplicer uses CMake (http://www.cmake.org) to generate the project files.

Once the necessary dependancies (see below) have been installed, use CMake to
configure the project for your platform.  I recommend building the binaries in a sub
folder such as "build", this will keep all generated files from mixing with the
primary source files.


Dependancies:
-------------
 - Qt Libraries of version 4.7.4 or later (http://qt.nokia.com/downloads/downloads#qt-lib)
