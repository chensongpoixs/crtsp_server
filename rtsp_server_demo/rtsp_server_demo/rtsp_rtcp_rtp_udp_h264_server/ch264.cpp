
/***********************************************************************************************
created: 		2023-12-06

author:			chensong

purpose:		rtsp

************************************************************************************************/
#define _CRT_SECURE_NO_WARNINGS
#include "ch264.h"


static inline int startCode3(unsigned char* buf)
{
	if (buf[0] == 0 && buf[1] == 0 && buf[2] == 1)
		return 1;
	else
		return 0;
}

static inline int startCode4(unsigned char* buf)
{
	if (buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 1)
		return 1;
	else
		return 0;
}

static unsigned char* findNextStartCode(unsigned char* buf, int len)
{
	int i;

	if (len < 3)
		return NULL;

	for (i = 0; i < len - 3; ++i)
	{
		if (startCode3(buf) || startCode4(buf))
			return buf;

		++buf;
	}

	if (startCode3(buf))
		return buf;

	return NULL;
}

static int getFrameFromH264File(FILE* fp, unsigned char* frame, int size) {
	int rSize, frameSize;
	unsigned char* nextStartCode;

	if (fp < 0)
		return -1;

	rSize = fread(frame, 1, size, fp);

	if (!startCode3(frame) && !startCode4(frame))
		return -1;

	nextStartCode = findNextStartCode(frame + 3, rSize - 3);
	if (!nextStartCode)
	{
		//lseek(fd, 0, SEEK_SET);
		//frameSize = rSize;
		return -1;
	}
	else
	{
		frameSize = (nextStartCode - frame);
		fseek(fp, frameSize - rSize, SEEK_CUR);

	}

	return frameSize;
}


int32_t parse_h264_file_sps_pps(const char* file_name, std::string&  sps, std::string&  pps)
{

	FILE* read_file_ptr = ::fopen(file_name, "rb");
	if (!read_file_ptr)
	{
		printf("[%s][%d]not open file name = %s\n", __FUNCTION__, __LINE__, file_name);
		return -1;
	}
	unsigned char* read_buffer = (unsigned char*) malloc(sizeof(unsigned char) * 50000);
	memset(read_buffer, 0, sizeof(read_buffer));
	char* sps_ = NULL;
	char* pps_ = NULL;
	while (!sps_ || !pps_)
	{
		int32_t read_size = getFrameFromH264File(read_file_ptr, read_buffer, 2048);
		if (read_size <= 0)
		{
			break;
		}
		int nal_length = 4;
		if (startCode3(read_buffer))
		{
			nal_length = 3;
		}

		// 找到sps和pps的数据
		int nal_unit_type = (read_buffer + nal_length)[0] & 0X1F;
		if (nal_unit_type == 6 /*SEI*/)
		{
		}
		else if (nal_unit_type == 7 /*SPS*/)
		{
			sps.resize(read_size - nal_length);
			memcpy((char *)sps.data(), read_buffer + nal_length, read_size - nal_length);
			sps_ = (char*)sps.data();
			if (pps_)
			{
				break;
			}
		}
		else if (nal_unit_type == 8 /*PPS*/)
		{
			pps.resize(read_size - nal_length);
			memcpy((char*)pps.data(), read_buffer + nal_length, read_size -nal_length);
			pps_ = (char*)pps.data();
			if (sps_)
			{
				break;
			}
		}


	}
	if (read_buffer)
	{
		free(read_buffer);
		read_buffer = NULL;
	}
	if (read_file_ptr)
	{
		fclose(read_file_ptr);
		read_file_ptr = NULL;
	}

	return 0;
}
