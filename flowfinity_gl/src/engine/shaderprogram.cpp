#include "shaderprogram.h"
#include <glm/gtc/type_ptr.hpp>

#include "glutil.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

ShaderProgram::Handles::Handles()
    : attr_pos(-1), attr_col(-1),
      // attr_nor(-1),
      unif_model(-1), unif_modelInvTr(-1), unif_viewProj(-1), unif_camPos(-1),
      unif_maxVelocity(-1), unif_numInstances(-1), unif_deltaTime(-1),
      unif_time(-1), unif_colors(-1) {}

ShaderProgram::ShaderProgram()
    : m_vertShader(), m_fragShader(), m_prog(), m_ssboPositions(),
      m_ssboVelocities(), m_handles() {}

void ShaderProgram::create(const char *vertFile, const char *fragFile) {
  // Load and compile the vertex and fragment shaders
  m_vertShader = glCreateShader(GL_VERTEX_SHADER);
  m_fragShader = glCreateShader(GL_FRAGMENT_SHADER);
  m_prog = glCreateProgram();

  std::string vertSource = textFileRead(vertFile);
  std::string fragSource = textFileRead(fragFile);
  const char *vertSourceC = vertSource.c_str();
  const char *fragSourceC = fragSource.c_str();

  glShaderSource(m_vertShader, 1, &vertSourceC, 0);
  glShaderSource(m_fragShader, 1, &fragSourceC, 0);

  glCompileShader(m_vertShader);
  glCompileShader(m_fragShader);

  GLUtil::printShaderCompileInfoLog(m_vertShader);
  GLUtil::printShaderCompileInfoLog(m_fragShader);

  // Link the vertex and fragment shader into a shader program
  glAttachShader(m_prog, m_vertShader);
  glAttachShader(m_prog, m_fragShader);
  glLinkProgram(m_prog);

  GLUtil::printLinkInfoLog(m_prog);

  // Get the locations of the attributes in the shader program
  m_handles.attr_pos = glGetAttribLocation(m_prog, "vs_Pos");
  m_handles.attr_col = glGetAttribLocation(m_prog, "vs_Col");
  // m_handles.attr_nor = glGetAttribLocation(m_prog, "vs_Nor");

  // Gets uniform locations in shader program
  m_handles.unif_model = glGetUniformLocation(m_prog, "u_Model");
  m_handles.unif_modelInvTr = glGetUniformLocation(m_prog, "u_ModelInvTr");
  m_handles.unif_viewProj = glGetUniformLocation(m_prog, "u_ViewProj");
  m_handles.unif_camPos = glGetUniformLocation(m_prog, "u_CamPos");
  m_handles.unif_maxVelocity = glGetUniformLocation(m_prog, "u_MaxVelocity");
  m_handles.unif_numInstances = glGetUniformLocation(m_prog, "u_NumInstances");
  m_handles.unif_time = glGetUniformLocation(m_prog, "u_Time");
  m_handles.unif_deltaTime = glGetUniformLocation(m_prog, "u_DeltaTime");
  m_handles.unif_colors = glGetUniformLocation(m_prog, "u_Colors");
}

void ShaderProgram::useMe() { glUseProgram(m_prog); }

void ShaderProgram::draw(Drawable &drawable) {
  GLUtil::printGLErrorLog();
  if (drawable.elemCount() < 0) {
    throw std::invalid_argument(
        "Attempting to draw a Drawable that has not initialized its count "
        "variable! Remember to set it to the length of your index array in "
        "create().");
  }
  useMe();

  bindDrawable(drawable);

  // Bind the index buffer and then draw shapes from it.
  // This invokes the shader program, which accesses the vertex buffers.
  drawable.m_attributes.idx.bind();
  glDrawElements(drawable.drawMode(), drawable.elemCount(), GL_UNSIGNED_INT, 0);

  unbindDrawable();

  GLUtil::printGLErrorLog();
}

void ShaderProgram::drawInstanced(Drawable &drawable, int numInstances,
                                  const std::vector<glm::vec3> &offsets,
                                  const std::vector<glm::vec3> &velocities) {
  GLUtil::printGLErrorLog();
  if (drawable.elemCount() < 0) {
    throw std::invalid_argument(
        "Attempting to draw a Drawable that has not initialized its count "
        "variable! Remember to set it to the length of your index array in "
        "create().");
  }
  useMe();

  // Update the SSBO for positions with the offsets for this frame
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssboPositions);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                  sizeof(glm::vec3) * offsets.size(), offsets.data());
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

  // Do the same for velocities
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssboVelocities);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                  sizeof(glm::vec3) * offsets.size(), velocities.data());
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

  bindDrawable(drawable);

  // Bind the index buffer and then draw shapes from it.
  // This invokes the shader program, which accesses the vertex buffers.
  drawable.m_attributes.idx.bind();
  glDrawElementsInstanced(drawable.drawMode(), drawable.elemCount(),
                          GL_UNSIGNED_INT, 0, numInstances);

  unbindDrawable();

  GLUtil::printGLErrorLog();
}

void ShaderProgram::setModelMatrix(const glm::mat4 &model) {
  useMe();
  if (m_handles.unif_model != -1) {
    glUniformMatrix4fv(m_handles.unif_model, 1, GL_FALSE, &model[0][0]);
  }
  if (m_handles.unif_modelInvTr != -1) {
    glm::mat3 modelInvTr = glm::inverse(glm::transpose(glm::mat3(model)));
    glUniformMatrix3fv(m_handles.unif_modelInvTr, 1, GL_FALSE,
                       &modelInvTr[0][0]);
  }
}

void ShaderProgram::setViewProjMatrix(const glm::mat4 &viewProj) {
  useMe();
  if (m_handles.unif_viewProj != -1) {
    glUniformMatrix4fv(m_handles.unif_viewProj, 1, GL_FALSE, &viewProj[0][0]);
  }
}

void ShaderProgram::setCamPos(const glm::vec3 &cp) {
  useMe();
  if (m_handles.unif_camPos != -1) {
    glUniform3fv(m_handles.unif_camPos, 1, &cp[0]);
  }
}

void ShaderProgram::setMaxVelocity(float maxVelocity) {
  useMe();
  if (m_handles.unif_maxVelocity != -1) {
    glUniform1f(m_handles.unif_maxVelocity, maxVelocity);
  }
}

void ShaderProgram::setNumInstances(int numInstances) {
  useMe();
  if (m_handles.unif_numInstances != -1) {
    glUniform1i(m_handles.unif_numInstances, numInstances);
  }

  // Create the SSBO for positions
  glGenBuffers(1, &m_ssboPositions);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssboPositions);
  glBufferData(GL_SHADER_STORAGE_BUFFER, numInstances * sizeof(glm::vec3),
               nullptr, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ssboPositions);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

  // Create the SSBO for velocities
  glGenBuffers(1, &m_ssboVelocities);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssboVelocities);
  glBufferData(GL_SHADER_STORAGE_BUFFER, numInstances * sizeof(glm::vec3),
               nullptr, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_ssboVelocities);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ShaderProgram::setTime(int time) {
  useMe();
  if (m_handles.unif_time != -1) {
    glUniform1i(m_handles.unif_time, time);
  }
}

void ShaderProgram::setDeltaTime(float deltaTime) {
  useMe();
  if (m_handles.unif_deltaTime != -1) {
    glUniform1f(m_handles.unif_deltaTime, deltaTime);
  }
}

void ShaderProgram::setColors(const std::vector<glm::vec3> &colors) {
  useMe();
  if (m_handles.unif_colors != -1) {
    glUniform3fv(m_handles.unif_colors, colors.size(), &colors[0][0]);
  }
}

void ShaderProgram::bindDrawable(Drawable &drawable) {
  // Each of the following blocks checks that:
  //   * This shader has this attribute, and
  //   * This Drawable has a vertex buffer for this attribute.
  // If so, it binds the appropriate buffers to each attribute.

  // By calling pos.bind(), we call glBindBuffer on the Drawable's VBO for
  // vertex position, meaning that glVertexAttribPointer associates vs_Pos
  // (referred to by attr_pos) with that VBO

  if (m_handles.attr_pos != -1 && drawable.m_attributes.pos.tryBind()) {
    glEnableVertexAttribArray(m_handles.attr_pos);
    glVertexAttribPointer(m_handles.attr_pos, 4, GL_FLOAT, false, 0, nullptr);
  }
  if (m_handles.attr_col != -1 && drawable.m_attributes.col.tryBind()) {
    glEnableVertexAttribArray(m_handles.attr_col);
    glVertexAttribPointer(m_handles.attr_col, 4, GL_FLOAT, false, 0, nullptr);
  }
  // if (m_handles.attr_nor != -1 && drawable.m_attributes.nor.tryBind()) {
  //   glEnableVertexAttribArray(m_handles.attr_nor);
  //   glVertexAttribPointer(m_handles.attr_nor, 4, GL_FLOAT, false, 0,
  //   nullptr);
  // }
}

void ShaderProgram::unbindDrawable() {
  if (m_handles.attr_pos != -1)
    glDisableVertexAttribArray(m_handles.attr_pos);
  if (m_handles.attr_col != -1)
    glDisableVertexAttribArray(m_handles.attr_col);
  // if (m_handles.attr_nor != -1)
  //   glDisableVertexAttribArray(m_handles.attr_nor);
}

std::string ShaderProgram::textFileRead(const char *filename) {
  fs::path path = fs::current_path() / "resources/glsl" / filename;

  std::ifstream file(path);
  if (file.fail()) {
    std::cerr << "Failed to open file: " << path << std::endl;
    return nullptr;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}