#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/tcp.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>

#include "fimc-is-config.h"

#define INADDR_RNDIS	0xc0a82a81 /* 192.168.42.129 */
#define INADDR_ADB	INADDR_LOOPBACK

/*
 * callback sequence
 * connecting: on_accept -> on_connect -> on_open
 * disconnecting: on_close
 */

#define ISC_CONNECTED	1
struct is_sock_con {
	struct list_head list;
	struct socket *sock;
	unsigned long flags;
	struct is_sock_server *server;
	void *priv;

	/* IS socket connection ops */
	int (*on_open)(struct is_sock_con *con);
	void (*on_close)(struct is_sock_con *con);
};

#define IS_SOCK_SERVER_NAME_LEN	32
struct is_sock_server {
	char name[IS_SOCK_SERVER_NAME_LEN];
	int type;
	unsigned int ip;
	int port;
	bool blocking_accept;
	void *priv;

	struct socket *sock;
	struct task_struct *thread;
	bool stop;
	atomic_t num_of_cons;
	struct list_head cons;
	spinlock_t slock;

	/* IS socket sever ops */
	int (*on_accept)(struct socket *newsock);
	int (*on_connect)(struct is_sock_con *con);
};

/* IS socket functions */
static int is_sock_create(struct socket **sockp, int type, unsigned int ip, int port)
{
	struct sockaddr_in addr;
	struct socket *sock;
	int opt;
	int ret;

	ret = sock_create(PF_INET, type, 0, &sock);
	*sockp = sock;
	if (ret) {
		err("failed to create socket: %d", ret);
		return ret;
	}

	/* TODO: SO_KEEPALIVE, SO_LINGER */
	opt = 1;
	ret = kernel_setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
			       (char *)&opt, sizeof(opt));
	if (ret) {
		err("failed to set SO_REUSEADDR for socket: %d", ret);
		goto err_so_reuseaddr;
	}

	if (type == SOCK_STREAM) {
		opt = 1;
		ret = kernel_setsockopt(sock, SOL_TCP, TCP_NODELAY,
				(char *)&opt, sizeof(opt));
		if (ret) {
			err("failed to set TCP_NODELAY for socket: %d", ret);
			goto err_tcp_nodelay;
		}

		opt = 0;
		ret = kernel_setsockopt(sock, SOL_TCP, TCP_CORK,
				(char *)&opt, sizeof(opt));
		if (ret) {
			err("failed to unset TCP_CORK for socket: %d", ret);
			goto err_tcp_cork;
		}
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = (ip) ? htonl(ip) : htonl(INADDR_ANY);

	ret = kernel_bind(sock, (struct sockaddr *)&addr,
			sizeof(addr));

	if (ret == -EADDRINUSE) {
		err("port %d already in use", port);
		goto err_addr_used;
	}

	if (ret) {
		err("error trying to bind to port %d: %d",
				port, ret);
		goto err_bind;
	}

	return 0;

err_bind:
err_addr_used:
err_so_reuseaddr:
err_tcp_nodelay:
err_tcp_cork:
	sock_release(sock);

	return ret;
}

static int is_sock_setbuf_size(struct socket *sock, int txsize, int rxsize)
{
	int opt;
	int ret;

	if (txsize) {
		opt = txsize;
		ret = kernel_setsockopt(sock, SOL_SOCKET, SO_SNDBUF,
				       (char *)&opt, sizeof(opt));
		if (ret) {
			err("failed to set send buffer size: %d", ret);
			return ret;
		}
	}

	if (rxsize) {
		opt = rxsize;
		ret = kernel_setsockopt(sock, SOL_SOCKET, SO_RCVBUF,
				      (char *)&opt, sizeof(opt));
		if (ret) {
			err("failed to set receive buffer size: %d", ret);
			return ret;
		}
	}

	return 0;
}

static int is_sock_create_listen(struct socket **sockp, int type, unsigned int ip, int port)
{
	int ret;

	ret = is_sock_create(sockp, type, ip, port);
	if (ret) {
		err("can't create socket");
		return ret;
	}

	ret = kernel_listen(*sockp, 5);
	if (ret) {
		err("failed to listen socket: %d", ret);
		sock_release(*sockp);
	}

	return 0;
}

static int is_sock_send_stream(struct socket *sock, void *buf, int len, int timeout)
{
	int ret;
	long ticks = timeout * HZ;
	unsigned long then;
	struct timeval tv;

	while (1) {
		struct kvec iov = {
			.iov_base = buf,
			.iov_len = len,
		};

		struct msghdr msg = {
			.msg_flags = (timeout == 0) ? MSG_DONTWAIT : 0,
		};

		if (timeout) {
			tv = (struct timeval) {
				.tv_sec = ticks / HZ,
				.tv_usec = ((ticks % HZ) * 1000000) / HZ
			};

			ret = kernel_setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO,
					       (char *)&tv, sizeof(tv));
			if (ret) {
				err("failed to set SO_SNDTIMEO(%ld.%06d) for socket: %d",
						(long)tv.tv_sec, (int)tv.tv_usec, ret);
				return ret;
			}
		}

		then = jiffies;
		ret = kernel_sendmsg(sock, &msg, &iov, 1, len);
		if (timeout)
			ticks -= jiffies - then;

		if (ret == len)
			return 0;

		if (ret < 0) {
			if (ret == -EAGAIN)
				continue;
			else
				return ret;
		}

		if (ret == 0) {
			err("Software caused connection abort");
			return -ECONNABORTED;
		}

		if (timeout && (ticks <= 0))
			return -EAGAIN;

		buf = ((char *)buf) + ret;
		len -= ret;
	}

	return 0;
}

static int is_sock_recv_stream(struct socket *sock, void *buf, int len, int timeout)
{
	int ret;
	long ticks = timeout * HZ;
	unsigned long then;
	struct timeval tv;

	while (1) {
		struct kvec iov = {
			.iov_base = buf,
			.iov_len = len,
		};

		struct msghdr msg = {
			.msg_flags = 0,
		};

		if (timeout) {
			tv = (struct timeval) {
				.tv_sec = ticks / HZ,
				.tv_usec = ((ticks % HZ) * 1000000) / HZ
			};

			ret = kernel_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
					(char *)&tv, sizeof(tv));
			if (ret) {
				err("failed to set SO_RCVTIMEO(%ld.%06d) for socket: %d",
						(long)tv.tv_sec, (int)tv.tv_usec, ret);
				return ret;
			}
		}

		then = jiffies;
		ret = kernel_recvmsg(sock, &msg, &iov, 1, len, 0);
		if (timeout)
			ticks -= jiffies - then;

		if (ret < 0) {
			if (ret == -EAGAIN)
				continue;
			else
				return ret;
		}

		if (ret == 0) {
			err("Connection reset by peer");
			return -ECONNRESET;
		}

		buf = ((char *)buf) + ret;
		len -= ret;

		if (len == 0)
			return 0;

		if (timeout && (ticks <= 0))
			return -ETIMEDOUT;
	}
}

/* IS socket connection functions */
static int is_sock_con_open(struct is_sock_con **newconp, struct socket *newsock,
		struct is_sock_server *svr)
{
	struct is_sock_con *newcon;
	int ret;

	newcon = kzalloc(sizeof(*newcon), GFP_KERNEL);
	if (!newcon) {
		err("failed to alloc new connection for %s", svr->name);
		return -ENOMEM;
	}

	newcon->sock = newsock;
	newcon->server = svr;

	if (svr->on_connect) {
		ret = svr->on_connect(newcon);
		if (ret) {
			err("connect callback error: %d", ret);
			kfree(newcon);
			return ret;
		}
	}

	spin_lock(&svr->slock);
	atomic_inc(&svr->num_of_cons);
	list_add_tail(&newcon->list, &svr->cons);
	spin_unlock(&svr->slock);

	if (newcon->on_open) {
		ret = newcon->on_open(newcon);
		if (ret) {
			err("open callback error: %d", ret);
			spin_lock(&svr->slock);
			atomic_dec(&svr->num_of_cons);
			list_del(&newcon->list);
			spin_unlock(&svr->slock);
			kfree(newcon);
			return ret;
		}
	}

	set_bit(ISC_CONNECTED, &newcon->flags);

	*newconp = newcon;

	return 0;
}

static void is_sock_con_close(struct is_sock_con *con)
{
	if (con && test_and_clear_bit(ISC_CONNECTED, &con->flags)) {
		if (con->on_close)
			con->on_close(con);

		kernel_sock_shutdown(con->sock, SHUT_RDWR);
		sock_release(con->sock);

		spin_lock(&con->server->slock);
		atomic_dec(&con->server->num_of_cons);
		list_del(&con->list);
		spin_unlock(&con->server->slock);

		kfree(con);
	}
}

static int is_sock_con_send_stream(struct is_sock_con *con, void *buf, int len, int timeout)
{
	int ret;

	if (!test_bit(ISC_CONNECTED, &con->flags))
		return -ENOTCONN;

	ret = is_sock_send_stream(con->sock, buf, len, timeout);
	if (ret) {
		err("send stream error: %d", ret);

		if (ret == -EWOULDBLOCK || ret == -EAGAIN)
			return ret;

		is_sock_con_close(con);
		return ret;
	}

	return ret;
}

static int is_sock_con_recv_stream(struct is_sock_con *con, void *buf, int len, int timeout)
{
	int ret;

	if (!test_bit(ISC_CONNECTED, &con->flags))
		return -ENOTCONN;

	ret = is_sock_recv_stream(con->sock, buf, len, timeout);
	if (ret) {
		err("recv stream error: %d", ret);

		if (ret == -EWOULDBLOCK || ret == -ETIMEDOUT)
			return ret;

		is_sock_con_close(con);
		return ret;
	}

	return ret;
}

/* IS socket server functions */
static int is_sock_server_accept(struct is_sock_server *svr)
{
	struct socket *sock = svr->sock;
	struct socket *newsock;
	struct is_sock_con *newcon;
	int ret;

	ret = kernel_accept(sock, &newsock, svr->blocking_accept ? 0 : O_NONBLOCK);
	if (ret < 0)
		return ret;

	if (svr->on_accept) {
		ret = svr->on_accept(newsock);
		if (ret) {
			err("accept callback error: %d", ret);
			sock_release(newsock);
			return ret;
		}
	}

	ret = is_sock_con_open(&newcon, newsock, svr);
	if (ret) {
		err("failed to open new connection for %s", svr->name);
		sock_release(newsock);
		return ret;
	}

	return ret;
}

static int is_sock_server_thread(void *data)
{
	struct is_sock_server *svr = (struct is_sock_server *)data;
	int ret;
	long timeo;

	ret = is_sock_create_listen(&svr->sock, svr->type, svr->ip, svr->port);
	if (ret)
		return ret;

	timeo = sock_rcvtimeo(svr->sock->sk, O_NONBLOCK);

	/* set_freezable(); */

	while (!svr->stop) {
		/* try_to_freeze(); */

		ret = is_sock_server_accept(svr);
		if (ret == -EAGAIN) {
			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout(timeo);
			set_current_state(TASK_RUNNING);
		} else if (ret) {
			err("sever accept error: %d", ret);
		}
	}

	kernel_sock_shutdown(svr->sock, SHUT_RDWR);
	sock_release(svr->sock);

	while (!kthread_should_stop()) {
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();
	}

	return 0;
}

static int is_sock_server_start(struct is_sock_server *svr)
{
	int ret = 0;

	svr->thread = kthread_run(is_sock_server_thread,
			(void *)svr, "sock-%s", svr->name);

	if (IS_ERR(svr->thread)) {
		err("failed to start socket server thread for %s", svr->name);
		ret = PTR_ERR(svr->thread);
		svr->thread = NULL;
	}

	atomic_set(&svr->num_of_cons, 0);
	INIT_LIST_HEAD(&svr->cons);
	spin_lock_init(&svr->slock);

	return ret;
}

static int is_sock_server_stop(struct is_sock_server *svr)
{
	struct is_sock_con *con, *temp;
	int ret;

	if (!svr)
		return -EINVAL;

	svr->stop = true;
	if (svr->thread)
		ret = kthread_stop(svr->thread);
	svr->thread = NULL;

	list_for_each_entry_safe(con, temp, &svr->cons, list)
		is_sock_con_close(con);

	return 0;
}

/* for tuning interface */
#define TUNING_PORT	5672
#define TUNING_BUF_SIZE SZ_512K

#define TUNING_CONTROL	0
#define TUNING_JSON	1
#define TUNING_IMAGE	2
#define TUNING_MAX	3

struct is_sock_server *tuning_svr;

struct is_tuning_con {
	char read_buf[TUNING_BUF_SIZE];
	char write_buf[TUNING_BUF_SIZE];
	struct file *filp;
};

struct is_tuning_svr {
	struct mutex mlock;
	atomic_t num_of_filp;
	struct file *filp[TUNING_MAX];
};

int is_tuning_on_open(struct is_sock_con *con)
{
	struct is_tuning_con *itc;
	struct is_tuning_svr *its = (struct is_tuning_svr *)tuning_svr->priv;
	unsigned int cur_con_idx;
	int ret;

	itc = vzalloc(sizeof(*itc));
	if (!itc) {
		err("failed to alloc connection private data");
		return -ENOMEM;
	}
	con->priv = (void *)itc;

	ret = is_sock_setbuf_size(con->sock, TUNING_BUF_SIZE, TUNING_BUF_SIZE);
	if (ret) {
		err("failed to set buf size for %p to %d", con, TUNING_BUF_SIZE);
		goto err_set_buf_size;
	}

	/* a number of connection includes new connecting socket */
	cur_con_idx = atomic_read(&con->server->num_of_cons) - 1;

	if (cur_con_idx >= TUNING_MAX) {
		err("a number of con. is exceed: %d", TUNING_MAX);
		ret = -EMFILE;
		goto err_too_many_con;
	}

	if (its->filp[cur_con_idx]) {
		itc->filp = its->filp[cur_con_idx];
		itc->filp->private_data = con;
	} else {
		err("there is no valid opened file for %d", cur_con_idx);
		ret = -ENOENT;
		goto err_invalid_filp;
	}

	return 0;

err_invalid_filp:
err_too_many_con:
err_set_buf_size:
	vfree(itc);

	return ret;
}

void is_tuning_on_close(struct is_sock_con *con)
{
	struct is_tuning_con *itc;

	if (con) {
		itc = (struct is_tuning_con *)con->priv;
		itc->filp->private_data = NULL;
		vfree(itc);
	}
}

int is_tuning_on_accept(struct socket *newsock)
{
	return 0;
}

int is_tuning_on_connect(struct is_sock_con *newcon)
{
	newcon->on_open = is_tuning_on_open;
	newcon->on_close = is_tuning_on_close;

	return 0;
}

int is_tuning_start(void)
{
	struct is_tuning_svr *its;
	int ret;

	tuning_svr = kzalloc(sizeof(*tuning_svr), GFP_KERNEL);
	if (!tuning_svr) {
		err("failed to alloc tuning server");
		return -ENOMEM;
	}

	strncpy(tuning_svr->name, "tuning", sizeof(tuning_svr->name) - 1);
	tuning_svr->type = SOCK_STREAM;
	tuning_svr->ip = INADDR_ADB;
	tuning_svr->port = TUNING_PORT;
	tuning_svr->blocking_accept = false;

	tuning_svr->on_accept = is_tuning_on_accept;
	tuning_svr->on_connect = is_tuning_on_connect;

	its = kzalloc(sizeof(*its), GFP_KERNEL);
	if (!its) {
		err("failed to alloc tuning server private data");
		kfree(tuning_svr);
		return -ENOMEM;
	}
	tuning_svr->priv = (void *)(its);
	mutex_init(&its->mlock);
	atomic_set(&its->num_of_filp, 0);

	ret = is_sock_server_start(tuning_svr);
	if (ret) {
		err("failed to start socket server for %s", tuning_svr->name);
		kfree(its);
		kfree(tuning_svr);
		tuning_svr = NULL;
	}

	return ret;
}

void is_tuning_stop(void *data)
{
	if (tuning_svr) {
		is_sock_server_stop(tuning_svr);
		kfree(tuning_svr->priv);
		kfree(tuning_svr);
		tuning_svr = NULL;
	}
}

static ssize_t is_tuning_read(struct file *filp, char __user *buf,
		size_t len, loff_t *ppos)
{
	struct is_sock_con *con = (struct is_sock_con *)filp->private_data;
	struct is_sock_con *other_con, *temp;
	struct is_tuning_con *itc;
	struct is_sock_server *server;
	int ret;

	if (!con)
		return -EINVAL;

	if (!test_bit(ISC_CONNECTED, &con->flags))
		return -ENOTCONN;

	itc = (struct is_tuning_con *)con->priv;
	server = con->server;

	if (len > TUNING_BUF_SIZE)
		len = TUNING_BUF_SIZE;

	ret = is_sock_con_recv_stream(con, itc->read_buf, len, 0);
	if (ret) {
		/*
		 * We can realize that the client was diconnected with this error.
		 * Any connected sockets which are waiting for client to receivce
		 * messages could get this error due to client's disconnection.
		 * So, we need to reset all connected sockets to restart.
		 */
		if (ret == -ECONNRESET) {
			list_for_each_entry_safe(other_con, temp,
					&server->cons, list)
				is_sock_con_close(other_con);
		}

		return ret;
	}

	ret = copy_to_user(buf, itc->read_buf, len);
	if (ret) {
		err("failed to copy to user: %d", ret);
		return -EPERM;
	}

	return len;
}

static ssize_t is_tuning_write(struct file *filp, const char __user *buf,
		size_t len, loff_t *ppos)
{
	struct is_sock_con *con = (struct is_sock_con *)filp->private_data;
	struct is_tuning_con *itc;
	int ret;

	if (!con)
		return -EINVAL;

	if (!test_bit(ISC_CONNECTED, &con->flags))
		return -ENOTCONN;

	itc = (struct is_tuning_con *)con->priv;

	if (len > TUNING_BUF_SIZE)
		len = TUNING_BUF_SIZE;

	ret = copy_from_user(itc->write_buf, buf, len);
	if (ret) {
		err("failed to copy from user: %d", ret);
		return -EPERM;
	}

	ret = is_sock_con_send_stream(con, itc->write_buf, len, 0);
	if (ret)
		return ret;

	return len;
}

static int is_tuning_open(struct inode *inode, struct file *filp)
{
	struct is_tuning_svr *its;
	unsigned int num_of_filp;
	int ret;

	if (!tuning_svr) {
		ret = is_tuning_start();
		if (ret) {
			err("failed to start tuning sever: %d", ret);
			return ret;
		}
	}

	its = (struct is_tuning_svr *)tuning_svr->priv;

	mutex_lock(&its->mlock);

	num_of_filp = atomic_read(&its->num_of_filp);
	if (num_of_filp >= TUNING_MAX) {
		mutex_unlock(&its->mlock);
		return -EINVAL;
	}

	its->filp[num_of_filp] = filp;
	atomic_inc(&its->num_of_filp);

	mutex_unlock(&its->mlock);

	filp->private_data = NULL;

	return 0;
}

static int is_tuning_close(struct inode *inodep, struct file *filp)
{
	struct is_sock_con *con = (struct is_sock_con *)filp->private_data;
	struct is_tuning_svr *its = (struct is_tuning_svr *)tuning_svr->priv;

	if (con)
		is_sock_con_close(con);

	if (!atomic_dec_return(&its->num_of_filp))
		is_tuning_stop(NULL);

	return 0;
}

static const struct file_operations is_tuning_fops = {
	.owner		= THIS_MODULE,
	.read		= is_tuning_read,
	.write		= is_tuning_write,
	.open		= is_tuning_open,
	.release	= is_tuning_close,
	.llseek		= no_llseek,
};

struct miscdevice is_tuning_device = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "is_tuning_socket",
	.fops	= &is_tuning_fops,
};

static int __init is_sock_init(void)
{
	int ret;

	/* tuning misc device */
	ret = misc_register(&is_tuning_device);
	if (ret)
		err("failed to register is_tuning_socket devices");

	return ret;
}

static void __exit is_sock_exit(void)
{
	/* tuning misc device */
	misc_deregister(&is_tuning_device);
}

/* init and cleanup functions */
module_init(is_sock_init);
module_exit(is_sock_exit);

MODULE_AUTHOR("Jeongtae Park <jtp.park@samsung.net>");
MODULE_DESCRIPTION("socket layer for image subsystem in Exynos");
MODULE_LICENSE("GPL");
