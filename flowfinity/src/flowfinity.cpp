#include "flowfinity.h"

#include <cmath>
#include <glm/ext/scalar_constants.hpp>
#include <glm/geometric.hpp>

FlowFinity::FlowFinity() {}

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

float FlowFinity::calculateDensity(glm::vec3 pos,
                                   const std::vector<glm::vec3> &positions,
                                   float smoothingRadius) {
  float density = 0;
  const float mass = 1;

  for (auto &p : positions) {
    float dst = glm::distance(pos, p);
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

glm::vec3 FlowFinity::CalulatePressureForce(
    int posIndex, const std::vector<glm::vec3> &positions,
    const std::vector<float> &densities, float smoothingRadius,
    float targetDensity, float pressureMultiplier) {
  // Calculate the pressure force for the specific position
  glm::vec3 pressureForce = glm::vec3(0);
  const float mass = 1;

  for (int i = 0; i < positions.size(); i++) {
    if (i == posIndex) {
      continue;
    }
    float dst = glm::distance(positions[posIndex], positions[i]);
    glm::vec3 dir =
        dst == 0 ? getRandomDir() : (positions[posIndex] - positions[i]) / dst;
    float slope = smoothingKernelDerivative(smoothingRadius, dst);
    float density = densities[i];
    float pressureA = pressureMultiplier * (density - targetDensity);
    float pressureB =
        pressureMultiplier * (densities[posIndex] - targetDensity);

    float sharedPressure = pressureA + pressureB / 2.f;

    pressureForce += -sharedPressure * dir * slope * mass / density;
  }
  return pressureForce;
}