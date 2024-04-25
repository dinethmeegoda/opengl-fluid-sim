#include "flowfinity.h"

#include <cmath>
#include <glm/ext/scalar_constants.hpp>
#include <glm/geometric.hpp>

FlowFinity::FlowFinity()
    : m_densities(nullptr), m_positions(nullptr), m_targetDensity(2.75),
      m_pressureMultiplier(2) {}

FlowFinity::~FlowFinity() {}

#define M_PI 3.14159265358979323846 /* pi */

float FlowFinity::smoothingKernel(float r, float dst) {
  if (dst >= r) {
    return 0;
  }
  float volume = M_PI * std::pow(r, 4) / 6;
  return (r - dst) * (r - dst) / volume;
}

float FlowFinity::smoothingKernelDerivative(float r, float dst) {
  if (dst >= r) {
    return 0;
  }
  float scale = 12 / (M_PI * std::pow(r, 4));
  return scale * (dst - r);
}

float FlowFinity::calculateDensity(int posIndex, int i, float smoothingRadius) {
  float density = 0;
  const float mass = 1;

  for (auto &p : *m_positions) {
    float dst = glm::distance((*m_positions)[posIndex], p);
    float influence = smoothingKernel(smoothingRadius, dst);
    density += mass * influence;
  }
  return density;
}

glm::vec3 getRandomDir() {
  float x = (rand() % 100) / 100.0f;
  float y = (rand() % 100) / 100.0f;
  return glm::vec3(x, y, 0);
}

glm::vec3 FlowFinity::CalulatePressureForce(int posIndex, int j,
                                            float smoothingRadius) {
  // Calculate the pressure force for the specific position
  glm::vec3 pressureForce = glm::vec3(0);
  const float mass = 1;

  for (int i = 0; i < m_positions->size(); i++) {
    if (i == posIndex) {
      continue;
    }
    float dst = glm::distance((*m_positions)[posIndex], (*m_positions)[i]);
    glm::vec3 dir = dst == 0
                        ? getRandomDir()
                        : ((*m_positions)[posIndex] - (*m_positions)[i]) / dst;
    float slope = smoothingKernelDerivative(smoothingRadius, dst);
    float density = (*m_densities)[i];
    float pressureA = m_pressureMultiplier * (density - m_targetDensity);
    float pressureB =
        m_pressureMultiplier * ((*m_densities)[posIndex] - m_targetDensity);

    float sharedPressure = pressureA + pressureB / 2.f;

    pressureForce += -sharedPressure * dir * slope * mass / density;
  }
  return pressureForce;
}

void FlowFinity::setPositions(std::vector<glm::vec3> *positions) {
  m_positions = positions;
}

void FlowFinity::setDensities(std::vector<float> *densities) {
  m_densities = densities;
}

void FlowFinity::setTargetDensity(float targetDensity) {
  m_targetDensity = targetDensity;
}

void FlowFinity::setPressureMultiplier(float pressureMultiplier) {
  m_pressureMultiplier = pressureMultiplier;
}
