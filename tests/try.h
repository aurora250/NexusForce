#ifndef TRY_H
#define TRY_H
#include <MSTL/MSTL.hpp>
using namespace MSTL;

void test_file();
void test_datetimes();
void test_print();
void test_console();
void test_device();
void test_rnd();
void test_format();
void test_color();
void test_enctype();

inline void handle_session_api(
    http_request &request, http_response &response, http_server &server) {
    session *sess = server.session(request);
    string action = request.parameter("action");

    if (action == "create") {
        sess = server.session(request, true);
        response.set_ok();
        response.set_status_msg("OK");
        response.set_content_type(HTTP_CONTENT::JSON_APP);
        response.set_body(R"({"sessionId":")" + sess->id() + R"("})");
    } else if (action == "invalidate" && sess) {
        sess->invalidate();
        response.set_ok();
        response.set_status_msg("OK");
        response.set_content_type(HTTP_CONTENT::JSON_APP);
        response.set_body(R"({"message":"Session invalidated"})");
    } else if (action == "info") {
        if (sess) {
            auto json = json_builder()
                .begin_object()
                .key("sessionId")
                .value(sess->id())
                .key("createTime")
                .value(sess->create_time().to_string_ISO_UTC())
                .key("lastAccess")
                .value(sess->last_access().to_string_ISO_UTC())
                .key("attributes")
                .value(sess->get_data()).end_object()
                .build();

            response.set_ok();
            response.set_status_msg("OK");
            response.set_content_type(HTTP_CONTENT::JSON_APP);
            response.set_body(json->to_string());
        } else {
            response.set_bad_request();
            response.set_status_msg("Bad Request");
            response.set_content_type(HTTP_CONTENT::JSON_APP);
            response.set_body(R"({"error":"No active session found"})");
        }
    } else {
        response.set_bad_request();
        response.set_status_msg("Bad Request");
        response.set_content_type(HTTP_CONTENT::JSON_APP);
        response.set_body(R"({"error":"Invalid session action"})");
    }
}

inline void handle_session_attribute(http_request &request, http_response &response, http_server &server) {
    string attrName, attrValue;
    string content_type = request.content_type();

    if (content_type.find(HTTP_CONTENT::JSON_APP.content()) == 0) {
        try {
            auto root = json_parser(request.body()).parse();
            if (root && root->is_object()) {
                const json_object *obj = root->as_object();

                const json_value *attrNameVal = obj->get_member("attrName");
                if (attrNameVal && attrNameVal->is_string()) {
                    attrName = attrNameVal->as_string()->get_value();
                }

                const json_value *attrValueVal = obj->get_member("attrValue");
                if (attrValueVal && attrValueVal->is_string()) {
                    attrValue = attrValueVal->as_string()->get_value();
                }
            }
        } catch (const exception &e) {
            println("JSON parse error:", e.what());
        }
    } else {
        attrName = request.parameter("attrName");
        attrValue = request.parameter("attrValue");
    }

    session *sess = server.session(request, true);

    if (!attrName.empty()) {
        (*sess)[attrName] = attrValue;

        auto json = json_builder()
            .begin_object()
            .key("attrName")
            .value(attrName)
            .key("attrValue")
            .value(attrValue)
            .end_object()
            .build();

        response.set_ok();
        response.set_status_msg("OK");
        response.set_content_type(HTTP_CONTENT::JSON_APP);
        response.set_body(json->to_string());
    } else {
        response.set_bad_request();
        response.set_status_msg("Bad Request");
        response.set_content_type(HTTP_CONTENT::JSON_APP);
        response.set_body(R"({"error":"Missing attribute name"})");
    }
}

inline void handle_cookie_api(
    http_request& request, http_response& response) {
    if (request.method().is_post()) {
        HTTP_COOKIE_NAME name;
        string value, max_age_str;
        string content_type = request.content_type();

        if (content_type.find(HTTP_CONTENT::JSON_APP.content()) != string::npos) {
            try {
                auto root = json_parser(request.body()).parse();
                if (root && root->is_object()) {
                    const json_object* obj = root->as_object();

                    const json_value* nameVal = obj->get_member("name");
                    const json_value* valueVal = obj->get_member("value");
                    const json_value* maxAgeVal = obj->get_member("maxAge");

                    if (nameVal && nameVal->is_string()) {
                        name = HTTP_COOKIE_NAME(nameVal->as_string()->get_value());
                    }
                    if (valueVal && valueVal->is_string()) {
                        value = valueVal->as_string()->get_value();
                    }
                    if (maxAgeVal) {
                        if (maxAgeVal->is_string()) {
                            max_age_str = maxAgeVal->as_string()->get_value();
                        } else if (maxAgeVal->is_number()) {
                            max_age_str = _MSTL to_string(maxAgeVal->as_number()->get_value());
                        }
                    }
                }
            } catch (const exception& e) {
                println("JSON parse error:", e.what());
            }
        } else {
            name = HTTP_COOKIE_NAME(request.parameter("name"));
            value = request.parameter("value");
            max_age_str = request.parameter("maxAge");
        }

        if (!name.cookie_name().empty()) {
            cookie ck(name, value);
            if (!max_age_str.empty()) {
                ck.set_max_age(_MSTL integer32::parse(max_age_str.view()));
            }
            response.add_cookie(_MSTL move(ck));

            auto json = json_builder()
                .begin_object()
                .key("name").value(name.cookie_name())
                .key("value").value(value)
                .end_object().build();

            response.set_ok();
            response.set_status_msg("OK");
            response.set_content_type(HTTP_CONTENT::JSON_APP);
            response.set_body(json->to_string());
        } else {
            response.set_bad_request();
            response.set_status_msg("Bad Request");
            response.set_content_type(HTTP_CONTENT::JSON_APP);
            response.set_body(R"({"error":"Missing cookie name"})");
        }
    }
    else if (request.method().is_delete()) {
        HTTP_COOKIE_NAME name(request.parameter("name"));
        if (!name.cookie_name().empty()) {
            cookie ck(name, "");
            ck.invalidate();
            response.add_cookie(_MSTL move(ck));

            response.set_ok();
            response.set_status_msg("OK");
            response.set_content_type(HTTP_CONTENT::JSON_APP);
            response.set_body(R"({"name":")" + name.to_string() + R"("})");
        } else {
            response.set_bad_request();
            response.set_status_msg("Bad Request");
            response.set_content_type(HTTP_CONTENT::JSON_APP);
            response.set_body(R"({"error":"Missing cookie name"})");
        }
    }
}

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
void test_log();
void test_ranges();
void test_sql();
void test_mysql();
void test_redis();
void test_postgre();
void test_dbpool();
void test_ext_tpool();
void test_tpool();
void test_dns();

#endif //TRY_H
