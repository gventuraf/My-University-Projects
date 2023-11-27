/*******************************************************
**                                                     *
**                                                     *
** author:              Goncalo Ventura Ferreira       *
** UC student number:   2017276264                     *
** DEI mail:            gventuraf99@student.dei.uc.pt  *
**                                                     *
**                                                     *
*******************************************************/

/**
//? How to Compile & Run on the command line (Linux)
** you need to link some stuff
*** compile:
** g++ project.cpp -o project -lGL -lGLU -lglut -lm
**     -L/usr/lib/x86_64-linux-gnu/libGLEW.so -lGLEW
*** run:
**      ./project
*/

#include <stdio.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include <fstream>
#include "RgbImage.h"

// OpenGL
#include <GL/glew.h>
#include <GL/freeglut.h>
#pragma comment(lib,"glew32.lib")
#pragma comment(lib,"glu32.lib")
#pragma comment(lib,"opengl32.lib")

//***

#define BLACK3  0.0, 0.0, 0.0
#define PI      3.14159
#define WHITE3  1.0, 1.0, 1.0
#define RED3    1.0, 0.0, 0.0
#define GREEN3  0.0, 1.0, 0.0
#define YELLOW3 1.0, 1.0, 0.0
#define BLUE3   0.0, 0.0, 1.0
//#define WORLD_COLOR 0.0, 0.0, 0.0, 1.0
#define WORLD_COLOR 1.0, 138/255.0, 101/255.0, 1.
//***

void initMaterials(int);

//* things for drawing a cube

static std::vector<std::vector<GLfloat>> vertices = {
    //* top, ordered so it's facing up
    {0.5, 1, 0.5},
    {0.5, 1, -0.5},
    {-0.5, 1, -0.5},
    {-0.5, 1, 0.5},
    //* bottom, ordered so it's facing down
    {0.5, 0, 0.5},
    {-0.5, 0, 0.5},
    {-0.5, 0, -0.5},
    {0.5, 0, -0.5}
}; //* cube of radius 1, from y=0 up, around y
static std::vector<std::vector<GLfloat>> normals = {
    {0, 1, 0},    // top
    {0, -1, 0},   // bot
    {1, 0, 0},    // right
    {-1, 0, 0},   // left
    {0, 0, 1},    // front
    {0, 0, -1}    // back
};
static std::vector<std::vector<GLuint>> faces = {
    {0, 1, 2, 3}, // top
    {4, 5, 6, 7}, // bot
    {0, 4, 7, 1}, // right
    {2, 6, 5, 3}, // left
    {0, 3, 5, 4}, // front
    {1, 7, 6, 2}  // back
};
static std::vector<std::vector<GLfloat>> texcoord_wrap = {
    {1,0, 1,1, 0,1, 0,0}, // top
    {1,0, 0,0, 0,1, 1,1}, // bot
    {0,1, 0,0, 0.25,0, 0.25,1}, // right
    {0.5,1, 0.5,0, 0.75,0, 0.75,1}, // left
    {1,1, 0.75,1, 0.75,0, 1,0}, // front
    {0.25,1, 0.25,0, 0.5,0, 0.5,1} // back
};

//* Window

GLint wScreen = 1920,
      hScreen = 1080;

//* World

GLfloat world_x = 50.0,
        world_y = 50.0,
        world_z = 50.0;

//* Visualization / Observer

GLfloat rVisao = 3.0, aVisao = 0.5 * PI, incVisao = 0.1, height_inc=0.1,
        angPersp = 109.0,
        obsPini[] = { 1, 7.8, 0.5f * world_x - 6 },
        obsPfin[] = { obsPini[0] - rVisao * (float)cos(aVisao),
                      5.3,
                      obsPini[2] - rVisao * (float)sin(aVisao)
                  };

GLfloat cam_zoom = 90,
        cam_zoom_inc = 5,
        cam_zoom_min = 5,
        cam_zoom_max = 150;

//* Transparency and Materials
GLfloat alpha = 0.0f;
GLuint texture[3];
RgbImage imag;
GLuint tex_loc, obspos_loc, ambientcolor_loc;

//** Ambient light
GLfloat day_intensity = 0.5;
GLfloat global_light_color[4] = { day_intensity, day_intensity, day_intensity, 1.0 };

//* Spotlight .. Focus light
GLfloat slight_ads[4] = { 1, 1, 1, 1 },
        slight_pos[4] = { 0, 4.0, 0, 1 },
        slight_dir[3] = { 0, -4.0, 0 },
        slight_cutoff = 30.0;
GLint   slight_is_on = 1;

//* animation
GLfloat theta = 0.0f,
        theta_inc = 5.0f,
        timer_msec = 150.0f;

//* shaders stuff

char filenameV[] = "Vshader.txt";
char filenameF[] = "Fshader.txt";
GLuint VertexShader, FragmentShader, ShaderProgram;
char *VertexShaderSource, *FragmentShaderSource;

//***

char* readShaderFile(char* FileName)
{
    char* DATA = NULL;
	int   flength = 0;
	FILE* filepoint;

	filepoint = fopen(FileName, "r");
	if (filepoint) {
		fseek(filepoint, 0, SEEK_END);
		flength = ftell(filepoint);
		rewind(filepoint);

		DATA = (char*)malloc(sizeof(char) * (flength + 1));
		flength = fread(DATA, sizeof(char), flength, filepoint);

		DATA[flength] = '\0';
		fclose(filepoint);
		return DATA;
	}
	else {
		printf(" --------  Error while reading  %s ", FileName);
		return NULL;
	}
}
void BuildShader(void)
{
    //* criar
	VertexShader = glCreateShader(GL_VERTEX_SHADER);
	FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	VertexShaderSource = readShaderFile(filenameV);
	FragmentShaderSource = readShaderFile(filenameF);

	const char* VS = VertexShaderSource;
	const char* FS = FragmentShaderSource;
	glShaderSource(VertexShader, 1, &VS, NULL);
	glShaderSource(FragmentShader, 1, &FS, NULL);
	free(VertexShaderSource);
	free(FragmentShaderSource);

	//* compilar
	glCompileShaderARB(VertexShader);
	glCompileShaderARB(FragmentShader);

	//* criar & linkar
	ShaderProgram = glCreateProgramObjectARB();
	glAttachShader(ShaderProgram, VertexShader);
	glAttachShader(ShaderProgram, FragmentShader);
	glLinkProgram(ShaderProgram);

	//* usar
	glUseProgramObjectARB(ShaderProgram);
}
void InitShader(void)
{
	BuildShader(); //* criar & linkar
}
void DeInitShader(void)
{
	glDetachShader(ShaderProgram, VertexShader);
	glDetachShader(ShaderProgram, FragmentShader);
	glDeleteShader(ShaderProgram);
}

void load_texture(const char* name, int i)
{
    //* Texture glass
	glGenTextures(1, texture + i);
	glBindTexture(GL_TEXTURE_2D, texture[i]);
	imag.LoadBmpFile(name);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D, 0, 3,
		imag.GetNumCols(),
		imag.GetNumRows(), 0, GL_RGB, GL_UNSIGNED_BYTE,
		imag.ImageData());
}

void init_light()
{
    //* Ambient
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_light_color);
    //* Spotlight (focus)
	glLightfv(GL_LIGHT0, GL_AMBIENT,  slight_ads);
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  slight_ads);
	glLightfv(GL_LIGHT0, GL_SPECULAR, slight_ads);
    glLightfv(GL_LIGHT0, GL_POSITION, slight_pos);
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, slight_dir);
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, slight_cutoff);
}

void init()
{
    glClearColor(WORLD_COLOR);
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
    //* load textures
    load_texture("Textures/dark_wood.bmp", 0);
    load_texture("Textures/metal.bmp", 1);
    load_texture("Textures/cow.bmp", 2);
    //* lights
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    init_light();
}

/** sides=1 ; from y=0 to y=1 ; around y axis*/
void drawCube(GLfloat dx=1, GLfloat dy=1, GLfloat dz=1, short int tex=0, GLboolean no_top=false)
{
    glScalef(dx, dy, dz);
    std::vector<std::vector<GLfloat>> *v = &texcoord_wrap;

    if (tex >= 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture[tex]);
    }

    for (int i=0; i<6; ++i) {
        if (no_top && !i) continue;

        glBegin(GL_QUADS);
            glNormal3f(normals[i][0], normals[i][1], normals[i][2]);

            if (tex >= 0) glTexCoord2f((*v)[i][0], (*v)[i][1]);
            glVertex3f(vertices[faces[i][0]][0], vertices[faces[i][0]][1], vertices[faces[i][0]][2]);

            if (tex >= 0) glTexCoord2f((*v)[i][2], (*v)[i][3]);
            glVertex3f(vertices[faces[i][1]][0], vertices[faces[i][1]][1], vertices[faces[i][1]][2]);

            if (tex >= 0) glTexCoord2f((*v)[i][4], (*v)[i][5]);
            glVertex3f(vertices[faces[i][2]][0], vertices[faces[i][2]][1], vertices[faces[i][2]][2]);

            if (tex >= 0) glTexCoord2f((*v)[i][6], (*v)[i][7]);
            glVertex3f(vertices[faces[i][3]][0], vertices[faces[i][3]][1], vertices[faces[i][3]][2]);

        glEnd();
    }
    if (tex >= 0) glDisable(GL_TEXTURE_2D);
}

void Timer(int value)
{
    theta += theta_inc;
    if (theta < 360) {
        slight_dir[0] = (GLfloat)cos(theta);
        slight_dir[1] = -4.0f;
        slight_dir[2] = (GLfloat)sin(theta);
        glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, slight_dir);
        glutTimerFunc(timer_msec, Timer, 0);
    }
    else {
        slight_dir[0] = 0.0f;
        slight_dir[1] = -4.0f;
        slight_dir[2] = 0.0f;
        glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, slight_dir);
    }
	glutPostRedisplay();
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, wScreen, hScreen);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
    gluPerspective(cam_zoom, (float)wScreen / hScreen, 0.1, 3 * world_z);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
    gluLookAt(obsPini[0], obsPini[1], obsPini[2],
              obsPfin[0], obsPfin[1], obsPfin[2],
              0, 1, 0);
    //*
    if (theta < 4.0) glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, slight_dir);
    //*
    obspos_loc = glGetUniformLocation(ShaderProgram, "obs_pos");
    glUniform3f(obspos_loc, obsPini[0], obsPini[1], obsPini[2]);
    tex_loc = glGetUniformLocation(ShaderProgram, "tex");
    ambientcolor_loc = glGetUniformLocation(ShaderProgram, "ambient_color");
    glUniform3fv(ambientcolor_loc, 3, global_light_color);
    //*
    //*
    //* draw
    //initMaterials(16);

    //*
    //* floor
    //initMaterials(16);
    glUniform1i(tex_loc, -1);
    initMaterials(5);
    glPushMatrix();
        glColor4f(BLUE3, 1.0);
        drawCube(20, 1, 20, -1);
    glPopMatrix();
    //*
    //* sphere
    initMaterials(4);
    glPushMatrix();
        glTranslatef(0.0, 2.0, 0.0);
        gluSphere(gluNewQuadric(), 1, 36, 36);
    glPopMatrix();
    //*
    //* cube 1 - wood
    glUniform1i(tex_loc, 0);
    initMaterials(9);
    glColor3f(WHITE3);
    glPushMatrix();
        glTranslatef(-4.0, 1.01, 0.0);
        drawCube(2.0, 2.0, 2.0, 0);
    glPopMatrix();
    //*
    //* cube 2 - metal
    initMaterials(11);
    glPushMatrix();
        glTranslatef(4.0, 1.01, 0.0);
        drawCube(2.0, 2.0, 2.0, 1);
    glPopMatrix();
    //*
    //* cube 3 - cow
    initMaterials(16);
    glPushMatrix();
        glTranslatef(0.0, 2.01, -4.0);
        glActiveTexture(GL_TEXTURE0);
        drawCube(2.0, 2.0, 2.0, 2);
    glPopMatrix();


    //*
    //*
    glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case 'a':
        cam_zoom -= cam_zoom_inc;
        if (cam_zoom < cam_zoom_min) cam_zoom = cam_zoom_min;
        glutPostRedisplay();
        break;
    case 'A':
        cam_zoom += cam_zoom_inc;
        if (cam_zoom > cam_zoom_max) cam_zoom = cam_zoom_max;
        glutPostRedisplay();
        break;
    case 'u': case 'U':
        obsPini[1] = obsPini[1] + (key=='u' ? -0.2 : 0.2);
        glutPostRedisplay();
        break;
    case 'i': case 'I':
        obsPfin[1] = obsPfin[1] + (key=='i' ? -0.1 : 0.1);
        glutPostRedisplay();
        break;
    case 's': case 'S':
        slight_is_on = !slight_is_on;
        if (slight_is_on) glEnable(GL_LIGHT0);
        else glDisable(GL_LIGHT0);
        glutPostRedisplay();
        break;
    case 'b': case 'B':
        day_intensity = day_intensity + (key=='b' ? -0.1 : 0.1);
        if (day_intensity < 0) day_intensity = 0; else if (day_intensity > 1) day_intensity = 1;
        global_light_color[0] = global_light_color[1] = global_light_color[2] = day_intensity;
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_light_color);
        glutPostRedisplay();
        break;
    case 'q':
        theta = 0.0;
        glutTimerFunc(timer_msec, Timer, 0);
        glutPostRedisplay();
        break;
    case 27:
        exit(0);
    }
}

void keysNotAscii(int key, int x, int y)
{
    if (key == GLUT_KEY_UP) {
		obsPini[0] = obsPini[0] + incVisao * cos(aVisao);
		obsPini[2] = obsPini[2] - incVisao * sin(aVisao);
	}
	if (key == GLUT_KEY_DOWN) {
		obsPini[0] = obsPini[0] - incVisao * cos(aVisao);
		obsPini[2] = obsPini[2] + incVisao * sin(aVisao);
	}
	if (key == GLUT_KEY_LEFT)
		aVisao = (aVisao + 0.1);
	if (key == GLUT_KEY_RIGHT)
		aVisao = (aVisao - 0.1);

	obsPfin[0] = obsPini[0] + rVisao * cos(aVisao);
	obsPfin[2] = obsPini[2] - rVisao * sin(aVisao);
	glutPostRedisplay();
}

void reshapeWin(int w, int h)
{
    wScreen = w;
    hScreen = h;
    glutPostRedisplay();
}

int main(int argc, char** argv) {

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(wScreen, hScreen);
	glutInitWindowPosition(1920, 0);
	glutCreateWindow("Project 3");

	init();

    //* glew
    GLenum err = glewInit();
    InitShader();
    DeInitShader();

	glutSpecialFunc(keysNotAscii);
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
    glutReshapeFunc(reshapeWin);

	glutMainLoop();

	return 0;
}
