
#include "MagneticMesh.h"

#define SONG_SOURCE CINDER_RESOURCE(../resources/, song.mp3, 128, MP3)

#define PINCH_FORCE 200.0f

#define MESH_PARTICLE_WIDTH 200
#define MESH_PARTICLE_HEIGHT 400
#define MESH_FORCE 0.2f
#define MESH_PARTICLE_DISTANCE 8
#define MESH_ALPHA 0.3f

#define MESH_WIDTH (MESH_PARTICLE_DISTANCE * MESH_PARTICLE_WIDTH)
#define MESH_HEIGHT (MESH_PARTICLE_DISTANCE * MESH_PARTICLE_HEIGHT)

#define PARTICLE_DAMPING 0.99f
#define FFT_BANDS 512

#define WINDOW_WIDTH 1440
#define WINDOW_HEIGHT 900

#define DEFAULT_LOUDNESS 5.0f

#define HAND_SCALE 2.0f
#define HAND_TRANSLATION (Vec3f(0.0f, -400.0f, -100.0f))

using namespace ci;
using namespace ci::app;

void MagneticMesh::prepareSettings(Settings *settings) {
  settings->setWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
  settings->setFrameRate(60.0f);
  settings->setFullScreen(true);
}

void MagneticMesh::setup() {
#ifdef __APPLE__
  track_ = audio::Output::addTrack(audio::load(loadResource(SONG_SOURCE)));
  track_->enablePcmBuffering(true);
#endif

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  gl::enableAdditiveBlending();
  createMesh();
  
  // Setup hand transformation.
  hand_controller_.setScale(HAND_SCALE);
  hand_controller_.setTranslation(HAND_TRANSLATION);
  hideCursor();
}

void MagneticMesh::resize() {
  camera_.lookAt(Vec3f(0.0f, 0.0f, 690.0f), Vec3f::zero());
  camera_.setPerspective(60, getWindowAspectRatio(), 1, 10000);
  gl::setMatrices(camera_);
}

void MagneticMesh::createMesh() {
  gl::VboMesh::Layout layout;
  layout.setStaticIndices();
  layout.setDynamicPositions();
  layout.setDynamicColorsRGBA();
  
  int num_vertices = MESH_PARTICLE_WIDTH * MESH_PARTICLE_HEIGHT;
  int num_quads = (MESH_PARTICLE_WIDTH - 1) * (MESH_PARTICLE_HEIGHT - 1);
  mesh_ = gl::VboMesh(num_vertices, 4 * num_quads, layout, GL_QUADS);
  
  std::vector<uint32_t> indices;
  
  for (int x = 0; x < MESH_PARTICLE_WIDTH - 1; ++x) {
    for (int y = 0; y < MESH_PARTICLE_HEIGHT - 1; ++y) {
      indices.push_back((x + 0) * MESH_PARTICLE_HEIGHT + (y + 0));
      indices.push_back((x + 1) * MESH_PARTICLE_HEIGHT + (y + 0));
      indices.push_back((x + 1) * MESH_PARTICLE_HEIGHT + (y + 1));
      indices.push_back((x + 0) * MESH_PARTICLE_HEIGHT + (y + 1));
    }
  }
  
  for (int x = 0; x < MESH_PARTICLE_WIDTH; ++x) {
    for(int y = 0; y < MESH_PARTICLE_HEIGHT; ++y) {
      float x_pos = ((1.0f * x) / MESH_PARTICLE_WIDTH - 0.5f) * MESH_WIDTH;
      float y_pos = ((1.0f * y) / MESH_PARTICLE_HEIGHT - 0.5f) * MESH_HEIGHT;
      Vec3f mesh_position = Vec3f(x_pos, y_pos, 1);
      positions_.push_back(mesh_position);
      velocities_.push_back(Vec3f(0, 0, 0));
      colors_.push_back(ColorA(1, 1, 1, 1));
    }
  }
  
  mesh_.bufferIndices(indices);
}

void MagneticMesh::updateMesh(pinch_list pinches, float bass_total, float treble_total) {
  int i = 0;
  int num_vertices = MESH_PARTICLE_WIDTH * MESH_PARTICLE_HEIGHT;
  
  for (int x = 0; x < MESH_PARTICLE_WIDTH; ++x) {
    for (int y = 0; y < MESH_PARTICLE_HEIGHT; ++y) {
      // Apply gravitational forces for all the pinches.
      for (int p = 0; p < pinches.size(); ++p) {
        Vec3f delta = pinches[p].first - positions_[i];
        float distance = delta.length() + 50;
        float force = pinches[p].second * PINCH_FORCE / (distance);
        
        velocities_[i] += force * delta.normalized();
      }
      
      // Apply the mesh forces. Particles are attached to neighbors by springs.
      if (y > 0) {
        Vec3f delta = positions_[i - 1] - positions_[i];
        velocities_[i] += MESH_FORCE * (delta - MESH_PARTICLE_DISTANCE * delta.normalized());
      }
      if (y < MESH_PARTICLE_HEIGHT - 1) {
        Vec3f delta = positions_[i + 1] - positions_[i];
        velocities_[i] += MESH_FORCE * (delta - MESH_PARTICLE_DISTANCE * delta.normalized());
      }
      if (x > 0) {
        Vec3f delta = positions_[i - MESH_PARTICLE_HEIGHT] - positions_[i];
        velocities_[i] += MESH_FORCE * (delta - MESH_PARTICLE_DISTANCE * delta.normalized());
      }
      if (x < MESH_PARTICLE_WIDTH - 1) {
        Vec3f delta = positions_[i + MESH_PARTICLE_HEIGHT] - positions_[i];
        velocities_[i] += MESH_FORCE * (delta - MESH_PARTICLE_DISTANCE * delta.normalized());
      }
      
      // Damping.
      velocities_[i] *= PARTICLE_DAMPING;
      
      // Color based on the music, hacked together so it looks good.
      float r = powf((1.0f * y) / MESH_PARTICLE_HEIGHT, 5.0f / bass_total);
      float g = powf((1.0f * y * x) / num_vertices, 5.0f / treble_total);
      
      if (x == 0 || y == 0 || x == MESH_PARTICLE_WIDTH - 1 || y == MESH_PARTICLE_HEIGHT - 1)
        colors_[i] = ColorA(treble_total, treble_total, treble_total, 1);
      else
        colors_[i] = ColorA(r, 0.13f + g, 1 - r, MESH_ALPHA);

      ++i;
    }
  }

  // Update the mesh positions using the velocities.
  gl::VboMesh::VertexIter iter = mesh_.mapVertexBuffer();
  for (int p = 0; p < num_vertices; ++p) {
    iter.setPosition(positions_[p]);
    iter.setColorRGBA(colors_[p]);
    positions_[p] += velocities_[p];
    ++iter;
  }
}

void MagneticMesh::update() {
  float bass_total = 0.0f;
  float treble_total = 0.0f;

  // Audio only works in OSX in Cinder.
#ifdef __APPLE__
  pcm_buffer_ = track_->getPcmBuffer();

  if (pcm_buffer_) {
    std::shared_ptr<float> fft =
        audio::calculateFft(pcm_buffer_->getChannelData(audio::CHANNEL_FRONT_LEFT), FFT_BANDS);
    if (fft) {
      float* fft_buffer = fft.get();
      
      for(int i = 0; i < FFT_BANDS / 2; i++) {
        bass_total += fft_buffer[i];
      }
      for(int i = FFT_BANDS / 2; i < FFT_BANDS; i++) {
        treble_total += fft_buffer[i];
      }
    }
  }
    
  bass_total /= (FFT_BANDS / 2);
  treble_total /= (FFT_BANDS / 2);
#else
  // If there's no audio, still add a little color.
  bass_total = DEFAULT_LOUDNESS;
  treble_total = DEFAULT_LOUDNESS;
#endif
  
  updateMesh(hand_controller_.getPinches(), bass_total, treble_total);
}

void MagneticMesh::draw() {

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_COLOR_MATERIAL);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  GLfloat light_position[] = { 0.0f, 0.0f, 750.0f, 1.0f };
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  
  gl::color(Color::white());
  gl::clear(ci::ColorA(0, 0, 0, 0));
  gl::draw(mesh_);
  gl::setMatrices(camera_);
  gl::setViewport(getWindowBounds());
  
  glDisable(GL_COLOR_MATERIAL);

  hand_controller_.drawHands();
  
  glPopMatrix();
}

CINDER_APP_BASIC(MagneticMesh, RendererGl)
