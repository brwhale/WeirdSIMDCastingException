// minimum size sample to demonstrate error

// issue: reinterpret_cast casting to a struct with a SIMD type inside of it causes a read acess violation
// Exception thrown: read access violation.
// buffer was 0xFFFFFFFFFFFFFFF7.

// run in release with optimizations and target x64, using Visual Studio 2017

#define USE_MEMCPY_WORKAROUND 0

#include <iostream>
#include <xmmintrin.h>
using std::cout;

class Vector3 {
	// just having this __m128 here is an issue. It doesn't matter if you use it or not
	// the original implementation where I discovered this used a union here (which is UB),
	// but the bug/weirdness doesn't seem to care either way
	__m128 mVec128;
	float m_floats[4];
public:
	Vector3(float x, float y, float z) {
		m_floats[0] = x;
		m_floats[1] = y;
		m_floats[2] = z;
		m_floats[3] = 0.f;
	}
	float x() { return m_floats[0]; }
	float y() { return m_floats[1]; }
	float z() { return m_floats[2]; }
	bool operator==(const Vector3& other) {
		return (m_floats[0] == other.m_floats[0] && m_floats[1] == other.m_floats[1] && m_floats[2] == other.m_floats[2]);
	}
	bool operator!=(const Vector3& other) {
		return (m_floats[0] != other.m_floats[0] || m_floats[1] != other.m_floats[1] || m_floats[2] != other.m_floats[2]);
	}
};

struct PackedData {
	uint64_t size = 0;
	Vector3 vector = Vector3(0,0,0);
};

int main() {
	// arbitrary buffer, represents a packet to de/serialize
	char* buffer = new char[10000];
	// set up values to pack
	uint64_t offset = sizeof(uint64_t);
	PackedData myData = { 123456789ull, Vector3(1,1,1) };
	
	// pack data into buffer
	*(reinterpret_cast<uint64_t*>(buffer)) = offset;
#if USE_MEMCPY_WORKAROUND
	memcpy(buffer + offset, &myData, sizeof(PackedData)); 
#else
	*(reinterpret_cast<PackedData*>(buffer + offset)) = myData; // exception here only in release
#endif


	// in real code, the buffer would get passed off to somewhere else to unpack
	// but for the purposes of this test it's unnecessary.

	// unpack data
	auto exOffset = *(reinterpret_cast<uint64_t*>(buffer));
#if USE_MEMCPY_WORKAROUND
	PackedData exData; memcpy(&exData, buffer + offset, sizeof(PackedData));
#else
	auto exData = *(reinterpret_cast<PackedData*>(buffer + offset)); // this would also throw
#endif
	
	// check integrity
	int exitCode = 0;
	if (offset != exOffset || myData.size != exData.size || myData.vector != exData.vector) {
		exitCode = 1;
	}

	cout << exitCode;
	return exitCode;
}