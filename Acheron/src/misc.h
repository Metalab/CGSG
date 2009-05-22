/*
 * misc.h
 *
 *  Created on: Apr 25, 2009
 *      Author: meta, cygen
 */

#ifndef MISC_H_
#define MISC_H_

#include <algorithm>
#include <map>

template<typename Tkey, typename Tvalue> struct deleteSecond : public std::unary_function<std::pair<Tkey,Tvalue*>, void>
{
	void operator() (std::pair<Tkey, Tvalue*> x) { delete x.second; }
};

void calcNV( float v1[], float v2[], float v3[], float* normalVector );

float calcDistance( float aX, float aY, float aZ, float bX, float bY, float bZ);



#endif /* MISC_H_ */
