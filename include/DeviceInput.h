/***************************************************************************
 *   Copyright (C) 2010 by Gael Boulet   *
 *   ggb0ot@pcfg3.dcs.aber.ac.uk   *
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
#include <cstdio>
#include <cstdlib>
/**
	This class provide an interface with a bluetooth device, using RFComm socket.
	The maximum size of data send simultaneously by the device is 500 characters.
	The class is optimized for the application develop on the mobile phone nokia n97.
*/

namespace Hemi
{
	class DeviceInput
	{
	public :
		/*! enumeration of the different type of information receive.
		*/
		enum input {DATA_TYPE, MOUSE_TYPE, KEY_TYPE, MOUSE_RIGHT_BUTTON, MOUSE_LEFT_BUTTON, DISCONNECTED, MOUSE_BUTTON_UP};
	
		/*! Constructor
		initialize the bluetooth connection, but doesn't start it.
		@param nbChannel number of data channel listened
		*/
		DeviceInput	(int nbChannel);
	
		virtual ~DeviceInput	() {};
	
		/*! Start the bluetooth connection
		this function should be called to connect the device to the computer. This function must be called just before the connection of the device to the computer. When this function is called, the program wait for the connection of a bluetooth device.
		@return  true if the connection is enable, false otherwise.
		*/
		virtual bool 		startListening() = 0;
	
		/*! Stop the bluetooth connection and close the socket.
		*/
		virtual void	 	stopListening() = 0;
	

		/*! return the character receive from the phone keyboard if the data receive are a character, if it's not, the function return the type of the data receive.
		@return the touch press by the user or '$' for enter, '<' for backspace and '^' for space.
		*/
		virtual char		getKeyInput() const;

		/*! return the mouse button if the data receive is a mouse button, if it's not, the function return the type of the data receive
		@return MOUSE_RIGHT_BUTTON ( = input(3)) for the right button or MOUSE_LEFT_BUTTON ( = input(4)) for the left button.
		*/
		virtual input		getMouseInput();

		/*! return the type of information receive. this function must be called regularly to receive new data.
		@return DATA_TYPE ( = input(0)) for the sensors information, MOUSE_TYPE ( = input(1)) for the mouse button information or KEY_TYPE ( = input(2))for the keyboard information. if the device is disconnect, the function return DISCONNECTD ( = input(5)).
		*/
		virtual input		getInputEvent() = 0;

		/*! return the angle of the phone on the horizontal plan, calcul with the accelerometer data or the magnetometer data.
		@return the horizontal angle;
		*/
		int 		getHorizontalAngle() const;

		/*! return the angle of the phone on the vertical plan, calcul with the accelerometer data.
		@return the vertical angle.
		*/
		virtual int 		getVerticalAngle() const;

		/*! return the state of the bluetooth connection.
		@return true if the device is connected, false otherwise.
		*/
		virtual bool 		isConnected() const;

		/*! return the state of the button, there is no difference betzeen the button, if one button is pushed, the function will return true.
		@return true if q button is pushed, false otherwise.
		*/
		virtual bool		buttonPushed() const;

		/*! return one sensor information if the data receive is a sensor information, if it's not, the function return the type of the data receive
		@param i number of the sensor channel :
			0 : accelerometer x-axis; 
			1 : accelerometer y-axis; 
			2 : accelerometer z-axis; 
			3 : north angle return by the phone API; 
			4 : magnetometer x-axis; 
			5 : magnetometer y-axis; 
			6 : magnetometer z-axis.
		@return the sensor data.
		*/
		virtual int getData	(int i) const;

		/*! return multiple sensor information.
		@param tab array of indices of sensors, for the list of channel's indice, see the other getData function.
		@return a integer pointer to the array of information.
		*/
		virtual const int* getData	(const int* tab) const;

		/*! return the buffer of incoming data without any process.
		@return a char pointer to the array of data.
		*/
		virtual const char* getBuffer() const;


	protected :
		bool 		connected;
		bool		button_pushed;
		int 		nbData;
		input 		type;

		char  		buf[500];
		int 		s, client, bytes_read;

		int getOneData( int i) const;
	};
}
