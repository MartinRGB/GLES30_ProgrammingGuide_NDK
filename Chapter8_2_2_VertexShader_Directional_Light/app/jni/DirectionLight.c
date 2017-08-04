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
// Simple_VertexShader.c
//
//    This is a simple example that draws a rotating cube in perspective
//    using a vertex shader to transform the object
//
#include <stdlib.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "esUtil.h"

//定义结构体 UserData
typedef struct
{
   // 程序对象
   GLuint programObject;

   // MVP 矩阵信息
   GLint  mvpLoc;
   GLint  mvLoc;

   // 顶点属性数据 顶点位置、顶点索引、顶点索引个数
   GLfloat  *vertices;
   GLfloat  *normals;
   GLuint   *indices;
   int       numIndices;

   // 旋转角度
   GLfloat   angle;

   // 矩阵
   ESMatrix  mvpMatrix;
   ESMatrix  mvMatrix;

   // material uniform block
   GLuint    materaialBindingPoint;
   GLuint    materialBuffer;
   GLuint    materialBlockIndex;

   // light uniform block
   GLuint    lightBindingPoint;  
   GLuint    lightBuffer;   
   GLuint    lightBlockIndex;  
} UserData;


GLfloat materialFloats[13] = {
//ambient_color
0.2f, 0.2f, 0.2f, 1.0f,
//diffuse_color
0.7f, 0.7f, 1.0f, 1.0f,
//specular_color
0.5f, 0.5f, 0.5f, 1.0f,
//specular_exponent
64.0f};  

GLfloat lightFloats[18] = {
   //direction
   0.0f, 1.0f, 1.0f, 
   //halfplane
   0.0f, 1.0f, 2.0f, 
   //ambient_color
   1.0f, 1.0f, 1.0f, 1.0f,
   //diffuse_color
   1.0f, 0.0f, 1.0f, 1.0f,
   //specular_color
   1.0f, 1.0f, 1.0f, 1.0f
}; 

//Log
#include <android/log.h>
#define APPNAME "DirectionLight"

///
// Initialize the shader and program object
//


int Init ( ESContext *esContext )
{
   //定义结构体指针，将 userData 中的值赋给 esContext
   UserData *userData = esContext->userData;
   const char vShaderStr[] =
      "#version 320 es                             \n"
      "uniform mat4 u_mvpMatrix;                   \n"
      "uniform mat3 u_mvMatrix;                   \n"

      "layout (binding = 0) uniform u_material {\n" 
      "   vec4 material_ambient_color;\n" 
      "   vec4 material_diffuse_color;\n" 
      "   vec4 material_specular_color;\n" 
      "   float material_specular_exponent;\n" 
      "};\n" 
      "\n" 
      "layout (binding = 1) uniform u_light {\n" 
      "   vec3 light_direction;\n" 
      "   vec3 light_halfplane;\n" 
      "   vec4 light_ambient_color;\n" 
      "   vec4 light_diffuse_color;\n" 
      "   vec4 light_specular_color;\n" 
      "};\n" 
      "\n" 

      "layout(location = 0) in vec4 a_position;    \n"
      "layout(location = 1) in vec3 a_normal;       \n"
      "out vec4 v_color;                           \n"
      "\n" 
      "const float c_zero = 0.0;\n" 
      "const float c_one = 1.0;\n" 
      "\n" 
      "//directional light\n" 
      "vec4 compute_color(vec3 normal)\n" 
      "{\n" 
      "   vec4 computed_color = vec4(c_zero, c_zero, c_zero, c_zero);\n" 
      "   float ndotl;   // normal dot light direction\n" 
      "   float ndoth;   // normal dot half-plane vector\n" 
      "   \n" 
      "   ndotl = max(c_zero, dot(normal, light_direction));\n" 
      "   ndoth = max(c_zero, dot(normal, light_halfplane));\n" 
      "   \n" 
      "   computed_color += (light_ambient_color * material_ambient_color);\n" 
      "   computed_color += (ndotl * light_diffuse_color * material_diffuse_color);\n" 
      "   \n" 
      "   if (ndoth > c_zero) {\n" 
      "      computed_color += (pow(ndoth, material_specular_exponent) * material_specular_color * light_specular_color);\n" 
      "   }\n" 
      "   return computed_color;\n" 
      "}\n" 
      "\n" 

      "void main()                                 \n"
      "{                                           \n"
      "   gl_Position = u_mvpMatrix * a_position;  \n"
      "   v_color = compute_color(normalize(u_mvMatrix * a_normal));         \n"
      "}                                           \n";

   //http://duanhengbin.iteye.com/blog/1706635

      // "   v_color = vec4(normalize(u_mvMatrix * a_normal),1.0); \n"
   const char fShaderStr[] =
      "#version 320 es                                \n"
      "precision mediump float;                       \n"
      "in vec4 v_color;                               \n"
      "layout(location = 0) out vec4 outColor;        \n"
      "void main()                                    \n"
      "{                                              \n"
      "  outColor = v_color;                          \n"
      "}                                              \n";
   //首先 a->b 的含義是 (*a).b  ,也就是 (*userData).programObject
   //Load the shaders and get a linked program object
   
   userData->programObject = esLoadProgram ( vShaderStr, fShaderStr );
   //userData->programObject = esLoadProgram ( "phong.vert", "phong.frag" );

   // Get the uniform locations
   userData->mvpLoc = glGetUniformLocation ( userData->programObject, "u_mvpMatrix" );
   userData->mvLoc = glGetUniformLocation ( userData->programObject, "u_mvMatrix" );

   // 将Shader中绑定点和 material 和 light 的索引分别绑定
   userData->materaialBindingPoint = 0;
   userData->materialBlockIndex = glGetUniformBlockIndex(userData -> programObject, "u_material");
   glUniformBlockBinding(userData -> programObject, userData -> materialBlockIndex, userData -> materaialBindingPoint);

   userData->lightBindingPoint = 1;
   userData->lightBlockIndex = glGetUniformBlockIndex(userData -> programObject, "u_light");
   glUniformBlockBinding(userData -> programObject, userData -> lightBlockIndex, userData -> lightBindingPoint);


   // Generate the vertex & normals data by using esGenCube
   userData->numIndices = esGenCube ( 0.5, &userData->vertices,
                                      &userData->normals, NULL, &userData->indices );

   // Starting rotation angle for the cube
   userData->angle = 45.0f;

   glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f );

    __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "The value of 1 + 1 is %d", 1+1);


   return GL_TRUE;
}


///
// Update MVP matrix based on time
//
void Update ( ESContext *esContext, float deltaTime )
{
   UserData *userData = esContext->userData;
   ESMatrix projection;
   //ESMatrix modelview;
   float    aspect;


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

   // 设置 viewport
   glViewport ( 0, 0, esContext->width, esContext->height );

   //开启剔除效果
   glFrontFace(GL_CCW);
   glCullFace(GL_BACK);
   glEnable(GL_CULL_FACE);

   // Clear the color buffer
   glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   // Use the program object
   glUseProgram ( userData->programObject );

   // 读取 Uniform
   glUniformMatrix4fv ( userData->mvpLoc, 1, GL_FALSE, ( GLfloat * ) &userData->mvpMatrix.m[0][0] );
   glUniformMatrix3fv ( userData->mvLoc, 1, GL_FALSE, ( GLfloat * ) &userData->mvMatrix.m[0][0] );

   // 绑定 Material 并且生成 UBO
   glGenBuffers(1, &userData->materialBuffer);
   glBindBuffer(GL_UNIFORM_BUFFER, userData->materialBuffer);
   glBufferData(GL_UNIFORM_BUFFER, sizeof(materialFloats), materialFloats, GL_DYNAMIC_DRAW);
   glBindBufferBase(GL_UNIFORM_BUFFER, userData->materaialBindingPoint, userData->materialBuffer);

   // 绑定 Material 并且生成 UBO
   glGenBuffers(1, &userData->lightBuffer);
   glBindBuffer(GL_UNIFORM_BUFFER, userData->lightBuffer);
   glBufferData(GL_UNIFORM_BUFFER, sizeof(lightFloats), lightFloats, GL_DYNAMIC_DRAW);
   glBindBufferBase(GL_UNIFORM_BUFFER, userData->lightBindingPoint, userData->lightBuffer);

   // Vertex Attribute
   glEnableVertexAttribArray ( 0 );
   glVertexAttribPointer ( 0, 3, GL_FLOAT,GL_FALSE, 3 * sizeof ( GLfloat ), userData->vertices );
   
   // Normal Attribute
   glEnableVertexAttribArray ( 1 );
   glVertexAttribPointer ( 1, 3, GL_FLOAT,GL_TRUE, 3 * sizeof ( GLfloat ), userData->normals );


   // 绘制
   glDrawElements ( GL_TRIANGLES, userData->numIndices, GL_UNSIGNED_INT, userData->indices );

   // 禁用顶点属性数组
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   // 解除 UBO
   glBindBuffer(GL_UNIFORM_BUFFER, 0);

   // 关闭剔除
   glDisable(GL_CULL_FACE);



}


///
// Cleanup
//
void Shutdown ( ESContext *esContext )
{
   UserData *userData = esContext->userData;

   if ( userData->vertices != NULL )
   {
      free ( userData->vertices );
      free ( userData->normals );
   }

   if ( userData->indices != NULL )
   {
      free ( userData->indices );
      free ( userData->normals );
   }

   // Delete program object
   glDeleteProgram ( userData->programObject );
}


int esMain ( ESContext *esContext )
{
   esContext->userData = malloc ( sizeof ( UserData ) );

   esCreateWindow ( esContext, "Simple_VertexShader", 320, 240, ES_WINDOW_RGB | ES_WINDOW_DEPTH );

   if ( !Init ( esContext ) )
   {
      return GL_FALSE;
   }

   esRegisterShutdownFunc ( esContext, Shutdown );
   esRegisterUpdateFunc ( esContext, Update );
   esRegisterDrawFunc ( esContext, Draw );

   return GL_TRUE;
}

