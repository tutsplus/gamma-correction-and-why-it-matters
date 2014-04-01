#include <vector>
#include <map>
#include <GL/glew.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "loader.h"

#include <string.h> // for memcmp

#include <stdio.h>
#include <string>
#include <cstring>

#include <glm/glm.hpp>

// Very, VERY simple OBJ loader.
// Here is a short list of features a real function would provide : 
// - Binary files. Reading a model should be just a few memcpy's away, not parsing a file at runtime. In short : OBJ is not very great.
// - Animations & bones (includes bones weights)
// - Multiple UVs
// - All attributes should be optional, not "forced"
// - More stable. Change a line in the OBJ file and it crashes.
// - More secure. Change another line and you can inject code.
// - Loading from memory, stream, etc

bool loadOBJ(
  const char * path,
  std::vector<glm::vec3> & out_vertices,
  std::vector<glm::vec2> & out_uvs,
  std::vector<glm::vec3> & out_normals
  ){
  printf("Loading OBJ file %s...\n", path);

  std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
  std::vector<glm::vec3> temp_vertices;
  std::vector<glm::vec2> temp_uvs;
  std::vector<glm::vec3> temp_normals;


  FILE * file = fopen(path, "r");
  if (file == NULL){
    printf("Impossible to open the file !\n");
    return false;
  }

  while (1){

    char lineHeader[128];
    // read the first word of the line
    int res = fscanf(file, "%s", lineHeader);
    if (res == EOF)
      break; // EOF = End Of File. Quit the loop.

    // else : parse lineHeader

    if (strcmp(lineHeader, "v") == 0){
      glm::vec3 vertex;
      fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
      temp_vertices.push_back(vertex);
    }
    else if (strcmp(lineHeader, "vt") == 0){
      glm::vec2 uv;
      fscanf(file, "%f %f\n", &uv.x, &uv.y);
      uv.y = -uv.y; // Invert V coordinate since we will only use DDS texture, which are inverted. Remove if you want to use TGA or BMP loaders.
      temp_uvs.push_back(uv);
    }
    else if (strcmp(lineHeader, "vn") == 0){
      glm::vec3 normal;
      fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
      temp_normals.push_back(normal);
    }
    else if (strcmp(lineHeader, "f") == 0){
      std::string vertex1, vertex2, vertex3;
      unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
      int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
      if (matches != 9){
        printf("File can't be read by our simple parser :-( Try exporting with other options\n");
        return false;
      }
      vertexIndices.push_back(vertexIndex[0]);
      vertexIndices.push_back(vertexIndex[1]);
      vertexIndices.push_back(vertexIndex[2]);
      uvIndices.push_back(uvIndex[0]);
      uvIndices.push_back(uvIndex[1]);
      uvIndices.push_back(uvIndex[2]);
      normalIndices.push_back(normalIndex[0]);
      normalIndices.push_back(normalIndex[1]);
      normalIndices.push_back(normalIndex[2]);
    }
    else{
      // Probably a comment, eat up the rest of the line
      char stupidBuffer[1000];
      fgets(stupidBuffer, 1000, file);
    }

  }

  // For each vertex of each triangle
  for (unsigned int i = 0; i<vertexIndices.size(); i++){

    // Get the indices of its attributes
    unsigned int vertexIndex = vertexIndices[i];
    unsigned int uvIndex = uvIndices[i];
    unsigned int normalIndex = normalIndices[i];

    // Get the attributes thanks to the index
    glm::vec3 vertex = temp_vertices[vertexIndex - 1];
    glm::vec2 uv = temp_uvs[uvIndex - 1];
    glm::vec3 normal = temp_normals[normalIndex - 1];

    // Put the attributes in buffers
    out_vertices.push_back(vertex);
    out_uvs.push_back(uv);
    out_normals.push_back(normal);

  }

  return true;
}

// Returns true iif v1 can be considered equal to v2
bool is_near(float v1, float v2){
  return fabs(v1 - v2) < 0.01f;
}

// Searches through all already-exported vertices
// for a similar one.
// Similar = same position + same UVs + same normal
bool getSimilarVertexIndex(
  glm::vec3 & in_vertex,
  glm::vec2 & in_uv,
  glm::vec3 & in_normal,
  std::vector<glm::vec3> & out_vertices,
  std::vector<glm::vec2> & out_uvs,
  std::vector<glm::vec3> & out_normals,
  unsigned short & result
  ){
  // Lame linear search
  for (unsigned int i = 0; i<out_vertices.size(); i++){
    if (
      is_near(in_vertex.x, out_vertices[i].x) &&
      is_near(in_vertex.y, out_vertices[i].y) &&
      is_near(in_vertex.z, out_vertices[i].z) &&
      is_near(in_uv.x, out_uvs[i].x) &&
      is_near(in_uv.y, out_uvs[i].y) &&
      is_near(in_normal.x, out_normals[i].x) &&
      is_near(in_normal.y, out_normals[i].y) &&
      is_near(in_normal.z, out_normals[i].z)
      ){
      result = i;
      return true;
    }
  }
  // No other vertex could be used instead.
  // Looks like we'll have to add it to the VBO.
  return false;
}

void indexVBO_slow(
  std::vector<glm::vec3> & in_vertices,
  std::vector<glm::vec2> & in_uvs,
  std::vector<glm::vec3> & in_normals,

  std::vector<unsigned short> & out_indices,
  std::vector<glm::vec3> & out_vertices,
  std::vector<glm::vec2> & out_uvs,
  std::vector<glm::vec3> & out_normals
  ){
  // For each input vertex
  for (unsigned int i = 0; i<in_vertices.size(); i++){

    // Try to find a similar vertex in out_XXXX
    unsigned short index;
    bool found = getSimilarVertexIndex(in_vertices[i], in_uvs[i], in_normals[i], out_vertices, out_uvs, out_normals, index);

    if (found){ // A similar vertex is already in the VBO, use it instead !
      out_indices.push_back(index);
    }
    else{ // If not, it needs to be added in the output data.
      out_vertices.push_back(in_vertices[i]);
      out_uvs.push_back(in_uvs[i]);
      out_normals.push_back(in_normals[i]);
      out_indices.push_back((unsigned short)out_vertices.size() - 1);
    }
  }
}

struct PackedVertex{
  glm::vec3 position;
  glm::vec2 uv;
  glm::vec3 normal;
  bool operator<(const PackedVertex that) const{
    return memcmp((void*)this, (void*)&that, sizeof(PackedVertex))>0;
  };
};

bool getSimilarVertexIndex_fast(
  PackedVertex & packed,
  std::map<PackedVertex, unsigned short> & VertexToOutIndex,
  unsigned short & result
  ){
  std::map<PackedVertex, unsigned short>::iterator it = VertexToOutIndex.find(packed);
  if (it == VertexToOutIndex.end()){
    return false;
  }
  else{
    result = it->second;
    return true;
  }
}

void indexVBO(
  std::vector<glm::vec3> & in_vertices,
  std::vector<glm::vec2> & in_uvs,
  std::vector<glm::vec3> & in_normals,

  std::vector<unsigned short> & out_indices,
  std::vector<glm::vec3> & out_vertices,
  std::vector<glm::vec2> & out_uvs,
  std::vector<glm::vec3> & out_normals
  ){
  std::map<PackedVertex, unsigned short> VertexToOutIndex;

  // For each input vertex
  for (unsigned int i = 0; i<in_vertices.size(); i++){

    PackedVertex packed = { in_vertices[i], in_uvs[i], in_normals[i] };


    // Try to find a similar vertex in out_XXXX
    unsigned short index;
    bool found = getSimilarVertexIndex_fast(packed, VertexToOutIndex, index);

    if (found){ // A similar vertex is already in the VBO, use it instead !
      out_indices.push_back(index);
    }
    else{ // If not, it needs to be added in the output data.
      out_vertices.push_back(in_vertices[i]);
      out_uvs.push_back(in_uvs[i]);
      out_normals.push_back(in_normals[i]);
      unsigned short newindex = (unsigned short)out_vertices.size() - 1;
      out_indices.push_back(newindex);
      VertexToOutIndex[packed] = newindex;
    }
  }
}

void indexVBO_TBN(
  std::vector<glm::vec3> & in_vertices,
  std::vector<glm::vec2> & in_uvs,
  std::vector<glm::vec3> & in_normals,
  std::vector<glm::vec3> & in_tangents,
  std::vector<glm::vec3> & in_bitangents,

  std::vector<unsigned short> & out_indices,
  std::vector<glm::vec3> & out_vertices,
  std::vector<glm::vec2> & out_uvs,
  std::vector<glm::vec3> & out_normals,
  std::vector<glm::vec3> & out_tangents,
  std::vector<glm::vec3> & out_bitangents
  ){
  // For each input vertex
  for (unsigned int i = 0; i<in_vertices.size(); i++){

    // Try to find a similar vertex in out_XXXX
    unsigned short index;
    bool found = getSimilarVertexIndex(in_vertices[i], in_uvs[i], in_normals[i], out_vertices, out_uvs, out_normals, index);

    if (found){ // A similar vertex is already in the VBO, use it instead !
      out_indices.push_back(index);

      // Average the tangents and the bitangents
      out_tangents[index] += in_tangents[i];
      out_bitangents[index] += in_bitangents[i];
    }
    else{ // If not, it needs to be added in the output data.
      out_vertices.push_back(in_vertices[i]);
      out_uvs.push_back(in_uvs[i]);
      out_normals.push_back(in_normals[i]);
      out_tangents.push_back(in_tangents[i]);
      out_bitangents.push_back(in_bitangents[i]);
      out_indices.push_back((unsigned short)out_vertices.size() - 1);
    }
  }
}

GLuint loadBMP(const char * imagepath, GLint internalFormat){

  printf("Reading image %s\n", imagepath);

  // Data read from the header of the BMP file
  unsigned char header[54];
  unsigned int dataPos;
  unsigned int imageSize;
  unsigned int width, height;
  // Actual RGB data
  unsigned char * data;

  // Open the file
  FILE * file = fopen(imagepath, "rb");
  if (!file)							    { printf("%s could not be opened.\n", imagepath); return 0; }

  // Read the header, i.e. the 54 first bytes

  // If less than 54 bytes are read, problem
  if (fread(header, 1, 54, file) != 54){
    printf("Not a correct BMP file\n");
    return 0;
  }
  // A BMP files always begins with "BM"
  if (header[0] != 'B' || header[1] != 'M'){
    printf("Not a correct BMP file\n");
    return 0;
  }
  // Make sure this is a 24bpp file
  if (*(int*)&(header[0x1E]) != 0)         { printf("Not a correct BMP file\n");    return 0; }
  if (*(int*)&(header[0x1C]) != 24)         { printf("Not a correct BMP file\n");    return 0; }

  // Read the information about the image
  dataPos = *(int*)&(header[0x0A]);
  imageSize = *(int*)&(header[0x22]);
  width = *(int*)&(header[0x12]);
  height = *(int*)&(header[0x16]);

  // Some BMP files are misformatted, guess missing information
  if (imageSize == 0)    imageSize = width*height * 3; // 3 : one byte for each Red, Green and Blue component
  if (dataPos == 0)      dataPos = 54; // The BMP header is done that way

  // Create a buffer
  data = new unsigned char[imageSize];

  // Read the actual data from the file into the buffer
  fread(data, 1, imageSize, file);

  // Everything is in memory now, the file wan be closed
  fclose(file);

  // Create one OpenGL texture
  GLuint textureID;
  glGenTextures(1, &textureID);

  // "Bind" the newly created texture : all future texture functions will modify this texture
  glBindTexture(GL_TEXTURE_2D, textureID);

  // Give the image to OpenGL
  glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);

  // OpenGL has now copied the data. Free our own version
  delete[] data;

  // Poor filtering, or ...
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 

  // ... nice trilinear filtering.
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glGenerateMipmap(GL_TEXTURE_2D);

  // Return the ID of the texture we just created
  return textureID;
}