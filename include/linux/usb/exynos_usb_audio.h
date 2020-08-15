#include <sound/samsung/abox.h>

/* for KM */

#define USB_AUDIO_MEM_BASE	0xC0000000

#define USB_AUDIO_SAVE_RESTORE	(USB_AUDIO_MEM_BASE)
#define USB_AUDIO_DEV_CTX	(USB_AUDIO_SAVE_RESTORE+PAGE_SIZE)
#define USB_AUDIO_INPUT_CTX	(USB_AUDIO_DEV_CTX+PAGE_SIZE)
#define USB_AUDIO_OUT_DEQ	(USB_AUDIO_INPUT_CTX+PAGE_SIZE)
#define USB_AUDIO_IN_DEQ	(USB_AUDIO_OUT_DEQ+PAGE_SIZE)
#define USB_AUDIO_FBOUT_DEQ	(USB_AUDIO_IN_DEQ+PAGE_SIZE)
#define USB_AUDIO_FBIN_DEQ	(USB_AUDIO_FBOUT_DEQ+PAGE_SIZE)
#define USB_AUDIO_ERST		(USB_AUDIO_FBIN_DEQ+PAGE_SIZE)
#define USB_AUDIO_DESC		(USB_AUDIO_ERST+PAGE_SIZE)
#define USB_AUDIO_PCM_OUTBUF	(USB_AUDIO_MEM_BASE+0x100000)
#define USB_AUDIO_PCM_INBUF	(USB_AUDIO_MEM_BASE+0x800000)

#define USB_AUDIO_XHCI_BASE	0x10c00000

#define USB_AUDIO_CONNECT		(1 << 0)
#define USB_AUDIO_REMOVING		(1 << 1)
#define USB_AUDIO_DISCONNECT		(1 << 2)
#define USB_AUDIO_TIMEOUT_PROBE	(1 << 3)

#define DISCONNECT_TIMEOUT	(500)

struct host_data {
	dma_addr_t out_data_dma;
	dma_addr_t in_data_dma;
	void *out_data_addr;
	void *in_data_addr;
};

extern struct host_data xhci_data;

struct exynos_usb_audio {
	struct usb_device *udev;
	struct platform_device *abox;
	struct platform_device *hcd_pdev;
	struct mutex    lock;
	struct work_struct usb_work;
	struct completion in_conn_stop;
	struct completion out_conn_stop;
	struct completion discon_done;

	u64 out_buf_addr;
	u64 in_buf_addr;
	u64 pcm_offset;
	u64 desc_addr;
	u64 offset;

	/* for hw_info */
	u64 dcbaa_dma;
	u64 in_ctx;
	u64 out_ctx;
	u64 erst_addr;

	int speed;
	/* 1: in, 0: out */
	int set_ep;
	int is_audio;
	int is_first_probe;
	u8 indeq_map_done;
	u8 outdeq_map_done;
	u8 fb_indeq_map_done;
	u8 fb_outdeq_map_done;
	u32 pcm_open_done;
	u32 usb_audio_state;

	void *pcm_buf;
	u64 save_dma;
};

extern struct exynos_usb_audio *usb_audio;
extern int otg_connection;

extern void exynos_usb_audio_work(struct work_struct *w);
int exynos_usb_audio_map_buf(struct usb_device *udev);
int exynos_usb_audio_pcmbuf(struct usb_device *udev);
int exynos_usb_audio_setrate(int intf, int rate, int alt);
int exynos_usb_audio_setintf(struct usb_device *udev, int iface, int alt, int direction);
int exynos_usb_audio_conn(int is_conn);
int exynos_usb_audio_pcm(int is_open, int direction);
int exynos_usb_audio_l2(int is_l2);
int exynos_usb_audio_desc(struct usb_device *udev);
int exynos_usb_audio_hcd(struct usb_device *udev);
int exynos_usb_audio_init(struct device *dev, struct platform_device *pdev);
int exynos_usb_audio_exit(void);
int exynos_usb_audio_unmap_all(void);
void exynos_usb_audio_set_device(struct usb_device *udev);
