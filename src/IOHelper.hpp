/*
 * InputHandleDefs.hpp
 *
 *  Created on: May 29, 2015
 *      Author: jusabiaga
 */

#ifndef IOHELPER_HPP_
#define IOHELPER_HPP_

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>


namespace JU
{

namespace IO
{

enum KeyID
{
	KEY_0,
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9,
	KEY_C,
	KEY_R,
	KEY_ESC,
	KEY_UNKNOWN,
};

enum MouseButtonID
{
	BUTTON_LEFT,
	BUTTON_MIDDLE,
	BUTTON_RIGHT,
	BUTTON_MIDDLE_SCROLL_UP,
	BUTTON_MIDDLE_SCROLL_DOWN,
	BUTTON_ID_UNKNOWN,
};

enum MouseButtonState
{
	BUTTON_DOWN,
	BUTTON_UP,
	BUTTON_STATE_UNKNOWN,
};

KeyID SDL2ToKeyID(SDL_Keycode keycode)
{
	switch(keycode)
	{
		case SDLK_ESCAPE:
			return KEY_ESC;
		case SDLK_0:
			return KEY_0;
		case SDLK_1:
			return KEY_1;
		case SDLK_2:
			return KEY_2;
		case SDLK_3:
			return KEY_3;
		case SDLK_4:
			return KEY_4;
		case SDLK_5:
			return KEY_5;
		case SDLK_6:
			return KEY_6;
		case SDLK_7:
			return KEY_7;
		case SDLK_8:
			return KEY_8;
		case SDLK_9:
			return KEY_9;
		case SDLK_l:
			return KEY_C;
		case SDLK_r:
			return KEY_R;

		default:
			break;

	}
	return KEY_UNKNOWN;
}



MouseButtonID SDL2ToMouseButtonID(JU::uint8 button_id)
{
	switch (button_id)
	{
		case SDL_BUTTON_LEFT:
			return BUTTON_LEFT;
		case SDL_BUTTON_MIDDLE:
			return BUTTON_MIDDLE;
		case SDL_BUTTON_RIGHT:
			return BUTTON_RIGHT;

		default:
			break;
	}

	return BUTTON_ID_UNKNOWN;
}



MouseButtonState SDL2ToMouseButtonState(JU::uint8 button_state)
{
	if (button_state == SDL_PRESSED)
		return BUTTON_DOWN;
	else
		return BUTTON_UP;

	return BUTTON_STATE_UNKNOWN;
}

} // namespace IO
} // namespace JU


#endif /* _HPP_ */
