/*
 * FasterGameStates.hpp
 *
 *  Created on: Jun 5, 2015
 *      Author: jusabiaga
 */

#ifndef FASTERGAMESTATES_HPP_
#define FASTERGAMESTATES_HPP_

#include <JU/core/GameState.hpp>	// GameState

class DefaultGameState : public JU::GameState
{
	public:

		bool load();
		bool initialize();
        bool update();
		bool draw();
		bool free();
		bool unload();
};

#endif /* FASTERGAMESTATES_HPP_ */
