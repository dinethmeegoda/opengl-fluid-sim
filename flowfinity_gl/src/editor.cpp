#include "editor.h"
#include "engine/drawable.h"
#include "flowfinity.h"

#include <SDL.h>
#include <SDL_events.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <SDL_video.h>
#include <chrono>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_int3_sized.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

Editor::Editor()
    : m_square(), m_square2(), m_circle(),
      m_densityCircle(1, 25, glm::vec3(255, 0, 0)), m_bounds(glm::vec2(7.5, 4)),
      m_prog_flat(), m_camera(), m_elapsed_time(0),
      m_lastTime(std::chrono::high_resolution_clock::now()), m_offsets(),
      m_velocities(), m_densities(), m_numInstances(10), m_particleSize(1),
      m_particleDamping(-0.1), m_particleSpacing(0), m_started(false),
      m_densityRadius(1), m_targetDensity(2.75), m_pressureMultiplier(10), m_gravity(0), m_randomLocation(false), m_randomLocationGenerated(false) {}

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
  // if the random locations are on, and they have been generated, don't reset the offsets
  if (!m_randomLocation || !m_randomLocationGenerated) {
	m_offsets.clear();
  }
  m_velocities.clear();
  m_densities.clear();
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
}

void Editor::setPressureMultiplier(float pressureMultiplier) {
  m_pressureMultiplier = pressureMultiplier;
}

void Editor::setGravity(float gravity) {
  m_gravity = gravity;
}

void Editor::setBounds(glm::vec2 bounds) { m_bounds = bounds; }

void Editor::setRandomLocation(bool randomLocation) {
  m_randomLocation = randomLocation;
}

void Editor::setRandomLocationGenerated(bool randomLocationGenerated) {
  m_randomLocationGenerated = randomLocationGenerated;
}

bool Editor::getStarted() { return m_started; }

float Editor::getDensity() {
  return FlowFinity::calculateDensity(glm::vec3(0), m_offsets, m_densityRadius);
}

void Editor::initInstances() {

  // Place particles in a grid formation
  int particlesPerRow = (int)sqrt(m_numInstances);
  int particlesPerCol = (m_numInstances - 1) / particlesPerRow + 1;
  float spacing = m_particleSpacing + m_particleSize * 2;

  for (int i = 0; i < m_numInstances; i++) {
      m_densities.push_back(0);
      m_velocities.push_back(glm::vec3(0, 0, 0));
      if (!m_randomLocation) {
          m_offsets.push_back(glm::vec3(
              (i % particlesPerRow) * spacing - (particlesPerRow - 1) * spacing / 2.f,
              (i / particlesPerRow) * spacing - (particlesPerCol - 1) * spacing / 2.f,
              0));
      }
      // If the random Locations havent already been generated
      else if (!m_randomLocationGenerated){
          // Push back a random position within the bounds -x to x, -y to y
          m_offsets.push_back(glm::vec3(
              (rand() % (int)(m_bounds.x * 2 * 100) - (int)m_bounds.x * 100) / 100.f,
              (rand() % (int)(m_bounds.y * 2 * 100) - (int)m_bounds.y * 100) / 100.f,
              0));
      }
  }
  // If the random locations are on but have not been generated, they should be generated now
  if (m_randomLocation && !m_randomLocationGenerated) {
	  m_randomLocationGenerated = true;
  }
  m_lastTime = std::chrono::high_resolution_clock::now();
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

void Editor::calculateOffsets(int num, float dt) {
  // Apply gravity and calculate Densities
  for (int i = 0; i < num; i++) {
    m_velocities[i] += glm::vec3(0, m_gravity, 0) * dt;
    m_densities[i] =
        FlowFinity::calculateDensity(m_offsets[i], m_offsets, m_densityRadius);
  }
  // Calculate and apply pressure forces
  for (int i = 0; i < num; i++) {
    glm::vec3 pressureForce = FlowFinity::CalulatePressureForce(
        i, m_offsets, m_densities, m_densityRadius, m_targetDensity,
        m_pressureMultiplier);
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