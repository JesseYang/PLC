#ifndef TOOL_H
#define TOOL_H

#include <string>

using namespace std;

int send_udp_request(string ip_addr, int port, string content = "");
int send_request(string ip_addr, int port, string content = "");
int find_string_in_ary(string array[], string value, int length);
time_t get_sys_time();
char* join_str_with_colon(string str1, char str2[]);
void number_to_string(int number, char* str);

#endif
