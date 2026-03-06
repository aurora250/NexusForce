#ifndef TRY_H
#define TRY_H
#include <NeForce/NeForce.hpp>
using namespace neforce;

const path& res_root();

bool signal_handler(SIGNAL_EVENT event, void* context);


void test_file();
void test_datetimes();
void test_print();
void test_console();
void test_sysinfo();
void test_device();
void test_env_var();
void test_signal();
void test_cmd(int argc, char* argv[]);
void test_rnd();
void test_regex();
void test_format();
void test_color();
void test_enctype();

void test_https_server();
void test_http_server();
void test_http_client();

void test_list();

void test_ini();
void test_env();
void test_toml();
void test_yaml();
void test_json();

class Foo {};
void test_check();

void test_copy();
void test_deque();
void test_stack();
void test_vector();
void test_pqueue();
void test_rbtree();

inline int sum_3(int a, int b, int c) {
    return a + b + c;
}

void test_tuple();
void test_hashtable();
void test_math();

void test_sort();
void test_variant();
void test_string();

void test_option();
void test_st();
void test_any();
void test_timer();
void test_vthread();
void test_logging();
void test_ranges();
void test_sql();
void test_mysql();
void test_redis();
void test_pgsql();
void test_dbpool();
void test_ext_tpool();
void test_tpool();
void test_dns();
void test_process();
void test_zlib();

#endif //TRY_H
