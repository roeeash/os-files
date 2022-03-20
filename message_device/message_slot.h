#ifndef _MESSAGE_SLOT_H_
#define _MESSAGE_SLOT_H_

#include <linux/ioctl.h>

#define MAJOR_NUM 240
#define MSG_SLOT_MESSAGE_FILE _IOW(MAJOR_NUM, 0, unsigned int)
#define DEVICE_RANGE_NAME "message_slot"
#define BUFFER_LENGTH 128
#define SUCCESS 0
#define FAIL 1	


typedef struct message_file {
	long message_file_id;
	char message[BUFFER_LENGTH];
        int message_length;
	struct message_file *next;
} message_file;






typedef struct info {
	
	long message_file_id;
        int minor;
	message_file *current_message_file;
} info;











#endif