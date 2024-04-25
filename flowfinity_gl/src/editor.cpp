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

// Editor Constructor (Default Values)
Editor::Editor()
    : m_square(), m_square2(), m_circle(),
      m_inputCircle(1, 25, glm::vec3(255, 0, 0)), m_bounds(glm::vec2(7.5, 4)),
      m_prog_flat(), m_camera(), m_flowFinity(), m_elapsed_time(0),
      m_lastTime(std::chrono::high_resolution_clock::now()), m_positions(),
      m_velocities(), m_predicted_positions(), m_densities(),
      m_numInstances(10), m_particleSize(1), m_particleDamping(-0.1),
      m_particleSpacing(0), m_started(false), m_densityRadius(1),
      m_targetDensity(2.75), m_pressureMultiplier(10), m_gravity(0),
      m_randomLocation(false), m_randomLocationGenerated(false),
      m_spatialHash(), m_startIndices(), m_maxVelocity(0),
      m_testClickPoint(0, 0), m_clickStrength(0) {}

Editor::~Editor() {
  glDeleteVertexArrays(1, &vao);
  m_square.destroy();
}

// Initialize the Editor OpenGL Context
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
  m_inputCircle.drawMode();
  m_inputCircle.createLines();
  m_prog_instanced.create("instanced.vert.glsl", "instanced.frag.glsl");
  m_prog_flat.create("passthrough.vert.glsl", "flat.frag.glsl");
  initInstances();

  // We have to have a VAO bound in OpenGL 3.2 Core. But if we're not
  // using multiple VAOs, we can just bind one once.
  glBindVertexArray(vao);

  // Last ticked time
  m_lastTime = std::chrono::high_resolution_clock::now();

  return 0;
}

// Command to start the simulation
void Editor::startSimulation() { m_started = true; }

// Refresh the simulation (runs every tick if simulation is not started)
void Editor::resetSimulation() {
  m_elapsed_time = 0;
  // if the random locations are on, and they have been generated, don't reset
  // the offsets
  if (!m_randomLocation || !m_randomLocationGenerated) {
    m_positions.clear();
  }
  m_velocities.clear();
  m_predicted_positions.clear();
  m_densities.clear();
  m_spatialHash.clear();
  m_startIndices.clear();
  m_started = false;
  m_maxVelocity = 0;
  initInstances();
}

// Run this right before starting up the simulation
void Editor::initInstances() {

  // Place particles in a grid formation
  int particlesPerRow = (int)sqrt(m_numInstances);
  int particlesPerCol = (m_numInstances - 1) / particlesPerRow + 1;
  float spacing = m_particleSpacing + m_particleSize * 2;

  // Give Vectors Initial Values
  for (int i = 0; i < m_numInstances; i++) {
    m_densities.push_back(0);
    m_spatialHash.push_back(std::make_pair(0, 0));
    m_startIndices.push_back(INT_MAX);
    m_velocities.push_back(glm::vec3(0, 0, 0));
    m_predicted_positions.push_back(glm::vec3(0, 0, 0));
    if (!m_randomLocation) {
      m_positions.push_back(glm::vec3((i % particlesPerRow) * spacing -
                                          (particlesPerRow - 1) * spacing / 2.f,
                                      (i / particlesPerRow) * spacing -
                                          (particlesPerCol - 1) * spacing / 2.f,
                                      0));
    }
    // If the random Locations havent already been generated
    else if (!m_randomLocationGenerated) {
      // Push back a random position within the bounds -x to x, -y to y
      m_positions.push_back(glm::vec3(
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
  for (int i = 0; i < m_positions.size(); i++) {
    // Gets Cell Key for each particle and updates for each index
    glm::vec2 cell = positionToCell(m_positions[i], radius);
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
      glm::vec3 point = m_positions[index];
      float sqrDst = std::pow(glm::distance(pos, point), 2);
      if (sqrDst < sqrRadius) {
        neighbors.push_back(index);
      }
    }
  }
}

// Resolve Collisions with the bounds and obstacles
void Editor::resolveCollisions() {
  glm::vec2 bounds = m_bounds - glm::vec2(m_particleSize / 2.f);
  for (int i = 0; i < m_numInstances; i++) {
    if (m_positions[i].x < -bounds.x) {
      m_positions[i].x = -bounds.x;
      m_velocities[i].x *= -(1 - m_particleDamping);
    } else if (m_positions[i].x > bounds.x) {
      m_positions[i].x = bounds.x;
      m_velocities[i].x *= -(1 - m_particleDamping);
    }
    if (m_positions[i].y < -bounds.y) {
      m_positions[i].y = -bounds.y;
      m_velocities[i].y *= -(1 - m_particleDamping);
    } else if (m_positions[i].y > bounds.y) {
      m_positions[i].y = bounds.y;
      m_velocities[i].y *= -(1 - m_particleDamping);
    }
  }
}

// Calculate the interaction force between a particle and the input point
// (usually the mouse)
glm::vec2 Editor::interactionForce(int index, float radius, float strength) {
  glm::vec2 interactionForce = glm::vec2(0);
  glm::vec2 offset = m_testClickPoint * 2.f - glm::vec2(m_positions[index]);
  float sqrDst = glm::dot(offset, offset);

  // If a particle is inside of input radius, calculate force towards input
  // point
  if (sqrDst < radius * radius) {
    float dst = sqrt(sqrDst);
    glm::vec2 dir = dst <= std::numeric_limits<float>::epsilon() ? glm::vec2(0)
                                                                 : offset / dst;
    // Value is 1 when particle is exactly at the input point, 0 at the edge
    float t = 1 - dst / radius;
    // Calculate interaction force
    interactionForce += (dir * (float)m_clickStrength * strength -
                         glm::vec2(m_velocities[index])) *
                        t;
  }
  return interactionForce;
}

// Check for interactions (clicks) and apply forces to the particles
void Editor::checkInterations() {
  // If there is a click, apply a force to the particles
  if (m_clickStrength != 0) {
    // Figure out the neighbors of the click point
    std::vector<int> neighbors;
    float radius = 2;
    float strength = 4;
    updateSpatialHash(radius);
    forEachPointInRadius(glm::vec3(m_testClickPoint * 2.f, 0), neighbors,
                         radius);
    // For each particle in the clickPoint's radius, apply a force
    for (auto &i : neighbors) {
      glm::vec2 force = interactionForce(i, radius, strength);
      m_velocities[i] += glm::vec3(force, 0) * (1 / 12.f);
      m_maxVelocity = std::max(m_maxVelocity, glm::length(m_velocities[i]));
    }
  }
}

// Using Leapfrog Integration to calculate the predicted positions and
// velocities
void Editor::calculateOffsets(int num, float dt) {
  m_flowFinity.setDensities(&m_densities);
  m_flowFinity.setPositions(&m_predicted_positions);
  // Apply gravity and calculate Densities
  for (int i = 0; i < num; i++) {
    // Leapfrog Step 1: Calculate half step velocity
    glm::vec3 halfStepVelocity =
        m_velocities[i] + glm::vec3(0, m_gravity, 0) * 0.5f * dt;
    m_predicted_positions[i] = m_positions[i] + halfStepVelocity * (1 / 120.f);
  }

  // Update the spatial hash
  updateSpatialHash(m_densityRadius);

  // Update Density Map for efficiency
  for (int i = 0; i < num; i++) {
    std::vector<int> neighbors;
    forEachPointInRadius(m_predicted_positions[i], neighbors, m_densityRadius);
    m_densities[i] =
        m_flowFinity.calculateDensity(i, m_densityRadius, neighbors);
  }

  // Calculate and apply forces (Pressure)
  for (int i = 0; i < num; i++) {
    std::vector<int> neighbors;
    forEachPointInRadius(m_positions[i], neighbors, m_densityRadius);
    glm::vec3 pressureForce =
        m_flowFinity.CalulatePressureForce(i, m_densityRadius, neighbors);
    glm::vec3 acceleration = pressureForce / m_densities[i];

    // Leapfrog Step 2: Calculate full step velocity
    m_velocities[i] = m_velocities[i] + acceleration * dt +
                      (glm::vec3(0, m_gravity, 0) * 0.5f * dt);
    // Update the max velocity
    m_maxVelocity = std::max(m_maxVelocity, glm::length(m_velocities[i]));
  }

  // Check if the mouse is interacting with the particles
  checkInterations();

  // Update Positions with Euler Integration and resolve collisions
  for (int i = 0; i < num; i++) {
    m_positions[i] += m_velocities[i] * dt;
    resolveCollisions();
  }
}

// Main OpenGL Rendering Loop
void Editor::paint() {
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
    m_prog_instanced.setMaxVelocity(m_maxVelocity);
    m_prog_instanced.setTime(m_elapsed_time);
    m_prog_instanced.setDeltaTime(deltaTime / 1000.f);
  } else if (!m_randomLocation) {
    // Only allow change of number of instances if random locations are off
    m_prog_instanced.setNumInstances(m_numInstances);
  }

  SDL_GL_GetDrawableSize(mp_window, &m_width, &m_height);
  m_camera.width = m_width;
  m_camera.height = m_height;
  glViewport(0, 0, m_width, m_height);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Code to draw square plane
  // m_prog_flat.setModelMatrix(glm::scale(
  //     glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(1, 0, 0)),
  //     glm::vec3(10, 10, 0)));

  // m_prog_flat.draw(m_square);

  // Draw the particles with instanced rendering and send the positions and
  // velocities to the shader
  m_prog_instanced.drawInstanced(m_circle, m_numInstances, m_positions,
                                 m_velocities);

  // Draw the input circle around the cursor
  m_prog_flat.setModelMatrix(glm::scale(
      glm::translate(glm::mat4(1.f), glm::vec3(m_testClickPoint * 2.19f, -1)),
      glm::vec3(m_inputRadius, m_inputRadius, 0)));
  m_prog_flat.setViewProjMatrix(m_camera.getViewProj());
  m_prog_flat.draw(m_inputCircle);
}

// Helper function to go from SDL event coordinates to world coordinates
glm::vec2 screenToWorld(int x, int y, int width, int height) {
  return glm::vec2((x / (float)width - 0.5f) * 2.f * 3.69,
                   (-y / (float)height + 0.5f) * 2.f * 2.09);
}

// Process Events (Mouse Clicks, Mouse Movement, etc.)
void Editor::processEvent(const SDL_Event &event) {
  // Get rid of camera motion for now
  // Update cursor position at all times, even if not clicked
  // If left clicked, strength is positive, right clicked, strength is negative
  switch (event.type) {
  case SDL_MOUSEBUTTONDOWN:
    m_testClickPoint =
        screenToWorld(event.button.x, event.button.y, m_width, m_height);
    if (event.button.button == SDL_BUTTON_LEFT) {
      m_clickStrength = 1;
    } else if (event.button.button == SDL_BUTTON_RIGHT) {
      m_clickStrength = -1;
    }
    break;
  case SDL_MOUSEMOTION:
    m_testClickPoint =
        screenToWorld(event.button.x, event.button.y, m_width, m_height);
    if (event.motion.state & SDL_BUTTON_LMASK) {
      m_clickStrength = 1;
    } else if (event.motion.state & SDL_BUTTON_RMASK) {
      m_clickStrength = -1;
    }
    break;
  case SDL_MOUSEBUTTONUP:
    m_testClickPoint =
        screenToWorld(event.button.x, event.button.y, m_width, m_height);
    if (event.button.button == SDL_BUTTON_LEFT ||
        event.button.button == SDL_BUTTON_RIGHT) {
      m_clickStrength = 0;
    }
    break;
    // case SDL_MOUSEWHEEL:
    //   m_camera.ScaleZoom(1. - event.wheel.y * 0.1);
    //   m_camera.RecomputeAttributes();
    //   break;
  }
}

// Getters and Setters
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
  m_inputCircle.setRadius(densityRadius);
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

void Editor::setInputRadius(float inputRadius) { m_inputRadius = inputRadius; }

void Editor::setInputStrengthMultiplier(float inputStrengthMultiplier) {
  m_inputStrengthMultiplier = inputStrengthMultiplier;
}

bool Editor::getStarted() { return m_started; }

float Editor::getDensity() {
  // return m_flowFinity.calculateDensity(glm::vec3(0), m_densityRadius);
  return 0;
}