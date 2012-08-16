__constant sampler_t mySampler = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_TRUE| CLK_ADDRESS_CLAMP_TO_EDGE;

/*
__kernel void hello(__global float* bufferA, __global float* bufferB, __global image2d_t img) {
	unsigned int i = get_global_id(0);
	float2 pos;
	pos.x = i/16.0f; pos.y = .5;


	float4 val = read_imagef(img, mySampler, pos);
	out[i] = val.s0;
	//out[i] = 1;
} */


float4 getPos(int i, __global float* sectionSize, __global float * offset, __global float* density) {
	float4 v;
	v.x = i%(int)(sectionSize[0]/density[0])*density[0]+offset[0];
	v.y = (int)(i/(sectionSize[0]/density[0]))%(int)(sectionSize[1]/density[0])*density[0]+offset[1];
	v.z = (int)(i/(sectionSize[0]/density[0])/(sectionSize[1]/density[0]))*density[0]+offset[2];
	return v;

}

int getOut(float4 pos, __global float* arraySize, __global float* density) {
	//return 1;
	float d = 1.0/density[0];
	/*printf("setting %d has size %d \n",
		static_cast<int>((x*d+size.x*d*y*d + size.x*d*size.y*d*z*d)),
		static_cast<size_t>(size.x/DENSITY*size.y/DENSITY*size.z/DENSITY)); */
	
	float x = (int)(pos.x*d+.4)/d;
	float y = (int)(pos.y*d+.4)/d;
	float z = (int)(pos.z*d+.4)/d;

	//float x = pos.x;
	//float y = pos.y;
	//float z = pos.z;

	return (int)((x*d+arraySize[0]*d*y*d + arraySize[0]*d*arraySize[1]*d*z*d));
	
	//data[static_cast<int>((x*d+size.x*d*y*d + size.x*d*size.y*d*z*d))] = val;


}
//TODO: Change from using me to ususing make parrell
__kernel void makeBox(__global float* out, __global float* x, //Center
					 __global float* dx,//Width off
					  __global float* arraySize,
					  __global float* sectionSize,
					  __global float* offset,
					  __global float* density) {
	unsigned int i = get_global_id(0);
	//Find centers
	//--------++++++++++++++++++-------------
	//        x                dx
	float4 v = getPos(i, sectionSize, offset, density);
	//int ii = i;
	int ii = getOut(v, arraySize, density);
	float vv = max( max(
				-dx[0]+fabs(v.x-x[0]),
				-dx[1]+fabs(v.y-x[1])
				),
				-dx[2]+fabs(v.z-x[2])
				);

	out[ii] = min(out[ii],vv);
	//out[ii] = i;
}


__kernel void makeParallel(__global float* out, __global float* pos, //Center
					 __global float* dx,//Width off
					 __global float* v1,
					 __global float* v2,
					 __global float* v3,
					  __global float* arraySize,
					  __global float* sectionSize,
					  __global float* offset,
					  __global float* density) {
	unsigned int i = get_global_id(0);

	float4 v = getPos(i, sectionSize, offset, density);
	int ii = getOut(v, arraySize, density);
	float vv = max( max(
		-dx[0]+fabs(-(v.x-pos[0])*v1[0]-(v.y-pos[1])*v1[1]-(v.z-pos[2])*v1[2]),
		-dx[1]+fabs(-(v.x-pos[0])*v2[0]-(v.y-pos[1])*v2[1]-(v.z-pos[2])*v2[2])
		),
		-dx[2]+fabs(-(v.x-pos[0])*v3[0]-(v.y-pos[1])*v3[1]-(v.z-pos[2])*v3[2])
	);

	out[ii] = min(out[ii],vv);
}


__kernel void subParallel(__global float* out, __global float* pos, //Center
					 __global float* dx,//Width off
					 __global float* v1,
					 __global float* v2,
					 __global float* v3,
					  __global float* arraySize,
					  __global float* sectionSize,
					  __global float* offset,
					  __global float* density) {
	unsigned int i = get_global_id(0);
	

	float4 v = getPos(i, sectionSize, offset, density);
	int ii = getOut(v, arraySize, density);
	float vv = max( max(
		-dx[0]+fabs(-(v.x-pos[0])*v1[0]-(v.y-pos[1])*v1[1]-(v.z-pos[2])*v1[2]),
		-dx[1]+fabs(-(v.x-pos[0])*v2[0]-(v.y-pos[1])*v2[1]-(v.z-pos[2])*v2[2])
		),
		-dx[2]+fabs(-(v.x-pos[0])*v3[0]-(v.y-pos[1])*v3[1]-(v.z-pos[2])*v3[2])
	);

	out[ii] = max(out[ii],-vv);
}

__kernel void addCylinder(__global float* out,
						  __global float* pos, //Vec3
						  __global float *normal, //Vec3
						  __global float* distance, //float
						  __global float* radius1, //float
						  __global float* radius2, //float
						  __global float* h, //float
						  __global float* arraySize, //vec3
						  __global float* sectionSize, //vec3
						  __global float* offset, //vec3
						  __global float* density //float
						  )
{
	unsigned int i = get_global_id(0);
	float4 v = getPos(i, sectionSize, offset, density);
	int ii = getOut(v, arraySize, density);
	//dot normal to v add dist
	float dist = normal[0]*v.x+normal[1]*v.y+normal[2]*v.z+distance[0];

	float val = max(
		-pow((radius1[0]*(.5+(dist)/(2*h[0]))+ (radius2[0]*(.5-(dist)/(2*h[0])))),2)
		-dist*dist+(v.x-pos[0])*(v.x-pos[0])+(v.y-pos[1])*(v.y-pos[1])+(v.z-pos[2])*(v.z-pos[2]),
		-h[0]*h[0]+dist*dist);

	out[ii] = min(out[ii],val);
}

__kernel void addSphere(__global float* out,
						  __global float* pos, //Vec3
						  __global float* radius,
						  __global float* arraySize, //vec3
						  __global float* sectionSize, //vec3
						  __global float* offset, //vec3
						  __global float* density //float
						  )
{
	unsigned int i = get_global_id(0);
	float4 v = getPos(i, sectionSize, offset, density);
	int ii = getOut(v, arraySize, density);


	float val = (pos[0]-v.x)*(pos[0]-v.x)+(pos[1]-v.y)*(pos[1]-v.y)+(pos[2]-v.z)*(pos[2]-v.z)-radius[0]*radius[0];

	out[ii] = min(out[ii],val);
}


__kernel void layerMax(__global float* out, __global float* bufferA, __global float* bufferB) {
	unsigned int i = get_global_id(0);


	out[i] = max(bufferA[i],bufferB[i]);
	//out[i] = 1;
}
