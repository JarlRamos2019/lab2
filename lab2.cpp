//modified by: Jarl Ramos
//date: 4 September 2022
//
//author: Gordon Griesel
//date: Spring 2022
//purpose: get openGL working on your personal computer
//
#include <iostream>
using namespace std;
#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include <GL/glut.h>
//#include "log.h"
//#include "fonts.h"

const int MAX_PARTICLES = 1000;
const int DEG_IN_CIRCLE = 360;

const char str1[] = "Requirements";
const char str2[] = "Design";
const char str3[] = "Coding";
const char str4[] = "Testing";
const char str5[] = "Maintenance";

//some structures


class Global {
public:
	int xres, yres;
	Global();
} g;
class Box {
public:
	float w;
	float h;
	float dir;
	float vel[2];  //vector2
	float pos[2];
	Box() {
	    w = 20.0f;
	    h = 20.0f;
	    dir = 25.0f;
	    pos[0] = g.xres / 2.0f;
	    pos[1] = g.yres / 2.0f;
	    vel[0] = vel[1] = 0.0f;
	}
	Box(float width, float height, float d, float p0, float p1) {
	    dir = d;
	    w = width;
	    h = height;
	    pos[0] = p0;
	    pos[1] = p1;
	    vel[0] = vel[1] = 0.0;
	}
} box, 
  rec1(50.0f, 10.0f, 0.0f, 60, g.yres - 30),
  rec2(50.0f, 10.0f, 0.0f, 100, g.yres - 60),
  rec3(50.0f, 10.0f, 0.0f, 140, g.yres - 90),
  rec4(50.0f, 10.0f, 0.0f, 180, g.yres - 120),
  rec5(50.0f, 10.0f, 0.0f, 220, g.yres - 150);

enum Quadrant {
    I,
    II,
    III,
    IV
};

Box particles[MAX_PARTICLES];
Box riemannBoxes[DEG_IN_CIRCLE];
int n = 0; //number of elements

class X11_wrapper {
private:
	Display *dpy;
	Window win;
	GLXContext glc;
public:
	~X11_wrapper();
	X11_wrapper();
	void set_title();
	bool getXPending();
	XEvent getXNextEvent();
	void swapBuffers();
	void reshape_window(int width, int height);
	void check_resize(XEvent *e);
	void check_mouse(XEvent *e);
	int check_keys(XEvent *e);
} x11;

//Function prototypes
void make_particle(int x, int y);
void init_opengl(void);
void physics(void);
void build_rectangle(Box rec, GLubyte red, GLubyte green, GLubyte blue);
void build_circle(float rad, float xPosition, float yPosition);
void riemann_init(Box & box, float deg, float radius, float xp, float yp);
void build_riemann(Box box, Quadrant quad);
void rectangle_collision(Box rec, Box & part);
void circle_collision(Box rec, Box & part);
//void text_rect(Rect & rec, int bot, int left, int center, const char string[]);
void render(void);

//=====================================
// MAIN FUNCTION IS HERE
//=====================================
int main()
{
	init_opengl();
	//Main loop
	int done = 0;
	while (!done) {
		//Process external events.
		while (x11.getXPending()) {
			XEvent e = x11.getXNextEvent();
			x11.check_resize(&e);
			x11.check_mouse(&e);
			done = x11.check_keys(&e);
		}
        make_particle(100, g.yres - 10);
		physics();
		render();
        
		x11.swapBuffers();
		usleep(200);
	}
	return 0;
}

Global::Global()
{
	xres = 400;
	yres = 200;
}

X11_wrapper::~X11_wrapper()
{
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
}

X11_wrapper::X11_wrapper()
{
	GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	int w = g.xres, h = g.yres;
	dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		cout << "\n\tcannot connect to X server\n" << endl;
		exit(EXIT_FAILURE);
	}
	Window root = DefaultRootWindow(dpy);
	XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
	if (vi == NULL) {
		cout << "\n\tno appropriate visual found\n" << endl;
		exit(EXIT_FAILURE);
	} 
	Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
	XSetWindowAttributes swa;
	swa.colormap = cmap;
	swa.event_mask =
		ExposureMask | KeyPressMask | KeyReleaseMask |
		ButtonPress | ButtonReleaseMask |
		PointerMotionMask |
		StructureNotifyMask | SubstructureNotifyMask;
	win = XCreateWindow(dpy, root, 0, 0, w, h, 0, vi->depth,
		InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
	set_title();
	glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
	glXMakeCurrent(dpy, win, glc);
}

void X11_wrapper::set_title()
{
	//Set the window title bar.
	XMapWindow(dpy, win);
	XStoreName(dpy, win, "3350 Lab1");
}

bool X11_wrapper::getXPending()
{
	//See if there are pending events.
	return XPending(dpy);
}

XEvent X11_wrapper::getXNextEvent()
{
	//Get a pending event.
	XEvent e;
	XNextEvent(dpy, &e);
	return e;
}

void X11_wrapper::swapBuffers()
{
	glXSwapBuffers(dpy, win);
}

void X11_wrapper::reshape_window(int width, int height)
{
	//window has been resized.
	g.xres = width;
	g.yres = height;
	//
	glViewport(0, 0, (GLint)width, (GLint)height);
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	glOrtho(0, g.xres, 0, g.yres, -1, 1);
}

void X11_wrapper::check_resize(XEvent *e)
{
	//The ConfigureNotify is sent by the
	//server if the window is resized.
	if (e->type != ConfigureNotify)
		return;
	XConfigureEvent xce = e->xconfigure;
	if (xce.width != g.xres || xce.height != g.yres) {
		//Window size did change.
		reshape_window(xce.width, xce.height);
	}
}
//-----------------------------------------------------------------------------

void make_particle(int x, int y)
{
    if (n >= MAX_PARTICLES) {
	return;
    }
    particles[n].w = 2.0;
    particles[n].h = 2.0;
    particles[n].pos[0] = x;
    particles[n].pos[1] = y;
    particles[n].vel[0] = particles[n].vel[1] = 0.0f;
    ++n;

}
void X11_wrapper::check_mouse(XEvent *e)
{
	static int savex = 0;
	static int savey = 0;

	//Weed out non-mouse events
	if (e->type != ButtonRelease &&
		e->type != ButtonPress &&
		e->type != MotionNotify) {
		//This is not a mouse event that we care about.
		return;
	}
	//
	if (e->type == ButtonRelease) {
		return;
	}
	if (e->type == ButtonPress) {
		if (e->xbutton.button==1) {
			//Left button was pressed.
			//int y = g.yres - e->xbutton.y;
			int y = g.yres - e->xbutton.y;
			int x = e->xbutton.x;
			return;
		}
		if (e->xbutton.button==3) {
			//Right button was pressed.
			return;
		}
	}
	if (e->type == MotionNotify) {
		//The mouse moved!
		if (savex != e->xbutton.x || savey != e->xbutton.y) {
			savex = e->xbutton.x;
			savey = e->xbutton.y;
			//Code placed here will execute whenever the mouse moves.


		}
	}
}

int X11_wrapper::check_keys(XEvent *e)
{
	if (e->type != KeyPress && e->type != KeyRelease)
		return 0;
	int key = XLookupKeysym(&e->xkey, 0);
	if (e->type == KeyPress) {
		switch (key) {
			case XK_1:
				//Key 1 was pressed
				break;
			case XK_Escape:
				//Escape key was pressed
				return 1;
		}
	}
	return 0;
}

void init_opengl(void)
{
	//OpenGL initialization
	glViewport(0, 0, g.xres, g.yres);
	//Initialize matrices
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	//Set 2D mode (no perspective)
	glOrtho(0, g.xres, 0, g.yres, -1, 1);
	//Set the screen background color
	glClearColor(0.1, 0.1, 0.1, 1.0);
}

void physics()
{
    //
    // check for collision
    //
    for (int i = 0; i < n; i++) {
        particles[i].vel[1] -= 0.05;
        particles[i].pos[0] += particles[i].vel[0];
        particles[i].pos[1] += particles[i].vel[1];
        rectangle_collision(rec1, particles[i]);
        rectangle_collision(rec2, particles[i]);
        rectangle_collision(rec3, particles[i]);
        rectangle_collision(rec4, particles[i]);
        rectangle_collision(rec5, particles[i]);
        for (int j = 0; j < DEG_IN_CIRCLE; ++j) {
            circle_collision(riemannBoxes[j], particles[i]);
        }
        if (particles[i].pos[1] < 0.0) {
            particles[i] = particles[--n];
        }
    }
}

void build_rectangle(Box rec, GLubyte red, GLubyte green, GLubyte blue)
{
	glPushMatrix();
	glColor3ub(red, green, blue);
	glTranslatef(rec.pos[0], rec.pos[1], 0.0f); //move it somewhere
	glBegin(GL_QUADS);
		glVertex2f(-rec.w, -rec.h);
		glVertex2f(-rec.w,  rec.h);
		glVertex2f( rec.w,  rec.h);
		glVertex2f( rec.w, -rec.h);
	glEnd();
	glPopMatrix();
}

void build_circle(float rad, float xPosition, float yPosition)
{
    int i = 0;
    
    for (int theta = 0; theta < 90; ++theta) {
        riemann_init(riemannBoxes[i], theta, rad, xPosition, yPosition);
        build_riemann(riemannBoxes[i] , I);
        ++i;
    }
    for (int theta = 90; theta > 0; --theta) {
        riemann_init(riemannBoxes[i], theta, rad, xPosition, yPosition);
        build_riemann(riemannBoxes[i] , II);
        ++i;
    }
    for (int theta = 0; theta < 90; ++theta) {
        riemann_init(riemannBoxes[i], theta, rad, xPosition, yPosition);
        build_riemann(riemannBoxes[i] , III);
        ++i;
    }
    for (int theta = 90; theta > 0; --theta) {
        riemann_init(riemannBoxes[i], theta, rad, xPosition, yPosition);
        build_riemann(riemannBoxes[i] , IV);
        ++i;
    }
}
void riemann_init(Box & box, float deg, float radius, float xp, float yp)
{
    box.w = radius * sin(deg * 0.0175f);
    box.h = radius * cos(deg * 0.0175f);
    box.dir = 0.0f;
    box.pos[0] = xp;
    box.pos[1] = yp;
    box.vel[0] = box.vel[1] = 0;
}
void build_riemann(Box box, Quadrant quad)
{
    switch (quad) {
        case (I):
            break;
        case (II):
            box.w = -box.w;
            break;
        case (III):
            box.w = -box.w;
            box.h = -box.h;
            break;
        case (IV):
            box.h = -box.h;
            break;
    }
    build_rectangle(box, 70, 100, 80);
}
void rectangle_collision(Box rec, Box & part)
{
    if ((part.pos[1] - part.h) < (rec.pos[1] + rec.h) &&
        (part.pos[1] + part.h) > (rec.pos[1] - rec.h) &&
	    (part.pos[0] + part.w) > (rec.pos[0] - rec.w) &&
	    (part.pos[0] - part.w) < (rec.pos[0] + rec.w)) {
	    part.vel[1] = 0.00;
	    part.vel[0] += 0.01;
	}
}
void circle_collision(Box rec, Box & part)
{
    if ((part.pos[1] - part.h) < (rec.pos[1] + rec.h) &&
        (part.pos[1] + part.h) > (rec.pos[1] - rec.h) &&
        (part.pos[0] + part.w) > (rec.pos[0] - rec.w) &&
        (part.pos[0] - part.w) < (rec.pos[0] + rec.w)) {
        part.vel[1] = 0.00;
        part.vel[0] -= 0.01;
    }
}
/*
void text_rect(Rect & rec, int bot, int left, int center, const char string[])
{
    rec.bot    = bot;
    rec.left   = left;
    rec.center = center;
    
    ggprint8b(&rec, 16, 0, string[]);
    
}
*/
void render()
{
	glClear(GL_COLOR_BUFFER_BIT);
	//Draw box.
    
    /*
    Rect r1;
    Rect r2;
    Rect r3;
    Rect r4;
    Rect r5;
     
    text_rect(r1, g.yres - 30 - rec1.h, rec1.pos[0] - rec1.w, 0, str1);
    text_rect(r1, g.yres - 60 - rec1.h, rec1.pos[0] - rec1.w, 0, str2);
    text_rect(r1, g.yres - 90 - rec1.h, rec1.pos[0] - rec1.w, 0, str3);
    text_rect(r1, g.yres - 120 - rec1.h, rec1.pos[0] - rec1.w, 0, str4);
    text_rect(r1, g.yres - 150 - rec1.h, rec1.pos[0] - rec1.w, 0, str5);
    */

	build_rectangle(rec1, 150, 225, 160);
	build_rectangle(rec2, 150, 225, 160);
	build_rectangle(rec3, 150, 225, 160);
	build_rectangle(rec4, 150, 225, 160);
	build_rectangle(rec5, 150, 225, 160);
    build_circle(70.0f, g.xres - 60.0f, -20.0f);
    
	for (int i = 0; i < n; i++) {
	    glPushMatrix();
	    glColor3ub(150, 160, 255);
	    glTranslatef(particles[i].pos[0], particles[i].pos[1], 0.0f);
	    glBegin(GL_QUADS);
		    glVertex2f(-particles[i].w, -particles[i].w);
		    glVertex2f(-particles[i].w,  particles[i].w);
		    glVertex2f( particles[i].w,  particles[i].w);
		    glVertex2f( particles[i].w, -particles[i].w);
	    glEnd();
	    glPopMatrix();
	}

}






