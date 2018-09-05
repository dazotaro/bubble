
// Global includes
#include <cstdio>                       // std::printf
#include <cstdlib>                      // EXIT_SUCCESS
//#include <GL/freeglut.h>                // GLUT
#ifndef WIN32
//    #include <JU/Timer.hpp>             // Timer
//    #include <JU/FrameRateTracker.hpp>  // FrameRateTracker
#endif

#include <core/GameManager.hpp>			// JU::GameManager
#include <core/Singleton.hpp>			// JU::Singleton
#include <core/Registry.hpp>			// JU::Registry

// Local includes
#include "GLSceneFaster.hpp"    		// GLSceneBounce
#include "BubbleGameStateFactory.hpp"	// BubbleGameStateFactory
#include "BubbleGameStates.hpp"			// BubbleGameState


#ifdef _WIN32
    #define ESC_KEY VK_ESCAPE
#elif __linux__ || __CYGWIN__
    #define ESC_KEY 27
#endif



static void cleanup(void)
{
}



// Main method - main entry point of application
//the freeglut library does the window creation work for us,
//regardless of the platform.
int main(int argc, char** argv)
{
	// Register Factories
	BubbleGameStateFactory state_factory;
	JU::Registry* registry = JU::Singleton<JU::Registry>::getInstance();
	registry->setObjectFactory(&state_factory);

	// GameManager: CREATE and INITIALIZE
	JU::GameManager game_manager;
	game_manager.initialize();

	// ADD STATES
	JU::GameStateManager& state_manager = game_manager.getStateManager();
	state_manager.addState("BubbleGameState", new BubbleGameState);
	state_manager.changeState("BubbleGameState");

	game_manager.loop();
	game_manager.exit();

    // Delete dynamically memory
    cleanup();

    return 0;
}
