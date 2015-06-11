/*
 * GLScene.cpp
 *
 *  Created on: May 21, 2013
 *      Author: jusabiaga
 */

// Local includes
#include <JU/graphics/GLMesh.hpp>               // GLMesh
#include <JU/graphics/GLMeshInstance.hpp>       // GLMeshInstance
#include <JU/graphics/Node3D.hpp>               // Node3D
#include <JU/graphics/CameraInterface.hpp>      // CameraInterface
#include <JU/graphics/CameraFirstPerson.hpp>    // CameraFirstPerson
#include <JU/graphics/CameraThirdPerson.hpp>    // CameraThirdPerson
#include <JU/graphics/ShapeHelper2.hpp>         // build Mesh helper funtions
#include <JU/graphics/TextureManager.hpp>       // loadTexture()
#include <JU/graphics/Material.hpp>             // MaterialManager
#include <JU/graphics/DebugGlm.hpp>             // debug::print
#include <JU/graphics/GLSLProgramExt.hpp>		// GLSLProgramExt::setUniform

// Global includes
#include "../core/Object3D.hpp"     // Object3D
#include <glm/gtx/transform.hpp>	// glm::rotate
#include <math.h>					// M_PI
#include "GLSceneFaster.hpp"      // GLSceneFaster


GLSceneFaster::GLSceneFaster(int width, int height) : GLScene(width, height),
									 scene_width_(0), scene_height_(0),
									 deferredFBO_(0), depthBuf_(0), posTex_(0), normTex_(0), colorTex_(0), shininessTex_(0),
									 pass1Index_(0), pass2Index_(0), record_depth_(true),
									 tp_camera_(nullptr), camera_(nullptr), control_camera_(true), camera_controller_(width, height, 0.2f),
									 light_mode_(LightManager::POSITIONAL), num_lights_(1),
									 ptw_bar_(nullptr), tw_force_direction_(glm::vec3(1.0f, 0.0f, 0.0f)), tw_force_strength_(5.0f),
									 tw_scale_maxi_(5.0f), tw_scale_mini_(2.5f)
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
    glClearColor(0.0,0.0,0.0,1.0);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDepthRange(0.0f, 1.0f);
    glEnable(GL_CULL_FACE); // enables face culling
    glCullFace(GL_BACK); // tells OpenGL to cull back faces (the sane default setting)

    initializePrograms();
    initializeFBO();
    initializeMaterials();
    initializeTextures();
    initializeObjects();
    initializeCameras();
    initializeLights();

    initAntTweakBar();
}



void GLSceneFaster::initializePrograms()
{
	glsl_program_map_["debug"]  = compileAndLinkShader("shaders/deferred.vs", "shaders/image_texture.fs");

	if (record_depth_)
	{
		glsl_program_map_["deferred"]  = compileAndLinkShader("shaders/deferred.vs", "shaders/deferred_depth.fs");
	}
	else
	{
		glsl_program_map_["deferred"]  = compileAndLinkShader("shaders/deferred.vs", "shaders/deferred.fs");
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
    pass1Index_ = glGetSubroutineIndex( programHandle, GL_FRAGMENT_SHADER, "pass1");
    pass2Index_ = glGetSubroutineIndex( programHandle, GL_FRAGMENT_SHADER, "pass2");
}



/**
* @brief Initialize RenderBuffers and Textures needed by Deferred Shading's first pass
*/
void GLSceneFaster::initializeFBO()
{
    // Create and bind the FBO
    glGenFramebuffers(1, &deferredFBO_);
    glBindFramebuffer(GL_FRAMEBUFFER, deferredFBO_);

    // The DEPTH buffer
    if (record_depth_)
    {
        // if want to access it in the shader
        glGenTextures(1, &depthBuf_);
        glBindTexture(GL_TEXTURE_2D, depthBuf_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, scene_width_, scene_height_, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthBuf_, 0);
        TextureManager::registerTexture("DepthTex", depthBuf_);
    }
    else
    {
    	// if we do not need to explicitly access it
		glGenRenderbuffers(1, &depthBuf_);
		glBindRenderbuffer(GL_RENDERBUFFER, depthBuf_);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, scene_width_, scene_height_);
	    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuf_);
    }

    // The position buffer
    glGenTextures(1, &posTex_);
    glBindTexture(GL_TEXTURE_2D, posTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, scene_width_, scene_height_, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // The normal buffer
    glGenTextures(1, &normTex_);
    glBindTexture(GL_TEXTURE_2D, normTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, scene_width_, scene_height_, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // The color buffer
    glGenTextures(1, &colorTex_);
    glBindTexture(GL_TEXTURE_2D, colorTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, scene_width_, scene_height_, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // The shininess buffer
    glGenTextures(1, &shininessTex_);
    glBindTexture(GL_TEXTURE_2D, shininessTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, scene_width_, scene_height_, 0,  GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Attach the images to the frame buffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, posTex_, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normTex_, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, colorTex_, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, shininessTex_, 0);

    GLenum drawBuffers[] = {GL_NONE,
    						GL_COLOR_ATTACHMENT0,
							GL_COLOR_ATTACHMENT1,
							GL_COLOR_ATTACHMENT2,
							GL_COLOR_ATTACHMENT3};
    glDrawBuffers(5, drawBuffers);

    // Check that our frame buffer is complete
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
    	std::printf("Incomplete FrameBuffer Status. Bailing out!!!");
    	exit(EXIT_FAILURE);
    }

    // Register the texture handles with TextureManager
    TextureManager::registerTexture("PositionTex", 	posTex_);
    TextureManager::registerTexture("NormalTex", 	normTex_);
    TextureManager::registerTexture("ColorTex", 	colorTex_);
    TextureManager::registerTexture("ShininessTex", shininessTex_);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}



void GLSceneFaster::initializeMaterials()
{
	Material* pmat;

    MaterialManager::init();

    pmat = new Material;
    if (!MaterialManager::getMaterial("ruby", *pmat))
        exit(EXIT_FAILURE);
    material_map_["ruby"] = pmat;

    pmat = new Material;
    if (!MaterialManager::getMaterial("gray_rubber", *pmat))
        exit(EXIT_FAILURE);
    material_map_["gray_rubber"] = pmat;

    pmat = new Material;
    if (!MaterialManager::getMaterial("pearl", *pmat))
        exit(EXIT_FAILURE);
    material_map_["pearl"] = pmat;

    pmat = new Material(1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
    material_map_["white_light"] = pmat;
}



void GLSceneFaster::initializeTextures()
{
    TextureManager::loadTexture("test",  "texture/test.tga");
    TextureManager::loadTexture("brick", "texture/brick1.jpg");
    TextureManager::loadTexture("pool",  "texture/pool.png");
    TextureManager::loadTexture("light", "texture/light_texture.tga");
}



void GLSceneFaster::initializeObjects()
{
	GLMesh* 		pmesh;
	GLMeshInstance* pmesh_instance;
	Node3D*			pnode;
	Mesh2 mesh;

	// BUBBLE
	// ------
	pbubble_ = new Bubble();
	pbubble_->init();
	// NODE: give the sphere a position and a orientation
	Object3D bubble3d(glm::vec3(0.0f, 10.0f,  0.0f), // Model's position
				      glm::vec3(1.0f,  0.0f,  0.0f), // Model's X axis
				      glm::vec3(0.0f,  0.0f, -1.0f), // Model's Y axis
				      glm::vec3(0.0f,  1.0f,  0.0f));// Model's Z axis
	pnode = new Node3D(bubble3d, pbubble_, true);
	node_map_["bubble"] = pnode;

	main_node_iter = node_map_.find("bubble");

	// SPHERE (to be used by lights)
	// ------
	// MESH
	ShapeHelper2::buildMesh(mesh, ShapeHelper2::SPHERE, 64, 32);
	mesh.computeTangents();
	pmesh = new GLMesh(mesh);
	// Load the Mesh into VBO and VAO
	pmesh->init();
	mesh_map_["sphere_64_32"] = pmesh;

	/*
	// TORUS
	// ------
	// MESH
	ShapeHelper2::buildMesh(mesh, ShapeHelper2::TORUS, 64, 32);
	mesh.computeTangents();
	pmesh = new GLMesh(mesh);
	// Load the Mesh into VBO and VAO
	pmesh->init();
	mesh_map_["torus_64_32"] = pmesh;
	*/

	// MESH INSTANCE
	/*
	pmesh_instance = new GLMeshInstance(pmesh, 5.0f, 5.0f, 5.0f, material_map_["ruby"]);
	pmesh_instance->addColorTexture("pool");
	mesh_instance_map_["sphere_ruby"] = pmesh_instance;
	// NODE
	// Give the sphere a position and a orientation
	Object3D sphere(glm::vec3(0.0f, 10.0f,  0.0f), // Model's position
				   glm::vec3(1.0f,  0.0f,  0.0f), // Model's X axis
				   glm::vec3(0.0f,  0.0f, -1.0f), // Model's Y axis
				   glm::vec3(0.0f,  1.0f,  0.0f));// Model's Z axis
	pnode = new Node3D(sphere, pmesh_instance, true);
	node_map_["sphere"] = pnode;

	main_node_iter = node_map_.find("sphere");
	*/

    // PLANE
    // ------
    // MESH
    ShapeHelper2::buildMesh(mesh, ShapeHelper2::PLANE);
    pmesh = new GLMesh(mesh);
    // Load the Mesh into VBO and VAO
    pmesh->init();
    mesh_map_["plane"] = pmesh;
    // MESH INSTANCE
    pmesh_instance = new GLMeshInstance(pmesh, 50.0f, 50.0f, 1.0f, material_map_["gray_rubber"]);
    pmesh_instance->addColorTexture("brick");
    mesh_instance_map_["plane_green"];
    // NODE
    // Give the plane a position and a orientation
    Object3D plane(glm::vec3(0.0f, 0.0f, 0.0f), // Model's position
                   glm::vec3(1.0f, 0.0f, 0.0f), // Model's X axis
                   glm::vec3(0.0f, 0.0f,-1.0f), // Model's Y axis
                   glm::vec3(0.0f, 1.0f, 0.0f));// Model's Z axis
    pnode = new Node3D(plane, pmesh_instance, true);
	node_map_["plane"] = pnode;

	// QUAD: For deferred shading's second pass (screen filling quad)
	// --------------------------------------------------------------
	pmesh_instance = new GLMeshInstance(mesh_map_["plane"], 2.0f, 2.0f, 1.0f);
    mesh_instance_map_["screen_quad"] = pmesh_instance;
}



void GLSceneFaster::initializeCameras()
{
    tp_camera_ = new CameraThirdPerson(CameraIntrinsic(90.f, scene_width_/(JU::f32)scene_height_, 0.5f, 1000.f),
    								   static_cast<Object3D>(*main_node_iter->second),
    								   10.0f, 0.0f, M_PI / 4.0f);
    camera_ = dynamic_cast<CameraInterface *>(tp_camera_);
}




void GLSceneFaster::initializeLights()
{
	switch (light_mode_)
	{
		case LightManager::POSITIONAL:
			initializePositionalLights();
			break;

		case LightManager::DIRECTIONAL:
			initializeDirectionalLights();
			break;

		case LightManager::SPOTLIGHT:
			initializeSpotlightLights();
	}
}



void GLSceneFaster::initializePositionalLights()
{
	JU::f32 radius = 10.0f;
	JU::f32 channel_intensity = 1.0f / num_lights_;
    glm::vec3 light_intensity (channel_intensity);

    // GLMeshInstance
    GLMeshInstance* pmesh_instance = new GLMeshInstance(mesh_map_["sphere_64_32"],		// mesh
    													0.5f, 0.5f, 0.5f,				// scale
														material_map_["white_light"]);	// material
    //pmesh_instance->addColorTexture("light");
    mesh_instance_map_["light_sphere"] = pmesh_instance;

    lights_positional_.resize(num_lights_, LightPositional(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f)));

    glm::vec3 light_ring_center (0.0f, 20.0f, 0.0f);
	JU::f32 angle_delta = 2.0f * M_PI / num_lights_;
    for (JU::uint8 index = 0; index < num_lights_; ++index)
    {
    	JU::f32 angle = index * angle_delta;
    	JU::f32 z = radius * cosf(angle);
    	JU::f32 x = radius * sinf(angle);
    	glm::vec3 light_pos (x, 20.f, z);
        Object3D light_frame(light_pos,
                           glm::vec3(1.0f, 0.0f,  0.0f), // Model's X axis
    					   glm::vec3(0.0f, 1.0f,  0.0f), // Model's Y axis
    					   glm::vec3(0.0f, 0.0f,  1.0f));// Model's Z axis

        Node3D *pnode = new Node3D(light_frame, pmesh_instance, true);

        node_map_[std::string("light_pos") + std::to_string(index)] = pnode;

        lights_positional_[index] = (LightPositional(light_pos, light_intensity));
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
    GLMeshInstance* pmesh_instance = new GLMeshInstance(mesh_map_["sphere_64_32"],		// mesh
    													0.5f, 0.5f, 0.5f,				// scale
														material_map_["white_light"]);	// material
    //pmesh_instance->addColorTexture("light");
    mesh_instance_map_["light_sphere"] = pmesh_instance;

    lights_spotlight_.resize(num_lights_, LightSpotlight(glm::vec3(0.0f, 0.0f, 0.0f),
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
        Object3D light_frame(light_pos,
                           glm::vec3(1.0f, 0.0f,  0.0f), // Model's X axis
    					   glm::vec3(0.0f, 1.0f,  0.0f), // Model's Y axis
    					   glm::vec3(0.0f, 0.0f,  1.0f));// Model's Z axis
        glm::vec3 light_dir = glm::normalize(light_pos - main_node_iter->second->getPosition());

        Node3D *pnode = new Node3D(light_frame, pmesh_instance, true);

        node_map_[std::string("light_pos") + std::to_string(index)] = pnode;

        lights_spotlight_[index] = (LightSpotlight(light_pos, light_dir, light_intensity, cutoff));
    }
}



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



void GLSceneFaster::loadLights(void) const
{
	switch (light_mode_)
	{
		case LightManager::POSITIONAL:
			loadPositionalLights();
			break;

		case LightManager::DIRECTIONAL:
			loadDirectionalLights();
			break;

		case LightManager::SPOTLIGHT:
			loadSpotlightLights();
	}
}



void GLSceneFaster::loadPositionalLights(void) const
{
    // WARNING: The shader expects the light position in eye coordinates
	glm::mat4 view_matrix = tp_camera_->getViewMatrix();

	LightPositionalVector eye_lights;
	eye_lights.reserve(lights_positional_.size());
    JU::uint32 index = 0;
    for (LightPositionalVector::const_iterator light = lights_positional_.begin(); light != lights_positional_.end(); ++light)
    {
    	glm::vec4 eye_position = view_matrix * glm::vec4(light->position_, 1.0f);
    	eye_lights.push_back(LightPositional(glm::vec3(eye_position), light->intensity_));

        ++index;
    }

    GLSLProgramExt::setUniform(current_program_iter_->second, eye_lights);

}



void GLSceneFaster::loadDirectionalLights(void) const
{
}



void GLSceneFaster::loadSpotlightLights(void) const
{
    // WARNING: The shader expects the light position in eye coordinates
	glm::mat4 view_matrix = tp_camera_->getViewMatrix();

	LightSpotlightVector eye_lights;
	eye_lights.reserve(lights_spotlight_.size());
    JU::uint32 index = 0;
    for (LightSpotlightVector::const_iterator light = lights_spotlight_.begin(); light != lights_spotlight_.end(); ++light)
    {
    	glm::vec4 eye_position = view_matrix * glm::vec4(light->position_, 1.0f);
    	glm::vec3 target_world_position = main_node_iter->second->getPosition();
    	glm::vec4 target_eye_position = view_matrix * glm::vec4(target_world_position, 1.0f);
    	glm::vec4 eye_direction (glm::normalize(eye_position - target_eye_position));
    	eye_lights.push_back(LightSpotlight(glm::vec3(eye_position),
    										glm::vec3(eye_direction),
											light->intensity_,
											light->cutoff_));

        ++index;
    }

    GLSLProgramExt::setUniform(current_program_iter_->second, eye_lights);

}



void GLSceneFaster::updateCamera(JU::f32 time)
{
    JU::f32 radius_delta, angle;
    glm::vec3 axis;
    camera_controller_.update(radius_delta, angle, axis);

    // Use the arcball to control the camera or an object?
    if (control_camera_)
    {
        // Convert the axis from the camera to the world coordinate system
        axis = glm::vec3(tp_camera_->getTransformToParent() * glm::vec4(axis, 0.0f));
        tp_camera_->update(static_cast<const Object3D&>(*main_node_iter->second), radius_delta, angle, axis);
    }
    else
    {
        axis = glm::vec3(tp_camera_->getTransformToParent() * glm::vec4(-axis, 0.0f));
        main_node_iter->second->rotate(angle, axis);
    }

}



void GLSceneFaster::updateLights(JU::f32 time)
{
	switch (light_mode_)
	{
		case LightManager::POSITIONAL:
			updatePositionalLights(time);
			break;

		case LightManager::DIRECTIONAL:
			updateDirectionalLights(time);
			break;

		case LightManager::SPOTLIGHT:
			updateSpotlightLights(time);
	}
}



void GLSceneFaster::updatePositionalLights(JU::f32 time)
{
	// LIGHTS: update position
    //static const JU::f32 angle_speed = (2.0 * M_PI * 0.1f) * 0.001f ; // 10 seconds to complete a revolution
	static const JU::f32 angle_speed = 0.0f;

    glm::mat4 rotation = glm::rotate(glm::mat4(1.f), angle_speed * time, glm::vec3(0.0f, 1.0f, 0.0f));
    JU::uint32 index = 0;
    for (LightPositionalVector::iterator light = lights_positional_.begin(); light != lights_positional_.end(); ++light)
    {
        glm::vec4 position = rotation * glm::vec4(light->position_, 0.0f);
        light->position_.x = position.x;
        light->position_.y = position.y;
        light->position_.z = position.z;

        node_map_[std::string("light_pos") + std::to_string(index)]->setPosition(light->position_);

        ++index;
    }
}



void GLSceneFaster::updateDirectionalLights(JU::f32 time)
{
}



void GLSceneFaster::updateSpotlightLights(JU::f32 time)
{
	// LIGHTS: update position
    //static const JU::f32 angle_speed = (2.0 * M_PI * 0.1f) * 0.001f ; // 10 seconds to complete a revolution
	static const JU::f32 angle_speed = 0.0f;

    glm::mat4 rotation = glm::rotate(glm::mat4(1.f), angle_speed * time, glm::vec3(0.0f, 1.0f, 0.0f));
    JU::uint32 index = 0;
    for (LightSpotlightVector::iterator light = lights_spotlight_.begin(); light != lights_spotlight_.end(); ++light)
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
void GLSceneFaster::updateBubble(JU::f32 time)
{
	pbubble_->setScale(Bubble::MAXI, tw_scale_maxi_);
	pbubble_->setScale(Bubble::MINI, tw_scale_mini_);
	pbubble_->update(tw_force_direction_, tw_force_strength_);
}



/**
* @brief Update everything that needs to be updated in the scene
*
* @param time Time elapsed since the last update (in milliseconds)
*/
void GLSceneFaster::update(JU::f32 time)
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
	renderDebug();

	// AntTweakBar
	TwDraw();  // draw the tweak bar(s)
}



/**
* @brief Deferred Shading first pass
*
* Render the scene to the render targets
*/
void GLSceneFaster::renderPass1()
{
    glViewport(0, 0, (GLsizei) scene_width_, (GLsizei) scene_height_);

    glBindFramebuffer(GL_FRAMEBUFFER, deferredFBO_);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    current_program_iter_ = glsl_program_map_.find("deferred");
    current_program_iter_->second.use();

    // WARNING: Cannot change CONTEXT STATE after making this call
    // Explanation: The state for the selection of which subroutine functions to use
    // for which subroutine uniforms is not part of the program object. Instead, it is
    // part of the context state, similar to how texture bindings and uniform buffer
    // bindings are part of context state.
    glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &pass1Index_);

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

    TextureManager::unbindAllTextures();

    // Wait for the buffer to be filled
    glFinish();
}



/**
* @brief Deferred Shading second pass
*
* Render a quad that fills the screen to shade each pixel with the visibility info from the first pass
*/
void GLSceneFaster::renderPass2()
{
    // Revert to default frame buffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //pass1Index_ = glGetSubroutineIndex( current_program_iter_->second.getHandle(), GL_FRAGMENT_SHADER, "pass2");
    glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &pass2Index_);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    // LOAD LIGHTS
    loadLights();

    // Load G-Buffer
    TextureManager::bindTexture(current_program_iter_->second, "PositionTex", "PositionTex");
    TextureManager::bindTexture(current_program_iter_->second, "NormalTex",   "NormalTex");
    TextureManager::bindTexture(current_program_iter_->second, "ColorTex", 	  "ColorTex");
    TextureManager::bindTexture(current_program_iter_->second, "ShininessTex","ShininessTex");
    if (record_depth_)
    	TextureManager::bindTexture(current_program_iter_->second, "DepthTex", 	  "DepthTex");

    // Model Matrix
    glm::mat4 M(1.0f);
    // View matrix
    glm::mat4 V(1.0f);
    // Perspective Matrix
    glm::mat4 P(1.0f);

    mesh_instance_map_["screen_quad"]->draw(current_program_iter_->second, M, V, P);

    TextureManager::unbindAllTextures();
}



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
    glViewport(scene_width_, height, (GLsizei) mini_width, (GLsizei) mini_height);
    TextureManager::bindTexture(current_program_iter_->second, "PositionTex", "tex_image");
    mesh_instance_map_["screen_quad"]->draw(current_program_iter_->second, M, V, P);
    height += mini_height;

    // Normal
    glViewport(scene_width_, height, (GLsizei) mini_width, (GLsizei) mini_height);
    TextureManager::bindTexture(current_program_iter_->second, "NormalTex",   "tex_image");
    mesh_instance_map_["screen_quad"]->draw(current_program_iter_->second, M, V, P);
    height += mini_height;

    // Color
    glViewport(scene_width_, height, (GLsizei) mini_width, (GLsizei) mini_height);
    TextureManager::bindTexture(current_program_iter_->second, "ColorTex", 	  "tex_image");
    mesh_instance_map_["screen_quad"]->draw(current_program_iter_->second, M, V, P);
    height += mini_height;

    // Shininess
    glViewport(scene_width_, height, (GLsizei) mini_width, (GLsizei) mini_height);
    TextureManager::bindTexture(current_program_iter_->second, "ShininessTex","tex_image");
    mesh_instance_map_["screen_quad"]->draw(current_program_iter_->second, M, V, P);
    height += mini_height;

    // Depth
    if (record_depth_)
    {
        glViewport(scene_width_, height, (GLsizei) mini_width, (GLsizei) mini_height);
    	TextureManager::bindTexture(current_program_iter_->second, "DepthTex", "tex_image");
    	mesh_instance_map_["screen_quad"]->draw(current_program_iter_->second, M, V, P);
    }


    TextureManager::unbindAllTextures();
}



void GLSceneFaster::computeSceneSize(JU::uint32 width, JU::uint32 height)
{
    // Leave 20% of the canvas for displaying the auxiliary textures
    scene_width_  = width * 0.8f;
    scene_height_ = height;
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
    GLScene::resize(width, height);
    computeSceneSize(width, height);
    glViewport(0, 0, (GLsizei) scene_width_, (GLsizei) scene_height_);
    camera_->setAspectRatio(static_cast<JU::f32>(scene_width_)/scene_height_);
    camera_controller_.windowResize(scene_width_, scene_height_);

    // AntTweakBar
	TwWindowSize(width, height);
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
	if (!TwEventKeyboardGLUT(key, x, y))
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
	if (!TwEventMouseButtonGLUT(button, state, x, y))
	{
		camera_controller_.mouseClick(button, state, x, y);
	}
}



void GLSceneFaster::mouseMotion(int x, int y)
{
	if (!TwEventMouseMotionGLUT(x, y))
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
	glDeleteFramebuffers (1, &deferredFBO_);
	if (record_depth_)
		glDeleteTextures(1, &depthBuf_);
	else
		glDeleteRenderbuffers(1, &depthBuf_);
	glDeleteTextures     (1, &posTex_);
	glDeleteTextures	 (1, &normTex_);
	glDeleteTextures	 (1, &colorTex_);
	glDeleteTextures	 (1, &shininessTex_);

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

    // Material map
    for (MaterialMap::const_iterator iter = material_map_.begin(); iter != material_map_.end(); ++iter)
    {
    	delete iter->second;
    }
    material_map_.clear();

    // Node Map
    std::map<std::string, Node3D *>::const_iterator iter;
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

    // AntTweakBar
    TwTerminate();
}


