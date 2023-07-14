#include "D3D.h"

#include <math.h>

// TODO: Try to make the near plane circle shaped omg.

D3D_OBJ D3D_cam;

static UCHAR* data;
static int w, h;

static inline void putpx(UCHAR c, int x, int y) {
	data[x+y*w] = c;
}

static inline void setvec(D3D_VEC v, float x, float y, float z) {
	v[0] = x;
	v[1] = y;
	v[2] = z;
}

float D3D_Q_rsqrt( float number ) {
	long i;
	float x2, y;
	const float threehalfs = 1.5F;

	x2 = number * 0.5F;
	y  = number;
	i  = * ( long * ) &y;                       // evil floating point bit level hacking
	i  = 0x5f3759df - ( i >> 1 );               // what the fuck?
	y  = * ( float * ) &i;
	y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
	// y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

	return y;
}

void D3D_init(UCHAR* _data, int width, int height) {
	data = _data;
	w = width;
	h = height;

	setvec(D3D_cam.offset, 0,0,0);
	setvec(D3D_cam.angles, 0,0,0);
	D3D_cam.cam.fov = D3D_PI/2;
	D3D_cam.cam.far = 100;
	D3D_cam.cam.near = 0.1f;
	D3D_cam.cam.exposure = 127;
	D3D_cam.cam.fog = 0;
}

// nearfactor should be set to w/h when using dst=x, dst=y, to avoid stretching.
// dst is not the distance of the point but the distance only on the x axis.
// height is how far it is from that ideal dst center point.
// Essentially D=sqrt(height^2+dst^2)
// Rasterizes a point on the near plane from 0 to 1 in a single dimention from a 2 dimentional reference.
// So you use it once to calculate the point from the dst=x height=y reference onto the x dimention in the near plane.
// and once the dst=x dst=z for the y dimention.
static float two_d_rasterize(float dst, float height, float nearfactor) {
	// If behind near plane ofc it wont work.
	if (dst < D3D_cam.cam.near)
		return -1;
	if (dst > D3D_cam.cam.far)
		return 2;

	// d is the location of the point on the near plane
	// d=0 is left, d=1 is right, d=0.5 is center
	// The point must not go outside the near plane, ofc, so we calculate the size of the plane.
	float maxd = tanf(D3D_cam.cam.fov/2) * D3D_cam.cam.near * nearfactor;

	// d/n = y/x
	float d = D3D_cam.cam.near * nearfactor * (height/dst);
	d /= 2*maxd;

	d += 0.5; // Shift it to be from left to right, ideally.

	return d;
}

// I tried to understand rotation for 2 days myself, failed unfortunately, so gonna use the average euler angles method.
static void D3D_rotate(D3D_VEC v, D3D_VEC angles) {
	float az = -angles[2];
	float ax = -angles[1];
	float ay = -angles[0];

	// Around z axis
	float tmpx = v[0], tmpy = v[1], tmpz;
	v[0] = tmpx * cosf(az) - tmpy * sinf(az);
	v[1] = tmpx * sinf(az) + tmpy * cosf(az);

	// Around y axis
	tmpx = v[0];
	tmpz = v[2];
	v[0] = tmpx * cosf(ay) + tmpz * sinf(ay);
	v[2] = -tmpx * sinf(ay) + tmpz * cosf(ay);

	// Around x axis
	tmpy = v[1];
	tmpz = v[2];
	v[1] = tmpy * cosf(ax) - tmpz * sinf(ax);
	v[2] = tmpy * sinf(ax) + tmpz * cosf(ax);
}

void D3D_rastervec(D3D_VEC in, D3D_VEC out) {
	D3D_vecsub(p, b, out);

	rotate(p, D3D_cam.angles[2], D3D_cam.angles[1], D3D_cam.angles[0]);

	float fx = two_d_rasterize(p[0], p[1], (1.0f*w)/h);

	float fy = two_d_rasterize(p[0], p[2], 1);

	p[2] = (p[0]-D3D_cam.cam.near)/(D3D_cam.cam.far-D3D_cam.cam.near);
	p[1] = fy;
	p[0] = fx;
}

void D3D_drawpoint(D3D_VEC vec) {
	D3D_VEC p = {vec[0], vec[1], vec[2]};
	D3D_rastervec(p);

	if (p[0] < .0f || p[0] > 1.0f)
		return;
	
	if (p[1] < .0f || p[1] > 1.0f)
		return;

	if (p[2] < .0f)
		return;

	putpx(1, p[0]*w, p[1]*h);
}
