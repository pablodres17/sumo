/****************************************************************************/
/// @file    GNEChange_LaneTransformation.h
/// @author  Pablo Alvarez Lopez
/// @date    August 2016
/// @version $Id: GNEChange_LaneTransformation.h 21143 2016-07-11 12:37:50Z palcraft $
///
// A network change in which a single lane is transformated
/****************************************************************************/
// SUMO, Simulation of Urban MObility; see http://sumo.dlr.de/
// Copyright (C) 2001-2016 DLR (http://www.dlr.de/) and contributors
/****************************************************************************/
//
//   This file is part of SUMO.
//   SUMO is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
/****************************************************************************/
#ifndef GNEChange_LaneTransformation_h
#define GNEChange_LaneTransformation_h


// ===========================================================================
// included modules
// ===========================================================================
#ifdef _MSC_VER
#include <windows_config.h>
#else
#include <config.h>
#endif

#include <fx.h>
#include <utils/foxtools/fxexdefs.h>
#include <netbuild/NBEdge.h>
#include "GNEChange.h"

// ===========================================================================
// class declarations
// ===========================================================================
class GNELane;

// ===========================================================================
// class definitions
// ===========================================================================
/**
 * @class GNEChange_LaneTransformation
 * A network change in which a single lane is created or deleted
 */
class GNEChange_LaneTransformation : public GNEChange {
    FXDECLARE_ABSTRACT(GNEChange_LaneTransformation)

public:
    /**@brief Constructor for creating/deleting an edge
     * @param[in] edge The edge on which to apply changes
     * @param[in] lane The lane to be deleted or 0 if a lane should be created
     * @param[in] laneAttrs The attributes of the lane to be created/deleted
     * @param[in] forward Whether to create/delete (true/false)
     */
    GNEChange_LaneTransformation(GNELane* lane, SUMOVehicleClass typeOfTransformation, bool forward);

    /// @brief Destructor
    ~GNEChange_LaneTransformation();

    /// @name inherited from GNEChange
    /// @{
    /// @brief get undo Name
    FXString undoName() const;

    /// @brief get Redo name
    FXString redoName() const;

    /// @brief undo action
    void undo();

    /// @brief redo action
    void redo();
    /// @}


private:
    /// @brief do transform lane to sidewalk
    void doTransformLaneToSidewalk();

    /// @brief undo transform lane to sidewalk
    void undoTransformLaneToSidewalk();

    /// @brief do transform lane to bikelane
    void doTransformLaneToBikeLane();

    /// @brief undo transform lane to bikelane
    void undoTransformLaneToBikeLane();
    
    /// @brief do transform lane to buslane
    void doTransformLaneToBuslane();

    /// @brief undo transform lane to buslane
    void undoTransformLaneToBuslane();

    /// @brief we need to preserve the lane because it maybe the target of GNEChange_Attribute commands
    GNELane* myLane;

    /// @brief type of transformation
    SUMOVehicleClass myTypeOfTransformation;

    std::vector<NBEdge::Lane> myVectorOfLanes;
    PositionVector myGeometry;
    std::vector<NBEdge::Connection> myConnections;
};

#endif
/****************************************************************************/
