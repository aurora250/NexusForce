#ifndef TRY_H
#define TRY_H
#include <MSTL/mstlc++.hpp>
USE_MSTL

void test_file();
void test_datetimes();
void test_print();
void test_rnd();
void test_hex();
void test_enctype();

class example_servlet final : public servlet {
public:
    example_servlet(const int id) : servlet(id) {
        set_session_cookie_name(HTTP_COOKIE::SESSIONID);
        auto cors_filt = ::new cors_filter();
        auto log_filt = ::new logging_filter();
        add_filter(cors_filt);
        add_filter(log_filt);
    }

    void do_get(http_request& request, http_response& response) override {
        do_post(request, response);
    }

    void do_post(http_request& request, http_response& response) override {
        const string& path = request.get_path();
        if (path == "/old-link") {
            response.redirect("/new-link");
            return;
        }
        if (path == "/forward-me") {
            response.forward("/forward-target");
            return;
        }
        if (path == "/forward-target") {
            response.set_ok();
            response.set_body("Forward Successfully");
            return;
        }

        if (path.ends_with(".css") || path.ends_with(".jpg")) {
            response.set_ok();
            if (path.ends_with(".css")) {
                response.set_content_type(HTTP_CONTENT::CSS_TEXT);
                response.set_body(_MSTL move(file::read("./resource" + path)));
            } else if (path.ends_with(".jpg")) {
                response.set_content_type(HTTP_CONTENT::JPEG_IMG);
                response.set_body(_MSTL move(file::read_binary("./resource" + path)));
            }
            return;
        }

        if (path == "/api/session") {
            handle_session_api(request, response);
            return;
        }
        if (path == "/api/session-attribute") {
            handle_session_attribute(request, response);
            return;
        }
        if (path == "/api/cookie") {
            handle_cookie_api(request, response);
            return;
        }
        if (path == "/api/logger-test") {
            response.set_ok();
            response.set_body("Logging filter test successful");
            return;
        }
        if (path == "/api/data") {
            response.set_ok();
            response.set_content_type(HTTP_CONTENT::JSON_APP);
            response.set_body(R"({"status":"success"})");
            return;
        }

        if (path == "/") {
            response.set_ok();
            response.set_status_msg("OK");
            static file f("./resource/index.html");
            response.set_body(_MSTL move(f.read()));
        } else if (path == "/detail") {
            response.set_ok();
            response.set_status_msg("OK");
            static file f("./resource/detail.html");
            response.set_body(_MSTL move(f.read()));
        } else if (path == "/new-link") {
            response.set_ok();
            response.set_status_msg("OK");
            static file f("./resource/index.html");
            response.set_body(_MSTL move(f.read()));
        } else if (path == "/test") {
            response.set_ok();
            response.set_status_msg("OK");
            static file f("./resource/test.html");
            response.set_body(_MSTL move(f.read()));
        } else {
            static file f("./resource/404err.html");
            response.set_body(_MSTL move(f.read()));
        }
    }

    void do_options(http_request& request, http_response& response) override {
        response.set_status(HTTP_STATUS::S2_NO_CONTENT);
        response.set_allow_origin("http://127.0.0.1:5500");
        response.set_allow_credentials(true);
        response.set_allow_method(HTTP_METHOD::GET & HTTP_METHOD::POST & HTTP_METHOD::OPTIONS);
        response.set_allow_headers("Content-Type, Cookie, Accept, X-Requested-With");
        response.set_max_age(86400);
    }

private:
    void handle_session_api(http_request& request, http_response& response) {
        session* session = get_session(request);
        string action = request.get_parameter("action");

        if (action == "create") {
            session = get_session(request, true);
            response.set_ok();
            response.set_content_type(HTTP_CONTENT::JSON_APP);
            response.set_body(R"({"sessionId":")" + session->get_id() + R"("})");
        }
        else if (action == "invalidate" && session) {
            session->invalidate();
            response.set_ok();
            response.set_body(R"({"message":"Session invalidated"})");
        }
        else if (action == "info") {
            if (session) {
                const auto json = json_builder()
                    .begin_object()
                    .key("sessionId").value(session->get_id())
                    .key("createTime").value(session->get_create_time().to_iso_utc())
                    .key("lastAccess").value(session->get_last_access().to_iso_utc())
                    .key("attributes").value(session->get_data())
                    .end_object().build();

                response.set_ok();
                response.set_content_type(HTTP_CONTENT::JSON_APP);
                response.set_body(json_to_string(json));
            } else {
                response.set_bad_request();
                response.set_content_type(HTTP_CONTENT::JSON_APP);
                response.set_body( R"({"error":"No active session found"})");
            }
        }
        else {
            response.set_bad_request();
            response.set_content_type(HTTP_CONTENT::JSON_APP);
            response.set_body( R"({"error":"Invalid session action"})");
        }
    }

    void handle_session_attribute(http_request& request, http_response& response) {
        string attrName, attrValue;
        string content_type = request.get_content_type();
        if (content_type.find(HTTP_CONTENT::JSON_APP) == 0) {
            const auto root = json_parser(request.get_body()).parse();
            if (root->is_object()) {
                const json_object* obj = root->as_object();
                const json_value* attrNameVal = obj->get_member("attrName");
                if (attrNameVal && attrNameVal->is_string()) {
                    attrName = attrNameVal->as_string()->get_value();
                }
                const json_value* attrValueVal = obj->get_member("attrValue");
                if (attrValueVal && attrValueVal->is_string()) {
                    attrValue = attrValueVal->as_string()->get_value();
                }
            }
        } else {
            attrName = request.get_parameter("attrName");
            attrValue = request.get_parameter("attrValue");
        }

        session* session = get_session(request, true);

        if (!attrName.empty()) {
            (*session)[attrName] = attrValue;
            const auto json = json_builder()
                .begin_object()
                .key("attrName").value(attrName)
                .key("attrValue").value(attrValue)
                .end_object().build();

            response.set_ok();
            response.set_content_type(HTTP_CONTENT::JSON_APP);
            response.set_body(json_to_string(json));
        } else {
            response.set_bad_request();
            response.set_body(R"({"error":"Missing attribute name"})");
        }
    }

    void handle_cookie_api(http_request& request, http_response& response) {
        if (request.get_method().is_post()) {
            string name, value, max_age_str;
            string content_type = request.get_content_type();

            if (content_type.find(HTTP_CONTENT::JSON_APP) != string::npos) {
                auto root = json_parser(request.get_body()).parse();
                if (root && root->is_object()) {
                    const json_object* obj = root->as_object();
                    const json_value* nameVal = obj->get_member("name");
                    const json_value* valueVal = obj->get_member("value");
                    const json_value* maxAgeVal = obj->get_member("maxAge");

                    if (nameVal && nameVal->is_string()) name = nameVal->as_string()->get_value();
                    if (valueVal && valueVal->is_string()) value = valueVal->as_string()->get_value();
                    if (maxAgeVal) {
                        if (maxAgeVal->is_string()) max_age_str = maxAgeVal->as_string()->get_value();
                        else if (maxAgeVal->is_number()) max_age_str = _MSTL to_string(maxAgeVal->as_number()->get_value());
                    }
                }
            } else {
                name = request.get_parameter("name");
                value = request.get_parameter("value");
                max_age_str = request.get_parameter("maxAge");
            }

            if (!name.empty()) {
                cookie cookie(name, value);
                if (!max_age_str.empty()) {
                    cookie.set_max_age(_MSTL to_int32(max_age_str.c_str()));
                }
                response.add_cookie(cookie);

                auto json = json_builder()
                    .begin_object()
                    .key("name").value(name)
                    .key("value").value(value)
                    .end_object().build();

                response.set_ok();
                response.set_content_type(HTTP_CONTENT::JSON_APP);
                response.set_body(json_to_string(json));
            } else {
                response.set_bad_request();
                response.set_content_type(HTTP_CONTENT::JSON_APP);
                response.set_body(R"({"error":"Missing cookie name"})");
            }
        }
        else if (request.get_method().is_delete()) {
            string name = request.get_parameter("name");
            if (!name.empty()) {
                cookie cookie(name, "");
                cookie.set_max_age(0);
                response.add_cookie(cookie);

                response.set_ok();
                response.set_body(R"({"name":")" + name + R"("})");
            } else {
                response.set_bad_request();
                response.set_body(R"({"error":"Missing cookie name"})");
            }
        }
    }
};

void test_serv();

void test_list();
void test_exce();
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
void test_hash();
void test_math();

struct Person {
    string name;
    int age;
};

template <>
struct MSTL::printer <Person> {
    static void print (const Person& p) {
        std::cout << p.name << " : " << p.age << " ";
    }
    static void print_feature(const Person& p) {
        printer::print(p);
    }
};

void test_sort();
void test_variant();
void test_string();

void test_option();

void test_any();
void test_timer();
void test_dbpool();
void test_tpool();
void test_dns();

#endif //TRY_H
