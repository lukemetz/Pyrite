#pragma once
#include "utMath.h"
#include <vector>
#include <stdio.h>

using namespace Horde3D;

namespace pyrite {
  class Node;
  class Node {
  public:
    Vec3f position;
    Node* parent;
    std::vector<Node*> children;

    Node(Vec3f pos, Node* p) {
      parent = p;
      if (p)
        p->addChild(this);
      position = pos;
    };

    void addChild(Node *p) {
      children.push_back(p);
    };
    void getTreeNodes(std::vector<Node*> &nodes) {
      nodes.push_back(this);
      std::vector<Node*>::iterator it;
      for(it = children.begin(); it != children.end(); ++it) {
        (*it)->getTreeNodes(nodes);
      }

    };

    void getTreeTips(std::vector<Node*> &nodes) {
      if (children.size() == 0) {
        nodes.push_back(this);
      }
      std::vector<Node*>::iterator it;
      for(it = children.begin(); it != children.end(); ++it) {
        (*it)->getTreeNodes(nodes);
      }
    }
  };
};
