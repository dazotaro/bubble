/*
 * Bubble.h
 *
 *  Created on: May 15, 2015
 *      Author: jusabiaga
 */

#ifndef BUBBLE_HPP_
#define BUBBLE_HPP_

// Global includes
#include <core/Object3D.hpp>				// Object3D
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

	private:
        void initMeshes(void);

	private:
		JU::Object3D 		mini_obj_;				//!< Position and orientation of "mini" in "maxi's" coordinate system
		JU::GLMesh*			pmesh_;					//!< Mesh to be drawn
		JU::GLMeshInstance* pmesh_instance_maxi_;	//!< Mini's instance of *pmesh_
		JU::GLMeshInstance* pmesh_instance_mini_;	//!< Maxi's instance of *pmesh_
		JU::Material*		pmaterial_maxi_;		//!< Maxi's material
		JU::Material*		pmaterial_mini_;		//!< Mini's material
		JU::f32				maxi_scale_;			//!< Maxi's scale respect to the world
		JU::f32				mini_scale_;			//!< Mini's scale respect to the world
};

#endif /* BUBBLE_HPP_ */
