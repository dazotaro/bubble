/*
 * Landscape.cpp
 *
 *  Created on: Mar 7, 2017
 *      Author: jusabiaga
 */

// Local includes
#include "Landscape.hpp"
// Global includes
#include <graphics/Material.hpp>            // Material
#include <graphics/ShapeHelper2.hpp>        // build Mesh helper funtions
#include <collision/BoundingVolumes.hpp>    // BoundingSphere
#include <collision/Intersection.hpp>       // testSphereBox
#include <cstring>                          // std::memcpy
#include <glm/gtx/transform.hpp>            // scale
#include <cfloat>                           // numeric limits
#ifdef _DEBUG
    #include <graphics/DebugGlm.hpp>            // printVec3
#endif

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

    scale_ = scale;

    // Initialize the Grid structure
    num_rows_ = num_rows;
    num_cols_ = num_cols;
    // Allocate storage for the landscape
    pblock_ = new BlockInfo[num_rows * num_cols];
    glm::f32 min_height(FLT_MAX), max_height(FLT_MIN);
    for (glm::uint32 row = 0; row < num_rows; row++)
    {
        for (glm::uint32 col = 0; col < num_cols; col++)
        {
            glm::uint32 index = row * num_cols + col;
            pblock_[index] = pblock[index];
            if (pblock_[index].height_ < min_height)
                min_height = pblock_[index].height_;
            if (pblock_[index].height_ > max_height)
                max_height = pblock_[index].height_;
        }
    }

    // Init bounding box
    box_.pmin_.x = 0.0f;
    box_.pmin_.y = 0.0f;
    box_.pmin_.z = min_height;
    box_.pmax_.x = num_cols_ * scale_.x;
    box_.pmax_.y = num_rows_ * scale_.y;
    box_.pmax_.z = max_height;


    // Initialize OpenGL stuff
    JU::Mesh2 mesh;
    JU::ShapeHelper2::buildMesh(mesh, JU::ShapeHelper2::CUBE);
    pmesh_ = new JU::GLMesh();
    // Load the Mesh into VBO and VAO
    pmesh_->init(mesh);
    pmesh_instance_ = new JU::GLMeshInstance(pmesh_, 1.0f, 1.0f, 1.0f, JU::MaterialManager::getMaterial("yellow_rubber"));
    pmesh_instance_->addColorTexture("pool");

     is_initialized_ = true;
}


/**
* @brief Is the sphere colliding with the Grid
*
* @param sphere Bounding sphere to test against
*               (center of the sphere is in the Landscape's local coordinate system)
*
* @return True if colliding, false otherwise
*
* \todo This is the dumb brute-force (check all cells in the grid) approach. Upgrade it
* so it uses a spatial data structure (uniform/hierarchical grids, octree)
*/
bool Landscape::isColliding(const JU::BoundingSphere& sphere) const
{
    if (!JU::testSphereBox(sphere, box_))
        return false;

    for (glm::uint32 row = 0; row < num_rows_; row++)
    {
        for (glm::uint32 col = 0; col < num_cols_; col++)
        {
            if (pblock_[row*num_cols_+col].height_)
            {
                glm::vec3 pmin(      col * scale_.x,       row * scale_.y,                               0.0f);
                glm::vec3 pmax((col + 1) * scale_.x, (row + 1) * scale_.y, pblock_[row*num_cols_+col].height_);

                JU::BoundingBox block(glm::vec3(      col * scale_.x,       row * scale_.y,                               0.0f),
                                      glm::vec3((col + 1) * scale_.x, (row + 1) * scale_.y, pblock_[row*num_cols_+col].height_));
                if (JU::testSphereBox(sphere, block))
                    return true;
            }
        }
    }

    return false;
}


/*
 * \todo Improve the brute-force solution
 * bool Landscape::isColliding(const JU::BoundingSphere& sphere) const
{
    std::printf("Position (%f, %f)\n", sphere.center_.x, sphere.center_.z);

    if (!JU::testSphereBox(sphere, box_))
    {
        std::printf("Early out!\n");
        return false;
    }

    const JU::uint32 col =  std::floor(sphere.center_.x / scale_.x);
    const JU::uint32 row =  std::floor(sphere.center_.z / scale_.y);
    const JU::uint32 row2 = -sphere.center_.z / scale_.y;

    std::printf("Row, column = %i, %i (row2 = %i)\n", row, col, row2);

    const JU::int32 last_cell = num_rows_ * num_cols_ - 1;
    const JU::int32 center_cell = row * num_cols_ + col;        // Index of cell containing the center of the object

    const JU::f32 offsetX = - sphere.center_.x - (col * scale_.x);
    const JU::f32 offsetY = - sphere.center_.z - (row * scale_.y);

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
    if (north_cell <= last_cell && (scale_.y - offsetY) < sphere.radius_)
    {
        north = true;
        if (pblock_[north_cell].height_)
            return true;
    }
    // EAST
    const JU::int32 east_cell = center_cell + 1;
    if (east_cell <= last_cell && (scale_.x - offsetX) < sphere.radius_)
    {
        east = true;
        if (pblock_[east_cell].height_)
            return true;
    }
    // SOUTH
    const JU::int32 south_cell = center_cell - num_cols_;
    if (south_cell >= 0 && offsetY < sphere.radius_)
    {
        south = true;
        if (pblock_[south_cell].height_)
            return true;
    }
    // WEST
    const JU::int32 west_cell = center_cell - 1;
    if (west_cell >= 0 && offsetX < sphere.radius_)
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

    return false;
}
*/

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
                // Translate this block (we only add half the height because the position of the object is at the center)
                glm::mat4 trans_mat = glm::translate(glm::mat4(), glm::vec3(col + 0.5f, row + 0.5f, 0.5f));
                glm::mat4 block_scale = glm::scale(glm::mat4(), glm::vec3(scale_.x, scale_.y, height));
                pmesh_instance_->draw(program, model * block_scale * trans_mat, view, projection);
            }
        }
    }

}
