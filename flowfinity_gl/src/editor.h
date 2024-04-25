#include "engine/camera.h"
#include "engine/scene/circle.h"
#include "engine/scene/square.h"
#include "engine/shaderprogram.h"
#include "flowfinity.h"

#include <SDL_events.h>
#include <SDL_video.h>
#include <chrono>
#include <functional>

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

  void updateSpatialHash(float radius);
  unsigned int getKeyFromHash(unsigned int hash);
  void forEachPointInRadius(glm::vec3 pos, std::vector<int> &neighbors,
                            float radius);

  glm::vec3 calcDensityHelper(int pos1, int pos2, float r);
  glm::vec3 calcPressureHelper(int pos1, int pos2, float r);

  void setNumInstances(int numInstances);
  void setParticleSize(float particleSize);
  void setParticleDamping(float particleDamping);
  void setParticleSpacing(float particleSpacing);
  void setDensityRadius(float densityRadius);
  void setTargetDensity(float targetDensity);
  void setPressureMultiplier(float pressureMultiplier);
  void setGravity(float gravity);
  void setBounds(glm::vec2 bounds);
  void setRandomLocation(bool randomLocation);
  void setRandomLocationGenerated(bool randomLocationGenerated);
  void setInputRadius(float inputRadius);
  void setInputStrengthMultiplier(float inputStrengthMultiplier);

  bool getStarted();
  float getDensity();

  // Click Strength
  int m_clickStrength;

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
  Circle m_inputCircle;
  glm::vec2 m_bounds;

  Camera m_camera;

  FlowFinity m_flowFinity;

  void checkInterations();
  glm::vec2 interactionForce(int index, float radius, float strength);

  // Elapsed time in milliseconds
  int m_elapsed_time;
  // Last time the paint function was called
  std::chrono::high_resolution_clock::time_point m_lastTime;

  // Velocites and Positions
  std::vector<glm::vec3> m_positions;
  std::vector<glm::vec3> m_velocities;
  std::vector<glm::vec3> m_predicted_positions;
  std::vector<float> m_densities;
  void resolveCollisions();

  // Particle Location Hashing
  // Spatial hash stores <cell key, particle index>
  std::vector<std::pair<int, int>> m_spatialHash;
  std::vector<int> m_startIndices;

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
  // Gravity
  float m_gravity;
  // Random Location
  bool m_randomLocation;
  // Random Location Generated
  bool m_randomLocationGenerated;
  // Max Velocity in the current timestep
  float m_maxVelocity;
  // Test Click Point
  glm::vec2 m_testClickPoint;
  // Input radius
  float m_inputRadius;
  // Input strength Multiplier
  float m_inputStrengthMultiplier;

  constexpr static const glm::vec2 cellOffsets[9] = {
      glm::vec2(-1, 1),  glm::vec2(0, 1),  glm::vec2(1, 1),
      glm::vec2(-1, 0),  glm::vec2(0, 0),  glm::vec2(1, 0),
      glm::vec2(-1, -1), glm::vec2(0, -1), glm::vec2(1, -1),
  };
};
;