
// Global includes
#include <cstdio>                       // std::printf
#include <cstdlib>                      // EXIT_SUCCESS
//#include <GL/freeglut.h>                // GLUT
#ifndef WIN32
    #include <JU/Timer.hpp>             // Timer
    #include <JU/FrameRateTracker.hpp>  // FrameRateTracker
#endif

#include <core/GameManager.hpp>			// JU::GameManager
#include <core/Singleton.hpp>			// JU::Singleton
#include <core/Registry.hpp>			// JU::Registry

// Local includes
#include "GLSceneFaster.hpp"    		// GLSceneBounce
#include "IOHelper.hpp"					// JU::KeyID
#include "BubbleGameStateFactory.hpp"	// BubbleGameStateFactory
#include "BubbleGameStates.hpp"			// BubbleGameState


#ifdef _WIN32
    #define ESC_KEY VK_ESCAPE
#elif __linux__ || __CYGWIN__
    #define ESC_KEY 27
#endif



//Called whenever the window is resized. The new window size is given, in pixels.
//This is an opportunity to call glViewport or glScissor to keep up with the change in size.
/*
static void reshape (int w, int h)
{
    gl::Viewport(0, 0, (GLsizei) w, (GLsizei) h);
    scene->resize(w, h);
}
*/


/*
static void eventHandler()
{
	//Event handler
	SDL_Event event;

    // Send event to AntTweakBar
    static bool handled = TwEventSDL(&event, SDL_MAJOR_VERSION, SDL_MINOR_VERSION);

    if (!handled)
    {
		//Handle events on queue
		while (SDL_PollEvent( &event ) != 0)
		{
			switch(event.type)
			{
				case SDL_QUIT:
					quit = true;
					break;

				case SDL_WINDOWEVENT:   // Window size has changed
					if (event.window.event == SDL_WINDOWEVENT_RESIZED)
					{
						// Resize SDL video mode
						int width = event.window.data1;
						int height = event.window.data2;
						SDL_SetWindowSize(mainwindow, width, height);
						//fprintf(stderr, "WARNING: Video mode set failed: %s\n", SDL_GetError());
						// Resize OpenGL viewport
						scene->resize(width, height);
					}
					break;

				case SDL_KEYDOWN:
					keyboard(&event);
					break;

				case SDL_MOUSEMOTION:
				{
					//Get mouse position
					int x, y;
					SDL_GetMouseState (&x, &y);
					scene->mouseMotion(x, y);
				}
					break;

				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
				{
					//Get mouse position
					int x, y;
					SDL_GetMouseState (&x, &y);
					scene->mouseClick(JU::IO::SDL2ToMouseButtonID(event.button.button),
									  JU::IO::SDL2ToMouseButtonState(event.button.state),
									  x, y);
				}
					break;

				case SDL_MOUSEWHEEL:
					// Scrolling up
					if (event.wheel.y > 0)
						scene->mouseClick(JU::IO::BUTTON_MIDDLE_SCROLL_UP, JU::IO::BUTTON_STATE_UNKNOWN, 0, 0);
					else if (event.wheel.y < 0)
						scene->mouseClick(JU::IO::BUTTON_MIDDLE_SCROLL_DOWN, JU::IO::BUTTON_STATE_UNKNOWN, 0, 0);
					break;

				default:
					std::printf ("Unhandled SDL2 event %i\n", event.type);
					break;
			}
		}
    }
}
*/


/*
//Called to update the display.
//You should call glutSwapBuffers after all of your rendering to display what you rendered.
//If you need continuous updates of the screen, call glutPostRedisplay() at the end of the function.
static void display(SDL_Window *window)
{
    static JU::f32 time_diff = 0.0f;

    #ifndef WIN32
        static JU::f32 last_time = timer.getElapsedMilliseconds();
        JU::f32 time 			 = timer.getElapsedMilliseconds();
	#else
        static JU::f32 last_time = glutGet(GLUT_ELAPSED_TIME);
        JU::f32 time 			 = glutGet(GLUT_ELAPSED_TIME);
	#endif

	time_diff = time - last_time;
	last_time = time;

	scene->update(time_diff);
    scene->render();

    SDL_GLSwapWindow(window);

    // Print stats
	frame_rate.update(time);

	static int frames = 0;
	if (!(++frames % 100))
	{
		printf("\rFPS = %f; Current Shader Program = %s                        ",
			   frame_rate.getFPS(),
			   scene->getGLSLCurrentProgramString());

		fflush(stdout);
	}
}
*/

/*
static void init(void)
{
    event_manager = new JU::EventManager;
    event_manager->initialize();

    //scene = new GLSceneParticles(WIDTH, HEIGHT);
    scene = new GLSceneFaster(WIDTH, HEIGHT);

    scene->init();

    #ifndef WIN32
        timer.start();
    #endif
}
*/


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
