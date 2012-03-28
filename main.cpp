/***************************************************************************
 *   Copyright (C) 2010 by Guillaume Saby   *
 *   ggs0ot@aber.ac.uk   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/*
* -------------------------------------------------------------------------------
 |			API Hemispherium : Example Application			|
 | This application demostrate the main aspect of the API :			|
 |	-OpenGL scene : display of Quad as basic "windows"			|
 |	-Video playing								|
 |	-Image display								|
 --------------------------------------------------------------------------------*/

#define NO_POINTER

#include "config.h"
#include "Tracknect.h"

//SDL used for event managing
//#include <SDL.h>

//Glut is used for font rendering only
#ifdef HAVE_GLUT_GLUT_H
# include <GLUT/glut.h>
#elif defined(HAVE_GL_GLUT_H)
# include <GL/glut.h>
#endif

#include <iostream>
#include <cstdlib>
#include <cstddef>
#include <cstdio>
#include <cassert>
#include <sys/types.h>
#include <dirent.h>

#include <vector>
#include <string>

//Here is the include for the API
#include "DeviceInput.h"
#include "Display.h"



#define GL_WIN_SIZE_X 720
#define GL_WIN_SIZE_Y 480

bool kinectError;


void glutDisplay (void)
{

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Setup the OpenGL viewpoint
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glDisable(GL_TEXTURE_2D);

	TnDisplay();

	glutSwapBuffers();
}


void glutIdle (void)
{
	// Display the frame
	glutPostRedisplay();
}


void glInit (int * pargc, char ** argv)
{
	glutInit(pargc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(GL_WIN_SIZE_X, GL_WIN_SIZE_Y);
	int f1 = glutCreateWindow ("Kinect");

	//glutFullScreen();
	glutSetCursor(GLUT_CURSOR_NONE);

	//glutKeyboardFunc(glutKeyboard);
	glutDisplayFunc(glutDisplay);
	glutIdleFunc(glutIdle);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	glEnableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}





namespace {
//lists containing the scanned files
std::vector<std::string> video;
std::vector<std::string> image;

//Angle of each "windows" according to the direction of the camera (1,0,0);
GLint angleMenu = 0;
GLint angleVideo = -90;
GLint angleImage = 90;

GLubyte selection = 0; 	//0 menu, 1 image, 2 video
GLubyte menuEntry = 0; 	//entry of the menu actually highlighted (0...3)
GLubyte menuOffsetImage = 0;	//offset of menu for more than 3 files
GLubyte menuOffsetVideo = 0;

//This angle are define by pointing the center of the quad and pressing right button
double offsetH = 0;//Horizontal offset angle
double offsetV = 0;//Vertical offset Angle

//Boolean used to played the rotating animation
bool leftRot = false;
bool rightRot = false;
//If this true, an image is currently displayed. Used to avoid quitting the app by pressing exc
bool imageDisplayed = false;
//used for listen the phone
#ifndef NO_POINTER
Hemi::DeviceInput *device;
#endif

//This is for the infinite loop of the program
bool ok = true;
bool debug = true;

GLdouble mouseX;
GLdouble mouseY;


double angleH = 0, angleV = 0;

Hemi::Display* pEssai;
}

void funcRender();
void scanFolders();
void genereMenuTex();
void drawText(const float& x, const float& y, const float& z, const std::string& text, const float& factor);

void drawMenu();
void drawPointer();
void drawMenuImage();
void drawMenuVideo();

#ifndef NO_POINTER
void grabPhoneInput();
#endif

void selectHandler();

int main(int argc, char** argv) {

	kinectError = TnInitialization(TN_MODE_HAND_TRACKING);

	if (kinectError)
		printf("Error while initializing kinect \n");

	glInit(&argc, argv);


#ifndef NO_POINTER
	device = new Hemi::DeviceInput(4);
	device->startListening();
#endif

	//Here the display class is created as "debug mode", aka windowed mode.
	//change to false to activate the Hemispherium Mode
	Hemi::Display essai(debug);
	pEssai = &essai;

	int tempsActuel, tempsPrecedent;

	//This function initialize the fisheye renderer of the class.
	essai.initOglRender();
	//Define the function that the renderer will use as an entry point for the scene rendering
	essai.setRenderFunc(funcRender);
	//position the camera in the ogl scene. gluLookAt-like.
	essai.setCameraCoordinates(0,0,0,1,0.36,0,0,1,0);
	//set the clear color of our scene.
	essai.setClearColor(0.0,0.0,0.0);

	SDL_Event event;

	tempsActuel = SDL_GetTicks();
	tempsPrecedent = tempsActuel;

	//We scan the current folder to grab video and image files.
	scanFolders();

	while(ok)
	{
		if (!kinectError)
			TnProcess();

		while( SDL_PollEvent( &event ) )
		{
			switch( event.type )
			{
			   case SDL_QUIT:
				  exit(0);
				case SDL_KEYDOWN:
					switch( event.key.keysym.sym ){
						case SDLK_ESCAPE:
							if(!imageDisplayed)
								ok = false;
							else
							{//if an image is displayed
								imageDisplayed = false;
								//we reactivate the scene rendering
								essai.redisplayScene();
							}
							break;
						case SDLK_LEFT:
						  if(imageDisplayed)
						      essai.moveImageX(0.001);
						  else{
							if(!rightRot && angleMenu != -90)
								leftRot = true;
						  }
							break;
						case SDLK_RIGHT:
						  if(imageDisplayed)
							essai.moveImageX(-0.001);
						  else{
							if(!leftRot && angleMenu != 90)
								rightRot = true;
						  }
							break;
						case SDLK_UP:
							if(imageDisplayed)
							      essai.moveImageY(0.001);
							else{
							if(menuEntry > 0)
								menuEntry--;
							}
							break;
						case SDLK_DOWN:
						   if(imageDisplayed)
							essai.moveImageY(-0.001);
						   else{
							if(menuEntry < 4)
								menuEntry++;
						   }
							break;
						case SDLK_RETURN:
							if(selection != 0)
								selectHandler();
							break;
						case SDLK_t:
							if(imageDisplayed)
							  essai.rotationImage(2.0);
							break;
						case SDLK_r:
							if(imageDisplayed)
							{
							  essai.rotationImage(-2.0);
							  XnPoint3D vector;
							  vector = TnGetTrackedObjectCoordonates();
							  printf("%ld ",vector.X);

							  TnStartTracking();
							}
							break;
						case SDLK_z:
							if(imageDisplayed)
							  essai.scaleImage(-0.001);
							break;
						case SDLK_x:
							if(imageDisplayed)
							  essai.scaleImage(0.001);
							break;
						case SDLK_c:
							essai.changeFilter(true);
							break;
						case SDLK_v :
							essai.changeFilter(false);
							break;
						case SDLK_p :
							essai.setTexDefaulft();
							break;
						default:
							break;
					}
				break;

				default:
					break;
			}
		}


		if (!kinectError)
		{

			// Affichage des images d'aide à la calibration :

			if (TnGetCalibrationState() == TN_NOT_CALIBRATED)
			{
				if (TnGetNbUsers() < 1)
				{
					essai.changeTex(const_cast<char*>("calibration/nouserfound.bmp"));
					imageDisplayed = true;
				}

				else
				{
					essai.changeTex(const_cast<char*>("calibration/userfound.bmp"));
					imageDisplayed = true;
				}
			}

			if (TnGetCalibrationState() == TN_CALIBRATION_STARTED)
			{
				essai.changeTex(const_cast<char*>("calibration/calibrationstarted.bmp"));
				imageDisplayed = true;
			}

			if (TnGetCalibrationState() == TN_CALIBRATED)
			{
				static int i = 0;
				if (i==0)
				{
					imageDisplayed = false;
					essai.redisplayScene();
					i++;
				}
			}


			if (TnGetCalibrationState() == TN_CALIBRATED)
			{
				int tnstate = TnGetTrackingState();

				if(imageDisplayed)
				{
					double normeVector = sqrt(Tn_X()*Tn_X() + Tn_Y()*Tn_Y() + Tn_Z()*Tn_Z());


					if (tnstate == TN_STATE_RIGHT_HAND_TRACKED)
					{

						if (Tn_X()>0)
							essai.rotationImage(Tn_X()/50);
						else if (Tn_X()<0)
							essai.rotationImage(Tn_X()/50);
					}

					if (tnstate == TN_STATE_LEFT_HAND_TRACKED)
					{
						if (Tn_Y()>200)
							essai.moveImageX(0.01);
						else{
							if(!rightRot && angleMenu != -90)
								leftRot = true;
						  }
						if (Tn_Y()<0)
							essai.moveImageX(-0.01);
						else{
							if(!leftRot && angleMenu != 90)
								rightRot = true;
						  }

						if (Tn_X()>200)
							essai.moveImageY(0.01);
						else{
							if(menuEntry > 0)
								menuEntry--;
							}
						if (Tn_X()<-200)
							essai.moveImageY(-0.01);
						else{
							if(menuEntry < 4)
								menuEntry++;
						   }
					}

					if (tnstate == TN_STATE_BOTH_HANDS_TRACKED)
					{
						if (normeVector>300)
							essai.scaleImage(-0.01);
						if (normeVector<300)
							essai.scaleImage(0.01);
					}

					if (tnstate == TN_STATE_HANDS_LOST)
						essai.setTexDefaulft();

				}


				else // !imageDisplayed
				{
					menuEntry = 1;

					if (tnstate == TN_STATE_RIGHT_HAND_TRACKED)
					{
						int a = Tn_X();
						if (Tn_X()>100)
						{
							if(!rightRot && angleMenu != -90)
								leftRot = true;
						}
						else if (Tn_X()<-100)
						{
							if(!leftRot && angleMenu != 90)
								rightRot = true;
						}

						if (Tn_Y()>0 && Tn_Y()<=100)
							menuEntry = 4;
						else if (Tn_Y()>100 && Tn_Y()<=200)
							menuEntry = 3;
						else if (Tn_Y()>200 && Tn_Y()<=300)
							menuEntry = 2;
						else if (Tn_Y()>300 && Tn_Y()<=400)
							menuEntry = 1;
						else if (Tn_Y()>400 && Tn_Y()<=500)
							menuEntry = 0;
					}

					if (tnstate == TN_STATE_BOTH_HANDS_TRACKED)
					{
						if(selection != 0)
							selectHandler();
					}
				}
			}
		}

#ifndef NO_POINTER
		grabPhoneInput();
#endif

		if(leftRot)
		{//if leftRot true, rotation animation to the left activate
			angleMenu  -=2;
			angleImage -=2;
			angleVideo -=2;

			if(angleMenu == -90)
			{//once 90 is reached, animation is stop ans we are on the image menu
				leftRot = false;
				selection = 1;
			}
			else if(angleMenu == 0)
			{//if 0 is reach we are back to the main window
				leftRot = false;
				selection = 0;
			}
		}
		else if(rightRot)
		{
			angleMenu  +=2;
			angleImage +=2;
			angleVideo +=2;

			if(angleMenu == 90)
			{//if -90 is reach, video menu
				rightRot = false;
				selection = 2;
			}
			else if(angleMenu == 0)
			{
				rightRot = false;
				selection = 0;
			}
		}

		tempsActuel = SDL_GetTicks();

		if (tempsActuel - tempsPrecedent > 30)
		{
			tempsPrecedent = tempsActuel;
			essai.update();
		}
		else
		{
			SDL_Delay(30 - (tempsActuel - tempsPrecedent));
		}
	}

	exit(0);
}

/*This function just scan the current folder and grab the files that interest us.
It's not part of the API so it will not be explain*/
void scanFolders()
{
	DIR *dp;
	struct dirent *ep;

	dp = opendir ("./");
	if (dp != NULL)
	{
  	   while ((ep = readdir (dp)))
		{
			const std::string temp(ep->d_name);
			size_t found;

			found=temp.find_last_of('.');

			if(found != std::string::npos)
			{
				std::string ext = temp.substr(found+1);

				if(ext == "avi" || ext == "flv" || ext == "mov" || ext == "mp4")
					video.push_back(temp);
				else if(ext == "bmp" || ext == "jpeg" || ext == "jpg")
					image.push_back(temp);
			}
		}
		(void) closedir (dp);
	}
	else exit(2);

}

//Simple utility function using glut to draw a text at x,y,z
void drawText(const float& x, const float& y, const float& z, const std::string& text, const float& factor)
{
	glPushMatrix();

	glTranslatef(x,y,z);
	glRotated(90,0,1,0);
	glRotated(-180,1,0,0);

	glLineWidth(20);

	glScaled(factor,factor,factor);

	for (std::string::const_iterator c=text.begin(); c != text.end(); ++c) {
		glutStrokeCharacter(GLUT_STROKE_ROMAN , *c);
	}
	glPopMatrix();
}

/*
	This is our main render function. It will be called at each Display.update().
	Avoid calling clear, or changing the matrix mode etc...
	gluLookAt should work, as a meaning of placing object
	but in the end the point of view will be the one define by setCameraCoordinates
*/
void funcRender()
{
	drawMenu();
	//drawPointer();
	drawMenuImage();
	drawMenuVideo();

	glPopMatrix();
}

void drawPointer()
{
	glPushMatrix();

	double angleHOK = angleH-offsetH;
	double angleVOK = angleV-offsetV;

	if(angleHOK < 0)
		angleHOK = 360 + angleHOK;

	if(angleVOK < 0)
		angleVOK = 360 + angleVOK;

	/*Menu*/
	glRotated(angleHOK, 0,1,0);
	glRotated(angleVOK, 0,0,1);
	glTranslated(2.5,0.0,0.0);

	glBegin(GL_QUADS);
	glColor3f(0.0,0.0,0.4);
	glVertex3f(0,0.2,0.2);
	glColor3f(0.0,0.0,0.6);
	glVertex3f(0,-0.2,0.2);
	glColor3f(0.0,0.0,0.8);
	glVertex3f(0,-0.2,-0.2);
	glColor3f(0.0,0.0,1.0);
	glVertex3f(0,0.2,-0.2);
	glEnd();

	glBegin(GL_LINES);
	  glVertex2i(0,0);glVertex2i(0,1);
	  glVertex2i(0,0);glVertex2i(1,0);
	  glVertex2i(0,0);glVertex3i(0,0,1);
	glEnd();

	glPopMatrix();
}

/*Draw the main windows*/
void drawMenu()
{
	glPushMatrix();

	/*Menu*/
	glRotated(angleMenu, 0,1,0);
	glTranslated(3.0,0.0,0.0);

	glBegin(GL_QUADS);
	glColor3f(0.0,0.0,0.4);
	glVertex3f(0,2,2);
	glColor3f(0.0,0.0,0.6);
	glVertex3f(0,-2,2);
	glColor3f(0.0,0.0,0.8);
	glVertex3f(0,-2,-2);
	glColor3f(0.0,0.0,1.0);
	glVertex3f(0,2,-2);
	glEnd();

	glColor3f(1.0,0.0,0.0);
	drawText(-0.1,-1.8,1.8,"Hemi Media Player", 0.003);
	drawText(-0.1,-0.6,1.8,"Press right for Videos", 0.002);
	drawText(-0.1,0.8,1.8,"Press left for Images", 0.002);

	glPopMatrix();
}

/*Draw the image menu*/
void drawMenuImage()
{
	glPushMatrix();

	glRotated(angleImage, 0,1,0);
	glTranslated(3.0,0.0,0.0);

	glBegin(GL_QUADS);
	glColor3f(0.0,0.4,0.0);
	glVertex3f(0,2,2);
	glColor3f(0.0,0.6,0.0);
	glVertex3f(0,-2,2);
	glColor3f(0.0,0.8,0.0);
	glVertex3f(0,-2,-2);
	glColor3f(0.0,1.0,0.0);
	glVertex3f(0,2,-2);
	glEnd();

	glColor3f(0.0,0.0,1.0);
	drawText(-0.1,-1.8,1.8,"Image", 0.004);

	int max = 0;

	/*if the size is greater than the display slots (3)*/
	if(image.size() > 3u+(menuOffsetImage*3u))
		max = 3;//we display only the 3+offset
	else if (image.size())
	  //we display the number of entry from the offset to the end of the list
		max = (3+(menuOffsetImage*3) - image.size())+1;

	for(int i = 0; i < max; i++)
	{
		std::string t = image.at(i+(menuOffsetImage*3));

		drawText(-0.1, -0.5+(i*0.7)+0.2,1.8,const_cast<char*>(t.c_str()), 0.002);
	}

	if(max == 3)
		drawText(-0.1, 1.8,1.8,"next...", 0.002);

	if(menuOffsetImage)
		drawText(-0.1, -1.0,1.8,"...previous", 0.002);

	if(selection ==1)
	{//if actually on image menu, draw selection quad

		glColor4f(1.0,1.0,1.0,1.0);
		glEnable(GL_BLEND);
		glBlendFunc(GL_DST_COLOR,GL_ONE_MINUS_DST_COLOR);
		glTranslated(0,0.7+(menuEntry*0.7),0);

		glBegin(GL_QUADS);
		glVertex3f(-0.1,-1.3,2);
		glVertex3f(-0.1,-2,2);
		glVertex3f(-0.1,-2,-2);
		glVertex3f(-0.1,-1.3,-2);
		glEnd();
		glDisable(GL_BLEND);
	}

	glPopMatrix();
}

/*draw the video menu, same as image menu - this should be sharing code properly (ajw) */
void drawMenuVideo()
{
	/*Video*/
	glPushMatrix();

	glRotated(angleVideo, 0,1,0);
	glTranslated(3.0,0.0,0.0);

	glBegin(GL_QUADS);
	glColor3f(0.4,0.0,0.0);
	glVertex3f(0,2,2);
	glColor3f(0.6,0.0,0.0);
	glVertex3f(0,-2,2);
	glColor3f(0.8,0.0,0.0);
	glVertex3f(0,-2,-2);
	glColor3f(1.0,0.0,0.0);
	glVertex3f(0,2,-2);
	glEnd();

	glColor3f(0.0,0.0,1.0);
	drawText(-0.1,-1.8,1.8,"Video", 0.004);

	int max = 0;

	if(video.size() > 3u+(menuOffsetVideo*3u))
		max = 3;
	else if (video.size())
	  max = (3+(menuOffsetVideo*3) - (video.size()+1));

	for(int i = 0; i < max; i++)
	{
		const std::string& t = video.at(i+(menuOffsetVideo*3));

		drawText(-0.2, -0.5+(i*0.7)+0.2,1.8,t, 0.002);
	}

	if(max == 3)
		drawText(-0.1, 1.8,1.8,"next...", 0.002);

	if(selection ==2)
	{//if actually on video menu, draw selection quad

		glColor4f(1.0,1.0,1.0,0.0);
		glEnable(GL_BLEND);
		glBlendFunc(GL_DST_COLOR,GL_ONE_MINUS_DST_COLOR);
		glTranslated(0,0.7+(menuEntry*0.7),0);

		glBegin(GL_QUADS);
		glVertex3f(-0.1,-1.3,2);
		glVertex3f(-0.1,-2,2);
		glVertex3f(-0.1,-2,-2);
		glVertex3f(-0.1,-1.3,-2);
		glEnd();
		glDisable(GL_BLEND);
	}

	glPopMatrix();
}

void selectHandler()
{
	if(selection == 1)
	{
		if(image.size() > menuEntry-1u+(menuOffsetImage*3u) )
		{
			if(menuEntry == 4)
			{
				menuEntry = 0;
				menuOffsetImage++;
			}
			else if(menuEntry == 0 && menuOffsetImage > 0)
			{
				menuEntry = 0;
				menuOffsetImage--;
			}
			else
			{
				pEssai->changeTex(const_cast<char*>(image.at(menuEntry-1+(menuOffsetImage*3)).c_str()));
				imageDisplayed = true;
			}
		}
	}
	else
	{

		if(video.size() > menuEntry-1u+(menuOffsetVideo*3u))
		{
			if(menuEntry == 4)
			{
				menuEntry = 0;
				menuOffsetVideo++;
			}
			else if(menuEntry == 0 && menuOffsetVideo > 0)
			{
				menuEntry = 0;
				menuOffsetVideo--;
			}
			else
			{
				pEssai->openVideo(const_cast<char*>(video.at(menuEntry-1+(menuOffsetVideo*3)).c_str()));
				pEssai->redisplayScene();
			}
		}
	}
}

#ifndef NO_POINTER
void grabPhoneInput()
{
	double angle;

	if (device->isConnected())
	{
		switch(device->getInputEvent())
		{
		  //switch according to what information the phone send
			case Hemi::DeviceInput::DATA_TYPE ://send angle data
				angleH = device->gelseetHorizontalAngle();
				angleV = device->getVerticalAngle();

				angle = angleV - offsetV;

				//Note : this angles were found empyricly.
				//For a better app, this could be found by
				//picking.
				if(angle >= 15 && angle <= 30)
					menuEntry = 3;
				else if(angle > 30)
					menuEntry = 4;
				else if(angle >= 0 && angle < 15)
					menuEntry = 2;
				else if(angle >= -15 && angle < 0)
					menuEntry = 1;
				else if(angle >= -30 && angle < -15)
					menuEntry = 0;

				break;

			case Hemi::DeviceInput::MOUSE_TYPE ://send touch screen button

				if (device->getMouseInput() == Hemi::DeviceInput::MOUSE_RIGHT_BUTTON)
				{

					if(!imageDisplayed)
					{
						offsetH = angleH;
						offsetV = angleV;
						std::cout<<"Offset H, V : "<<offsetH<<","<<offsetV<<std::endl;
					}
					else
					{
						imageDisplayed = false;
						pEssai->redisplayScene();
					}
				}
				else
				{
					angle = angleH-offsetH;

					if(angle < 0)
						angle = 360 + angle;

					std::cout<<angle<<std::endl;

					if(!rightRot && angle >= 30 && angle <= 90)
					{
						leftRot = true;
						return;
					}

					if(!leftRot && angle >= 270 && angle <= 315)
					{
						rightRot = true;
						return;
					}

					if(selection != 0)
						selectHandler();

				}
				break;

			case Hemi::DeviceInput::KEY_TYPE ://send keyboard input
				break;
		   default:
			   assert(false);
			   break;
		}
	}
	else
		ok = false;

}

#endif
