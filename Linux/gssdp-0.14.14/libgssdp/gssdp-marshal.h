
#ifndef __gssdp_marshal_MARSHAL_H__
#define __gssdp_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:STRING,UINT,INT,BOXED (./gssdp-marshal.list:1) */
extern void gssdp_marshal_VOID__STRING_UINT_INT_BOXED (GClosure     *closure,
                                                       GValue       *return_value,
                                                       guint         n_param_values,
                                                       const GValue *param_values,
                                                       gpointer      invocation_hint,
                                                       gpointer      marshal_data);

/* VOID:STRING,POINTER (./gssdp-marshal.list:2) */
extern void gssdp_marshal_VOID__STRING_POINTER (GClosure     *closure,
                                                GValue       *return_value,
                                                guint         n_param_values,
                                                const GValue *param_values,
                                                gpointer      invocation_hint,
                                                gpointer      marshal_data);

/* VOID:STRING (./gssdp-marshal.list:3) */
#define gssdp_marshal_VOID__STRING	g_cclosure_marshal_VOID__STRING

G_END_DECLS

#endif /* __gssdp_marshal_MARSHAL_H__ */

