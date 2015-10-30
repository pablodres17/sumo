/****************************************************************************/
/// @file    MSAbstractLaneChangeModel.h
/// @author  Daniel Krajzewicz
/// @author  Friedemann Wesner
/// @author  Sascha Krieg
/// @author  Michael Behrisch
/// @author  Jakob Erdmann
/// @date    Fri, 29.04.2005
/// @version $Id$
///
// Interface for lane-change models
/****************************************************************************/
// SUMO, Simulation of Urban MObility; see http://sumo.dlr.de/
// Copyright (C) 2001-2015 DLR (http://www.dlr.de/) and contributors
/****************************************************************************/
//
//   This file is part of SUMO.
//   SUMO is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
/****************************************************************************/

// ===========================================================================
// included modules
// ===========================================================================
#ifdef _MSC_VER
#include <windows_config.h>
#else
#include <config.h>
#endif

#include <utils/options/OptionsCont.h>
#include "MSAbstractLaneChangeModel.h"
#include <microsim/MSNet.h>
#include <microsim/MSEdge.h>
#include <microsim/MSLane.h>
#include <microsim/MSGlobals.h>
#include "MSLCM_DK2008.h"
#include "MSLCM_LC2013.h"
#include "MSLCM_JE2013.h"
#include "MSLCM_SL2015.h"

/* -------------------------------------------------------------------------
 * static members
 * ----------------------------------------------------------------------- */
bool MSAbstractLaneChangeModel::myAllowOvertakingRight(false);

/* -------------------------------------------------------------------------
 * MSAbstractLaneChangeModel-methods
 * ----------------------------------------------------------------------- */

void
MSAbstractLaneChangeModel::initGlobalOptions(const OptionsCont& oc) {
    myAllowOvertakingRight = oc.getBool("lanechange.overtake-right");
}


MSAbstractLaneChangeModel*
MSAbstractLaneChangeModel::build(LaneChangeModel lcm, MSVehicle& v) {
    switch (lcm) {
        case LCM_DK2008:
            return new MSLCM_DK2008(v);
        case LCM_LC2013:
            return new MSLCM_LC2013(v);
        case LCM_JE2013:
            return new MSLCM_JE2013(v);
        case LCM_SL2015:
            return new MSLCM_SL2015(v);
        case LCM_DEFAULT:
            if (MSGlobals::gLateralResolution < 0) {
                return new MSLCM_LC2013(v);
            } else {
                return new MSLCM_SL2015(v);
            }
        default:
            throw ProcessError("Lane change model '" + toString(lcm) + "' not implemented");
    }
}


MSAbstractLaneChangeModel::MSAbstractLaneChangeModel(MSVehicle& v, const LaneChangeModel model) :
    myVehicle(v),
    myOwnState(0),
    myLaneChangeCompletion(1.0),
    myLaneChangeDirection(0),
    myLateralspeed(0),
    myAlreadyMoved(false),
    myShadowLane(0),
    myCarFollowModel(v.getCarFollowModel()),
    myModel(model),
    myLastLaneChangeOffset(0) {
}


MSAbstractLaneChangeModel::~MSAbstractLaneChangeModel() {
}


bool
MSAbstractLaneChangeModel::congested(const MSVehicle* const neighLeader) {
    if (neighLeader == 0) {
        return false;
    }
    // Congested situation are relevant only on highways (maxSpeed > 70km/h)
    // and congested on German Highways means that the vehicles have speeds
    // below 60km/h. Overtaking on the right is allowed then.
    if ((myVehicle.getLane()->getSpeedLimit() <= 70.0 / 3.6) || (neighLeader->getLane()->getSpeedLimit() <= 70.0 / 3.6)) {

        return false;
    }
    if (myVehicle.congested() && neighLeader->congested()) {
        return true;
    }
    return false;
}


    
bool
MSAbstractLaneChangeModel::predInteraction(const std::pair<MSVehicle*, SUMOReal>& leader) {
    if (leader.first == 0) {
        return false;
    }
    // let's check it on highways only
    if (leader.first->getSpeed() < (80.0 / 3.6)) {
        return false;
    }
    return leader.second < myCarFollowModel.interactionGap(&myVehicle, leader.first->getSpeed());
}


bool
MSAbstractLaneChangeModel::startLaneChangeManeuver(MSLane* source, MSLane* target, int direction) {
    if (MSGlobals::gLaneChangeDuration > DELTA_T) {
        myLaneChangeCompletion = 0;
        myLaneChangeDirection = direction;
        myLateralspeed = (target->getCenterOnEdge() - source->getCenterOnEdge()) * (SUMOReal)DELTA_T / (SUMOReal)MSGlobals::gLaneChangeDuration;
        myVehicle.switchOffSignal(MSVehicle::VEH_SIGNAL_BLINKER_RIGHT | MSVehicle::VEH_SIGNAL_BLINKER_LEFT);
        myVehicle.switchOnSignal(direction == 1 ? MSVehicle::VEH_SIGNAL_BLINKER_LEFT : MSVehicle::VEH_SIGNAL_BLINKER_RIGHT);
        return true;
    } else {
        primaryLaneChanged(source, target, direction);
        return false;
    }
}


void
MSAbstractLaneChangeModel::primaryLaneChanged(MSLane* source, MSLane* target, int direction) {
    initLastLaneChangeOffset(direction);
    myVehicle.leaveLane(MSMoveReminder::NOTIFICATION_LANE_CHANGE);
    source->leftByLaneChange(&myVehicle);
    myVehicle.enterLaneAtLaneChange(target);
    target->enteredByLaneChange(&myVehicle);
    changed();
}


bool
MSAbstractLaneChangeModel::updateCompletion() {
    const bool pastBefore = pastMidpoint();
    myLaneChangeCompletion += (SUMOReal)DELTA_T / (SUMOReal)MSGlobals::gLaneChangeDuration;
    return !pastBefore && pastMidpoint();
}


void
MSAbstractLaneChangeModel::endLaneChangeManeuver(const MSMoveReminder::Notification reason) {
    myLaneChangeCompletion = 1;
    cleanupShadowLane();
    myNoPartiallyOccupatedByShadow.clear();
    myVehicle.switchOffSignal(MSVehicle::VEH_SIGNAL_BLINKER_RIGHT | MSVehicle::VEH_SIGNAL_BLINKER_LEFT);
    myVehicle.fixPosition();
}


MSLane*
MSAbstractLaneChangeModel::getShadowLane(const MSLane* lane) const {
    if (std::find(myNoPartiallyOccupatedByShadow.begin(), myNoPartiallyOccupatedByShadow.end(), lane) == myNoPartiallyOccupatedByShadow.end()) {
        // initialize shadow lane
        const SUMOReal overlap = myVehicle.getLateralOverlap();
        if (myVehicle.getID() == "disabled") std::cout << SIMTIME << " veh=" << myVehicle.getID() << " posLat=" << myVehicle.getLateralPositionOnLane() << " overlap=" << overlap << "\n";
        if (overlap > NUMERICAL_EPS || 
                // "reserve" target lane even when there is no overlap yet
                (isChangingLanes() && myLaneChangeCompletion < 0.5)) {
            const int shadowDirection = myVehicle.getLateralPositionOnLane() < 0 ? -1 : 1;
            return lane->getParallelLane(shadowDirection);
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}


void
MSAbstractLaneChangeModel::cleanupShadowLane() {
    if (myShadowLane != 0) {
        if (myVehicle.getID() == "disabled") 
            std::cout << SIMTIME << " cleanupShadowLane\n";
        myShadowLane->resetPartialOccupation(&myVehicle);
        myShadowLane = 0;
    }
    for (std::vector<MSLane*>::const_iterator it = myShadowFurtherLanes.begin(); it != myShadowFurtherLanes.end(); ++it) {
        if (myVehicle.getID() == "disabled") std::cout << SIMTIME << " cleanupShadowLane2\n";
        (*it)->resetPartialOccupation(&myVehicle);
    }
    myShadowFurtherLanes.clear();
    myNoPartiallyOccupatedByShadow.clear();
}


bool
MSAbstractLaneChangeModel::cancelRequest(int state) {
    int ret = myVehicle.influenceChangeDecision(state);
    return ret != state;
}


void
MSAbstractLaneChangeModel::initLastLaneChangeOffset(int dir) {
    if (dir > 0) {
        myLastLaneChangeOffset = 1;
    } else if (dir < 0) {
        myLastLaneChangeOffset = -1;
    }
}

void
MSAbstractLaneChangeModel::updateShadowLane() {
    if (myShadowLane != 0) {
        if (myVehicle.getID() == "disabled") std::cout << SIMTIME << " updateShadowLane\n";
        myShadowLane->resetPartialOccupation(&myVehicle);
    }
    myShadowLane = getShadowLane(myVehicle.getLane());
    std::vector<MSLane*> passed;
    if (myShadowLane != 0) {
        myShadowLane->setPartialOccupation(&myVehicle);
        const std::vector<MSLane*>& further = myVehicle.getFurtherLanes();
        for (std::vector<MSLane*>::const_reverse_iterator i = further.rbegin(); i != further.rend(); ++i) {
            MSLane* shadowFurther = getShadowLane(*i);
            if (shadowFurther != 0) {
                passed.push_back(shadowFurther);
            }
        }
        passed.push_back(myShadowLane);
    } else {
        if (isChangingLanes() && myVehicle.getLateralOverlap() > NUMERICAL_EPS) {
            WRITE_WARNING("Vehicle '" + myVehicle.getID() + "' could not finish continuous lane change (lane disappeared) time=" +
                    time2string(MSNet::getInstance()->getCurrentTimeStep()) + ".");
            endLaneChangeManeuver();
        }
    }
    if (myVehicle.getID() == "disabled") std::cout << SIMTIME << " updateShadowLane veh=" << myVehicle.getID() 
        << " newShadowLane=" << Named::getIDSecure(myShadowLane)
        << "\n   before:" << " myShadowFurtherLanes=" << toString(myShadowFurtherLanes) << " passed=" << toString(passed)
        << "\n";
    myVehicle.updateFurtherLanes(myShadowFurtherLanes, passed);
    if (myVehicle.getID() == "disabled") std::cout 
        << "\n   after:" << " myShadowFurtherLanes=" << toString(myShadowFurtherLanes) << "\n";
}


int 
MSAbstractLaneChangeModel::getShadowDirection() const {
    if (!isChangingLanes()) {
        assert(false);
        return 0;
    } else if (pastMidpoint()) {
        return -myLaneChangeDirection;
    } else {
        return myLaneChangeDirection;
    }
}


SUMOReal 
MSAbstractLaneChangeModel::getAngleOffset() const {
    const SUMOReal angleOffset = 60 / STEPS2TIME(MSGlobals::gLaneChangeDuration) * (pastMidpoint() ? 1 - myLaneChangeCompletion : myLaneChangeCompletion);
    return myLaneChangeDirection * angleOffset;
}


SUMOTime 
MSAbstractLaneChangeModel::remainingTime() const {
    return (1 - myLaneChangeCompletion) * MSGlobals::gLaneChangeDuration;
}


void 
MSAbstractLaneChangeModel::setShadowApproachingInformation(MSLink* link) const {
    //std::cout << SIMTIME << " veh=" << myVehicle.getID() << " @=" << &myVehicle << " set shadow approaching=" << link->getViaLaneOrLane()->getID() << "\n";
    myApproachedByShadow.push_back(link);
}

void 
MSAbstractLaneChangeModel::removeShadowApproachingInformation() const {
    for (std::vector<MSLink*>::iterator it = myApproachedByShadow.begin(); it != myApproachedByShadow.end(); ++it) {
        //std::cout << SIMTIME << " veh=" << myVehicle.getID() << " @=" << &myVehicle << " remove shadow approaching=" << (*it)->getViaLaneOrLane()->getID() << "\n";
        (*it)->removeApproaching(&myVehicle);
    }
    myApproachedByShadow.clear();
}
