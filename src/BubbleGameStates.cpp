/*
 * FasterGameStates.cpp
 *
 *  Created on: Jun 5, 2015
 *      Author: jusabiaga
 */

// Local includes
#include "BubbleGameStates.hpp"
#include "GLSceneFaster.hpp"		// GLSceneFaster


BubbleGameState::BubbleGameState() : JU::GameStateInterface("BubbleGameState")
{

}


BubbleGameState::~BubbleGameState()
{

}


bool BubbleGameState::load()
{
	p_scene_ = new GLSceneFaster(800, 800);

    return true;
}


bool BubbleGameState::initialize()
{
	p_scene_->init();

    return true;
}



bool BubbleGameState::update()
{
	p_scene_->update(1000);

    return true;
}



bool BubbleGameState::draw()
{
	p_scene_->render();

    return true;
}



bool BubbleGameState::free()
{

    return true;
}



bool BubbleGameState::unload()
{
	p_scene_->clear();

    return true;
}
