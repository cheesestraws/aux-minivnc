/****************************************************************************
 *   MiniVNC (c) 2022 Marcio Teixeira                                       *
 *                                                                          *
 *   This program is free software: you can redistribute it and/or modify   *
 *   it under the terms of the GNU General Public License as published by   *
 *   the Free Software Foundation, either version 3 of the License, or      *
 *   (at your option) any later version.                                    *
 *                                                                          *
 *   This program is distributed in the hope that it will be useful,        *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU General Public License for more details.                           *
 *                                                                          *
 *   To view a copy of the GNU General Public License, go to the following  *
 *   location: <http://www.gnu.org/licenses/>.                              *
 ****************************************************************************/

#pragma once

/**
 * Specify a compression level for the color TTRL encoder,
 * from 0 to 4
 */

#define VNC_COMPRESSION_LEVEL 2

/**
 * If the following is defined, MiniVNC will automatically
 * start the server if it finds the application is in the
 * "Startup Items" folder. It also will return to listening
 * for connections after a client disconnects, allowing
 * MiniVNC to be used in headless server mode.
 */
//#define VNC_HEADLESS_MODE

/**
 * To build for a specific resolution, uncomment one of
 * the following. Otherwise, a generic binary will be built
 */

//#define VNC_FB_RES_512_342
//#define VNC_FB_RES_512_384
//#define VNC_FB_RES_640_480
//#define VNC_FB_RES_1024_768

/**
 * To build for a specific color depth, uncomment one of
 * the following. Otherwise, a generic binary will be built
 */

//#define VNC_FB_MONOCHROME
//#define VNC_FB_4_COLORS
//#define VNC_FB_16_COLORS
//#define VNC_FB_256_COLORS


