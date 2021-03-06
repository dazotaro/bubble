/*
 * GLScene.hpp
 *
 *  Created on: May 21, 2013
 *      Author: jusabiaga
 */

#ifndef GLSCENEFASTER_HPP_
#define GLSCENEFASTER_HPP_

// Global includes
#include <core/SDLEventManager.hpp>			// JU::SDLEventHandler
#include <graphics/GLScene.hpp>      		// GLScene
#include <graphics/Lights.hpp>       		// LightPositionalVector, LightDirectionalVector, LightSpotlightVector
#include <graphics/ArcBallController.hpp>	// ArcBallController
#ifdef _DEBUG
    #include <AntTweakBar.h>                     // TwBar
#endif


// FORWARD DECLARATIONS
class Bubble;
class Landscape;

namespace JU
{
class GLMesh;
class GLMeshInstance;
class Node3D;
class Transform3D;
class CameraInterface;
class CameraThirdPerson;
}

/*
 * @brief Scene class
 *
 * \todo Maybe unnecessary class
 */
class GLSceneFaster: public JU::GLScene, public JU::SDLEventHandler
{
    public:
        static const int MAX_POS_LIGHTS = 20;

        // TYPEDEFS
        typedef std::map<std::string, JU::Node3D *> 		NodeMap;
        typedef std::map<std::string, JU::GLMesh*> 			MeshMap;
        typedef std::map<std::string, JU::GLMeshInstance*> 	MeshInstanceMap;

    public:
        GLSceneFaster(int width, int height);
        virtual ~GLSceneFaster();

        void init(void);
        void update(JU::uint32 time);
        void render(void);
        void keyboard(unsigned char key, int x, int y);
        void mouseClick(int button, int state, int x, int y);
        void mouseMotion(int x, int y);
        void resize(int width, int height);
        void clear(void);

    public:
        // SDLEventHandler Interface
		void handleSDLEvent(const SDL_Event* event);

    private:
        void reload();

        void computeSceneSize(JU::uint32 width, JU::uint32 height);

        void initializePrograms();
        void initializeFBO();
        void initializeTextures();
        void initializeObjects();
        void initializeCameras();
        void initializeLights();
        void initializePositionalLights();
        void initializeDirectionalLights();
        void initializeSpotlightLights();
#ifdef _DEBUG
        void initAntTweakBar();
#endif

        void updateCamera(JU::uint32 time);
        void updateLights(JU::uint32 time);
        void updatePositionalLights(JU::uint32 time);
        void updateDirectionalLights(JU::uint32 time);
        void updateSpotlightLights(JU::uint32 time);
        void updateBubble(JU::uint32 time);
#ifdef _DEBUG
        void updateFPS(JU::uint32 time);
#endif


        void loadLights(void) const;
        void loadPositionalLights(void) const;
        void loadDirectionalLights(void) const;
        void loadSpotlightLights(void) const;

        void renderPass1();
        void renderPass2();
#ifdef _DEBUG
        void renderDebug();
#endif

    private:
        // INTERFACE
        JU::uint32 			scene_width_;
        JU::uint32 			scene_height_;

        // DEFERRED SHADING SPECIFIC
        GLuint 				deferredFBO_;
        GLuint 				depthBuf_;
        GLuint				posTex_;
        GLuint				normTex_;
        GLuint				colorTex_;
        GLuint				shininessTex_;
        GLuint 				pass1Index_;
        GLuint 				pass2Index_;
        bool				record_depth_;

        // ASSET MANAGEMENT
        Bubble*                 pbubble_;
        Landscape*              plandscape_;
        MeshMap 				mesh_map_;
        MeshInstanceMap 		mesh_instance_map_;
        NodeMap					node_map_;
        NodeMap::const_iterator main_node_iter;

        // CAMERA CONTROLS
        JU::CameraThirdPerson* tp_camera_;
        bool control_camera_;
        JU::ArcBallController camera_controller_;

        // LIGHT MANAGEMENT
        JU::LightManager::LightType light_mode_;			//!< Light Mode
        JU::int32			    	num_lights_;			//!< Number of lights (only one type of light active at one time)
        JU::LightPositionalVector   lights_positional_;		//!< Vector holding all positional lights
        JU::LightDirectionalVector  lights_directional_;	//!< Vector holding all directional lights
        JU::LightSpotlightVector    lights_spotlight_;		//!< Vector holding all spotlights

#ifdef _DEBUG
        // AntTweakBar
        TwBar* 		ptw_bar_;               //!< AnttweakBar handle
        glm::vec3 	tw_force_direction_;    //!< Bubble's force direction
        JU::f32 	tw_force_strength_;     //!< Bubble's force strength
        JU::f32		tw_scale_maxi_;         //!< Bubble's maxi's scale
        JU::f32 	tw_scale_mini_;         //!< Bubble's mini's scale
        JU::f32     tw_FPS_;                //!< Application's FPS
        JU::f32     tw_FOV_;                //!< Camera's FOV
#endif
};

#endif /* GLSCENEFASTER_HPP_ */
