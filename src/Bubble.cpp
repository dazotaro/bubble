/*
 * Bubble.cpp
 *
 *  Created on: May 15, 2015
 *      Author: jusabiaga
 */

// Global include
#include <graphics/Material.hpp>		// Material
#include <graphics/ShapeHelper2.hpp>    // build Mesh helper funtions
#include <graphics/TextureManager.hpp>  // JU::TextureManager

// Local include
#include "Bubble.hpp"					// Bubble


/**
* @brief Default Constructor
*
*/
Bubble::Bubble(void) : pmesh_(nullptr), pmesh_instance_maxi_(nullptr), pmesh_instance_mini_(nullptr),
						maxi_scale_(1.0f), mini_scale_(0.5f),
						force_dir_(glm::vec3(0.0f, 0.0f, 0.0f)), force_mag_(0.0f)
{
    // Mini's position in Maxi's coordinate system
	mini_obj_.setPosition(glm::vec3(0.0f, 0.0f, 1.0f));
}



/**
* @brief Destructor
*
*/
Bubble::~Bubble()
{
	delete pmesh_;
	delete pmesh_instance_maxi_;
	delete pmesh_instance_mini_;
}



/**
* @brief Initialize Bubble object
*
*/
void Bubble::init(void)
{
	initMeshes();
}



/**
* @brief Update Bubble
*
* @param direction 	Direction to displace mini respect to maxi
* @param distance	Distance of mini respect to maxi (needs to be scaled)
*
*/
void Bubble::update(const glm::vec3& direction, const JU::f32 distance)
{
	// INFO:
	// - At this point only mini's position (respect to maxi's) is updated.
	// - Mini's orientation is not considered

	// Maxi is at the origin (in it's own coordinate system)

	// If using AntTweakBard
	mini_obj_.setPosition(direction * distance);
}



/**
* @brief Initialize the meshes needed for this object
*
*/
void Bubble::setScale(BubbleMember member_id, JU::f32 scale)
{
    if (member_id == MAXI)
        pmesh_instance_maxi_->setScale(scale, scale, scale);
    else if (member_id == MINI)
        pmesh_instance_mini_->setScale(scale, scale, scale);
    else
        exit(EXIT_FAILURE);
}


/**
* @brief Initialize the meshes needed for this object
*
*/
JU::f32 Bubble::getScale (BubbleMember member_id) const
{
    JU::f32 scale;

    switch(member_id)
    {
        case MAXI:
            scale = maxi_scale_;
            break;

        case MINI:
            scale = mini_scale_;
            break;

        default:
            std::exit(EXIT_FAILURE);

    }

    return scale;
}


/**
* @brief Initialize the meshes needed for this object
*
*/
void Bubble::initMeshes(void)
{
    JU::TextureManager::loadTexture("pool",  "data/textures/pool.png");

	// SPHERE (to be used for maxi an mini)
	// ------
	// MESH
	JU::Mesh2 mesh;
	JU::ShapeHelper2::buildMesh(mesh, JU::ShapeHelper2::SPHERE, 64, 32);
	mesh.computeTangents();
	pmesh_ = new JU::GLMesh();
	// Load the Mesh into VBO and VAO
	pmesh_->init(mesh);

	// MESH INSTANCES
	// ------
	// maxi
	pmesh_instance_maxi_ = new JU::GLMeshInstance(pmesh_, maxi_scale_, maxi_scale_, maxi_scale_, JU::MaterialManager::getMaterial("ruby"));
	pmesh_instance_maxi_->addColorTexture("pool");

	// mini
	pmesh_instance_mini_ = new JU::GLMeshInstance(pmesh_, mini_scale_, mini_scale_, mini_scale_, JU::MaterialManager::getMaterial("pearl"));
	pmesh_instance_mini_->addColorTexture("pool");
}



/**
* @brief Draw function
*
* @param program	GLSLProgram to draw this object
* @param model		Model matrix to transform the position and orientation of this object to that of its parent's coordinate system
* @param view		View matrix
* @param projection	Camera's projection matrix
*
*/
void Bubble::draw(const JU::GLSLProgram &program, const glm::mat4 & model, const glm::mat4 &view, const glm::mat4 &projection) const
{
	// Draw "maxi"
	//glm::mat4 new_model = model * getTransformToParent();
    pmesh_instance_maxi_->draw(program, model, view, projection);

    // draw "mini"
    glm::mat4 new_model = model * mini_obj_.getTransformToParent();
    pmesh_instance_mini_->draw(program, new_model, view, projection);
}
