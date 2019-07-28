#pragma once
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "Error.h"

namespace rhythmus
{

/* @brief Rendering vertex info. */
struct VertexInfo {
  float x, y, z;
  float sx, sy;
  float r, g, b, a;
};

/* @brief Projection info for each object. */
struct ProjectionInfo
{
  float rotx, roty, rotz;
  float tx, ty;   // translation center, which means center of rotation.
};

/* @brief Total info required for rendering a object. */
struct DrawInfo
{
  VertexInfo vi[4];
  ProjectionInfo pi;
};

/**
 * @brief Shader info.
 */
struct ShaderInfo {
  const char* vertex_shader;
  const char* frag_shader;
  GLuint prog_id;
  const char* VAO_params[16];   // TODO?

  /* Round-robin buffering */
  GLuint VAO_id, buffer_id;
};

/**
 * @brief
 * Contains graphic context of game.
 * Singleton class.
 */

class Graphic
{
public:
  void Initialize();
  void LoopRendering();
  void ExitRendering();
  void Cleanup();
  void SetProjOrtho();
  void SetProjPerspective();

  static Graphic& getInstance();

  static void RenderQuad(const VertexInfo* vi);
  static void RenderQuad(const DrawInfo& di);
private:
  Graphic();
  ~Graphic();
  bool CompileShader();
  bool CompileShaderInfo(ShaderInfo& shader);

  GLFWwindow* window_;
  ShaderInfo quad_shader_;
  int current_proj_mode_;
  glm::mat4 projection_;
};

}