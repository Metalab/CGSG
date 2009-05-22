/*
 * RingBuffer.h
 *
 *  Created on: Mar 14, 2009
 *      Author: max
 */

#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

//#include <sdlvu.h>


class RingBuffer {

	public:
		RingBuffer( int, int );
		virtual ~RingBuffer();
		void write( float* );
		void write( float*, int );
		void read( float* );
		void read( float*, int);
	private:
		float *buffer;

		int pageSize;
		int pages;
		int size;

		int readPosition;
		int offsetReadPosition;
		int writePosition;
		int offsetWritePosition;
};

#endif /* RINGBUFFER_H_ */
