/******************************************************************************
 * Project         Persistency
 * (c) copyright   2012
 * Company         XS Embedded GmbH
 *****************************************************************************/
/******************************************************************************
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a  copy of the MPL was not distributed
 * with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
******************************************************************************/
 /**
 * @file           persistence_health_monitor_test.c
 * @author         Ingo Huerner
 * @brief          Test of persistence health monitor
 * @see            
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>     /* exit */
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <dlt/dlt.h>
#include <dlt/dlt_common.h>

#include "persCheck.h"


void data_teardown(void)
{
   printf("* * * tear down * * *\n");	// nothing
}


START_TEST(test_SendNotification)
{
   X_TEST_REPORT_TEST_NAME("persistence_health_monitor_test");
   X_TEST_REPORT_COMP_NAME("libpersistence_health_monitor");
   X_TEST_REPORT_REFERENCE("NONE");
   X_TEST_REPORT_DESCRIPTION("Send notification to healt monitor");
   X_TEST_REPORT_TYPE(GOOD);



}
END_TEST




static Suite * persistencyClientLib_suite()
{
   Suite * s  = suite_create("Persistency client library");

   TCase * tc_SendNotification = tcase_create("SendNotification");
   tcase_add_test(tc_SendNotification, test_SendNotification);
   tcase_set_timeout(tc_SendNotification, 1);
   suite_add_tcase(s, tc_SendNotification);

   return s;
}


int main(int argc, char *argv[])
{
   int nr_failed = 0,
          nr_run = 0,
               i = 0;

   TestResult** tResult;

   /// debug log and trace (DLT) setup
   DLT_REGISTER_APP("PCLt","tests the persistence client library");

#if 1
   Suite * s = persistencyClientLib_suite();
   SRunner * sr = srunner_create(s);
   srunner_set_xml(sr, "/tmp/persistenceHealthMonitor.xml");
   srunner_set_log(sr, "/tmp/persistenceHealthMonitor.log");
   srunner_run_all(sr, CK_VERBOSE /*CK_NORMAL CK_VERBOSE*/);

   nr_failed = srunner_ntests_failed(sr);
   nr_run = srunner_ntests_run(sr);

   tResult = srunner_results(sr);
   for(i = 0; i< nr_run; i++)
   {
      (void)tr_rtype(tResult[i]);  // get status of each test
      //fail = tr_rtype(tResult[i]);  // get status of each test
      //printf("[%d] Fail: %d \n", i, fail);
   }

   srunner_free(sr);
#endif

   // unregister debug log and trace
   DLT_UNREGISTER_APP();

   dlt_free();

   return (0==nr_failed)?EXIT_SUCCESS:EXIT_FAILURE;

}

