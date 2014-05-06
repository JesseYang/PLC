#include <iostream>
#include <cstring>
#include <thread>
#include <list>
#include <unordered_map>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <alsa/asoundlib.h>
#include <time.h>
#include "slave.h"
#include "timer.h"
#include "tool.h"
#include "log.h"

using namespace std;

Slave::Slave(string ip_addr, list<string> node_ip_list) {
  this->ip_addr = ip_addr;
  this->is_connected = false;
  this->is_record = false;
  this->children_number = 0;
  this->rank = -1;
  list<string>::const_iterator iterator;
  for (iterator = node_ip_list.begin(); iterator != node_ip_list.end(); ++iterator) {
    this->node_ip_list.push_back(*iterator);
  }
}

thread Slave::start_record() {
  is_record = true;
  // start the record thread
  thread record_thread(&Slave::record, this);
  return record_thread;
}

void Slave::stop_record() {
  is_record = false;
}

void Slave::record() {
  cout << "Start record thread" << endl;
  // the audio initialization part
  long loops;
  int rc;
  int size;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *params;
  unsigned int val;
  int dir;
  snd_pcm_uframes_t frames;
  int channel_num = 1;

  cout << "Begin alsa configuration" << endl;
  /* Open PCM device for recording (capture). */
  rc = snd_pcm_open(&handle, "hw:0,0",
          SND_PCM_STREAM_CAPTURE, 0);
  if (rc < 0) {
    fprintf(stderr,
        "unable to open pcm device: %s\n",
        snd_strerror(rc));
    exit(1);
  }

  cout << "Device open" << endl;
  /* Allocate a hardware parameters object. */
  snd_pcm_hw_params_alloca(&params);

  /* Fill it in with default values. */
  snd_pcm_hw_params_any(handle, params);

  /* Set the desired hardware parameters. */

  /* Interleaved mode */
  snd_pcm_hw_params_set_access(handle, params,
                SND_PCM_ACCESS_RW_INTERLEAVED);

  /* Signed 16-bit little-endian format */
  snd_pcm_hw_params_set_format(handle, params,
                SND_PCM_FORMAT_S16_LE);

  /* Two channels (stereo) */
  snd_pcm_hw_params_set_channels(handle, params, channel_num);

  /* 44100 bits/second sampling rate (CD quality) */
  do {
    val = 48000;
    snd_pcm_hw_params_set_rate_near(handle, params,
                  &val, &dir);
  } while (val != 48000);

  /* Set period size to 48 frames. */
  frames = 48;
  snd_pcm_hw_params_set_period_size_near(handle,
                    params, &frames, &dir);

  /* Write the parameters to the driver */
  rc = snd_pcm_hw_params(handle, params);
  if (rc < 0) {
    fprintf(stderr,
        "unable to set hw parameters: %s\n",
        snd_strerror(rc));
    exit(1);
  }

  cout << "Parameter set" << endl;
  /* Use a buffer large enough to hold one period */
  snd_pcm_hw_params_get_period_size(params,
                  &frames, &dir);
  size = frames * 2 * channel_num; /* 2 bytes/sample, 2 channels */

  /* We want to loop for 5 seconds */
  snd_pcm_hw_params_get_period_time(params,
                  &val, &dir);

  cout << "End alsa configuration" << endl;

  // the network part
  int fd = socket(AF_INET,SOCK_DGRAM,0);
  if(fd==-1) {
    perror("socket create error!\n");
    exit(-1);
  }
  printf("socket fd=%d\n",fd);

  struct sockaddr_in addr_to;//目标服务器地址
  addr_to.sin_family = AF_INET;
  addr_to.sin_port = htons(RECV_DATA_PORT);
  addr_to.sin_addr.s_addr = inet_addr(this->parent.c_str());

  struct sockaddr_in addr_from;
  addr_from.sin_family = AF_INET;
  addr_from.sin_port = htons(0);//获得任意空闲端口
  addr_from.sin_addr.s_addr = htons(INADDR_ANY);//获得本机地址
  int yes = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  int r = ::bind(fd, (struct sockaddr*)&addr_from, sizeof(addr_from));

  if (r == -1) {
    printf("Bind error!\n");
    close(fd);
    exit(-1);
  }

  char buf[size];
  int len;
  char* data_with_ip;
  // send data to the server
  while (is_record) {
    
    data_with_ip = new char[strlen(this->ip_addr.c_str()) + 1 + size];
    strcpy(data_with_ip, this->ip_addr.c_str());
    strcat(data_with_ip, ":");
    rc = snd_pcm_readi(handle, &data_with_ip[strlen(this->ip_addr.c_str()) + 1], frames);
    if (rc == -EPIPE) {
      /* EPIPE means overrun */
      // fprintf(stderr, "overrun occurred\n");
      cout << "overrun occurred" << endl;
      snd_pcm_prepare(handle);
    } else if (rc < 0) {
      fprintf(stderr,
          "error from read: %s\n",
          snd_strerror(rc));
    } else if (rc != (int)frames) {
      // fprintf(stderr, "short read, read %d frames\n", rc);
      cout << "short read, read frames" << endl;
    }

    len = sendto(fd, data_with_ip, strlen(this->ip_addr.c_str()) + 1 + size, 0, (struct sockaddr*)&addr_to, sizeof(addr_to)); 
  }

  char command[] = "stop";
  char* command_with_ip = join_str_with_colon(this->ip_addr, command);
  len = sendto(fd, command_with_ip, strlen(command_with_ip), 0, (struct sockaddr*)&addr_to, sizeof(addr_to)); 
  free(command_with_ip);

  close(fd);
  snd_pcm_drain(handle);
  snd_pcm_close(handle);
  return;
}

void Slave::init_route() {
  thread threads[this->node_ip_list.size()];
  struct sockaddr_in server, client;
  int server_socket, client_socket, c;

  // first clear previous route info
  this->clear();
  while (true) {
    // start a server
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(RECV_INIT_ROUTE_DOWN_PORT);

    int yes = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    int retval = ::bind(server_socket, (struct sockaddr *)&server, sizeof(server));
    if (retval != 0) {
      log_error("%s", "bind failed");
    }
    listen(server_socket, 3);
    log_trace("%s %d", "waiting requests from parent at", RECV_INIT_ROUTE_DOWN_PORT);
    /*
    char str[30];
    number_to_string(RECV_INIT_ROUTE_DOWN_PORT, str);
    cout << "Waiting requests from parent at: " + (string)str << endl;
    */
    c = sizeof(struct sockaddr_in);
    client_socket = ::accept(server_socket, (struct sockaddr *)&client, (socklen_t*)&c);

#ifdef TEST_RELAY
    while (strcmp(inet_ntoa(client.sin_addr), ROOT_IP) == 0) {
      client_socket = ::accept(server_socket, (struct sockaddr *)&client, (socklen_t*)&c);
    }
#endif

    // receive parent request, record the parent and the rank
    this->parent = inet_ntoa(client.sin_addr);
    char rank_str[10];
    int read_size = recv(client_socket, rank_str, 10, 0);

    this->rank = atoi(rank_str);
    // cout << "  parent ip: " + this->parent << endl;
    // cout << "  rank: " + (string)rank_str << endl;
    log_trace("%s. [ip:%s] [rankd:%d]", "parent request received", this->parent.c_str(), this->rank);

    // close the sockets
    close(client_socket);
    close(server_socket);

    // send requests to all the other nodes
    int index = 0;
    for (list<string>::const_iterator ci = this->node_ip_list.begin(); ci != this->node_ip_list.end(); ++ci) {
      number_to_string(this->rank + 1, rank_str);
      threads[index] = thread(send_request, *ci, RECV_INIT_ROUTE_DOWN_PORT, rank_str);
      index++;
    }

    // start a timer, when it timeouts, stop the following server
    int timeout = INIT_ROUTE_BASIC_TIMEOUT - this->rank * INIT_ROUTE_DELTA_TIMEOUT;
    // number_to_string(timeout, str);
    // cout << "Waiting " + (string)str  + " seconds for children's response..." << endl;
    log_trace("waiting %d seconds for children's response...", timeout);
    Timer timer(timeout, Slave::stop_init_route);
    thread timer_t = timer.start();

    // start a server to receive request from children
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(RECV_INIT_ROUTE_UP_PORT);
    yes = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    ::bind(server_socket, (struct sockaddr *)&server, sizeof(server));
    listen(server_socket, 3);
    while ( (client_socket = accept(server_socket, (struct sockaddr *)&client, (socklen_t*)&c)) ) {
      string ip = inet_ntoa(client.sin_addr);
      if (strcmp(ip.c_str(), "127.0.0.1") == 0) {
        close(server_socket);
        close(client_socket);
        for (int i = 0; i < this->node_ip_list.size(); i++) {
          threads[i].join();
        }
        timer_t.join();
        break;
      }
      // a new child
      this->children[this->children_number] = ip;
      this->children_number += 1;
      this->route_update_time.erase(ip);
      this->route_update_time.insert(make_pair<string, time_t>((string)ip, get_sys_time()));
      this->children_route_info.erase(ip);
      char route_info[100];
      int byte_number = recv(client_socket, route_info, 100, 0);
      route_info[byte_number] = '\0';
      this->children_route_info.insert(make_pair<string, string>((string)ip, (string)route_info));
    }

    // report to parent
    log_trace("sending route report to parent %s", generate_children_route_info().c_str());
    // cout << "Sending route report to parent" << " " << generate_children_route_info() << endl;
    if (send_request(this->parent, RECV_INIT_ROUTE_UP_PORT, generate_children_route_info()) != 0) {
      // clear all parent and children information, back to the stage of waiting for parent request
      this->clear();
      continue;
    } else {
      break;
    }
  }
  log_trace("init route finished");
  // cout << "Init route finished" << endl;
  is_connected = true;
}

void Slave::clear() {
  int i = 0;
  for (i = 0; i < MAX_NUM; i++) {
    this->children[i] = "";
  }
  this->parent = "";
  this->rank = -1;
  this->children_number = 0;
  this->route_update_time.clear();
  this->children_route_info.clear();
}

void Slave::check_route() {
  // start a thread to accept report requests from children
  thread t(&Slave::recv_route_report, this);

  // start a client to send report requests to parent regularly
  int fail_time = 0;
  while (fail_time < CHECK_ROUTE_THRESHOLD && is_connected) {
    // cout << "Route info: " + this->generate_children_route_info() << endl;
    if (send_request(this->parent, RECV_CHECK_ROUTE_PORT, this->generate_children_route_info()) != 0) {
      // cout << "Fail: send check route request to " + this->parent << endl;
      fail_time++;
    } else {
      // cout << "Success: send check route request to " + this->parent << endl;
      fail_time = 0;
    }
    sleep(CHECK_ROUTE_INTERVAL);
  }

  // fail_time reaches threhold, the connection to the parent lost
  is_connected = false;
  send_request("127.0.0.1", RECV_CHECK_ROUTE_PORT);
  t.join();
}

void Slave::recv_route_report() {
  // start a server
  struct sockaddr_in server, client;
  int server_socket, client_socket, c;

  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(RECV_CHECK_ROUTE_PORT);
  int yes = 1;
  setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  ::bind(server_socket, (struct sockaddr *)&server, sizeof(server));
  listen(server_socket, 3);
  c = sizeof(struct sockaddr_in);
  char child_route_info[1000];
  while ( (client_socket = accept(server_socket, (struct sockaddr *)&client, (socklen_t*)&c)) ) {
    string ip = inet_ntoa(client.sin_addr);
    if (strcmp(ip.c_str(), "127.0.0.1") == 0) {
      // time out, should stop checking route for itself and all children
      is_connected = false;
      for (int i = 0; i < this->children_number; i++) {
        send_request(this->children[i], RECV_CHECK_ROUTE_PORT, "shutdown");
      }
      close(server_socket);
      close(client_socket);
      return;
    }
    if (strcmp(ip.c_str(), this->parent.c_str()) == 0) {
      // parent sends command to shutdown
      is_connected = false;
      // first send shutdown command to all the children
      for (int i = 0; i < this->children_number; i++) {
        send_request(this->children[i], RECV_CHECK_ROUTE_PORT, "shutdown");
      }
      // then close sockets
      close(server_socket);
      close(client_socket);
      return;
    }

    // cout << "Receive check route request from " + ip << endl;
    if (find_string_in_ary(this->children, ip, this->children_number)) {
      this->route_update_time.erase(ip);
      this->route_update_time.insert(make_pair<string, time_t>((string)ip, get_sys_time()));
      // update children route info
      int temp = read(client_socket, child_route_info, 1000);
      this->children_route_info.erase(ip);
      this->children_route_info.insert(make_pair<string, string>((string)ip, ((string)child_route_info).substr(0, temp)));
      // cout << "Update route info " + this->children_route_info[ip] << endl;
    }
  }
}

void Slave::recv_data() {
  // prepare the udp server
  int server_socket = socket(AF_INET, SOCK_DGRAM, 0);
  if(server_socket == -1) {
    perror("socket create error!\n");
    exit(-1);
  }

  struct sockaddr_in server;
  server.sin_family=AF_INET;
  server.sin_port=htons(RECV_DATA_PORT);
  server.sin_addr.s_addr=htonl(INADDR_ANY);

  int yes = 1;
  setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  int r = ::bind(server_socket, (struct sockaddr*)&server, sizeof(server));
  if(r == -1) {
    printf("Bind error!\n");
    close(server_socket);
    exit(-1);
  }

  char buf[1024];
  struct sockaddr_in children_client;
  socklen_t len;
  len = sizeof(children_client);

  // prepare the udp client
  int client_socket = socket(AF_INET,SOCK_DGRAM,0);
  if(client_socket==-1) {
    perror("socket create error!\n");
    exit(-1);
  }

  struct sockaddr_in parent_server;//目标服务器地址
  parent_server.sin_family = AF_INET;
  parent_server.sin_port = htons(RECV_DATA_PORT);
  parent_server.sin_addr.s_addr = inet_addr(this->parent.c_str());

  struct sockaddr_in client;
  client.sin_family = AF_INET;
  client.sin_port = htons(0);//获得任意空闲端口
  client.sin_addr.s_addr = htons(INADDR_ANY);//获得本机地址
  yes = 1;
  setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  r = ::bind(client_socket, (struct sockaddr*)&client, sizeof(client));

  while(1) {
    r = recvfrom(server_socket, buf, sizeof(buf), 0, (struct sockaddr*)&children_client, &len);
    string ip = inet_ntoa(children_client.sin_addr);
    if (strcmp(ip.c_str(), "127.0.0.1") == 0) {
      close(server_socket);
      close(client_socket);
      return;
    }
    // forward the data to parent
    len = sendto(client_socket, buf, r, 0, (struct sockaddr*)&parent_server, sizeof(parent_server)); 
  }
  close(server_socket);
  return;
}

void Slave::recv_cmd() {
  struct sockaddr_in server, client;
  int server_socket, client_socket, c, char_number;
  thread record_thread;
  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(RECV_CMD_PORT);
  int yes = 1;
  setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  ::bind(server_socket, (struct sockaddr *)&server, sizeof(server));
  listen(server_socket, 3);
  cout << "Waiting for commands..." << endl;
  c = sizeof(struct sockaddr_in);
  while ( (client_socket = accept(server_socket, (struct sockaddr *)&client, (socklen_t*)&c)) ) {
    string ip = inet_ntoa(client.sin_addr);
    if (strcmp(ip.c_str(), "127.0.0.1") == 0) {
      // should stop this thread
      // first stop the record thread
      if (is_record) {
        stop_record();
        record_thread.join();
      }
      // then return
      close(client_socket);
      close(server_socket);
      return;
    }

    char cmd_content[100];
    char_number = recv(client_socket, cmd_content, 100, 0);
    char copy_cmd_content[char_number + 1];
    cout << char_number << endl;
    memcpy(copy_cmd_content, cmd_content, char_number);
    copy_cmd_content[char_number] = '\0';
    cout << "Receiving command: " << cmd_content << endl;

    char * pch;
    pch = strtok(cmd_content, ":\r\n");
    // cout << "Get Destination IP: " << pch << endl;
    bool for_children = false;
    for (int i = 0; i < this->children_number; i++) {
      if (strcmp(this->children[i].c_str(), pch) == 0) {
        // forward the command to this child, and then return
        send_request(this->children[i], RECV_CMD_PORT, copy_cmd_content);
        for_children = true;
      }
    }

    if (for_children) {
      continue;
    }

    if (strcmp(this->ip_addr.c_str(), pch) != 0) {
      // forward the command to all the thildren
      for (int i = 0; i < this->children_number; i++) {
        // send_request(this->children[i], RECV_CMD_PORT, cmd_content);
        string temp = this->children[i];
        send_request(temp, RECV_CMD_PORT, copy_cmd_content);
      }
      continue;
    }

    // the command is for myself, execute it
    pch = strtok(NULL, ":\r\n");
    // cout << "Get command content: " << pch << endl;
    if (strcmp(pch, "start") == 0) {
      record_thread = start_record();
    }
    if (strcmp(pch, "stop") == 0) {
      stop_record();
      record_thread.join();
    }
  }
}


string Slave::generate_children_route_info() {
  string route_info = "";
  time_t raw_time;
  time(&raw_time);
  for (int i = 0; i < this->children_number; i++) {
    if (raw_time - route_update_time[this->children[i]] > CHECK_ROUTE_TIMEOUT) {
      continue;
    }
    if (strcmp(route_info.c_str(), "") == 0) {
      route_info += children_route_info[this->children[i]];
    } else {
      route_info += "," + children_route_info[this->children[i]];
    }
  }
  if (route_info == "") {
    return "[" + this->ip_addr + "]";
  }
  return "[" + this->ip_addr + ":" + route_info + "]";
}

void Slave::stop_init_route() {
  send_request("127.0.0.1", RECV_INIT_ROUTE_UP_PORT);
}
