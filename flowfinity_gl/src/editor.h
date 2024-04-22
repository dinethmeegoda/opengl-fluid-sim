#include "engine/camera.h"
#include "engine/scene/circle.h"
#include "engine/scene/square.h"
#include "engine/shaderprogram.h"

#include <SDL_events.h>
#include <SDL_video.h>
#include <chrono>

class Editor {
public:
  Editor();
  ~Editor();

  int initialize(SDL_Window *window, SDL_GLContext gl_context);
  void resize(int width, int height);
  void paint();
  void processEvent(const SDL_Event &event);
  void calculateOffsets(int numInstances, float dt);
  void initInstances();
  void startSimulation();
  void resetSimulation();

  void setNumInstances(int numInstances);
  void setParticleSize(float particleSize);
  void setParticleDamping(float particleDamping);
  void setParticleSpacing(float particleSpacing);
  void setDensityRadius(float densityRadius);
  void setTargetDensity(float targetDensity);
  void setPressureMultiplier(float pressureMultiplier);
  void setBounds(glm::vec2 bounds);

  bool getStarted();
  float getDensity();

private:
  SDL_Window *mp_window;
  int m_width;
  int m_height;

  GLuint vao;

  ShaderProgram m_prog_flat;
  ShaderProgram m_prog_instanced;
  Square m_square;
  Square m_square2;
  Circle m_circle;
  Circle m_densityCircle;
  glm::vec2 m_bounds;

  Camera m_camera;

  // Elapsed time in milliseconds
  int m_elapsed_time;
  // Last time the paint function was called
  std::chrono::high_resolution_clock::time_point m_lastTime;

  // Velocites and Positions
  std::vector<glm::vec3> m_offsets;
  std::vector<glm::vec3> m_velocities;
  std::vector<float> m_densities;
  void resolveCollisions();

  // Number of instances
  int m_numInstances;
  // Particle Size
  float m_particleSize;
  // Particle Damping Factor
  float m_particleDamping;
  // Particle Spacing
  float m_particleSpacing;
  // Started Simulation:
  bool m_started;
  // Density Radius
  float m_densityRadius;
  // Target Density
  float m_targetDensity;
  // Pressure Multiplier
  float m_pressureMultiplier;
};
;