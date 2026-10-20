#include <NeForce/core/file/file.hpp>
#include <NeForce/core/file/json/json_builder.hpp>
#include <NeForce/core/file/json/json_parser.hpp>
#include <NeForce/core/string/to_string.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <NeForce/tui/recorder.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

namespace {
    string normalize_frame(string text) {
        while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) {
            text.pop_back();
        }
        return text;
    }

    const char* kind_name(const char kind) {
        switch (kind) {
            case 'b': {
                return "begin";
            }
            case 'i': {
                return "in";
            }
            case 'f': {
                return "frame";
            }
            default: {
                return "mark";
            }
        }
    }

    char kind_of(const string_view name) {
        if (name == "begin"_sv) {
            return 'b';
        }
        if (name == "in"_sv) {
            return 'i';
        }
        if (name == "frame"_sv) {
            return 'f';
        }
        return 'm';
    }
} // namespace


recorder::recorder(string name) :
name_(move(name)) {}

void recorder::begin(string name, const int columns, const int rows) {
    name_ = move(name);
    columns_ = columns;
    rows_ = rows;
    records_.clear();
    inputs_ = 0;
    frames_ = 0;
    records_.push_back(record{'b', name_});
}

void recorder::record_input(const string_view bytes) {
    if (bytes.empty()) {
        return;
    }
    records_.push_back(record{'i', string(bytes)});
    ++inputs_;
}

void recorder::record_frame(const string_view plaintext) {
    records_.push_back(record{'f', string(plaintext)});
    ++frames_;
}

void recorder::mark(string name) { records_.push_back(record{'m', move(name)}); }

void recorder::clear() {
    records_.clear();
    inputs_ = 0;
    frames_ = 0;
}

string recorder::serialize() const {
    json_builder builder;
    builder.begin_object();
    builder.key("name");
    builder.value(name_.view());
    builder.key("cols");
    builder.value(static_cast<int64_t>(columns_));
    builder.key("rows");
    builder.value(static_cast<int64_t>(rows_));

    builder.key("records");
    builder.begin_array();
    for (const auto& item: records_) {
        builder.begin_object();
        builder.key("t");
        builder.value(kind_name(item.kind));
        builder.key("v");
        builder.value(item.value.view());
        builder.end_object();
    }
    builder.end_array();
    builder.end_object();

    const auto root = builder.build();
    return root->to_string();
}

bool recorder::save(const path& file_path) const {
    file out(file_path, false, file_access::read_write, file_shared::share_read, file_creation::create_force);
    if (!out.is_opened()) {
        return false;
    }
    out.write(serialize());
    return true;
}

replay_report recorder::replay(const string_view text, headless& app, const size_t max_details) {
    replay_report report;

    unique_ptr<json_value> root;
    try {
        const string document(text);
        json_parser parser(document);
        root = parser.parse();
    } catch (const exception&) {
        ++report.mismatches;
        report.mismatch_details.push_back("recording is not valid JSON");
        return report;
    }

    const auto* object = root != nullptr ? root->as_object() : nullptr;
    if (object == nullptr) {
        ++report.mismatches;
        report.mismatch_details.push_back("recording root is not an object");
        return report;
    }

    if (const auto* records = object->get_member("records"); records != nullptr) {
        const auto* array = records->as_array();
        if (array == nullptr) {
            ++report.mismatches;
            report.mismatch_details.push_back("recording has no records array");
            return report;
        }

        size_t seq = 0;
        for (size_t i = 0; i < array->size(); ++i) {
            const auto* entry = array->get_element(i);
            const auto* item = entry != nullptr ? entry->as_object() : nullptr;
            if (item == nullptr) {
                continue;
            }
            const auto* kind_value = item->get_member("t");
            const auto* payload = item->get_member("v");
            const string kind = kind_value != nullptr && kind_value->as_string() != nullptr
                                        ? kind_value->as_string()->get_value()
                                        : string();
            const string value = payload != nullptr && payload->as_string() != nullptr
                                         ? payload->as_string()->get_value()
                                         : string();

            ++report.events;
            if (kind_of(kind.view()) == 'i') {
                app.feed(value.view());
                app.tick();
                ++report.inputs;
            } else if (kind_of(kind.view()) == 'f') {
                const string expected = normalize_frame(value);
                const string actual = normalize_frame(app.plaintext());
                ++report.frames;
                if (expected != actual) {
                    ++report.mismatches;
                    report.mismatch_events.push_back(seq);
                    if (report.mismatch_details.size() < max_details) {
                        string detail = "frame #";
                        detail += to_string(static_cast<int64_t>(seq));
                        detail += "\n--- expected ---\n";
                        detail += expected;
                        detail += "\n--- actual ---\n";
                        detail += actual;
                        report.mismatch_details.push_back(move(detail));
                    }
                }
            }
            ++seq;
        }
    }

    return report;
}

replay_report recorder::replay_file(const path& file_path, headless& app, const size_t max_details) {
    file in(file_path);
    if (!in.is_opened()) {
        replay_report report;
        ++report.mismatches;
        report.mismatch_details.push_back("cannot open recording");
        return report;
    }
    return replay(in.read_binary().view(), app, max_details);
}

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
