#include "editor.h"
#include "engine/drawable.h"
#include "flowfinity.h"

#include <SDL.h>
#include <SDL_events.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <SDL_video.h>
#include <algorithm>
#include <chrono>
#include <climits>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_int3_sized.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

Editor::Editor()
    : m_square(), m_square2(), m_circle(),
      m_densityCircle(1, 25, glm::vec3(255, 0, 0)), m_bounds(glm::vec2(7.5, 4)),
      m_prog_flat(), m_camera(), m_flowFinity(), m_elapsed_time(0),
      m_lastTime(std::chrono::high_resolution_clock::now()), m_offsets(),
      m_velocities(), m_predicted_positions(), m_densities(),
      m_numInstances(10), m_particleSize(1), m_particleDamping(-0.1),
      m_particleSpacing(0), m_started(false), m_densityRadius(1),
      m_targetDensity(2.75), m_pressureMultiplier(10), m_gravity(0),
      m_randomLocation(false), m_randomLocationGenerated(false),
      m_spatialHash(), m_startIndices() {}

Editor::~Editor() {
  glDeleteVertexArrays(1, &vao);
  m_square.destroy();
}

int Editor::initialize(SDL_Window *window, SDL_GLContext gl_context) {
  mp_window = window;

  SDL_GL_GetDrawableSize(window, &m_width, &m_height);
  m_camera = Camera(m_width, m_height);
  m_camera.TranslateAlongUp(-1);

  GLenum err = glewInit();
  if (err != GLEW_OK) {
    std::cout << "Failed to init GLEW" << std::endl;
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  // Set a few settings/modes in OpenGL rendering
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_POLYGON_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
  // Set the size with which points should be rendered
  glPointSize(5);
  // Set the color with which the screen is filled at the start of each render
  // call.
  glClearColor(0.25, 0.25, 0.25, 1);

  // Create a Vertex Attribute Object
  glGenVertexArrays(1, &vao);

  // m_square.create();
  // m_square2.create();
  m_circle.create();
  // m_densityCircle.create();
  m_prog_instanced.create("instanced.vert.glsl", "instanced.frag.glsl");
  // m_prog_flat.create("passthrough.vert.glsl", "flat.frag.glsl");
  initInstances();

  // We have to have a VAO bound in OpenGL 3.2 Core. But if we're not
  // using multiple VAOs, we can just bind one once.
  glBindVertexArray(vao);

  m_lastTime = std::chrono::high_resolution_clock::now();

  return 0;
}

// Getters and Setters
void Editor::startSimulation() { m_started = true; }

void Editor::resetSimulation() {
  m_elapsed_time = 0;
  // if the random locations are on, and they have been generated, don't reset
  // the offsets
  if (!m_randomLocation || !m_randomLocationGenerated) {
    m_offsets.clear();
  }
  m_velocities.clear();
  m_predicted_positions.clear();
  m_densities.clear();
  m_spatialHash.clear();
  m_startIndices.clear();
  m_started = false;
  initInstances();
}

void Editor::setNumInstances(int numInstances) {
  m_numInstances = numInstances;
}

void Editor::setParticleSize(float particleSize) {
  if (particleSize == m_particleSize) {
    return;
  } else {
    m_circle.setRadius(particleSize);
    m_circle.create();
    m_particleSize = particleSize;
  }
}

void Editor::setParticleDamping(float particleDamping) {
  m_particleDamping = particleDamping;
}

void Editor::setParticleSpacing(float particleSpacing) {
  m_particleSpacing = particleSpacing;
}

void Editor::setDensityRadius(float densityRadius) {
  m_densityCircle.setRadius(densityRadius);
  // m_densityCircle.create();
  m_densityRadius = densityRadius;
}

void Editor::setTargetDensity(float targetDensity) {
  m_targetDensity = targetDensity;
  m_flowFinity.setTargetDensity(targetDensity);
}

void Editor::setPressureMultiplier(float pressureMultiplier) {
  m_pressureMultiplier = pressureMultiplier;
  m_flowFinity.setPressureMultiplier(pressureMultiplier);
}

void Editor::setGravity(float gravity) { m_gravity = gravity; }

void Editor::setBounds(glm::vec2 bounds) { m_bounds = bounds; }

void Editor::setRandomLocation(bool randomLocation) {
  m_randomLocation = randomLocation;
}

void Editor::setRandomLocationGenerated(bool randomLocationGenerated) {
  m_randomLocationGenerated = randomLocationGenerated;
}

bool Editor::getStarted() { return m_started; }

float Editor::getDensity() {
  // return m_flowFinity.calculateDensity(glm::vec3(0), m_densityRadius);
  return 0;
}

void Editor::initInstances() {

  // Place particles in a grid formation
  int particlesPerRow = (int)sqrt(m_numInstances);
  int particlesPerCol = (m_numInstances - 1) / particlesPerRow + 1;
  float spacing = m_particleSpacing + m_particleSize * 2;

  for (int i = 0; i < m_numInstances; i++) {
    m_densities.push_back(0);
    m_spatialHash.push_back(std::make_pair(0, 0));
    m_startIndices.push_back(INT_MAX);
    m_velocities.push_back(glm::vec3(0, 0, 0));
    m_predicted_positions.push_back(glm::vec3(0, 0, 0));
    if (!m_randomLocation) {
      m_offsets.push_back(glm::vec3((i % particlesPerRow) * spacing -
                                        (particlesPerRow - 1) * spacing / 2.f,
                                    (i / particlesPerRow) * spacing -
                                        (particlesPerCol - 1) * spacing / 2.f,
                                    0));
    }
    // If the random Locations havent already been generated
    else if (!m_randomLocationGenerated) {
      // Push back a random position within the bounds -x to x, -y to y
      m_offsets.push_back(glm::vec3(
          (rand() % (int)(m_bounds.x * 2 * 100) - (int)m_bounds.x * 100) /
              100.f,
          (rand() % (int)(m_bounds.y * 2 * 100) - (int)m_bounds.y * 100) /
              100.f,
          0));
    }
  }
  // If the random locations are on but have not been generated, they should be
  // generated now
  if (m_randomLocation && !m_randomLocationGenerated) {
    m_randomLocationGenerated = true;
  }
  m_lastTime = std::chrono::high_resolution_clock::now();
}

// Hashing Helper Functions

glm::vec2 positionToCell(glm::vec3 pos, float radius) {
  return glm::vec2((int)(pos.x / radius), (int)(pos.y / radius));
}

unsigned int hashCell(glm::vec2 cell) {
  unsigned int a = (unsigned int)cell.x * 15823;
  unsigned int b = (unsigned int)cell.y * 9737333;
  return a + b;
}

unsigned int Editor::getKeyFromHash(unsigned int hash) {
  // Maybe size? Idk
  return hash % (unsigned int)m_spatialHash.size();
}

void Editor::updateSpatialHash(float radius) {
  for (int i = 0; i < m_offsets.size(); i++) {
    // Gets Cell Key for each particle and updates for each index
    glm::vec2 cell = positionToCell(m_offsets[i], radius);
    unsigned int hash = getKeyFromHash(hashCell(cell));
    m_spatialHash[i] = std::make_pair(hash, i);
  }

  // Sort the spatial hash array by the first value in the pair, the cell key
  std::sort(m_spatialHash.begin(), m_spatialHash.end(),
            [](auto &left, auto &right) { return left.first < right.first; });

  // Find the start indices for each cell
  for (int i = 0; i < m_spatialHash.size(); i++) {
    unsigned int hash = m_spatialHash[i].first;
    // If the hash is different from the previous hash, update the start index
    unsigned int hashPrev = i == 0 ? INT_MAX : m_spatialHash[i - 1].first;
    if (hash != hashPrev) {
      m_startIndices[hash] = i;
    }
  }
}

void Editor::forEachPointInRadius(glm::vec3 pos, std::vector<int> &neighbors,
                                  float radius) {
  // Get the cell of the position, the center of the 3x3 grid
  glm::vec2 cell = positionToCell(pos, radius);
  float sqrRadius = radius * radius;

  glm::vec3 sum(0);
  // Loop through the 3x3 grid of cells
  for (glm::vec2 offset : cellOffsets) {
    // Get key of current cell
    unsigned int key = getKeyFromHash(hashCell(cell + offset));
    int cellStartIndex = m_startIndices[key];

    // Loop over all points that have the key
    for (int i = cellStartIndex; i < m_spatialHash.size(); i++) {
      // Exit if the key is different (not the correct cell)
      if (m_spatialHash[i].first != key) {
        break;
      }

      int index = m_spatialHash[i].second;
      glm::vec3 point = m_offsets[index];
      float sqrDst = std::pow(glm::distance(pos, point), 2);
      if (sqrDst < sqrRadius) {
        neighbors.push_back(index);
      }
    }
  }
}

void Editor::resolveCollisions() {
  glm::vec2 bounds = m_bounds - glm::vec2(m_particleSize / 2.f);
  for (int i = 0; i < m_numInstances; i++) {
    if (m_offsets[i].x < -bounds.x) {
      m_offsets[i].x = -bounds.x;
      m_velocities[i].x *= -(1 - m_particleDamping);
    } else if (m_offsets[i].x > bounds.x) {
      m_offsets[i].x = bounds.x;
      m_velocities[i].x *= -(1 - m_particleDamping);
    }
    if (m_offsets[i].y < -bounds.y) {
      m_offsets[i].y = -bounds.y;
      m_velocities[i].y *= -(1 - m_particleDamping);
    } else if (m_offsets[i].y > bounds.y) {
      m_offsets[i].y = bounds.y;
      m_velocities[i].y *= -(1 - m_particleDamping);
    }
  }
}

// glm::vec3 Editor::calcDensityHelper(int pos0, int pos2, float r) {
//   return glm::vec3(m_flowFinity.calculateDensity(pos0, pos2, r), 0, 0);
// }

// glm::vec3 Editor::calcPressureHelper(int pos0, int pos2, float r) {
//   return m_flowFinity.CalulatePressureForce(pos0, pos2, r);
// }

void Editor::calculateOffsets(int num, float dt) {
  m_flowFinity.setDensities(&m_densities);
  m_flowFinity.setPositions(&m_predicted_positions);
  // Apply gravity and calculate Densities
  for (int i = 0; i < num; i++) {
    m_velocities[i] += glm::vec3(0, m_gravity, 0) * dt;
    m_predicted_positions[i] = m_offsets[i] + m_velocities[i] * (1 / 120.f);
  }

  // Update the spatial hash
  updateSpatialHash(m_densityRadius);

  for (int i = 0; i < num; i++) {
    std::vector<int> neighbors;
    forEachPointInRadius(m_predicted_positions[i], neighbors, m_densityRadius);
    m_densities[i] =
        m_flowFinity.calculateDensity(i, m_densityRadius, neighbors);
    // forEachPointInRadius(m_offsets[i], i, m_densityRadius,
    // &Editor::calcDensityHelper)[0];
  }

  // Calculate and apply pressure forces
  for (int i = 0; i < num; i++) {
    std::vector<int> neighbors;
    forEachPointInRadius(m_offsets[i], neighbors, m_densityRadius);
    glm::vec3 pressureForce =
        m_flowFinity.CalulatePressureForce(i, m_densityRadius, neighbors);
    // forEachPointInRadius(m_offsets[i], i, m_densityRadius,
    // &Editor::calcPressureHelper);
    glm::vec3 acceleration = pressureForce / m_densities[i];
    m_velocities[i] += acceleration * dt;
  }
  // Update Positions and resolve collisions
  for (int i = 0; i < num; i++) {
    m_offsets[i] += m_velocities[i] * dt;
    resolveCollisions();
  }
}

void Editor::paint() {
  // m_prog_flat.setModelMatrix(
  //     glm::translate(glm::mat4(1.f), glm::vec3(0, 0, -1)));
  // m_prog_flat.setViewProjMatrix(m_camera.getViewProj());

  // Set Camera Position and Matrices
  m_prog_instanced.setModelMatrix(glm::mat4(1.f));
  m_prog_instanced.setViewProjMatrix(m_camera.getViewProj());

  if (m_started) {
    // Calculate Time

    int deltaTime = (std::chrono::high_resolution_clock::now() - m_lastTime) /
                    std::chrono::milliseconds(1);
    m_elapsed_time += deltaTime;
    m_lastTime = std::chrono::high_resolution_clock::now();
    // Set Instanced Rendering Variables and Velocites
    calculateOffsets(m_numInstances, deltaTime / 1000.f);
    m_prog_instanced.setTime(m_elapsed_time);
    m_prog_instanced.setDeltaTime(deltaTime / 1000.f);
  }

  m_prog_instanced.setOffsets(m_offsets, m_numInstances);
  m_prog_instanced.setNumInstances(m_numInstances);

  SDL_GL_GetDrawableSize(mp_window, &m_width, &m_height);
  m_camera.width = m_width;
  m_camera.height = m_height;
  glViewport(0, 0, m_width, m_height);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // m_prog_flat.setModelMatrix(glm::scale(
  //     glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(1, 0, 0)),
  //     glm::vec3(10, 10, 0)));

  // m_prog_flat.setModelMatrix(
  //     glm::scale(glm::mat4(1.f), glm::vec3(7.5, 4.25, 0)));

  // m_prog_flat.draw(m_square);
  // m_prog_flat.draw(m_densityCircle);
  m_prog_instanced.drawInstanced(m_circle, m_numInstances);
}

void Editor::processEvent(const SDL_Event &event) {
  switch (event.type) {
  case SDL_MOUSEMOTION:
    if (event.motion.state & SDL_BUTTON_LMASK) {
      m_camera.RotateAboutUp(-event.motion.xrel * 0.25);
      m_camera.RotateAboutRight(-event.motion.yrel * 0.25);
      m_camera.RecomputeAttributes();
    } else if (event.motion.state & SDL_BUTTON_RMASK) {
      m_camera.ScaleZoom(1. + event.motion.yrel * 0.005);
      m_camera.RecomputeAttributes();
    }
    break;
  case SDL_MOUSEWHEEL:
    m_camera.ScaleZoom(1. - event.wheel.y * 0.1);
    m_camera.RecomputeAttributes();
    break;
  }
}