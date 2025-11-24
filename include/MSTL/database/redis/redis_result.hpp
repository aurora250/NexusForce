#ifndef MSTL_DATABASE_REDIS_RESULT_HPP__
#define MSTL_DATABASE_REDIS_RESULT_HPP__
#ifdef MSTL_SUPPORT_REDIS__
#include "../../core/config/undef_cmacro.hpp"
#include "MSTL/database/db_interface.hpp"
#include "redis_config.hpp"
MSTL_BEGIN_NAMESPACE__

struct MSTL_API redis_result final : idb_kv_result {
private:
    _MSTL_REDIS redisReply* reply_ = nullptr;
    size_type cursor_ = 0;
    size_type rows_ = 0;
    unique_ptr<vector<string>> column_names_ =
        make_unique<vector<string>>();
    unique_ptr<vector<pair<string, string>>> kv_pairs_ =
        make_unique<vector<pair<string, string>>>();

    size_type kv_cursor_ = 0;
    bool is_array_ = false;

    static string format_redis_reply_element(_MSTL_REDIS redisReply* element);
    void process_reply();
    string get_string() const;

public:
    redis_result() noexcept = default;

    explicit redis_result(_MSTL_REDIS redisReply* reply) noexcept
    : reply_(reply) {
        process_reply();
    }

    ~redis_result() override {
        if (reply_) _MSTL_REDIS freeReplyObject(reply_);
    }

    MSTL_NODISCARD bool empty() const noexcept override { return !reply_ || (rows_ == 0 && kv_pairs_->empty()); }
    MSTL_NODISCARD bool next() noexcept override;

    MSTL_NODISCARD string_view key() const noexcept override;
    MSTL_NODISCARD string_view value() const noexcept override;

    MSTL_NODISCARD bool value_bool() const override;
    MSTL_NODISCARD int64_t value_int64() const override;
    MSTL_NODISCARD double value_double() const override;
    MSTL_NODISCARD vector<string> value_array() const override;
    MSTL_NODISCARD const vector<pair<string, string>>& value_hash() const override { return *kv_pairs_; }

    MSTL_NODISCARD int type() const noexcept { return reply_ ? reply_->type : -1; }
    MSTL_NODISCARD bool is_nil() const noexcept { return reply_ && reply_->type == REDIS_REPLY_NIL; }
};

MSTL_END_NAMESPACE__
#endif
#endif // MSTL_DATABASE_REDIS_RESULT_HPP__
