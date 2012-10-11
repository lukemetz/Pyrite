#include "VoxelData.h"

#include <noise/noise.h>

using namespace std;
using namespace pyrite;
using namespace Horde3D;

VoxelData::VoxelData(Vec3f s) {
	size.x = s.x;
	size.y = s.y;
	size.z = s.z;

	data = (float*) calloc(static_cast<size_t>(size.x/DENSITY*size.y/DENSITY*size.z/DENSITY), sizeof(float));
	clearData();
}

VoxelData::VoxelData(Vec3f s, VoxelData *d1, VoxelData *d2) {
	size.x = s.x;
	size.y = s.y;
	size.z = s.z;
	data = (float*) calloc(static_cast<size_t>(size.x/DENSITY*size.y/DENSITY*size.z/DENSITY), sizeof(float));
	clearData();

}

void VoxelData::clearData() {
	for(float x =0; x < size.x; x+= DENSITY) {
		for(float y =0; y < size.y; y+= DENSITY) {
			for(float z =0; z < size.z; z += DENSITY) {
				setDensity(x,y,z, 9999); //TODO change to min value of float
			}
		}
	}
}
VoxelData::~VoxelData() {
	delete data;
}

static void gtsFunc (gdouble ** f, GtsCartesianGrid g, guint k, gpointer data) {
  //Sphere for now
	VoxelData *voxelData = (VoxelData*) (data);

	gdouble x, y, z= g.z;

	guint i, j;

	for (i = 0, x = g.x; i < g.nx-1; i++, x += g.dx){
	for (j = 0, y = g.y; j < g.ny-1; j++, y += g.dy){
		f[i][j] = voxelData->getDensity(x,y,z);
	}}
}

GtsSurface * VoxelData::marchingCube() {
	int c = 0;
	GtsCartesianGrid g;
	gdouble iso;
	gboolean verbose = FALSE, tetra = FALSE, dual = FALSE;
	GtsIsoCartesianFunc func = gtsFunc;

	iso = 0; //Iso Value
	g.nx = (int)size.x/DENSITY;


	g.ny= (int) size.y/DENSITY;
	g.nz= (int) size.z/DENSITY;
	printf("nz is: %d nz was \n", g.nz);
	/* interval is [-10:10][-10:10][-10:10] */
	g.x = 0; g.dx = DENSITY;
	g.y = 0; g.dy = DENSITY;//size.y/(gdouble) (g.ny - 1);
	g.z = 0; g.dz = DENSITY;


	surface = gts_surface_new (gts_surface_class(),
			gts_face_class(),
			gts_edge_class(),
			gts_vertex_class());

	noise::module::Perlin module;
	module.SetFrequency(.1);

	gts_isosurface_cartesian (surface, g, func, this, iso);
	//gts_isosurface_tetra (surface, g, func, this, iso);
	//gts_isosurface_tetra_bcl (surface, g, func, this, iso);
	//gts_isosurface_tetra_bounded (surface, g, func, this, iso);
	//if (verbose)
	gts_surface_print_stats (surface, stderr);
  return surface;
}

static gboolean stop_func (gdouble cost, guint number, guint * max) {
  printf("%d,\n",number);
  if (number > *max)
    return TRUE;
  return FALSE;
}



void VoxelData::coursenSurface(int numVerts) {
	GtsKeyFunc refine_func = NULL;
	guint v = (guint)numVerts;
	gpointer stop_data = &v;
	gdouble fold = 1/180.0*3.1415;
	GtsKeyFunc cost_func = NULL;
	GtsCoarsenFunc coarsen_func = NULL;
	GtsVolumeOptimizedParams params = { 0.5, 0.5, 0. };
	gpointer cost_data = &params;
	gpointer coarsen_data = &params;

 gts_surface_coarsen (surface,
			 cost_func, cost_data,
			 coarsen_func, coarsen_data,
			 (GtsStopFunc) gts_coarsen_stop_number, stop_data, fold);
}
void VoxelData::write_face (GtsTriangle * t, std::ofstream *file)
{

  GtsVertex * v1, * v2, * v3;
  GtsVector n;

  gts_triangle_vertices (t, &v1, &v2, &v3);
  gts_triangle_normal (t, &n[0], &n[1], &n[2]);
  gts_vector_normalize (n);

  *file << "facet normal " << n[0] << " " << n[1] << " " << n[2] << "\nouter loop\n";

  *file << "vertex " << GTS_POINT (v1)->x << " " << GTS_POINT (v1)->y << " " << GTS_POINT (v1)->z << "\n";

  *file << "vertex " << GTS_POINT (v2)->x << " " << GTS_POINT (v2)->y << " " << GTS_POINT (v2)->z << "\n";

  *file << "vertex " << GTS_POINT (v3)->x << " " << GTS_POINT (v3)->y << " " << GTS_POINT (v3)->z << "\n";

  *file << "endloop\nendfacet\n";
}


void VoxelData::exportStl(std::string filePath) {

  std::ofstream file;
  file.open (filePath.c_str());
  int c = 0;
  gboolean verbose = FALSE;
  gboolean revert  = FALSE;
  GtsFile * fp;

  if (revert)
    gts_surface_foreach_face (surface, (GtsFunc) gts_triangle_revert, NULL);
  if (verbose)
    gts_surface_print_stats (surface, stderr);
  //puts ("solid");
  file << "solid \n";
  gts_surface_foreach_face (surface, (GtsFunc) VoxelData::write_face, &file);
  //puts ("endsolid");
  file << "endsolid \n";

  file.close();

}

void VoxelData::setDensity(Vec3f point, float val) {
	setDensity(point.x, point.y, point.z, val);

}

void VoxelData::setDensity(float x, float y, float z, float val) {

	assert(x >= 0);
	assert(y >= 0);
	assert(z >= 0);

	assert(x < size.x);
	assert(y < size.y);
	assert(z < size.z);
	//Round to nearest
	x = static_cast<int>(x/DENSITY)*DENSITY;
	y = static_cast<int>(y/DENSITY)*DENSITY;
	z = static_cast<int>(z/DENSITY)*DENSITY;
	float d = 1/DENSITY;
	/*printf("setting %d has size %d \n",
		static_cast<int>((x*d+size.x*d*y*d + size.x*d*size.y*d*z*d)),
		static_cast<size_t>(size.x/DENSITY*size.y/DENSITY*size.z/DENSITY)); */
	data[static_cast<int>((x*d+size.x*d*y*d + size.x*d*size.y*d*z*d))] = val;
}
float VoxelData::getDensity(Vec3f point) {
	return getDensity(point.x, point.y, point.z);
}

void VoxelData::addDensity(float x, float y, float z, float val) {

	assert(x >= 0);
	assert(y >= 0);
	assert(z >= 0);

	assert(x < size.x);
	assert(y < size.y);
	assert(z < size.z);
	//Round to nearest
	x = static_cast<int>(x/DENSITY)*DENSITY;
	y = static_cast<int>(y/DENSITY)*DENSITY;
	z = static_cast<int>(z/DENSITY)*DENSITY;
	float d = 1/DENSITY;

	data[static_cast<int>((x*d+size.x*d*y*d + size.x*d*size.y*d*z*d))] =
		data[static_cast<int>((x*d+size.x*d*y*d + size.x*d*size.y*d*z*d))]+val;

	//printf("%f, %f\n",data[static_cast<int>((x*d+size.x*d*y*d + size.x*d*size.y*d*z*d))], val);
}

void VoxelData::unionDensity(float x, float y, float z, float val) {
	assert(x >= 0);
	assert(y >= 0);
	assert(z >= 0);

	assert(x < size.x);
	assert(y < size.y);
	assert(z < size.z);
	//Round to nearest
	x = static_cast<int>(x/DENSITY)*DENSITY;
	y = static_cast<int>(y/DENSITY)*DENSITY;
	z = static_cast<int>(z/DENSITY)*DENSITY;
	float d = 1/DENSITY;

	data[static_cast<int>((x*d+size.x*d*y*d + size.x*d*size.y*d*z*d))] = min(data[static_cast<int>((x*d+size.x*d*y*d + size.x*d*size.y*d*z*d))],val);
}
void VoxelData::addSphere(Vec3f pos, float radius) {
	for(float x =0; x < size.x; x+= DENSITY) {
		for(float y =0; y < size.y; y+= DENSITY) {
			for(float z =0; z < size.z; z += DENSITY) {
				//printf ("setting density for sphere %f, %f \n", z, DENSITY);
				unionDensity(x,y,z, (pos[0]-x)*(pos[0]-x)+(pos[1]-y)*(pos[1]-y)+(pos[2]-z)*(pos[2]-z)-radius*radius);
			}
		}
	}
}

float VoxelData::getDensity(float x, float y, float z) {
	x = static_cast<int>(x/DENSITY)*DENSITY;
	y = static_cast<int>(y/DENSITY)*DENSITY;
	z = static_cast<int>(z/DENSITY)*DENSITY;
	//printf("%f x %f sizex %f\n", y, size.x, size.y);
	float d = 1/DENSITY;
	/*printf("geting %d (%f, %f, %f) has size %d \n",
		static_cast<int>((x*d+size.x*d*y*d + size.x*d*size.y*d*z*d)), x,y,z,
		static_cast<size_t>(size.x/DENSITY*size.y/DENSITY*size.z/DENSITY));*/
	assert(x >= 0);
	assert(y >= 0);
	assert(z >= 0);

	assert(x < size.x);
	assert(y < size.y);
	assert(z < size.z);
	//printf("n %f \n", static_cast<int>((1/DENSITY)*(x+size.x*y + size.x*size.y*z)));
	return data[static_cast<int>((x*d+size.x*d*y*d + size.x*d*size.y*d*z*d))];

}



void VoxelData::addParallel(Vec3f pos, Vec3f offset, Vec3f v1, Vec3f v2, Vec3f v3) {
	float val;
	for(float x =0; x < size.x; x+= DENSITY) {
		for(float y =0; y < size.y; y+= DENSITY) {
			for(float z =0; z < size.z; z += DENSITY) {
				val = max( max(
					-offset[0]+fabs(-(x-pos[0])*v1[0]-(y-pos[1])*v1[1]-(z-pos[2])*v1[2]),
					-offset[1]+fabs(-(x-pos[0])*v2[0]-(y-pos[1])*v2[1]-(z-pos[2])*v2[2])
					),
					-offset[2]+fabs(-(x-pos[0])*v3[0]-(y-pos[1])*v3[1]-(z-pos[2])*v3[2])
				);
				unionDensity(x,y,z, val);
			}
		}
	}

}
void VoxelData::addCylinder(Vec3f pos, Vec3f direction, float radius1, float radius2) {

	//create the plane for the middle of the cylinder.
	float val;
	Plane p(direction.x,direction.y,direction.z,0.0f);
	float dist = p.distToPoint(pos);
	p.dist = -dist; //Offset so at pos input.
	//printf("%f\n",dist);
	float h = direction.length();
	for(float x =0; x < size.x; x+= DENSITY) {
		for(float y =0; y < size.y; y+= DENSITY) {
			for(float z =0; z < size.z; z += DENSITY) {
				dist = p.distToPoint(Vec3f(x,y,z));

				val = fmax(-pow((radius1*(.5+(dist)/(2*h))+ (radius2*(.5-(dist)/(2*h)))),2)
					-dist*dist+(x-pos[0])*(x-pos[0])+(y-pos[1])*(y-pos[1])+(z-pos[2])*(z-pos[2]),-h*h+dist*dist);//+fabs((x-pos[0])*(x-pos[0])*0+(y-pos[1])*(y-pos[1])*.3+(z-pos[2])*(z-pos[2])*.3);
				//printf("posdist %f distancefromplane %f\n",dist*dist,(x-pos[0])*(x-pos[0])+(y-pos[1])*(y-pos[1])+(z-pos[2])*(z-pos[2]));
				unionDensity(x,y,z, val);
			}
		}
	}
}

void VoxelData::addPlane(Plane plane) {
	float val;
	for(float x =0; x < size.x; x+= DENSITY) {
		for(float y =0; y < size.y; y+= DENSITY) {
			for(float z =0; z < size.z; z += DENSITY) {
				val = -plane.distToPoint(Vec3f(x,y,z));
				unionDensity(x,y,z,val);
			}
		}
	}
}

void VoxelData::makeShell() {
	float val = 99;
	for(float x =0; x < size.x; x+= DENSITY) {
		for(float y =0; y < size.y; y+= DENSITY) {
			setDensity(x,y,size.z-DENSITY/2.0f,val);
			setDensity(x,y,0,val);
		}
	}
	for(float x =0; x < size.x; x+= DENSITY) {
		for(float z =0; z < size.z; z+= DENSITY) {
			setDensity(x,size.y-DENSITY/2.0f,z,val);
			setDensity(x,0,z,val);
		}
	}
	for(float z =0; z < size.z; z+= DENSITY) {
		for(float y =0; y < size.y; y+= DENSITY) {
			setDensity(size.x-DENSITY/2.0f,y,z,val);
			setDensity(0,y,z,val);
		}
	}
}

void VoxelData::addNoise(float frequency, float mult) {
	noise::module::Perlin perlin;
	perlin.SetFrequency(frequency);
	for(float x =0; x < size.x; x+= DENSITY) {
		for(float y =0; y < size.y; y+= DENSITY) {
			for(float z =0; z < size.z; z += DENSITY) {
				//printf ("setting density for sphere %f, %f \n", z, DENSITY);
				//printf("%f\n",perlin.GetValue(x+.001, y+.001, z+.001)*.0001);
				addDensity(x,y,z, mult*perlin.GetValue(x+.001, y+.001, z+.001));
			}
		}
	}

}
void VoxelData::applyNoiseModule(noise::module::Module *mod, float amount) {
	noise::module::Perlin perlin;
	for(float x =0; x < size.x; x+= DENSITY) {
		for(float y =0; y < size.y; y+= DENSITY) {
			for(float z =0; z < size.z; z += DENSITY) {
				//printf ("setting density for sphere %f, %f \n", z, DENSITY);
				//printf("%f\n",perlin.GetValue(x+.001, y+.001, z+.001)*.0001);
				addDensity(x,y,z, amount*mod->GetValue(x+.001, y+.001, z+.001));
			}
		}
	}
}

void VoxelData::dumpData() {
	std::ofstream file;
	file.open ("dump.txt");
	for(int i=0; i < size.x/DENSITY*size.y/DENSITY*size.z/DENSITY; i++) {
		file << data[i] << ",";
	}
	file.close();

}
