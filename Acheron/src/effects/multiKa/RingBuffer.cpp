/*
 * RingBuffer.cpp
 *
 *  Created on: Mar 14, 2009
 *      Author: max
 */

#include "RingBuffer.h"

RingBuffer::RingBuffer(int pages, int pageSize) {
	// TODO Auto-generated constructor stub
	this->pages = pages;
	this->pageSize = pageSize;
	this->size = pages * pageSize;
	this->buffer = new float[size];
	this->writePosition = 0;
	this->readPosition = 0;

	for( int i=0; i<this->size; i++ ){
		this->buffer[i] = 0.0;
	}

}

RingBuffer::~RingBuffer() {
	// TODO Auto-generated destructor stub
}

void RingBuffer::write( float *input) {
	for( int i=0; i<pageSize; i++) {
		buffer[ writePosition*pageSize + i ] = input[i];
	}
	writePosition = (writePosition + 1) %pages;
	readPosition = ( readPosition +1 ) %pages;
}

void RingBuffer::write( float *input, int offset ) {
	offsetWritePosition = ( writePosition + offset + pages ) %pages;
	for( int i=0; i<pageSize; i++ ) {
		buffer[ offsetWritePosition*pageSize + i ] = input[i];
	}
}

void RingBuffer::read( float *output) {
	for( int i=0; i<pageSize; i++ ){
		output[ i ] = buffer[ readPosition*pageSize + i ];
	}
	readPosition = ( readPosition + 1 ) %pages;
}

void RingBuffer::read( float *output, int offset ) {
	offsetReadPosition = ( readPosition + offset + pages ) %pages;
	for( int i=0; i<pageSize; i++ ){
		output[ i ] = buffer[ offsetReadPosition*pageSize + i ];
	}
}
