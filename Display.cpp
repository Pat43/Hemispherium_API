/***************************************************************************
 *   Copyright (C) 2010 by Guillaume Saby                                  *
 *   saby.guillaume@gmail.com                                              *
 *   ajw@aber.ac.uk
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
#ifdef HAVE_CONFIG_H
# include "../config.h"
#endif
#ifdef HAVE_GL_GLEW_H
# include <GL/glew.h>
#elif defined(HAVE_GLEW_H)
# include <glew.h>
#endif
#include "Display.h"
#ifdef HAVE_SDL_IMAGE_H
# include <SDL/SDL_image.h>
#endif
#include <iostream>
#include <cassert>
#include <iterator>
#define _STRINGIFY(x) # x 
#define STRINGIFY(x) _STRINGIFY(x)

namespace Hemi {
  struct Calibration {
	 static const double values[];  
  };

  Display::Display(bool debugMode) : m_debug(debugMode), fbo(0)
	{
		/*If debug Mode, windowed and reduced cubemapsize*/
		if(m_debug)
		{
			m_Hscreen = 900;
			m_Wscreen = 800;
			
			m_CUBEMAP_SIZE = 1024;
			
			flags = SDL_OPENGL;
		}
		else
		{//Fullscreen, huge cubemap to avoid loosing information
			m_Hscreen = 1440;
			m_Wscreen = 1280;
			
			m_CUBEMAP_SIZE = 2048;
			
			flags = SDL_OPENGL|SDL_FULLSCREEN;
		}
		
		m_HImage = m_Hscreen;
		m_WImage = m_Wscreen;
	
		m_screen = NULL;
	
		m_image = NULL;
		m_oglRender = false;
	
		m_angleRot = 0.0;
		m_zoom = 1.0;
		m_moveTexX = 0.0;
		m_moveTexY = 0.0;
		m_moveTexZ = 0.0;
		
		//init SDl and OpenGl
		initGL();

		if (!(m_CUBEMAP_SIZE <= m_Wscreen && m_CUBEMAP_SIZE <= m_Hscreen)) {
#if defined(HAVE_GL_GLEW_H) || defined(HAVE_GLEW_H)
		  if (!glewGetExtension("GL_EXT_framebuffer_object"))
#endif
			 std::cerr << "Warning: cubemaps this big need FBOs" << std::endl;
		}

	
		vertex_Woffset = 1280.0/51.0;
		vertex_Hoffset = 720.0/51.0;
		nb_Wvertex = 51;
		nb_Hvertex = 51;
			
		//Create the two grids (loaded in a display List)
		//createMesh();
	
		/**********************************/
		glEnable (GL_TEXTURE_2D);
		glEnable(GL_DEPTH_TEST);

		glClearColor(0.0f,0.0f,0.0f,1.0f);

		//glViewport( 0, 0, m_Wscreen, m_Hscreen );

		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();

		const double ratio = static_cast<float>(m_Wscreen)/static_cast<float>(m_Hscreen);
		//gluOrtho2D(-1200.0/2.0, 1200.0/2.0, -1200.0/2.0/ratio, 1200.0/2.0/ratio);
	
		glGenTextures( 1, &tex);
		
		m_image = new RGB*[m_WImage];
	
		for(int i = 0; i < m_WImage; i++)
		{
			m_image[i] = new RGB[m_HImage];
		}
		
		//textureGen();
		
		posCam = -0.1;
			
		SDL_EnableKeyRepeat(500, 30);
		
		//Read the conf file
		readFile();
		
		// This seems to override what the config file says! (ajw)
		colorUp = 1.0;
		colorDown = 1.0;

		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity( );
		
		const double offCam = (1440.0-(1280.0/ratio))/2.0;
		gluLookAt(nb_Wvertex*vertex_Woffset/2.0,nb_Hvertex*vertex_Hoffset+offCam,posCam, 
			   nb_Wvertex*vertex_Woffset/2.0,nb_Hvertex*vertex_Hoffset+offCam,-1, 
			    0,-1,0);
		
		createList();
		/***************************************/
	}

	Display::~Display()
	{
		SDL_Quit();
	}

	void Display::initGL() const
	{
		const SDL_VideoInfo* info = NULL;
		int bpp = 0;

		if ( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
			fprintf( stdout, "Video initialization failed: %s\n", SDL_GetError( ) );
		}

		info = SDL_GetVideoInfo( );

		if( !info ) {
			fprintf( stdout, "Video query failed: %s\n", SDL_GetError( ) );
		}

		bpp = info->vfmt->BitsPerPixel;

		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8);
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8);
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

		if ( SDL_SetVideoMode( m_Wscreen, m_Hscreen, bpp, flags ) == 0 ) {
			fprintf( stdout, "Video mode set failed: %s\n", SDL_GetError( ) );
		}
		
#ifdef HAVE_SDL_IMAGE_H
		// I thought this was needed for SDL_image, but it seems to not
		// be needed any more
		/*const int imgflags = IMG_INIT_JPG|IMG_INIT_PNG|IMG_INIT_TIF;
		if (IMG_Init(imgflags) & imgflags != imgflags) {
		  fprintf( stdout, "Failed to initialise image loader for one or more formats: %s\n", IMG_GetError());
		  }*/
#else
		fprintf( stdout, "Built without SDL_Image support\n");
#endif

#if defined(HAVE_GL_GLEW_H) || defined(HAVE_GLEW_H)
		glewInit();
#endif

		glEnable (GL_TEXTURE_2D);
		glEnable(GL_DEPTH_TEST);
	}

	void Display::update(const bool swap)
	{
		//If oglrender is used, update the current image with the fisheye render first
		if (m_oglRender) m_display();
		
		  glMatrixMode( GL_PROJECTION );
		  glLoadIdentity();

		if(!m_debug)
		{
		  const double ratio = (float)m_Wscreen/(float)m_Hscreen;
		  gluOrtho2D(-1280.0/2.0, 1280.0/2.0, -1280.0/2.0/ratio, 1280.0/2.0/ratio);
		} 
		else gluPerspective(45.0,1.0,m_near,m_far);
		
		  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		  
		  if(!m_oglRender){
		    glMatrixMode(GL_TEXTURE);
		    glLoadIdentity();
		    glTranslatef(0.5+m_moveTexX,0.5+m_moveTexY,0.0+m_moveTexZ);
		    glRotated(m_angleRot,0,0,1);
		    glScalef(m_zoom,m_zoom,m_zoom);
		    glTranslatef(-0.5,-0.5,0.0);
		  }
		  
		  glMatrixMode( GL_MODELVIEW );
		  glLoadIdentity( );
		  
		if(!m_debug)
		{
		  const double offCam = (1440.0-(1280.0/((float)m_Wscreen/(float)m_Hscreen)))/2.0;
		  
		  gluLookAt(nb_Wvertex*vertex_Woffset/2.0,nb_Hvertex*vertex_Hoffset+offCam,posCam, 
			    nb_Wvertex*vertex_Woffset/2.0,nb_Hvertex*vertex_Hoffset+offCam,-1, 
			      0,-1,0);
		  glEnable (GL_TEXTURE_2D);
		  //just call the list previously made
		  glCallList( list );
		
		}else
		{
			     
		  gluLookAt(0,0,-2.4,0,0,0,0,1,0);
	
		  glEnable (GL_TEXTURE_2D);
		  glBindTexture(GL_TEXTURE_2D,tex);
		   
		  //just call the list previously made
		  glCallList( listdebug );
		  
		}
		 glBindTexture(GL_TEXTURE_2D, 0);
		 glDisable (GL_TEXTURE_2D);
		  
		if (swap)
		  SDL_GL_SwapBuffers( );
		
	}

	/*
	* Set the pixel at (x, y) to the given value
	*/
	void Display::putPixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
	{
		const int bpp = surface->format->BytesPerPixel;
		/* Here p is the address to the pixel we want to set */
		Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

		switch (bpp) {
			case 1:
				*p = pixel;
				break;

			case 2:
				*(Uint16 *)p = pixel;
				break;

			case 3:
				if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
					p[0] = (pixel >> 16) & 0xff;
					p[1] = (pixel >> 8) & 0xff;
					p[2] = pixel & 0xff;
				}
				else {
					p[0] = pixel & 0xff;
					p[1] = (pixel >> 8) & 0xff;
					p[2] = (pixel >> 16) & 0xff;
				}
				break;

			case 4:
				*(Uint32 *)p = pixel;
				break;

			default:
				break;           /* shouldn't happen, but avoids warnings */
		} // switch
	}

	Uint32 Display::getpixel(const SDL_Surface *surface, int x, int y)
	{
		const int bpp = surface->format->BytesPerPixel;
		/* Here p is the address to the pixel we want to retrieve */
		Uint8 *p = static_cast<Uint8*>(surface->pixels) + y * surface->pitch + x * bpp;

		switch(bpp) {
			case 1:
				return *p;
				break;

			case 2:
				return *(Uint16 *)p;
				break;

			case 3:
				if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
					return p[0] << 16 | p[1] << 8 | p[2];
				else
					return p[0] | p[1] << 8 | p[2] << 16;
				break;

			case 4:
				return *(Uint32 *)p;
				break;

			default:
				return 0;       /* shouldn't happen, but avoids warnings */
		}
	}

   template <typename Iterator>
   void Display::readTable(std::vector<Vertex>& table, Iterator& in) const {
	  assert(table.size() == static_cast<std::vector<Vertex>::size_type>(nb_Wvertex * nb_Hvertex));
	  for(int i = 0; i < nb_Wvertex; i++) {
		 for(int j = 0; j < nb_Hvertex; j++) {
			Vertex& current = table.at((i*nb_Hvertex) + j);
			current.m_x = *in++;
			current.m_y = *in++;
			current.m_z = *in++;
			current.texCoordx = *in++;
			current.texCoordy = *in++;
		 }
	  }
	}

	void Display::readFile()
	{  
  	   m_tabVertexUp = std::vector<Vertex>(nb_Wvertex*nb_Hvertex);
		m_tabVertexDown = std::vector<Vertex>(nb_Wvertex*nb_Hvertex);

		{
		  // can't assert this, the size isn't known!
		  //assert(sizeof(Calibration::values) / sizeof(double) == nb_Wvertex * nb_Hvertex * 5);
		  // read the compiled in values
		  const double *it = Calibration::values;
		  readTable(m_tabVertexUp,it);
		  readTable(m_tabVertexDown,it);
		  colorUp = *it++;
		  colorDown = *it++;
		  if (it != Calibration::values)
			 update();
		}

		std::ifstream local("hemi.cfg");
		if (local)
		  std::cerr << "Found local hemi.cfg" << std::endl;
		std::ifstream system(STRINGIFY(DATADIR) "/hemi.cfg");
		if (system && !local)
		  std::cerr << "Found system hemi.cfg" << std::endl;
		else if (system && local)
		  std::cerr << "Found system hemi.cfg (ignored)" << std::endl;
		else if (!system & !local)
		  std::cerr << "Using built in configuration" << std::endl;

		std::istream& in = local ? local : system;

		if(in)
		{
		  std::istream_iterator<double> it(in);
		  ///----------------------------UP---------------------------------
		  readTable(m_tabVertexUp,it);
		  
		  ///-------------------------------DOWN--------------------------------
		  readTable(m_tabVertexDown,it);
		  update();
		
		  colorUp = *it++;
		  colorDown = *it++;
		}
		else
		  std::cerr << "Unable to read external calibration file" << std::endl;
	}
	
	void Display::createMesh()
	{
		/**----------------------------UP---------------------------------*/
	   m_tabVertexUp = std::vector<Vertex>(nb_Wvertex * nb_Hvertex);
	
		for(int i = 0; i < nb_Wvertex; i++)
		{
			for(int j = 0; j < nb_Hvertex; j++)
			{
			   Vertex& vert = m_tabVertexUp.at((i*nb_Hvertex)+j);
				vert = Vertex(i*vertex_Woffset, j*vertex_Hoffset, 0.0);
			
				vert.texCoordx = vert.m_x/static_cast<float>(nb_Wvertex*vertex_Woffset);
				vert.texCoordy = 1 - vert.m_y/static_cast<float>(nb_Hvertex*vertex_Hoffset);
			}
		}
	
		/**-------------------------------DOWN--------------------------------*/
	   m_tabVertexDown = std::vector<Vertex>(nb_Wvertex * nb_Hvertex);
	
		for(int i = 0; i < nb_Wvertex; i++)
		{
			for(int j = 0; j < nb_Hvertex; j++)
			{
			   Vertex& vert = m_tabVertexDown.at((i*nb_Hvertex)+j);
				vert = Vertex(i*vertex_Woffset, (nb_Hvertex-1)*vertex_Hoffset + j*vertex_Hoffset, 0.0);
			
				vert.texCoordx = vert.m_x/static_cast<float>(nb_Wvertex*vertex_Woffset);
				vert.texCoordy = 1 - (vert.m_y-(nb_Hvertex*vertex_Hoffset))/static_cast<float>(nb_Hvertex*vertex_Hoffset);
			}
		}
	}
	
   void Display::drawTable(const std::vector<Vertex>& table) const {
	  glBindTexture(GL_TEXTURE_2D, tex);
	  
	  glBegin(GL_QUADS);
	  for(int i = 0; i < nb_Wvertex-1; i++) {
		 for(int j = 0; j < nb_Hvertex-1; j++)	{
			const Vertex& v1 = table.at((i*nb_Hvertex)+j);       // [i][j];
			const Vertex& v2 = table.at((i*nb_Hvertex)+j+1);     // [i][j+1];
			const Vertex& v3 = table.at(((i+1)*nb_Hvertex)+j+1); // [i+1][j+1];
			const Vertex& v4 = table.at(((i+1)*nb_Hvertex)+j);   // [i+1][j];
			
			glTexCoord2f(v1.texCoordx, v1.texCoordy);
			glVertex3d(v1.m_x, v1.m_y, v1.m_z);
			
			glTexCoord2f(v2.texCoordx, v2.texCoordy);
			glVertex3d(v2.m_x, v2.m_y, v2.m_z);
			
			glTexCoord2f(v3.texCoordx, v3.texCoordy);
			glVertex3d(v3.m_x, v3.m_y, v3.m_z);
			
			glTexCoord2f(v4.texCoordx, v4.texCoordy);
			glVertex3d(v4.m_x, v4.m_y, v4.m_z);
		 }
	  }
	  glEnd();
	}

	void Display::createList()
	{
	  if(!m_debug)
	  {
		list = glGenLists(1);
		
		glNewList( list, GL_COMPILE );
			
			glEnable (GL_TEXTURE_2D);
		
			/*---------------------UP-----------------------*/
			glColor3f(colorUp,colorUp,colorUp);
			drawTable(m_tabVertexUp);
			/*---------------------------------------------------------------------*/
			/*-----------------------------DOWN------------------------------------*/
			glColor3f(colorDown,colorDown,colorDown);
			drawTable(m_tabVertexDown);
			/*------------------------------------------------------------------------------*/
		glEndList();
	  }
	  else{
	    listdebug = glGenLists(2);
		  
		  glNewList( listdebug, GL_COMPILE );
			  glEnable (GL_TEXTURE_2D);
			  glBegin(GL_QUADS);
			    glVertex2f(-1,-1);
			    glTexCoord2f(0.0,0.0);
			    glVertex2f(-1.0,1);
			    glTexCoord2f(0.0,1.0);
			    glVertex2f(1,1);
			    glTexCoord2f(1.0,1.0);
			    glVertex2f(1,-1);
			    glTexCoord2f(1.0,0.0);
			  glEnd();
		  glEndList();
	  }
	}
	
	void Display::createTexture(GLubyte* &data) const
	{//create a texture with the info contain in data
	  createTexture(data,m_WImage,m_HImage,m_image);
	}
	
	void Display::createTexture(GLubyte* &data, int w, int h, RGB** im)
	{//Same thing but this time with any data, not only the one from the current image
		data = new GLubyte[w*h*4];
	
		for(int i = 0; i < h; i++)
		{
			for(int j = 0; j < w; j++)
			{
				data[(i*w*4)+(j*4)] = im[j][h-1-i].m_r;
				data[(i*w*4)+(j*4)+1] = im[j][h-1-i].m_g;
				data[(i*w*4)+(j*4)+2] = im[j][h-1-i].m_b;
				data[(i*w*4)+(j*4)+3] = 0xff;
			}
		}
	}

	void Display::LoadTexture(const std::string& filename )
	{//Load a texture into the current Image
		SDL_Surface *surface;

#ifdef HAVE_SDL_IMAGE_H
		surface = IMG_Load(filename.c_str());
#else
		surface = SDL_LoadBMP(filename.c_str());
		
#endif
		// could not load filename
		if (!surface) {
		  fprintf(stdout, "filename : %s ERROR LOADING\n", filename.c_str());
			return;
		}

		if(m_image != NULL)
		{
			for(int i = 0; i < m_WImage; i++)
				delete[] m_image[i];
			delete[] m_image;
		}

		m_HImage = surface->h;
		m_WImage = surface->w;
	
		m_image = new RGB*[m_WImage];
	
		for(int i = 0; i < m_WImage; i++)
		{
			m_image[i] = new RGB[m_HImage];
		}
	
		for(int i = 0; i < m_WImage; i++)
		{
			for(int j = 0; j < m_HImage; j++)
			{
				Uint32 pix = getpixel(surface, i, j);
				Uint8 r,g,b;
				SDL_GetRGB(pix, surface->format, &r, &g, &b);
				m_image[i][j] = RGB(static_cast<unsigned char>(r),static_cast<unsigned char>(g),static_cast<unsigned char>(b));
			}
		}
		
		SDL_FreeSurface( surface );
	}
	
	GLuint Display::loadTexture(const std::string& filename)
	{
		SDL_Surface *surface;

#ifdef HAVE_SDL_IMAGE_H
		surface = IMG_Load(filename.c_str());
#else
		surface = SDL_LoadBMP(filename.c_str());
#endif
		// could not load filename
		if (!surface) {
	 	   fprintf(stdout , "filename : %s ERROR LOADING\n", filename.c_str());
			return 0;
		}

		int h = surface->h;
		int w = surface->w;
		
		RGB** im = new RGB*[w];

		for(int i = 0; i < w; i++)
		{
			im[i] = new RGB[h];
		}
	
		for(int i = 0; i < w; i++)
		{
			for(int j = 0; j < h; j++)
			{
				Uint32 pix = getpixel(surface, i, j);
				Uint8 r,g,b;
				SDL_GetRGB(pix, surface->format, &r, &g, &b);
				im[i][j] = RGB((unsigned char)r,(unsigned char)g,(unsigned char)b);
			}
		}
		
		GLuint ret;
		glGenTextures(1, &ret);
		
		glBindTexture (GL_TEXTURE_2D, ret);
		GLubyte* c;
		createTexture(c, w, h, im);
		glTexImage2D (GL_TEXTURE_2D, 0, 4, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, c);
	
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glBindTexture (GL_TEXTURE_2D, 0);
		
		delete[] c;
		
		for(int i = 0; i < w; i++)
		{
			delete[] im[i];
		}
		delete[] im;
		
		SDL_FreeSurface( surface );
		
		return ret;
	}
	
	void Display::textureGen()
	{
		glBindTexture (GL_TEXTURE_2D, tex);
		GLubyte* c;
		createTexture(c);
		glTexImage2D (GL_TEXTURE_2D, 0, 4, m_WImage, m_HImage, 0, GL_RGBA, GL_UNSIGNED_BYTE, c);
	
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glBindTexture (GL_TEXTURE_2D, 0);
		
		delete[] c;
	}
	
	void Display::changeFilter(bool smooth)
	{
		glBindTexture (GL_TEXTURE_2D, tex);
		if(smooth)
		{
			glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
		else
		{
			glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
		glBindTexture (GL_TEXTURE_2D, 0);
	}

	void Display::changeTex(const std::string& filename)
	{
		m_oglRender = false;
		
		if(!filename.size())
		{
			return;
		}
		
		LoadTexture(filename);
		textureGen();
	}
	
	void Display::changeTex(RGB** tab, int w, int h)
	{
		m_oglRender = false;
		
		if(m_image != NULL)
		{
			for(int i = 0; i < m_WImage; i++)
				delete[] m_image[i];
			delete[] m_image;
		}
		
		m_HImage = h;
		m_WImage = w;
	
		m_image = new RGB*[m_WImage];
	
		for(int i = 0; i < m_WImage; i++)
		{
			m_image[i] = new RGB[m_HImage];
		}
		
		for(int i = 0; i < m_WImage; i++)
		{
			for(int j = 0; j < m_HImage; j++)
			{
				m_image[i][j] = tab[i][j];
			}
		}
	}	
  
	//OpenGL section; see Maxime CHAMBEFORT
	
	void Display::initOglRender()
	{
		if(m_oglRender == false)
		{
			if(m_image != NULL)
			{
				for(int i = 0; i < m_WImage; i++)
					delete[] m_image[i];
				delete[] m_image;
				m_image = NULL;
			}
	
			m_WImage = m_Wscreen;//m_CUBEMAP_SIZE;
			m_HImage = m_Hscreen;//m_CUBEMAP_SIZE;
			m_image = new RGB*[m_WImage];
			for(int i = 0; i < m_WImage; i++)
			{
				m_image[i] = new RGB[m_HImage];
			}
			for(int i = 0; i < m_WImage; i++)
			{
				for (int j = 0; j < m_HImage; j++)
				{
					m_image[i][j].m_r = 255;
					m_image[i][j].m_g = 0;
					m_image[i][j].m_b = 0;
				}
			}
			textureGen();
			
			m_cubeSurfaces[0] = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB; //ok
			m_cubeSurfaces[1] = GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB; //ok
			m_cubeSurfaces[2] = GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB; //ok
			m_cubeSurfaces[3] = GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB; //ok
			m_cubeSurfaces[4] = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB; //ok
			m_cubeSurfaces[5] = GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB; //ok
			
			
			m_BALL_COMPLEXITY = 180;
			m_radius = 1.0;
			
			m_far = 500;
			m_near = m_radius / 2.0;
		
			m_cubeTexturesCreated = false;
			m_debugTexturesCreated = false;
		
			m_ClearColor.m_r = 255;
			m_ClearColor.m_g = 255;
			m_ClearColor.m_b = 255;
		
			setCameraCoordinates(0,0,0,0,0,1,0,1,0);
		
			m_quadric = gluNewQuadric();
			gluQuadricNormals(m_quadric, GLU_SMOOTH); 
			gluQuadricTexture(m_quadric, true);
		
			//&(*m_scene()) = NULL;
		
			InitTextures();
		}
			
		m_oglRender = true;
	}
	
	void Display::redisplayScene()
	{
		m_oglRender = true;
		m_angleRot = 0.0;
		m_zoom = 1.0;
		m_moveTexX = 0.0;
		m_moveTexY = 0.0;
		m_moveTexZ = 0.0;
		
		if(m_image != NULL)
		{
			for(int i = 0; i < m_WImage; i++)
				delete[] m_image[i];
			delete[] m_image;
			m_image = NULL;
		}
		
		m_WImage = m_Wscreen;//m_CUBEMAP_SIZE;
		m_HImage = m_Hscreen;//m_CUBEMAP_SIZE;
		
		m_image = new RGB*[m_WImage];
		for(int i = 0; i < m_WImage; i++)
		{
			m_image[i] = new RGB[m_HImage];
		}
		
		for(int i = 0; i < m_WImage; i++)
		{
			for (int j = 0; j < m_HImage; j++)
			{
				m_image[i][j].m_r = 255;
				m_image[i][j].m_g = 255;
				m_image[i][j].m_b = 255;
			}
		}
		
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		textureGen();
	}
	
	void Display::InitTextures()
	{
	  if(!m_debug)
	  {
		//********************CUBE MAPPING INITIALISATION****************************/
		if(m_cubeTexturesCreated)
		{
			glDeleteTextures(1, &m_cubeTex);
			m_cubeTexturesCreated = false;
		}
		
		glEnable(GL_TEXTURE_CUBE_MAP_ARB);
		glGenTextures(1, &m_cubeTex);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, m_cubeTex);
		glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
	
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glClearColor(0,0,255,255);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
#if defined(HAVE_GL_GLEW_H) || defined(HAVE_GLEW_H)
		// If we need an FBO try to make one or if we can use FBOs try and make one
		if (((m_CUBEMAP_SIZE > m_Wscreen || m_CUBEMAP_SIZE > m_Hscreen) && glewGetExtension("GL_EXT_framebuffer_object")) || glewGetExtension("GL_EXT_framebuffer_object")) {
		  glGenFramebuffersEXT(1, &fbo);
		  if (fbo) {
			 std::cerr << "Using FBO for RTT" << std::endl;
		  }
		}
#endif

		for (int ii = 0; ii < 6; ii++)
		{
			// Fill the m_texture with whatever is on the framebuffer (I don't see why this isn't just a glTexImage2D call instead, but it doesn't matter much)
			glCopyTexImage2D(m_cubeSurfaces[ii], 0, GL_RGB, 0, 0, m_CUBEMAP_SIZE, m_CUBEMAP_SIZE, 0);
		}

#if defined(HAVE_GL_GLEW_H) || defined(HAVE_GLEW_H)
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
#endif
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, 0);
		glDisable(GL_TEXTURE_CUBE_MAP_ARB);

		m_cubeTexturesCreated = true;
	  }
	  else{
	      if(m_debugTexturesCreated)
		  {
			  glDeleteTextures(1, &m_glTextDebug);
			  m_debugTexturesCreated = false;
		  }
	      glEnable(GL_TEXTURE_2D);
	      glGenTextures(1,&m_glTextDebug);
	      glBindTexture(GL_TEXTURE_2D,m_glTextDebug);
	      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	      glClearColor(0,0,0,255);
	      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	      glCopyTexImage2D(GL_TEXTURE_2D,0, GL_RGB, 0, 0, m_Wscreen, m_Hscreen, 0);
	      
	      glBindTexture(GL_TEXTURE_2D,0);
	      glDisable(GL_TEXTURE_2D);
	      m_debugTexturesCreated = true;
	  }
		
	}
	
	void Display::drawMirror()
	{
	  
	    if(!m_debug){

	      //saving openGL context
	      glPushAttrib(GL_ENABLE_BIT|GL_TEXTURE_BIT);
	      glDisable(GL_LIGHTING);
	    
	      //Enable cube mapping.
	      glEnable(GL_TEXTURE_CUBE_MAP_ARB);

	      glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
	      glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
	      glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
	      glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);

	      //Enable auto generation of tex-coords
	      glEnable(GL_TEXTURE_GEN_S);
	      glEnable(GL_TEXTURE_GEN_T);
	      glEnable(GL_TEXTURE_GEN_R);

	      glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, m_cubeTex);


	      gluSphere(m_quadric, m_radius, m_BALL_COMPLEXITY, m_BALL_COMPLEXITY);
	      //gluDisk(m_quadric, 0, m_radius, 80, 40);  //activite this line to be able to vue the m_scene in a disc without any deformation

	      //setting back OpenGL context as it was before
	      glPopAttrib();
	      
	    }else
	    {
	      glEnable(GL_TEXTURE_2D);
	      glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
	      glBindTexture(GL_TEXTURE_2D, m_glTextDebug);
	      glCallList( listdebug );
	    }
	    
	}
	
	void Display::m_updateCube()
	{
		glPushMatrix();
		// Save the view port so we can restore it later
		glPushAttrib(GL_VIEWPORT_BIT);
		    
		  if(!m_debug)
		  {
		    glViewport(0,0,m_CUBEMAP_SIZE,m_CUBEMAP_SIZE);
		    glMatrixMode(GL_PROJECTION);
		    glLoadIdentity();
		    gluPerspective (90, 1.0f, m_near, m_far);
		    
		    Vertex center;
		      for (int ii = 0; ii < 6; ii++)
		      {
			      Vertex up = m_targets[5];
			      center = m_targets[ii];

			      if (ii > 3)
			      {
				center += m_eye;
				up = m_targets[2];
			      }
			      
			      if(ii == 2)
			      {
				center += m_eye;
				up = -up;
			      }
			      if(ii == 1 || ii == 3)
			      {
				      center += m_eye;
				      up = m_targets[2];
			      }

      #if defined(HAVE_GL_GLEW_H) || defined(HAVE_GLEW_H)
			      if (fbo) {
				glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

				glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, m_cubeSurfaces[ii], m_cubeTex, 0); 
				glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
			      }
      #endif
			      glClearColor( m_ClearColor.m_r, m_ClearColor.m_g, m_ClearColor.m_b, 1.0);
			      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			      glMatrixMode(GL_MODELVIEW);
			      glLoadIdentity();
			      gluLookAt(m_eye.m_x,m_eye.m_y,m_eye.m_z,center.m_x,center.m_y,center.m_z, -up.m_x,-up.m_y,-up.m_z);

			      /**----here is the m_scene function given by the user---*/
			      glPushMatrix();
			      assert(scene.get());
			      scene->draw();
			      glPopMatrix();

      #if defined(HAVE_GL_GLEW_H) || defined(HAVE_GLEW_H)
			      if (fbo) {
				glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
			      }
      #else
			      if (0) {}
      #endif
			      else {
				glEnable(GL_TEXTURE_CUBE_MAP_ARB);
				glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, m_cubeTex);
				
				glCopyTexSubImage2D(m_cubeSurfaces[ii], 0, 0, 0, 0, 0, m_CUBEMAP_SIZE, m_CUBEMAP_SIZE);
				glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, 0);
				glDisable(GL_TEXTURE_CUBE_MAP_ARB);
			      }
		      }
		      glPopAttrib();		
		      glPopMatrix();
	      
		      glMatrixMode(GL_PROJECTION);
		      glLoadIdentity();
		      gluPerspective(90, 1.0f, 0.01, 1000.0);
		      
		      
		  }//------------------- DEGUG ------------------------------------//
		  else{
		    glViewport(0,0,m_Wscreen,m_Hscreen);
		    glMatrixMode(GL_PROJECTION);
		    glLoadIdentity();
		    gluPerspective (70.0, 1.0f, m_near, m_far);
		    
		    glClearColor( m_ClearColor.m_r, m_ClearColor.m_g, m_ClearColor.m_b, 1.0);
		    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		    glMatrixMode(GL_MODELVIEW);
		    glLoadIdentity();
			    
		    gluLookAt(m_debugCoordinates[0].m_x-2,m_debugCoordinates[0].m_y,m_debugCoordinates[0].m_z,
			      m_debugCoordinates[1].m_x,m_debugCoordinates[1].m_y,m_debugCoordinates[1].m_z,
			      m_debugCoordinates[2].m_x,m_debugCoordinates[2].m_y,m_debugCoordinates[2].m_z);
		    
		    /**----here is the m_scene function given by the user---*/
		    glPushMatrix();
		    assert(scene.get());
		    scene->draw();
		    glPopMatrix();
		    
 #if defined(HAVE_GL_GLEW_H) || defined(HAVE_GLEW_H)
			  if (fbo) {
			    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
			  }
  #else
			  if (0) {}
  #endif
			  else {
			    glEnable(GL_TEXTURE_2D);
			    glBindTexture(GL_TEXTURE_2D, m_glTextDebug);
			    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, m_Wscreen, m_Hscreen);
			    glBindTexture(GL_TEXTURE_2D,0);
			    glDisable(GL_TEXTURE_2D);
			  }
			  glPopAttrib();		
			  glPopMatrix();
		  
			  glMatrixMode(GL_PROJECTION);
			  glLoadIdentity();
			  gluPerspective(45, 1.0f, m_near, m_far);

		  }
		
	}
	
	void Display::m_display()
	{
	 
		glPushMatrix();
		m_updateCube();
	
		glClearColor(0,255,0,255);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		if (!m_debug)
		  gluLookAt(0,1.415,0,0,0,0,0,0,1);
		else gluLookAt(0,0,-2.4,0,0,0,0,-1,0);
		
		drawMirror();
		
		glFinish();
	
		//Plane renderering start here
		glPushAttrib(GL_VIEWPORT_BIT);
		glViewport(0,0,m_Wscreen,m_Hscreen);
	
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, tex);
	
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, m_Wscreen, m_Hscreen);

		glFlush();
		glPopAttrib();
		glPopMatrix();

		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);

	}
	
	void Display::setClearColor(const double& red, const double& green, const double& blue)
	{
		m_ClearColor.m_r = red;
		m_ClearColor.m_g = green;
		m_ClearColor.m_b = blue;
	}
	void Display::setCameraCoordinates(const double& eyex, const double& eyey, const double& eyez, const double& centerx, const double& centery, const double& centerz, const double& upx, const double& upy, const double& upz)
	{
	  if(!m_debug)
	  {
		m_eye.m_x = eyex;
		m_eye.m_y = eyey;
		m_eye.m_z = eyez;
		
		Vertex vectLockOn;
		vectLockOn.m_x = eyex - centerx;
		vectLockOn.m_y = eyey - centery;
		vectLockOn.m_z = eyez - centerz;
	
		m_targets[0].m_x = centerx;//face
		m_targets[0].m_y = centery;
		m_targets[0].m_z = centerz;
		m_targets[1].m_x = vectLockOn.m_y*upz - upy*vectLockOn.m_z;//gauche
		m_targets[1].m_y = vectLockOn.m_z*upx - upz*vectLockOn.m_x;
		m_targets[1].m_z = vectLockOn.m_x*upy - upx*vectLockOn.m_y;
		m_targets[2].m_x = vectLockOn.m_x;//dos
		m_targets[2].m_y = vectLockOn.m_y;
		m_targets[2].m_z = vectLockOn.m_z;
		m_targets[3].m_x = -m_targets[1].m_x;//droite
		m_targets[3].m_y = -m_targets[1].m_y;
		m_targets[3].m_z = -m_targets[1].m_z;
		m_targets[4].m_x = -(vectLockOn.m_y*m_targets[1].m_z - m_targets[1].m_y*vectLockOn.m_z);//bas
		m_targets[4].m_y = -(vectLockOn.m_z*m_targets[1].m_x - m_targets[1].m_z*vectLockOn.m_x);
		m_targets[4].m_z = -(vectLockOn.m_x*m_targets[1].m_y - m_targets[1].m_x*vectLockOn.m_y);
		m_targets[5].m_x = -m_targets[4].m_x;//haut
		m_targets[5].m_y = -m_targets[4].m_y;
		m_targets[5].m_z = -m_targets[4].m_z;
	  }
	  else
	  {
	    m_debugCoordinates[0].m_x = eyex;
	    m_debugCoordinates[0].m_y = eyey;
	    m_debugCoordinates[0].m_z = eyez;
	    m_debugCoordinates[1].m_x = centerx;
	    m_debugCoordinates[1].m_y = centery;
	    m_debugCoordinates[1].m_z = centerz;
	    m_debugCoordinates[2].m_x = upx;
	    m_debugCoordinates[2].m_y = upy;
	    m_debugCoordinates[2].m_z = upz;
	  }
	  
	}
	
	void Display::setSettings(const double& near, const double& far)
	{
		m_far = far;
		m_near = near;
	}
	
	void Display::setRenderFunc(void (*render)())
	{
	  class RendererCompat : public Renderer {
		 void (*m_scene)();
		 virtual void draw() {
			m_scene();
		 }
	  public:
		 RendererCompat(void (*scene)()) : m_scene(scene) { }
	  };
	  renderer(new RendererCompat(render));
	}

  std::auto_ptr<Renderer> Display::renderer(std::auto_ptr<Renderer> renderer) {
	 std::auto_ptr<Renderer> old = scene;
	 if (renderer.get())
		scene = renderer;
	 return old;
  }
	 void Display::rotationImage (GLdouble alpha)
	 {
	    m_angleRot+=alpha;
	 }
	 void Display::scaleImage (GLdouble zoom)
	 {
	   m_zoom+=zoom;
	 }
	 void Display::moveImageX(GLdouble dist)
	 {
	   m_moveTexX += dist;
	 }
	 void Display::moveImageY(GLdouble dist)
	 {
	   m_moveTexY += dist;
	 }
	 void Display::moveImageZ(GLdouble dist)
	 {
	   m_moveTexZ += dist;
	 }
	 void Display::setTexDefaulft()
	 {
	    m_angleRot = 0.0;
	    m_zoom = 1.0;
	    m_moveTexX = 0.0;
	    m_moveTexY = 0.0;
	    m_moveTexZ = 0.0;
	 }
	 void Display::setCursorCoordinates(GLdouble x , GLdouble y)
	 {
	   m_mouseX = x;
	   m_mouseY = y;
	 }
	 void Display::setDisplayCursor(SDL_Surface* cursor,std::string filename)
	 {
	   m_cursor = cursor;
	 }
	 
}//end of Hemi Namespace
