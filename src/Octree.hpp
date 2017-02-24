/*
 * Octree.hpp
 *
 *  Created on: Feb 24, 2017
 *      Author: jusabiaga
 */

#ifndef OCTREE_HPP_
#define OCTREE_HPP_

// Forward Declarations
class aiMesh;

namespace JU
{

class Octree
{
    public:
        Octree(const aiMesh* mesh);
        virtual ~Octree();
};

} /* namespace JU */

#endif /* OCTREE_HPP_ */
