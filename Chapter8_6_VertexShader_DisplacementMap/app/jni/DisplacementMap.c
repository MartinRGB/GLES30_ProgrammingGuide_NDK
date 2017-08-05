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
#include <math.h>
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
   GLfloat  *texCoords;
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

   // Sampler location
   GLint samplerLoc;
   // Texture handle
   GLuint textureId;

   // Time Location
   GLint timeLoc;
   // Current time
   float time;

} UserData;


GLfloat materialFloats[13] = {
//ambient_color
0.0f, 0.4f, 0.8f, 1.0f,
//diffuse_color
0.4f, 0.9f, 0.8f, 1.0f,
//specular_color
0.8f, 0.2f, 0.9f, 1.0f,
//specular_exponent
20.0f};  

GLfloat lightFloats[24] = {
//light position
0.0f, 1.0f, 0.0f, 1.0f,
//ambient_color
0.0f, 0.8f, 0.4f, 1.0f,
//diffuse_color
0.2f, 0.1f, 0.8f, 1.0f,
//specular_color
0.5f, 0.1f, 0.9f, 1.0f,
//spot direction
1.0f, 0.0f, 1.0f,
//light attenuation factors
1.0f, 0.0f, 0.0f,
//spot exponent
3.0f,
//cutoff angle
90.0f
}; 

//Log
#include <android/log.h>
#define APPNAME "TexMap"

///
// Initialize the shader and program object
//

/// 从本地磁盘读取 tga 材质
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
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
   glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

   free ( buffer );

   return texId;
}



int Init ( ESContext *esContext )
{
   //定义结构体指针，将 userData 中的值赋给 esContext
   UserData *userData = esContext->userData;
   const char vShaderStr[] =
         "#version 320 es\n"
         "\n"
         "precision highp float;                       \n"
         "layout (binding = 1) uniform u_light\n"
         "{\n"
         "    vec4 light_position;\n"
         "    vec4 light_ambient_color;\n"
         "    vec4 light_diffuse_color;\n"
         "    vec4 light_specular_color;\n"
         "    vec3 light_spot_direction;\n"
         "    vec3 light_attenuation_factors;\n"
         "    float light_spot_exponent;\n"
         "    float light_spot_cutoff_angle;\n"
         "};\n"
         "\n"
         "layout (binding = 0) uniform u_material\n"
         "{\n"
         "    vec4 material_ambient_color;\n"
         "    vec4 material_diffuse_color;\n"
         "    vec4 material_specular_color;\n"
         "    float material_specular_exponent;\n"
         "};\n"
         "\n"
         "uniform mat4 u_mvpMatrix;\n"
         "uniform mat4 u_mvMatrix;\n"
         "uniform float u_time;\n"
         "uniform sampler2D u_texture;                        \n"
         "layout(location = 0) in vec4 a_position;    \n"
         "layout(location = 1) in vec3 a_normal;       \n"
         "layout(location = 2) in vec2 a_texCoord;       \n"
         "out vec4 v_color;                           \n"
         "out vec2 v_texCoord;                           \n"
         "out vec2 v_texMap;                           \n"
         "\n"
         "const float c_zero = 0.0;\n"
         "const float c_one = 1.0;\n"
         "\n"
         "vec4 spot_light_color (vec3 normal, vec4 position)\n"
         "{\n"
         "    vec4 computed_color = vec4 (c_zero, c_zero, c_zero, c_zero);\n"
         "\n"
         "    vec3 lightDir;\n"
         "    bool light_compute_distance_attenuation = true;\n"
         "    vec3 halfplane;\n"
         "    float NdotL, NdotH;\n"
         "    float att_factor;\n"
         "\n"
         "    att_factor = c_one;\n"
         "    lightDir = light_position.xyz - position.xyz;\n"
         "\n"
         "    if (light_compute_distance_attenuation) {\n"
         "        vec3 att_dist;\n"
         "        att_dist.x = c_one;\n"
         "        att_dist.z = dot (lightDir, lightDir);\n"
         "        att_dist.y = sqrt(att_dist.z);    \n"
         "        att_factor = c_one / dot(att_dist, light_attenuation_factors);\n"
         "   }\n"
         "\n"
         "    lightDir = normalize (lightDir);\n"
         "\n"
         "    if (light_spot_cutoff_angle < 180.0) {\n"
         "        float spot_factor = dot (-lightDir, light_spot_direction);\n"
         "        if (spot_factor >= cos(radians(light_spot_cutoff_angle)) )\n"
         "            spot_factor = pow (spot_factor, light_spot_exponent);\n"
         "        else\n"
         "            spot_factor = c_zero;\n"
         "\n"
         "        att_factor *= spot_factor;\n"
         "    }\n"
         "\n"
         "    if (att_factor > c_zero) {\n"
         "        computed_color += light_ambient_color * material_ambient_color;\n"
         "\n"
         "       NdotL = max(c_zero, dot(normal, lightDir));\n"
         "        computed_color += NdotL * light_diffuse_color * material_diffuse_color;\n"
         "\n"
         "        halfplane = normalize (lightDir + vec3(c_zero, c_zero, c_one));\n"
         "        NdotH = (dot (normal, halfplane));\n"
         "        if (NdotH > c_zero) {\n"
         "            computed_color += pow (NdotH, material_specular_exponent) * material_specular_color * light_specular_color;\n"
         "       }\n"
         "\n"
         "        computed_color *= att_factor;\n"
         "   }\n"
         "\n"
         "    return computed_color;\n"
         "}\n"
         "\n"
         "mat4 rotationMatrix(vec3 axis, float angle)\n"
         "{\n"
         "    axis = normalize(axis);\n"
         "    float s = sin(angle);\n"
         "    float c = cos(angle);\n"
         "    float oc = 1.0 - c;\n"
         "    \n"
         "    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,\n"
         "                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,\n"
         "                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,\n"
         "                0.0,                                0.0,                                0.0,                                1.0);\n"
         "}\n"
         "vec2 sphere_map(vec3 position,vec3 normal){\n"
         "    vec3 reflection = reflect(position,normal);\n"
         "    float m = 2.0 * sqrt(reflection.x * reflection.x + reflection.y * reflection.y+\n"
         "                   (reflection.z + 1.0) *  (reflection.z + 1.0));\n"
         "    return vec2((reflection.x/m+0.5),(reflection.y/m+0.5));\n"
         "}\n"
         "\n"
         "vec2 cube_map(vec3 position,vec3 normal){\n"
         "    vec3 reflection = reflect(position,normal);\n"
         "    return reflection.xy;\n"
         "}\n"
         "void main()                                 \n"
         "{                                           \n"
         "   vec4 newPosition = a_position * rotationMatrix(vec3(0.,1.*u_time,0.),1.*u_time); \n"
         "   float displacement = texture(u_texture,a_texCoord).a;\n"
         "   vec4 displaced_position = a_position + vec4(a_normal * displacement,0.0)*(sin(u_time)+1.)/2.; \n"
         "   gl_Position = u_mvpMatrix * displaced_position;  \n"
         "   v_color = vec4(0.,0.4,0.8,1.0)*0.5 +spot_light_color(normalize(u_mvMatrix * vec4(a_normal,1.0)).xyz,u_mvpMatrix*newPosition);         \n"
         "   v_texCoord = a_texCoord;                \n"
         "   v_texMap = sphere_map((u_mvMatrix * vec4(a_texCoord,1.,1.)).xyz,normalize(u_mvMatrix * vec4(a_normal,1.0)).xyz); \n"
         "}";

   //http://duanhengbin.iteye.com/blog/1706635
   
   const char fShaderStr[] =
         "#version 320 es                                \n"
         "precision highp float;                       \n"
         "uniform sampler2D u_texture;                        \n"
         "uniform float u_time;                        \n"
         "in vec4 v_color;                               \n"
         "in vec2 v_texCoord;                               \n"
         "in vec2 v_texMap;                               \n"
         "layout(location = 0) out vec4 outColor;        \n"
         "void main()                                    \n"
         "{                                              \n"
         "  vec4 outTexture = texture( u_texture,  vec2(v_texMap.x+u_time/10.,v_texMap.y+u_time/10.)); \n"
         "  outColor = v_color*0.5 + outTexture *0.5;                          \n"
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

   // 获取材质的内存位置，从本地磁盘获取材质
   userData->samplerLoc = glGetUniformLocation ( userData->programObject, "u_texture" );
   userData->textureId = LoadTexture(esContext->platformData, "sfsunset.tga");

   userData->timeLoc = glGetUniformLocation ( userData->programObject, "u_time" );


   // Generate the vertex & normals data by using esGenCube
   userData->numIndices = esGenCube ( 0.5, &userData->vertices,
                                      &userData->normals, &userData->texCoords, &userData->indices );

   // Starting rotation angle for the cube
   userData->angle = 0.0f;

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
   ESMatrix viewMatrix;
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
   esTranslate ( &userData->mvMatrix, 0.0, 0.0, -5.0 + sin(deltaTime) );

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
   //glEnable(GL_CULL_FACE);

   // Clear the color buffer
   glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   // Use the program object
   glUseProgram ( userData->programObject );

   // 读取 Uniform
   glUniformMatrix4fv ( userData->mvpLoc, 1, GL_FALSE, ( GLfloat * ) &userData->mvpMatrix.m[0][0] );
   glUniformMatrix4fv ( userData->mvLoc, 1, GL_FALSE, ( GLfloat * ) &userData->mvMatrix.m[0][0] );


   // 绑定 Material 并且生成 UBO
   glGenBuffers(1, &userData->materialBuffer);
   glBindBuffer(GL_UNIFORM_BUFFER, userData->materialBuffer);
   glBufferData(GL_UNIFORM_BUFFER, sizeof(materialFloats), materialFloats, GL_DYNAMIC_DRAW);
   glBindBufferBase(GL_UNIFORM_BUFFER, userData->materaialBindingPoint, userData->materialBuffer);

   // 绑定 Light 并且生成 UBO
   glGenBuffers(1, &userData->lightBuffer);
   glBindBuffer(GL_UNIFORM_BUFFER, userData->lightBuffer);
   glBufferData(GL_UNIFORM_BUFFER, sizeof(lightFloats), lightFloats, GL_DYNAMIC_DRAW);
   glBindBufferBase(GL_UNIFORM_BUFFER, userData->lightBindingPoint, userData->lightBuffer);

   // 绑定材质
   glActiveTexture ( GL_TEXTURE0 );
   glBindTexture ( GL_TEXTURE_2D, userData->textureId );
   // 将材质设置到 shader 里面
   glUniform1i ( userData->samplerLoc, 0 );

   // 时间
   glUniform1f ( userData->timeLoc, userData->time );

   // Vertex Attribute
   glEnableVertexAttribArray ( 0 );
   glVertexAttribPointer ( 0, 3, GL_FLOAT,GL_FALSE, 3 * sizeof ( GLfloat ), userData->vertices );
   
   // Normal Attribute
   glEnableVertexAttribArray ( 1 );
   glVertexAttribPointer ( 1, 3, GL_FLOAT,GL_FALSE, 3 * sizeof ( GLfloat ), userData->normals );

   // TexCoord Attribute
   glEnableVertexAttribArray ( 2 );
   glVertexAttribPointer ( 2, 2, GL_FLOAT,GL_FALSE, 2 * sizeof ( GLfloat ), userData->texCoords );




   // 绘制
   glDrawElements ( GL_TRIANGLES, userData->numIndices, GL_UNSIGNED_INT, userData->indices );

   // 禁用顶点属性数组
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
   glDisableVertexAttribArray(2);

   // 解除 UBO
   glBindBuffer(GL_UNIFORM_BUFFER, 0);

   // 关闭剔除
   //glDisable(GL_CULL_FACE);



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

   // Delete texture object
   glDeleteTextures ( 1, &userData->textureId );

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

