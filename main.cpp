
#if defined(__APPLE__) || defined(MACOSX)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <stdlib.h>
#include <math.h>

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fstream>

// glm.h

/*
      glm.h
      Nate Robins, 1997, 2000
      nate@pobox.com, http://www.pobox.com/~nate

      Wavefront OBJ model file format reader/writer/manipulator.

      Includes routines for generating smooth normals with
      preservation of edges, welding redundant vertices & texture
      coordinate generation (spheremap and planar projections) + more.

 */

#ifndef M_PI
#define M_PI 3.14159265f
#endif

#define GLM_NONE     (0)            /* render with only vertices */
#define GLM_FLAT     (1 << 0)       /* render with facet normals */
#define GLM_SMOOTH   (1 << 1)       /* render with vertex normals */
#define GLM_TEXTURE  (1 << 2)       /* render with texture coords */
#define GLM_COLOR    (1 << 3)       /* render with colors */
#define GLM_MATERIAL (1 << 4)       /* render with materials */


/* GLMmaterial: Structure that defines a material in a model.
 */
typedef struct _GLMmaterial
{
  char*   name;                 /* name of material */
  GLfloat diffuse[4];           /* diffuse component */
  GLfloat ambient[4];           /* ambient component */
  GLfloat specular[4];          /* specular component */
  GLfloat emmissive[4];         /* emmissive component */
  GLfloat shininess;            /* specular exponent */
} GLMmaterial;

/* GLMtriangle: Structure that defines a triangle in a model.
 */
typedef struct _GLMtriangle {
  GLuint vindices[3];           /* array of triangle vertex indices */
  GLuint nindices[3];           /* array of triangle normal indices */
  GLuint tindices[3];           /* array of triangle texcoord indices*/
  GLuint findex;                /* index of triangle facet normal */
} GLMtriangle;

/* GLMgroup: Structure that defines a group in a model.
 */
typedef struct _GLMgroup {
  char*             name;           /* name of this group */
  GLuint            numtriangles;   /* number of triangles in this group */
  GLuint*           triangles;      /* array of triangle indices */
  GLuint            material;       /* index to material for group */
  struct _GLMgroup* next;           /* pointer to next group in model */
} GLMgroup;

/* GLMmodel: Structure that defines a model.
 */
typedef struct _GLMmodel {
  char*    pathname;            /* path to this model */
  char*    mtllibname;          /* name of the material library */

  GLuint   numvertices;         /* number of vertices in model */
  GLfloat* vertices;            /* array of vertices  */

  GLuint   numnormals;          /* number of normals in model */
  GLfloat* normals;             /* array of normals */

  GLuint   numtexcoords;        /* number of texcoords in model */
  GLfloat* texcoords;           /* array of texture coordinates */

  GLuint   numfacetnorms;       /* number of facetnorms in model */
  GLfloat* facetnorms;          /* array of facetnorms */

  GLuint       numtriangles;    /* number of triangles in model */
  GLMtriangle* triangles;       /* array of triangles */

  GLuint       nummaterials;    /* number of materials in model */
  GLMmaterial* materials;       /* array of materials */

  GLuint       numgroups;       /* number of groups in model */
  GLMgroup*    groups;          /* linked list of groups */

  GLfloat position[3];          /* position of the model */

} GLMmodel;


GLfloat
glmUnitize(GLMmodel* model);

GLvoid
glmDimensions(GLMmodel* model, GLfloat* dimensions);

GLvoid
glmScale(GLMmodel* model, GLfloat scale);

GLvoid
glmReverseWinding(GLMmodel* model);

GLvoid
glmFacetNormals(GLMmodel* model);

GLvoid
glmVertexNormals(GLMmodel* model, GLfloat angle);

GLvoid
glmLinearTexture(GLMmodel* model);

GLvoid
glmSpheremapTexture(GLMmodel* model);

GLvoid
glmDelete(GLMmodel* model);

GLMmodel*
glmReadOBJ(char* filename);

GLvoid
glmWriteOBJ(GLMmodel* model, char* filename, GLuint mode);

GLvoid
glmDraw(GLMmodel* model, GLuint mode);

GLuint
glmList(GLMmodel* model, GLuint mode);

GLvoid
glmWeld(GLMmodel* model, GLfloat epsilon);

GLubyte*
glmReadPPM(char* filename, int* width, int* height);

// glm.cpp

#define T(x) (model->triangles[(x)])


/* _GLMnode: general purpose node */
typedef struct _GLMnode {
    GLuint index;
    GLboolean averaged;
    struct _GLMnode* next;
} GLMnode;


/* glmMax: returns the maximum of two floats */
static GLfloat
glmMax(GLfloat a, GLfloat b)
{
    if (b > a)
        return b;
    return a;
}

/* glmAbs: returns the absolute value of a float */
static GLfloat
glmAbs(GLfloat f)
{
    if (f < 0)
        return -f;
    return f;
}

/* glmDot: compute the dot product of two vectors
 *
 * u - array of 3 GLfloats (GLfloat u[3])
 * v - array of 3 GLfloats (GLfloat v[3])
 */
static GLfloat
glmDot(GLfloat* u, GLfloat* v)
{
    assert(u); assert(v);

    return u[0]*v[0] + u[1]*v[1] + u[2]*v[2];
}

/* glmCross: compute the cross product of two vectors
 *
 * u - array of 3 GLfloats (GLfloat u[3])
 * v - array of 3 GLfloats (GLfloat v[3])
 * n - array of 3 GLfloats (GLfloat n[3]) to return the cross product in
 */
static GLvoid
glmCross(GLfloat* u, GLfloat* v, GLfloat* n)
{
    assert(u); assert(v); assert(n);

    n[0] = u[1]*v[2] - u[2]*v[1];
    n[1] = u[2]*v[0] - u[0]*v[2];
    n[2] = u[0]*v[1] - u[1]*v[0];
}

/* glmNormalize: normalize a vector
 *
 * v - array of 3 GLfloats (GLfloat v[3]) to be normalized
 */
static GLvoid
glmNormalize(GLfloat* v)
{
    GLfloat l;

    assert(v);

    l = (GLfloat)sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    v[0] /= l;
    v[1] /= l;
    v[2] /= l;
}

/* glmEqual: compares two vectors and returns GL_TRUE if they are
 * equal (within a certain threshold) or GL_FALSE if not. An epsilon
 * that works fairly well is 0.000001.
 *
 * u - array of 3 GLfloats (GLfloat u[3])
 * v - array of 3 GLfloats (GLfloat v[3])
 */
static GLboolean
glmEqual(GLfloat* u, GLfloat* v, GLfloat epsilon)
{
    if (glmAbs(u[0] - v[0]) < epsilon &&
        glmAbs(u[1] - v[1]) < epsilon &&
        glmAbs(u[2] - v[2]) < epsilon)
    {
        return GL_TRUE;
    }
    return GL_FALSE;
}

/* glmWeldVectors: eliminate (weld) vectors that are within an
 * epsilon of each other.
 *
 * vectors     - array of GLfloat[3]'s to be welded
 * numvectors - number of GLfloat[3]'s in vectors
 * epsilon     - maximum difference between vectors
 *
 */
GLfloat*
glmWeldVectors(GLfloat* vectors, GLuint* numvectors, GLfloat epsilon)
{
    GLfloat* copies;
    GLuint copied;
    GLuint i, j;

    copies = (GLfloat*)malloc(sizeof(GLfloat) * 3 * (*numvectors + 1));
    memcpy(copies, vectors, (sizeof(GLfloat) * 3 * (*numvectors + 1)));

    copied = 1;
    for (i = 1; i <= *numvectors; i++) {
        for (j = 1; j <= copied; j++) {
            if (glmEqual(&vectors[3 * i], &copies[3 * j], epsilon)) {
                goto duplicate;
            }
        }

        /* must not be any duplicates -- add to the copies array */
        copies[3 * copied + 0] = vectors[3 * i + 0];
        copies[3 * copied + 1] = vectors[3 * i + 1];
        copies[3 * copied + 2] = vectors[3 * i + 2];
        j = copied;             /* pass this along for below */
        copied++;

duplicate:
        /* set the first component of this vector to point at the correct
        index into the new copies array */
        vectors[3 * i + 0] = (GLfloat)j;
    }

    *numvectors = copied-1;
    return copies;
}

/* glmFindGroup: Find a group in the model */
GLMgroup*
glmFindGroup(GLMmodel* model, char* name)
{
    GLMgroup* group;

    assert(model);

    group = model->groups;
    while(group) {
        if (!strcmp(name, group->name))
            break;
        group = group->next;
    }

    return group;
}

/* glmAddGroup: Add a group to the model */
GLMgroup*
glmAddGroup(GLMmodel* model, char* name)
{
    GLMgroup* group;

    group = glmFindGroup(model, name);
    if (!group) {
        group = (GLMgroup*)malloc(sizeof(GLMgroup));
        group->name = strdup(name);
        group->material = 0;
        group->numtriangles = 0;
        group->triangles = NULL;
        group->next = model->groups;
        model->groups = group;
        model->numgroups++;
    }

    return group;
}

/* glmFindGroup: Find a material in the model */
GLuint
glmFindMaterial(GLMmodel* model, char* name)
{
    GLuint i;

    /* XXX doing a linear search on a string key'd list is pretty lame,
    but it works and is fast enough for now. */
    for (i = 0; i < model->nummaterials; i++) {
        if (!strcmp(model->materials[i].name, name))
            goto found;
    }

    /* didn't find the name, so print a warning and return the default
    material (0). */
    printf("glmFindMaterial():  can't find material \"%s\".\n", name);
    i = 0;

found:
    return i;
}


/* glmDirName: return the directory given a path
 *
 * path - filesystem path
 *
 * NOTE: the return value should be free'd.
 */
static char*
glmDirName(char* path)
{
    char* dir;
    char* s;

    dir = strdup(path);

    s = strrchr(dir, '/');
    if (s)
        s[1] = '\0';
    else
        dir[0] = '\0';

    return dir;
}


/* glmReadMTL: read a wavefront material library file
 *
 * model - properly initialized GLMmodel structure
 * name  - name of the material library
 */
static GLvoid
glmReadMTL(GLMmodel* model, char* name)
{
    FILE* file;
    char* dir;
    char* filename;
    char buf[128];
    GLuint nummaterials, i;

    dir = glmDirName(model->pathname);
    filename = (char*)malloc(sizeof(char) * (strlen(dir) + strlen(name) + 1));
    strcpy(filename, dir);
    strcat(filename, name);
    free(dir);

    file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "glmReadMTL() failed: can't open material file \"%s\".\n",
            filename);
        exit(1);
    }
    free(filename);

    /* count the number of materials in the file */
    nummaterials = 1;
    while(fscanf(file, "%s", buf) != EOF) {
        switch(buf[0]) {
        case '#':               /* comment */
            /* eat up rest of line */
            fgets(buf, sizeof(buf), file);
            break;
        case 'n':               /* newmtl */
            fgets(buf, sizeof(buf), file);
            nummaterials++;
            sscanf(buf, "%s %s", buf, buf);
            break;
        default:
            /* eat up rest of line */
            fgets(buf, sizeof(buf), file);
            break;
        }
    }

    rewind(file);

    model->materials = (GLMmaterial*)malloc(sizeof(GLMmaterial) * nummaterials);
    model->nummaterials = nummaterials;

    /* set the default material */
    for (i = 0; i < nummaterials; i++) {
        model->materials[i].name = NULL;
        model->materials[i].shininess = 65.0;
        model->materials[i].diffuse[0] = 0.8;
        model->materials[i].diffuse[1] = 0.8;
        model->materials[i].diffuse[2] = 0.8;
        model->materials[i].diffuse[3] = 1.0;
        model->materials[i].ambient[0] = 0.2;
        model->materials[i].ambient[1] = 0.2;
        model->materials[i].ambient[2] = 0.2;
        model->materials[i].ambient[3] = 1.0;
        model->materials[i].specular[0] = 0.0;
        model->materials[i].specular[1] = 0.0;
        model->materials[i].specular[2] = 0.0;
        model->materials[i].specular[3] = 1.0;
    }
    model->materials[0].name = strdup("default");

    /* now, read in the data */
    nummaterials = 0;
    while(fscanf(file, "%s", buf) != EOF) {
        switch(buf[0]) {
        case '#':               /* comment */
            /* eat up rest of line */
            fgets(buf, sizeof(buf), file);
            break;
        case 'n':               /* newmtl */
            fgets(buf, sizeof(buf), file);
            sscanf(buf, "%s %s", buf, buf);
            nummaterials++;
            model->materials[nummaterials].name = strdup(buf);
            break;
        case 'N':
            fscanf(file, "%f", &model->materials[nummaterials].shininess);
            /* wavefront shininess is from [0, 1000], so scale for OpenGL */
            model->materials[nummaterials].shininess /= 1000.0;
            model->materials[nummaterials].shininess *= 128.0;
            break;
        case 'K':
            switch(buf[1]) {
            case 'd':
                fscanf(file, "%f %f %f",
                    &model->materials[nummaterials].diffuse[0],
                    &model->materials[nummaterials].diffuse[1],
                    &model->materials[nummaterials].diffuse[2]);
                break;
            case 's':
                fscanf(file, "%f %f %f",
                    &model->materials[nummaterials].specular[0],
                    &model->materials[nummaterials].specular[1],
                    &model->materials[nummaterials].specular[2]);
                break;
            case 'a':
                fscanf(file, "%f %f %f",
                    &model->materials[nummaterials].ambient[0],
                    &model->materials[nummaterials].ambient[1],
                    &model->materials[nummaterials].ambient[2]);
                break;
            default:
                /* eat up rest of line */
                fgets(buf, sizeof(buf), file);
                break;
            }
            break;
            default:
                /* eat up rest of line */
                fgets(buf, sizeof(buf), file);
                break;
        }
    }
}

/* glmWriteMTL: write a wavefront material library file
 *
 * model   - properly initialized GLMmodel structure
 * modelpath  - pathname of the model being written
 * mtllibname - name of the material library to be written
 */
static GLvoid
glmWriteMTL(GLMmodel* model, char* modelpath, char* mtllibname)
{
    FILE* file;
    char* dir;
    char* filename;
    GLMmaterial* material;
    GLuint i;

    dir = glmDirName(modelpath);
    filename = (char*)malloc(sizeof(char) * (strlen(dir)+strlen(mtllibname)));
    strcpy(filename, dir);
    strcat(filename, mtllibname);
    free(dir);

    /* open the file */
    file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "glmWriteMTL() failed: can't open file \"%s\".\n",
            filename);
        exit(1);
    }
    free(filename);

    /* spit out a header */
    fprintf(file, "#  \n");
    fprintf(file, "#  Wavefront MTL generated by GLM library\n");
    fprintf(file, "#  \n");
    fprintf(file, "#  GLM library\n");
    fprintf(file, "#  Nate Robins\n");
    fprintf(file, "#  ndr@pobox.com\n");
    fprintf(file, "#  http://www.pobox.com/~ndr\n");
    fprintf(file, "#  \n\n");

    for (i = 0; i < model->nummaterials; i++) {
        material = &model->materials[i];
        fprintf(file, "newmtl %s\n", material->name);
        fprintf(file, "Ka %f %f %f\n",
            material->ambient[0], material->ambient[1], material->ambient[2]);
        fprintf(file, "Kd %f %f %f\n",
            material->diffuse[0], material->diffuse[1], material->diffuse[2]);
        fprintf(file, "Ks %f %f %f\n",
            material->specular[0],material->specular[1],material->specular[2]);
        fprintf(file, "Ns %f\n", material->shininess / 128.0 * 1000.0);
        fprintf(file, "\n");
    }
}


/* glmFirstPass: first pass at a Wavefront OBJ file that gets all the
 * statistics of the model (such as #vertices, #normals, etc)
 *
 * model - properly initialized GLMmodel structure
 * file  - (fopen'd) file descriptor
 */
static GLvoid
glmFirstPass(GLMmodel* model, FILE* file)
{
    GLuint numvertices;        /* number of vertices in model */
    GLuint numnormals;         /* number of normals in model */
    GLuint numtexcoords;       /* number of texcoords in model */
    GLuint numtriangles;       /* number of triangles in model */
    GLMgroup* group;           /* current group */
    int v, n, t;
    char buf[128];

    /* make a default group */
    char mode[] = "default";
    group = glmAddGroup(model, mode);

    numvertices = numnormals = numtexcoords = numtriangles = 0;
    while(fscanf(file, "%s", buf) != EOF) {
        switch(buf[0]) {
        case '#':               /* comment */
            /* eat up rest of line */
            fgets(buf, sizeof(buf), file);
            break;
        case 'v':               /* v, vn, vt */
            switch(buf[1]) {
            case '\0':          /* vertex */
                /* eat up rest of line */
                fgets(buf, sizeof(buf), file);
                numvertices++;
                break;
            case 'n':           /* normal */
                /* eat up rest of line */
                fgets(buf, sizeof(buf), file);
                numnormals++;
                break;
            case 't':           /* texcoord */
                /* eat up rest of line */
                fgets(buf, sizeof(buf), file);
                numtexcoords++;
                break;
            default:
                printf("glmFirstPass(): Unknown token \"%s\".\n", buf);
                exit(1);
                break;
            }
            break;
            case 'm':
                fgets(buf, sizeof(buf), file);
                sscanf(buf, "%s %s", buf, buf);
                model->mtllibname = strdup(buf);
                glmReadMTL(model, buf);
                break;
            case 'u':
                /* eat up rest of line */
                fgets(buf, sizeof(buf), file);
                break;
            case 'g':               /* group */
                /* eat up rest of line */
                fgets(buf, sizeof(buf), file);
#if SINGLE_STRING_GROUP_NAMES
                sscanf(buf, "%s", buf);
#else
                buf[strlen(buf)-1] = '\0';  /* nuke '\n' */
#endif
                group = glmAddGroup(model, buf);
                break;
            case 'f':               /* face */
                v = n = t = 0;
                fscanf(file, "%s", buf);
                /* can be one of %d, %d//%d, %d/%d, %d/%d/%d %d//%d */
                if (strstr(buf, "//")) {
                    /* v//n */
                    sscanf(buf, "%d//%d", &v, &n);
                    fscanf(file, "%d//%d", &v, &n);
                    fscanf(file, "%d//%d", &v, &n);
                    numtriangles++;
                    group->numtriangles++;
                    while(fscanf(file, "%d//%d", &v, &n) > 0) {
                        numtriangles++;
                        group->numtriangles++;
                    }
                } else if (sscanf(buf, "%d/%d/%d", &v, &t, &n) == 3) {
                    /* v/t/n */
                    fscanf(file, "%d/%d/%d", &v, &t, &n);
                    fscanf(file, "%d/%d/%d", &v, &t, &n);
                    numtriangles++;
                    group->numtriangles++;
                    while(fscanf(file, "%d/%d/%d", &v, &t, &n) > 0) {
                        numtriangles++;
                        group->numtriangles++;
                    }
                } else if (sscanf(buf, "%d/%d", &v, &t) == 2) {
                    /* v/t */
                    fscanf(file, "%d/%d", &v, &t);
                    fscanf(file, "%d/%d", &v, &t);
                    numtriangles++;
                    group->numtriangles++;
                    while(fscanf(file, "%d/%d", &v, &t) > 0) {
                        numtriangles++;
                        group->numtriangles++;
                    }
                } else {
                    /* v */
                    fscanf(file, "%d", &v);
                    fscanf(file, "%d", &v);
                    numtriangles++;
                    group->numtriangles++;
                    while(fscanf(file, "%d", &v) > 0) {
                        numtriangles++;
                        group->numtriangles++;
                    }
                }
                break;

            default:
                /* eat up rest of line */
                fgets(buf, sizeof(buf), file);
                break;
        }
  }

  /* set the stats in the model structure */
  model->numvertices  = numvertices;
  model->numnormals   = numnormals;
  model->numtexcoords = numtexcoords;
  model->numtriangles = numtriangles;

  /* allocate memory for the triangles in each group */
  group = model->groups;
  while(group) {
      group->triangles = (GLuint*)malloc(sizeof(GLuint) * group->numtriangles);
      group->numtriangles = 0;
      group = group->next;
  }
}

/* glmSecondPass: second pass at a Wavefront OBJ file that gets all
 * the data.
 *
 * model - properly initialized GLMmodel structure
 * file  - (fopen'd) file descriptor
 */
static GLvoid
glmSecondPass(GLMmodel* model, FILE* file)
{
    GLuint numvertices;        /* number of vertices in model */
    GLuint numnormals;         /* number of normals in model */
    GLuint numtexcoords;       /* number of texcoords in model */
    GLuint numtriangles;       /* number of triangles in model */
    GLfloat* vertices;         /* array of vertices  */
    GLfloat* normals;          /* array of normals */
    GLfloat* texcoords;        /* array of texture coordinates */
    GLMgroup* group;           /* current group pointer */
    GLuint material;           /* current material */
    int v, n, t;
    char buf[128];

    /* set the pointer shortcuts */
    vertices       = model->vertices;
    normals    = model->normals;
    texcoords    = model->texcoords;
    group      = model->groups;

    /* on the second pass through the file, read all the data into the
    allocated arrays */
    numvertices = numnormals = numtexcoords = 1;
    numtriangles = 0;
    material = 0;
    while(fscanf(file, "%s", buf) != EOF) {
        switch(buf[0]) {
        case '#':               /* comment */
            /* eat up rest of line */
            fgets(buf, sizeof(buf), file);
            break;
        case 'v':               /* v, vn, vt */
            switch(buf[1]) {
            case '\0':          /* vertex */
                fscanf(file, "%f %f %f",
                    &vertices[3 * numvertices + 0],
                    &vertices[3 * numvertices + 1],
                    &vertices[3 * numvertices + 2]);
                numvertices++;
                break;
            case 'n':           /* normal */
                fscanf(file, "%f %f %f",
                    &normals[3 * numnormals + 0],
                    &normals[3 * numnormals + 1],
                    &normals[3 * numnormals + 2]);
                numnormals++;
                break;
            case 't':           /* texcoord */
                fscanf(file, "%f %f",
                    &texcoords[2 * numtexcoords + 0],
                    &texcoords[2 * numtexcoords + 1]);
                numtexcoords++;
                break;
            }
            break;
            case 'u':
                fgets(buf, sizeof(buf), file);
                sscanf(buf, "%s %s", buf, buf);
                group->material = material = glmFindMaterial(model, buf);
                break;
            case 'g':               /* group */
                /* eat up rest of line */
                fgets(buf, sizeof(buf), file);
#if SINGLE_STRING_GROUP_NAMES
                sscanf(buf, "%s", buf);
#else
                buf[strlen(buf)-1] = '\0';  /* nuke '\n' */
#endif
                group = glmFindGroup(model, buf);
                group->material = material;
                break;
            case 'f':               /* face */
                v = n = t = 0;
                fscanf(file, "%s", buf);
                /* can be one of %d, %d//%d, %d/%d, %d/%d/%d %d//%d */
                if (strstr(buf, "//")) {
                    /* v//n */
                    sscanf(buf, "%d//%d", &v, &n);
                    T(numtriangles).vindices[0] = v < 0 ? v + numvertices : v;
                    T(numtriangles).nindices[0] = n < 0 ? n + numnormals : n;
                    fscanf(file, "%d//%d", &v, &n);
                    T(numtriangles).vindices[1] = v < 0 ? v + numvertices : v;
                    T(numtriangles).nindices[1] = n < 0 ? n + numnormals : n;
                    fscanf(file, "%d//%d", &v, &n);
                    T(numtriangles).vindices[2] = v < 0 ? v + numvertices : v;
                    T(numtriangles).nindices[2] = n < 0 ? n + numnormals : n;
                    group->triangles[group->numtriangles++] = numtriangles;
                    numtriangles++;
                    while(fscanf(file, "%d//%d", &v, &n) > 0) {
                        T(numtriangles).vindices[0] = T(numtriangles-1).vindices[0];
                        T(numtriangles).nindices[0] = T(numtriangles-1).nindices[0];
                        T(numtriangles).vindices[1] = T(numtriangles-1).vindices[2];
                        T(numtriangles).nindices[1] = T(numtriangles-1).nindices[2];
                        T(numtriangles).vindices[2] = v < 0 ? v + numvertices : v;
                        T(numtriangles).nindices[2] = n < 0 ? n + numnormals : n;
                        group->triangles[group->numtriangles++] = numtriangles;
                        numtriangles++;
                    }
                } else if (sscanf(buf, "%d/%d/%d", &v, &t, &n) == 3) {
                    /* v/t/n */
                    T(numtriangles).vindices[0] = v < 0 ? v + numvertices : v;
                    T(numtriangles).tindices[0] = t < 0 ? t + numtexcoords : t;
                    T(numtriangles).nindices[0] = n < 0 ? n + numnormals : n;
                    fscanf(file, "%d/%d/%d", &v, &t, &n);
                    T(numtriangles).vindices[1] = v < 0 ? v + numvertices : v;
                    T(numtriangles).tindices[1] = t < 0 ? t + numtexcoords : t;
                    T(numtriangles).nindices[1] = n < 0 ? n + numnormals : n;
                    fscanf(file, "%d/%d/%d", &v, &t, &n);
                    T(numtriangles).vindices[2] = v < 0 ? v + numvertices : v;
                    T(numtriangles).tindices[2] = t < 0 ? t + numtexcoords : t;
                    T(numtriangles).nindices[2] = n < 0 ? n + numnormals : n;
                    group->triangles[group->numtriangles++] = numtriangles;
                    numtriangles++;
                    while(fscanf(file, "%d/%d/%d", &v, &t, &n) > 0) {
                        T(numtriangles).vindices[0] = T(numtriangles-1).vindices[0];
                        T(numtriangles).tindices[0] = T(numtriangles-1).tindices[0];
                        T(numtriangles).nindices[0] = T(numtriangles-1).nindices[0];
                        T(numtriangles).vindices[1] = T(numtriangles-1).vindices[2];
                        T(numtriangles).tindices[1] = T(numtriangles-1).tindices[2];
                        T(numtriangles).nindices[1] = T(numtriangles-1).nindices[2];
                        T(numtriangles).vindices[2] = v < 0 ? v + numvertices : v;
                        T(numtriangles).tindices[2] = t < 0 ? t + numtexcoords : t;
                        T(numtriangles).nindices[2] = n < 0 ? n + numnormals : n;
                        group->triangles[group->numtriangles++] = numtriangles;
                        numtriangles++;
                    }
                } else if (sscanf(buf, "%d/%d", &v, &t) == 2) {
                    /* v/t */
                    T(numtriangles).vindices[0] = v < 0 ? v + numvertices : v;
                    T(numtriangles).tindices[0] = t < 0 ? t + numtexcoords : t;
                    fscanf(file, "%d/%d", &v, &t);
                    T(numtriangles).vindices[1] = v < 0 ? v + numvertices : v;
                    T(numtriangles).tindices[1] = t < 0 ? t + numtexcoords : t;
                    fscanf(file, "%d/%d", &v, &t);
                    T(numtriangles).vindices[2] = v < 0 ? v + numvertices : v;
                    T(numtriangles).tindices[2] = t < 0 ? t + numtexcoords : t;
                    group->triangles[group->numtriangles++] = numtriangles;
                    numtriangles++;
                    while(fscanf(file, "%d/%d", &v, &t) > 0) {
                        T(numtriangles).vindices[0] = T(numtriangles-1).vindices[0];
                        T(numtriangles).tindices[0] = T(numtriangles-1).tindices[0];
                        T(numtriangles).vindices[1] = T(numtriangles-1).vindices[2];
                        T(numtriangles).tindices[1] = T(numtriangles-1).tindices[2];
                        T(numtriangles).vindices[2] = v < 0 ? v + numvertices : v;
                        T(numtriangles).tindices[2] = t < 0 ? t + numtexcoords : t;
                        group->triangles[group->numtriangles++] = numtriangles;
                        numtriangles++;
                    }
                } else {
                    /* v */
                    sscanf(buf, "%d", &v);
                    T(numtriangles).vindices[0] = v < 0 ? v + numvertices : v;
                    fscanf(file, "%d", &v);
                    T(numtriangles).vindices[1] = v < 0 ? v + numvertices : v;
                    fscanf(file, "%d", &v);
                    T(numtriangles).vindices[2] = v < 0 ? v + numvertices : v;
                    group->triangles[group->numtriangles++] = numtriangles;
                    numtriangles++;
                    while(fscanf(file, "%d", &v) > 0) {
                        T(numtriangles).vindices[0] = T(numtriangles-1).vindices[0];
                        T(numtriangles).vindices[1] = T(numtriangles-1).vindices[2];
                        T(numtriangles).vindices[2] = v < 0 ? v + numvertices : v;
                        group->triangles[group->numtriangles++] = numtriangles;
                        numtriangles++;
                    }
                }
                break;

            default:
                /* eat up rest of line */
                fgets(buf, sizeof(buf), file);
                break;
    }
  }

#if 0
  /* announce the memory requirements */
  printf(" Memory: %d bytes\n",
      numvertices  * 3*sizeof(GLfloat) +
      numnormals   * 3*sizeof(GLfloat) * (numnormals ? 1 : 0) +
      numtexcoords * 3*sizeof(GLfloat) * (numtexcoords ? 1 : 0) +
      numtriangles * sizeof(GLMtriangle));
#endif
}


/* public functions */


/* glmUnitize: "unitize" a model by translating it to the origin and
 * scaling it to fit in a unit cube around the origin.   Returns the
 * scalefactor used.
 *
 * model - properly initialized GLMmodel structure
 */
GLfloat
glmUnitize(GLMmodel* model)
{
    GLuint i;
    GLfloat maxx, minx, maxy, miny, maxz, minz;
    GLfloat cx, cy, cz, w, h, d;
    GLfloat scale;

    assert(model);
    assert(model->vertices);

    /* get the max/mins */
    maxx = minx = model->vertices[3 + 0];
    maxy = miny = model->vertices[3 + 1];
    maxz = minz = model->vertices[3 + 2];
    for (i = 1; i <= model->numvertices; i++) {
        if (maxx < model->vertices[3 * i + 0])
            maxx = model->vertices[3 * i + 0];
        if (minx > model->vertices[3 * i + 0])
            minx = model->vertices[3 * i + 0];

        if (maxy < model->vertices[3 * i + 1])
            maxy = model->vertices[3 * i + 1];
        if (miny > model->vertices[3 * i + 1])
            miny = model->vertices[3 * i + 1];

        if (maxz < model->vertices[3 * i + 2])
            maxz = model->vertices[3 * i + 2];
        if (minz > model->vertices[3 * i + 2])
            minz = model->vertices[3 * i + 2];
    }

    /* calculate model width, height, and depth */
    w = glmAbs(maxx) + glmAbs(minx);
    h = glmAbs(maxy) + glmAbs(miny);
    d = glmAbs(maxz) + glmAbs(minz);

    /* calculate center of the model */
    cx = (maxx + minx) / 2.0;
    cy = (maxy + miny) / 2.0;
    cz = (maxz + minz) / 2.0;

    /* calculate unitizing scale factor */
    scale = 2.0 / glmMax(glmMax(w, h), d);

    /* translate around center then scale */
    for (i = 1; i <= model->numvertices; i++) {
        model->vertices[3 * i + 0] -= cx;
        model->vertices[3 * i + 1] -= cy;
        model->vertices[3 * i + 2] -= cz;
        model->vertices[3 * i + 0] *= scale;
        model->vertices[3 * i + 1] *= scale;
        model->vertices[3 * i + 2] *= scale;
    }

    return scale;
}

/* glmDimensions: Calculates the dimensions (width, height, depth) of
 * a model.
 *
 * model   - initialized GLMmodel structure
 * dimensions - array of 3 GLfloats (GLfloat dimensions[3])
 */
GLvoid
glmDimensions(GLMmodel* model, GLfloat* dimensions)
{
    GLuint i;
    GLfloat maxx, minx, maxy, miny, maxz, minz;

    assert(model);
    assert(model->vertices);
    assert(dimensions);

    /* get the max/mins */
    maxx = minx = model->vertices[3 + 0];
    maxy = miny = model->vertices[3 + 1];
    maxz = minz = model->vertices[3 + 2];
    for (i = 1; i <= model->numvertices; i++) {
        if (maxx < model->vertices[3 * i + 0])
            maxx = model->vertices[3 * i + 0];
        if (minx > model->vertices[3 * i + 0])
            minx = model->vertices[3 * i + 0];

        if (maxy < model->vertices[3 * i + 1])
            maxy = model->vertices[3 * i + 1];
        if (miny > model->vertices[3 * i + 1])
            miny = model->vertices[3 * i + 1];

        if (maxz < model->vertices[3 * i + 2])
            maxz = model->vertices[3 * i + 2];
        if (minz > model->vertices[3 * i + 2])
            minz = model->vertices[3 * i + 2];
    }

    /* calculate model width, height, and depth */
    dimensions[0] = glmAbs(maxx) + glmAbs(minx);
    dimensions[1] = glmAbs(maxy) + glmAbs(miny);
    dimensions[2] = glmAbs(maxz) + glmAbs(minz);
}

/* glmScale: Scales a model by a given amount.
 *
 * model - properly initialized GLMmodel structure
 * scale - scalefactor (0.5 = half as large, 2.0 = twice as large)
 */
GLvoid
glmScale(GLMmodel* model, GLfloat scale)
{
    GLuint i;

    for (i = 1; i <= model->numvertices; i++) {
        model->vertices[3 * i + 0] *= scale;
        model->vertices[3 * i + 1] *= scale;
        model->vertices[3 * i + 2] *= scale;
    }
}

/* glmReverseWinding: Reverse the polygon winding for all polygons in
 * this model.   Default winding is counter-clockwise.  Also changes
 * the direction of the normals.
 *
 * model - properly initialized GLMmodel structure
 */
GLvoid
glmReverseWinding(GLMmodel* model)
{
    GLuint i, swap;

    assert(model);

    for (i = 0; i < model->numtriangles; i++) {
        swap = T(i).vindices[0];
        T(i).vindices[0] = T(i).vindices[2];
        T(i).vindices[2] = swap;

        if (model->numnormals) {
            swap = T(i).nindices[0];
            T(i).nindices[0] = T(i).nindices[2];
            T(i).nindices[2] = swap;
        }

        if (model->numtexcoords) {
            swap = T(i).tindices[0];
            T(i).tindices[0] = T(i).tindices[2];
            T(i).tindices[2] = swap;
        }
    }

    /* reverse facet normals */
    for (i = 1; i <= model->numfacetnorms; i++) {
        model->facetnorms[3 * i + 0] = -model->facetnorms[3 * i + 0];
        model->facetnorms[3 * i + 1] = -model->facetnorms[3 * i + 1];
        model->facetnorms[3 * i + 2] = -model->facetnorms[3 * i + 2];
    }

    /* reverse vertex normals */
    for (i = 1; i <= model->numnormals; i++) {
        model->normals[3 * i + 0] = -model->normals[3 * i + 0];
        model->normals[3 * i + 1] = -model->normals[3 * i + 1];
        model->normals[3 * i + 2] = -model->normals[3 * i + 2];
    }
}

/* glmFacetNormals: Generates facet normals for a model (by taking the
 * cross product of the two vectors derived from the sides of each
 * triangle).  Assumes a counter-clockwise winding.
 *
 * model - initialized GLMmodel structure
 */
GLvoid
glmFacetNormals(GLMmodel* model)
{
    GLuint  i;
    GLfloat u[3];
    GLfloat v[3];

    assert(model);
    assert(model->vertices);

    /* clobber any old facetnormals */
    if (model->facetnorms)
        free(model->facetnorms);

    /* allocate memory for the new facet normals */
    model->numfacetnorms = model->numtriangles;
    model->facetnorms = (GLfloat*)malloc(sizeof(GLfloat) *
                       3 * (model->numfacetnorms + 1));

    for (i = 0; i < model->numtriangles; i++) {
        model->triangles[i].findex = i+1;

        u[0] = model->vertices[3 * T(i).vindices[1] + 0] -
            model->vertices[3 * T(i).vindices[0] + 0];
        u[1] = model->vertices[3 * T(i).vindices[1] + 1] -
            model->vertices[3 * T(i).vindices[0] + 1];
        u[2] = model->vertices[3 * T(i).vindices[1] + 2] -
            model->vertices[3 * T(i).vindices[0] + 2];

        v[0] = model->vertices[3 * T(i).vindices[2] + 0] -
            model->vertices[3 * T(i).vindices[0] + 0];
        v[1] = model->vertices[3 * T(i).vindices[2] + 1] -
            model->vertices[3 * T(i).vindices[0] + 1];
        v[2] = model->vertices[3 * T(i).vindices[2] + 2] -
            model->vertices[3 * T(i).vindices[0] + 2];

        glmCross(u, v, &model->facetnorms[3 * (i+1)]);
        glmNormalize(&model->facetnorms[3 * (i+1)]);
    }
}

/* glmVertexNormals: Generates smooth vertex normals for a model.
 * First builds a list of all the triangles each vertex is in.   Then
 * loops through each vertex in the the list averaging all the facet
 * normals of the triangles each vertex is in.   Finally, sets the
 * normal index in the triangle for the vertex to the generated smooth
 * normal.   If the dot product of a facet normal and the facet normal
 * associated with the first triangle in the list of triangles the
 * current vertex is in is greater than the cosine of the angle
 * parameter to the function, that facet normal is not added into the
 * average normal calculation and the corresponding vertex is given
 * the facet normal.  This tends to preserve hard edges.  The angle to
 * use depends on the model, but 90 degrees is usually a good start.
 *
 * model - initialized GLMmodel structure
 * angle - maximum angle (in degrees) to smooth across
 */
GLvoid
glmVertexNormals(GLMmodel* model, GLfloat angle)
{
    GLMnode* node;
    GLMnode* tail;
    GLMnode** members;
    GLfloat* normals;
    GLuint numnormals;
    GLfloat average[3];
    GLfloat dot, cos_angle;
    GLuint i, avg;

    assert(model);
    assert(model->facetnorms);

    /* calculate the cosine of the angle (in degrees) */
    cos_angle = cos(angle * M_PI / 180.0);

    /* nuke any previous normals */
    if (model->normals)
        free(model->normals);

    /* allocate space for new normals */
    model->numnormals = model->numtriangles * 3; /* 3 normals per triangle */
    model->normals = (GLfloat*)malloc(sizeof(GLfloat)* 3* (model->numnormals+1));

    /* allocate a structure that will hold a linked list of triangle
    indices for each vertex */
    members = (GLMnode**)malloc(sizeof(GLMnode*) * (model->numvertices + 1));
    for (i = 1; i <= model->numvertices; i++)
        members[i] = NULL;

    /* for every triangle, create a node for each vertex in it */
    for (i = 0; i < model->numtriangles; i++) {
        node = (GLMnode*)malloc(sizeof(GLMnode));
        node->index = i;
        node->next  = members[T(i).vindices[0]];
        members[T(i).vindices[0]] = node;

        node = (GLMnode*)malloc(sizeof(GLMnode));
        node->index = i;
        node->next  = members[T(i).vindices[1]];
        members[T(i).vindices[1]] = node;

        node = (GLMnode*)malloc(sizeof(GLMnode));
        node->index = i;
        node->next  = members[T(i).vindices[2]];
        members[T(i).vindices[2]] = node;
    }

    /* calculate the average normal for each vertex */
    numnormals = 1;
    for (i = 1; i <= model->numvertices; i++) {
    /* calculate an average normal for this vertex by averaging the
        facet normal of every triangle this vertex is in */
        node = members[i];
        if (!node)
            fprintf(stderr, "glmVertexNormals(): vertex w/o a triangle\n");
        average[0] = 0.0; average[1] = 0.0; average[2] = 0.0;
        avg = 0;
        while (node) {
        /* only average if the dot product of the angle between the two
        facet normals is greater than the cosine of the threshold
        angle -- or, said another way, the angle between the two
            facet normals is less than (or equal to) the threshold angle */
            dot = glmDot(&model->facetnorms[3 * T(node->index).findex],
                &model->facetnorms[3 * T(members[i]->index).findex]);
            if (dot > cos_angle) {
                node->averaged = GL_TRUE;
                average[0] += model->facetnorms[3 * T(node->index).findex + 0];
                average[1] += model->facetnorms[3 * T(node->index).findex + 1];
                average[2] += model->facetnorms[3 * T(node->index).findex + 2];
                avg = 1;            /* we averaged at least one normal! */
            } else {
                node->averaged = GL_FALSE;
            }
            node = node->next;
        }

        if (avg) {
            /* normalize the averaged normal */
            glmNormalize(average);

            /* add the normal to the vertex normals list */
            model->normals[3 * numnormals + 0] = average[0];
            model->normals[3 * numnormals + 1] = average[1];
            model->normals[3 * numnormals + 2] = average[2];
            avg = numnormals;
            numnormals++;
        }

        /* set the normal of this vertex in each triangle it is in */
        node = members[i];
        while (node) {
            if (node->averaged) {
                /* if this node was averaged, use the average normal */
                if (T(node->index).vindices[0] == i)
                    T(node->index).nindices[0] = avg;
                else if (T(node->index).vindices[1] == i)
                    T(node->index).nindices[1] = avg;
                else if (T(node->index).vindices[2] == i)
                    T(node->index).nindices[2] = avg;
            } else {
                /* if this node wasn't averaged, use the facet normal */
                model->normals[3 * numnormals + 0] =
                    model->facetnorms[3 * T(node->index).findex + 0];
                model->normals[3 * numnormals + 1] =
                    model->facetnorms[3 * T(node->index).findex + 1];
                model->normals[3 * numnormals + 2] =
                    model->facetnorms[3 * T(node->index).findex + 2];
                if (T(node->index).vindices[0] == i)
                    T(node->index).nindices[0] = numnormals;
                else if (T(node->index).vindices[1] == i)
                    T(node->index).nindices[1] = numnormals;
                else if (T(node->index).vindices[2] == i)
                    T(node->index).nindices[2] = numnormals;
                numnormals++;
            }
            node = node->next;
        }
    }

    model->numnormals = numnormals - 1;

    /* free the member information */
    for (i = 1; i <= model->numvertices; i++) {
        node = members[i];
        while (node) {
            tail = node;
            node = node->next;
            free(tail);
        }
    }
    free(members);

    /* pack the normals array (we previously allocated the maximum
    number of normals that could possibly be created (numtriangles *
    3), so get rid of some of them (usually alot unless none of the
    facet normals were averaged)) */
    normals = model->normals;
    model->normals = (GLfloat*)malloc(sizeof(GLfloat)* 3* (model->numnormals+1));
    for (i = 1; i <= model->numnormals; i++) {
        model->normals[3 * i + 0] = normals[3 * i + 0];
        model->normals[3 * i + 1] = normals[3 * i + 1];
        model->normals[3 * i + 2] = normals[3 * i + 2];
    }
    free(normals);
}


/* glmLinearTexture: Generates texture coordinates according to a
 * linear projection of the texture map.  It generates these by
 * linearly mapping the vertices onto a square.
 *
 * model - pointer to initialized GLMmodel structure
 */
GLvoid
glmLinearTexture(GLMmodel* model)
{
    GLMgroup *group;
    GLfloat dimensions[3];
    GLfloat x, y, scalefactor;
    GLuint i;

    assert(model);

    if (model->texcoords)
        free(model->texcoords);
    model->numtexcoords = model->numvertices;
    model->texcoords=(GLfloat*)malloc(sizeof(GLfloat)*2*(model->numtexcoords+1));

    glmDimensions(model, dimensions);
    scalefactor = 2.0 /
        glmAbs(glmMax(glmMax(dimensions[0], dimensions[1]), dimensions[2]));

    /* do the calculations */
    for(i = 1; i <= model->numvertices; i++) {
        x = model->vertices[3 * i + 0] * scalefactor;
        y = model->vertices[3 * i + 2] * scalefactor;
        model->texcoords[2 * i + 0] = (x + 1.0) / 2.0;
        model->texcoords[2 * i + 1] = (y + 1.0) / 2.0;
    }

    /* go through and put texture coordinate indices in all the triangles */
    group = model->groups;
    while(group) {
        for(i = 0; i < group->numtriangles; i++) {
            T(group->triangles[i]).tindices[0] = T(group->triangles[i]).vindices[0];
            T(group->triangles[i]).tindices[1] = T(group->triangles[i]).vindices[1];
            T(group->triangles[i]).tindices[2] = T(group->triangles[i]).vindices[2];
        }
        group = group->next;
    }

#if 0
    printf("glmLinearTexture(): generated %d linear texture coordinates\n",
        model->numtexcoords);
#endif
}

/* glmSpheremapTexture: Generates texture coordinates according to a
 * spherical projection of the texture map.  Sometimes referred to as
 * spheremap, or reflection map texture coordinates.  It generates
 * these by using the normal to calculate where that vertex would map
 * onto a sphere.  Since it is impossible to map something flat
 * perfectly onto something spherical, there is distortion at the
 * poles.  This particular implementation causes the poles along the X
 * axis to be distorted.
 *
 * model - pointer to initialized GLMmodel structure
 */
GLvoid
glmSpheremapTexture(GLMmodel* model)
{
    GLMgroup* group;
    GLfloat theta, phi, rho, x, y, z, r;
    GLuint i;

    assert(model);
    assert(model->normals);

    if (model->texcoords)
        free(model->texcoords);
    model->numtexcoords = model->numnormals;
    model->texcoords=(GLfloat*)malloc(sizeof(GLfloat)*2*(model->numtexcoords+1));

    for (i = 1; i <= model->numnormals; i++) {
        z = model->normals[3 * i + 0];  /* re-arrange for pole distortion */
        y = model->normals[3 * i + 1];
        x = model->normals[3 * i + 2];
        r = sqrt((x * x) + (y * y));
        rho = sqrt((r * r) + (z * z));

        if(r == 0.0) {
            theta = 0.0;
            phi = 0.0;
        } else {
            if(z == 0.0)
                phi = 3.14159265 / 2.0;
            else
                phi = acos(z / rho);

            if(y == 0.0)
                theta = 3.141592365 / 2.0;
            else
                theta = asin(y / r) + (3.14159265 / 2.0);
        }

        model->texcoords[2 * i + 0] = theta / 3.14159265;
        model->texcoords[2 * i + 1] = phi / 3.14159265;
    }

    /* go through and put texcoord indices in all the triangles */
    group = model->groups;
    while(group) {
        for (i = 0; i < group->numtriangles; i++) {
            T(group->triangles[i]).tindices[0] = T(group->triangles[i]).nindices[0];
            T(group->triangles[i]).tindices[1] = T(group->triangles[i]).nindices[1];
            T(group->triangles[i]).tindices[2] = T(group->triangles[i]).nindices[2];
        }
        group = group->next;
    }
}

/* glmDelete: Deletes a GLMmodel structure.
 *
 * model - initialized GLMmodel structure
 */
GLvoid
glmDelete(GLMmodel* model)
{
    GLMgroup* group;
    GLuint i;

    assert(model);

    if (model->pathname)     free(model->pathname);
    if (model->mtllibname) free(model->mtllibname);
    if (model->vertices)     free(model->vertices);
    if (model->normals)  free(model->normals);
    if (model->texcoords)  free(model->texcoords);
    if (model->facetnorms) free(model->facetnorms);
    if (model->triangles)  free(model->triangles);
    if (model->materials) {
        for (i = 0; i < model->nummaterials; i++)
            free(model->materials[i].name);
    }
    free(model->materials);
    while(model->groups) {
        group = model->groups;
        model->groups = model->groups->next;
        free(group->name);
        free(group->triangles);
        free(group);
    }

    free(model);
}

/* glmReadOBJ: Reads a model description from a Wavefront .OBJ file.
 * Returns a pointer to the created object which should be free'd with
 * glmDelete().
 *
 * filename - name of the file containing the Wavefront .OBJ format data.
 */
GLMmodel*
glmReadOBJ(char* filename)
{
    GLMmodel* model;
    FILE* file;

    /* open the file */
    file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "glmReadOBJ() failed: can't open data file \"%s\".\n",
            filename);
        exit(1);
    }

    /* allocate a new model */
    model = (GLMmodel*)malloc(sizeof(GLMmodel));
    model->pathname    = strdup(filename);
    model->mtllibname    = NULL;
    model->numvertices   = 0;
    model->vertices    = NULL;
    model->numnormals    = 0;
    model->normals     = NULL;
    model->numtexcoords  = 0;
    model->texcoords       = NULL;
    model->numfacetnorms = 0;
    model->facetnorms    = NULL;
    model->numtriangles  = 0;
    model->triangles       = NULL;
    model->nummaterials  = 0;
    model->materials       = NULL;
    model->numgroups       = 0;
    model->groups      = NULL;
    model->position[0]   = 0.0;
    model->position[1]   = 0.0;
    model->position[2]   = 0.0;

    /* make a first pass through the file to get a count of the number
    of vertices, normals, texcoords & triangles */
    glmFirstPass(model, file);

    /* allocate memory */
    model->vertices = (GLfloat*)malloc(sizeof(GLfloat) *
        3 * (model->numvertices + 1));
    model->triangles = (GLMtriangle*)malloc(sizeof(GLMtriangle) *
        model->numtriangles);
    if (model->numnormals) {
        model->normals = (GLfloat*)malloc(sizeof(GLfloat) *
            3 * (model->numnormals + 1));
    }
    if (model->numtexcoords) {
        model->texcoords = (GLfloat*)malloc(sizeof(GLfloat) *
            2 * (model->numtexcoords + 1));
    }

    /* rewind to beginning of file and read in the data this pass */
    rewind(file);

    glmSecondPass(model, file);

    /* close the file */
    fclose(file);

    return model;
}

/* glmWriteOBJ: Writes a model description in Wavefront .OBJ format to
 * a file.
 *
 * model - initialized GLMmodel structure
 * filename - name of the file to write the Wavefront .OBJ format data to
 * mode  - a bitwise or of values describing what is written to the file
 *             GLM_NONE     -  render with only vertices
 *             GLM_FLAT     -  render with facet normals
 *             GLM_SMOOTH   -  render with vertex normals
 *             GLM_TEXTURE  -  render with texture coords
 *             GLM_COLOR    -  render with colors (color material)
 *             GLM_MATERIAL -  render with materials
 *             GLM_COLOR and GLM_MATERIAL should not both be specified.
 *             GLM_FLAT and GLM_SMOOTH should not both be specified.
 */
GLvoid
glmWriteOBJ(GLMmodel* model, char* filename, GLuint mode)
{
    GLuint i;
    FILE* file;
    GLMgroup* group;

    assert(model);

    /* do a bit of warning */
    if (mode & GLM_FLAT && !model->facetnorms) {
        printf("glmWriteOBJ() warning: flat normal output requested "
            "with no facet normals defined.\n");
        mode &= ~GLM_FLAT;
    }
    if (mode & GLM_SMOOTH && !model->normals) {
        printf("glmWriteOBJ() warning: smooth normal output requested "
            "with no normals defined.\n");
        mode &= ~GLM_SMOOTH;
    }
    if (mode & GLM_TEXTURE && !model->texcoords) {
        printf("glmWriteOBJ() warning: texture coordinate output requested "
            "with no texture coordinates defined.\n");
        mode &= ~GLM_TEXTURE;
    }
    if (mode & GLM_FLAT && mode & GLM_SMOOTH) {
        printf("glmWriteOBJ() warning: flat normal output requested "
            "and smooth normal output requested (using smooth).\n");
        mode &= ~GLM_FLAT;
    }
    if (mode & GLM_COLOR && !model->materials) {
        printf("glmWriteOBJ() warning: color output requested "
            "with no colors (materials) defined.\n");
        mode &= ~GLM_COLOR;
    }
    if (mode & GLM_MATERIAL && !model->materials) {
        printf("glmWriteOBJ() warning: material output requested "
            "with no materials defined.\n");
        mode &= ~GLM_MATERIAL;
    }
    if (mode & GLM_COLOR && mode & GLM_MATERIAL) {
        printf("glmWriteOBJ() warning: color and material output requested "
            "outputting only materials.\n");
        mode &= ~GLM_COLOR;
    }


    /* open the file */
    file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "glmWriteOBJ() failed: can't open file \"%s\" to write.\n",
            filename);
        exit(1);
    }

    /* spit out a header */
    fprintf(file, "#  \n");
    fprintf(file, "#  Wavefront OBJ generated by GLM library\n");
    fprintf(file, "#  \n");
    fprintf(file, "#  GLM library\n");
    fprintf(file, "#  Nate Robins\n");
    fprintf(file, "#  ndr@pobox.com\n");
    fprintf(file, "#  http://www.pobox.com/~ndr\n");
    fprintf(file, "#  \n");

    if (mode & GLM_MATERIAL && model->mtllibname) {
        fprintf(file, "\nmtllib %s\n\n", model->mtllibname);
        glmWriteMTL(model, filename, model->mtllibname);
    }

    /* spit out the vertices */
    fprintf(file, "\n");
    fprintf(file, "# %d vertices\n", model->numvertices);
    for (i = 1; i <= model->numvertices; i++) {
        fprintf(file, "v %f %f %f\n",
            model->vertices[3 * i + 0],
            model->vertices[3 * i + 1],
            model->vertices[3 * i + 2]);
    }

    /* spit out the smooth/flat normals */
    if (mode & GLM_SMOOTH) {
        fprintf(file, "\n");
        fprintf(file, "# %d normals\n", model->numnormals);
        for (i = 1; i <= model->numnormals; i++) {
            fprintf(file, "vn %f %f %f\n",
                model->normals[3 * i + 0],
                model->normals[3 * i + 1],
                model->normals[3 * i + 2]);
        }
    } else if (mode & GLM_FLAT) {
        fprintf(file, "\n");
        fprintf(file, "# %d normals\n", model->numfacetnorms);
        for (i = 1; i <= model->numnormals; i++) {
            fprintf(file, "vn %f %f %f\n",
                model->facetnorms[3 * i + 0],
                model->facetnorms[3 * i + 1],
                model->facetnorms[3 * i + 2]);
        }
    }

    /* spit out the texture coordinates */
    if (mode & GLM_TEXTURE) {
        fprintf(file, "\n");
        fprintf(file, "# %d texcoords\n", model->numtexcoords);
        for (i = 1; i <= model->numtexcoords; i++) {
            fprintf(file, "vt %f %f\n",
                model->texcoords[2 * i + 0],
                model->texcoords[2 * i + 1]);
        }
    }

    fprintf(file, "\n");
    fprintf(file, "# %d groups\n", model->numgroups);
    fprintf(file, "# %d faces (triangles)\n", model->numtriangles);
    fprintf(file, "\n");

    group = model->groups;
    while(group) {
        fprintf(file, "g %s\n", group->name);
        if (mode & GLM_MATERIAL)
            fprintf(file, "usemtl %s\n", model->materials[group->material].name);
        for (i = 0; i < group->numtriangles; i++) {
            if (mode & GLM_SMOOTH && mode & GLM_TEXTURE) {
                fprintf(file, "f %d/%d/%d %d/%d/%d %d/%d/%d\n",
                    T(group->triangles[i]).vindices[0],
                    T(group->triangles[i]).tindices[0],
                    T(group->triangles[i]).nindices[0],
                    T(group->triangles[i]).vindices[1],
                    T(group->triangles[i]).tindices[1],
                    T(group->triangles[i]).nindices[1],
                    T(group->triangles[i]).vindices[2],
                    T(group->triangles[i]).tindices[2],
                    T(group->triangles[i]).nindices[2]);
            } else if (mode & GLM_FLAT && mode & GLM_TEXTURE) {
                fprintf(file, "f %d/%d %d/%d %d/%d\n",
                    T(group->triangles[i]).vindices[0],
                    T(group->triangles[i]).findex,
                    T(group->triangles[i]).vindices[1],
                    T(group->triangles[i]).findex,
                    T(group->triangles[i]).vindices[2],
                    T(group->triangles[i]).findex);
            } else if (mode & GLM_TEXTURE) {
                fprintf(file, "f %d/%d %d/%d %d/%d\n",
                    T(group->triangles[i]).vindices[0],
                    T(group->triangles[i]).tindices[0],
                    T(group->triangles[i]).vindices[1],
                    T(group->triangles[i]).tindices[1],
                    T(group->triangles[i]).vindices[2],
                    T(group->triangles[i]).tindices[2]);
            } else if (mode & GLM_SMOOTH) {
                fprintf(file, "f %d//%d %d//%d %d//%d\n",
                    T(group->triangles[i]).vindices[0],
                    T(group->triangles[i]).nindices[0],
                    T(group->triangles[i]).vindices[1],
                    T(group->triangles[i]).nindices[1],
                    T(group->triangles[i]).vindices[2],
                    T(group->triangles[i]).nindices[2]);
            } else if (mode & GLM_FLAT) {
                fprintf(file, "f %d//%d %d//%d %d//%d\n",
                    T(group->triangles[i]).vindices[0],
                    T(group->triangles[i]).findex,
                    T(group->triangles[i]).vindices[1],
                    T(group->triangles[i]).findex,
                    T(group->triangles[i]).vindices[2],
                    T(group->triangles[i]).findex);
            } else {
                fprintf(file, "f %d %d %d\n",
                    T(group->triangles[i]).vindices[0],
                    T(group->triangles[i]).vindices[1],
                    T(group->triangles[i]).vindices[2]);
            }
        }
        fprintf(file, "\n");
        group = group->next;
    }

    fclose(file);
}

/* glmDraw: Renders the model to the current OpenGL context using the
 * mode specified.
 *
 * model - initialized GLMmodel structure
 * mode  - a bitwise OR of values describing what is to be rendered.
 *             GLM_NONE     -  render with only vertices
 *             GLM_FLAT     -  render with facet normals
 *             GLM_SMOOTH   -  render with vertex normals
 *             GLM_TEXTURE  -  render with texture coords
 *             GLM_COLOR    -  render with colors (color material)
 *             GLM_MATERIAL -  render with materials
 *             GLM_COLOR and GLM_MATERIAL should not both be specified.
 *             GLM_FLAT and GLM_SMOOTH should not both be specified.
 */
GLvoid
glmDraw(GLMmodel* model, GLuint mode)
{
    static GLuint i;
    static GLMgroup* group;
    static GLMtriangle* triangle;
    static GLMmaterial* material;

    assert(model);
    assert(model->vertices);

    /* do a bit of warning */
    if (mode & GLM_FLAT && !model->facetnorms) {
        printf("glmDraw() warning: flat render mode requested "
            "with no facet normals defined.\n");
        mode &= ~GLM_FLAT;
    }
    if (mode & GLM_SMOOTH && !model->normals) {
        printf("glmDraw() warning: smooth render mode requested "
            "with no normals defined.\n");
        mode &= ~GLM_SMOOTH;
    }
    if (mode & GLM_TEXTURE && !model->texcoords) {
        printf("glmDraw() warning: texture render mode requested "
            "with no texture coordinates defined.\n");
        mode &= ~GLM_TEXTURE;
    }
    if (mode & GLM_FLAT && mode & GLM_SMOOTH) {
        printf("glmDraw() warning: flat render mode requested "
            "and smooth render mode requested (using smooth).\n");
        mode &= ~GLM_FLAT;
    }
    if (mode & GLM_COLOR && !model->materials) {
        printf("glmDraw() warning: color render mode requested "
            "with no materials defined.\n");
        mode &= ~GLM_COLOR;
    }
    if (mode & GLM_MATERIAL && !model->materials) {
        printf("glmDraw() warning: material render mode requested "
            "with no materials defined.\n");
        mode &= ~GLM_MATERIAL;
    }
    if (mode & GLM_COLOR && mode & GLM_MATERIAL) {
        printf("glmDraw() warning: color and material render mode requested "
            "using only material mode.\n");
        mode &= ~GLM_COLOR;
    }
    if (mode & GLM_COLOR)
        glEnable(GL_COLOR_MATERIAL);
    else if (mode & GLM_MATERIAL)
        glDisable(GL_COLOR_MATERIAL);

    /* perhaps this loop should be unrolled into material, color, flat,
       smooth, etc. loops?  since most cpu's have good branch prediction
       schemes (and these branches will always go one way), probably
       wouldn't gain too much?  */

    group = model->groups;
    while (group) {
        if (mode & GLM_MATERIAL) {
            material = &model->materials[group->material];
            glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, material->ambient);
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material->diffuse);
            glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material->specular);
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, material->shininess);
        }

        if (mode & GLM_COLOR) {
            glColor3fv(material->diffuse);
        }

        glBegin(GL_TRIANGLES);
        for (i = 0; i < group->numtriangles; i++) {
            triangle = &T(group->triangles[i]);

            if (mode & GLM_FLAT)
                glNormal3fv(&model->facetnorms[3 * triangle->findex]);

            if (mode & GLM_SMOOTH)
                glNormal3fv(&model->normals[3 * triangle->nindices[0]]);
            if (mode & GLM_TEXTURE)
                glTexCoord2fv(&model->texcoords[2 * triangle->tindices[0]]);
            glVertex3fv(&model->vertices[3 * triangle->vindices[0]]);

            if (mode & GLM_SMOOTH)
                glNormal3fv(&model->normals[3 * triangle->nindices[1]]);
            if (mode & GLM_TEXTURE)
                glTexCoord2fv(&model->texcoords[2 * triangle->tindices[1]]);
            glVertex3fv(&model->vertices[3 * triangle->vindices[1]]);

            if (mode & GLM_SMOOTH)
                glNormal3fv(&model->normals[3 * triangle->nindices[2]]);
            if (mode & GLM_TEXTURE)
                glTexCoord2fv(&model->texcoords[2 * triangle->tindices[2]]);
            glVertex3fv(&model->vertices[3 * triangle->vindices[2]]);

        }
        glEnd();

        group = group->next;
    }
}

/* glmList: Generates and returns a display list for the model using
 * the mode specified.
 *
 * model - initialized GLMmodel structure
 * mode  - a bitwise OR of values describing what is to be rendered.
 *             GLM_NONE     -  render with only vertices
 *             GLM_FLAT     -  render with facet normals
 *             GLM_SMOOTH   -  render with vertex normals
 *             GLM_TEXTURE  -  render with texture coords
 *             GLM_COLOR    -  render with colors (color material)
 *             GLM_MATERIAL -  render with materials
 *             GLM_COLOR and GLM_MATERIAL should not both be specified.
 * GLM_FLAT and GLM_SMOOTH should not both be specified.
 */
GLuint
glmList(GLMmodel* model, GLuint mode)
{
    GLuint list;

    list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glmDraw(model, mode);
    glEndList();

    return list;
}

/* glmWeld: eliminate (weld) vectors that are within an epsilon of
 * each other.
 *
 * model   - initialized GLMmodel structure
 * epsilon     - maximum difference between vertices
 *               ( 0.00001 is a good start for a unitized model)
 *
 */
GLvoid
glmWeld(GLMmodel* model, GLfloat epsilon)
{
    GLfloat* vectors;
    GLfloat* copies;
    GLuint numvectors;
    GLuint i;

    /* vertices */
    numvectors = model->numvertices;
    vectors  = model->vertices;
    copies = glmWeldVectors(vectors, &numvectors, epsilon);

#if 0
    printf("glmWeld(): %d redundant vertices.\n",
        model->numvertices - numvectors - 1);
#endif

    for (i = 0; i < model->numtriangles; i++) {
        T(i).vindices[0] = (GLuint)vectors[3 * T(i).vindices[0] + 0];
        T(i).vindices[1] = (GLuint)vectors[3 * T(i).vindices[1] + 0];
        T(i).vindices[2] = (GLuint)vectors[3 * T(i).vindices[2] + 0];
    }

    /* free space for old vertices */
    free(vectors);

    /* allocate space for the new vertices */
    model->numvertices = numvectors;
    model->vertices = (GLfloat*)malloc(sizeof(GLfloat) *
        3 * (model->numvertices + 1));

    /* copy the optimized vertices into the actual vertex list */
    for (i = 1; i <= model->numvertices; i++) {
        model->vertices[3 * i + 0] = copies[3 * i + 0];
        model->vertices[3 * i + 1] = copies[3 * i + 1];
        model->vertices[3 * i + 2] = copies[3 * i + 2];
    }

    free(copies);
}

/* glmReadPPM: read a PPM raw (type P6) file.  The PPM file has a header
 * that should look something like:
 *
 *    P6
 *    # comment
 *    width height max_value
 *    rgbrgbrgb...
 *
 * where "P6" is the magic cookie which identifies the file type and
 * should be the only characters on the first line followed by a
 * carriage return.  Any line starting with a # mark will be treated
 * as a comment and discarded.   After the magic cookie, three integer
 * values are expected: width, height of the image and the maximum
 * value for a pixel (max_value must be < 256 for PPM raw files).  The
 * data section consists of width*height rgb triplets (one byte each)
 * in binary format (i.e., such as that written with fwrite() or
 * equivalent).
 *
 * The rgb data is returned as an array of unsigned chars (packed
 * rgb).  The malloc()'d memory should be free()'d by the caller.  If
 * an error occurs, an error message is sent to stderr and NULL is
 * returned.
 *
 * filename   - name of the .ppm file.
 * width      - will contain the width of the image on return.
 * height     - will contain the height of the image on return.
 *
 */
GLubyte*
glmReadPPM(char* filename, int* width, int* height)
{
    FILE* fp;
    int i, w, h, d;
    unsigned char* image;
    char head[70];          /* max line <= 70 in PPM (per spec). */

    fp = fopen(filename, "rb");
    if (!fp) {
        perror(filename);
        return NULL;
    }

    /* grab first two chars of the file and make sure that it has the
       correct magic cookie for a raw PPM file. */
    fgets(head, 70, fp);
    if (strncmp(head, "P6", 2)) {
        fprintf(stderr, "%s: Not a raw PPM file\n", filename);
        return NULL;
    }

    /* grab the three elements in the header (width, height, maxval). */
    i = 0;
    while(i < 3) {
        fgets(head, 70, fp);
        if (head[0] == '#')     /* skip comments. */
            continue;
        if (i == 0)
            i += sscanf(head, "%d %d %d", &w, &h, &d);
        else if (i == 1)
            i += sscanf(head, "%d %d", &h, &d);
        else if (i == 2)
            i += sscanf(head, "%d", &d);
    }

    /* grab all the image data in one fell swoop. */
    image = (unsigned char*)malloc(sizeof(unsigned char)*w*h*3);
    fread(image, sizeof(unsigned char), w*h*3, fp);
    fclose(fp);

    *width = w;
    *height = h;
    return image;
}

#if 0
/* normals */
if (model->numnormals) {
    numvectors = model->numnormals;
    vectors  = model->normals;
    copies = glmOptimizeVectors(vectors, &numvectors);

    printf("glmOptimize(): %d redundant normals.\n",
        model->numnormals - numvectors);

    for (i = 0; i < model->numtriangles; i++) {
        T(i).nindices[0] = (GLuint)vectors[3 * T(i).nindices[0] + 0];
        T(i).nindices[1] = (GLuint)vectors[3 * T(i).nindices[1] + 0];
        T(i).nindices[2] = (GLuint)vectors[3 * T(i).nindices[2] + 0];
    }

    /* free space for old normals */
    free(vectors);

    /* allocate space for the new normals */
    model->numnormals = numvectors;
    model->normals = (GLfloat*)malloc(sizeof(GLfloat) *
        3 * (model->numnormals + 1));

    /* copy the optimized vertices into the actual vertex list */
    for (i = 1; i <= model->numnormals; i++) {
        model->normals[3 * i + 0] = copies[3 * i + 0];
        model->normals[3 * i + 1] = copies[3 * i + 1];
        model->normals[3 * i + 2] = copies[3 * i + 2];
    }

    free(copies);
}

/* texcoords */
if (model->numtexcoords) {
    numvectors = model->numtexcoords;
    vectors  = model->texcoords;
    copies = glmOptimizeVectors(vectors, &numvectors);

    printf("glmOptimize(): %d redundant texcoords.\n",
        model->numtexcoords - numvectors);

    for (i = 0; i < model->numtriangles; i++) {
        for (j = 0; j < 3; j++) {
            T(i).tindices[j] = (GLuint)vectors[3 * T(i).tindices[j] + 0];
        }
    }

    /* free space for old texcoords */
    free(vectors);

    /* allocate space for the new texcoords */
    model->numtexcoords = numvectors;
    model->texcoords = (GLfloat*)malloc(sizeof(GLfloat) *
        2 * (model->numtexcoords + 1));

    /* copy the optimized vertices into the actual vertex list */
    for (i = 1; i <= model->numtexcoords; i++) {
        model->texcoords[2 * i + 0] = copies[2 * i + 0];
        model->texcoords[2 * i + 1] = copies[2 * i + 1];
    }

    free(copies);
}
#endif

#if 0
/* look for unused vertices */
/* look for unused normals */
/* look for unused texcoords */
for (i = 1; i <= model->numvertices; i++) {
    for (j = 0; j < model->numtriangles; i++) {
        if (T(j).vindices[0] == i ||
            T(j).vindices[1] == i ||
            T(j).vindices[1] == i)
            break;
    }
}
#endif



// imageloader.h

//Represents an image
class Image {
	public:
		Image(char* ps, int w, int h);
		~Image();

		/* An array of the form (R1, G1, B1, R2, G2, B2, ...) indicating the
		 * color of each pixel in image.  Color components range from 0 to 255.
		 * The array starts the bottom-left pixel, then moves right to the end
		 * of the row, then moves up to the next column, and so on.  This is the
		 * format in which OpenGL likes images.
		 */
		char* pixels;
		int width;
		int height;
};

//Reads a bitmap image from file.
Image* loadBMP(const char* filename);


// imageloader.cpp

using namespace std;

Image::Image(char* ps, int w, int h) : pixels(ps), width(w), height(h) {

}

Image::~Image() {
	delete[] pixels;
}

namespace {
	//Converts a four-character array to an integer, using little-endian form
	int toInt(const char* bytes) {
		return (int)(((unsigned char)bytes[3] << 24) |
					 ((unsigned char)bytes[2] << 16) |
					 ((unsigned char)bytes[1] << 8) |
					 (unsigned char)bytes[0]);
	}

	//Converts a two-character array to a short, using little-endian form
	short toShort(const char* bytes) {
		return (short)(((unsigned char)bytes[1] << 8) |
					   (unsigned char)bytes[0]);
	}

	//Reads the next four bytes as an integer, using little-endian form
	int readInt(ifstream &input) {
		char buffer[4];
		input.read(buffer, 4);
		return toInt(buffer);
	}

	//Reads the next two bytes as a short, using little-endian form
	short readShort(ifstream &input) {
		char buffer[2];
		input.read(buffer, 2);
		return toShort(buffer);
	}

	//Just like auto_ptr, but for arrays
	template<class T>
	class auto_array {
		private:
			T* array;
			mutable bool isReleased;
		public:
			explicit auto_array(T* array_ = NULL) :
				array(array_), isReleased(false) {
			}

			auto_array(const auto_array<T> &aarray) {
				array = aarray.array;
				isReleased = aarray.isReleased;
				aarray.isReleased = true;
			}

			~auto_array() {
				if (!isReleased && array != NULL) {
					delete[] array;
				}
			}

			T* get() const {
				return array;
			}

			T &operator*() const {
				return *array;
			}

			void operator=(const auto_array<T> &aarray) {
				if (!isReleased && array != NULL) {
					delete[] array;
				}
				array = aarray.array;
				isReleased = aarray.isReleased;
				aarray.isReleased = true;
			}

			T* operator->() const {
				return array;
			}

			T* release() {
				isReleased = true;
				return array;
			}

			void reset(T* array_ = NULL) {
				if (!isReleased && array != NULL) {
					delete[] array;
				}
				array = array_;
			}

			T* operator+(int i) {
				return array + i;
			}

			T &operator[](int i) {
				return array[i];
			}
	};
}

Image* loadBMP(const char* filename) {
	ifstream input;
	input.open(filename, ifstream::binary);
	assert(!input.fail() || !"Could not find file");
	char buffer[2];
	input.read(buffer, 2);
    input.ignore(8);
	int dataOffset = readInt(input);

	//Read the header
	int headerSize = readInt(input);
	int width;
	int height;
	width = readInt(input);
			height = readInt(input);
			input.ignore(2);

	//Read the data
	int bytesPerRow = ((width * 3 + 3) / 4) * 4 - (width * 3 % 4);
	int size = bytesPerRow * height;
	auto_array<char> pixels(new char[size]);
	input.seekg(dataOffset, ios_base::beg);
	input.read(pixels.get(), size);

	//Get the data into the right format
	auto_array<char> pixels2(new char[width * height * 3]);
	for(int y = 0; y < height; y++) {
		for(int x = 0; x < width; x++) {
			for(int c = 0; c < 3; c++) {
				pixels2[3 * (width * y + x) + c] =
					pixels[bytesPerRow * y + 3 * x + (2 - c)];
			}
		}
	}

	input.close();
	return new Image(pixels2.release(), width, height);
}


// the helpers file
void rotatePoint(double a[], float theta, double p[])
{
	// convert to radian
	theta = theta * 3.14159f / 180;
	double temp[3];
	temp[0] = p[0];
	temp[1] = p[1];
	temp[2] = p[2];

	temp[0] = -a[2] * p[1] + a[1] * p[2];
	temp[1] = a[2] * p[0] - a[0] * p[2];
	temp[2] = -a[1] * p[0] + a[0] * p[1];

	temp[0] *= sin(theta);
	temp[1] *= sin(theta);
	temp[2] *= sin(theta);

	temp[0] += (1 - cos(theta))*(a[0] * a[0] * p[0] + a[0] * a[1] * p[1] + a[0] * a[2] * p[2]);
	temp[1] += (1 - cos(theta))*(a[0] * a[1] * p[0] + a[1] * a[1] * p[1] + a[1] * a[2] * p[2]);
	temp[2] += (1 - cos(theta))*(a[0] * a[2] * p[0] + a[1] * a[2] * p[1] + a[2] * a[2] * p[2]);

	temp[0] += cos(theta)*p[0];
	temp[1] += cos(theta)*p[1];
	temp[2] += cos(theta)*p[2];

	p[0] = temp[0];
	p[1] = temp[1];
	p[2] = temp[2];

}

void crossProduct(double a[], double b[], double c[])
{
	c[0] = a[1] * b[2] - a[2] * b[1];
	c[1] = a[2] * b[0] - a[0] * b[2];
	c[2] = a[0] * b[1] - a[1] * b[0];
}

void normalize(double a[])
{
	double norm;
	norm = a[0] * a[0] + a[1] * a[1] + a[2] * a[2];
	norm = sqrt(norm);
	a[0] /= norm;
	a[1] /= norm;
	a[2] /= norm;
}

void zoom(double direction[], double center[], double eye[],double speed)
{
	direction[0] = center[0] - eye[0];
	direction[1] = center[1] - eye[1];
	direction[2] = center[2] - eye[2];

	eye[0]    += direction[0] * speed;
	eye[1]    += direction[1] * speed;
	eye[2]    += direction[2] * speed;

	center[0] += direction[0] * speed;
	center[1] += direction[1] * speed;
	center[2] += direction[2] * speed;

}


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
