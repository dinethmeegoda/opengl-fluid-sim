#include "flowfinity.h"

#include <cmath>
#include <glm/ext/scalar_constants.hpp>
#include <glm/geometric.hpp>

FlowFinity::FlowFinity() {}

FlowFinity::~FlowFinity() {}

#define M_PI 3.14159265358979323846 /* pi */

float FlowFinity::SmoothingKernel(float r, float dst) {
  float volume = M_PI * std::pow(r, 8) / 4;
  float value = fmax(0, r * r - dst * dst);
  return value * value * value / volume;
}

float FlowFinity::calculateDensity(glm::vec3 pos,
                                   const std::vector<glm::vec3> &positions,
                                   float smoothingRadius) {
  float density = 0;
  const float mass = 1;

  for (auto &p : positions) {
    float dst = glm::distance(pos, p);
    float influence = SmoothingKernel(smoothingRadius, dst);
    density += mass * influence;
  }
  return density;
}