#include "circle.h"
#include "glm/ext/matrix_float4x4.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <vector>

Circle::Circle() : Drawable(), radius(1), sides(40), color(0, 255, 0) {}

Circle::Circle(float r, float s, glm::vec3 col)
    : Drawable(), radius(r), sides(s), color(col) {}

Circle::~Circle() {}

void Circle::create() {
  std::vector<glm::vec4> pos{};

  // std::vector<glm::vec4> nor{};

  std::vector<glm::vec4> col{};

  std::vector<GLuint> idx{};

  for (int vert = 0; vert < sides; vert++) {
    glm::mat4 transform =
        glm::translate(glm::mat4(1.f), glm::vec3(0, radius, 0));
    transform = glm::rotate(transform, glm::radians(360.f / sides * vert),
                            glm::vec3(0, 0, 1));
    pos.push_back(transform * glm::vec4(0, 0, 0, 1));
    // nor.push_back(glm::vec4(0, 0, 1, 0));
    col.push_back(glm::vec4(color / 255.f, 1));
  }
  for (int i = 0; i < sides - 1; i++) {
    idx.push_back(0);
    idx.push_back(i + 1);
    idx.push_back(i + 2);
  }
  m_count = idx.size();

  m_attributes.idx.generate();
  m_attributes.idx.bind();
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(GLuint), idx.data(),
               GL_STATIC_DRAW);

  m_attributes.pos.generate();
  m_attributes.pos.bind();
  glBufferData(GL_ARRAY_BUFFER, pos.size() * sizeof(glm::vec4), pos.data(),
               GL_STATIC_DRAW);

  m_attributes.col.generate();
  m_attributes.col.bind();
  glBufferData(GL_ARRAY_BUFFER, col.size() * sizeof(glm::vec4), col.data(),
               GL_STATIC_DRAW);

  // m_attributes.nor.generate();
  // m_attributes.nor.bind();
  // glBufferData(GL_ARRAY_BUFFER, nor.size() * sizeof(glm::vec4), nor.data(),
  //              GL_STATIC_DRAW);
}