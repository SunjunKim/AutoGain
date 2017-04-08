/* -*- mode: c++ -*-
 *
 * pointing/utils/osx/osxPlistUtils.cpp --
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

#include <pointing/utils/osx/osxPlistUtils.h>

#include <iostream>

namespace pointing {

  CFPropertyListRef
  getPropertyListFromXML(const char *xml) {
    CFDataRef cfdata = CFDataCreate(kCFAllocatorDefault, (const UInt8*)xml, strlen(xml)) ;
    CFPropertyListRef plist = CFPropertyListCreateWithData(kCFAllocatorDefault,
							   cfdata,
							   kCFPropertyListImmutable,
							   NULL, NULL) ;
    CFRelease(cfdata) ;
    return plist ;
  }

  CFPropertyListRef
  getPropertyListFromFile(const char *path) {
#if 1
    CFStringRef cfpath = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault,
							 path,
							 kCFStringEncodingISOLatin1,
							 kCFAllocatorNull) ;
    CFURLRef cfurl = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, cfpath, 
						   kCFURLPOSIXPathStyle, false) ;
    CFRelease(cfpath) ;
    CFDataRef xmldata = 0 ;
    SInt32 errorCode = 0 ;
    // FIXME: this function is now deprecated
    Boolean ok = CFURLCreateDataAndPropertiesFromResource(kCFAllocatorDefault,
							  cfurl,
							  &xmldata,
							  NULL,
							  NULL,
							  &errorCode) ;
    CFRelease(cfurl) ;
    if (!ok) {
      std::cerr << "getPropertyListFromFile: error " << errorCode << " when trying to create CFURL" << std::endl ;
      return 0 ;
    }

    CFPropertyListRef plist = CFPropertyListCreateWithData(kCFAllocatorDefault,
							   xmldata,
							   kCFPropertyListImmutable,
							   NULL, NULL) ;
    CFRelease(xmldata) ;
#else
    // FIXME: possible replacement, when it works...
    
    CFStringRef cfpath = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault,
							 path,
							 kCFStringEncodingISOLatin1,
							 kCFAllocatorNull) ;
    CFURLRef cfurl = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, cfpath, 
						   kCFURLPOSIXPathStyle, false) ;
    CFErrorRef *error= 0 ;
    CFReadStreamRef stream = CFReadStreamCreateWithFile(kCFAllocatorDefault, cfurl );

    // FIXME: the following code fails to work, for unknown reason (yet)
    CFPropertyListRef plist = CFPropertyListCreateWithStream(kCFAllocatorDefault,
							     stream, 0 /* streamLength */,
							     kCFPropertyListImmutable,
							     NULL /* *format */,
							     error /* *error*/) ;
    // if (error) CFShow(error) ;
    
    CFRelease(stream) ;
    CFRelease(cfurl) ;
    CFRelease(cfpath) ;    
#endif

    return plist ;
  }

}
