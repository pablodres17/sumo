#ifndef MSDetectorFileOutput_H
#define MSDetectorFileOutput_H

/**
 * @file   MSDetectorFileOutput.h
 * @author Christian Roessel
 * @date   Thu Jul 31 12:28:19 2003
 * @version $Id$
 * @brief  Declaration of pure virtual base class MSDetectorFileOutput
 * 
 */

/* Copyright (C) 2003 by German Aerospace Center (http://www.dlr.de) */

//---------------------------------------------------------------------------//
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
//---------------------------------------------------------------------------//

// $Id$

#include <string>
#include "MSNet.h"

/**
 * Pure virtual base class for classes (e.g. MSLaneState and
 * MSInductLoop) that should produce XML-output via the
 * MSDetector2File or MSTravelcostDetector singletons.
 *
 * @see MSDetector2File
 * @see MSTravelcostDetector
 * @see MSLaneState
 * @see MSInductLoop
 */
class MSDetectorFileOutput
{
public:
    /** 
     * Returns a string indentifying an object of this class. Used for
     * distinct filenames.
     */
    virtual std::string  getNamePrefix( void ) const = 0;
    /** 
     * Get a header for file output which shall contain some
     * explanation of the output generated by getXMLOutput.
     */ 
    virtual std::string& getXMLHeader( void ) const = 0;
    /** 
     * Get the XML-formatted output of the concrete detector.
     *
     * @param lastNTimesteps Generate data out of the interval
     * (now-lastNTimesteps, now].
     */
    virtual std::string  getXMLOutput( MSNet::Time lastNTimesteps ) = 0;
    /** 
     * Get an opening XML-element containing information about the detector.
     */
    virtual std::string  getXMLDetectorInfoStart( void ) const = 0;
    /** 
     * Get a closing XML-element to getXMLDetectorInfoStart.
     */
    virtual std::string& getXMLDetectorInfoEnd( void ) const = 0;
    /** 
     * Get the data-clean up interval in timesteps.
     */
    virtual MSNet::Time getDataCleanUpSteps( void ) const = 0;
};

#endif

// Local Variables:
// mode:C++
// End:
