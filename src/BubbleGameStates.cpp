/*
 * FasterGameStates.cpp
 *
 *  Created on: Jun 5, 2015
 *      Author: jusabiaga
 */

// Local includes
#include "BubbleGameStates.hpp"
#include "GLSceneFaster.hpp"		// GLSceneFaster


BubbleGameState::BubbleGameState() : JU::GameStateInterface("BubbleGameState"), p_scene_(nullptr)
{

}


BubbleGameState::~BubbleGameState()
{

}


bool BubbleGameState::enter()
{
	p_scene_ = new GLSceneFaster(1280, 720);
	p_scene_->init();

    return true;
}


bool BubbleGameState::synchronize()
{
    return true;
}


bool BubbleGameState::commonEnterSynchronize()
{
    return true;
}


bool BubbleGameState::update(JU::uint32 time)
{
	p_scene_->update(time);

    return true;
}


bool BubbleGameState::draw()
{
	p_scene_->render();

    return true;
}


bool BubbleGameState::exit()
{
	p_scene_->clear();
	delete p_scene_;

	return true;
}


bool BubbleGameState::suspend()
{
    return true;
}


bool BubbleGameState::commonExitSuspend()
{
    return true;
}

