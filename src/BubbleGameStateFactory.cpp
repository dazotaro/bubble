/*
 * BubbleGameStateFactory.cpp
 *
 *  Created on: Jun 25, 2015
 *      Author: jusabiaga
 */

// Local includes
#include "BubbleGameStateFactory.hpp"
#include "BubbleGameStates.hpp"
// Global includes
#include <core/GameStateInterface.hpp>


BubbleGameStateFactory::BubbleGameStateFactory()
{
	// TODO Auto-generated constructor stub

}


BubbleGameStateFactory::~BubbleGameStateFactory()
{
	// TODO Auto-generated destructor stub
}


JU::GameStateInterface* BubbleGameStateFactory::create(const std::string& state_name) const
{
	if (state_name == "BubbleGameState")
		return new BubbleGameState();
	else
		// Let the base class handle it
		return JU::GameStateFactory::create(state_name);
}


