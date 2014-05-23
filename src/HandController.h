#pragma once

#ifndef HANDS_H
#define HANDS_H

#include <vector>
#include "cinder/Cinder.h"
#include "cinder/Utilities.h"
#include "cinder/gl/gl.h"
#include "Leap.h"

typedef std::vector<std::pair<ci::Vec3f, float> > pinch_list;

class HandController {
public:
  HandController() : scale_(1.0f) { }
  
  pinch_list getPinches();
  
  void setScale(float scale) { scale_ = scale; }
  void setTranslation(ci::Vec3f translation) { translation_ = translation; }
  
  void drawHands();
  
private:
  void drawJoint(const Leap::Vector& joint_position);
  void drawBone(const Leap::Bone& bone);
  
  Leap::Controller controller_;
  ci::Vec3f translation_;
  float scale_;
};

#endif