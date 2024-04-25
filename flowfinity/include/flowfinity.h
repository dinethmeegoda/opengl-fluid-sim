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

  // Static Helper Functions
  static float smoothingKernel(float r, float dst);
  static float smoothingKernelDerivative(float r, float dst);

  // Instance Functions for calculating simulation properties
  float calculateDensity(int posIndex, int i, float smoothingRadius);
  glm::vec3 CalulatePressureForce(int posIndex, int i, float smoothingRadius);

  // Setters
  void setPositions(std::vector<glm::vec3> *positions);
  void setDensities(std::vector<float> *densities);
  void setTargetDensity(float targetDensity);
  void setPressureMultiplier(float pressureMultiplier);

private:
  std::vector<glm::vec3> *m_positions;
  std::vector<float> *m_densities;
  float m_targetDensity;
  float m_pressureMultiplier;
};