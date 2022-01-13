#include "CHotplugClient.h"
#include "projecttypes.h"

DLT_DECLARE_CONTEXT(raContext);

int main (int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	// instantiate the hotplug client
	CHotplugClient client;

	am::am_Source_s source;
	source.sourceID = 0;
	source.domainID = 1;
	source.name = "iPod";
	source.sourceClassID = 1;
	source.sourceState = am::SS_ON;
	source.volume = 80;
	source.visible = true;
	source.available.availability = am::A_AVAILABLE;
	source.available.availabilityReason = am::AR_UNKNOWN;
	source.interruptState = am::IS_OFF;
	source.listSoundProperties.clear();
	source.listConnectionFormats.push_back(am::CF_GENIVI_STEREO);
	source.listMainSoundProperties.clear();
	source.listMainNotificationConfigurations.clear();
	source.listNotificationConfigurations.clear();

	// call the interface
	client.asyncRegisterSource(source);

	// wait for ack
	sleep(3);

	return 0;
}
