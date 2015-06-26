/*
 * FasterGameStates.hpp
 *
 *  Created on: Jun 5, 2015
 *      Author: jusabiaga
 */

#ifndef BUBBLEGAMESTATES_HPP_
#define BUBBLEGAMESTATES_HPP_

#include <core/GameStateInterface.hpp>	// GameState

// Forward Declarations
namespace JU
{
	class GLScene;
}


class BubbleGameState : public JU::GameStateInterface
{
	public:
		BubbleGameState();
		~BubbleGameState();

	public:
		// GameStateInterface
		// ------------------
		bool enter();
		bool synchronize();
		bool commonEnterSynchronize();
        bool update();
		bool draw();
		bool exit();
		bool suspend();
		bool commonExitSuspend();

	private:
		JU::GLScene* p_scene_;
};

#endif /* BUBBLEGAMESTATES_HPP_ */
