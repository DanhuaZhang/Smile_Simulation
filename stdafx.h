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

struct Face {
	float* left;
	float* right;
	float* left_fp;
	float* right_fp;
	int num_left;       //amount of triangles in left half face
	int num_right;      //amount of triangles in right half face
	int num_left_fp;  //amount of feature points in left half face
	int num_right_fp; //amount of feature points in right half face
						//num_left_edge and num_right_edge should be the same
						//i.e. the feature points in the middle are duplicated
};

// Each row of the CSV file is a face at a certain time and stored as a column in m
// Faces are in canonical sapace
void GetCSVFile(Eigen::MatrixXf &matrix, const char* filename, int pnum);
void SplitBVector(Eigen::MatrixXf &m, const int bitmap[], int bitmap_size);

// Return the obj models in model space, which can be directly used for glDrawArrays() with primitive triangle
Face* ParseObj(const char* filename, int &model_numVerts, const int edgeidx[], const int edge_numVerts, const int bitmap[]);
float* ParseObj(const char* filename, int &model_numVerts);

GLuint LoadToVBO(float* model, int model_numVerts);
GLuint LoadToVAO(GLuint shaderprogram);
GLuint LoadToVAO_Point(GLuint shaderprogram);
static char* readShaderSource(const char* shaderFile);
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName);
GLuint AllocateTexture(const char* tex_filename, int i);

// interpolate between columns and return a matrix of column number num_loop
int Interpolation(Eigen::MatrixXf m, Eigen::MatrixXf& rm, float time_interval);
int Interpolation_Split(Eigen::MatrixXf m, Eigen::MatrixXf& rm, float time_interval);