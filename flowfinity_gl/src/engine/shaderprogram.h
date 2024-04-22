#pragma once

#include "drawable.h"

#include <GL/glew.h>
#include <glm/mat4x4.hpp>
#include <string>
#include <vector>

class ShaderProgram {
  struct Handles {
    Handles();

    // in vec4 => vertex position
    int attr_pos;
    // in vec4 => vertex color
    int attr_col;
    // in vec4 => vertex normal
    // int attr_nor;
    // uniform mat4 => model matrix
    int unif_model;
    // uniform mat4 => inverse transpose model matrix
    int unif_modelInvTr;
    // uniform mat4 => combined projection and view matrices
    int unif_viewProj;
    // uniform vec3 => camera position
    int unif_camPos;
    // uniform vec3[] => instanced rendering positions
    int unif_offsets;
    // uniform int => number of instances
    int unif_numInstances;
    // uniform int -> time
    int unif_time;
    // uniform float -> deltaTime
    int unif_deltaTime;
  };

public:
  // Handles to attributes and uniforms on GPU
  Handles m_handles;
  // The vertex shader stored in this shader program
  GLuint m_vertShader;
  // The fragment shader stored in this shader program
  GLuint m_fragShader;
  // The linked shader program stored in this class
  GLuint m_prog;

  ShaderProgram();
  void create(const char *vertFile, const char *fragFile);
  void useMe();

  // Draw the given object to our screen using this ShaderProgram's shaders
  void draw(Drawable &drawable);

  // Draw the given object instanced
  void drawInstanced(Drawable &drawable, int numInstances);

  // Pass model matrix to this shader on the GPU
  void setModelMatrix(const glm::mat4 &model);
  // Pass Projection * View matrix to this shader on the GPU
  void setViewProjMatrix(const glm::mat4 &vp);
  // Pass camera position to this shader on the GPU
  void setCamPos(const glm::vec3 &cp);
  // Pass offsets array to this shader on the GPU
  void setOffsets(const std::vector<glm::vec3> &offsets, int numInstances);
  // Pass number of instances to this shader on the GPU
  void setNumInstances(int numInstances);
  // Pass time to this shader on the GPU
  void setTime(int time);
  // Pass deltaTime to this shader on the GPU
  void setDeltaTime(float deltaTime);

private:
  // Utility functions used by draw()
  void bindDrawable(Drawable &drawable);
  void unbindDrawable();

  // Utility function used in create()
  std::string textFileRead(const char *);
};
