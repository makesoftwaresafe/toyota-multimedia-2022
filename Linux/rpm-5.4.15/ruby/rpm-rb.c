/*! 
 * \ingroup rb_c
 * \file rpm-rb.c
 * Ruby Bindings initialization file
 *
 * This file is the entry point for RPM's Ruby Bindings and contains the
 * infamous Init_rpm() function that starts every Ruby binding. It also
 * stores the ::rpmModule variable used for referencing the newly created
 * Ruby module, e.g. to add methods.
 */


#include "system.h"
#include "debug.h"

#include "rpm-rb.h"

#include "rpmts-rb.h"
#include "spec-rb.h"
#include "package-rb.h"
#include "rpmds-rb.h"
#include "rpmmc-rb.h"

#include <rpmrc.h>
#include <rpmcb.h>

#include <mire.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>


VALUE rpmModule;


void Init_rpm(void)
{
    if(rpmReadConfigFiles(NULL, NULL) != 0)
        rpm_rb_raise(1, "Reading config files failed");

    rpmModule = rb_define_module("RPM");

    Init_rpmts();
    Init_spec();
    Init_Package();
    Init_rpmmc();
    Init_rpmds();
}


void rpm_rb_raise(rpmRC error, char *message)
{
    rb_require("rpmexceptions");
    char *rb;
    int i = asprintf(&rb, "raise RPM::Error.new(%i), '%s'", error, message);
    if(i) rb_eval_string(rb);
}
