/*
 * Landscape.cpp
 *
 *  Created on: Mar 7, 2017
 *      Author: jusabiaga
 */

// Local includes
#include "Landscape.hpp"
// Global includes
#include <graphics/Material.hpp>        // Material
#include <graphics/ShapeHelper2.hpp>    // build Mesh helper funtions
#include <cstring>                      // std::memcpy
#include <glm/gtx/transform.hpp>        // scale

/**
* @brief Default Constructor
*
*/
Landscape::Landscape() : is_initialized_(false), pmesh_(nullptr), pmesh_instance_(nullptr),
                      pblock_(nullptr), num_rows_(0), num_cols_(0)
{
}



/**
* @brief Destructor
*
*/
Landscape::~Landscape()
{
    release();
}


/**
* @brief Release dynamically allocated data
*
*/
void Landscape::release()
{
    if (is_initialized_)
    {
        delete pmesh_;
        delete pmesh_instance_;
        delete [] pblock_;

        pmesh_ = nullptr;
        pmesh_instance_ = nullptr;
        pblock_ = nullptr;
        num_rows_ = 0;
        num_cols_ = 0;
    }
}



/**
* @brief Load the grid data (height, color, dimensions)
*
*/
void Landscape::init(const BlockInfo* pblock, const JU::uint32 num_rows, const JU::uint32 num_cols, const glm::vec3& scale)
{
    if (is_initialized_)
        release();

    // Initialize the Grid structure
    num_rows_ = num_rows;
    num_cols_ = num_cols;
    pblock_ = new BlockInfo[num_rows * num_cols];
    std::memcpy(pblock_, pblock, num_rows * num_cols * sizeof(pblock[0]));

    // Initialize OpenGL stuff
    JU::Mesh2 mesh;
    JU::ShapeHelper2::buildMesh(mesh, JU::ShapeHelper2::CUBE);
    pmesh_ = new JU::GLMesh();
    // Load the Mesh into VBO and VAO
    pmesh_->init(mesh);
    pmesh_instance_ = new JU::GLMeshInstance(pmesh_, 1.0f, 1.0f, 1.0f, JU::MaterialManager::getMaterial("yellow_rubber"));
    pmesh_instance_->addColorTexture("pool");

    scale_ = scale;

    is_initialized_ = true;
}


/**
* @brief Is the sphere colliding with the Grid
*
* @param position   Coordinates of the center of the sphere respect to the grid's origin
* @param radius     Radius of the sphere
* @return True if colliding, false otherwise
*/
bool Landscape::isCollidingWithSphere(glm::vec3 position, JU::f32 radius) const
{
    const JU::uint32 col = std::floor(position.x / scale_.x);
    const JU::uint32 row = -std::floor(position.z / scale_.y);

    std::printf("Position (%f, %f)\n", position.x, -position.z);
    std::printf("Row, column = %i, %i\n", row, col);

    const JU::int32 last_cell = num_rows_ * num_cols_ - 1;
    const JU::int32 center_cell = row * num_cols_ + col;        // Index of cell containing the center of the object

    const JU::f32 offsetX = - position.x - (col * scale_.x);
    const JU::f32 offsetY = - position.z - (row * scale_.y);

    // No collision if outside of the grid's bounds
    if (row >= 0 && row < num_rows_ && col >= 0 && col < num_cols_)
    {
        // Center cell
        if (pblock_[center_cell].height_)
        return true;

        // overlapping?
        bool north = false;
        bool south = false;
        bool east  = false;
        bool west  = false;

        // NORTH
        const JU::int32 north_cell = center_cell + num_cols_;
        if (north_cell <= last_cell && (scale_.y - offsetY) < radius)
        {
            north = true;
            if (pblock_[north_cell].height_)
                return true;
        }
        // EAST
        const JU::int32 east_cell = center_cell + 1;
        if (east_cell <= last_cell && (scale_.x - offsetX) < radius)
        {
            east = true;
            if (pblock_[east_cell].height_)
                return true;
        }
        // SOUTH
        const JU::int32 south_cell = center_cell - num_cols_;
        if (south_cell >= 0 && offsetY < radius)
        {
            south = true;
            if (pblock_[south_cell].height_)
                return true;
        }
        // WEST
        const JU::int32 west_cell = center_cell - 1;
        if (west_cell >= 0 && offsetX < radius)
        {
            west = true;
            if (pblock_[west_cell].height_)
                return true;
        }


        // NORTH-EAST
        if (north && east && pblock_[center_cell + num_cols_ + 1].height_)
            return true;
        // NORTH-WEST
        if (north && west && pblock_[center_cell + num_cols_ - 1].height_)
            return true;
        // SOUTH-EAST
        if (south && east && pblock_[center_cell - num_cols_ + 1].height_)
            return true;
        // SOUTH-WEST
        if (south && west && pblock_[center_cell - num_cols_ - 1].height_)
            return true;
    }

    return false;
}


/**
* @brief Draw function
*
* @param program    GLSLProgram to draw this object
* @param model      Model matrix to transform the position and orientation of this object to that of its parent's coordinate system
* @param view       View matrix
* @param projection Camera's projection matrix
*
*/
void Landscape::draw(const JU::GLSLProgram &program, const glm::mat4 & model, const glm::mat4 &view, const glm::mat4 &projection) const
{
    //const glm::mat4 scale_mat = glm::scale(glm::mat4(), scale_);

    for (JU::uint32 row = 0; row < num_rows_; row++)
    {
        for (JU::uint32 col = 0; col < num_cols_; col++)
        {
            JU::uint32 height = pblock_[row * num_cols_ + col].height_;
            if (height > 0)
            {
                // Translate this block (we only add half the height because the position of the object is at the center
                glm::mat4 trans_mat = glm::translate(glm::mat4(), glm::vec3(col, row, 0.5f));
                glm::mat4 block_scale = glm::scale(glm::mat4(), glm::vec3(scale_.x, scale_.y, height));
                pmesh_instance_->draw(program, model * block_scale * trans_mat, view, projection);
            }
        }
    }

}
