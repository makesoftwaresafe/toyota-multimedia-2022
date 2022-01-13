#ifndef _QNX_LINUX_TRACEPOINT_H
#define _QNX_LINUX_TRACEPOINT_H

#define TP_PROTO(args...)	args
#define TP_ARGS(args...)	args
#define TP_CONDITION(args...)	args
#define PARAMS(args...) args

#define DECLARE_EVENT_CLASS(name, proto, args, tstruct, assign, print)
#define DEFINE_EVENT(template, name, proto, args)		\
	DECLARE_TRACE(name, PARAMS(proto), PARAMS(args))
#define DEFINE_EVENT_FN(template, name, proto, args, reg, unreg)\
	DECLARE_TRACE(name, PARAMS(proto), PARAMS(args))
#define DEFINE_EVENT_PRINT(template, name, proto, args, print)	\
	DECLARE_TRACE(name, PARAMS(proto), PARAMS(args))
#define DEFINE_EVENT_CONDITION(template, name, proto,		\
			       args, cond)			\
	DECLARE_TRACE_CONDITION(name, PARAMS(proto),		\
				PARAMS(args), PARAMS(cond))

#define TRACE_EVENT(name, proto, args, struct, assign, print)	\
	DECLARE_TRACE(name, PARAMS(proto), PARAMS(args))
#define TRACE_EVENT_FN(name, proto, args, struct,		\
		assign, print, reg, unreg)			\
	DECLARE_TRACE(name, PARAMS(proto), PARAMS(args))
#define TRACE_EVENT_CONDITION(name, proto, args, cond,		\
			      struct, assign, print)		\
	DECLARE_TRACE_CONDITION(name, PARAMS(proto),		\
				PARAMS(args), PARAMS(cond))

#define TRACE_EVENT_FLAGS(event, flag)



#define DECLARE_TRACE_CONDITION(name, proto, args, cond)		\
	__DECLARE_TRACE(name, PARAMS(proto), PARAMS(args), PARAMS(cond), \
			PARAMS(void *__data, proto),			\
			PARAMS(__data, args))



#define DECLARE_TRACE(name, proto, args)				\
		__DECLARE_TRACE(name, PARAMS(proto), PARAMS(args), 1,	\
				PARAMS(void *__data, proto),		\
				PARAMS(__data, args))

#define TRACE_EVENT(name, proto, args, struct, assign, print)	\
	DECLARE_TRACE(name, PARAMS(proto), PARAMS(args))


#ifdef CONFIG_TRACEPOINTS
/*
 * it_func[0] is never NULL because there is at least one element in the        array
 * when the array itself is non NULL.
 *
 * Note, the proto and args passed in includes "__data" as the first            parameter.
 * The reason for this is to handle the "void" prototype. If a tracepoint
 * has a "void" prototype, then it is invalid to declare a function
 * as "(void *, void)". The DECLARE_TRACE_NOARGS() will pass in just
 * "void *data", where as the DECLARE_TRACE() will pass in "void *data,         proto".
 */
#define __DO_TRACE(tp, proto, args, cond, prercu, postrcu)	\
	do {													\
        struct tracepoint_func *it_func_ptr;				\
        void *it_func;                      \
        void *__data;                       \
											\
		if (!(cond))                        \
            return;							\
        prercu;										\
        rcu_read_lock_sched_notrace();              \
        it_func_ptr = rcu_dereference_sched((tp)->funcs);   \
        if (it_func_ptr) {                  \
            do {                        \
                it_func = (it_func_ptr)->func;      \
                __data = (it_func_ptr)->data;       \
                ((void(*)(proto))(it_func))(args);  \
            } while ((++it_func_ptr)->func);        \
        }                           \
		rcu_read_unlock_sched_notrace();	\
        postrcu;							\
	} while (0)

struct static_key {
	atomic_t enabled;
};

static __always_inline bool static_key_false(struct static_key *key)
{
	if (unlikely(atomic_read(&key->enabled)) > 0)
		return true;
	return false;
}

#define __DECLARE_TRACE(name, proto, args, cond, data_proto, data_args) \
	extern struct tracepoint __tracepoint_##name;			\
	static inline void trace_##name(proto)				\
	{								\
		if (static_key_false(&__tracepoint_##name.key))		\
			__DO_TRACE(&__tracepoint_##name,		\
				TP_PROTO(data_proto),			\
				TP_ARGS(data_args),			\
				TP_CONDITION(cond),,);			\
	}								\
	__DECLARE_TRACE_RCU(name, PARAMS(proto), PARAMS(args),		\
		PARAMS(cond), PARAMS(data_proto), PARAMS(data_args))	\
	static inline int						\
	register_trace_##name(void (*probe)(data_proto), void *data)	\
	{								\
		return tracepoint_probe_register(#name, (void *)probe,	\
						 data);			\
	}								\
	static inline int						\
	unregister_trace_##name(void (*probe)(data_proto), void *data)	\
	{								\
		return tracepoint_probe_unregister(#name, (void *)probe, \
						   data);		\
	}								\
	static inline void						\
	check_trace_callback_type_##name(void (*cb)(data_proto))	\
	{								\
	}

#else

#define __DECLARE_TRACE(name, proto, args, cond, data_proto, data_args) \
	static inline void trace_##name(proto)				\
	{ }								\
	static inline void trace_##name##_rcuidle(proto)		\
	{ }								\
	static inline int						\
	register_trace_##name(void (*probe)(data_proto),		\
			      void *data)				\
	{								\
		return -ENOSYS;						\
	}								\
	static inline int						\
	unregister_trace_##name(void (*probe)(data_proto),		\
				void *data)				\
	{								\
		return -ENOSYS;						\
	}								\
	static inline void check_trace_callback_type_##name(void (*cb)(data_proto)) \
	{								\
	}

#endif /* CONFIG_TRACEPOINTS */

static inline void tracepoint_synchronize_unregister(void)
{
}

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/tracepoint.h $ $Rev: 837534 $")
#endif
