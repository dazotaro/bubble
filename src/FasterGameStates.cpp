/*
 * FasterGameStates.cpp
 *
 *  Created on: Jun 5, 2015
 *      Author: jusabiaga
 */

// Local includes
#include "FasterGameStates.hpp"

// Global includes
#include <JU/graphics/GLScene.hpp>


bool DefaultGameState::load()
{
	GLSceneFaster* p_scene = new GLScene(800, 800);

	registerGLScene(p_scene);

    return true;
}


bool DefaultGameState::initialize()
{
	p_scene_->init();

    return true;
}



bool DefaultGameState::update()
{

    return true;
}



bool DefaultGameState::free()
{

    return true;
}



bool DefaultGameState::unload()
{

    return true;
}
