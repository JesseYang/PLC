#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include <cstring>
#include "tool.h"

using namespace std;

int send_udp_request(string ip_addr, int port, string content) {
  int fd = socket(AF_INET,SOCK_DGRAM,0);
  if (fd == -1) {
    return -1;
  }
  struct sockaddr_in addr_to;//目标服务器地址
  addr_to.sin_family = AF_INET;
  addr_to.sin_port = htons(port);
  addr_to.sin_addr.s_addr = inet_addr(ip_addr.c_str());
  int retval = sendto(fd, content.c_str(), strlen(content.c_str()), 0, (struct sockaddr*)&addr_to, sizeof(addr_to));
  return retval;
}

int send_request(string ip_addr, int port, string content) {
  int socket_desc;
  struct sockaddr_in server;
  socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  server.sin_addr.s_addr = inet_addr(ip_addr.c_str());
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  
  if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
  {
    return -1;
  }
  if (content != "") {
    if (send(socket_desc, content.c_str(), strlen(content.c_str()), 0) < 0) {
      return -1;
    }
  }
  close(socket_desc);
  return 0;
}

int find_string_in_ary(string array[], string value, int length) {
  for (int i = 0; i < length; i++) {
    if (strcmp(array[i].c_str(), value.c_str()) == 0) {
      return 1;
    }
  }
  return -1;
}

time_t get_sys_time() {
  time_t raw_time;
  time(&raw_time);
  return raw_time;
}

char* join_str_with_colon(string str1, char str2[]) {
  char* joined_str;
  joined_str = new char[strlen(str1.c_str())+2+strlen(str2)];
  strcpy(joined_str, str1.c_str());
  strcat(joined_str, ":");
  strcat(joined_str, str2);
  return joined_str;
}

void number_to_string(int number, char* str) {
  sprintf(str, "%d\0", number);
}
