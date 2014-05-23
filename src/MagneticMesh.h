#pragma once 

#ifndef MAGNETIC_MESH_H
#define MAGNETIC_MESH_H

#include "cinder/Cinder.h"
#include "cinder/Utilities.h"
#include "cinder/app/AppBasic.h"
#include "cinder/audio/Io.h"
#include "cinder/audio/Output.h"
#include "cinder/audio/FftProcessor.h"
#include "cinder/audio/PcmBuffer.h"
#include "cinder/Camera.h"
#include "cinder/CinderResources.h"

#include "cinder/gl/gl.h"
#include "cinder/gl/Vbo.h"
#include "HandController.h"

typedef std::vector<std::pair<cinder::Vec3f, float> > pinch_list;

class MagneticMesh : public cinder::app::AppBasic {

public:
  void prepareSettings(Settings *settings);

  void setup();
  void resize();
  
  void update();
  void draw();
  
private:
  void createMesh();
  void initMesh();
  void updateMesh(pinch_list pinches, float bass_total, float treble_total);
  
  HandController hand_controller_;
  
  cinder::CameraPersp camera_;
  Leap::Controller controller_;
  cinder::gl::VboMesh mesh_;
  std::vector<cinder::Vec3f> velocities_;
  std::vector<cinder::Vec3f> positions_;
  std::vector<cinder::ColorA> colors_;

  cinder::audio::TrackRef track_;
  cinder::audio::PcmBuffer32fRef pcm_buffer_;
};

#endif
