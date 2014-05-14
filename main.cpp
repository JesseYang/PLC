#include <iostream>
#include <thread>
#include <list>
#include <cstring>
#include "slave.h"
#include "tool.h"
#include "log.h"

using namespace std;

int stop_recv_cmd_thread();
int stop_recv_data_thread();

int main() {
  log_init(LL_DEBUG, "log", "/root/");
  // initialize
  int retval, i;
  list<string> other_nodes;
  for (i = 0; i < NODE_NUM; i++) {
    if (strcmp(node_ip[i].c_str(), LOCAL_IP) != 0) {
      other_nodes.push_back(node_ip[i]);
    }
  }
  Slave slave(LOCAL_IP, other_nodes);

  // forever loop
  while(true) {
    slave.init_route();

    thread t_cmd(&Slave::recv_cmd, slave);
    thread t_data(&Slave::recv_data, slave);

    // this method blocks until the connect to the master lost
    slave.check_route();
    log_trace("check route finished");

    // stop the recv_cmd thread by sending a stop request
    do {
      retval = stop_recv_cmd_thread();
    } while (retval < 0);
    log_trace("terminate receive command thread");
    // stop the recv_data thread by sending a stop request
    do {
      retval = stop_recv_data_thread();
    } while (retval < 0);
    log_trace("terminate receive data thread");

    // wait the recv_cmd terminate
    t_cmd.join();
    t_data.join();
  }
}

int stop_recv_cmd_thread() {
  int retval = send_request("127.0.0.1", RECV_CMD_PORT, "");
  return retval;
}

int stop_recv_data_thread() {
  int retval = send_udp_request("127.0.0.1", RECV_DATA_PORT, "");
  return retval;
}
