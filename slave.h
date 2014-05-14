#ifndef SLAVE_H
#define SLAVE_H

#include <thread>
#include <string>
#include <unordered_map>
#include <list>
using namespace std;

// #define TEST_RELAY

#define MAX_NUM 8
#define RECV_INIT_ROUTE_DOWN_PORT 8888
#define RECV_INIT_ROUTE_UP_PORT 8889
#define RECV_CMD_PORT 8890
#define RECV_DATA_PORT 8891
#define RECV_CHECK_ROUTE_PORT 8892
#define CHECK_ROUTE_THRESHOLD 3
#define CHECK_ROUTE_INTERVAL 3
#define CHECK_ROUTE_TIMEOUT CHECK_ROUTE_THRESHOLD * CHECK_ROUTE_INTERVAL

#define INIT_ROUTE_BASIC_TIMEOUT 6
#define INIT_ROUTE_DELTA_TIMEOUT 2

#define LOCAL_IP "192.168.1.2"
#define NODE_NUM 2
#define ROOT_IP "192.168.1.4"

const string node_ip[] = {"192.168.1.1", "192.168.1.2"};


class Slave {
  private:
    bool is_connected;
    string parent;
    string ip_addr;
    string children[MAX_NUM];
    list<string> node_ip_list;
    int children_number;
    int rank;
    unordered_map<string, time_t> route_update_time;
    unordered_map<string, string> children_route_info;
    bool is_record;
    void record();
    void recv_route_report();
    string generate_children_route_info();
    void clear();
  public:
    Slave(string, list<string>);
    thread start_record();
    void stop_record();
    void check_route();
    static void stop_init_route();
    void init_route();
    void recv_cmd();
    void recv_data();
};

#endif
