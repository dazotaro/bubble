/*
 * Landscape.hpp
 *
 *  Created on: Mar 7, 2017
 *      Author: jusabiaga
 */

#ifndef LANDSCAPE_HPP_
#define LANDSCAPE_HPP_

// Global includes
#include <core/Transform3D.hpp>         // Transform3D
#include <graphics/DrawInterface.hpp>   // DrawInterface
#include <graphics/GLMesh.hpp>          // GLMesh
#include <graphics/GLMeshInstance.hpp>  // GLMeshInstance



/*!
  This class represents a landscape object which is made out of blocks
*/
class Landscape : public JU::DrawInterface
{
    public:
        struct BlockInfo
        {
            JU::int32 height_;
        };

    public:
        Landscape();
        virtual ~Landscape();
        void init(const BlockInfo* pblock, const JU::uint32 num_rows, const JU::uint32 num_cols, const glm::vec3& scale);

    //DrawInterface
    public:
        void draw(const JU::GLSLProgram &program, const glm::mat4 & model, const glm::mat4 &view, const glm::mat4 &projection) const;

    private:
        void initMeshes();
        void release(void);

    private:
        bool                is_initialized_;   //!< Initialized?
        JU::GLMesh*         pmesh_;            //!< Mesh to be drawn
        JU::GLMeshInstance* pmesh_instance_;   //!< Mesh Instance
        // Lanscape of blocks
        BlockInfo* pblock_;     //!< Grid data
        JU::uint32 num_rows_;   //!< Number of rows
        JU::uint32 num_cols_;   //!< Number of columns
        glm::vec3  scale_;      //!< Scale parameters
};

#endif /* LANDSCAPE_HPP_ */
