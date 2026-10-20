#ifndef NEFORCE_TUI_RECORDER_HPP__
#define NEFORCE_TUI_RECORDER_HPP__

/**
 * @file recorder.hpp
 * @brief 会话录制与回放
 *
 * 把一次交互（输入字节 + 帧快照）录成文本，
 * 回放时喂给 `headless` 并逐帧比对，用于交互回归测试与问题复现。
 */

#include "NeForce/core/file/path.hpp"
#include "NeForce/tui/headless.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 回放报告
 */
struct replay_report {
    size_t events = 0;               ///< 处理的记录条数
    size_t inputs = 0;               ///< 注入的输入次数
    size_t frames = 0;               ///< 比对的帧数
    size_t mismatches = 0;           ///< 帧不一致的次数
    vector<size_t> mismatch_events;  ///< 不一致事件在记录中的序号
    vector<string> mismatch_details; ///< 前若干条不一致的说明

    /**
     * @brief 回放是否完全一致
     * @return 是否无差异
     */
    NEFORCE_NODISCARD bool ok() const noexcept { return mismatches == 0; }
};

/**
 * @brief 会话录制器
 *
 * 记录顺序与交互一致：先记输入，再记一次输入处理后的帧快照。
 * 序列化格式为每行一条记录的 JSON 文本：
 *
 * @code
 * {"t":"begin","name":"chat","cols":80,"rows":24}
 * {"t":"in","s":3,"v":"a"}
 * {"t":"frame","s":4,"v":"..."}
 * {"t":"mark","s":5,"v":"submitted"}
 * @endcode
 */
class NEFORCE_API recorder {
public:
    /**
     * @brief 构造函数
     * @param name 会话名
     */
    explicit recorder(string name = string());

    /**
     * @brief 开始一次录制
     * @param name 会话名
     * @param columns 终端列数
     * @param rows 终端行数
     */
    void begin(string name, int columns = 0, int rows = 0);

    /**
     * @brief 记录一段输入
     * @param bytes 输入字节
     */
    void record_input(string_view bytes);

    /**
     * @brief 记录一次帧快照
     * @param plaintext 帧的纯文本
     */
    void record_frame(string_view plaintext);

    /**
     * @brief 记一个语义标记
     * @param name 标记名
     */
    void mark(string name);

    /**
     * @brief 序列化为 JSON 文本
     * @return JSON 文本
     */
    NEFORCE_NODISCARD string serialize() const;

    /**
     * @brief 写入文件
     * @param file_path 目标路径
     * @return 是否写入成功
     */
    NEFORCE_NODISCARD bool save(const path& file_path) const;

    /**
     * @brief 会话名
     * @return 会话名
     */
    NEFORCE_NODISCARD const string& name() const noexcept { return name_; }

    /**
     * @brief 记录条数
     * @return 条数
     */
    NEFORCE_NODISCARD size_t size() const noexcept { return records_.size(); }

    /**
     * @brief 已记录帧数
     * @return 帧数
     */
    NEFORCE_NODISCARD size_t frame_count() const noexcept { return frames_; }

    /**
     * @brief 已记录输入数
     * @return 输入数
     */
    NEFORCE_NODISCARD size_t input_count() const noexcept { return inputs_; }

    /**
     * @brief 清空
     */
    void clear();

    /**
     * @brief 回放录制的文本
     * @param text 序列化文本
     * @param app 目标 headless 驱动
     * @param max_details 最多保留的不一致说明条数
     * @return 回放报告
     */
    NEFORCE_NODISCARD static replay_report replay(string_view text, headless& app, size_t max_details = 3);

    /**
     * @brief 回放录制文件
     * @param file_path 录制文件
     * @param app 目标 headless 驱动
     * @param max_details 最多保留的不一致说明条数
     * @return 回放报告
     */
    NEFORCE_NODISCARD static replay_report replay_file(const path& file_path, headless& app, size_t max_details = 3);

private:
    /**
     * @brief 单条记录
     */
    struct record {
        char kind = 'i'; ///< `b`egin / `i`nput / `f`rame / `m`ark
        string value;    ///< 载荷
    };

    string name_;
    vector<record> records_;
    size_t inputs_ = 0;
    size_t frames_ = 0;
    int columns_ = 0;
    int rows_ = 0;
};

/** @} */ // TUI

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_RECORDER_HPP__
