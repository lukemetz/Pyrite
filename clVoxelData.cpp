#include "VoxelData.h"

#include <noise/noise.h>
#include "kernel.h"
using namespace std;

void VoxelData::initOpenCl() {

	cl_int err = CL_SUCCESS;
	try {

		std::vector<cl::Platform> platforms;
		cl::Platform::get(&platforms);
		if (platforms.size() == 0) {
			std::cout << "Platform size 0\n";
			return;
		}

    std::string kernelStr((const char *)kernel_cl,static_cast<size_t>(kernel_cl_len) );
    printf("kernel %s \n", kernelStr.c_str());
		cl_context_properties properties[] =
			{ CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[0])(), 0};

		context = new cl::Context(CL_DEVICE_TYPE_GPU, properties);

		std::vector<cl::Device> devices = context->getInfo<CL_CONTEXT_DEVICES>();

		cl::Program::Sources source(1,
			std::make_pair(kernelStr.c_str(), strlen( kernelStr.c_str()) )
			);

		program_ = new cl::Program(*context, source);

		program_->build(devices);

	}
	catch (cl::Error err) {
		std::cerr
			<< "ERROR: "
			<< err.what()
			<< "("
			<< err.err()
			<< ")"
			<< std::endl;
	}
}


cl::Buffer VoxelData::Vec3fToCl(Vec3f val, unsigned int argNum, cl::Kernel &kernel, cl::CommandQueue &queue, cl::Event &event) {
	cl_int err = CL_SUCCESS;
	try {
		size_t sfloat3 = sizeof(float)*3;

		float* x = (float*) malloc(sfloat3);
		x[0] = val.x;
		x[1] = val.y;
		x[2] = val.z;
		cldata.push_back(x);

		cl::Buffer cl_x = cl::Buffer(*context, CL_MEM_READ_ONLY, sfloat3, NULL, &err);

		err = queue.enqueueWriteBuffer(cl_x, CL_TRUE, 0, sfloat3, x, NULL, &event);
		kernel.setArg(argNum, cl_x);

		return cl_x;
	}
	catch (cl::Error err) {
		std::cerr
			<< "ERROR: "
			<< err.what()
			<< "("
			<< err.err()
			<< ")"
			<< std::endl;
	}
}

cl::Buffer VoxelData::floatToCl(float val, unsigned int argNum, cl::Kernel &kernel, cl::CommandQueue &queue, cl::Event &event) {
	cl_int err = CL_SUCCESS;
	try {
		size_t sfloat = sizeof(float);

		float* x = (float*) malloc(sfloat);
		x[0] = val;

		cldata.push_back(x);

		cl::Buffer cl_x = cl::Buffer(*context, CL_MEM_READ_ONLY, sfloat, NULL, &err);

		err = queue.enqueueWriteBuffer(cl_x, CL_TRUE, 0, sfloat, x, NULL, &event);
		kernel.setArg(argNum, cl_x);

		return cl_x;
	}
	catch (cl::Error err) {
		std::cerr
			<< "ERROR: "
			<< err.what()
			<< "("
			<< err.err()
			<< ")"
			<< std::endl;
	}
}


void VoxelData::cleanInternalStorage() {
	std::vector<float *>::iterator it;
	for(it = cldata.begin(); it != cldata.end(); ++it) {
		delete *it;
	}
	cldata.clear();
}


void VoxelData::clAddBox(Vec3f center, Vec3f dx) {
		//__global float* out, __global float* x, __global float* dx, __global float* arraySize, __global float* density
	cl_int err = CL_SUCCESS;
	try {

		std::vector<cl::Device> devices = context->getInfo<CL_CONTEXT_DEVICES>();

		cl::Kernel kernel(*program_, "makeBox", &err);
		cl::Event event;
		cl::CommandQueue queue(*context, devices[0], 0, &err);

		size_t out_size = (size.x/DENSITY)*(size.y/DENSITY)*(size.z/DENSITY)*sizeof(float);

		cl::Buffer cl_out = cl::Buffer(*context, CL_MEM_WRITE_ONLY, out_size, NULL, &err);
		err = queue.enqueueWriteBuffer(cl_out, CL_TRUE, 0, out_size, data, NULL, &event); //Output directly to the main buffer
		kernel.setArg(0,cl_out);



		float cuttoff = 2;
		//Ensure that we are within the boundary
		Vec3f newSize = min(dx*2*cuttoff, size);
		Vec3f offset = max(Vec3f(0,0,0), center - newSize/2);

		Vec3f overflow = (newSize+offset)-size;
		overflow = max(overflow, Vec3f(0,0,0));
		newSize -= overflow;

//		offset = Vec3f(0,0,0);
//		newSize = size;
		// Set up the inputs
		cl::Buffer cl_x = Vec3fToCl(center, 1, kernel, queue, event);
		cl::Buffer cl_dx = Vec3fToCl(dx, 2, kernel, queue, event);

		cl::Buffer cl_arraySize = Vec3fToCl(size, 3, kernel, queue, event); //Area with some margin of error. makes small boxes go faster.
		cl::Buffer cl_sectionSize = Vec3fToCl(newSize, 4, kernel, queue, event);
		cl::Buffer cl_offset = Vec3fToCl(offset, 5, kernel, queue, event);
		cl::Buffer cl_density = floatToCl(DENSITY, 6, kernel, queue, event);

		size_t out_num = (newSize.x/DENSITY)*(newSize.y/DENSITY)*(newSize.z/DENSITY);

		queue.enqueueNDRangeKernel(
			kernel,
			cl::NullRange,
			cl::NDRange(out_num), //GLobal workgroup size
			cl::NullRange,
			NULL,
			&event);

		queue.finish();
		event.wait();

		err = queue.enqueueReadBuffer(cl_out, CL_TRUE, 0, out_size, data, NULL, &event);

		cleanInternalStorage(); //Deletes all of the buffers used to copy data over to gpu.
		queue.flush();
		printf("Done with make boxkernel \n");
	}
	catch (cl::Error err) {
		std::cerr
			<< "ERROR: "
			<< err.what()
			<< "("
			<< err.err()
			<< ")"
			<< std::endl;
	}

}

void VoxelData::clSubtractParallel(Vec3f center, Vec3f dx, Vec3f v1, Vec3f v2, Vec3f v3) {
		//__global float* out, __global float* x, __global float* dx, __global float* arraySize, __global float* density
	cl_int err = CL_SUCCESS;
	try {

		std::vector<cl::Device> devices = context->getInfo<CL_CONTEXT_DEVICES>();

		cl::Kernel kernel(*program_, "subParallel", &err);
		cl::Event event;


		cl::CommandQueue queue(*context, devices[0], 0, &err);
		size_t out_size = (size.x/DENSITY)*(size.y/DENSITY)*(size.z/DENSITY)*sizeof(float);
		cl::Buffer cl_out = cl::Buffer(*context, CL_MEM_WRITE_ONLY, out_size, NULL, &err);
		err = queue.enqueueWriteBuffer(cl_out, CL_TRUE, 0, out_size, data, NULL, &event); //Output directly to the main buffer
		kernel.setArg(0,cl_out);

		float cuttoff = 2;
		//Ensure that we are within the boundary
		Vec3f newSize = min(dx*2*cuttoff, size);
		Vec3f offset = max(Vec3f(0,0,0), center - newSize/2);

		Vec3f overflow = (newSize+offset)-size;
		overflow = max(overflow, Vec3f(0,0,0));
		newSize -= overflow;

		//offset = Vec3f(0,0,0);
		//newSize = size;
		// Set up the inputs
		cl::Buffer cl_x = Vec3fToCl(center, 1, kernel, queue, event);
		cl::Buffer cl_dx = Vec3fToCl(dx, 2, kernel, queue, event);

		cl::Buffer cl_v1 = Vec3fToCl(v1, 3, kernel, queue, event);
		cl::Buffer cl_v2 = Vec3fToCl(v2, 4, kernel, queue, event);
		cl::Buffer cl_v3 = Vec3fToCl(v3, 5, kernel, queue, event);

		cl::Buffer cl_arraySize = Vec3fToCl(size, 6, kernel, queue, event); //Area with some margin of error. makes small boxes go faster.
		cl::Buffer cl_sectionSize = Vec3fToCl(newSize, 7, kernel, queue, event);
		cl::Buffer cl_offset = Vec3fToCl(offset, 8, kernel, queue, event);
		cl::Buffer cl_density = floatToCl(DENSITY, 9, kernel, queue, event);
		size_t out_num = (newSize.x/DENSITY)*(newSize.y/DENSITY)*(newSize.z/DENSITY);

		queue.enqueueNDRangeKernel(
			kernel,
			cl::NullRange,
			cl::NDRange(out_num), //GLobal workgroup size
			cl::NullRange,
			NULL,
			&event);

		queue.finish();
		event.wait();

		err = queue.enqueueReadBuffer(cl_out, CL_TRUE, 0, out_size, data, NULL, &event);
		queue.flush();
		cleanInternalStorage();
		printf("Done with make parallel kernel \n");
	}
	catch (cl::Error err) {
		std::cerr
			<< "ERROR: "
			<< err.what()
			<< "("
			<< err.err()
			<< ")"
			<< std::endl;
	}
}


void VoxelData::clAddParallel(Vec3f center, Vec3f dx, Vec3f v1, Vec3f v2, Vec3f v3) {
		//__global float* out, __global float* x, __global float* dx, __global float* arraySize, __global float* density
	cl_int err = CL_SUCCESS;
	try {

		std::vector<cl::Device> devices = context->getInfo<CL_CONTEXT_DEVICES>();

		cl::Kernel kernel(*program_, "makeParallel", &err);
		cl::Event event;

		cl::CommandQueue queue(*context, devices[0], 0, &err);
		size_t out_size = (size.x/DENSITY)*(size.y/DENSITY)*(size.z/DENSITY)*sizeof(float);
		cl::Buffer cl_out = cl::Buffer(*context, CL_MEM_WRITE_ONLY, out_size, NULL, &err);
		err = queue.enqueueWriteBuffer(cl_out, CL_TRUE, 0, out_size, data, NULL, &event); //Output directly to the main buffer
		kernel.setArg(0,cl_out);

		float cuttoff = 2;
		//Ensure that we are within the boundary
		Vec3f newSize = min(dx*2*cuttoff, size);
		Vec3f offset = max(Vec3f(0,0,0), center - newSize/2);

		Vec3f overflow = (newSize+offset)-size;
		overflow = max(overflow, Vec3f(0,0,0));
		newSize -= overflow;

		//soffset = Vec3f(0,0,0);
		//newSize = size;
		// Set up the inputs
		cl::Buffer cl_x = Vec3fToCl(center, 1, kernel, queue, event);
		cl::Buffer cl_dx = Vec3fToCl(dx, 2, kernel, queue, event);

		cl::Buffer cl_v1 = Vec3fToCl(v1, 3, kernel, queue, event);
		cl::Buffer cl_v2 = Vec3fToCl(v2, 4, kernel, queue, event);
		cl::Buffer cl_v3 = Vec3fToCl(v3, 5, kernel, queue, event);

		cl::Buffer cl_arraySize = Vec3fToCl(size, 6, kernel, queue, event); //Area with some margin of error. makes small boxes go faster.
		cl::Buffer cl_sectionSize = Vec3fToCl(newSize, 7, kernel, queue, event);
		cl::Buffer cl_offset = Vec3fToCl(offset, 8, kernel, queue, event);
		cl::Buffer cl_density = floatToCl(DENSITY, 9, kernel, queue, event);

		size_t out_num = (newSize.x/DENSITY)*(newSize.y/DENSITY)*(newSize.z/DENSITY);
		queue.enqueueNDRangeKernel(
			kernel,
			cl::NullRange,
			cl::NDRange(out_num), //GLobal workgroup size
			cl::NullRange,
			NULL,
			&event);

		queue.finish();
		event.wait();
		//scl_out.release();
		err = queue.enqueueReadBuffer(cl_out, CL_TRUE, 0, out_size, data, NULL, &event);
		queue.flush();
		cleanInternalStorage();
		printf("Done with make parallel kernel \n");
	}
	catch (cl::Error err) {
		std::cerr
			<< "ERROR: "
			<< err.what()
			<< "("
			<< err.err()
			<< ")"
			<< std::endl;
	}
}



void VoxelData::clAddCylinder(Vec3f pos, Vec3f direction, float radius1, float radius2) {
		//__global float* out, __global float* x, __global float* dx, __global float* arraySize, __global float* density
	cl_int err = CL_SUCCESS;
	try {

		std::vector<cl::Device> devices = context->getInfo<CL_CONTEXT_DEVICES>();

		cl::Kernel kernel(*program_, "addCylinder", &err);
		cl::Event event;

		cl::CommandQueue queue(*context, devices[0], 0, &err);
		size_t out_size = (size.x/DENSITY)*(size.y/DENSITY)*(size.z/DENSITY)*sizeof(float);
		cl::Buffer cl_out = cl::Buffer(*context, CL_MEM_WRITE_ONLY, out_size, NULL, &err);
		err = queue.enqueueWriteBuffer(cl_out, CL_TRUE, 0, out_size, data, NULL, &event); //Output directly to the main buffer
		kernel.setArg(0,cl_out);


		float val;
		Plane p(direction.x,direction.y,direction.z,0.0f);
		float dist = p.distToPoint(pos);
		p.dist = -dist;
		float h = direction.length();


		float cuttoff = 1.5;
		//Ensure that we are within the boundary
		Vec3f newSize = min(Vec3f(1,1,1)*2*fmax(fmax(radius2, radius1),h)*cuttoff, size);
		Vec3f offset = max(Vec3f(0,0,0), pos - newSize/2);

		Vec3f overflow = (newSize+offset)-size;
		overflow = max(overflow, Vec3f(0,0,0));
		newSize -= overflow;


		//soffset = Vec3f(0,0,0);
		//newSize = size;
		// Set up the inputs
		cl::Buffer cl_x = Vec3fToCl(pos, 1, kernel, queue, event);
		cl::Buffer cl_dx = Vec3fToCl(p.normal, 2, kernel, queue, event);

		cl::Buffer cl_dist = floatToCl(p.dist, 3, kernel, queue, event);
		cl::Buffer cl_radius1 = floatToCl(radius1, 4, kernel, queue, event);
		cl::Buffer cl_radius2 = floatToCl(radius2, 5, kernel, queue, event);
		cl::Buffer cl_h = floatToCl(h, 6, kernel, queue, event);

		cl::Buffer cl_arraySize = Vec3fToCl(size, 7, kernel, queue, event); //Area with some margin of error. makes small boxes go faster.
		cl::Buffer cl_sectionSize = Vec3fToCl(newSize, 8, kernel, queue, event);
		cl::Buffer cl_offset = Vec3fToCl(offset, 9, kernel, queue, event);
		cl::Buffer cl_density = floatToCl(DENSITY, 10, kernel, queue, event);



		size_t out_num = (newSize.x/DENSITY)*(newSize.y/DENSITY)*(newSize.z/DENSITY);
		queue.enqueueNDRangeKernel(
			kernel,
			cl::NullRange,
			cl::NDRange(out_num), //GLobal workgroup size
			cl::NullRange,
			NULL,
			&event);

		queue.finish();
		event.wait();

		err = queue.enqueueReadBuffer(cl_out, CL_TRUE, 0, out_size, data, NULL, &event);
		queue.flush();
		queue.finish();
		cleanInternalStorage();
		//printf("Done with make parallel kernel \n");
	}
	catch (cl::Error err) {
		std::cerr
			<< "ERROR: "
			<< err.what()
			<< "("
			<< err.err()
			<< ")"
			<< std::endl;
	}
}
void VoxelData::clAddSphere(Vec3f pos, float radius) {
		cl_int err = CL_SUCCESS;
	try {

		std::vector<cl::Device> devices = context->getInfo<CL_CONTEXT_DEVICES>();

		cl::Kernel kernel(*program_, "addSphere", &err);
		cl::Event event;

		cl::CommandQueue queue(*context, devices[0], 0, &err);
		size_t out_size = (size.x/DENSITY)*(size.y/DENSITY)*(size.z/DENSITY)*sizeof(float);
		cl::Buffer cl_out = cl::Buffer(*context, CL_MEM_WRITE_ONLY, out_size, NULL, &err);
		err = queue.enqueueWriteBuffer(cl_out, CL_TRUE, 0, out_size, data, NULL, &event); //Output directly to the main buffer
		kernel.setArg(0,cl_out);




		float cuttoff = 2;
		//Ensure that we are within the boundary
		Vec3f newSize = min(Vec3f(1,1,1)*radius*2*cuttoff, size);
		Vec3f offset = max(Vec3f(0,0,0), pos - newSize/2);

		Vec3f overflow = (newSize+offset)-size;
		overflow = max(overflow, Vec3f(0,0,0));
		newSize -= overflow;


		//soffset = Vec3f(0,0,0);
		//newSize = size;
		// Set up the inputs
		cl::Buffer cl_x = Vec3fToCl(pos, 1, kernel, queue, event);

		cl::Buffer cl_dx = floatToCl(radius, 2, kernel, queue, event);


		cl::Buffer cl_arraySize = Vec3fToCl(size, 3, kernel, queue, event); //Area with some margin of error. makes small boxes go faster.
		cl::Buffer cl_sectionSize = Vec3fToCl(newSize, 4, kernel, queue, event);
		cl::Buffer cl_offset = Vec3fToCl(offset, 5, kernel, queue, event);
		cl::Buffer cl_density = floatToCl(DENSITY, 6, kernel, queue, event);



		size_t out_num = (newSize.x/DENSITY)*(newSize.y/DENSITY)*(newSize.z/DENSITY);
		queue.enqueueNDRangeKernel(
			kernel,
			cl::NullRange,
			cl::NDRange(out_num), //GLobal workgroup size
			cl::NullRange,
			NULL,
			&event);

		queue.finish();
		event.wait();

		err = queue.enqueueReadBuffer(cl_out, CL_TRUE, 0, out_size, data, NULL, &event);
		queue.flush();
		queue.finish();
		cleanInternalStorage();
		//printf("Done with make parallel kernel \n");
	}
	catch (cl::Error err) {
		std::cerr
			<< "ERROR: "
			<< err.what()
			<< "("
			<< err.err()
			<< ")"
			<< std::endl;
	}
}
