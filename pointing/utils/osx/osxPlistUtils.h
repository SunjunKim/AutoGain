/* -*- mode: c++ -*-
 *
 * pointing/utils/osx/osxPlistUtils.h --
 *
 * Initial software
 * Authors: Nicolas Roussel
 * Copyright © Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

#ifndef osxPlistUtils_h
#define osxPlistUtils_h

#include <CoreFoundation/CoreFoundation.h>

namespace pointing {

  CFPropertyListRef getPropertyListFromXML(const char *xml) ;

  CFPropertyListRef getPropertyListFromFile(const char *path) ;

}

#endif
