#pragma once

#include <glm/vec3.hpp>

#include <vector>

/**
 * Class representing a crowd simulation instance
 */
class FlowFinity {
public:
  FlowFinity();
  ~FlowFinity();

  void performTimeStep(float dt);

  static float SmoothingKernel(float r, float dst);
  static float calculateDensity(glm::vec3 pos,
                                const std::vector<glm::vec3> &positions,
                                float smoothingRadius);

private:
};