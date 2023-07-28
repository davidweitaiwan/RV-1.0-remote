#pragma once

#include <iostream>
#include <assert.h>
#include <time.h>
#include <string.h>

#include <sstream>
#include "MD5ChecksumDefines.h"

//Boost
#include <boost/thread/thread.hpp>
#include <boost/array.hpp>
#include <boost/asio.hpp>

#include <mutex>
#include <vector>
#include <chrono>

typedef long long LLONG;

enum class SocketState { Connected, Disconnect };

using boost::asio::ip::tcp;

#define PACKET_HEADER_SIZE	22
#define PACKET_PAYLOAD_SIZE	1450
#define PACKET_SIZE			(PACKET_HEADER_SIZE + PACKET_PAYLOAD_SIZE)

#define SPLIT_DATA_CNT 7

unsigned char header;
void RecvThread(void* dummy);

class NetworkClient
{
private:
	//Modified
	int deviceCnt = 0;
	std::vector<std::string> deviceVec;
	bool deviceVecF = false;
	std::mutex deviceVec_lock;
	std::mutex splitData_lock;
	std::mutex connState_lock;
	std::vector<int> splitData;
	std::string recvStr;
	SocketState socState;
	std::string remoteDevName = "NULL";
	bool remoteF = false;

	std::string timestamp;

	//Package counter
	LLONG transTime = 0;

	tcp::resolver resolver_;
	tcp::socket socket_;

	std::string ipaddress_;
	int port_;
	unsigned short id_;

	unsigned int last_packet_num_;
	unsigned char* buffer_;

	unsigned char response_;
	int dhcp_waiting_time_;
	unsigned int max_file_;
	unsigned int last_index_;
	bool withsound_;
	bool remotehidden_;

	char TaskInfo_Index[3];
	unsigned char TaskInfo_Enable;

	std::string device_name_;
	std::string msg_;

private:
	unsigned char* PacketHeader(unsigned char* lpucBuffer, unsigned int wPacketNum)
	{
		unsigned char* p;

		p = lpucBuffer;

		// Discriminator
		*p = 'Y'; p++;
		*p = 'I'; p++;
		*p = 'N'; p++;
		*p = 'G'; p++;

		// Version
		*p = 0; p++;

		// Descriptor Id
		*p++ = static_cast<unsigned char>(id_ >> 8);
		*p++ = static_cast<unsigned char>(id_ >> 0);
		//*p++ = 0x77;
		//*p++ = 0x83;

		*p++;
		*p++;

		// Current Packet Number
		*p = static_cast<unsigned char>(wPacketNum >> 24); p++;
		*p = static_cast<unsigned char>(wPacketNum >> 16); p++;
		*p = static_cast<unsigned char>(wPacketNum >> 8); p++;
		*p = static_cast<unsigned char>(wPacketNum >> 0); p++;

		// Last Packet Number
		*p = static_cast<unsigned char>(last_packet_num_ >> 24); p++;
		*p = static_cast<unsigned char>(last_packet_num_ >> 16); p++;
		*p = static_cast<unsigned char>(last_packet_num_ >> 8); p++;
		*p = static_cast<unsigned char>(last_packet_num_ >> 0); p++;

		// Reserved
		*p = 0x00; p++;
		*p = 0x00; p++;
		*p = 0x00; p++;
		*p = 0x00; p++;
		// Payload_CheckSum
		*p = 0; p++;
		return p;
	}

	int PutData(unsigned char* lpucData, unsigned int wLength)
	{
		int i;
		unsigned int offset, left;
		unsigned int x, y;
		unsigned int bsize;
		unsigned char* pdst, * psrc, * p;
		//char *flag;

		/*if (id_ == 0x00)
		return -1;*/

		x = wLength + 7;

		if (id_ == 0x29)
		{
			x = wLength + 15;
		}
		else if (id_ == 0x0001)
		{
			//x = 1 /*descriptor*/ + 4/*d_length*/ + 2 /*login & passwd length*/ 	+ strlen((const char*)login_) + strlen((const char*)passwd_);
			//modify by George 071213 (MD5 code with len(8))
			x = 2 /*descriptor*/ + 4/*d_length*/ + 2 /*login & passwd length*/ + 8 + 8 + 1;// + 1 /* VideoSelect_ */;
			//eden
		}
		else if (id_ == 0x0002)
		{
			x = 2 /*descriptor*/ + 4/*d_length*/;
		}
		else if (id_ == 0x0003)
		{
			x = 2 /*descriptor*/ + 4/*d_length*/;
		}
		else
		{
			x = wLength + 7;
		}

		last_packet_num_ = static_cast<unsigned short>(x / PACKET_PAYLOAD_SIZE);
		y = x % PACKET_PAYLOAD_SIZE;
		if (y == 0)
			last_packet_num_ -= 1;
		bsize = (last_packet_num_ + 1) * PACKET_SIZE;

		pdst = buffer_;
		psrc = lpucData;
		for (i = 0; i <= (int)last_packet_num_; i++)		// 0x89
		{
			//put Current Packet Number etc...
			PacketHeader(pdst, i);
			p = pdst + PACKET_HEADER_SIZE;

			// First Packet
			if (i == 0)
			{
				// Descriptor Id
				*p++ = static_cast<unsigned char>(id_ >> 8);
				*p++ = static_cast<unsigned char>(id_ >> 0);

				//*p = id_; p++;

				// Descriptor Length
				if (id_ == 0x0001)
				{
					size_t Length = 2 /*descriptor*/ + 4/*d_length*/ + device_name_.length(); //string length;
					*p = static_cast<unsigned char>((Length) >> 24); p++;
					*p = static_cast<unsigned char>((Length) >> 16); p++;
					*p = static_cast<unsigned char>((Length) >> 8); p++;
					*p = static_cast<unsigned char>((Length) >> 0); p++;
					*p++;
					*p++;


					//fprintf(stderr, "MyString is [%s], with length [%lld]\n", device_name_.c_str(), device_name_.length());
					memcpy(p, device_name_.c_str(), device_name_.length());
					p += device_name_.length();
					/*
				*p = static_cast<unsigned char> (8) ; p++;
				memcpy(p, login_, 8 ); p += 8;
				*p = static_cast<unsigned char> (8) ; p++;
				memcpy(p, passwd_,8 ); p += 8;
				*/
					*p = 0x00; p++;

					//eden
					//*p = VideoSelect_; p++;

				}
				else if (id_ == 0x0002)
				{

					size_t Length = 2 /*descriptor*/ + 4/*d_length*/;
					*p = static_cast<unsigned char>((Length) >> 24); p++;
					*p = static_cast<unsigned char>((Length) >> 16); p++;
					*p = static_cast<unsigned char>((Length) >> 8); p++;
					*p = static_cast<unsigned char>((Length) >> 0); p++;
					//Functional code 
					*p++;
					*p++;
					*p = 0x00; p++;

					//eden
					//*p = VideoSelect_; p++;
				}
				else if (id_ == 0x0003)
				{

					size_t Length = 2 /*descriptor*/ + 4/*d_length*/ + device_name_.length() + 4 + msg_.length(); //string length;
					*p = static_cast<unsigned char>((Length) >> 24); p++;
					*p = static_cast<unsigned char>((Length) >> 16); p++;
					*p = static_cast<unsigned char>((Length) >> 8); p++;
					*p = static_cast<unsigned char>((Length) >> 0); p++;
					*p++;
					*p++;

					int device_length = device_name_.length();
					memcpy(p, &device_length, sizeof(int));
					p += 4;
					memcpy(p, device_name_.c_str(), device_length);
					p += device_length;

					int msg_length = msg_.length();
					memcpy(p, &msg_length, sizeof(int));
					p += 4;
					memcpy(p, msg_.c_str(), msg_length);
					p += msg_length;


					//fprintf(stderr, "To Device is [%s], with length [%d], Message is [%s], with length [%d]\n", device_name_.c_str(), device_length, msg_.c_str(), msg_length);
					*p = 0x00; p++;


				}

				else //if (id == 0x47)
				{
					fprintf(stderr, "Error!! Should not be here\n");
				}
			}

			offset = static_cast<unsigned int>(p - pdst);
			left = PACKET_SIZE - offset;
			memcpy(p, psrc, left);
			p += left;
			psrc += left;
			PayloadCheckSum(pdst);

			pdst += PACKET_SIZE;
		}

		return bsize;
	}

	unsigned char PayloadCheckSum(unsigned char* pucBuffer)
	{
		unsigned char sum = 0, * psum;
		unsigned char* p;

		p = pucBuffer + PACKET_HEADER_SIZE;
		psum = p - 1;
		while (p < (pucBuffer + PACKET_SIZE))
		{
			sum ^= *p; p++;
		}
		*psum = sum;
		return sum;
	}

public:
	NetworkClient(boost::asio::io_service& io_service) : resolver_(io_service), socket_(io_service)
	{
		ipaddress_ = "";
		buffer_ = new unsigned char[6000000];
		memset(buffer_, 0, 6000000);
		port_ = 0;
	}

	~NetworkClient()
	{
		ipaddress_ = "";
		port_ = 0;
		delete[] buffer_;
	}

	void Init(std::string ipaddress, int port)
	{
		ipaddress_ = ipaddress;
		port_ = port;
	}

	int Connect()
	{
		tcp::resolver::query query(ipaddress_.c_str(), std::to_string(port_));
		tcp::resolver::iterator endpoint_iterator = resolver_.resolve(query);

		boost::system::error_code error = boost::asio::error::host_not_found;
		socket_.connect(*endpoint_iterator++, error);

		socState = SocketState::Disconnect;

		if (error)
		{
			std::cerr << "Unable to connect" << std::endl;
			throw boost::system::system_error(error);
		}
		else //Turn on Receving Thread
		{
			fprintf(stderr, "Turn on RecvThread\n");
			boost::thread thrd1(&RecvThread, this);
			boost::thread::id id1 = thrd1.get_id();

			// Init
			socState = SocketState::Connected;
			splitData = std::vector<int>(SPLIT_DATA_CNT, 0);

			return true;
		}
		return false;
	}

	bool SendAck(unsigned char* buffer, unsigned int size)
	{
		//cerr << "About to sent from CStreaming"<<endl;
		std::string message((const char*)buffer, size);
		boost::system::error_code ignored_error;
		boost::asio::write(socket_, boost::asio::buffer(message), boost::asio::transfer_all(), ignored_error);

		return true; //for now
	}

	//Modified method
	void Register(std::string deviceName)
	{
		unsigned char ack[PACKET_SIZE];
		memset(ack, 0, sizeof(ack));

		id_ = 0x0001;
		device_name_ = deviceName;

		int res = PutData(ack, 0);
		if (!SendAck(buffer_, res))
		{
			std::cerr << "(Register) Send Ack Fails, exit client" << std::endl;
			return;
		}
	}

	void SendMsg(std::string toDeviceName, std::string msg)
	{
		unsigned char ack[PACKET_SIZE];
		memset(ack, 0, sizeof(ack));

		id_ = 0x0003;
		device_name_ = toDeviceName;
		msg_ = msg;

		int res = PutData(ack, 0);
		if (!SendAck(buffer_, res))
		{
			std::cerr << "(SendMsg) Send Ack Fails, exit client" << std::endl;
			return;
		}
	}

	void Receiving_new()
	{
		unsigned char packet[PACKET_SIZE], * p, * ptr, ver, pcs, cs;
		unsigned int id = 0;
		unsigned short cpn, lpn;
		unsigned int descriptor_length = 0;
		unsigned char function_code = 0;

		memset(packet, 0, PACKET_SIZE);
		p = packet;
		boost::this_thread::sleep(boost::posix_time::milliseconds(100));

		while (1)
		{
			//Blocking Receive is Okay
			memset(packet, 0, PACKET_SIZE);
			std::string pstr((const char*)packet, (int)PACKET_SIZE);
			size_t len = 0;
			boost::system::error_code e;

			do
			{
				len += socket_.read_some(boost::asio::buffer(packet + len, PACKET_SIZE - len), e);
			} while ((len < PACKET_SIZE) && (!e));

			if (len != PACKET_SIZE)
			{
				std::cerr << "Disconnect: " << len << std::endl;
				std::cerr << "Err: " << e << std::endl;
				connState_lock.lock();
				socket_.shutdown(boost::asio::socket_base::shutdown_both);
				remoteF = false;
				socState = SocketState::Disconnect;
				connState_lock.unlock();
				break;
			}

			p = packet;
			if (memcmp(packet, "YING", 4))
			{
				continue;
			}
			ptr = packet + 4;
			ver = *ptr++;

			id = (*(ptr + 0) << 8) + *(ptr + 1);
			ptr += 2;

			ptr++;
			ptr++;

			// Resvered
			header = *ptr++;

			// Current_Packet_Num
			cpn = ((*(ptr + 0) << 24) | (*(ptr + 1) << 16) | (*(ptr + 2) << 8) | *(ptr + 3)); ptr += 4;

			// Last_Packet_Num
			lpn = ((*(ptr + 0) << 24) | (*(ptr + 1) << 16) | (*(ptr + 2) << 8) | *(ptr + 3)); ptr += 4;

			ptr += 4; //Skip reserved
			ptr += 1; //skip checksum

			// Payload_CheckSum
			pcs = *ptr;//++;

			cs = 0;

			if (id == 0x0001)
			{
				std::cout << "Registered!" << std::endl;
				memset(packet, 0, PACKET_SIZE);
				continue;
			}
			if (id == 0x0002)
			{
				//cout << "Received 0x0002" << endl;
				ptr = packet + PACKET_HEADER_SIZE;

				id = (*(ptr + 0) << 8) + *(ptr + 1);
				ptr += 2;
				//fprintf(stderr, "DescID = 0x%04X\n", id);

				int length = ((*(ptr + 0) << 24) | (*(ptr + 1) << 16) | (*(ptr + 2) << 8) | *(ptr + 3));
				ptr += 4;

				//fprintf(stderr, "length [%d]\n", length);

				//First packet
				id = (*(ptr + 0) << 8) + *(ptr + 1);
				ptr += 2;
				//fprintf(stderr, "Func Code = 0x%04X\n", id);

				length = ((*(ptr + 0) << 24) | (*(ptr + 1) << 16) | (*(ptr + 2) << 8) | *(ptr + 3));
				ptr += 4;

				//fprintf(stderr, "length [%d]\n", length);

				if (length > 0)
				{
					deviceVec_lock.lock();// Lock deviceVec

					int num = 0;
					memcpy(&num, ptr, sizeof(int));
					//fprintf(stderr, "num of devices [%d]\n", num);

					deviceCnt = num;

					ptr += 4;

					deviceVec = std::vector<std::string>(num);
					for (int i = 0; i < num; i++)
					{
						int length = 0;
						memcpy(&length, ptr, sizeof(int));
						ptr += 4;
						char device_name[80];
						memset(device_name, 0, 80);
						memcpy(device_name, ptr, length);
						ptr += length;
						deviceVec[i] = (std::string)device_name;
						deviceVecF = true;
					}
					deviceVec_lock.unlock();
				}
				memset(packet, 0, PACKET_SIZE);
				continue;
			}
			if (id == 0x0003)
			{
				//cout << "Received 0x0003" << endl;
				ptr = packet + PACKET_HEADER_SIZE;

				id = (*(ptr + 0) << 8) + *(ptr + 1);
				ptr += 2;
				//fprintf(stderr, "DescID = 0x%04X\n", id);

				int length = ((*(ptr + 0) << 24) | (*(ptr + 1) << 16) | (*(ptr + 2) << 8) | *(ptr + 3));
				ptr += 4;

				//fprintf(stderr, "length [%d]\n", length);

				//First packet
				id = (*(ptr + 0) << 8) + *(ptr + 1);
				ptr += 2;
				//fprintf(stderr, "Func Code = 0x%04X\n", id);

				length = ((*(ptr + 0) << 24) | (*(ptr + 1) << 16) | (*(ptr + 2) << 8) | *(ptr + 3));
				ptr += 4;

				//fprintf(stderr, "length [%d]\n", length);

				if (length > 0)
				{
					int length = 0;
					memcpy(&length, ptr, sizeof(int));
					ptr += 4;
					char device_name[80];
					memset(device_name, 0, 80);
					memcpy(device_name, ptr, length);
					ptr += length;

					int msg_length = 0;
					memcpy(&msg_length, ptr, sizeof(int));
					ptr += 4;
					char msg[80];
					memset(msg, 0, 80);
					memcpy(msg, ptr, msg_length);
					ptr += msg_length;
					//recv: msg

					splitData_lock.lock();
					std::string msg_str = static_cast<std::string>(msg);
					if (msg_str == "ControlRegister" && !remoteF)// Set control device name
					{
						remoteDevName = device_name;
						remoteF = true;
						printf("Remote device name: %s\n", remoteDevName.c_str());
					}
					else
					{
						if (msg_str[0] == '#')
						{
							std::string ts_now = this->getTimestampStr();
							std::string recvTimeStr = msg_str.substr(1, msg_str.find('!'));

							LLONG tSend = this->cvtTimestampStrToLong(recvTimeStr);
							LLONG tRecv = this->cvtTimestampStrToLong(ts_now);
							transTime = tRecv - tSend;// ms
							recvStr = msg_str;
						}
						else
						{
							recvStr = msg_str;
						}
					}
					splitData_lock.unlock();
				}
				memset(packet, 0, PACKET_SIZE);
				continue;
			}

		} //while(1)

	}

	std::vector<std::string> getDeviceList()
	{
		unsigned char ack[PACKET_SIZE];
		memset(ack, 0, sizeof(ack));
		deviceVecF = false;
		id_ = 0x0002;
		int res = PutData(ack, 0);
		if (!SendAck(buffer_, res))
		{
			std::cerr << "Send Ack Fails, exit client" << std::endl;
			return deviceVec;
		}
		while (!deviceVecF)
			boost::this_thread::sleep(boost::posix_time::milliseconds(100));
		return deviceVec;
	}

	std::vector<int> getSplitData() const
	{
		return splitData;
	}

	std::string getIPServerStateStr() const
	{
		if (this->socState == SocketState::Connected)
			return (std::string)"Connected";
		else if (this->socState == SocketState::Disconnect)
			return (std::string)"Disconnect";
	}

	std::string getControlStateStr() const
	{
		if (this->remoteF)
			return (std::string)"Connected";
		else
			return (std::string)"Disconnect";
	}

	std::string getHostInfo() const
	{
		return this->ipaddress_ + ":" + std::to_string(this->port_);
	}

	std::string getRecvData()
	{
		return this->recvStr;
	}

	std::string getRemoteDevName() const
	{
		return remoteDevName;
	}

	bool isRemote() const
	{
		return remoteF;
	}

	static std::string getTimestampStr()
	{
		std::stringstream ss;
		auto t_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
		auto val = t_ms.time_since_epoch();
		ss << std::hex << val.count();
		return ss.str();
	}

	LLONG cvtTimestampStrToLong(std::string str)
	{
		LLONG val = strtoll(str.c_str(), nullptr, 16);
		return val;
	}

	LLONG getTransTime() const
	{
		return this->transTime;
	}

	std::string getTimestamp() const
	{
		return this->timestamp;
	}

};

void RecvThread(void* dummy)
{
	NetworkClient* nc = (NetworkClient*)dummy;
	nc->Receiving_new();
}