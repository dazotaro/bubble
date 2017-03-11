/*
 * Bubble.h
 *
 *  Created on: May 15, 2015
 *      Author: jusabiaga
 */

#ifndef BUBBLE_HPP_
#define BUBBLE_HPP_

// Global includes
#include <core/Transform3D.hpp>			// Transform3D
#include <graphics/DrawInterface.hpp>	// DrawInterface
#include <graphics/GLMesh.hpp>			// GLMesh
#include <graphics/GLMeshInstance.hpp>	// GLMeshInstance



/*!
  This class represents a bubble object, which as an initial implementation
  is a composite made out of two spheres
*/
class Bubble : public JU::DrawInterface
{
	public:
		enum BubbleMember
		{
			MAXI = 0,
			MINI,
		};

	public:
		Bubble(void);
		virtual ~Bubble();
		void init();

	//DrawInterface
	public:
        void draw(const JU::GLSLProgram &program, const glm::mat4 & model, const glm::mat4 &view, const glm::mat4 &projection) const;

	public:
        void update(const glm::vec3& direction, const JU::f32 distance);
        void setScale (BubbleMember member_id, JU::f32 scale);
        JU::f32 getScale (BubbleMember member_id) const;

	private:
        void initMeshes(void);

    private:
		JU::Transform3D 	mini_obj_;				//!< Position and orientation of "mini" in "maxi's" coordinate system
		JU::GLMesh*			pmesh_;					//!< Mesh to be drawn
		JU::GLMeshInstance* pmesh_instance_maxi_;	//!< Mini's instance of *pmesh_
		JU::GLMeshInstance* pmesh_instance_mini_;	//!< Maxi's instance of *pmesh_
		JU::f32				maxi_scale_;			//!< Maxi's scale respect to the world
		JU::f32				mini_scale_;			//!< Mini's scale respect to the world
		// Physics
		glm::vec3 			force_dir_;				//!< Direction of total force acting on Bubble
		JU::f32				force_mag_;				//!< Magnitude of force acting on Bubble
};

#endif /* BUBBLE_HPP_ */
