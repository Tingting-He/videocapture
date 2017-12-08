#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "string.h"
#include "video.h"
#include "threadcontrol.h"


int main(int argc,char **argv)
{
	VIDEO_RESOURCE vr;

	init_video_resource(&vr);
	printf("init_video_resource finish!\n");
	
}