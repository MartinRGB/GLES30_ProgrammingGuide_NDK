// The MIT License (MIT)
//
// Copyright (c) 2013 Dan Ginsburg, Budirijanto Purnomo
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

//
// Book:      OpenGL(R) ES 3.0 Programming Guide, 2nd Edition
// Authors:   Dan Ginsburg, Budirijanto Purnomo, Dave Shreiner, Aaftab Munshi
// ISBN-10:   0-321-93388-5
// ISBN-13:   978-0-321-93388-1
// Publisher: Addison-Wesley Professional
// URLs:      http://www.opengles-book.com
//            http://my.safaribooksonline.com/book/animation-and-3d/9780133440133
//
// TextureWrap.c
//
//    This is an example that demonstrates the three texture
//    wrap modes available on 2D textures
//
#include <stdlib.h>
#include "esUtil.h"

typedef struct
{
   // Handle to a program object
   GLuint programObject;

   // Sampler location
   GLint samplerLoc;

   // Texture handle
   GLuint textureId;

   // Vertex data
   int      numIndices;
   GLfloat *vertices;
   GLfloat *normals;
   GLuint  *indices;

   // MVP 矩阵信息
   GLint  mvpLoc;
   GLint  mvLoc;
    // 旋转角度
   GLfloat   angle;
   // 矩阵
   ESMatrix  mvpMatrix;
   ESMatrix  mvMatrix;

   GLint lightLoc;
   GLint timeLoc;

   // Current time
   float time;

} UserData;

///
// Create a simple cubemap with a 1x1 face with a different
// color for each face
GLuint CreateSimpleTextureCubemap( )
{
   GLuint textureId;
   // Six 1x1 RGB faces
   GLubyte cubePixels[6][3] =
   {
      // Face 0 - Red
      255, 0, 0,
      // Face 1 - Green,
      0, 255, 0,
      // Face 2 - Blue
      0, 0, 255,
      // Face 3 - Yellow
      255, 255, 0,
      // Face 4 - Purple
      255, 0, 255,
      // Face 5 - White
      255, 255, 255
   };

   // Generate a texture object
   glGenTextures ( 1, &textureId );

   // Bind the texture object
   glBindTexture ( GL_TEXTURE_CUBE_MAP, textureId );

   // Load the cube face - Positive X
   glTexImage2D ( GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, 1, 1, 0,
                  GL_RGB, GL_UNSIGNED_BYTE, &cubePixels[0] );

   // Load the cube face - Negative X
   glTexImage2D ( GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, 1, 1, 0,
                  GL_RGB, GL_UNSIGNED_BYTE, &cubePixels[1] );

   // Load the cube face - Positive Y
   glTexImage2D ( GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, 1, 1, 0,
                  GL_RGB, GL_UNSIGNED_BYTE, &cubePixels[2] );

   // Load the cube face - Negative Y
   glTexImage2D ( GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, 1, 1, 0,
                  GL_RGB, GL_UNSIGNED_BYTE, &cubePixels[3] );

   // Load the cube face - Positive Z
   glTexImage2D ( GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, 1, 1, 0,
                  GL_RGB, GL_UNSIGNED_BYTE, &cubePixels[4] );

   // Load the cube face - Negative Z
   glTexImage2D ( GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, 1, 1, 0,
                  GL_RGB, GL_UNSIGNED_BYTE, &cubePixels[5] );

   // Set the filtering mode
   glTexParameteri ( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glTexParameteri ( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

   return textureId;

}


///
// Initialize the shader and program object
//
int Init ( ESContext *esContext )
{
   UserData *userData = esContext->userData;
   char vShaderStr[] =
      "#version 300 es                            \n"
      "layout(location = 0) in vec4 a_position;   \n"
      "layout(location = 1) in vec3 a_normal;     \n"
      "uniform mat4 u_mvpMatrix;                  \n"
      "uniform mat4 u_mvMatrix;                   \n"
      "uniform vec3 u_LightPos;                   \n"
      "out vec3 v_normal;                         \n"
      "out vec3 rotationUV;                       \n"
      "out vec3 v_position;                       \n"
      "void main()                                \n"
      "{                                          \n"
      "   gl_Position = u_mvpMatrix * a_position; \n"
      "   v_normal = vec3(u_mvMatrix*vec4(a_normal,0.0));\n"
      "   v_position=vec3(u_mvMatrix*a_position); \n"
      "   rotationUV = a_normal;                  \n"
      "}                                          \n";

   char fShaderStr[] =
      "#version 300 es                                     \n"
      "precision mediump float;                            \n"
      "in vec3 v_normal;                                   \n"
      "in vec3 v_position;                                 \n"
      "in vec3 rotationUV;                                 \n"
      "layout(location = 0) out vec4 outColor;             \n"
      "uniform samplerCube s_texture;                      \n"
      "uniform vec3 u_LightPos;                   \n"
      "void main()                                         \n"
      "{                                                   \n"
      "   float distance = length(u_LightPos - v_position);\n"
      "   vec3 lightVector = normalize(u_LightPos - v_position);\n"
      "   float diffuse = max(dot(v_normal, lightVector), 0.3);\n"
      "   diffuse = diffuse * (1.0 / (1.0 + (0.25 * distance * distance)));\n"
      "   outColor = texture( s_texture, rotationUV) * (diffuse*2.0);       \n"
      "}                                                   \n";

   // Load the shaders and get a linked program object
   userData->programObject = esLoadProgram ( vShaderStr, fShaderStr );

   // Get the sampler location
   // Get the uniform locations
   userData->mvpLoc = glGetUniformLocation ( userData->programObject, "u_mvpMatrix" );
   userData->mvLoc = glGetUniformLocation ( userData->programObject, "u_mvMatrix" );
   userData->samplerLoc = glGetUniformLocation ( userData->programObject, "s_texture" );
   userData->lightLoc = glGetUniformLocation ( userData->programObject, "u_LightPos" );
   userData->timeLoc = glGetUniformLocation ( userData->programObject, "u_time" );

   // Load the texture
   userData->textureId = CreateSimpleTextureCubemap ();

   // Generate the vertex data
   userData->numIndices = esGenCube (0.5f, &userData->vertices, &userData->normals,
                                        NULL, &userData->indices );

   glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f );
   return TRUE;
}

void Update ( ESContext *esContext, float deltaTime )
{
   UserData *userData = esContext->userData;
   ESMatrix projection;
   float    aspect;

   userData->time += deltaTime;

   glUniform1f(userData->time,deltaTime);

   //Compute a rotation angle based on time to rotate the cube
   userData->angle += ( deltaTime * 40.0f );

   if ( userData->angle >= 360.0f )
   {
      userData->angle -= 360.0f;
   }

   // Compute the window aspect ratio
   aspect = ( GLfloat ) esContext->width / ( GLfloat ) esContext->height;

   // Generate a projection matrix with a 60 degree FOV，60度的视场
   // &projection 指针指向 projection 的地址
   esMatrixLoadIdentity ( &projection );
   esPerspective ( &projection, 60.0f, aspect, 1.0f, 20.0f );

   // Generate a model view matrix to rotate/translate the cube
   // 在modelView矩阵中加载一个单位矩阵
   esMatrixLoadIdentity ( &userData->mvMatrix );

   // Translate away from the viewer
   esTranslate ( &userData->mvMatrix, 0.0, 0.0, -2.0 );

   // Rotate the cube
   esRotate ( &userData->mvMatrix, userData->angle, 1.0, 0.0, 1.0 );

   // Compute the final MVP by multiplying the
   // modevleiw and perspective matrices together
   esMatrixMultiply ( &userData->mvpMatrix, &userData->mvMatrix, &projection );
}

///
// Draw a triangle using the shader pair created in Init()
//
void Draw ( ESContext *esContext )
{
   UserData *userData = esContext->userData;

   // Set the viewport
   glViewport ( 0, 0, esContext->width, esContext->height );

   // Clear the color buffer
   glClear ( GL_COLOR_BUFFER_BIT );

   glCullFace ( GL_BACK );
   glEnable ( GL_CULL_FACE );

   // Use the program object
   glUseProgram ( userData->programObject );

   // 读取 Uniform
   glUniformMatrix4fv ( userData->mvpLoc, 1, GL_FALSE, ( GLfloat * ) &userData->mvpMatrix.m[0][0] );
   glUniformMatrix4fv ( userData->mvLoc, 1, GL_FALSE, ( GLfloat * ) &userData->mvMatrix.m[0][0] );
   glUniform3f(userData->lightLoc,0.f,0.f,1.f);

   // Load the vertex position
   glVertexAttribPointer ( 0, 3, GL_FLOAT,
                           GL_FALSE, 0, userData->vertices );
   // Load the normal
   glVertexAttribPointer ( 1, 3, GL_FLOAT,
                           GL_FALSE, 0, userData->normals );

   glEnableVertexAttribArray ( 0 );
   glEnableVertexAttribArray ( 1 );

   // Bind the texture
   glActiveTexture ( GL_TEXTURE0 );
   glBindTexture ( GL_TEXTURE_CUBE_MAP, userData->textureId );
   // Set the sampler texture unit to 0
   glUniform1i ( userData->samplerLoc, 0 );

   glDrawElements ( GL_TRIANGLES, userData->numIndices,
                    GL_UNSIGNED_INT, userData->indices );

   // glDisableVertexAttribArray (0);
   // glDisableVertexAttribArray (1);

   // glDisable(GL_CULL_FACE);


}

///
// Cleanup
//
void ShutDown ( ESContext *esContext )
{
   UserData *userData = esContext->userData;

   // Delete texture object
   glDeleteTextures ( 1, &userData->textureId );

   // Delete program object
   glDeleteProgram ( userData->programObject );

   free ( userData->vertices );
   free ( userData->normals );
}


int esMain ( ESContext *esContext )
{
   esContext->userData = malloc ( sizeof ( UserData ) );

   esCreateWindow ( esContext, "Simple Texture Cubemap", 320, 240, ES_WINDOW_RGB );

   if ( !Init ( esContext ) )
   {
      return GL_FALSE;
   }

   esRegisterDrawFunc ( esContext, Draw );
   esRegisterUpdateFunc ( esContext, Update );
   esRegisterShutdownFunc ( esContext, ShutDown );

   return GL_TRUE;
}
