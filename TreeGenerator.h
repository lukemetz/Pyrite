#include "utMath.h"
#include "Node.h"
#include "VoxelData.h"
#include <map>

namespace pyrite {
  class TreeGenerator {
  public:
    void generateTreeNodes(std::vector<Horde3D::Vec3f> attractors, Node* baseNode, float nodeLength, float killDistance);

    void generateElipticalChrown(std::vector<Horde3D::Vec3f> &att, int numPoints, Horde3D::Vec3f pos, Horde3D::Vec3f radius);

    void makeBranches(Node *baseNode, VoxelData * v, float tipRadius, float power);
  };
};
