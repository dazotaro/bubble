/*
 * GLScene.cpp
 *
 *  Created on: May 21, 2013
 *      Author: jusabiaga
 */

// Local includes
#include "GLSceneFaster.hpp"      	// GLSceneFaster
#include "IOHelper.hpp"				// SDL to IO helper functions
#include "Bubble.hpp"               // Bubble
#include "Landscape.hpp"            // Landscape

// Global includes
#include <graphics/GLMesh.hpp>               // GLMesh
#include <graphics/GLMeshInstance.hpp>       // GLMeshInstance
#include <graphics/Node3D.hpp>               // Node3D
#include <graphics/CameraInterface.hpp>      // CameraInterface
#include <graphics/CameraFirstPerson.hpp>    // CameraFirstPerson
#include <graphics/CameraThirdPerson.hpp>    // CameraThirdPerson
#include <graphics/ShapeHelper2.hpp>         // build Mesh helper funtions
#include <graphics/TextureManager.hpp>       // loadTexture()
#include <graphics/Material.hpp>             // MaterialManager
#include <graphics/DebugGlm.hpp>             // debug::print
#include <graphics/GLSLProgramExt.hpp>		 // GLSLProgramExt::setUniform
#include <graphics/MeshImporter.hpp>         // MeshImporter (Assimp)
#include <core/Transform3D.hpp>     		 // Transform3D
#include <core/Singleton.hpp>				 // JU::Singleton
#include <core/Keyboard.hpp>                 // JU::Keyboard
#include <core/SystemLog.hpp>				 // JU::SystemLog
#include <glm/gtx/transform.hpp>			 // glm::rotate
#include <math.h>							 // M_PI



GLSceneFaster::GLSceneFaster(int width, int height) : JU::GLScene(width, height),
									 scene_width_(0), scene_height_(0),
									 deferredFBO_(0), depthBuf_(0), posTex_(0), normTex_(0), colorTex_(0), shininessTex_(0),
									 pass1Index_(0), pass2Index_(0), record_depth_(true),
									 pbubble_(nullptr), plandscape_(nullptr),
									 tp_camera_(nullptr), camera_(nullptr), control_camera_(true), camera_controller_(width, height, 0.2f),
									 light_mode_(JU::LightManager::POSITIONAL), num_lights_(2)
#ifdef _DEBUG
                                     , ptw_bar_(nullptr), tw_force_direction_(glm::vec3(0.0f, 0.0f, 1.0f)), tw_force_strength_(5.0f),
									 tw_scale_maxi_(5.0f), tw_scale_mini_(1.5f)
#endif
{
	computeSceneSize(width, height);
	main_node_iter = node_map_.end();
}



GLSceneFaster::~GLSceneFaster()
{
    // TODO Auto-generated destructor stub
}



/**
* @brief Initialized the Scene
*/
void GLSceneFaster::init(void)
{
    gl::ClearColor(0.0,0.0,0.0,1.0);
    gl::Enable(gl::DEPTH_TEST);
    gl::DepthMask(gl::TRUE_);
    gl::DepthFunc(gl::LEQUAL);
    gl::DepthRange(0.0f, 1.0f);
    gl::Enable(gl::CULL_FACE); // enables face culling
    gl::CullFace(gl::BACK); // tells OpenGL to cull back faces (the sane default setting)

    initializePrograms();
    initializeFBO();
    initializeTextures();
    initializeObjects();
    initializeCameras();
    initializeLights();

	JU::SDLEventManager* SDL_event_manager = JU::Singleton<JU::SDLEventManager>::getInstance();
	SDL_event_manager->attachEventHandler(SDL_WINDOWEVENT, 		"SceneResize", 		this);
	SDL_event_manager->attachEventHandler(SDL_MOUSEMOTION, 		"SceneMouseMotion", this);
	SDL_event_manager->attachEventHandler(SDL_MOUSEBUTTONDOWN, 	"SceneButtonDown", 	this);
	SDL_event_manager->attachEventHandler(SDL_MOUSEBUTTONUP, 	"SceneButtonUp", 	this);
	SDL_event_manager->attachEventHandler(SDL_MOUSEWHEEL, 		"SceneMouseWheel", 	this);

#ifdef _DEBUG
    initAntTweakBar();
#endif
}



void GLSceneFaster::initializePrograms()
{
	glsl_program_map_["debug"]  = compileAndLinkShader("data/shaders/deferred.vs", "data/shaders/image_texture.fs");

	if (record_depth_)
	{
		glsl_program_map_["deferred"]  = compileAndLinkShader("data/shaders/deferred.vs", "data/shaders/deferred_depth.fs");
	}
	else
	{
		glsl_program_map_["deferred"]  = compileAndLinkShader("data/shaders/deferred.vs", "data/shaders/deferred.fs");
	}

    current_program_iter_ = glsl_program_map_.find("deferred");
    current_program_iter_->second.use();

    /*
    current_program_iter_ ->second.setUniform("PositionTex", 0);
    current_program_iter_ ->second.setUniform("NormalTex", 1);
    current_program_iter_ ->second.setUniform("ColorTex", 2);
    */

    // Set up the subroutine indexes
    GLuint programHandle = current_program_iter_ ->second.getHandle();
    pass1Index_ = gl::GetSubroutineIndex( programHandle, gl::FRAGMENT_SHADER, "pass1");
    pass2Index_ = gl::GetSubroutineIndex( programHandle, gl::FRAGMENT_SHADER, "pass2");
}



/**
* @brief Initialize RenderBuffers and Textures needed by Deferred Shading's first pass
*/
void GLSceneFaster::initializeFBO()
{
    // Create and bind the FBO
    gl::GenFramebuffers(1, &deferredFBO_);
    gl::BindFramebuffer(gl::FRAMEBUFFER, deferredFBO_);

    // The DEPTH buffer
    if (record_depth_)
    {
        // if want to access it in the shader
        gl::GenTextures(1, &depthBuf_);
        gl::BindTexture(gl::TEXTURE_2D, depthBuf_);
        gl::TexImage2D(gl::TEXTURE_2D, 0, gl::DEPTH_COMPONENT24, scene_width_, scene_height_, 0, gl::DEPTH_COMPONENT, gl::FLOAT, 0);
        gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MAG_FILTER, gl::NEAREST);
        gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MIN_FILTER, gl::NEAREST);
        gl::FramebufferTexture2D(gl::FRAMEBUFFER, gl::DEPTH_ATTACHMENT, gl::TEXTURE_2D, depthBuf_, 0);
        JU::TextureManager::registerTexture("DepthTex", depthBuf_);
    }
    else
    {
    	// if we do not need to explicitly access it
		gl::GenRenderbuffers(1, &depthBuf_);
		gl::BindRenderbuffer(gl::RENDERBUFFER, depthBuf_);
		gl::RenderbufferStorage(gl::RENDERBUFFER, gl::DEPTH_COMPONENT, scene_width_, scene_height_);
	    gl::FramebufferRenderbuffer(gl::FRAMEBUFFER, gl::DEPTH_ATTACHMENT, gl::RENDERBUFFER, depthBuf_);
    }

    // The position buffer
    gl::GenTextures(1, &posTex_);
    gl::BindTexture(gl::TEXTURE_2D, posTex_);
    gl::TexImage2D(gl::TEXTURE_2D, 0, gl::RGB32F, scene_width_, scene_height_, 0, gl::RGB, gl::UNSIGNED_BYTE, NULL);
    gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MIN_FILTER, gl::NEAREST);
    gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MAG_FILTER, gl::NEAREST);

    // The normal buffer
    gl::GenTextures(1, &normTex_);
    gl::BindTexture(gl::TEXTURE_2D, normTex_);
    gl::TexImage2D(gl::TEXTURE_2D, 0, gl::RGB32F, scene_width_, scene_height_, 0, gl::RGB, gl::UNSIGNED_BYTE, NULL);
    gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MIN_FILTER, gl::NEAREST);
    gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MAG_FILTER, gl::NEAREST);

    // The color buffer
    gl::GenTextures(1, &colorTex_);
    gl::BindTexture(gl::TEXTURE_2D, colorTex_);
    gl::TexImage2D(gl::TEXTURE_2D, 0, gl::RGB, scene_width_, scene_height_, 0, gl::RGB, gl::UNSIGNED_BYTE, NULL);
    gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MIN_FILTER, gl::NEAREST);
    gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MAG_FILTER, gl::NEAREST);

    // The shininess buffer
    gl::GenTextures(1, &shininessTex_);
    gl::BindTexture(gl::TEXTURE_2D, shininessTex_);
    gl::TexImage2D(gl::TEXTURE_2D, 0, gl::R32F, scene_width_, scene_height_, 0,  gl::RED, gl::FLOAT, NULL);
    gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MIN_FILTER, gl::NEAREST);
    gl::TexParameteri(gl::TEXTURE_2D, gl::TEXTURE_MAG_FILTER, gl::NEAREST);

    // Attach the images to the frame buffer
    gl::FramebufferTexture2D(gl::FRAMEBUFFER, gl::COLOR_ATTACHMENT0, gl::TEXTURE_2D, posTex_, 0);
    gl::FramebufferTexture2D(gl::FRAMEBUFFER, gl::COLOR_ATTACHMENT1, gl::TEXTURE_2D, normTex_, 0);
    gl::FramebufferTexture2D(gl::FRAMEBUFFER, gl::COLOR_ATTACHMENT2, gl::TEXTURE_2D, colorTex_, 0);
    gl::FramebufferTexture2D(gl::FRAMEBUFFER, gl::COLOR_ATTACHMENT3, gl::TEXTURE_2D, shininessTex_, 0);

    GLenum drawBuffers[] = {gl::NONE,
    						gl::COLOR_ATTACHMENT0,
							gl::COLOR_ATTACHMENT1,
							gl::COLOR_ATTACHMENT2,
							gl::COLOR_ATTACHMENT3};
    gl::DrawBuffers(5, drawBuffers);

    // Check that our frame buffer is complete
    if(gl::CheckFramebufferStatus(gl::FRAMEBUFFER) != gl::FRAMEBUFFER_COMPLETE)
    {
    	std::printf("Incomplete FrameBuffer Status. Bailing out!!!");
    	exit(EXIT_FAILURE);
    }

    // Register the texture handles with TextureManager
    JU::TextureManager::registerTexture("PositionTex", 	posTex_);
    JU::TextureManager::registerTexture("NormalTex", 	normTex_);
    JU::TextureManager::registerTexture("ColorTex", 	colorTex_);
    JU::TextureManager::registerTexture("ShininessTex", shininessTex_);

    gl::BindFramebuffer(gl::FRAMEBUFFER, 0);
}



void GLSceneFaster::initializeTextures()
{
    JU::TextureManager::loadTexture("test",  "data/textures/test.tga");
    JU::TextureManager::loadTexture("brick", "data/textures/brick1.jpg");
    JU::TextureManager::loadTexture("pool",  "data/textures/pool.png");
    JU::TextureManager::loadTexture("light", "data/textures/light_texture.tga");
}



void GLSceneFaster::initializeObjects()
{
	JU::GLMesh* 		pmesh;
	JU::GLMeshInstance* pmesh_instance;
	JU::Node3D*			pnode;
	JU::Mesh2 			mesh;

    // BUBBLE
    // ------
    pbubble_ = new Bubble();
    pbubble_->init();
    // NODE: give the sphere a position and a orientation
    JU::Transform3D bubble3d(glm::vec3(0.0f, 2.0f,  0.0f), // Model's position
                             glm::vec3(1.0f,  0.0f, 0.0f), // Model's X axis
                             glm::vec3(0.0f,  1.0f, 0.0f), // Model's Y axis
                             glm::vec3(0.0f,  0.0f, 1.0f));// Model's Z axis
    pnode = new JU::Node3D(bubble3d, pbubble_, true);
    node_map_["bubble"] = pnode;

    main_node_iter = node_map_.find("bubble");

    // LANDSCAPE OF BLOCKS
    // ------
    const JU::uint32 num_rows = 4;
    const JU::uint32 num_cols = 8;
    Landscape::BlockInfo land_data[num_rows * num_cols] =
    {
        4, 1, 0, 0, 0, 0, 1, 5,
        3, 2, 0, 0, 0, 0, 1, 4,
        2, 2, 0, 0, 0, 0, 1, 3,
        1, 1, 0, 0, 0, 0, 1, 2
    };
    glm::vec3 grid_scale(2.0f, 2.0f, 1.0f);
    plandscape_ = new Landscape();
    plandscape_->init(land_data, num_rows, num_cols, grid_scale);
    // NODE: give the sphere a position and a orientation
    JU::Transform3D landscape3d(glm::vec3(num_cols*(-0.5f)*grid_scale.x ,
                                grid_scale.z * 0.5f,
                                num_rows*(-0.5f)*grid_scale.y ), // Model's position in wold coordinates
                                glm::vec3(1.0f, 0.0f,  0.0f), // Model's X axis
                                glm::vec3(0.0f, 0.0f, -1.0f), // Model's Y axis
                                glm::vec3(0.0f, 1.0f,  0.0f));// Model's Z axis
    pnode = new JU::Node3D(landscape3d, plandscape_, true);
    node_map_["landscape"] = pnode;

	// SPHERE (to be used by lights)
	// ------
	// MESH
	JU::ShapeHelper2::buildMesh(mesh, JU::ShapeHelper2::SPHERE, 64, 32);
	mesh.computeTangents();
	pmesh = new JU::GLMesh();
	// Load the Mesh into VBO and VAO
	pmesh->init(mesh);
	mesh_map_["sphere_64_32"] = pmesh;

    // PLANE
    // ------
    // MESH
    JU::ShapeHelper2::buildMesh(mesh, JU::ShapeHelper2::PLANE);
    pmesh = new JU::GLMesh();
    // Load the Mesh into VBO and VAO
    pmesh->init(mesh);
    mesh_map_["plane"] = pmesh;
    // MESH INSTANCE
    glm::vec3 scale(50.0f, 50.0f, 1.0f);
    pmesh_instance = new JU::GLMeshInstance(pmesh, scale.x, scale.y, scale.z, JU::MaterialManager::getMaterial("gray_rubber"));
    pmesh_instance->addColorTexture("brick");
    mesh_instance_map_["plane_green"] = pmesh_instance;
    // NODE
    // Give the plane a position and a orientation
    JU::Transform3D plane(glm::vec3(0.0f, 0.0f, 0.0f), // Model's position
                          glm::vec3(1.0f, 0.0f, 0.0f), // Model's X axis
                          glm::vec3(0.0f, 0.0f,-1.0f), // Model's Y axis
                          glm::vec3(0.0f, 1.0f, 0.0f));// Model's Z axis
    pnode = new JU::Node3D(plane, pmesh_instance, true);
	node_map_["plane"] = pnode;

	// ASSIMP MESH
	// -----------
    const char* filename = "./models/monkey.obj";
    if (!JU::MeshImporter::import(filename, mesh))
    {
        std::printf("Could not load %s\n", filename);
        exit(EXIT_FAILURE);
    }
    //mesh.computeTangents();
    pmesh = new JU::GLMesh();
    pmesh->init(mesh);
    mesh_map_["assimp"] = pmesh;
    // MESH INSTANCE
    pmesh_instance = new JU::GLMeshInstance(pmesh, 1.0f, 1.0f, 1.0f, JU::MaterialManager::getMaterial("gold"));
    pmesh_instance->addColorTexture("assimp");
    mesh_instance_map_["assimp"] = pmesh_instance;
    // NODE: give the sphere a position and a orientation
    JU::Transform3D assimp3d(glm::vec3(0.0f,  5.0f,  0.0f), // Model's position
                             glm::vec3(1.0f,  0.0f, 0.0f), // Model's X axis
                             glm::vec3(0.0f,  1.0f, 0.0f), // Model's Y axis
                             glm::vec3(0.0f,  0.0f, 1.0f));// Model's Z axis
    pnode = new JU::Node3D(assimp3d, pmesh_instance, true);
    node_map_["assimp"] = pnode;



	// QUAD: For deferred shading's second pass (screen filling quad)
	// --------------------------------------------------------------
	pmesh_instance = new JU::GLMeshInstance(mesh_map_["plane"], 2.0f, 2.0f, 1.0f);
    mesh_instance_map_["screen_quad"] = pmesh_instance;
}



void GLSceneFaster::initializeCameras()
{
    tp_camera_ = new JU::CameraThirdPerson(JU::CameraIntrinsic(90.f, scene_width_/(JU::f32)scene_height_, 0.5f, 1000.f),
    								   	   static_cast<JU::Transform3D>(*main_node_iter->second),
										   10.0f, 0.0f, M_PI / 4.0f);
    camera_ = dynamic_cast<JU::CameraInterface *>(tp_camera_);
}




void GLSceneFaster::initializeLights()
{
	switch (light_mode_)
	{
		case JU::LightManager::POSITIONAL:
			initializePositionalLights();
			break;

		case JU::LightManager::DIRECTIONAL:
			initializeDirectionalLights();
			break;

		case JU::LightManager::SPOTLIGHT:
			initializeSpotlightLights();
	}
}



void GLSceneFaster::initializePositionalLights()
{
	JU::f32 radius = 10.0f;
	JU::f32 channel_intensity = 1.0f / num_lights_;
    glm::vec3 light_intensity (channel_intensity);

    // GLMeshInstance
    JU::GLMeshInstance* pmesh_instance = new JU::GLMeshInstance(mesh_map_["sphere_64_32"],		// mesh
    															0.5f, 0.5f, 0.5f,				// scale
																JU::MaterialManager::getMaterial("white_plastic"));	// material
    //pmesh_instance->addColorTexture("light");
    mesh_instance_map_["light_sphere"] = pmesh_instance;

    lights_positional_.resize(num_lights_, JU::LightPositional(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f)));

    glm::vec3 light_ring_center (0.0f, 20.0f, 0.0f);
	JU::f32 angle_delta = 2.0f * M_PI / num_lights_;
    for (JU::uint8 index = 0; index < num_lights_; ++index)
    {
    	JU::f32 angle = index * angle_delta;
    	JU::f32 z = radius * cosf(angle);
    	JU::f32 x = radius * sinf(angle);
    	glm::vec3 light_pos (x, 20.f, z);
    	JU::Transform3D light_frame(light_pos,
                           	   	    glm::vec3(1.0f, 0.0f,  0.0f), // Model's X axis
							        glm::vec3(0.0f, 1.0f,  0.0f), // Model's Y axis
							        glm::vec3(0.0f, 0.0f,  1.0f));// Model's Z axis

    	JU::Node3D *pnode = new JU::Node3D(light_frame, pmesh_instance, true);

        node_map_[std::string("light_pos") + std::to_string(index)] = pnode;

        lights_positional_[index] = (JU::LightPositional(light_pos, light_intensity));
    }
}



void GLSceneFaster::initializeDirectionalLights()
{
}



void GLSceneFaster::initializeSpotlightLights()
{
	JU::f32 radius = 10.0f;
	JU::f32 channel_intensity = 1.0f / num_lights_;
    glm::vec3 light_intensity (channel_intensity);
    JU::f32 cutoff = M_PI / 4.0f;

    // GLMeshInstance
    JU::GLMeshInstance* pmesh_instance = new JU::GLMeshInstance(mesh_map_["sphere_64_32"],		// mesh
    															0.5f, 0.5f, 0.5f,				// scale
    															JU::MaterialManager::getMaterial("yellow_plastic"));  // material
    //pmesh_instance->addColorTexture("light");
    mesh_instance_map_["light_sphere"] = pmesh_instance;

    lights_spotlight_.resize(num_lights_, JU::LightSpotlight(glm::vec3(0.0f, 0.0f, 0.0f),
    													 glm::vec3(0.0f, 0.0f, 0.0f),
														 glm::vec3(0.0f, 0.0f, 0.0f),
														 0.0f));

    glm::vec3 light_ring_center (0.0f, 20.0f, 0.0f);
    JU::f32 angle_delta = 2.0f * M_PI / num_lights_;
    for (JU::uint8 index = 0; index < num_lights_; ++index)
    {
    	JU::f32 angle = index * angle_delta;
    	JU::f32 z = radius * cosf(angle);
    	JU::f32 x = radius * sinf(angle);
    	glm::vec3 light_pos (x, 20.f, z);
    	JU::Transform3D light_frame(light_pos,
                           	        glm::vec3(1.0f, 0.0f,  0.0f), // Model's X axis
								    glm::vec3(0.0f, 1.0f,  0.0f), // Model's Y axis
								    glm::vec3(0.0f, 0.0f,  1.0f));// Model's Z axis
        glm::vec3 light_dir = glm::normalize(light_pos - main_node_iter->second->getPosition());

        JU::Node3D* pnode = new JU::Node3D(light_frame, pmesh_instance, true);

        node_map_[std::string("light_pos") + std::to_string(index)] = pnode;

        lights_spotlight_[index] = (JU::LightSpotlight(light_pos, light_dir, light_intensity, cutoff));
    }
}


#ifdef _DEBUG

/**
* @brief Initialize AntTweakBar
*/
void GLSceneFaster::initAntTweakBar()
{
	// AntTweakBar
	TwInit(TW_OPENGL_CORE, NULL); // for core profile
	ptw_bar_ = TwNewBar("Bubble Testing");
    // AntTweakBar
	TwWindowSize(width_, height_);


    TwAddVarRW(ptw_bar_, "MaxiScale", TW_TYPE_FLOAT, &tw_scale_maxi_,
               " group ='Bubble' min=1.0 max=10.0 step=0.2 label='Maxi scale' help='Change the scale of the Bubbles maxi.' ");
    TwAddVarRW(ptw_bar_, "MiniScale", TW_TYPE_FLOAT, &tw_scale_mini_,
               " group ='Bubble' min=0.5 max=10.0 step=0.1 label='Mini scale' help='Change the scale of the Bubbles mini.' ");
    TwAddVarRW(ptw_bar_, "ForceDir", TW_TYPE_DIR3F, &tw_force_direction_,
               " group ='Bubble' label='ForceDir' help='Change the force direction.' ");
    TwAddVarRW(ptw_bar_, "ForceStrength", TW_TYPE_FLOAT, &tw_force_strength_,
               " group ='Bubble' min=1.0 max=10.0 step=0.1 label='ForceMag' help='Change the force strength.' ");
}
#endif


void GLSceneFaster::loadLights(void) const
{
	switch (light_mode_)
	{
		case JU::LightManager::POSITIONAL:
			loadPositionalLights();
			break;

		case JU::LightManager::DIRECTIONAL:
			loadDirectionalLights();
			break;

		case JU::LightManager::SPOTLIGHT:
			loadSpotlightLights();
	}
}



void GLSceneFaster::loadPositionalLights(void) const
{
    // WARNING: The shader expects the light position in eye coordinates
	glm::mat4 view_matrix = tp_camera_->getViewMatrix();

	JU::LightPositionalVector eye_lights;
	eye_lights.reserve(lights_positional_.size());
    JU::uint32 index = 0;
    for (JU::LightPositionalVector::const_iterator light = lights_positional_.begin(); light != lights_positional_.end(); ++light)
    {
    	glm::vec4 eye_position = view_matrix * glm::vec4(light->position_, 1.0f);
    	eye_lights.push_back(JU::LightPositional(glm::vec3(eye_position), light->intensity_));

        ++index;
    }

    JU::GLSLProgramExt::setUniform(current_program_iter_->second, eye_lights);

}



void GLSceneFaster::loadDirectionalLights(void) const
{
}



void GLSceneFaster::loadSpotlightLights(void) const
{
    // WARNING: The shader expects the light position in eye coordinates
	glm::mat4 view_matrix = tp_camera_->getViewMatrix();

	JU::LightSpotlightVector eye_lights;
	eye_lights.reserve(lights_spotlight_.size());
    JU::uint32 index = 0;
    for (JU::LightSpotlightVector::const_iterator light = lights_spotlight_.begin(); light != lights_spotlight_.end(); ++light)
    {
    	glm::vec4 eye_position = view_matrix * glm::vec4(light->position_, 1.0f);
    	glm::vec3 target_world_position = main_node_iter->second->getPosition();
    	glm::vec4 target_eye_position = view_matrix * glm::vec4(target_world_position, 1.0f);
    	glm::vec4 eye_direction (glm::normalize(eye_position - target_eye_position));
    	eye_lights.push_back(JU::LightSpotlight(glm::vec3(eye_position),
    											glm::vec3(eye_direction),
												light->intensity_,
												light->cutoff_));

        ++index;
    }

    JU::GLSLProgramExt::setUniform(current_program_iter_->second, eye_lights);

}



void GLSceneFaster::updateCamera(JU::uint32 time)
{
    JU::f32 radius_delta, angle;
    glm::vec3 axis;
    camera_controller_.update(radius_delta, angle, axis);

    // Use the arcball to control the camera or an object?
    if (control_camera_)
    {
        JU::Transform3D camera = static_cast<JU::Transform3D>(*main_node_iter->second);
        //camera.translate(glm::vec3(0.0f, 0.0f, 5.0f));
        // Convert the axis from the camera to the world coordinate system
        //axis = glm::vec3(tp_camera_->getTransformToParent() * glm::vec4(axis, 0.0f));
        //tp_camera_->update(static_cast<const JU::Transform3D&>(*main_node_iter->second), radius_delta, angle, axis);
        tp_camera_->update(camera);

    }
    else
    {
        axis = glm::vec3(tp_camera_->getTransformToParent() * glm::vec4(-axis, 0.0f));
        main_node_iter->second->rotate(angle, axis);
    }

}



void GLSceneFaster::updateLights(JU::uint32 time)
{
	switch (light_mode_)
	{
		case JU::LightManager::POSITIONAL:
			updatePositionalLights(time);
			break;

		case JU::LightManager::DIRECTIONAL:
			updateDirectionalLights(time);
			break;

		case JU::LightManager::SPOTLIGHT:
			updateSpotlightLights(time);
	}
}



void GLSceneFaster::updatePositionalLights(JU::uint32 time)
{
	// LIGHTS: update position
    static const JU::f32 angle_speed = (2.0 * M_PI * 0.25f) * 0.001f ; // 2000 milliseconds to complete a revolution

    glm::mat4 rotation = glm::rotate(glm::mat4(1.f), angle_speed * time, glm::vec3(0.0f, 1.0f, 0.0f));
    JU::uint32 num_lights = lights_positional_.size();

    if (num_lights)
    {
        JU::uint32 index = 0;
        // First light is the one mounted on the camera
        // Last light is positioned at the camera's position
        lights_positional_[index].position_ = tp_camera_->getPosition();
        node_map_[std::string("light_pos") + std::to_string(index)]->setPosition(lights_positional_[index].position_);

        index++;

        for (; index < num_lights; ++index)
        {
            glm::vec4 position = rotation * glm::vec4(lights_positional_[index].position_, 0.0f);
            lights_positional_[index].position_.x = position.x;
            lights_positional_[index].position_.y = position.y;
            lights_positional_[index].position_.z = position.z;

            node_map_[std::string("light_pos") + std::to_string(index)]->setPosition(lights_positional_[index].position_);
        }
    }
}


void GLSceneFaster::updateDirectionalLights(JU::uint32 time)
{
}



void GLSceneFaster::updateSpotlightLights(JU::uint32 time)
{
	// LIGHTS: update position
    //static const JU::f32 angle_speed = (2.0 * M_PI * 0.1f) * 0.001f ; // 10 seconds to complete a revolution
	static const JU::f32 angle_speed = 0.0f;

    glm::mat4 rotation = glm::rotate(glm::mat4(1.f), angle_speed * time, glm::vec3(0.0f, 1.0f, 0.0f));
    JU::uint32 index = 0;
    for (JU::LightSpotlightVector::iterator light = lights_spotlight_.begin(); light != lights_spotlight_.end(); ++light)
    {
        glm::vec4 position = rotation * glm::vec4(light->position_, 0.0f);
        light->position_.x = position.x;
        light->position_.y = position.y;
        light->position_.z = position.z;

        node_map_[std::string("light_pos") + std::to_string(index)]->setPosition(light->position_);

        ++index;
    }
}



/**
* @brief Update Bubble object
*
* @param time Time elapsed since the last update (in milliseconds)
*/
void GLSceneFaster::updateBubble(JU::uint32 time)
{
    // Update bubble's position
    static JU::Keyboard* pkeyboard = JU::Singleton<JU::Keyboard>::getInstance();
    if (pkeyboard->isKeyDown(SDL_SCANCODE_UP))
    {
        auto bubble = node_map_["bubble"];

        glm::vec3 forward(-bubble->getZAxis());
        bubble->translate(forward * 0.1f);
    }

    if (pkeyboard->isKeyDown(SDL_SCANCODE_LEFT))
    {
        auto bubble = node_map_["bubble"];

        bubble->rotateY(M_PI/180.0f);
    }

    if (pkeyboard->isKeyDown(SDL_SCANCODE_RIGHT))
    {
        auto bubble = node_map_["bubble"];

        bubble->rotateY(-M_PI/180.0f);
    }

#ifdef _DEBUG
    // ANTWEAKBAR
	pbubble_->setScale(Bubble::MAXI, tw_scale_maxi_);
	pbubble_->setScale(Bubble::MINI, tw_scale_mini_);
	pbubble_->update(tw_force_direction_, tw_force_strength_);
#endif
}



/**
* @brief Update everything that needs to be updated in the scene
*
* @param time Time elapsed since the last update (in milliseconds)
*/
void GLSceneFaster::update(JU::uint32 time)
{
	updateCamera(time);
	updateLights(time);
	updateBubble(time);
}



/**
* @brief Render the scene
*/
void GLSceneFaster::render(void)
{
	// WARNING: EVERY TIME you call glUseProgram​, glBindProgramPipeline​ or glUseProgramStages​,
	// all of the current subroutine state is completely lost. This state is never preserved,
	// so you should consider it ephemeral.
    //current_program_iter_->second.use();

    renderPass1();
	renderPass2();
#ifdef _DEBUG
	renderDebug();
    // AntTweakBar
    TwDraw();  // draw the tweak bar(s)
#endif
}



/**
* @brief Deferred Shading first pass
*
* Render the scene to the render targets
*/
void GLSceneFaster::renderPass1()
{
    gl::Viewport(0, 0, (GLsizei) scene_width_, (GLsizei) scene_height_);

    gl::BindFramebuffer(gl::FRAMEBUFFER, deferredFBO_);
    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(gl::COLOR_BUFFER_BIT | gl::DEPTH_BUFFER_BIT);
    gl::Enable(gl::DEPTH_TEST);

    current_program_iter_ = glsl_program_map_.find("deferred");
    current_program_iter_->second.use();

    // WARNING: Cannot change CONTEXT STATE after making this call
    // Explanation: The state for the selection of which subroutine functions to use
    // for which subroutine uniforms is not part of the program object. Instead, it is
    // part of the context state, similar to how texture bindings and uniform buffer
    // bindings are part of context state.
    gl::UniformSubroutinesuiv(gl::FRAGMENT_SHADER, 1, &pass1Index_);

    // Model Matrix
    static glm::mat4 M(1.0f);
    // View matrix
    glm::mat4 V(tp_camera_->getViewMatrix());
    // Perspective Matrix
    glm::mat4 P(tp_camera_->getPerspectiveMatrix());
    // Draw each object
    for (NodeMap::const_iterator iter = node_map_.begin(); iter != node_map_.end(); ++iter)
    {
        (iter->second)->draw(current_program_iter_->second, M, V, P);
    }

    JU::TextureManager::unbindAllTextures();

    // Wait for the buffer to be filled
    gl::Finish();
}



/**
* @brief Deferred Shading second pass
*
* Render a quad that fills the screen to shade each pixel with the visibility info from the first pass
*/
void GLSceneFaster::renderPass2()
{
    // Revert to default frame buffer
    gl::BindFramebuffer(gl::FRAMEBUFFER, 0);

    //pass1Index_ = glGetSubroutineIndex( current_program_iter_->second.getHandle(), gl::FRAGMENT_SHADER, "pass2");
    gl::UniformSubroutinesuiv(gl::FRAGMENT_SHADER, 1, &pass2Index_);

    gl::ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    gl::Clear(gl::COLOR_BUFFER_BIT | gl::DEPTH_BUFFER_BIT);
    gl::Disable(gl::DEPTH_TEST);

    // LOAD LIGHTS
    loadLights();

    // Load G-Buffer
    JU::TextureManager::bindTexture(current_program_iter_->second, "PositionTex", "PositionTex");
    JU::TextureManager::bindTexture(current_program_iter_->second, "NormalTex",   "NormalTex");
    JU::TextureManager::bindTexture(current_program_iter_->second, "ColorTex", 	  "ColorTex");
    JU::TextureManager::bindTexture(current_program_iter_->second, "ShininessTex","ShininessTex");
    if (record_depth_)
    	JU::TextureManager::bindTexture(current_program_iter_->second, "DepthTex", 	  "DepthTex");

    // Model Matrix
    glm::mat4 M(1.0f);
    // View matrix
    glm::mat4 V(1.0f);
    // Perspective Matrix
    glm::mat4 P(1.0f);

    mesh_instance_map_["screen_quad"]->draw(current_program_iter_->second, M, V, P);

    JU::TextureManager::unbindAllTextures();
}



#ifdef _DEBUG
/**
* @brief Debug rendering of textures
*
* Render the auxiliary textures for debug purposes. To be called after the last pass
* of Deferred Shading
*/
void GLSceneFaster::renderDebug()
{
    current_program_iter_ = glsl_program_map_.find("debug");
    current_program_iter_->second.use();

    // Model Matrix
    glm::mat4 M(1.0f);
    // View matrix
    glm::mat4 V(1.0f);
    // Perspective Matrix
    glm::mat4 P(1.0f);

    // Size of each mini-viewport
    JU::uint32 mini_width  = width_ - scene_width_;
    JU::uint32 mini_height = scene_height_ / 5;
    JU::uint32 height = 0;
    // Position
    gl::Viewport(scene_width_, height, (GLsizei) mini_width, (GLsizei) mini_height);
    JU::TextureManager::bindTexture(current_program_iter_->second, "PositionTex", "tex_image");
    mesh_instance_map_["screen_quad"]->draw(current_program_iter_->second, M, V, P);
    height += mini_height;

    // Normal
    gl::Viewport(scene_width_, height, (GLsizei) mini_width, (GLsizei) mini_height);
    JU::TextureManager::bindTexture(current_program_iter_->second, "NormalTex",   "tex_image");
    mesh_instance_map_["screen_quad"]->draw(current_program_iter_->second, M, V, P);
    height += mini_height;

    // Color
    gl::Viewport(scene_width_, height, (GLsizei) mini_width, (GLsizei) mini_height);
    JU::TextureManager::bindTexture(current_program_iter_->second, "ColorTex", 	  "tex_image");
    mesh_instance_map_["screen_quad"]->draw(current_program_iter_->second, M, V, P);
    height += mini_height;

    // Shininess
    gl::Viewport(scene_width_, height, (GLsizei) mini_width, (GLsizei) mini_height);
    JU::TextureManager::bindTexture(current_program_iter_->second, "ShininessTex","tex_image");
    mesh_instance_map_["screen_quad"]->draw(current_program_iter_->second, M, V, P);
    height += mini_height;

    // Depth
    if (record_depth_)
    {
        gl::Viewport(scene_width_, height, (GLsizei) mini_width, (GLsizei) mini_height);
        JU::TextureManager::bindTexture(current_program_iter_->second, "DepthTex", "tex_image");
    	mesh_instance_map_["screen_quad"]->draw(current_program_iter_->second, M, V, P);
    }

    JU::TextureManager::unbindAllTextures();
}
#endif


void GLSceneFaster::computeSceneSize(JU::uint32 width, JU::uint32 height)
{
    std::printf("%s --> %i, %i\n", __PRETTY_FUNCTION__, width, height);
    // Leave 20% of the canvas for displaying the auxiliary textures
    scene_width_  = width * 0.8f;
    scene_height_ = height;
}


/**
* @brief Handle the event to resize the scene
*
* @param width  Width of the window
* @param height Height of the window
*/
void GLSceneFaster::handleSDLEvent(const SDL_Event* event)
{

    //static bool handled = TwEventSDL(event, SDL_MAJOR_VERSION, SDL_MINOR_VERSION);
	bool handled = false;
    if (!handled)
    {
    	switch (event->type)
		{
			case SDL_WINDOWEVENT:
			    if (event->window.event == SDL_WINDOWEVENT_RESIZED)
				resize(event->window.data1, event->window.data2);
				break;

			case SDL_MOUSEMOTION:
				{
					//Get mouse position
					int x, y;
					SDL_GetMouseState (&x, &y);
					mouseMotion(x, y);
				}
				break;

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				{
					//Get mouse position
					int x, y;
					SDL_GetMouseState (&x, &y);
					mouseClick(JU::IO::SDL2ToMouseButtonID(event->button.button),
							   JU::IO::SDL2ToMouseButtonState(event->button.state),
							   x, y);
				}
				break;

			case SDL_MOUSEWHEEL:
				// Scrolling up
				if (event->wheel.y > 0)
					mouseClick(JU::IO::BUTTON_MIDDLE_SCROLL_UP, JU::IO::BUTTON_STATE_UNKNOWN, 0, 0);
				else if (event->wheel.y < 0)
					mouseClick(JU::IO::BUTTON_MIDDLE_SCROLL_DOWN, JU::IO::BUTTON_STATE_UNKNOWN, 0, 0);
				break;

			default:
				char buffer[100];
				std::sprintf(buffer, "%s:Event type (%x) has no handler assigned\n", __PRETTY_FUNCTION__, event->type);
				JU::SystemLog::logMessage(FUNCTION_NAME, buffer, false);
				break;


		}
	}
}


/**
* @brief Resize the scene
*
* @param width  Width of the window
* @param height Height of the window
*/
void GLSceneFaster::resize(int width, int height)
{
	// Actual viewport will be smaller to accommodate textures
	// in the right pane of the interface
	JU::GLScene::resize(width, height);
    computeSceneSize(width, height);
    gl::Viewport(0, 0, (GLsizei) scene_width_, (GLsizei) scene_height_);
    camera_->setAspectRatio(static_cast<JU::f32>(scene_width_)/scene_height_);
    camera_controller_.windowResize(scene_width_, scene_height_);

#ifdef _DEBUG
    // AntTweakBar
	TwWindowSize(width, height);
#endif
}



/**
* @brief Handle keyboard input
*
* @param key    Id of key pressed
* @param x      Location of the mouse when the key was pressed
* @param y      Location of the mouse when the key was pressed
*/
void GLSceneFaster::keyboard(unsigned char key, int x, int y)
{
    bool handled = false;

#ifdef _DEBUG
	handled = TwEventKeyboardGLUT(key, x, y);
#endif

    if (!handled)
    {
		switch (key)
		{
			// GLSL Program: change active program
			// -----------------------------------
			case '0':
				if(++current_program_iter_ == glsl_program_map_.end())
					current_program_iter_ = glsl_program_map_.begin();
				break;

			// ArcballController: change between controlling the camera and the active object
			// -----------------------------------
			case 'c':
			case 'C':
				control_camera_ = !control_camera_;
				break;

			// LIGHTS
			// -----------------------------------
			/*
			// Change light mode
			case 'm':
			case 'M':
				if (light_mode_ == LightManager::POSITIONAL)
					light_mode_ = LightManager::DIRECTIONAL;
				else if (light_mode_ == LightManager::DIRECTIONAL)
					light_mode_ = LightManager::SPOTLIGHT;
				else
					light_mode_ = LightManager::POSITIONAL;

				reload();
				break;

			// Increase number of lights
			case 'l':
				if (num_lights_ < MAX_POS_LIGHTS)
					++num_lights_;
				reload();
				break;

			// Decrease number of lights
			case 'L':
				if (num_lights_ > 0)
					--num_lights_;
				reload();
				break;
			*/

			// RELOAD
			// -----------------------------------
			case 'r':
			case 'R':
				reload();
				break;
		}
	}
}



void GLSceneFaster::mouseClick(int button, int state, int x, int y)
{
    bool handled = false;

#ifdef _DEBUG
	handled = TwEventMouseButtonGLUT(button, state, x, y);
#endif

	if (!handled)
	{
		camera_controller_.mouseClick(button, state, x, y);
	}
}



void GLSceneFaster::mouseMotion(int x, int y)
{
    bool handled = false;

#ifdef _DEBUG
	handled = TwEventMouseMotionGLUT(x, y);
#endif

	if (!handled)
	{
		camera_controller_.mouseMotion(x, y);
	}
}



void GLSceneFaster::reload()
{
	clear();
	init();
}



void GLSceneFaster::clear(void)
{
	// FBOs and Textures
	gl::DeleteFramebuffers (1, &deferredFBO_);
	if (record_depth_)
		gl::DeleteTextures(1, &depthBuf_);
	else
		gl::DeleteRenderbuffers(1, &depthBuf_);
	gl::DeleteTextures(1, &posTex_);
	gl::DeleteTextures(1, &normTex_);
	gl::DeleteTextures(1, &colorTex_);
	gl::DeleteTextures(1, &shininessTex_);

	// Camera
    delete tp_camera_;

    // GLSLProgram map
    glsl_program_map_.clear();

    // GLMesh map
    for (MeshMap::iterator iter = mesh_map_.begin(); iter != mesh_map_.end(); ++iter)
    {
    	delete iter->second;
    }
    mesh_map_.clear();

    // GLMeshInstance map
    for (MeshInstanceMap::iterator iter = mesh_instance_map_.begin(); iter != mesh_instance_map_.end(); ++iter)
    {
    	delete iter->second;
    }
    mesh_instance_map_.clear();

    // Node Map
    std::map<std::string, JU::Node3D *>::const_iterator iter;
    for (iter = node_map_.begin(); iter != node_map_.end(); ++iter)
    {
        delete iter->second;
    }
    node_map_.clear();

    // Lights
    lights_positional_.clear();
    lights_directional_.clear();
    lights_spotlight_.clear();

    // Bubble
    delete pbubble_;
    delete plandscape_;

#ifdef _DEBUG
    // AntTweakBar
    TwTerminate();
#endif

}


