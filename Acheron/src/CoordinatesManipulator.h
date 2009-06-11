/*
 * EigenWrapper.h
 *
 *  Created on: May 25, 2009
 *      Author: max
 */

#ifndef COORDINATESMANIPULATOR_H_
#define COORDINATESMANIPULATOR_H_

#include <Eigen/Core>
// import most common Eigen types
USING_PART_OF_NAMESPACE_EIGEN

class CoordinatesManipulator {

public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

	CoordinatesManipulator();
	void resetPosition();
	void setPosition();
//	void setPosition( float x, float y, float z );
//	void setPosition( float position[3] );
	void loadMatrix( float matrix[16] );
	void rotate( float angle, int x, int y, int z );
	void translate( float x, float y, float z );
	//float* getPosition();
	void getPosition( float* position );
	void setModelViewMatrix();

	void getMatrix( float* matrix );

	float* vertex3f( float x, float y, float z );

	void debug_ShowCalcMatrix();

private:
	Matrix4f calcM;
	Matrix4f modelViewMatrix;
};

#endif /* COORDINATESMANIPULATOR_H_ */
