#ifndef __WS_HPP__
#define __WS_HPP__

#include<stdio.h>
#include<string.h>
#include<stdint.h>

class ws_frame
{
public:
	ws_frame(const char* buffer)
	{
		m_frame_header=((ws_frame_header*)(buffer));
		if(m_frame_header->FIN != 1)
		{
			printf("not support frame fragment!\n");
		}
	}
	bool decode_data(char* buffer)
	{
		const char* ptr=get_data_ptr();
		int count=get_data_len();

		int mask=get_masking_key();
		char* pmask=(char*)(&mask);

		for(int i=0;i<count;i++)
		{
			buffer[i]=ptr[i] ^ pmask[i % 4];
		}
		return true;
	}
public:
	struct ws_frame_header
	{
		__attribute__ ((packed))
		uint8_t opcode:4;
		uint8_t RSV3:1;
		uint8_t RSV2:1;
		uint8_t RSV1:1;
		uint8_t FIN:1;

		uint8_t payload_len:7;
		uint8_t MASK:1;

		ws_frame_header(void)
		{
			static_assert(sizeof(ws_frame_header) == 2,"sizeof ws_frame_header != 2");
		}
	};
	struct ws_frame_payload_len_equal_126:public ws_frame_header
	{
		__attribute__ ((packed))
		uint16_t extern_payload_len:16;
	};
	struct ws_frame_payload_len_equal_127:public ws_frame_header
	{
		__attribute__ ((packed))
		uint64_t extern_payload_len:64;
	};
	struct ws_frame_payload_len_shorter_126_masked:public ws_frame_header
	{
		__attribute__ ((packed))
		uint32_t masking_key:32;
	};
	struct ws_frame_payload_len_equal_126_masked:public ws_frame_payload_len_equal_126
	{
		__attribute__ ((packed))
		uint32_t masking_key:32;
	};
	struct ws_frame_payload_len_equal_127_masked:public ws_frame_payload_len_equal_127
	{
		__attribute__ ((packed))
		uint32_t masking_key:32;
	};
public:
	int get_FIN(void)
	{
		return m_frame_header->FIN;
	}
	int get_opcode(void)
	{
		return m_frame_header->opcode;
	}
	int get_MASK(void)
	{
		return m_frame_header->MASK;
	}
	int get_payload_len(void)
	{
		return m_frame_header->payload_len;
	}
	int get_extern_payload_len(void)
	{
		if(m_frame_header->payload_len == 126)
		{
			return ((ws_frame_payload_len_equal_126*)(m_frame_header))->extern_payload_len;
		}
		else if(m_frame_header->payload_len == 127)
		{
			return ((ws_frame_payload_len_equal_127*)(m_frame_header))->extern_payload_len;
		}
		else
		{
			printf("you can not get extern_payload_len!\n");
			return 0;
		}
	}
	int get_masking_key(void)
	{
		if(m_frame_header->MASK)
		{
			switch(m_frame_header->payload_len)
			{
				case 126:
					{
						return ((ws_frame_payload_len_equal_126_masked*)(m_frame_header))->masking_key;
					}break;
				case 127:
					{
						return ((ws_frame_payload_len_equal_127_masked*)(m_frame_header))->masking_key;
					}break;
				default:
					{
						return ((ws_frame_payload_len_shorter_126_masked*)(m_frame_header))->masking_key;
					}break;
			}
		}
		else
		{
			printf("you can not get masking_key!\n");
			return 0;
		}
	}

	const char* get_data_ptr(void)
	{
		if(m_frame_header->payload_len == 126)
		{
			if(m_frame_header->MASK)
			{
				return ((const char*)(m_frame_header)) + sizeof(ws_frame_payload_len_equal_126_masked);
			}
			else
			{
				return ((const char*)(m_frame_header)) + sizeof(ws_frame_payload_len_equal_126);
			}
		}
		else if(m_frame_header->payload_len == 127)
		{
			if(m_frame_header->MASK)
			{
				return ((const char*)(m_frame_header)) + sizeof(ws_frame_payload_len_equal_127_masked);
			}
			else
			{
				return ((const char*)(m_frame_header)) + sizeof(ws_frame_payload_len_equal_127);
			}
		}
		else
		{
			if(m_frame_header->MASK)
			{
				return ((const char*)(m_frame_header)) + sizeof(ws_frame_payload_len_shorter_126_masked);
			}
			else
			{
				return ((const char*)(m_frame_header)) + sizeof(ws_frame_header);
			}
		}
	}
	uint64_t get_data_len(void)
	{
		if(m_frame_header->payload_len == 126)
		{
			return ((ws_frame_payload_len_equal_126*)(m_frame_header))->extern_payload_len;
		}
		else if(m_frame_header->payload_len == 127)
		{
			return ((ws_frame_payload_len_equal_127*)(m_frame_header))->extern_payload_len;
		}
		else
		{
			return m_frame_header->payload_len;
		}
	}
public:
	ws_frame_header* m_frame_header=nullptr;
public:
};

class simple_ws_s
{
public:
	int m_fd=0;
	//bool m_new_head=true;
	//int m_pending_data_len=0;
public:
	simple_ws_s(int fd)
	{
		m_fd=fd;
	}
	bool handshake(void)
	{
		char buffer[4096]={0};
		int count = ::read(m_fd,buffer,4096);
		get_handshake_header(buffer,count);
		::write(m_fd,m_handshake_header,m_handshake_header_count);

		if((count == 0) || (count == -1))
			return false;
		else
			return true;
	}

	int write(const char* buffer,size_t count)
	{
		char m_buffer[4096]={0};

		((ws_frame::ws_frame_header*)(m_buffer))->FIN=1;
		((ws_frame::ws_frame_header*)(m_buffer))->opcode=2;
		((ws_frame::ws_frame_header*)(m_buffer))->MASK=0;
		if(count < 126)
		{
			((ws_frame::ws_frame_header*)(m_buffer))->payload_len=count;
			::write(m_fd,m_buffer,2);
			return ::write(m_fd,buffer,count);
		}
		else if(count == 126)
		{
			//TODO
			printf("not implement!\n");
			((ws_frame::ws_frame_header*)(m_buffer))->payload_len=126;
			return 0;
		}
		else if(count ==127)
		{
			//TODO
			printf("not implement!\n");
			((ws_frame::ws_frame_header*)(m_buffer))->payload_len=127;
			return 0;
		}
		else
		{
			//TODO
			printf("not implement!\n");
			return 0;
		}
	}
	int read(char* buffer,size_t count)
	{
		char buf[40960]={0};

		int cnt=::read(m_fd,buf,40960);
		if((cnt == -1) || (cnt == 0))
		{
			return cnt;
		}

		if(((ws_frame::ws_frame_header*)(buf))->FIN != 1)
		{
			printf("frame fragment not implement.\n");
			return -1;
		}
		if(((ws_frame::ws_frame_header*)(buf))->opcode != 2)
		{
			printf("only binary frame supported.\n");
			return -1;
		}

		ws_frame f(buf);
		f.decode_data(buffer);

		return f.get_data_len();
	}
public:
	char m_handshake_header[4096]={0};
	size_t m_handshake_header_count=0;
	const char* get_handshake_header(char* arr,size_t count)
	{
		get_key(arr,count);
		get_protocol(arr,count);
		append_guid();
		get_sha1sum();
		get_base64();
		get_response(m_handshake_header);
		m_handshake_header_count=strlen(m_handshake_header);
		return m_handshake_header;
	}
public:
	bool get_key(const char* arr,size_t count)
	{
		const char* key=strnstr(arr,count,"Sec-WebSocket-Key: ",19);
		if(key == nullptr)
		{
			printf("serch field key failed.\n");
			return false;
		}
		key+=19;

		for(int i=0;i<4096;i++)
		{
			if((key[i] == '\r') || (key[i] == '\n') || (key[i] == ' ') || (key[i] == '\t'))
			{
				m_key_count=i;
				m_key[i]='\0';
				return true;
			}
			else
			{
				m_key[i]=key[i];
			}
		}

		printf("serch too long, maybe something error...\n");
		return false;
	}
	bool get_protocol(const char* arr,size_t count)
	{
		const char* protocol = strnstr(arr,count,"Sec-WebSocket-Protocol: ",24);
		if(protocol == nullptr)
		{
			printf("serch field protocol failed.\n");
			return false;
		}
		protocol+=24;

		for(int i=0;i<4096;i++)
		{
			if((protocol[i] == '\r') || (protocol[i] == '\n') || (protocol[i] == ' ') || (protocol[i] == '\t'))
			{
				m_protocol_count=i;
				m_protocol[i]='\0';
				return true;
			}
			else
			{
				m_protocol[i]=protocol[i];
			}
		}

		printf("serch too long, maybe something error...\n");
		return false;
	}
	bool append_guid(void)
	{
		if((strlen(m_guid) + m_key_count) >= 4096)
		{
			printf("buffer too short or data too long.\n");
			return false;
		}

		strcpy(m_key_guid,m_key);
		strcat(m_key_guid,m_guid);
		m_key_guid_count=m_key_count+strlen(m_guid);
		return true;
	}
	bool get_sha1sum(void)
	{
		int file=open("./.temp",O_WRONLY | O_CREAT | O_TRUNC);
		if(-1 == file)
		{
			printf("open file error.\n");
			return false;
		}

		int count = ::write(file,m_key_guid,m_key_guid_count);
		close(file);

		system("sha1sum ./.temp > ./.temp_sha1sum");

		file = open("./.temp_sha1sum",O_RDONLY);
		count = ::read(file,m_sha1sum,4096);
		close(file);

		m_sha1sum_count=count;
		m_sha1sum[m_sha1sum_count]='\0';
		return true;
	}
	bool get_base64(void)
	{
		char sha1sum[20]={0};

		for(int i=0;i<20;i++)
		{
			char _arr[3]={0};
			_arr[0]=m_sha1sum[i*2];
			_arr[1]=m_sha1sum[i*2+1];

			sscanf(_arr,"%02x",&(sha1sum[i]));
		}

		int file=open("./.temp_sha1sum_bin",O_WRONLY | O_CREAT | O_TRUNC);
		if(-1 == file)
		{
			printf("open file error.\n");
			return false;
		}
		int count = ::write(file,sha1sum,20);
		close(file);

		system("base64 ./.temp_sha1sum_bin > ./.temp_base64");

		file = open("./.temp_base64",O_RDONLY);
		count = ::read(file,m_base64,4096);
		//因为软件生成的最后带有\n符号
		count-=1;
		close(file);

		m_base64[count]='\0';
		m_base64_count=count;
		return true;
	}
	bool get_response(char* arr)
	{
		sprintf(arr,m_response_template,m_base64,m_protocol);
		return true;
	}
	bool clean_temp_file(void)
	{
		return system("rm -rf ./.temp ./.temp_sha1sum ./.temp_sha1sum_bin ./.temp_base64");
	}
public:
	char m_key[4096]={0};
	size_t m_key_count=0;

	char m_key_guid[4096]={0};
	size_t m_key_guid_count=0;

	char m_protocol[4096]={0};
	size_t m_protocol_count=0;

	char m_sha1sum[4096]={0};
	size_t m_sha1sum_count=0;

	char m_base64[4096]={0};
	size_t m_base64_count=0;
public:
	static const char* strnstr(const char* s1,size_t n1,const char* s2,size_t n2)
	{
		if((n1 == 0) || (n2 == 0) || (s1 == nullptr) || (s2 == nullptr) || (n2 > n1))
			return nullptr;

		for(size_t i=0;i<n1;i++)
		{
			if(s1[i] == s2[0])
			{
				for(int j=0;j<n2;j++)
				{
					if(s1[i+j] != s2[j])
					{
						goto next;
					}
				}
				return &(s1[i]);
			}
next:;
		}

		return nullptr;
	}
private:
	const char* m_guid="258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	const char* m_response_template="\
HTTP/1.1 101 Switching Protocols\r\n\
Upgrade: websocket\r\n\
Connection: Upgrade\r\n\
Sec-WebSocket-Accept: %s\r\n\
Sec-WebSocket-Protocol: %s\r\n\
\r\n";

};

#endif//__WS_HPP__
