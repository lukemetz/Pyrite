#include "utMath.h"
#include "Node.h"
#include "VoxelData.h"
#include <map>

namespace pyrite {
  class TreeGenerator {
  public:
    void generateTreeNodes(std::vector<Vec3f> attractors, Node* baseNode, float nodeLength, float killDistance);

    void generateElipticalChrown(std::vector<Vec3f> &att, int numPoints, Vec3f pos, Vec3f radius);

    void makeBranches(Node *baseNode, VoxelData * v, float tipRadius, float power);
  };
};
