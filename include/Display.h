/***************************************************************************
 *   Copyright (C) 2010 by Guillaume Saby and Maxime Chambefort		      *
 *   saby.guillaume@gmail.com   &  mmc0ot@aber.ac.uk	& ajw@aber.ac.uk     *
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
#ifndef HEMIDISPLAY_H
#define HEMIDISPLAY_H

#if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))
#define DEPRECATED  __attribute__((__deprecated__))
#else
#define DEPRECATED
#endif /* __GNUC__ */

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include <cmath>
#include <SDL/SDL.h>
#ifdef HAVE_OPENGL_GL_H
# include <OpenGL/gl.h>
#elif defined(HAVE_GL_GL_H)
# include <GL/gl.h>
#else
# warning Neither GL/gl.h or OpenGL/gl.h were found by configure, guessing!
# warning Ideally this header should not be included here anyway
# include <GL/gl.h>
#endif
#ifdef HAVE_OPENGL_GL_H
# include <OpenGL/glu.h>
#elif defined(HAVE_GL_GLU_H)
# include <GL/glu.h>
#else
# warning did not find glu.h, guessing!
# include <GL/glu.h>
#endif
#include <fstream>
#include <string>
#include <memory>
#include <vector>


struct AVFrame;

/**
	@namespace Hemi namespace containing every class and structs managing dome
 */
namespace Hemi {
	
/**
	Struct for RGB managing

 */
	struct RGB
	{
		unsigned char m_r,m_g,m_b;
	   RGB(unsigned char r = 0, unsigned char g = 0, unsigned char b = 0) : m_r(r), m_g(g), m_b(b) {}
	};

	struct Vertex
	{
	  double m_x,m_y,m_z;
	  double texCoordx, texCoordy;
  	  Vertex(const double& x = 0, const double& y = 0, const double& z = 0) : m_x(x), m_y(y), m_z(z) {}
	  
	  void normalize(){const double norme=sqrt(m_x*m_x+m_y*m_y+m_z*m_z);m_x/=norme;m_y/=norme;m_z/=norme;}
	  double norme() const {return sqrt(m_x*m_x+m_y*m_y+m_z*m_z);}
	  double scalar(const Vertex& v) const;
	  
	  Vertex operator+(const Vertex& v) const;
	  Vertex operator*(const double& k) const;
	  Vertex operator-(const Vertex& v) const;
	  Vertex operator^(const Vertex& v) const;
	  Vertex& operator+=(const Vertex& v);
	  Vertex& operator-=(const Vertex& v);
	  Vertex& operator*=(const double& k);
	  Vertex operator-() const;
	};

	class Renderer {
	public:
	  virtual ~Renderer() {}
	  virtual void draw() = 0;
	};

/**
	Class managing the display for the hemispherium.
	Took either images or OpenGL function.

	@author Guillaume Saby <saby.guillaume@gmail.com>
	@author Maxime Chambefort <mmc0ot@aber.ac.uk>
	
	@note note that the top of every image you provide will be the back and the bottom the front.
 */


	class Display{
	
		private:
			int m_Hscreen;
			int m_Wscreen;
			SDL_Surface* m_screen;
			SDL_Surface* m_cursor;
			RGB** m_image;
			double posCam;
			
			bool m_debug;
			int flags;
	
			int m_HImage;
			int m_WImage;
			
			GLdouble m_angleRot;
			GLdouble m_zoom;
			GLdouble m_moveTexX;
			GLdouble m_moveTexY;
			GLdouble m_moveTexZ;
			
			GLdouble m_mouseX;
			GLdouble m_mouseY;
	
			float colorUp, colorDown;
	
			//! Use FBOs to permit higher resolution cubemaps if required.
			GLuint fbo;
			
			//OpenGL section: see Maxime CHAMBEFORT
			bool m_oglRender;
			GLuint m_cubeSurfaces[6];
			
			GLuint tex;
			GLuint list;
			GLuint listdebug;
			GLuint m_cubeTex;
			GLuint m_texture;
			GLuint m_glTextDebug;
	
			int nb_Hvertex;
			int nb_Wvertex;
			double vertex_Woffset;
			double vertex_Hoffset;
	
			std::vector<Vertex> m_tabVertexUp;
			std::vector<Vertex> m_tabVertexDown;
	
			static void putPixel(SDL_Surface *surface, int x, int y, Uint32 pixel);

			void initGL() const;
			void createMesh();
			void createList();
			void createTexture(GLubyte* &data) const;
			static void createTexture(GLubyte* &data, int w, int h, RGB** im);
	
			void LoadTexture(const std::string& filename);
			void textureGen();
	
			template<typename Iterator>
			void readTable(std::vector<Vertex>& table, Iterator& in) const;
			void drawTable(const std::vector<Vertex>& table) const;
			void readFile();
			
			void InitTextures();
			void drawMirror();
			void m_updateCube();
			void m_display();
			
			void saveMatrixRotation();
			
			// Removing for now, I intend to seperate this functionality anyway
			void createImageFromeFrame(AVFrame *pFrame, int width, int height);

			int m_CUBEMAP_SIZE;
			//unsigned char * m_pixels;

			int m_BALL_COMPLEXITY;
			GLUquadric *m_quadric;
			int m_radius;
			
			//clipping values
			double m_far;
			double m_near;
		
			//true if a cube mapping texture is created
			bool m_cubeTexturesCreated;
			bool m_debugTexturesCreated;
			RGB m_ClearColor;
		
			Vertex m_targets[6];
			Vertex m_eye;
			Vertex m_debugCoordinates[3];

			std::auto_ptr<Renderer> scene;
		
		public:
			/*!
			@brief Constructor.
			@param file the image file to load
			@param debugMode true to display video output in windowed mode(900x800, usefull for debugging). If false (the default) display in fullscreen with the Hemispherium resolution(1280x1440)
			*/
			Display(bool debugMode = false);
			~Display();
			
			/*! Draw a new image to the framebuffer. If swap is true swap the buffers also. Not swapping buffers might be useful for implementing picking or some types of post processing. */
			void update(const bool swap=true);
			
			void rotationImage(GLdouble alpha);
			void scaleImage(GLdouble zoom);
			void moveImageX(GLdouble dist);
			void moveImageY(GLdouble dist);
			void moveImageZ(GLdouble dist);
			void setTexDefaulft();
			
			void setDisplayCursor(SDL_Surface* cursor,std::string filename);
			void setCursorCoordinates(GLdouble x , GLdouble y);
			
			/*! Change the current texture
			@brief this function should be called juste before update*/
			void changeTex(const std::string& filename);
			
			/*! Change the current texture
			@brief this take an RGB tab filled with pixels information of the image you want to display.
			@brief (0,0) should be the upper left corner. 
			@param tab a pointer to the filled array of RGB
			@param w the width of the image
			@param h the height of the image
			*/
			void changeTex(RGB** tab, int w, int h);
			
			/*! Change the filter method of the display
			@param smooth if true, smoothing is enable, if false no interpolation done
			@note the smoothing is enabled by default
			*/
			void changeFilter(bool smooth);
			
			/*!
			@brief open a video file and display it
			@note this function halt the execution until it's finished
			*/
			void openVideo(const std::string& file);
			
			/*!
			@brief Utility function that load an image as an OpenGl texture
			@return return the OpenGl id of the loaded texture. -1 if fail
			*/
			static GLuint loadTexture(const std::string& filename);
			static Uint32 getpixel(const SDL_Surface *surface, int x, int y);
			
			//OpenGL section: see Maxime CHAMBEFORT
			
			/*!
			@brief Function to call before using the openGL fisheye scene render
			@note If your application will use openGl rendering scene, call this just after the construction of the object
			*/
			void initOglRender();
			
			/*!
			@brief Set the clear color of the scene render avoid putting it inside the render func. DEFAULT : white.
			*/
			void setClearColor(const double& red, const double& green, const double& blue);
			
			/*!
			@brief Describe the camera position in a gluLookAt function fashion
			*/
			void setCameraCoordinates(const double& eyex, const double& eyey, const double& eyez, const double& centerx, const double& centery, const double& centerz, const double& upx, const double& upy, const double& upz);
			/*!
			@brief Set the Near and Far clipping plane. DEFAUT : Near = 0.5, Far = 500
			*/
			void setSettings(const double& near, const double& far);
			/*!
			@brief Set the render function
			@note The function should NOT contain any Projection command, it will mess with the fisheye renderer. It's not needed to set the matrix mode to modelview, evrything is taken care of. The function should only place the object into the scene.
			*/
			void setRenderFunc(void (*render)()) DEPRECATED;

			/*!
			  @brief Set the renderer object if a non NULL auto_ptr is passed in
			  @note This should not change any of the view matricies in order to not interfer with the fisheye renderer.
			  @return The currently used renderer
			*/
			std::auto_ptr<Renderer> renderer(std::auto_ptr<Renderer> set = std::auto_ptr<Renderer>(NULL));
			
			/*!
			  @brief Set the renderer object if a non NULL pointer is given
			  @note The pointer is owned by the Display once the function has returned
			  @sa renderer
			*/
			std::auto_ptr<Renderer> renderer(Renderer* rend) { return renderer(std::auto_ptr<Renderer>(rend)); }

			/*!
			@brief Re-display the openGL fisheyeRender
			@note this function is meant to be called after display an image or a video, to redisplay the scene.
			*/
			void redisplayScene();
			
			const int& width() const { return m_Wscreen; }
			const int& height() const { return m_Hscreen; }
    
    
    
    
	};
}

#endif
