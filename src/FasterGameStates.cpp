/*
 * FasterGameStates.cpp
 *
 *  Created on: Jun 5, 2015
 *      Author: jusabiaga
 */

// Local includes
#include "FasterGameStates.hpp"

// Global includes
#include "GLSceneFaster.hpp"


bool DefaultGameState::load()
{
	p_scene_ = new GLSceneFaster(800, 800);

	registerGLScene(static_cast<JU::GLScene*>(p_scene_));

    return true;
}


bool DefaultGameState::initialize()
{
	p_scene_->init();

    return true;
}



bool DefaultGameState::update()
{
	p_scene_->update(1000);

    return true;
}



bool DefaultGameState::draw()
{
	p_scene_->render();

    return true;
}



bool DefaultGameState::free()
{

    return true;
}



bool DefaultGameState::unload()
{
	p_scene_->clear();

    return true;
}
