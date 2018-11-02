#pragma once

#include "Config.h"

#include "imgui.h"
#include "imgui_impl_sdl_gl3.h"

#include "glad/glad.h"  //Include order can matter here
#ifndef _WIN32
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#else
#include <SDL.h>
#include <SDL_opengl.h>
#endif
//#include <SDL_image.h>

#define GLM_FORCE_RADIANS
#define MAX_CHARACTER 1024

#include <stdio.h>
#include <stdlib.h>

#if PLATFORM_FLAG == PF_WIN32
#include <tchar.h>
#endif // PLATFORM_FLAG == PF_LINUX

#include "Eigen/Core"
#include "Eigen/Dense"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/rotate_vector.hpp"

#include <iostream>
#include <fstream>
#include <chrono>

extern float unit;
extern glm::vec2 trans;
extern float scale;

void GetCSVFile(Eigen::MatrixXf &matrix, const char* filename, int pnum);
float* ParseObj(const char* filename, int &model_numVerts, const int edgeidx[], glm::vec3** edge, const int face_id, const int edge_numVerts);
float* ParseObj(const char* filename, int &model_numVerts, const int edgeidx[], glm::vec3* edge, const int edge_numVerts);
float* ParseObj(const char* filename, int &model_numVerts);
GLuint LoadToVBO(float* model, int model_numVerts);
GLuint LoadToVAO(GLuint shaderprogram);
GLuint LoadToVAO_Point(GLuint shaderprogram);
static char* readShaderSource(const char* shaderFile);
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName);
GLuint AllocateTexture(const char* tex_filename, int i);
// interpolate between columns and return a matrix of column number num_loop
Eigen::MatrixXf Interpolation(Eigen::MatrixXf m, Eigen::MatrixXf& rm, float time_interval);