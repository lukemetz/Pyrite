#pragma once

#include <math.h>
#include <locale.h>
#include <string.h>
#include <fstream>
#include <algorithm>

#include <iostream>
#include <stdio.h>
#include <stdlib.h> //for calloc
#include <assert.h>


#include <gts.h>


#define __CL_ENABLE_EXCEPTIONS
#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif

#include "utMath.h"


#include <noise/noise.h>
//#define DENSITY (1.0/8.0f)
//#define DENSITY (1.0/16.0f)
#define DENSITY (1.0/32.0f)
//#define DENSITY (1.0/64.0f)
//#define DENSITY (1.0/128.0f)
//#define DENSITY (1.0/(128.0f*1.5f))

//class VoxelData;


namespace pyrite {
  class VoxelData {

  public:

    VoxelData(Horde3D::Vec3f s);
    VoxelData(Horde3D::Vec3f s, VoxelData *d1, VoxelData *d2);
    ~VoxelData();

    void initOpenCl();

    void setDensity(Horde3D::Vec3f point, float val);
    void setDensity(float x, float y, float z, float val);
    float getDensity(Horde3D::Vec3f point);
    float getDensity(float x, float y, float z);
    void addDensity(float x, float y, float z, float val);
    void unionDensity(float x, float y, float z, float val);

    void marchingCube(); //Turns voxelData ino a mesh, GTSSurface.
    void coursenSurface(int numVerts); //Refines the mesh to specific amount of verts.

    void exportStl(std::string filePath); //Exports current GTSSurface.
    void dumpData();

    void clearData();
    void addSphere(Horde3D::Vec3f pos, float radius);
    void addParallel(Horde3D::Vec3f pos, Horde3D::Vec3f offset, Horde3D::Vec3f v1, Horde3D::Vec3f v2, Horde3D::Vec3f v3);

    void addCylinder(Horde3D::Vec3f pos, Horde3D::Vec3f direction, float radius1, float radius2);
    void addPlane(Horde3D::Plane plane);

    void makeShell();

    void applyNoiseModule(noise::module::Module *mod, float amount);

    void clAddBox(Horde3D::Vec3f center, Horde3D::Vec3f dx);
    void clAddSphere(Horde3D::Vec3f pos, float radius);
    void clAddParallel(Horde3D::Vec3f pos, Horde3D::Vec3f dx, Horde3D::Vec3f v1, Horde3D::Vec3f v2, Horde3D::Vec3f v3);
    void clSubtractParallel(Horde3D::Vec3f pos, Horde3D::Vec3f dx, Horde3D::Vec3f v1, Horde3D::Vec3f v2, Horde3D::Vec3f v3);

    void clAddCylinder(Horde3D::Vec3f pos, Horde3D::Vec3f direction, float radius1, float radius2);

    void addNoise(float frequency, float mult);
  protected:
    //Helper functunctions for cl
    cl::Buffer Vec3fToCl(Horde3D::Vec3f val, unsigned int argNum, cl::Kernel &kernel, cl::CommandQueue &queue, cl::Event &event);
    cl::Buffer floatToCl(float val, unsigned int argNum, cl::Kernel &kernel, cl::CommandQueue &queue, cl::Event &event);
    cl::Buffer createOutBuffer(cl::Kernel &kernel, cl::CommandQueue &queue, cl::Event &event);
    void cleanInternalStorage();
    std::vector<float *> cldata;
  private:
    float *data;
    Horde3D::Vec3f size;
    cl::Program* program_;
    cl::Context* context;
    static void write_face(GtsTriangle * t, std::ofstream *file);
    GtsSurface *surface;
  };
};
