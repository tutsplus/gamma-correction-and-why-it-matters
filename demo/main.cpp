/* 
 * This code, along with the shader, are heavily based on tutorials #8 and #9 from www.opengl-tutorial.org, used under
 * the terms of the WTFPL 
 *
 */
#include <GL/glew.h>
#if defined(_WIN32)
#include <GL/wglew.h>
#elif !defined(__APPLE__) || defined(GLEW_APPLE_GLX)
#include <GL/glxew.h>
#endif
#include <GLFW/glfw3.h>
#include "loader.h"
#include <cstdio>
#include <cstdlib>
#include <malloc.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#define TRUE 1
#define FALSE 0

#define SCREENWIDTH 1280
#define SCREENHEIGHT 720

GLFWwindow *window;

GLuint modelVAO, modelPositionBuffer, modelNormalBuffer, modelUVBuffer, modelIndexBuffer,
       vertexShader, fragmentShader, mainProgram,
       texture_nc, texture_c, spheremap_c, spheremap_nc,
       modelViewProjUniform, modelUniform, viewUniform, lightPositionUniform, texSampler, modelViewUniform, normalMatrixUniform, sphereMapSampler, light2PositionUniform;

double lightAngle, light2Angle;

#define ROTATION_SPEED                   -12
#define LIGHT1_ROTATION_SPEED            (0.5)
#define LIGHT1_ROTATION_RADIUS           3.6
#define LIGHT2_ROTATION_SPEED            (-1.5)
#define LIGHT2_ROTATION_RADIUS           4.6

int correctTextures, correctFramebuffer;
size_t indexCount;

glm::mat4x4 model, view, proj, modelView, modelViewProj;
glm::mat3x3 normalMatrix;
glm::vec3 light, light2;

/* Prototypes */
void fatal(const char*);
char* file_to_buffer(const char*);
void init_all();
void setup_stage();
void update(double);
void render();
void load_model(const char*);
void main_loop();
void key_callback(GLFWwindow*, int, int, int, int);

/*
 * This function will print an error to the standard error output
 * and then quit. We call it if something unrecoverable happens
 */
void fatal(const char* err)
{
  fprintf(stderr, "FATAL: %s\n", err);
  exit(-1);
}

/*
 * This small function will open a file, allocate a buffer of appropriate
 * size and load the contents of the file into it.
 */
char* file_to_buffer(const char* fname)
{
  FILE * f = fopen(fname, "rb");
  if (!f) return NULL;

  fseek(f, 0, SEEK_END);
  size_t sz = ftell(f);
  fseek(f, 0, SEEK_SET);

  char* buf = (char*)malloc(sz + 1);
  fread(buf, 1, sz, f);
  buf[sz] = '\0';

  return buf;
}

/* 
 * This function is called when a key is pressed. We'll use it to change
 * the global flags for gamma correction.
 */
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  /* See what key was pressed and flip the global flags accordingly */
  int change = 0;
  if (key == GLFW_KEY_1 && action == GLFW_RELEASE)
    correctTextures = !correctTextures, change = 1;
  else if (key == GLFW_KEY_2 && action == GLFW_RELEASE)
    correctFramebuffer = !correctFramebuffer, change = 1;
  else if (key == GLFW_KEY_3 && action == GLFW_RELEASE) {
    correctFramebuffer = !correctFramebuffer;
    correctTextures = !correctTextures;
    change = 1;
  }

  /* Tell OpenGL to fix the framebuffer setting right away, right textures will
   * be bound by render() */
  if (correctFramebuffer) {
    glEnable(GL_FRAMEBUFFER_SRGB);
  }
  else {
    glDisable(GL_FRAMEBUFFER_SRGB);
  }

  /* Show to the user what the current settings are */
  if (change) {
    fprintf(stderr, "Correcting framebuffer: %s\n", correctFramebuffer ? "yes" : "no");
    fprintf(stderr, "Correcting textures: %s\n", correctTextures ? "yes" : "no");
    fputc('\n', stderr);
  }
}
/*
 * This function initializes the context and other stuff we need
 * for the demo.
 */
void init_all()
{
  /* First, try to initialize GLFW */
  if (glfwInit() == 0)
    fatal("could not initialize GLFW");

  /* Now, let's give GLFW hints about the window we need */
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); /* We want OpenGL version at least 3.3 */
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); /* We will also request a bleeding-edge core profile*/

  /* IMPORTANT: Tell GLFW to give us a framebuffer which can do sRGB */
  glfwWindowHint(GLFW_SRGB_CAPABLE, TRUE);

  /* Time to create our window*/
  window = glfwCreateWindow(SCREENWIDTH, SCREENHEIGHT, "Gamma correction demo", NULL, NULL);
  if (window == NULL) {
    glfwTerminate(); /* Deinitialize GLFW */
    fatal("could not create window");
  }

  /* Now do some housekeeping and initialize GLEW */
  glfwMakeContextCurrent(window);
  glewExperimental = TRUE; /* This is needed if we want to load core profile, as requested from GLFW up there */
  if (glewInit() != GLEW_OK) /* Try to init GLEW */
    fatal("could not initialize GLEW");

  /* Set the key callback so we can respond to key presses */
  glfwSetKeyCallback(window, &key_callback);

  /* Finally, set our stage up. */
  setup_stage();

  /* Done now! */
}

/* This functions updates the rotations */
void update(double dt)
{
  /* Rotate the model a little and recompute other matrixes */
  model = glm::rotate<float>(model, ROTATION_SPEED * dt, glm::vec3(0, 1, 0));
  modelView = view * model;
  modelViewProj = proj * modelView;
  normalMatrix = glm::mat3x3(glm::inverse(glm::transpose(view)));

  /* Rotate the two lights */
  lightAngle -= LIGHT1_ROTATION_SPEED * dt;
  light2Angle -= LIGHT2_ROTATION_SPEED * dt;
  if (lightAngle < 0) lightAngle = 2 * glm::pi<double>();
  if (light2Angle < 0) light2Angle = 2 * glm::pi<double>();

  /* Compute the light positions */
  light = glm::vec3(
    -sin(lightAngle) * LIGHT1_ROTATION_RADIUS,
    2.1,
    cos(lightAngle) * LIGHT1_ROTATION_RADIUS
  );

  light2 = glm::vec3(
    -sin(light2Angle) * LIGHT2_ROTATION_RADIUS,
    0.1,
    cos(light2Angle) * LIGHT2_ROTATION_RADIUS
  );

}

/* This function renders to the screen */
void render()
{
  /* Clear the screen */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  /* Bind the vertex array */
  glBindVertexArray(modelVAO);

  /* Bind our shader program */
  glUseProgram(mainProgram);

  /* Setup all of our rendering context */
  glUniformMatrix4fv(viewUniform, 1, GL_FALSE, &view[0][0]); /* Set the view matrix uniform */
  glUniformMatrix4fv(modelViewUniform, 1, GL_FALSE, &modelView[0][0]); /* Set the model-view matrix uniform */
  glUniformMatrix4fv(modelViewProjUniform, 1, GL_FALSE, &modelViewProj[0][0]); /* Set the model-view-projection matrix uniform */
  glUniformMatrix4fv(modelUniform, 1, GL_FALSE, &model[0][0]); /* Set the modelmatrix uniform */
  glUniformMatrix3fv(normalMatrixUniform, 1, GL_FALSE, &normalMatrix[0][0]); /* Set the normalmatrix uniform */

  glUniform3f(lightPositionUniform, light.x, light.y, light.z); /* Set the light 1 position uniform */
  glUniform3f(light2PositionUniform, light2.x, light2.y, light2.z); /* Set the light 2 position uniform */

  glActiveTexture(GL_TEXTURE0); /* Work with texture unit 0 */

  /* Bind the texture, corrected or uncorrected based on the flag */
  if (correctTextures)
    glBindTexture(GL_TEXTURE_2D, texture_c);
  else glBindTexture(GL_TEXTURE_2D, texture_nc);

  glUniform1i(texSampler, 0); /* Set the main sampler to use texture unit 0 */

  glActiveTexture(GL_TEXTURE1); /* Bring about texture unit 1 */
  if (correctTextures)
    glBindTexture(GL_TEXTURE_2D, spheremap_c);
  else glBindTexture(GL_TEXTURE_2D, spheremap_nc);

  glUniform1i(sphereMapSampler, 1); /* Set the spheremap sampler to use texture unit 1 */

  /* Issue the actual draw command */
  glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, (void*)0);
}

/* 
 * This function will load the model, create textures, compile our
 * shaders and prepare everythinng else we need
 */
void setup_stage()
{
  glEnable(GL_DEPTH_TEST); /* Enable z-testing */
  glDepthFunc(GL_LESS); /* We use the standard less-than z-test function */
  glEnable(GL_CULL_FACE); /* Enable face culling, by default backfaces are culled which is consistent with how we lay our model down */

  /* Ask OpenGL to create a VAO for us, which will hold all references to all other bound buffers, and bind it too */
  glGenVertexArrays(1, &modelVAO);
  glBindVertexArray(modelVAO);

  /* Now let's create real buffer objects to put our model data inside */
  glGenBuffers(1, &modelPositionBuffer);
  glGenBuffers(1, &modelNormalBuffer);
  glGenBuffers(1, &modelUVBuffer);
  glGenBuffers(1, &modelIndexBuffer);

  /* Buffers to hold our final model data */
  std::vector<glm::vec3> fpositions, fnormals;
  std::vector<glm::vec2> fuvs;
  std::vector<char16_t> findices;
  {
    /* Ask the loader component to first load data we need... */
    std::vector<glm::vec3> positions, normals;
    std::vector<glm::vec2> uvs;
    if (!loadOBJ("../../assets/model.obj", positions, uvs, normals))
      fatal("could not load model");

    /* Now ask the VBO indexer to compile the final, indexed data */
    indexVBO(positions, uvs, normals, findices, fpositions, fuvs, fnormals);
    indexCount = findices.size();
  } /* Here the auxiliary buffers go out of scope, freeing memory */

  /* Bind all of our buffers consecutively and fill them with data */
  glBindBuffer(GL_ARRAY_BUFFER, modelPositionBuffer);
  glBufferData(GL_ARRAY_BUFFER, fpositions.size() * sizeof(glm::vec3), &fpositions[0], GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, modelNormalBuffer);
  glBufferData(GL_ARRAY_BUFFER, fnormals.size() * sizeof(glm::vec3), &fnormals[0], GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, modelUVBuffer);
  glBufferData(GL_ARRAY_BUFFER, fuvs.size() * sizeof(glm::vec2), &fuvs[0], GL_STATIC_DRAW);

  /* Do the same for the index buffer as well */
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelIndexBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, findices.size() * sizeof(char16_t), &findices[0], GL_STATIC_DRAW);

  /* Create shader programs*/
  vertexShader = glCreateShader(GL_VERTEX_SHADER);
  fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

  /* Now, let's load the fragment and vertex programs */
  char *vertsh = file_to_buffer("../../assets/demo.vert"), *fragsh = file_to_buffer("../../assets/demo.frag");

  /* Check if any of these failed */
  if (vertsh == NULL || fragsh == NULL)
    fatal("could not load vertex/fragment shader");

  /* Forward the shader source to OGL */
  glShaderSource(vertexShader, 1, (const GLchar**)&vertsh, NULL);
  glShaderSource(fragmentShader, 1, (const GLchar**)&fragsh, NULL);

  /* Compile the shaders */
  glCompileShader(vertexShader);
  glCompileShader(fragmentShader);

  /* Ask to check if the shaders compiled correctly */
  GLint statusV = 0, statusF = 0;
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &statusV);
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &statusF);
  int InfoLogLength;
  glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &InfoLogLength);
  if (InfoLogLength > 0){
    std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
    glGetShaderInfoLog(fragmentShader, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
    printf("%s\n", &VertexShaderErrorMessage[0]);
  }

  /* If not, fail */
  if (!statusV || !statusF)
    fatal("could not compile vertex/fragment shader");

  /* So it compiled---so far, so good! Let's create a program, which is actually
   * what we use to render the model */
  mainProgram = glCreateProgram();

  /* Attach our shaders and have the program linked */
  glAttachShader(mainProgram, vertexShader);
  glAttachShader(mainProgram, fragmentShader);
  glLinkProgram(mainProgram);

  /* Check if linking finished successfully */
  GLint statusL = 0;
  glGetProgramiv(mainProgram, GL_LINK_STATUS, &statusL);

  /* If not, fail */
  if (!statusL)
    fatal("could not link vertex and fragment shaders");

  /* Dispose of the shaders because we don't need them as they're in the linked program */
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  /* Load the textures in two variants: with an RGB8 internal format and an SRGB8 one */
  texture_nc = loadBMP("../../assets/texture.bmp", GL_RGB8); /* Ordinary diffuse texture */
  texture_c = loadBMP("../../assets/texture.bmp", GL_SRGB8);
  spheremap_nc = loadBMP("../../assets/spheremap.bmp", GL_RGB8); /* Sphere map texture, using for reflectance */
  spheremap_c = loadBMP("../../assets/spheremap.bmp", GL_SRGB8);

  /* Check if the loading failed for some reason */
  if (!texture_c || !texture_nc)
    fatal("could not load textures");

  /* Bind our shader program */
  glUseProgram(mainProgram);

  /* Get locations for our shader uniforms */
  modelViewProjUniform = glGetUniformLocation(mainProgram, "modelViewProj"); /* Projection * view * model matrix */
  modelUniform = glGetUniformLocation(mainProgram, "modelMat"); /* Model matrix */
  viewUniform = glGetUniformLocation(mainProgram, "viewMat"); /* View matrix */
  texSampler = glGetUniformLocation(mainProgram, "diffuse"); /* Diffuse sampler */
  sphereMapSampler = glGetUniformLocation(mainProgram, "sphereMap"); /* Sphere map sampler */
  modelViewUniform = glGetUniformLocation(mainProgram, "modelView"); /* View * model matrix */
  lightPositionUniform = glGetUniformLocation(mainProgram, "lightPosWorld"); /* Light 1 position in world space */
  light2PositionUniform = glGetUniformLocation(mainProgram, "light2PosWorld"); /* Light 2 position in world space */
  normalMatrixUniform = glGetUniformLocation(mainProgram, "normalMatrix"); /* 3x3 truncated inverse-transpose of the model view matrix, for transforming normals */

  /* Create base matrixes */
  view = glm::lookAt(
    glm::vec3( /* Position */
      2.57,
      0.25,
      -2.78
    ),
    glm::vec3( /* Where do we look at? */
      0.0,
      0.0,
      0.0
    ),
    glm::vec3( /* What is the up vector? */
      0.0,
      1.0,
      0.0
    )
  );

  model = glm::mat4x4(
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1);

  proj = glm::perspective(
    45.0, /* FOV of 45 degrees */
    (double)SCREENWIDTH / (double)SCREENHEIGHT, /* Aspect ratio */
    0.2, /* Near clip plane */
    120.0 /* Far clip plane */
  );

  /* Derive other necessary matrixes */
  modelView = view * model;
  modelViewProj = proj * modelView;

  /* Now give the shader some data to work with */
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, modelPositionBuffer);
  glVertexAttribPointer(
    0, /* Attribute set 0 */
    3, /* 3 components in one item */
    GL_FLOAT, /* 32-bit floating point components */
    GL_FALSE, /* These are not normalized */
    0, /* No stride since we're neatly packing them */
    (void*)0 /* No buffer offset, see docs for details on this one */
  );

  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, modelUVBuffer);
  glVertexAttribPointer(
    1, /* Attribute set 1 */
    2, /* 2 components in one item */
    GL_FLOAT, /* 32-bit floating point components */
    GL_FALSE, /* These are not normalized */
    0, /* No stride since we're neatly packing them */
    (void*)0 /* No buffer offset, see docs for details on this one */
    );

  glEnableVertexAttribArray(2);
  glBindBuffer(GL_ARRAY_BUFFER, modelNormalBuffer);
  glVertexAttribPointer(
    2, /* Attribute set 2 */
    3, /* 3 components in one item */
    GL_FLOAT, /* 32-bit floating point components */
    GL_TRUE, /* These are normals, so they're normalized */
    0, /* No stride since we're neatly packing them */
    (void*)0 /* No buffer offset, see docs for details on this one */
  );

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelIndexBuffer); /* Bind the index buffer */

}
/* This function will run the main loop and take care of user's input */
void main_loop()
{
  double previous = glfwGetTime(), current, dt; /* Record the initial time */

  /* Loop while the user has not pressed the "close" button */
  while (!glfwWindowShouldClose(window)) {
    /* Record the current time, compute delta (frame time) and set
     * the time for this frame to be used as 'previous' in the next
     * frame */
    current = glfwGetTime();
    dt = current - previous;
    previous = current;

    /* Update the viewport and render to it */
    update(dt);
    render();

    /* Ask GLFW to present what we rendered to the screen and to do the regular
     * message pump */
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
}

int main(int argc, char** argv)
{
  init_all();
  main_loop();

  return(0);
}