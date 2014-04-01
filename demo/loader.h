/* Taken from opengl-tutorial.org for demo purposes */
#ifndef OBJLOADER_H
#define OBJLOADER_H
#include <vector>
#include <glm/glm.hpp>

bool loadOBJ(
  const char * path,
  std::vector<glm::vec3> & out_vertices,
  std::vector<glm::vec2> & out_uvs,
  std::vector<glm::vec3> & out_normals
  );

void indexVBO(
  std::vector<glm::vec3> & in_vertices,
  std::vector<glm::vec2> & in_uvs,
  std::vector<glm::vec3> & in_normals,

  std::vector<unsigned short> & out_indices,
  std::vector<glm::vec3> & out_vertices,
  std::vector<glm::vec2> & out_uvs,
  std::vector<glm::vec3> & out_normals
  );

GLuint loadBMP(const char * imagepath, GLint internalFormat);

#endif