/* -*- mode: c++ -*-
 *
 * pointing/utils/PointingCursor.cpp --
 *
 * Initial software
 * Authors: Izzatbek Mukhanov
 * Copyright © Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

#include <pointing/utils/PointingCursor.h>

#ifdef __linux__
#include <stdio.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif
#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif

namespace pointing {

  void PointingCursor::setPosition(double x, double y)
  {
#ifdef __linux__
    Display *dpy;
    Window rw;
    dpy = XOpenDisplay(0);
    rw = XRootWindow(dpy, 0);
    
    XSelectInput(dpy, rw, KeyReleaseMask);
    XWarpPointer(dpy, None, rw, 0, 0, 0, 0, x, y);
    XFlush(dpy);
    XCloseDisplay(dpy);
#endif
#ifdef __APPLE__
    CGPoint point ;
    point.x = x ;
    point.y = y ;

    CGEventRef mouseMoveEvent = CGEventCreateMouseEvent(NULL, kCGEventMouseMoved, point, kCGMouseButtonLeft) ;
    CGEventPost(kCGSessionEventTap, mouseMoveEvent) ;
    CFRelease(mouseMoveEvent) ;
#endif
#ifdef _WIN32
	SetCursorPos(x, y);
#endif
  }
  
  void PointingCursor::getPosition(double *x, double *y)
  {
#ifdef __linux__
    Display *dpy;
    Window rw;
    dpy = XOpenDisplay(0);
    rw = XRootWindow(dpy, 0);
    Window ret_root, ret_child;
	int root_x, root_y, win_x, win_y;
	unsigned int mask;
	
	if (XQueryPointer(dpy, rw, &ret_root, &ret_child, &root_x, &root_y,
					 &win_x, &win_y, &mask))
    {
      *x = root_x;
      *y = root_y;
    }
    XCloseDisplay(dpy);
#endif
#ifdef __APPLE__
    CGPoint point;
    CGEventRef event = CGEventCreate(NULL);
    point = CGEventGetLocation(event);
    CFRelease(event);
    *x = point.x;
    *y = point.y;
#endif
#ifdef _WIN32
	POINT point;
	GetCursorPos(&point);
	*x = point.x;
	*y = point.y;
#endif
  }

}
