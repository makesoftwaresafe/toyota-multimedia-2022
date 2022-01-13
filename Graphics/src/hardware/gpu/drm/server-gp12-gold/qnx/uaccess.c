#include <linux/qnx.h>
#include <linux/linux.h>
#include <drm/drmP.h>
#include <linux/vaddr_cache.h>

int
memmgr_peer_sendnc(pid_t pid,
		int coid,
		void *smsg,
		size_t sbytes,
		void *rmsg,
		size_t rbytes)
{
	mem_peer_t  peer;
	iov_t       siov[2];

	peer.i.type = _MEM_PEER;
	peer.i.peer_msg_len = sizeof(peer);
	peer.i.pid = pid;

	SETIOV(siov + 0, &peer, sizeof peer);
	SETIOV(siov + 1, smsg, sbytes);
	return MsgSendvsnc( coid, siov, 2, rmsg, rbytes );
}

void *
_mmap2_peer(pid_t pid,
			void *addr,
			size_t len,
			int prot,
			int flags,
			int fd,
			off64_t off,
			unsigned align,
			unsigned pre_load,
			void **base,
			size_t *size)
{
	mem_map_t	msg;

	msg.i.type = _MEM_MAP;
	msg.i.zero = 0;
	msg.i.addr = (uintptr_t)addr;
	msg.i.len = len;
	msg.i.prot = prot;
	msg.i.flags = flags;
	msg.i.fd = fd;
	msg.i.offset = off;
	msg.i.align = align;
	msg.i.preload = pre_load;
	msg.i.reserved1 = 0;
	if (memmgr_peer_sendnc(pid, MEMMGR_COID, &msg.i, sizeof msg.i, &msg.o, sizeof msg.o) == -1) {
		qnx_error("memmgr_peer_sendnc(pid=%d(%d), tid=%d, len=%zd) failed!", pid, current->pid, pthread_self(), len);
		return MAP_FAILED;
	}
	if(base) {
		*base = (void *)(uintptr_t)msg.o.real_addr;
	}
	if(size) {
		*size = msg.o.real_size;
	}
	return (void *)(uintptr_t)msg.o.addr;
}

int
mem_offset64_peer(pid_t pid, const uintptr_t addr, size_t len, off64_t *offset, size_t *contig_len)
{
	struct _peer_mem_off {
		struct _mem_peer peer;
		struct _mem_offset msg;
	};
	typedef union {
		struct _peer_mem_off i;
		struct _mem_offset_reply o;
	} memoffset_peer_t;
	memoffset_peer_t msg;

	msg.i.peer.type = _MEM_PEER;
	msg.i.peer.peer_msg_len = sizeof(msg.i.peer);
	msg.i.peer.pid = pid;
	msg.i.peer.reserved1 = 0;

	msg.i.msg.type = _MEM_OFFSET;
	msg.i.msg.subtype = _MEM_OFFSET_PHYS;
	msg.i.msg.addr = addr;
	msg.i.msg.reserved = -1;
	msg.i.msg.len = len;

	if (MsgSendnc(MEMMGR_COID, &msg.i, sizeof msg.i, &msg.o, sizeof msg.o) == -1) {
		/* ESRCH means that process died during devctl execution. Since we */
		/* do not block clients anymore this happens sometime. */
		if (errno != ESRCH) {
			qnx_error("MsgSendnc(pid=%d, len=%zd) failed!", pid, len);
		}
		return -1;
	}
	*offset = msg.o.offset;
	*contig_len = msg.o.size;
	return(0);
}

#define DENSO_UADDR_TO_VADDR
#ifdef DENSO_UADDR_TO_VADDR
/*
 * DENSO copy from/to pid
 */
unsigned long
copy_from_pid(pid_t pid, void *to, const void __user *from, unsigned long n)
{
	struct task_struct *task;

	if(pid == getpid()){
		memcpy(to, from, n);
		return 0;
	}
	task = current;
	while (n) {
		struct uaddr uaddr = {
			.uaddr = from,
			.size = n,
		};
		int ret;

		ret = uaddr_to_vaddr(task->attachment.uaddr_cache_id,
				pid, &uaddr);
		if (ret) {
			qnx_error("uaddr_to_vaddr ret=%d", ret);
			break;
		}

		memcpy(to, uaddr.vaddr, uaddr.size);

		from += uaddr.size;
		to += uaddr.size;
		if (n >= uaddr.size) {
			n -= uaddr.size;
		} else {
			n = 0;
		}
	}
	return n;
}

unsigned long
copy_to_pid(pid_t pid, void __user *to, const void *from, unsigned long n)
{
	struct task_struct *task;

	if(pid == getpid()){
		memcpy(to, from, n);
		return 0;
	}
	task = current;
	while (n) {
		struct uaddr uaddr = {
			.uaddr = to,
			.size = n,
		};
		int ret;

		ret = uaddr_to_vaddr(task->attachment.uaddr_cache_id,
				pid, &uaddr);
		if (ret) {
			qnx_error("uaddr_to_vaddr ret=%d", ret);
			break;
		}

		memcpy(uaddr.vaddr, from, uaddr.size);

		from += uaddr.size;
		to += uaddr.size;
		if (n >= uaddr.size) {
			n -= uaddr.size;
		} else {
			n = 0;
		}
	}
	return n;
}
#else
unsigned long
copy_from_pid(pid_t pid, void *to, const void __user *from, unsigned long n)
{
	if(pid == getpid()){
		memcpy(to, from, n);
		return 0;
	}
	unsigned long remain = n;
	struct vaddr_list_t r;

	if(-1 == vaddr_cache_get_pages(pid, (void *)from, n, &r)){
		return remain;
	}

#ifdef DEBUG_UADDR
	static int debug=30;
	if (debug && (debug > 15 || n >= 16)) {
		struct uaddr uaddr = {
			.uaddr = (void*)from,
			.size = n,
		};
		int ret;

		debug--;
		ret = uaddr_to_vaddr( current->attachment.uaddr_cache_id,
				pid, &uaddr);
		printf("%s[%d] %s pid=%d uaddr_to_vaddr=%d uaddr=%#lx "
				"vaddr=%#lx size=%ld/%ld:%#016lx\n"
				"/vaddr_cache vaddr=%#lx %ld:%#016lx\n",
				__func__, pthread_self(),
				current->comm, pid, ret,
				(unsigned long)from,
				(unsigned long)uaddr.vaddr,
				n, uaddr.size,
				*(unsigned long*)uaddr.vaddr,

				(unsigned long)r.vaddrs[0] + r.off,
				remain,
				*(unsigned long*)(r.vaddrs[0] + r.off));
	}
#endif
	int off = r.off, page = 0;
	while(remain>0){
		void * vaddr = r.vaddrs[page++] + off;

		int size_limit = __PAGESIZE - off;
		int size = remain > size_limit ? size_limit : remain;
		memcpy(to, vaddr, size);

		to += size;
		remain -= size;
		off = 0;
	}
	free(r.vaddrs);

	return remain;
}

unsigned long
copy_to_pid(pid_t pid, void __user *to, const void *from, unsigned long n)
{
	if(pid == getpid()){
		memcpy(to, from, n);
		return 0;
	}
	unsigned long remain = n;
	struct vaddr_list_t r;

	if(-1 == vaddr_cache_get_pages(pid, to, n, &r)){
		return remain;
	}

	int off = r.off, page = 0;
	while(remain>0){
		void * vaddr = r.vaddrs[page++] + off;

		int size_limit = __PAGESIZE - off;
		int size = remain > size_limit ? size_limit : remain;
		/*TODO. check next vaddrs and do multi pages at once */
		memcpy(vaddr, from, size);

		from += size;
		remain -= size;
		off = 0;
	}
	free(r.vaddrs);
	return remain;
}
#endif

unsigned long
copy_from_user(void *to, const void __user *from, unsigned long n)
{
	if (current->attachment.copy_to_user_memcpy) {
		memcpy(to, from, n);
		return 0;
	}

	return copy_from_pid(current->pid, to, from, n);
}

unsigned long
copy_to_user(void __user *to, const void *from, unsigned long n)
{
	if (current->attachment.copy_to_user_memcpy) {
		memcpy(to, from, n);
		return 0;
	}

	return copy_to_pid(current->pid, to, from, n);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/uaccess.c $ $Rev: 856018 $")
#endif
