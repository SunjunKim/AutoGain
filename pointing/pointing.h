/* -*- mode: c++ -*-
 *
 * pointing/pointing.h --
 *
 * Initial software
 * Authors: Géry Casiez, Nicolas Roussel
 * Copyright © Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

#ifndef libpointing_h
#define libpointing_h

 // Doxygen main page documentation
 /**
  * \mainpage Libpointing
  *
  * <p>Libpointing is a software toolkit that provides direct access to HID pointing devices and supports the design and evaluation of pointing transfer functions.</p>
  * <p>The toolkit</p>
  * <ul>
  * <li>provides resolution and frequency information for the available pointing and display devices.</li>
  * <li>makes it easy to choose the devices at run-time through the use of URIs.</li>
  * <li>provides raw information from input devices.</li>
  * <li>provides resolution and frequency information for the available pointing and display devices.</li>
  * <li>supports hot-plugging</li>
  * <li>allows to bypass the system's transfer functions to receive raw asynchronous events from one or more pointing devices.</li>
  * <li>replicates as faithfully as possible the transfer functions used by <strong>Microsoft Windows</strong>, <strong>Apple OS X</strong> and <strong>Xorg</strong> (the X.Org Foundation server).</li>
  * <li>running on these three platforms, it makes it possible to compare the replicated functions to the genuine ones as well as custom ones.</li>
  * <li>provides the functionality to use existing transfer functions, custom ones or even build your <a href="https://github.com/INRIA/libpointing/wiki/Custom-Functions">own</a> functions.</li>
  * <li>allows <a href="https://github.com/INRIA/libpointing/wiki/SubPixel-Interaction">subpixel interaction</a>.
  * <li>Java, Python, Node.js are available.</li>
  * </ul>
  *
  * <h3>Find more information <a href="https://github.com/INRIA/libpointing/wiki">here</a>.</h3>
  *
  */

#define LIBPOINTING_VER_STRINGIZE(str)	#str
#define LIBPOINTING_VER_STRINGNUM(num)	LIBPOINTING_VER_STRINGIZE(num)

#define LIBPOINTING_VER_MAJOR   1
#define LIBPOINTING_VER_MINOR   0
#define LIBPOINTING_VER_RELEASE 7

#define LIBPOINTING_VER_STRING	LIBPOINTING_VER_STRINGNUM(LIBPOINTING_VER_MAJOR) "."  \
				LIBPOINTING_VER_STRINGNUM(LIBPOINTING_VER_MINOR) "."  \
				LIBPOINTING_VER_STRINGNUM(LIBPOINTING_VER_RELEASE)

// This should be enough for many applications
#include <pointing/transferfunctions/TransferFunction.h>

#endif
