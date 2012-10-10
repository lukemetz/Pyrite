#include "VoxelData.h"
#include <stdio.h>
#include <noise/noise.h>
#include "Node.h"
#include "TreeGenerator.h"

using namespace pyrite;

int mainLargeTest() {
	Vec3f point(8,8,4);
	VoxelData *v = new VoxelData(point);
	v->initOpenCl();

	v->addPlane(Plane(0,0,-1,2));

	noise::module::Perlin mod;
	mod.SetFrequency(0.05f);
	v->applyNoiseModule(&mod,3.0f);

	noise::module::Voronoi mod2;
	mod2.SetFrequency(1.0f);
	//mod2.EnableDistance();
	v->applyNoiseModule(&mod2,.2f);

	//noise::module::Voronoi mod3;
	//mod3.SetFrequency(4.0f);
	//v->applyNoiseModule(&mod3,.1f);

	v->makeShell();

	v->marchingCube();
	v->exportStl("export.stl");
}
int main2() {
	Vec3f point(4,4,1);
		VoxelData *v = new VoxelData(point);
	v->initOpenCl();

	//Block out the blade
	Vec3f v1(1,0,0);

	Vec3f v2(0,1,0);
	Vec3f v3(0,0,1);

	Matrix4f rotx45 = Matrix4f::RotMat(0,0,3.1415/4.0f);
	float x = 0.05f;
	v->clAddParallel(Vec3f(.5,.5,.5), Vec3f(.01,.05,.2),v1,v2,v3);
	v->clSubtractParallel(Vec3f(.5,.5-x,.5), Vec3f(.01,.08,.5),rotx45*v1,rotx45*v2,rotx45*v3);
	v->clSubtractParallel(Vec3f(.5,.5+x,.5), Vec3f(.01,.08,.5),rotx45*v1,rotx45*v2,rotx45*v3);

	rotx45 = Matrix4f::RotMat(0,0,-3.1415/4.0f);
	v->clSubtractParallel(Vec3f(.5,.5-x,.5), Vec3f(.01,.08,.5),rotx45*v1,rotx45*v2,rotx45*v3);
	v->clSubtractParallel(Vec3f(.5,.5+x,.5), Vec3f(.01,.08,.5),rotx45*v1,rotx45*v2,rotx45*v3);

	float f = .03;
	v->clAddParallel(Vec3f(.5,.5,.3-f), Vec3f(.02,.1,f),v1,v2,v3);

	float z = .07;
	v->clAddParallel(Vec3f(.5,.5,.3-f*2-z), Vec3f(.02,.02,z),v1,v2,v3);

	v->addNoise(7,.004f);
	v->marchingCube();
	//v->coursenSurface(70000);

	v->exportStl("export.stl");
	//v->coursenSurface(100);
	v->exportStl("exportLow.stl");

	return 0;
}
int main() {
	Vec3f point(1,1,1);
	VoxelData *v = new VoxelData(point);
	v->initOpenCl();
	//v->clAddCylinder(Vec3f(.5,.5,.5),Vec3f(.2,.0,0),.3,.1);
	//v->addSphere(Vec3f(.5,.5,.5),.1);
	Node *n = new Node(Vec3f(.5,.5,.05),NULL);
	//Node *c1 = new Node(Vec3f(0,0,0),n);
	//Node *c2 = new Node(Vec3f(0,0,0),c1);
	//Node *c3 = new Node(Vec3f(0,0,0),n);
	TreeGenerator t;
	std::vector<Vec3f> attractors;
	t.generateElipticalChrown(attractors, 200, Vec3f(.5,.5,.6), Vec3f(.5,.5,.4));

	t.generateTreeNodes(attractors, n, 0.01f, 0.014f);
	//v->clAddBox(Vec3f(.5,.5,.5),Vec3f(.1,.1,.1));

	std::vector<Node *> k;
	n->getTreeNodes(k);
	std::vector<Node *>::iterator it;
	//printf("Size %d \n",k.size());
	//for(it=k.begin(); it != k.end(); ++it) {
	//	printf("childCount %d \n",(*it)->children.size());
	//	v->addSphere((*it)->position,.05);
	//}
	t.makeBranches(n,v,.01,3);
	//std::vector<Vec3f>::iterator vit;
	//int count = 0;
	//for(vit = attractors.begin(); vit != attractors.end(); ++vit) {
	//	v->clAddSphere(*vit, .05);
	//	printf("%d/%d\n",++count, attractors.size());
	//}


	v->dumpData();
	v->marchingCube();
	v->exportStl("export.stl");
}
int maincreateFloor() {
	Vec3f point(4,4,1);

	VoxelData *v = new VoxelData(point);
//	VoxelData *v2 = new VoxelData(point);
//	v->makeSphere(.5);
	v->initOpenCl();

//	v2->initOpenCl();
//	v2->clMakeBox();

//	v->clMakeBox();
	Vec3f v1(1,0,0);

	Vec3f v2(0,1,0);
	Vec3f v3(0,0,1);

	//Matrix4f mat = Matrix4f::RotMat(3.1415/4.0f,3.1415/4.0f,0);
	//v->addCylinder(Vec3f(.5,.5,.5),Vec3f(.5,0,0),.2);
	//v->clAddParallel(Vec3f(.5,.5,.5),Vec3f(.1,.1,.1),mat*v1,mat*v2,mat*v3);
	//v->addSphere(Vec3f(.4,.5,.4), .3);
	//v->addParallel(Vec3f(.5,.5,.5),Vec3f(.1,.1,.1),mat*v1,mat*v2,mat*v3);
	//v->clAddBox(Vec3f(.5,.5,.5),Vec3f(.5,.05,.5));
	//v->addNoise(6,.01f);
	//v->addNoise(15,.005f);

	noise::module::Perlin random;
	random.SetFrequency(20);
	float mag = .5;

	#define randomVal(h) random.GetValue(x+.01,y+.1,h+12.1)*mag

	v->addPlane(Plane(0,0,-1,.5));
	for(int x=0; x < point.x/.2; x++) {
		for(int y=0; y < point.y/.2; y++) {

			Vec3f v1(1+randomVal(1),randomVal(2),randomVal(3));
			Vec3f v2(randomVal(4),1+randomVal(5),randomVal(6));
			Vec3f v3(randomVal(7),randomVal(8),1+randomVal(9));
			v1.normalize();
			v2.normalize();
			v3.normalize();

			v->clAddParallel(Vec3f(x*.22,y*.22+x%2*.1,.5), Vec3f(.1,.1,.1),v1,v2,v3);
			//v->clAddBox(Vec3f(x*.22,y*.22+x%2*.1,.5), Vec3f(.1,.1,.1));
		}
	}

	#undef randomVal

	//v->addSphere(Vec3f(.5,.5,.5), .3);
	printf("starting noise \n");
	noise::module::Perlin mod;
	mod.SetFrequency(6);
	v->applyNoiseModule(&mod,.01f);
	v->makeShell();
	v->dumpData();
	printf("done with cube");
	v->marchingCube();
	//v->coursenSurface(70000);

	v->exportStl("export.stl");
	//v->coursenSurface(100);
	v->exportStl("exportLow.stl");


}

int makeStoneWall() {
	Vec3f point(1,1,1);

	VoxelData *v = new VoxelData(point);
//	VoxelData *v2 = new VoxelData(point);
//	v->makeSphere(.5);
	v->initOpenCl();

//	v2->initOpenCl();
//	v2->clMakeBox();

//	v->clMakeBox();
	for(int x=0; x < 5; x++) {
		for(int y=0; y < 5; y++) {
			v->clAddBox(Vec3f(x*.22,.5,y*.22+x%2*.1), Vec3f(.1,.1,.1));
		}
	}

	v->clAddBox(Vec3f(.5,.5,.5),Vec3f(.5,.05,.5));
	v->addNoise(6,.01f);
	v->addNoise(15,.005f);
	v->dumpData();
	printf("done with cube");
	v->marchingCube();
	v->exportStl("export.stl");


}
