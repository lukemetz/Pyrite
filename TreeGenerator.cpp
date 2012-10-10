#include "TreeGenerator.h"

#include <cstdlib> //for rand max

using namespace pyrite;

void TreeGenerator::generateTreeNodes(std::vector<Vec3f> attractors, Node* baseNode, float nodeLength, float killDistance) {
	int iterationsLeft = 300;

	std::map<Node*, std::vector<Vec3f> > closestNode;
	while (attractors.size() > 0 && iterationsLeft > 0) {
		//Find which node is closest to each attractor.
		closestNode.clear();
		//Gets populated automatically.s

		std::vector<Node *> nodes;
		baseNode->getTreeNodes(nodes);

		std::vector<Vec3f>::iterator it;
		std::vector<Node *>::iterator nit;
		int onIndex = 0;
		for(it = attractors.begin(); it!=attractors.end(); ++it) {
			Node* closest = 0;
			float closestLength = 0;
			for(nit=nodes.begin(); nit!=nodes.end(); ++nit) {
				if ( closest ==0 ) {
					closest = *nit;
					closestLength = (closest->position-*it).length();
					continue;
				}
				float length = ((*nit)->position-*it).length();
				if (closestLength > length) {
					closest = *nit;
					closestLength = length;
				}
			}
			closestNode[closest].push_back(*it);
			//printf("Closest %f, %f, %f \n",closest->position.x,closest->position.y,closest->position.z);
		}

		//Clear so as to repopulate latter.
		attractors.clear();

		//for each node
		std::map<Node*, std::vector<Vec3f> >::iterator cit;
		for(cit=closestNode.begin(); cit != closestNode.end(); ++cit) {
			//average the attractors
			Vec3f sum;
			for(it=cit->second.begin(); it != cit->second.end(); ++it) {
				Vec3f diff = *it-cit->first->position;
				diff.normalize();
				sum += diff;
			}

			//Resize
			sum.normalize();
			sum *= nodeLength;
			//create the new node with the offset;
			Vec3f newPos = cit->first->position+sum;
			//Do some checks to ensure that we are not creating the same node over and over again.
			bool proceed = true;
			for(nit = cit->first->children.begin(); nit != cit->first->children.end();++nit) {
				if ((*nit)->position == newPos) {
					proceed = false;
				}
			}

			if (!proceed)
				continue;
			Node* addedNode = new Node(newPos, cit->first);
			//printf("%d, size in loop \n",cit->first->children.size());
			//Check this point to see if it is close enough to any attractors. If so remove them.
			for(it = cit->second.begin(); it != cit->second.end(); ++it) {
				Vec3f diff = newPos-(*it);
				float length = diff.length();
				if (length > killDistance) {
					//Remove the attractor set.
					attractors.push_back(*it);
				}
			}

		}

		iterationsLeft--;
	}

	void generateCircularChrown(std::vector<Vec3f> &att);
}


void TreeGenerator::generateElipticalChrown(std::vector<Vec3f> &att, int numPoints, Vec3f pos, Vec3f radius) {
	srand(12323); //seed the random;
	while (numPoints > 0) {
		float x = 2*((float)rand()/(float)RAND_MAX-.5)*radius.x;
		float y = 2*((float)rand()/(float)RAND_MAX-.5)*radius.y;
		float z = 2*((float)rand()/(float)RAND_MAX-.5)*radius.z;

		float val = x*x/(radius.x*radius.x)+y*y/(radius.y*radius.y)+z*z/(radius.z*radius.z);
		if (val < 1) {
			att.push_back(pos+Vec3f(x,y,z));
			numPoints--;
		}

	}

}

void TreeGenerator::makeBranches(Node *baseNode, VoxelData * v, float tipRadius, float power) {
	std::vector<Node *> tips;
	baseNode->getTreeTips(tips);
	std::map<Node *, int>::iterator it;
	std::map<Node*, float> data;

	std::map<Node *, int> newNodes; //Make this a map to remove duplicate keys when doign parents.

	std::vector<Node *>::iterator nit;
	for(nit=tips.begin(); nit != tips.end(); ++nit) {
		data[*nit] = tipRadius;
		newNodes[*nit] = 1;
	}

	std::map<Node *, int> temp;
	for(it = newNodes.begin(); it != newNodes.end(); ++it) {
		temp[it->first] = 1;
	}

	while (newNodes.size() != 0) {
		temp.clear();
		for(it = newNodes.begin(); it != newNodes.end(); ++it) {
			temp[it->first] = 1;
		}
		newNodes.clear();
		for(it=temp.begin(); it != temp.end(); ++it) {
			Node* parent = it->first->parent;
			bool goodToGo = true;
			if (parent != NULL) {
				if (parent->children.size() == 1) {
					data[parent] = data[it->first];
				} else {
					float sum = 0;


					for(nit = parent->children.begin(); nit != parent->children.end(); ++nit) {
						if (data.count(*nit) ==0) {
							goodToGo = false;
							break;
						}
						sum += pow(data[*nit],power);
					}
					sum = pow(sum,1.0/power); //Change to base power.
					if (goodToGo)
						data[parent] = sum;
				}
				if (goodToGo) {
					newNodes[parent] = 1; //Mark this node, 1 means nothing.
				} else {
					newNodes[it->first] = 1; //throw oneself back into the que to get waited on again.
				}
			}
		}
	}

	/*
	for(it=tips.begin(); it != tips.end(); ++it) {
		int level = 0;
		Node *temp = (*it);
		while (temp->parent != 0) {
			//Check if level already exists
			if (data.count(temp) == 0) {
				data[temp] = level;
			} else {
				data[temp] = max(data[temp], level);
			}
			temp = temp->parent;
			level ++;
		}
	}
	*/

	//Todo: Separate to nother function?
	//Display data now
	std::map<Node *, float>::iterator dit;
	int count = 0;
	int sizes = data.size();

	for(dit = data.begin(); dit != data.end(); ++dit) {
		Node* n1 = dit->first;
		Node* n2 = dit->first->parent;
		if (n2 == NULL)
			continue;
		float size = dit->second;
		//v->clAddBox(n1->position, Vec3f(size,size,size));
		//v->clAddBox(n1->position, Vec3f(.04,.04,.04));
		Vec3f avgPos = (n1->position+n2->position)/2;
		Vec3f direction = (n1->position - n2->position)/2;
		//printf("  creating \n");
		v->clAddCylinder(avgPos,direction,data[n1], data[n2]);
		v->clAddSphere(n1->position,data[n1]);
		printf("Generating voxels %d/%d \n",count,sizes);
		count ++;
	}


}
