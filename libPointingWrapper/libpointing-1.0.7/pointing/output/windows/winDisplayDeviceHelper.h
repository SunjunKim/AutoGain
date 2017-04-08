/* -*- mode: c++ -*-
 *
 * pointing/output/windows/winDisplayDeviceHelper.h --
 *
 * Initial software
 * Authors: Géry Casiez, Nicolas Roussel, Izzat Mukhanov
 * Copyright © Inria
 *
 * http://libpointing.org/
 *
 * This software may be used and distributed according to the terms of
 * the GNU General Public License version 2 or any later version.
 *
 */

#ifndef winDisplayDeviceHelper_h
#define winDisplayDeviceHelper_h

#include <windows.h>
#include <iostream>
#include <list>

namespace pointing {

     /**
     * @brief Stores all the information about the monitor
     */
    struct winDisplayInfo {
      int w; /// width in mm
      int h; /// height in mm
      int resx; /// pixels in horizontal
      int resy; /// vertical
      int refreshRate; /// in Hz
      float ppi; /// resolution in pixel per inch
      char monitorName[14]; /// the name of the monitor
    } ;

    /**
     * @brief Checks if this displayID exists in registry
     * @param displayID
     * @return
     */
    bool isdisplayIDvalid(std::wstring displayID);

    /**
     * @brief Decodes the Extended display identification data
     * @param edid A data structure used to describe display capabilities.
     * It stores manufacturer name, serial number, product type, timings, display size
     * luminance and pixel mapping data.
     *
     * @param dinfo The pointer to the output structure which stores the information.
     */
    void decodeEDID(unsigned char edid[], winDisplayInfo* dinfo);

    /**
     * @brief Fills the display information according to the given key (form registry)
     * @param displayKey The requested key.
     * @param dinfo The pointer to the output structure which stores the information.
     */
    void getDisplayInfo(std::wstring displayKey, winDisplayInfo* dinfo);

    /**
     * @brief Outputs the Display list to the given output stream.
     */
    void listDisplays(std::wostream& out);

    /**
     * @brief
     * @return Returns all the available keys for displays in the registry
     */
    std::list<std::wstring> getAllKeys();

    /**
     * @return Number of displays known in the registry.
     */
    int numberDisplays();

    /**
     * @return First display found in the registry different from the default one
     */
    std::wstring getFirstDisplay();

    /**
     * @return Main Display Name and the number of displays
     */
    std::wstring getControlPanelDisplayName(int* numberDisplays);

    /**
     * @return The registry key for a given display name
     */
    std::wstring getMatchingRegistryKey(std::wstring displayName);

    bool matchStrings(std::wstring nameControlPanel, std::string nameEDID);
}

#endif
