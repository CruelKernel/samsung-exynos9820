
#ifndef _LINUX_DMA_BUF_CONTAINER_H_
#define _LINUX_DMA_BUF_CONTAINER_H_

#ifdef CONFIG_DMA_BUF_CONTAINER

int dmabuf_container_get_count(struct dma_buf *dmabuf);
struct dma_buf *dmabuf_container_get_buffer(struct dma_buf *dmabuf, int index);
int dmabuf_container_set_mask(struct dma_buf *dmabuf, u32 mask);
int dmabuf_container_get_mask(struct dma_buf *dmabuf, u32 *mask);

#else

static inline int dmabuf_container_get_count(struct dma_buf *dmabuf)
{
	return -EINVAL;
}

static inline struct dma_buf *dmabuf_container_get_buffer(struct dma_buf *dbuf,
							  int index)
{
	return NULL;
}

static inline int dmabuf_container_set_mask(struct dma_buf *dmabuf, u32 mask)
{
	return -EINVAL;
}

static inline int dmabuf_container_get_mask(struct dma_buf *dmabuf, u32 *mask)
{
	return -EINVAL;
}

#endif

#endif /* _LINUX_DMA_BUF_CONTAINER_H_ */
