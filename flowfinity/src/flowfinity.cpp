#include "flowfinity.h"
#include <algorithm>
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

float FlowFinity::smoothingViscosityKernel(float r, float dst) {
  float value = r * r - dst * dst;
  return value * value * value;
}

float FlowFinity::calculateDensity(int posIndex, float smoothingRadius,
                                   std::vector<int> &neighbors) {
  float density = 0;
  const float mass = 1;

  for (auto &i : neighbors) {
    float dst = glm::distance((*m_positions)[posIndex], (*m_positions)[i]);
    float influence = smoothingKernel(smoothingRadius, dst);
    density += mass * influence;
  }
  return density;
}

float FlowFinity::calculateDensity(glm::vec3 pos, int neighborIndex,
                                   float smoothingRadius) {
  const float mass = 1;

  float dst = glm::distance(pos, (*m_positions)[neighborIndex]);
  float influence = smoothingKernel(smoothingRadius, dst);
  return mass * influence;
}

glm::vec3 getRandomDir() {
  float x = (rand() % 100) / 100.0f;
  float y = (rand() % 100) / 100.0f;
  return glm::vec3(x, y, 0);
}

glm::vec3 FlowFinity::CalulatePressureForce(int posIndex, int neighborIndex,
                                            float smoothingRadius) {
  // Calculate the pressure force for the specific position
  const float mass = 1;
  if (posIndex == neighborIndex) {
    return glm::vec3(0);
  }
  float dst =
      glm::distance((*m_positions)[posIndex], (*m_positions)[neighborIndex]);
  glm::vec3 dir =
      dst == 0
          ? getRandomDir()
          : ((*m_positions)[posIndex] - (*m_positions)[neighborIndex]) / dst;
  float slope = smoothingKernelDerivative(smoothingRadius, dst);
  float density = (*m_densities)[neighborIndex];
  float pressureA = m_pressureMultiplier * (density - m_targetDensity);
  float pressureB =
      m_pressureMultiplier * ((*m_densities)[posIndex] - m_targetDensity);

  float sharedPressure = pressureA + pressureB / 2.f;

  return -sharedPressure * dir * slope * mass / density;
}

glm::vec3 FlowFinity::CalulatePressureForce(int posIndex, float smoothingRadius,
                                            std::vector<int> &neighbors) {
  // Calculate the pressure force for the specific position
  glm::vec3 pressureForce = glm::vec3(0);
  const float mass = 1;

  for (auto &i : neighbors) {
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
