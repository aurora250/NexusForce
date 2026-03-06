#ifndef NEFORCE_DATABASE_REDIS_RESULT_HPP__
#define NEFORCE_DATABASE_REDIS_RESULT_HPP__
#ifdef NEFORCE_SUPPORT_HIREDIS
#include "NeForce/db/db_interface.hpp"
#include <hiredis/hiredis.h>
NEFORCE_BEGIN_NAMESPACE__

struct NEFORCE_API redis_result final : idb_kv_result {
private:
    ::redisReply* reply_ = nullptr;
    size_type cursor_ = 0;
    size_type rows_ = 0;
    unique_ptr<vector<string>> column_names_ =
        make_unique<vector<string>>();
    unique_ptr<vector<pair<string, string>>> kv_pairs_ =
        make_unique<vector<pair<string, string>>>();

    size_type kv_cursor_ = 0;
    bool is_array_ = false;

    static string format_redis_reply_element(::redisReply* element);
    void process_reply();
    string get_string() const;

public:
    redis_result() noexcept = default;

    explicit redis_result(::redisReply* reply) noexcept
    : reply_(reply) {
        process_reply();
    }

    ~redis_result() override {
        if (reply_) ::freeReplyObject(reply_);
    }

    NEFORCE_NODISCARD bool empty() const noexcept override { return !reply_ || (rows_ == 0 && kv_pairs_->empty()); }
    NEFORCE_NODISCARD bool next() noexcept override;

    NEFORCE_NODISCARD string_view key() const noexcept override;
    NEFORCE_NODISCARD string_view value() const noexcept override;

    NEFORCE_NODISCARD bool value_bool() const override;
    NEFORCE_NODISCARD int64_t value_int64() const override;
    NEFORCE_NODISCARD double value_double() const override;
    NEFORCE_NODISCARD vector<string> value_array() const override;
    NEFORCE_NODISCARD const vector<pair<string, string>>& value_hash() const override { return *kv_pairs_; }

    NEFORCE_NODISCARD int type() const noexcept { return reply_ ? reply_->type : -1; }
    NEFORCE_NODISCARD bool is_nil() const noexcept { return reply_ && reply_->type == REDIS_REPLY_NIL; }
};

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_DATABASE_REDIS_RESULT_HPP__
