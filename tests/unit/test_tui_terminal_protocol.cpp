#include <NeForce/tui/terminal_protocol.hpp>
#include <gtest/gtest.h>
using namespace neforce;
using namespace neforce::tui;

TEST(terminal_protocol, detects_kitty) {
    const auto profile = detect_terminal_profile("xterm-kitty", "truecolor", "", "", "1", "");
    EXPECT_TRUE(profile.truecolor);
    EXPECT_FALSE(profile.ansi256);
    EXPECT_TRUE(profile.unicode);
    EXPECT_TRUE(profile.kitty_keyboard);
    EXPECT_TRUE(profile.focus_events);
    EXPECT_TRUE(profile.synchronized_output);
    EXPECT_TRUE(profile.kitty_graphics);
    EXPECT_FALSE(profile.sixel);
    EXPECT_FALSE(profile.tmux);
}

TEST(terminal_protocol, detects_wezterm) {
    const auto profile = detect_terminal_profile("xterm-256color", "", "WezTerm", "", "", "");
    EXPECT_TRUE(profile.truecolor);
    EXPECT_TRUE(profile.ansi256);
    EXPECT_TRUE(profile.unicode);
    EXPECT_FALSE(profile.kitty_keyboard);
    EXPECT_TRUE(profile.focus_events);
    EXPECT_TRUE(profile.synchronized_output);
    EXPECT_FALSE(profile.kitty_graphics);
}

TEST(terminal_protocol, detects_tmux) {
    const auto profile =
            detect_terminal_profile("screen-256color", "truecolor", "WezTerm", "/tmp/tmux-1000/default,12,0", "", "");
    EXPECT_TRUE(profile.tmux);
    EXPECT_TRUE(profile.truecolor);
    EXPECT_TRUE(profile.ansi256);
    EXPECT_TRUE(profile.unicode);
    EXPECT_FALSE(profile.synchronized_output);
    EXPECT_TRUE(profile.focus_events);
}

TEST(terminal_protocol, detects_vscode) {
    const auto profile = detect_terminal_profile("xterm-256color", "", "vscode", "", "", "");
    EXPECT_TRUE(profile.truecolor);
    EXPECT_TRUE(profile.ansi256);
    EXPECT_TRUE(profile.synchronized_output);
    EXPECT_FALSE(profile.kitty_graphics);
    EXPECT_FALSE(profile.sixel);
}

TEST(terminal_protocol, detects_windows_terminal) {
    const auto profile = detect_terminal_profile("xterm-256color", "", "", "", "", "session-id");
    EXPECT_TRUE(profile.truecolor);
    EXPECT_TRUE(profile.synchronized_output);
    EXPECT_FALSE(profile.tmux);
    EXPECT_FALSE(profile.kitty_keyboard);
}

TEST(terminal_protocol, detects_dumb) {
    const auto profile = detect_terminal_profile("dumb", "", "", "", "", "");
    EXPECT_FALSE(profile.truecolor);
    EXPECT_FALSE(profile.ansi256);
    EXPECT_FALSE(profile.unicode);
    EXPECT_FALSE(profile.kitty_keyboard);
    EXPECT_FALSE(profile.focus_events);
    EXPECT_FALSE(profile.synchronized_output);
    EXPECT_FALSE(profile.kitty_graphics);
    EXPECT_FALSE(profile.sixel);
}

TEST(terminal_protocol, detects_cons25_focus_events) {
    const auto profile = detect_terminal_profile("cons25", "", "", "", "", "");
    EXPECT_FALSE(profile.focus_events);
    EXPECT_TRUE(profile.unicode);
}

TEST(terminal_protocol, detects_other_terminal_families) {
    const auto warp = detect_terminal_profile("xterm-256color", "", "WarpTerminal", "", "", "");
    EXPECT_TRUE(warp.truecolor);
    EXPECT_FALSE(warp.synchronized_output);

    const auto iterm = detect_terminal_profile("xterm-256color", "", "iTerm.app", "", "", "");
    EXPECT_TRUE(iterm.truecolor);
    EXPECT_TRUE(iterm.synchronized_output);

    const auto ghostty = detect_terminal_profile("xterm-256color", "", "ghostty", "", "", "");
    EXPECT_TRUE(ghostty.truecolor);
    EXPECT_TRUE(ghostty.synchronized_output);

    const auto alacritty = detect_terminal_profile("alacritty", "", "", "", "", "");
    EXPECT_TRUE(alacritty.synchronized_output);

    const auto foot = detect_terminal_profile("foot", "", "", "", "", "");
    EXPECT_TRUE(foot.synchronized_output);
}

TEST(terminal_protocol, detects_colorterm_and_sixel) {
    const auto truecolor = detect_terminal_profile("xterm", "24bit", "", "", "", "");
    EXPECT_TRUE(truecolor.truecolor);
    EXPECT_FALSE(truecolor.ansi256);

    const auto sixel = detect_terminal_profile("xterm-sixel", "", "", "", "", "");
    EXPECT_TRUE(sixel.sixel);
    EXPECT_TRUE(sixel.unicode);

    const auto tmux_kitty = detect_terminal_profile("xterm-kitty", "truecolor", "", "1", "7", "");
    EXPECT_TRUE(tmux_kitty.kitty_graphics);
    EXPECT_TRUE(tmux_kitty.kitty_keyboard);
    EXPECT_TRUE(tmux_kitty.tmux);
    EXPECT_FALSE(tmux_kitty.synchronized_output);
}

TEST(terminal_protocol, builds_osc11_query) { EXPECT_EQ(build_osc11_query(), string("\033]11;?\a")); }

TEST(terminal_protocol, parses_osc11_rgb_reply) {
    const auto parsed = parse_osc11_reply("\033]11;rgb:ffff/0000/8080\a");
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->R(), 255);
    EXPECT_EQ(parsed->G(), 0);
    EXPECT_EQ(parsed->B(), 128);
    EXPECT_EQ(parsed->A(), 255);
}

TEST(terminal_protocol, parses_osc11_hex_reply) {
    const auto parsed = parse_osc11_reply("\033]11;#1e1e2e\a");
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->R(), 30);
    EXPECT_EQ(parsed->G(), 30);
    EXPECT_EQ(parsed->B(), 46);
}

TEST(terminal_protocol, parses_osc11_st_terminator_and_whitespace) {
    const auto parsed = parse_osc11_reply("  \033]11;RGB:ABCD/EF01/2345\033\\  ");
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->R(), 0xAB);
    EXPECT_EQ(parsed->G(), 0xEF);
    EXPECT_EQ(parsed->B(), 0x23);
}

TEST(terminal_protocol, parses_osc11_short_components) {
    const auto parsed = parse_osc11_reply("\033]11;rgb:ff/00/80\a");
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->R(), 255);
    EXPECT_EQ(parsed->G(), 0);
    EXPECT_EQ(parsed->B(), 128);
}

TEST(terminal_protocol, rejects_malformed_osc11_replies) {
    EXPECT_FALSE(parse_osc11_reply("").has_value());
    EXPECT_FALSE(parse_osc11_reply("garbage").has_value());
    EXPECT_FALSE(parse_osc11_reply("\033]10;#123456\a").has_value());
    EXPECT_FALSE(parse_osc11_reply("\033]11;rgb:zz/00/00\a").has_value());
    EXPECT_FALSE(parse_osc11_reply("\033]11;rgb:00/00\a").has_value());
    EXPECT_FALSE(parse_osc11_reply("\033]11;rgb:0000/0000/0000/0000\a").has_value());
    EXPECT_FALSE(parse_osc11_reply("\033]11;#12345\a").has_value());
    EXPECT_FALSE(parse_osc11_reply("\033]11;#1234567\a").has_value());
}

TEST(terminal_protocol, light_background_threshold) {
    EXPECT_FALSE(is_light_background(color(140, 140, 140)));
    EXPECT_TRUE(is_light_background(color(141, 141, 141)));
    EXPECT_FALSE(is_light_background(color(0, 0, 0)));
    EXPECT_TRUE(is_light_background(color(255, 255, 255)));
    EXPECT_FALSE(is_light_background(color(0, 128, 0)));
}

TEST(terminal_protocol, builds_notifications) {
    EXPECT_EQ(build_notification("Build", "OK", notify_method::osc777), string("\033]777;notify;Build;OK\a"));
    EXPECT_EQ(build_notification("", "body", notify_method::osc777), string("\033]777;notify;;body\a"));
    EXPECT_EQ(build_notification("Build", "OK", notify_method::osc99), string("\033]99;;Build\a\033]99;;OK\a"));
    EXPECT_EQ(build_notification("Build", "OK", notify_method::osc9), string("\033]9;Build: OK\a"));
    EXPECT_EQ(build_notification("", "Done", notify_method::osc9), string("\033]9;Done\a"));
    EXPECT_EQ(build_notification("Build", "OK", notify_method::bel), string("\a"));
    EXPECT_EQ(build_notification("Build", "OK", notify_method::windows), string("\a"));
    EXPECT_EQ(build_notification("Build", "OK"), build_notification("Build", "OK", notify_method::osc9));
}

TEST(terminal_protocol, builds_cursor_shapes) {
    EXPECT_EQ(build_cursor_shape(cursor_shape::default_), string("\033[0 q"));
    EXPECT_EQ(build_cursor_shape(cursor_shape::block_blink), string("\033[1 q"));
    EXPECT_EQ(build_cursor_shape(cursor_shape::block), string("\033[2 q"));
    EXPECT_EQ(build_cursor_shape(cursor_shape::underline_blink), string("\033[3 q"));
    EXPECT_EQ(build_cursor_shape(cursor_shape::underline), string("\033[4 q"));
    EXPECT_EQ(build_cursor_shape(cursor_shape::bar_blink), string("\033[5 q"));
    EXPECT_EQ(build_cursor_shape(cursor_shape::bar), string("\033[6 q"));
}

TEST(terminal_protocol, builds_focus_events) {
    EXPECT_EQ(build_focus_events(true), string("\033[?1004h"));
    EXPECT_EQ(build_focus_events(false), string("\033[?1004l"));
}

TEST(terminal_protocol, builds_kitty_keyboard) {
    EXPECT_EQ(build_kitty_keyboard(true), string("\033[>1u"));
    EXPECT_EQ(build_kitty_keyboard(true, 3), string("\033[>3u"));
    EXPECT_EQ(build_kitty_keyboard(true, 7), string("\033[>7u"));
    EXPECT_EQ(build_kitty_keyboard(false), string("\033[<u"));
    EXPECT_EQ(build_kitty_keyboard(false, 7), string("\033[<u"));
}
