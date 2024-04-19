#pragma once

#include "../drawable.h"
#include <GL/glew.h>
#include <glm/vec3.hpp>

class Circle : public Drawable {
public:
  Circle();
  Circle(float r, float sides, glm::vec3 color);
  ~Circle();

  void create() override;

private:
  float radius;
  float sides;
  glm::vec3 color;
};
