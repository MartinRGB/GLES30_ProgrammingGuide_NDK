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
// MultiTexture.c
//
//    This is an example that draws a quad with a basemap and
//    lightmap to demonstrate multitexturing.
//
#include <stdlib.h>
#include "esUtil.h"

typedef struct
{
   // Handle to a program object
   GLuint programObject;

   // Sampler locations
   GLint baseMapLoc;
   GLint lightMapLoc;

   // Texture handle
   GLuint baseMapTexId;
   GLuint lightMapTexId;

   // Vertex data
   int      numIndices;
   GLfloat *vertices;
   GLfloat *texcoords;
   GLuint  *indices;

   GLint eyeLoc;
   GLint fogColorLoc;
   GLint fogMaxDistLoc;
   GLint fogMinDistLoc;

   // MVP 矩阵信息
   GLint  mvpLoc;
   GLint mvLoc;
    // 旋转角度
   GLfloat   angle;
   // 矩阵
   ESMatrix  mvpMatrix;
   ESMatrix  mvMatrix;
   // Current time
   float time;

} UserData;

///
// Load texture from disk
//
GLuint LoadTexture ( void *ioContext, char *fileName )
{
   int width,
       height;

   char *buffer = esLoadTGA ( ioContext, fileName, &width, &height );
   GLuint texId;

   if ( buffer == NULL )
   {
      esLogMessage ( "Error loading (%s) image.\n", fileName );
      return 0;
   }

   glGenTextures ( 1, &texId );
   glBindTexture ( GL_TEXTURE_2D, texId );

   glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, buffer );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

   free ( buffer );

   return texId;
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
      "layout(location = 1) in vec2 a_texCoord;   \n"
      "uniform mat4 u_mvpMatrix;                  \n"
      "uniform mat4 u_mvMatrix;                   \n"
      "uniform vec4 u_eyePosition;                \n"
      "out float v_eyeDist;                       \n"
      "out vec2 v_texCoord;                       \n"
      "void main()                                \n"
      "{                                          \n"
      "   vec4 vViewPos = u_mvMatrix * a_position;\n"
      "   gl_Position = u_mvpMatrix * a_position; \n"
      "   v_texCoord = a_texCoord;                \n"
      "   v_eyeDist = sqrt((vViewPos.x - u_eyePosition.x)*(vViewPos.x - u_eyePosition.x)+(vViewPos.y - u_eyePosition.y)*(vViewPos.y - u_eyePosition.y)+(vViewPos.z - u_eyePosition.z)*(vViewPos.z - u_eyePosition.z));                        \n"
      "}                                          \n";

      //s
   char fShaderStr[] =
      "#version 300 es                                     \n"
      "precision mediump float;                            \n"
      "in float v_eyeDist;                                 \n"
      "in vec2 v_texCoord;                                 \n"
      "layout(location = 0) out vec4 outColor;             \n"
      "uniform vec4 u_fogColor;                        \n"
      "uniform float u_fogMaxDist;                        \n"
      "uniform float u_fogMinDist;                        \n"
      "uniform sampler2D s_baseMap;                        \n"
      "uniform sampler2D s_lightMap;                       \n"
      "float computeLinearFogFactor(){\n"
      "    float factor;\n"
      "    factor = (u_fogMaxDist - v_eyeDist)/(u_fogMaxDist - u_fogMinDist);\n"
      "    factor = clamp(factor,0.0,1.0);\n"
      "    return factor;\n"
      "}\n"
      "void main()                                         \n"
      "{                                                   \n"
      "  vec4 baseColor;                                   \n"
      "  vec4 lightColor;                                  \n"
      "  float fogFactor = computeLinearFogFactor();       \n"
      "  baseColor = texture( s_baseMap, v_texCoord );     \n"
      "  lightColor = texture( s_lightMap, v_texCoord );   \n"
      "  vec4 texColor = baseColor * (lightColor + 0.25);       \n"
      "  outColor = texColor * fogFactor + u_fogColor * (1.0 - fogFactor); \n"
      "}                                                   \n";

   // Load the shaders and get a linked program object
   userData->programObject = esLoadProgram ( vShaderStr, fShaderStr );

   // Get the sampler location
   userData->mvpLoc = glGetUniformLocation ( userData->programObject, "u_mvpMatrix" );
   userData->mvLoc = glGetUniformLocation ( userData->programObject, "u_mvMatrix" );
   userData->eyeLoc = glGetUniformLocation ( userData->programObject, "u_eyePosition" );
   userData->fogColorLoc = glGetUniformLocation ( userData->programObject, "u_fogColor" );
   userData->fogMaxDistLoc = glGetUniformLocation ( userData->programObject, "u_fogMaxDist" );
   userData->fogMinDistLoc = glGetUniformLocation ( userData->programObject, "u_fogMinDist" );
   userData->baseMapLoc = glGetUniformLocation ( userData->programObject, "s_baseMap" );
   userData->lightMapLoc = glGetUniformLocation ( userData->programObject, "s_lightMap" );

   // Load the textures
   userData->baseMapTexId = LoadTexture ( esContext->platformData, "basemap.tga" );
   userData->lightMapTexId = LoadTexture ( esContext->platformData, "lightmap.tga" );

   if ( userData->baseMapTexId == 0 || userData->lightMapTexId == 0 )
   {
      return FALSE;
   }

   // Generate the vertex data
   userData->numIndices = esGenCube (0.75f, &userData->vertices, NULL,
                                        &userData->texcoords, &userData->indices );

   glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f );
   return TRUE;
}

void Update ( ESContext *esContext, float deltaTime )
{
   UserData *userData = esContext->userData;
   ESMatrix projection;
   //ESMatrix modelview;
   float    aspect;

   userData->time += deltaTime;

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
   glUniform4f( userData->eyeLoc, 0.0f,0.0f,-1.25f,1.0f);
   glUniform4f( userData->fogColorLoc, 0.0f,0.0f,0.0f,1.0f);
   glUniform1f( userData->fogMaxDistLoc, 1.0f);
   glUniform1f( userData->fogMinDistLoc, 0.0f);

   // Load the vertex position
   glVertexAttribPointer ( 0, 3, GL_FLOAT,
                           GL_FALSE, 0, userData->vertices );
   // Load the normal
   glVertexAttribPointer ( 1, 2, GL_FLOAT,
                           GL_FALSE, 0, userData->texcoords );

   glEnableVertexAttribArray ( 0 );
   glEnableVertexAttribArray ( 1 );

   // Bind the base map
   glActiveTexture ( GL_TEXTURE0 );
   glBindTexture ( GL_TEXTURE_2D, userData->baseMapTexId );

   // Set the base map sampler to texture unit to 0
   glUniform1i ( userData->baseMapLoc, 0 );

   // Bind the light map
   glActiveTexture ( GL_TEXTURE1 );
   glBindTexture ( GL_TEXTURE_2D, userData->lightMapTexId );

   // Set the light map sampler to texture unit 1
   glUniform1i ( userData->lightMapLoc, 1 );

   glDrawElements ( GL_TRIANGLES, userData->numIndices, GL_UNSIGNED_INT, userData->indices );

   glDisableVertexAttribArray ( 0 );
   glDisableVertexAttribArray ( 1 );

   glDisable(GL_CULL_FACE);
}

///
// Cleanup
//
void ShutDown ( ESContext *esContext )
{
   UserData *userData = esContext->userData;

   // Delete texture object
   glDeleteTextures ( 1, &userData->baseMapTexId );
   glDeleteTextures ( 1, &userData->lightMapTexId );

   // Delete program object
   glDeleteProgram ( userData->programObject );

   free ( userData->vertices );
   free ( userData->texcoords );
}

int esMain ( ESContext *esContext )
{
   esContext->userData = malloc ( sizeof ( UserData ) );

   esCreateWindow ( esContext, "MultiTexture", 320, 240, ES_WINDOW_RGB );

   if ( !Init ( esContext ) )
   {
      return GL_FALSE;
   }

   esRegisterDrawFunc ( esContext, Draw );
   esRegisterUpdateFunc ( esContext, Update );
   esRegisterShutdownFunc ( esContext, ShutDown );

   return GL_TRUE;
}
