#ifndef NEFORCE_TUI_IMAGE_HPP__
#define NEFORCE_TUI_IMAGE_HPP__

/**
 * @file image.hpp
 * @brief 终端图形协议
 *
 * 提供 PNG 解码、Kitty / iTerm2 / Sixel 图形协议编码，
 * 以及根据终端环境探测可用图形协议的功能。
 */

#include "NeForce/core/string/string.hpp"
#include "NeForce/core/utility/optional.hpp"
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_TUI__

/**
 * @addtogroup TUI TUI
 * @{
 */

/**
 * @brief 终端图形协议
 */
enum class image_protocol : uint8_t {
    none = 0, ///< 不支持
    kitty,    ///< Kitty graphics protocol (APC G)
    iterm2,   ///< iTerm2 inline images (OSC 1337)
    sixel,    ///< DEC sixel
};

/**
 * @brief 已解码的 RGBA 图像
 */
struct image_source {
    int width = 0;       ///< 像素宽
    int height = 0;      ///< 像素高
    vector<byte_t> rgba; ///< 行优先 RGBA

    /**
     * @brief 判断图像数据是否完整有效
     * @return 宽高均为正且 rgba 长度与宽高匹配时返回 true
     */
    NEFORCE_NODISCARD bool valid() const noexcept {
        if (width <= 0 || height <= 0 || rgba.size() % 4 != 0) {
            return false;
        }
        const size_t pixels = rgba.size() / 4;
        const auto w = static_cast<size_t>(width);
        return pixels % w == 0 && pixels / w == static_cast<size_t>(height);
    }
};

/**
 * @brief 目标单元格盒
 */
struct image_cell_box {
    int columns = 0;      ///< 列数
    int rows = 0;         ///< 行数
    int pixel_width = 0;  ///< 单元格像素宽（0 表示未知）
    int pixel_height = 0; ///< 单元格像素高（0 表示未知）
};

/**
 * @brief 解码 PNG 图像
 * @param bytes PNG 文件字节
 * @return 解码成功返回 RGBA 图像，失败返回 none
 *
 * 仅支持 8 位 RGB（颜色类型 2）与 RGBA（颜色类型 6）的 PNG，
 * 调色板、灰度、非 8 位深度与 Adam7 隔行扫描均会被拒绝。
 *
 * @note 本函数不校验数据块 CRC，遇到未知数据块时直接跳过。
 * @warning 仅解析单帧 PNG，APNG 的附加数据块会被忽略。
 */
NEFORCE_API optional<image_source> decode_png(string_view bytes);

/**
 * @brief 探测终端支持的图形协议
 * @param term TERM 环境变量
 * @param term_program TERM_PROGRAM 环境变量
 * @param kitty_window_id KITTY_WINDOW_ID 环境变量
 * @param sixel_support 终端 sixel 支持标志
 * @return 探测到的协议，无可用协议时返回 image_protocol::none
 *
 * 优先级依次为 Kitty、iTerm2、Sixel。
 */
NEFORCE_API image_protocol detect_image_protocol(string_view term, string_view term_program,
                                                 string_view kitty_window_id, string_view sixel_support);

/**
 * @brief 编码 Kitty 图形协议命令
 * @param image 源图像
 * @param box 目标单元格盒
 * @param image_id 图像 ID
 * @param transmit 是否传输图形数据；为 false 时仅生成显示已有图像的放置命令
 * @return 控制序列；图像无效时返回空字符串
 *
 * 图像数据以 Base64 编码并按每块不超过 4096 字节分块发送。
 *
 * @note 第一块携带完整参数与 q=2，最后一块的 m 为 0。
 */
NEFORCE_API string encode_kitty(const image_source& image, const image_cell_box& box, uint32_t image_id, bool transmit);

/**
 * @brief 编码 iTerm2 内联图像命令
 * @param image 源图像
 * @param box 目标单元格盒
 * @return 控制序列；图像无效时返回空字符串
 *
 * 使用 OSC 1337 序列，并将 RGBA 数据以 Base64 编码内联。
 */
NEFORCE_API string encode_iterm2(const image_source& image, const image_cell_box& box);

/**
 * @brief 编码 DEC sixel 图像
 * @param image 源图像
 * @param box 目标单元格盒
 * @return 控制序列；图像无效时返回空字符串
 *
 * 使用固定的 6x6x6 颜色立方与灰度调色板量化颜色，
 * 每六个像素行组成一个 sixel 波段，并采用游程编码压缩。
 *
 * @note 完全透明的像素不会被绘制。
 */
NEFORCE_API string encode_sixel(const image_source& image, const image_cell_box& box);

/** @} */

NEFORCE_END_TUI__
NEFORCE_END_NAMESPACE__
#endif // NEFORCE_TUI_IMAGE_HPP__
