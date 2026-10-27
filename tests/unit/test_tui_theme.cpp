#include <NeForce/core/file/filesystem.hpp>
#include <NeForce/tui/dom/theme.hpp>
#include <gtest/gtest.h>
using namespace neforce;
using namespace neforce::tui;


TEST(TuiThemeRoleTest, NameRoundTrip) {
    EXPECT_EQ(theme_role_count, 81u);
    EXPECT_EQ(theme_role_name(theme_role::auto_accept), "autoAccept");
    EXPECT_EQ(theme_role_name(theme_role::prompt_border), "promptBorder");
    EXPECT_EQ(theme_role_name(theme_role::rate_limit_fill), "rate_limit_fill");
    EXPECT_EQ(theme_role_name(static_cast<theme_role>(200)), string_view{});
}

TEST(TuiThemeRoleTest, ParseName) {
    const auto role = parse_theme_role("toolCardBackground");
    ASSERT_TRUE(role.has_value());
    EXPECT_EQ(*role, theme_role::tool_card_background);
    EXPECT_EQ(*parse_theme_role("ACCENT"), theme_role::accent);
    EXPECT_FALSE(parse_theme_role("no-such-role").has_value());
}

TEST(TuiThemeRoleTest, PaletteNames) {
    EXPECT_EQ(palette_name(theme_palette_id::dark), "dark");
    EXPECT_EQ(palette_name(theme_palette_id::pink_night), "pink-night");
    EXPECT_EQ(palette_name(theme_palette_id::count_), string_view{});
}

TEST(TuiThemePaletteTest, DarkPaletteMatchesUpstream) {
    const theme t = make_palette(theme_palette_id::dark);
    ASSERT_TRUE(t.has_palette);
    EXPECT_EQ(t.get(theme_role::accent).to_string(), "7DA1DEFF");
    EXPECT_EQ(t.get(theme_role::success).to_string(), "82B89DFF");
    EXPECT_EQ(t.get(theme_role::syntax_keyword).to_string(), "78A0D6FF");
    EXPECT_EQ(t.get(theme_role::diff_added).to_string(), "27392CFF");
}

TEST(TuiThemePaletteTest, LightPaletteMatchesUpstream) {
    const theme t = make_palette(theme_palette_id::light);
    ASSERT_TRUE(t.has_palette);
    EXPECT_EQ(t.get(theme_role::text).to_string(), "343945FF");
    EXPECT_EQ(t.get(theme_role::accent).to_string(), "3F6CC4FF");
}

TEST(TuiThemePaletteTest, AllPalettesFillEveryRole) {
    for (size_t i = 2; i < static_cast<size_t>(theme_palette_id::count_); ++i) {
        const theme t = make_palette(static_cast<theme_palette_id>(i));
        for (size_t r = 0; r < theme_role_count; ++r) {
            const color c = t.get(static_cast<theme_role>(r));
            const auto role = static_cast<theme_role>(r);
            if (role == theme_role::cursor || role == theme_role::user_message_background) {
                continue;
            }
            EXPECT_EQ(c.A(), 255) << "palette " << i << " role " << r;
        }
    }
}

TEST(TuiThemePaletteTest, LegacyFieldsFollowPalette) {
    const theme t = make_palette(theme_palette_id::dark);
    EXPECT_EQ(t.bg.A(), 0u);
    EXPECT_EQ(t.fg.to_string(), t.get(theme_role::text).to_string());
    EXPECT_EQ(t.primary.to_string(), t.get(theme_role::accent).to_string());
    EXPECT_EQ(t.danger.to_string(), t.get(theme_role::error).to_string());
    EXPECT_EQ(t.border.to_string(), t.get(theme_role::prompt_border).to_string());
}

TEST(TuiThemePaletteTest, LegacyPresetsKeepOldColors) {
    EXPECT_EQ(dark_theme.bg.to_string(), "101010FF");
    EXPECT_EQ(dark_theme.primary.to_string(), color::cyan().to_string());
    EXPECT_FALSE(dark_theme.has_palette);
    EXPECT_EQ(dark_theme.get(theme_role::accent).to_string(), color::cyan().to_string());
    EXPECT_EQ(dark_theme.get(theme_role::error).to_string(), color::red().to_string());
}

TEST(TuiThemePaletteTest, MakePaletteByName) {
    ASSERT_TRUE(make_palette(string_view{"pink-day"}).has_value());
    EXPECT_FALSE(make_palette(string_view{"nope"}).has_value());
}

TEST(TuiThemeSetTest, SetOnLegacyThemeMaterializes) {
    theme t = dark_theme;
    t.set(theme_role::accent, color{1, 2, 3});
    EXPECT_TRUE(t.has_palette);
    EXPECT_EQ(t.get(theme_role::accent).to_string(), "010203FF");
    EXPECT_EQ(t.primary.to_string(), "010203FF");
    EXPECT_EQ(t.get(theme_role::text).to_string(), dark_theme.fg.to_string());
}

TEST(TuiThemeSetTest, SetOutOfRangeIsIgnored) {
    theme t = make_palette(theme_palette_id::light);
    const color before = t.get(theme_role::accent);
    t.set(static_cast<theme_role>(200), color{1, 2, 3});
    EXPECT_EQ(t.get(theme_role::accent).to_string(), before.to_string());
}

TEST(TuiThemeColorTest, LightenAndDarken) {
    const color base{100, 100, 100};
    EXPECT_EQ(lighten(base, 0.0).to_string(), "646464FF");
    EXPECT_EQ(lighten(base, 1.0).to_string(), "FFFFFFFF");
    EXPECT_EQ(darken(base, 1.0).to_string(), "000000FF");
    EXPECT_EQ(lighten(base, 0.5).to_string(), "B2B2B2FF");
}

TEST(TuiThemeColorTest, ParseHex) {
    const auto short_hex = parse_color(string_view{"#abc"});
    ASSERT_TRUE(short_hex.has_value());
    EXPECT_EQ(short_hex->to_string(), "AABBCCFF");

    const auto full = parse_color(string_view{"#112233"});
    ASSERT_TRUE(full.has_value());
    EXPECT_EQ(full->to_string(), "112233FF");

    const auto with_alpha = parse_color(string_view{"#11223344"});
    ASSERT_TRUE(with_alpha.has_value());
    EXPECT_EQ(with_alpha->A(), 0x44u);
}

TEST(TuiThemeColorTest, ParseFunctionForms) {
    const auto rgb_value = parse_color(string_view{"rgb(10, 20, 30)"});
    ASSERT_TRUE(rgb_value.has_value());
    EXPECT_EQ(rgb_value->to_string(), "0A141EFF");

    const auto ansi256 = parse_color(string_view{"ansi256(196)"});
    ASSERT_TRUE(ansi256.has_value());
    EXPECT_EQ(ansi256->to_string(), "FF0000FF");

    const auto named = parse_color(string_view{"ansi:cyan"});
    ASSERT_TRUE(named.has_value());
    EXPECT_EQ(named->to_string(), "008080FF");
}

TEST(TuiThemeColorTest, ParseRejectsGarbage) {
    EXPECT_FALSE(parse_color(string_view{""}).has_value());
    EXPECT_FALSE(parse_color(string_view{"#12"}).has_value());
    EXPECT_FALSE(parse_color(string_view{"rgb(1,2)"}).has_value());
    EXPECT_FALSE(parse_color(string_view{"ansi256(300)"}).has_value());
    EXPECT_FALSE(parse_color(string_view{"ansi:nope"}).has_value());
}

TEST(TuiThemeCatalogTest, BuiltinNames) {
    auto& catalog = theme_catalog::instance();
    const vector<string> names = catalog.names();
    ASSERT_GE(names.size(), 7u);
    EXPECT_EQ(names[0], string{"auto"});
    EXPECT_EQ(names[1], string{"dark"});
    EXPECT_EQ(names[6], string{"pink-ansi"});
}

TEST(TuiThemeCatalogTest, AutoFollowsTerminalBackground) {
    auto& catalog = theme_catalog::instance();
    catalog.set_terminal_light(false);
    EXPECT_EQ(catalog.resolve("auto").get(theme_role::accent).to_string(),
              catalog.resolve("dark").get(theme_role::accent).to_string());
    catalog.set_terminal_light(true);
    EXPECT_EQ(catalog.resolve("auto").get(theme_role::accent).to_string(),
              catalog.resolve("light").get(theme_role::accent).to_string());
    catalog.set_terminal_light(false);
}

TEST(TuiThemeCatalogTest, UnknownResolvesToBase) {
    auto& catalog = theme_catalog::instance();
    EXPECT_EQ(catalog.resolve("does-not-exist").get(theme_role::accent).to_string(),
              catalog.resolve("dark").get(theme_role::accent).to_string());
}

TEST(TuiThemeCatalogTest, RegisterUserTheme) {
    auto& catalog = theme_catalog::instance();
    catalog.clear_user_themes();
    const string document =
            R"({"name":"sakura","displayName":"Sakura","base":"dark","colors":{"accent":"#FF9EC7","text":"#E8E6E0"}})";
    ASSERT_TRUE(catalog.register_theme(document.view()));
    ASSERT_EQ(catalog.user_theme_count(), 1u);

    const auto found = catalog.find("sakura");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->get(theme_role::accent).to_string(), "FF9EC7FF");
    EXPECT_EQ(found->get(theme_role::accent).to_string(), "FF9EC7FF");

    EXPECT_EQ(catalog.names().back(), string{"sakura"});
    catalog.clear_user_themes();
}

TEST(TuiThemeCatalogTest, RegisterRejectsBadDocuments) {
    auto& catalog = theme_catalog::instance();
    catalog.clear_user_themes();
    EXPECT_FALSE(catalog.register_theme(string_view{"{"}));
    EXPECT_FALSE(catalog.register_theme(string_view{R"({"base":"nope"})"}));
    EXPECT_FALSE(catalog.register_theme(string_view{R"({"base":"dark"})"}));
    EXPECT_FALSE(catalog.register_theme(string_view{R"({"name":"auto","base":"dark"})"}));
    EXPECT_EQ(catalog.user_theme_count(), 0u);
}

TEST(TuiThemeCatalogTest, RegisterIgnoresUnknownColorKeys) {
    auto& catalog = theme_catalog::instance();
    catalog.clear_user_themes();
    const string document =
            R"({"name":"x","base":"light","colors":{"accent":"#010203","nope":"#FFFFFF","text":"bad"}})";
    ASSERT_TRUE(catalog.register_theme(document.view()));
    const auto found = catalog.find("x");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->get(theme_role::accent).to_string(), "010203FF");
    EXPECT_EQ(found->get(theme_role::text).to_string(),
              make_palette(theme_palette_id::light).get(theme_role::text).to_string());
    catalog.clear_user_themes();
}

TEST(TuiThemeCatalogTest, SameNameOverwrites) {
    auto& catalog = theme_catalog::instance();
    catalog.clear_user_themes();
    ASSERT_TRUE(catalog.register_theme(string_view{R"({"name":"dup","base":"dark","colors":{"accent":"#000001"}})"}));
    ASSERT_TRUE(catalog.register_theme(string_view{R"({"name":"dup","base":"dark","colors":{"accent":"#000002"}})"}));
    EXPECT_EQ(catalog.user_theme_count(), 1u);
    EXPECT_EQ(catalog.find("dup")->get(theme_role::accent).to_string(), "000002FF");
    catalog.clear_user_themes();
}

TEST(TuiThemeCatalogTest, LoadUserThemesFromDirectory) {
    auto& catalog = theme_catalog::instance();
    catalog.clear_user_themes();

    const path directory = filesystem::temp_directory() / "neforce_theme_catalog_scan";
    filesystem::remove_all(directory);
    ASSERT_TRUE(filesystem::create_directories(directory));
    ASSERT_TRUE(filesystem::create_and_write(directory / "first.json",
                                             R"({"name":"scanned","base":"dark","colors":{"accent":"#123456"}})"));
    ASSERT_TRUE(filesystem::create_and_write(directory / ".hidden.json", R"({"name":"hidden","base":"dark"})"));
    ASSERT_TRUE(filesystem::create_and_write(directory / "notes.txt", "not a theme"));

    EXPECT_TRUE(catalog.load_user_themes(directory.view()));
    EXPECT_EQ(catalog.user_theme_count(), 2u);
    const auto found = catalog.find("scanned");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->get(theme_role::accent).to_string(), "123456FF");
    EXPECT_TRUE(catalog.find("hidden").has_value());

    filesystem::remove_all(directory);
    catalog.clear_user_themes();
}
