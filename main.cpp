#include<iostream>
#include"socket.hpp"
#include<vector>
#include"ws.hpp"
#include<unistd.h>
using namespace std;
using namespace coder1;

int main(int argc,char* argv[])
{
	tcp_socket s;
	s.set_block(true);
	s.bind("0.0.0.0",9000);
	s.listen(5);

	int fd=s.accept();

	simple_ws_s ws(fd);
	ws.handshake();

	char arr[4096]={0};
	int count = ws.read(arr,4096);

	for(int i=0;i<count;i++)
	{
		printf("%c",arr[i]);
	}

	while(1)
	{
		ws.write(arr,count);
		sleep(1);
		//arr[0]+=1;
	}

	fflush(stdout);
	close(fd);
    return 0;
}
