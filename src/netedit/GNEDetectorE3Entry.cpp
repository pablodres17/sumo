/****************************************************************************/
/// @file    GNEDetectorE3Entry.cpp
/// @author  Pablo Alvarez Lopez
/// @date    Nov 2015
/// @version $Id: GNEDetectorE3.cpp 19861 2016-02-01 09:08:47Z palcraft $
///
/// 
/****************************************************************************/
// SUMO, Simulation of Urban MObility; see http://sumo-sim.org/
// Copyright (C) 2001-2013 DLR (http://www.dlr.de/) and contributors
/****************************************************************************/
//
//   This file is part of SUMO.
//   SUMO is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 3 of the License, or
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

#include <string>
#include <iostream>
#include <utility>
#include <foreign/polyfonts/polyfonts.h>
#include <utils/geom/PositionVector.h>
#include <utils/common/RandHelper.h>
#include <utils/common/SUMOVehicleClass.h>
#include <utils/common/ToString.h>
#include <utils/geom/GeomHelper.h>
#include <utils/gui/windows/GUISUMOAbstractView.h>
#include <utils/gui/windows/GUIAppEnum.h>
#include <utils/gui/images/GUIIconSubSys.h>
#include <utils/gui/div/GUIParameterTableWindow.h>
#include <utils/gui/globjects/GUIGLObjectPopupMenu.h>
#include <utils/gui/div/GUIGlobalSelection.h>
#include <utils/gui/div/GLHelper.h>
#include <utils/gui/windows/GUIAppEnum.h>
#include <utils/gui/images/GUITexturesHelper.h>
#include <utils/xml/SUMOSAXHandler.h>

#include "GNEDetectorE3Entry.h"
#include "GNEDetectorE3.h"
#include "GNELane.h"
#include "GNEViewNet.h"
#include "GNEUndoList.h"
#include "GNENet.h"
#include "GNEChange_Attribute.h"
#include "GNELogo_Entry.cpp"

#ifdef CHECK_MEMORY_LEAKS
#include <foreign/nvwa/debug_new.h>
#endif

// ===========================================================================
// static member definitions
// ===========================================================================
GUIGlID GNEDetectorE3Entry::detectorE3EntryGlID = 0;
bool GNEDetectorE3Entry::detectorE3EntryInitialized = false;

// ===========================================================================
// member method definitions
// ===========================================================================

GNEDetectorE3Entry::GNEDetectorE3Entry(const std::string &id, GNEViewNet* viewNet, GNELane *lane, SUMOReal pos, GNEDetectorE3 *parent, bool blocked) :
    GNEDetector(id, viewNet, SUMO_TAG_DET_ENTRY, lane, pos, 0, "", blocked, SUMO_TAG_E3DETECTOR, parent) {
    // Update geometry;
    updateGeometry();
    // load logo, if wasn't inicializated
    if (!detectorE3EntryInitialized) {
        FXImage* i = new FXGIFImage(getViewNet()->getNet()->getApp(), GNELogo_Entry, IMAGE_KEEP | IMAGE_SHMI | IMAGE_SHMP);
        detectorE3EntryGlID = GUITexturesHelper::add(i);
        detectorE3EntryInitialized = true;
        delete i;
    }
    // Set colors
    myBaseColor = RGBColor(0, 204, 0, 255);
    myBaseColorSelected = RGBColor(125, 204, 0, 255);
}


GNEDetectorE3Entry::~GNEDetectorE3Entry() {}


void 
GNEDetectorE3Entry::updateGeometry() {
    // Clear all containers
    myShapeRotations.clear();
    myShapeLengths.clear();
   
    // clear Shape
    myShape.clear();

    // Get shape of lane parent
    myShape.push_back(myLane->getShape().positionAtOffset(myLane->getPositionRelativeToParametricLenght(myPosition.x())));

    // Obtain first position
    Position f = myShape[0] - Position(1, 0);

    // Obtain next position
    Position s = myShape[0] + Position(1, 0);

    // Save rotation (angle) of the vector constructed by points f and s
    myShapeRotations.push_back(myLane->getShape().rotationDegreeAtOffset(myLane->getPositionRelativeToParametricLenght(myPosition.x())) * -1);

    // Set offset of logo
    myDetectorLogoOffset = Position(-2, 0);

    // Set block icon rotation, and using their rotation for logo
    setBlockIconRotation();

    // Update parent geometry
    myParent->updateGeometry();
}


void 
GNEDetectorE3Entry::writeAdditional(OutputDevice& device) {
    // Write parameters
    device.openTag(getTag());
    device.writeAttr(SUMO_ATTR_LANE, myLane->getID());
    device.writeAttr(SUMO_ATTR_POSITION, myPosition.x());
    // Close tag
    device.closeTag();
}


GUIGLObjectPopupMenu* 
GNEDetectorE3Entry::getPopUpMenu(GUIMainWindow& app, GUISUMOAbstractView& parent) {
    GUIGLObjectPopupMenu* ret = new GUIGLObjectPopupMenu(app, parent, *this);
    buildPopupHeader(ret, app);
    buildCenterPopupEntry(ret);
    new FXMenuCommand(ret, "Copy detector E3 name to clipboard", 0, ret, MID_COPY_EDGE_NAME);
    buildNameCopyPopupEntry(ret);
    buildSelectionPopupEntry(ret);
    buildPositionCopyEntry(ret, false);
    // buildShowParamsPopupEntry(ret, false);
    const SUMOReal pos = myShape.nearest_offset_to_point2D(parent.getPositionInformation());
    new FXMenuCommand(ret, ("pos: " + toString(pos)).c_str(), 0, 0, 0);
    // new FXMenuSeparator(ret);
    // buildPositionCopyEntry(ret, false);
    // let the GNEViewNet store the popup position
    (dynamic_cast<GNEViewNet&>(parent)).markPopupPosition();
    return ret;
}


GUIParameterTableWindow* 
GNEDetectorE3Entry::getParameterWindow(GUIMainWindow& app, GUISUMOAbstractView& parent) {
    GUIParameterTableWindow* ret =
        new GUIParameterTableWindow(app, *this, 2);
    /* not supported yet
    // add items
    ret->mkItem("length [m]", false, getLane().getParentEdge().getNBEdge()->getLength());
    // close building
    ret->closeBuilding();
    */
    return ret;
}


void 
GNEDetectorE3Entry::drawGL(const GUIVisualizationSettings& s) const {
    // Additonals element are drawed using a drawGLAdditional
    drawGLAdditional(0, s);
}


void 
GNEDetectorE3Entry::drawGLAdditional(GUISUMOAbstractView* const parent, const GUIVisualizationSettings& s) const {
    // Ignore Warning
    UNUSED_PARAMETER(parent);

    // Start drawing adding gl identificator
    glPushName(getGlID());
    
    // Push detector matrix
    glPushMatrix();
    glTranslated(0, 0, getType());
    
    // Set initial values
    if(isAdditionalSelected())
        glColor3d(myBaseColorSelected.red(), myBaseColorSelected.green(), myBaseColorSelected.blue());
    else
        glColor3d(myBaseColor.red(), myBaseColor.green(), myBaseColor.blue());
    const SUMOReal exaggeration = s.addSize.getExaggeration(s);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    // Push poligon matrix
    glPushMatrix();
    glScaled(exaggeration, exaggeration, 1);
    glTranslated(myShape[0].x(), myShape[0].y(), 0);
    glRotated(myShapeRotations[0], 0, 0, 1);
    
    // Draw poligon
    glBegin(GL_LINES);
    glVertex2d(1.7, 0);
    glVertex2d(-1.7, 0);
    glEnd();
    glBegin(GL_QUADS);
    glVertex2d(-1.7, .5);
    glVertex2d(-1.7, -.5);
    glVertex2d(1.7, -.5);
    glVertex2d(1.7, .5);
    glEnd();
    
    // first Arrow
    glTranslated(1.5, 0, 0);
    GLHelper::drawBoxLine(Position(0, 4), 0, 2, .05);
    GLHelper::drawTriangleAtEnd(Position(0, 4), Position(0, 1), (SUMOReal) 1, (SUMOReal) .25);
    
    // second Arrow
    glTranslated(-3, 0, 0);
    GLHelper::drawBoxLine(Position(0, 4), 0, 2, .05);
    GLHelper::drawTriangleAtEnd(Position(0, 4), Position(0, 1), (SUMOReal) 1, (SUMOReal) .25);
    
    // Pop poligon matrix
    glPopMatrix();
    
    // Pop detector matrix
    glPopMatrix();
    
    // Check if the distance is enought to draw details
    if (s.scale * exaggeration >= 10) {
        // Draw icon
        drawDetectorIcon(detectorE3EntryGlID, 1.5, 1);
        
        // Show Lock icon depending of the Edit mode
        if(dynamic_cast<GNEViewNet*>(parent)->showLockIcon())
            drawLockIcon(0.4);
    }
    // Draw name
    drawName(getCenteringBoundary().getCenter(), s.scale, s.addName);
    
    // pop gl identificator
    glPopName();
}


std::string 
GNEDetectorE3Entry::getAttribute(SumoXMLAttr key) const {
    switch (key) {
        case SUMO_ATTR_ID:
            return getMicrosimID();
        case SUMO_ATTR_LANE:
            return toString(myLane->getAttribute(SUMO_ATTR_ID));
        case SUMO_ATTR_POSITION:
            return toString(myPosition.x());
        default:
            throw InvalidArgument("detector E3 attribute '" + toString(key) + "' not allowed");
    }
}


void 
GNEDetectorE3Entry::setAttribute(SumoXMLAttr key, const std::string& value, GNEUndoList* undoList) {
if (value == getAttribute(key)) {
        return; //avoid needless changes, later logic relies on the fact that attributes have changed
    }
    switch (key) {
        case SUMO_ATTR_ID:
        case SUMO_ATTR_LANE:
            throw InvalidArgument("modifying detector E3 attribute '" + toString(key) + "' not allowed");
        case SUMO_ATTR_POSITION:
            undoList->p_add(new GNEChange_Attribute(this, key, value));
            updateGeometry();
            break;
        default:
            throw InvalidArgument("detector E3 attribute '" + toString(key) + "' not allowed");
    }
}


bool 
GNEDetectorE3Entry::isValid(SumoXMLAttr key, const std::string& value) {
    switch (key) {
        case SUMO_ATTR_ID:
        case SUMO_ATTR_LANE:
            throw InvalidArgument("modifying detector E3 attribute '" + toString(key) + "' not allowed");
        case SUMO_ATTR_POSITION:
            return (canParse<SUMOReal>(value) && parse<SUMOReal>(value) >= 0 && parse<SUMOReal>(value) <= (myLane->getLaneParametricLenght()));
        default:
            throw InvalidArgument("detector E3 attribute '" + toString(key) + "' not allowed");
    }
}

void
GNEDetectorE3Entry::setAttribute(SumoXMLAttr key, const std::string& value) {
    switch (key) {
        case SUMO_ATTR_ID:
        case SUMO_ATTR_LANE:
            throw InvalidArgument("modifying detector E3 attribute '" + toString(key) + "' not allowed");
        case SUMO_ATTR_POSITION:
            myPosition = Position(parse<SUMOReal>(value), 0);
            updateGeometry();
            getViewNet()->update();
            break;
        default:
            throw InvalidArgument("detector E3 attribute '" + toString(key) + "' not allowed");
    }
}

/****************************************************************************/