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

  static float smoothingKernel(float r, float dst);
  static float calculateDensity(glm::vec3 pos,
                                const std::vector<glm::vec3> &positions,
                                float smoothingRadius);
  static float smoothingKernelDerivative(float r, float dst);

  static glm::vec3
  CalulatePressureForce(int posIndex, const std::vector<glm::vec3> &positions,
                        const std::vector<float> &densities,
                        float smoothingRadius, float targetDensity,
                        float pressureMultiplier);

private:
};