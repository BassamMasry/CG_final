/*
 * Copyright (c) 1993-1997, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 * Permission to use, copy, modify, and distribute this software for
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * US Government Users Restricted Rights
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(R) is a registered trademark of Silicon Graphics, Inc.
 */

/*
 * robot.c
 * This program shows how to composite modeling transformations
 * to draw translated and rotated hierarchical models.
 * Interaction:  pressing the s and e keys (shoulder and elbow)
 * alters the rotation of the robot arm.
 */
#include <GL/glut.h>
#include <stdlib.h>
#include <math.h>
// for testing purpose
#include <stdio.h>
#include "glm.h"
#include "imageloader.h"
#include "helpers.h"

// shoulder flex left and rigth, shoulder abduct left and right
static float sh_fl = 0.0f, sh_fr = 0.0f, sh_al = -90.0f, sh_ar = 90.0f;
static float elbow = 0.0f, fingerBase = 0.0f, fingerUp = 0.0f;
static float torso_f = 0.0f, upperback_f = 0.0f;
static float hip_r = 0.0f, knee_r = 0.0f, ankle_r = 0.0f, abduct_r = 0.0f;
static float hip_l = 0.0f, knee_l = 0.0f, ankle_l = 0.0f, abduct_l = 0.0f;
static double eye[3] = {0.0, 0.0, 20.0}, center[3] = {0.0,0.0,0.0}, up[3] = {0.0,1.0,0.0};;
static float hor_speed = 5.0f, ver_speed = 5.0f;
static double zoom_speed = 1.0f, move_speed = 5.0f;
static bool b_zoom = true;
//int camera_hor = 0, camera_ver = 0;
int moving, startx, starty;

// this is to compile to C++ ISO as it forbids converting char* to constant string
char path[] = "res/obj/flowers.obj";
GLMmodel* flower = glmReadOBJ(path);
char path2[] = "res/obj/bed.obj";
GLMmodel* bed = glmReadOBJ(path2);
char path3[] = "res/obj/wardrobe.obj";
GLMmodel* ward = glmReadOBJ(path3);

GLfloat light_ambient[] = { 0.1, 0.1, 0.1, 1.0 };
GLfloat light_diffuse[] = { 1.0, 1.0, 1.0,1.0 };
GLfloat light_specular[] = {0.1, 0.1, 0.1, 1.0 };
// x , y, z, w
GLfloat light_position[] = {0.0,5.0, 10.0, 1.0 };
// material properties
GLfloat mat_amb_diff[] = {0.643, 0.753, 0.934, 1.0 };
//GLfloat mat_specular[] = { 0.0, 0.0, 0.0, 1.0 };
//GLfloat shininess[] = {100.0 };

//Makes the image into a texture, and returns the id of the texture
GLuint loadTexture(Image* image) {
      GLuint textureId;
      glGenTextures(1, &textureId); //Make room for our texture
      glBindTexture(GL_TEXTURE_2D, textureId); //Tell OpenGL which texture to edit
      //Map the image to the texture
      glTexImage2D(GL_TEXTURE_2D,                //Always GL_TEXTURE_2D
                               0,                            //0 for now
                               GL_RGB,                       //Format OpenGL uses for image
                               image->width, image->height,  //Width and height
                               0,                            //The border of the image
                               GL_RGB, //GL_RGB, because pixels are stored in RGB format
                               GL_UNSIGNED_BYTE, //GL_UNSIGNED_BYTE, because pixels are stored
                                                 //as unsigned numbers
                               image->pixels);               //The actual pixel data
      return textureId; //Returns the id of the texture
}

GLuint* _texture = new GLuint; //The id of the texture
GLuint _tex_brick1; //The id of the texture
GLuint _tex_brick2; //The id of the texture
GLuint _tex_wood1; //The id of the texture
GLuint _tex_wood2; //The id of the texture



GLfloat angle = 180.0f;   /* in degrees */
GLfloat angle2 = 90.0f;   /* in degrees */

// using keyframe method
bool play = false;
bool repeat = true;

float offset_x = 0, offset_y = 0, offset_z = 0;

float anim1_sh_fl[200];
float anim1_sh_fr[200];
float anim1_hip_l[200];
float anim1_hip_r[200];

float anim2_torso[270];
float anim2_upperback[270];
float anim2_sh_fl[270];
float anim2_sh_fr[270];

float anim3_posx[200];
float anim3_posy[200];
float anim3_posz[200];
float anim3_hip[90];
float anim3_knee[90];

void reset(int x)
{
  sh_fl = 0.0f, sh_fr = 0.0f, sh_al = -90.0f, sh_ar = 90.0f;
  hip_r = 0.0f, knee_r = 0.0f, ankle_r = 0.0f, abduct_r = 0.0f;
  hip_l = 0.0f, knee_l = 0.0f, ankle_l = 0.0f, abduct_l = 0.0f;
  torso_f = 0.0f, upperback_f = 0.0f, elbow = 0.0f;
  offset_x = 0.0f, offset_y = 0.0f, offset_z = 0.0f;
}

void init_anim1(void)
{
  // init shoulders
  for (int i = 0; i < 30; ++i)
  {
    // from 70 to 40
    anim1_sh_fl[i] = 70 - i;
    // from 110 to 140
    anim1_sh_fr[i] = 110 + i;
    // from 20 to 50
    anim1_hip_l[i] = 20 + i;
    // from -20 to -50
    anim1_hip_r[i] = -20 - i;
  }
  for (int i =30; i < 60; ++i)
  {
    // from 40 to 70
    anim1_sh_fl[i] = 10 + i;
    // from 140 to 110
    anim1_sh_fr[i] = 170 - i;
    // from 50 to 20
    anim1_hip_l[i] = 80 - i;
    // from -50 to -20
    anim1_hip_r[i] = -80 + i;
  }
  for(int i = 60; i< 100; ++i)
  {
    // from 70 to 110
    anim1_sh_fl[i] = 10 + i;
    // from 110 to 70
    anim1_sh_fr[i] = 170 - i;
    // from 20 to -20
    anim1_hip_l[i] = 80 - i;
    // from -20 to 20
    anim1_hip_r[i] = -80 + i;
  }
  for(int i = 100; i< 130; ++i)
  {
    // from 110 to 140
    anim1_sh_fl[i] = 10 + i;
    // from 70 to 40
    anim1_sh_fr[i] = 170 - i;
    // from -20 to -50
    anim1_hip_l[i] = 80 - i;
    // from 20 to 50
    anim1_hip_r[i] = -80 + i;
  }
  for(int i = 130; i < 160; ++i)
  {
    // from 140 to 110
    anim1_sh_fl[i] = 270 - i;
    // from 40 to 70
    anim1_sh_fr[i] = -90 + i;
    // from -50 to -20
    anim1_hip_l[i] = -180 + i;
    // from 50 to 20
    anim1_hip_r[i] = 180 - i;
  }
  for(int i = 160; i < 200; ++i)
  {
    // from 110 to 70
    anim1_sh_fl[i] = 270 - i;
    // from 70 to 110
    anim1_sh_fr[i] = -90 + i;
    // from -20 to 20
    anim1_hip_l[i] = -180 + i;
    // from 20 to -20
    anim1_hip_r[i] = 180 - i;
  }
}

void init_anim2(void)
{
  for(int i = 0; i < 60; ++i)
  {
    // from 0 to -60
    anim2_torso[i] = -i;
    anim2_upperback[i] = -i;
    anim2_sh_fl[i] = 0;
    anim2_sh_fr[i] = 0;
  }
  for(int i = 60; i < 80; ++i)
  {
    // from -60 to -40
    anim2_torso[i] = -120 + i;
    anim2_upperback[i] = -120 + i;
    anim2_sh_fl[i] = 0;
    anim2_sh_fr[i] = 0;
  }
  for(int i = 80; i < 100; ++i)
  {
    // from -40 to -60
    anim2_torso[i] = 40 - i;
    anim2_upperback[i] = 40 - i;
    anim2_sh_fl[i] = 0;
    anim2_sh_fr[i] = 0;
  }
  for(int i = 100; i < 160; ++i)
  {
    // from -60 to 0
    anim2_torso[i] = -160 + i;
    anim2_upperback[i] = -160 + i;
    anim2_sh_fl[i] = 0;
    anim2_sh_fr[i] = 0;
  }
  for(int i = 160; i < 205; ++i)
  {
    anim2_torso[i] = 0;
    anim2_upperback[i] = 0;
    anim2_sh_fl[i] = 320 -2 * i;
    anim2_sh_fr[i] = anim2_sh_fl[i];
  }
  for(int i = 205; i < 215; ++i)
  {
    anim2_torso[i] = 0;
    anim2_upperback[i] = 0;
    anim2_sh_fl[i] = -500 + 2 * i;
    anim2_sh_fr[i] = anim2_sh_fl[i];
  }
  for(int i = 215; i < 225; ++i)
  {
    anim2_torso[i] = 0;
    anim2_upperback[i] = 0;
    anim2_sh_fl[i] = 360 -2 * i;
    anim2_sh_fr[i] = anim2_sh_fl[i];
  }
  for(int i = 225; i < 270; ++i)
  {
    anim2_torso[i] = 0;
    anim2_upperback[i] = 0;
    anim2_sh_fl[i] = -540 + 2 * i;
    anim2_sh_fr[i] = anim2_sh_fl[i];
  }
}

void init_anim3(void)
{
  for(int i = 0; i < 200; ++i)
  {
    anim3_posx[i] = -(float)i/13.3333;
    anim3_posy[i] = -(float)i/16.6666;
  }
  for(int i = 0; i < 90; ++i)
  {
    anim3_hip[i] = i;
    anim3_knee[i] = -i;
    anim3_posz[i] = -(float)i/25;
  }
}

void play_anim1(int frame)
{
  sh_fl = anim1_sh_fl[frame];
  sh_fr = anim1_sh_fr[frame];
  hip_l = anim1_hip_l[frame];
  hip_r = anim1_hip_r[frame];
}

void play_anim2(int frame)
{
  torso_f = anim2_torso[frame];
  upperback_f = anim2_upperback[frame];
  sh_fl = anim2_sh_fl[frame];
  sh_fr = anim2_sh_fr[frame];
}

void play_anim3(int frame)
{
  if (frame < 200)
  {
    offset_x = anim3_posx[frame];
    offset_y = anim3_posy[frame];
  }
  if (frame > 199)
  {
    sh_fl = 70;
    sh_fr = 70;
    hip_l = anim3_hip[frame-200];
    hip_r = anim3_hip[frame-200];
    knee_l = anim3_knee[frame-200];
    knee_r = anim3_knee[frame-200];
    offset_z = anim3_posz[frame-200];
  }
}

void Timer1(int t)
{
  play_anim1(t);
  glutPostRedisplay();
  // play at 60 frames per second
  if(play)
    if(t < 200)
      glutTimerFunc(16.66, Timer1, t+1);
    else
      if(repeat)
        Timer1(0);
}

void Timer2(int t)
{
  play_anim2(t);
  glutPostRedisplay();
  // play at 60 frames per second
  if(play)
    if(t < 270)
      glutTimerFunc(16.66, Timer2, t+1);
    else
      Timer2(0);
}

void Timer3(int t)
{
  play_anim3(t);
  glutPostRedisplay();
  // play at 60 frames per second
  if(play)
    if(t < 289)
      glutTimerFunc(16.66, Timer3, t+1);

}

void setplay(int t)
{
  if(t == 0)
    play = true;
  else
    play = false;
}

void init(void)
{
  //set background color
  glClearColor(0.94f, 0.66f, 0.54f, 1.0f);

  // make a local scope to delete these variables after operating
  {
    char path1[] = "res/img/low_res/brick1.bmp";
    Image* image1 = loadBMP(path1);
    _tex_brick1 = loadTexture(image1);
    char path2[] = "res/img/low_res/brick2.bmp";
    Image* image2 = loadBMP(path2);
    _tex_brick2 = loadTexture(image2);
    char path3[] = "res/img/low_res/wood1.bmp";
    Image* image3 = loadBMP(path3);
    _tex_wood1 = loadTexture(image3);
    char path4[] = "res/img/low_res/wood2.bmp";
    Image* image4 = loadBMP(path4);
    _tex_wood2 = loadTexture(image4);

    _texture = &_tex_brick1;
  }
  GLfloat zPlane[] = { 1.0f, 1.0f, 0.0f, 0.0f };
  glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
  glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);

  glEnable(GL_LIGHTING);
  // Flip light switch
  glEnable(GL_LIGHT0);

  glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,mat_amb_diff);

  glEnable(GL_NORMALIZE);

  glShadeModel(GL_SMOOTH);
  // Enable Depth buffer
  glEnable(GL_DEPTH_TEST);
}


void drawflower(void)
{
		glmUnitize(flower);
		glmFacetNormals(flower);
		glmVertexNormals(flower, 90.0);
		glmScale(flower, 8);
		glmDraw(flower, GLM_SMOOTH | GLM_MATERIAL);
}

void drawbed(void)
{
		glmUnitize(bed);
		glmFacetNormals(bed);
		glmVertexNormals(bed, 90.0);
		glmScale(bed, 8);
		glmDraw(bed, GLM_SMOOTH | GLM_MATERIAL);
}

void drawward(void)
{
		glmUnitize(ward);
		glmFacetNormals(ward);
		glmVertexNormals(ward, 90.0);
		glmScale(ward, 8);
		glmDraw(ward, GLM_SMOOTH | GLM_MATERIAL);
}


void drawfloor(void)
{
  glPushMatrix();

  glEnable(GL_TEXTURE_2D);

  glEnable(GL_TEXTURE_GEN_S); //enable texture coordinate generation
  glEnable(GL_TEXTURE_GEN_T);
  glBindTexture(GL_TEXTURE_2D, *_texture);



  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glRotatef(90.0f, 1.0f,0.0f,0.0f);
  glScalef(50.0f, 50.0f, 0.5f);
  glTranslatef(0.0f,0.0f,20.0f);
  glutSolidCube(1.0f);
  glDisable(GL_TEXTURE_GEN_S); //enable texture coordinate generation
  glDisable(GL_TEXTURE_GEN_T);
  glDisable(GL_TEXTURE_2D);
  glPopMatrix();
}

// make lower back (torso)
// the torso the parent of hierarchy of all bones
void torso(void)
{
  // any rotation and translation should be made here in this line
  glPushMatrix();
  glColor3ub (0, 255, 0);
  glTranslatef (0.0f, 0.0f, 2.0f);
  glScalef(4.0f, 1.0f, 3.0f);
  glutSolidCube(1.0f);

  glPopMatrix();
}

// make upper back
// the upper back is the parent of arms and head
void upperback(void)
{
  glTranslatef(0.0f, 0.0f, 3.0f);
  glRotatef ((GLfloat) upperback_f, 1.0f,0.0f, 0.0f);
  glTranslatef (0.0f, 0.0f, 2.0f);
  glPushMatrix();
  glColor3ub (0, 255, 0);
  glScalef(4.0f, 1.0f, 3.0f);
  glutSolidCube(1.0f);
  glPopMatrix();
}

void head(void)
{
  glPushMatrix();
  // skin color
  glColor3ub (80, 73, 60);
  glTranslatef(0.0f, 0.0f, 3.5f);
  glutSolidSphere(1.0, 20,20);
  glPopMatrix();
}

// make shoulders
// the shoulder is the parent of elbows
void shoulders(GLfloat scale = 1.0f)
{
  float* abd;
  float* flex;
  if (scale == 1.0f)
  {
    flex = &sh_fr;
    abd = &sh_ar;
  }else
  {
    flex = &sh_fl;
    abd = &sh_al;
  }
  glTranslatef (scale * 2.3f, 0.0f, 1.0f);
  glRotatef ((GLfloat) *abd, 0.0f,0.0f, 1.0f);
  glRotatef ((GLfloat) scale * (*flex), 0.0f, 1.0f, 0.0f);

  glTranslatef (scale * 1.2f, 0.0f, 0.0f);
  glPushMatrix();
  glScalef (3.0f, 0.6f, 1.0f);
  glColor3ub (0, 255, 0);
  glutSolidCube (1.0f);
  glPopMatrix();
}

// make elbows
// the elbow is the parent of hand and fingers
void elbows(GLfloat scale = 1.0f)
{
  glTranslatef (scale * 1.5f, 0.0f, 0.0f);
  glRotatef ((GLfloat) scale * elbow, 0.0f, 1.0f, 0.0f);
  glTranslatef (scale * 1.5f, 0.0f, 0.0f);
  glPushMatrix();
  glScalef (3.0f, 0.6f, 1.0f);
  glColor3ub (0, 255, 0);
  glutSolidCube (1.0f);
  glPopMatrix();
}


// my own implemented function to avoid repeating
void Flang(GLfloat offset, GLfloat scale = 1.0f)
{
  //Draw finger flang 1
  // added this line
  glPushMatrix();
  glTranslatef(scale * 1.5f, -0.25f, 0.0f);
  glRotatef((GLfloat) scale * fingerBase, 0.0f, 0.0f, 1.0f);
  glTranslatef(scale * 0.15, 0.0f, offset);
  glPushMatrix();
  glScalef(0.3, 0.1, 0.1);
  // skin color
  glColor3ub (80, 73, 60);
  glutSolidCube(1.0f);
  glPopMatrix();


  //Draw finger flang 1
  glTranslatef(scale * 0.15, 0.0f, 0.0f);
  glRotatef((GLfloat)scale * fingerUp, 0.0f, 0.0f, 1.0f);
  glTranslatef(scale * 0.15, 0.0f, 0.0f);
  glPushMatrix();
  glScalef(0.3, 0.1, 0.1);
  glutSolidCube(1.0f);
  // added this line
  glPopMatrix();
  glPopMatrix();
}

void Thumb(GLfloat scale = 1.0f)
{
  //Draw finger flang 1
  // added this line
  glPushMatrix();
  glTranslatef(scale * 1.5f, 0.0f, 0.5f);
  glRotatef((GLfloat)scale * fingerBase, 0.0f, 1.0f, 0.0f);
  glTranslatef(scale * 0.15, 0.0f, 0.0f);
  glPushMatrix();
  glScalef(0.3, 0.1, 0.1);
  // skin color
  glColor3ub (80, 73, 60);
  glutSolidCube(1.0f);
  glPopMatrix();

  //Draw finger flang 1
  glTranslatef(scale * 0.15, 0.0f, 0.0f);
  glRotatef((GLfloat)scale * fingerUp, 0.0f, 1.0f, 0.0f);
  glTranslatef(scale * 0.15, 0.0f, 0.0f);
  glPushMatrix();
  glScalef(0.3, 0.1, 0.1);
  glutSolidCube(1.0f);
  // added this line
  glPopMatrix();
  glPopMatrix();
}

// make hips
// the hip is parent to knee and foot
void hips(bool right = true)
{
  GLfloat scale;
  float* hip;
  float* abduct;
  if (right)
  {
    scale = 1.0f;
    hip = &hip_r;
    abduct = &abduct_r;
  }else
  {
    scale = -1.0f;
    hip = &hip_l;
    abduct = &abduct_l;
  }
  glTranslatef (scale * 1.25f, 0.0f, -1.5f);
  glRotatef ((GLfloat) *hip, 1.0f,0.0f, 0.0f);
  glRotatef ((GLfloat) *abduct, 0.0f,1.0f, 0.0f);
  glTranslatef (0.0f, 0.0f, -2.0f);
  glPushMatrix();
  glScalef (1.5f, 1.0f, 4.0f);
  // Jeans color
  glColor3ub (0, 0, 255);
  glutSolidCube (1.0f);
  glPopMatrix();
}

// make knee
// knee is the parent to foot
void knees(bool right = true)
{
  float* knee;
  if (right)
  {
    knee = &knee_r;
  }else
  {
    knee = &knee_l;
  }
  glTranslatef (0.0f, 0.0f, -2.0f);
  glRotatef ((GLfloat) *knee, 1.0f, 0.0f, 0.0f);
  glTranslatef (0.0f, 0.0f, -2.0f);
  glPushMatrix();
  glScalef (1.5f, 1.0f, 4.0f);
  // Jeans color
  glColor3ub (0, 0, 255);
  glutSolidCube (1.0f);
  glPopMatrix();
}

// make ankles
void ankles(bool right = true)
{
  GLfloat scale;
  float* ankle;
  if (right)
  {
    ankle = &ankle_r;
  }else
  {
    ankle = &ankle_l;
  }
  glTranslatef (0.0f, 0.0f, -2.0f);
  glRotatef ((GLfloat) *ankle, 1.0f, 0.0f, 0.0f);
  glTranslatef (0.0f, 1.0f, 0.0f);
  glPushMatrix();
  glScalef (1.5f, 3.0f, 1.0f);
  // Shoes color
  glColor3ub (28, 21, 7);
  glutSolidCube (1.0f);
  glPopMatrix();
}

void pan(float hor_speed)
{
  rotatePoint(up,hor_speed, eye);
}

void pinch(float ver_speed)
{
  // due to cross product problem
  // I had to invert up vector when pinching more than 180 degrees
  static double rot_axis[3];
  static double old_rot[3];
  crossProduct(eye, up, rot_axis);
  normalize(rot_axis);
  if (rot_axis[0] == -old_rot[0])
  {
    up[0] = -up[0];
    up[1] = -up[1];
    up[2] = -up[2];
  }
  old_rot[0] = rot_axis[0];
  old_rot[1] = rot_axis[1];
  old_rot[2] = rot_axis[2];
  rotatePoint(rot_axis,ver_speed, eye);
}

void zoom(double speed)
{
  double look[3];
  look[0] = center[0] - eye[0];
  look[1] = center[1] - eye[1];
  look[2] = center[2] - eye[2];
  normalize(look);
  eye[0] += look[0] * speed;
  eye[1] += look[1] * speed;
  eye[2] += look[2] * speed;
}

void move(double speed)
{
  double look[3];
  look[0] = center[0] - eye[0];
  look[1] = center[1] - eye[1];
  look[2] = center[2] - eye[2];
  normalize(look);
  eye[0] += look[0] * speed;
  eye[1] += look[1] * speed;
  eye[2] += look[2] * speed;

  center[0] += look[0] * speed;
  center[1] += look[1] * speed;
  center[2] += look[2] * speed;

}
void display(void)
{
   glClear(GL_COLOR_BUFFER_BIT);
   glClear(GL_DEPTH_BUFFER_BIT);
   glPushMatrix();
   gluLookAt(eye[0],eye[1],eye[2],center[0],center[1],center[2],up[0],up[1],up[2]);
   glRotatef(angle2, 1.0f, 0.0f, 0.0f);
   glRotatef(angle, 0.0f, 1.0f, 0.0f);

   glLightfv(GL_LIGHT0, GL_POSITION, light_position);
   //materials properties
   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,mat_amb_diff);

   //start making environment
   glColor3ub(255,255,255);
   // imported objects block
   glPushMatrix();
   glRotatef(90,1.0f,0.0f,0.0f);
   // make floor
   drawfloor();
   // draw flowers
   glPushMatrix();
   glTranslatef(20.0f, -2.0f, 0.0f);
   glRotatef(180,0.0f,1.0f,0.0f);
   drawflower();
   glPopMatrix();

   // draw bed`
   glPushMatrix();
   glTranslatef(-15.0f, -4.5f, 15.0f);
   drawbed();
   glPopMatrix();

   // draw wardrobe
   glPushMatrix();
   glTranslatef(20.0f, -2.0f, 15.0f);
   glRotatef(90,0.0f,1.0f,0.0f);
   drawward();
   glPopMatrix();

   glPopMatrix();

   glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
   glEnable(GL_COLOR_MATERIAL);


   // start of making body

   // global moving
   glTranslatef(offset_x,offset_y,offset_z);

   // make a new hierarchy for upper body
   // make torso
   glPushMatrix();

   glTranslatef (0.0f, 0.0f, -2.0f);
   glRotatef ((GLfloat) torso_f, 1.0f,0.0f, 0.0f);
   torso();
   glTranslatef (0.0f, 0.0f, 2.0f);

   // make upper back
   glTranslatef (0.0f, 0.0f, -2.0f);
   upperback();
   //make head
   head();
   // make a new hierarchy for right side
   glPushMatrix();
   // make shoulders
   shoulders(1.0f);
   // make elbows
   elbows(1.0f);
   // make each finger in its place
   Flang(-0.45f);
   Flang(-0.15f);
   Flang(0.15f);
   Flang(0.45f);
   Thumb();
   // end of right side hierarchy
   glPopMatrix();

   // make a new hierarchy for left side
   glPushMatrix();
   // make shoulders
   shoulders(-1.0f);
   // make elbows
   elbows(-1.0f);
   // make each finger in its place
   Flang(-0.45f, -1.0f);
   Flang(-0.15f, -1.0f);
   Flang(0.15f, -1.0f);
   Flang(0.45f, -1.0f);
   Thumb(-1.0f);
   // end of left side hierarchy
   glPopMatrix();
   // end of upper body hierarchy
   glPopMatrix();

   // make a new hierarchy for lower body
   glPushMatrix();
   // make a new hierarchy for right side
   glPushMatrix();
   hips();
   knees();
   ankles();
   // end of right side hierarcy
   glPopMatrix();

   // make a new hierarchy for left side
   glPushMatrix();
   hips(false);
   knees(false);
   ankles(false);
   //end of left side hierarcy
   glPopMatrix();

   // end of lower body hierarchy
   glPopMatrix();


   glPopMatrix();
   glutPostRedisplay();
   glutSwapBuffers();
}

void reshape(int w, int h)
{
   glViewport(0, 0, (GLsizei)w, (GLsizei)h);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(120.0f, (GLfloat)w / (GLfloat)h, 0.5f, 50.0f);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   // this was from previous code
   //glTranslatef(0.0f, 0.0f, -5.0f);
}

void keyboard(unsigned char key, int x, int y)
{
   switch (key)
   {
     case '1':
      play = false;
      reset(0);
      glutTimerFunc(16,reset,0);
      glutTimerFunc(16,setplay,0);
      repeat = true;
      glutTimerFunc(16,Timer1,0);
      break;

     case '2':
     play = false;
     reset(0);
     glutTimerFunc(16,reset,0);
     glutTimerFunc(16,setplay,0);
     glutTimerFunc(16,Timer2,0);
     break;

     case '3':
     play = false;
     reset(0);
     glutTimerFunc(16,reset,0);
     glutTimerFunc(16,setplay,0);
     repeat = false;
     glutTimerFunc(16,Timer1,0);
     glutTimerFunc(16,Timer3,0);
     break;

     case 'r':
      play = false;
      glutTimerFunc(16,reset,0);
      break;

      /*

   case 's':
      sh_fl += 5;
      if (sh_fl > 90)
  		{
  			sh_fl = 90;
  		}
      break;
   case 'S':
      sh_fl -= 5;
      if (sh_fl < -90)
  		{
  			sh_fl = -90;
  		}
      break;
   case 'e':
      elbow += 5;
      if (elbow  >  0)
    	{
    		elbow =  0;
    	}
      break;
   case 'E':
      elbow -= 5;
      if (elbow < -120)
    	{
    		elbow = -120;
    	}
      break;
   case 'f':
      fingerBase += 5;
      if (fingerBase > 120)
      {
        fingerBase = 120;
      }
      break;
   case 'F':
      fingerBase -= 5;
      if (fingerBase < 0)
      {
        fingerBase = 0;
      }
      break;
    case 'g':
      fingerUp += 5;
      if (fingerUp > 120)
      {
        fingerUp = 120;
      }
      break;
   case 'G':
      fingerUp -= 5;
      if (fingerUp < 0)
      {
        fingerUp = 0;
      }
      break;
    case 'h':
       hip_r += 5;
       if (hip_r > 120)
       {
         hip_r = 120;
       }
       break;
    case 'H':
       hip_r -= 5;
       if (hip_r < -90)
       {
         hip_r = -90;
       }
       break;
     case 'j':
        hip_l += 5;
        if (hip_l > 120)
        {
          hip_l = 120;
        }
        break;
     case 'J':
        hip_l -= 5;
        if (hip_l < -90)
        {
          hip_l = -90;
        }
       break;
     case 'a':
        abduct_r += 5;
        if (abduct_r > 120)
        {
          abduct_r = 120;
        }
        break;
     case 'A':
        abduct_r -= 5;
        if (abduct_r < -90)
        {
          abduct_r = -90;
        }
        break;
      case 'q':
         abduct_l += 5;
         if (abduct_l > 120)
         {
           abduct_l = 120;
         }
         break;
      case 'Q':
         abduct_l -= 5;
         if (abduct_l < -90)
         {
           abduct_l = -90;
         }
         break;
       case 'k':
          knee_r += 5;
          if (knee_r > 5)
          {
            knee_r = 5;
          }
          break;
       case 'K':
          knee_r -= 5;
          if (knee_r < -120)
          {
            knee_r = -120;
          }
          break;
        case 'l':
           knee_l += 5;
           if (knee_l > 5)
           {
             knee_l = 5;
           }
           break;
        case 'L':
           knee_l -= 5;
           if (knee_l < -120)
           {
             knee_l = -120;
           }
          break;

        */
        case 'p':
           b_zoom = !b_zoom;
          break;

   case 27:
      exit(0);
      break;
   default:
      break;
   }
   glutPostRedisplay();
}

void keySpecial(int key, int x, int y)
{
   switch (key)
   {
     case GLUT_KEY_LEFT:
     if (b_zoom)
     {
       zoom(zoom_speed);
     }else
     {
       pan(hor_speed);
     }
     break;

     case GLUT_KEY_RIGHT:
     if (b_zoom)
     {
       zoom(-zoom_speed);
     }else
     {
       pan(-hor_speed);
     }
     break;

     default:
     break;

     case GLUT_KEY_UP:
     if (b_zoom)
     {
       move(move_speed);
     }else
     {
       pinch(ver_speed);
     }
     break;

     case GLUT_KEY_DOWN:
     if (b_zoom)
     {
       move(-move_speed);
     }else
     {
       pinch(-ver_speed);
     }
     break;
   }
   glutPostRedisplay();
}

static void mouse(int button, int state, int x, int y)
{
  if (button == GLUT_LEFT_BUTTON) {
    if (state == GLUT_DOWN) {
      moving = 1;
      startx = x;
      starty = y;
    }
    if (state == GLUT_UP) {
      moving = 0;
    }
  }
  glutPostRedisplay();
}


static void motion(int x, int y)
{
  if (moving) {
    angle = angle + (x - startx);
    angle2 = angle2 + (y - starty);
    startx = x;
    starty = y;
    //glutPostRedisplay();
  }
  glutPostRedisplay();
}

void Textures_menu(int value)
{
  switch (value) {
    case 1:
      _texture = &_tex_brick1;
      break;
    case 2:
      _texture = &_tex_brick2;
      break;
    case 3:
      _texture = &_tex_wood1;
      break;
    case 4:
      _texture = &_tex_wood2;
      break;
    default:
    break;
  }
}

int main(int argc, char **argv)
{
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
   glutInitWindowSize(500, 500);
   glutInitWindowPosition(100, 100);
   glutCreateWindow("My room");
   init_anim1();
   init_anim2();
   init_anim3();
   init();
   glutMouseFunc(mouse);
   glutMotionFunc(motion);
   glutReshapeFunc(reshape);
   glutDisplayFunc(display);
   glutKeyboardFunc(keyboard);
   glutSpecialFunc(keySpecial);

   glutCreateMenu(Textures_menu);
   glutAddMenuEntry("Textures", 0);
   glutAddMenuEntry("", 0);
   glutAddMenuEntry("Brick1", 1);
   glutAddMenuEntry("Brick2", 2);
   glutAddMenuEntry("Wood1", 3);
   glutAddMenuEntry("Wood2", 4);
   glutAttachMenu(GLUT_RIGHT_BUTTON);

   glutMainLoop();
   return 0;
}
