/**
 * Tiny FIFO implemenation - simplest FIFO only for handling 1-dimensional
 * single unstructured unsigned integer data array in circular manner.
 **/

#include <linux/slab.h>

struct __tfifo {
	unsigned int	*data;
	unsigned int	length;
	unsigned int	in;
	unsigned int	out;
}; 

/**
 * For now, we only consider of 'unsigned integer' use case.
 **/
#define __entity_size	(sizeof(unsigned int))

#define tfifo_init(fifo)						\
({									\
	typeof((fifo) + 1) __tmp = (fifo);				\
	*__tmp = (struct __tfifo) {					\
		.data	= NULL,						\
		.length	= 0,						\
		.in	= 0,						\
		.out	= 0 };						\
})
	
#define tfifo_alloc(fifo, len)						\
({									\
	bool __ret = true;						\
	typeof((fifo) + 1) __tmp = (fifo);				\
	__tmp->data = kmalloc(__entity_size * (len + 1), GFP_KERNEL);	\
	if (__tmp->data)						\
		__tmp->length = (len + 1);				\
	else								\
		__ret = false;						\
	__ret;								\
})

#define is_tfifo_empty(fifo)						\
({									\
	typeof((fifo) + 1) __tmp = (fifo);				\
	bool __ret = (__tmp->in == __tmp->out) ? true : false;		\
	__ret;								\
})
	
#define tfifo_free(fifo)						\
({									\
	typeof((fifo) + 1) __tmp = (fifo);				\
	if (__tmp->data)	kfree(__tmp->data);			\
	__tmp->length	= 0;						\
	__tmp->in	= 0;						\
	__tmp->out	= 0;						\
})

#define tfifo_in(fifo, val)						\
({									\
	typeof((fifo) + 1) __tmp = (fifo);				\
	__tmp->data[__tmp->in] = (val);					\
	__tmp->in = (__tmp->in + 1) % __tmp->length;			\
})

#define tfifo_out(fifo)							\
({									\
	unsigned int __ret;						\
	typeof((fifo) + 1) __tmp = (fifo);				\
	__ret = __tmp->data[__tmp->out];				\
	__tmp->out = (__tmp->out + 1) % __tmp->length;			\
	__ret;								\
})
