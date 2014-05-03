#include <iostream>
#include <thread>
#include <list>
#include <cstring>
#include "slave.h"
#include "tool.h"

using namespace std;

int stop_recv_cmd_thread();
int stop_recv_data_thread();

int main() {
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
    cout << "Check route finished" << endl;

    // stop the recv_cmd thread by sending a stop request
    do {
      retval = stop_recv_cmd_thread();
    } while (retval < 0);
    cout << "Terminate recevie command thread." << endl;
    // stop the recv_data thread by sending a stop request
    do {
      retval = stop_recv_data_thread();
    } while (retval < 0);
    cout << "Terminate recevie data thread." << endl;

    // wait the recv_cmd terminate
    t_cmd.join();
    cout << "AAAAA" << endl;
    t_data.join();
    cout << "BBBBB" << endl;
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
