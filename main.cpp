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

static float shoulder = 0.0f, elbow = 0.0f, fingerBase = 0.0f, fingerUp = 0.0f;
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
  glScalef(4.0f, 1.0f, 3.0f);
  glutSolidCube(1.0f);

  glPopMatrix();
}

// make upper back
// the upper back is the parent of arms and head
void upperback(void)
{
  glTranslatef(0.0f, 0.0f, 3.0f);
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
  glTranslatef (scale * 2.3f, 0.0f, 2.0f);
  glRotatef ((GLfloat) scale * shoulder, 0.0f, 1.0f, 0.0f);
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

   glPopMatrix();

   glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
   glEnable(GL_COLOR_MATERIAL);


   // start of making body
   // make torso
   torso();

   // make a new hierarchy for upper body
   glPushMatrix();
   // make upper back
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
   case 's':
      shoulder += 5;
      if (shoulder > 90)
  		{
  			shoulder = 90;
  		}
      break;
   case 'S':
      shoulder -= 5;
      if (shoulder < -90)
  		{
  			shoulder = -90;
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
     case GLUT_KEY_UP:
     if (b_zoom)
     {
       zoom(zoom_speed);
     }else
     {
       pan(hor_speed);
     }
     break;

     case GLUT_KEY_DOWN:
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

     case GLUT_KEY_LEFT:
     if (b_zoom)
     {
       move(move_speed);
     }else
     {
       pinch(ver_speed);
     }
     break;

     case GLUT_KEY_RIGHT:
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
