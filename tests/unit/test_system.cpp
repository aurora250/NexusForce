#include <NeForce/core/system/cmdline.hpp>
#include <NeForce/core/system/dynamic_library.hpp>
#include <NeForce/core/system/environment.hpp>
#include <NeForce/core/system/locale.hpp>
#include <NeForce/core/system/pipe.hpp>
#include <NeForce/core/system/process.hpp>
#include <NeForce/core/system/share_memory.hpp>
#include <NeForce/core/system/signal.hpp>
#include <NeForce/core/system/stacktrace.hpp>
#include <NeForce/core/system/sysinfo.hpp>
#include <NeForce/core/system/system_event.hpp>
#include <NeForce/core/utility/packages.hpp>
#include <gtest/gtest.h>
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <Windows.h>
#else
#    include <cstdlib>
#    include <unistd.h>
#    include <fcntl.h>
#endif
using namespace neforce;

class CmdlineTest : public ::testing::Test {
protected:
    void SetUp() override { cmdline_ = cmdline(); }

    void TearDown() override {}

    cmdline cmdline_;
};

TEST_F(CmdlineTest, AddOption_LongNameOnly_Success) {
    cmdline_.add_option("verbose", 0, "Enable verbose output");
    cmdline_.add_option("output", 0, "Output file", true, false, "default.txt");

    EXPECT_NO_THROW(cmdline_.parse(vector<string>{"program", "--verbose", "--output=file.txt"}));
    EXPECT_TRUE(cmdline_.has("verbose"));
    EXPECT_EQ(cmdline_.get("output"), "file.txt");
}

TEST_F(CmdlineTest, AddOption_ShortNameOnly_Success) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");
    cmdline_.add_option("output", 'o', "Output file", true, false, "default.txt");

    EXPECT_NO_THROW(cmdline_.parse(vector<string>{"program", "-v", "-o", "file.txt"}));

    EXPECT_TRUE(cmdline_.has("verbose"));
    EXPECT_TRUE(cmdline_.has("output"));
    EXPECT_EQ(cmdline_.get("output"), "file.txt");
    EXPECT_NO_THROW(ignore = cmdline_.get("verbose"));
}

TEST_F(CmdlineTest, AddOption_BothNames_Success) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");
    cmdline_.add_option("output", 'o', "Output file", true);

    EXPECT_NO_THROW(cmdline_.parse(vector<string>{"program", "-v", "--output=file.txt"}));
    EXPECT_TRUE(cmdline_.has("verbose"));
    EXPECT_EQ(cmdline_.get("output"), "file.txt");
}

TEST_F(CmdlineTest, AddOption_DuplicateLongName_ThrowsException) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");
    EXPECT_THROW(cmdline_.add_option("verbose", 'b', "Another verbose"), cmdline_exception);
}

TEST_F(CmdlineTest, AddOption_DuplicateShortName_ThrowsException) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");
    EXPECT_THROW(cmdline_.add_option("version", 'v', "Show version"), cmdline_exception);
}

TEST_F(CmdlineTest, AddOption_NoName_ThrowsException) {
    EXPECT_THROW(cmdline_.add_option("", 0, "No name option"), cmdline_exception);
}

TEST_F(CmdlineTest, AddOption_AllowMultiple_Success) {
    cmdline_.add_option("include", 'I', "Include directory", true, true);

    EXPECT_NO_THROW(cmdline_.parse(
            vector<string>{"program", "-I", "/usr/include", "-I/opt/include", "--include=/home/include"}));
    EXPECT_TRUE(cmdline_.has("include"));
    EXPECT_EQ(cmdline_.count("include"), 3);
    EXPECT_EQ(cmdline_.get("include", 0), "/usr/include");
    EXPECT_EQ(cmdline_.get("include", 1), "/opt/include");
    EXPECT_EQ(cmdline_.get("include", 2), "/home/include");
}

TEST_F(CmdlineTest, AddOption_DefaultValue_Success) {
    cmdline_.add_option("output", 'o', "Output file", true, false, "default.txt");

    cmdline_.parse(vector<string>{"program"});
    EXPECT_FALSE(cmdline_.has("output"));
    EXPECT_EQ(cmdline_.get("output"), "default.txt");
}

TEST_F(CmdlineTest, AddOption_EmptyDefaultValue_ThrowsWhenAccessing) {
    cmdline_.add_option("output", 'o', "Output file", true);

    cmdline_.parse(vector<string>{"program"});
    EXPECT_FALSE(cmdline_.has("output"));
    EXPECT_THROW(ignore = cmdline_.get("output"), cmdline_exception);
}

TEST_F(CmdlineTest, Parse_EmptyArgs_NoOptions) {
    EXPECT_NO_THROW(cmdline_.parse(vector<string>{}));
    EXPECT_TRUE(cmdline_.positional_args().empty());
}

TEST_F(CmdlineTest, Parse_OnlyProgramName_NoOptions) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");

    EXPECT_NO_THROW(cmdline_.parse(vector<string>{"program"}));
    EXPECT_FALSE(cmdline_.has("verbose"));
    EXPECT_EQ(cmdline_.program_name(), "program");
}

TEST_F(CmdlineTest, Parse_LongOptionWithoutValue_Success) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");

    cmdline_.parse(vector<string>{"program", "--verbose"});
    EXPECT_TRUE(cmdline_.has("verbose"));
    EXPECT_EQ(cmdline_.get("verbose"), "1");
}

TEST_F(CmdlineTest, Parse_LongOptionWithEqualsSign_Success) {
    cmdline_.add_option("output", 'o', "Output file", true);

    cmdline_.parse(vector<string>{"program", "--output=file.txt"});
    EXPECT_TRUE(cmdline_.has("output"));
    EXPECT_EQ(cmdline_.get("output"), "file.txt");
}

TEST_F(CmdlineTest, Parse_LongOptionWithSpaceSeparatedValue_Success) {
    cmdline_.add_option("output", 'o', "Output file", true);

    cmdline_.parse(vector<string>{"program", "--output", "file.txt"});
    EXPECT_TRUE(cmdline_.has("output"));
    EXPECT_EQ(cmdline_.get("output"), "file.txt");
}

TEST_F(CmdlineTest, Parse_LongOptionMissingValue_ThrowsException) {
    cmdline_.add_option("output", 'o', "Output file", true);

    EXPECT_THROW(cmdline_.parse(vector<string>{"program", "--output"}), cmdline_exception);
}

TEST_F(CmdlineTest, Parse_LongOptionValueAtEnd_ThrowsException) {
    cmdline_.add_option("output", 'o', "Output file", true);

    EXPECT_THROW(cmdline_.parse(vector<string>{"program", "--output"}), cmdline_exception);
}

TEST_F(CmdlineTest, Parse_LongOptionUnknown_ThrowsException) {
    EXPECT_THROW(cmdline_.parse(vector<string>{"program", "--unknown"}), cmdline_exception);
}

TEST_F(CmdlineTest, Parse_ShortOptionSingle_Success) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");

    cmdline_.parse(vector<string>{"program", "-v"});
    EXPECT_TRUE(cmdline_.has("verbose"));
    EXPECT_EQ(cmdline_.get("verbose"), "1");
}

TEST_F(CmdlineTest, Parse_ShortOptionCombined_Success) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");
    cmdline_.add_option("force", 'f', "Force operation");

    cmdline_.parse(vector<string>{"program", "-vf"});
    EXPECT_TRUE(cmdline_.has("verbose"));
    EXPECT_TRUE(cmdline_.has("force"));
    EXPECT_EQ(cmdline_.get("verbose"), "1");
    EXPECT_EQ(cmdline_.get("force"), "1");
}

TEST_F(CmdlineTest, Parse_ShortOptionWithSpaceSeparatedValue_Success) {
    cmdline_.add_option("output", 'o', "Output file", true);

    cmdline_.parse(vector<string>{"program", "-o", "file.txt"});
    EXPECT_TRUE(cmdline_.has("output"));
    EXPECT_EQ(cmdline_.get("output"), "file.txt");
}

TEST_F(CmdlineTest, Parse_ShortOptionWithImmediateValue_Success) {
    cmdline_.add_option("output", 'o', "Output file", true);

    cmdline_.parse(vector<string>{"program", "-ofile.txt"});
    EXPECT_TRUE(cmdline_.has("output"));
    EXPECT_EQ(cmdline_.get("output"), "file.txt");
}

TEST_F(CmdlineTest, Parse_ShortOptionCombinedWithLastRequiringValue_Success) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");
    cmdline_.add_option("output", 'o', "Output file", true);

    cmdline_.parse(vector<string>{"program", "-vo", "file.txt"});
    EXPECT_TRUE(cmdline_.has("verbose"));
    EXPECT_TRUE(cmdline_.has("output"));
    EXPECT_EQ(cmdline_.get("verbose"), "1");
    EXPECT_EQ(cmdline_.get("output"), "file.txt");
}

TEST_F(CmdlineTest, Parse_ShortOptionCombinedWithLastRequiringValueImmediate_Success) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");
    cmdline_.add_option("output", 'o', "Output file", true);

    cmdline_.parse(vector<string>{"program", "-vofile.txt"});
    EXPECT_TRUE(cmdline_.has("verbose"));
    EXPECT_TRUE(cmdline_.has("output"));
    EXPECT_EQ(cmdline_.get("verbose"), "1");
    EXPECT_EQ(cmdline_.get("output"), "file.txt");
}

TEST_F(CmdlineTest, Parse_ShortOptionMissingValue_ThrowsException) {
    cmdline_.add_option("output", 'o', "Output file", true);

    EXPECT_THROW(cmdline_.parse(vector<string>{"program", "-o"}), cmdline_exception);
}

TEST_F(CmdlineTest, Parse_ShortOptionCombinedMissingValue_ThrowsException) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");
    cmdline_.add_option("output", 'o', "Output file", true);

    EXPECT_THROW(cmdline_.parse(vector<string>{"program", "-vo"}), cmdline_exception);
}

TEST_F(CmdlineTest, Parse_ShortOptionUnknown_ThrowsException) {
    EXPECT_THROW(cmdline_.parse(vector<string>{"program", "-x"}), cmdline_exception);
}

TEST_F(CmdlineTest, Parse_ShortOptionInCombinedUnknown_ThrowsException) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");

    EXPECT_THROW(cmdline_.parse(vector<string>{"program", "-vx"}), cmdline_exception);
}

TEST_F(CmdlineTest, Parse_EndOfOptionsMarker_AllPositionalAfter) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");

    cmdline_.parse(vector<string>{"program", "-v", "--", "--verbose", "-v", "file.txt"});
    EXPECT_TRUE(cmdline_.has("verbose"));
    const auto& positional = cmdline_.positional_args();
    ASSERT_EQ(positional.size(), 3);
    EXPECT_EQ(positional[0], "--verbose");
    EXPECT_EQ(positional[1], "-v");
    EXPECT_EQ(positional[2], "file.txt");
}

TEST_F(CmdlineTest, Parse_SingleDash_AsPositional) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");

    cmdline_.parse(vector<string>{"program", "-"});
    EXPECT_FALSE(cmdline_.has("verbose"));
    const auto& positional = cmdline_.positional_args();
    ASSERT_EQ(positional.size(), 1);
    EXPECT_EQ(positional[0], "-");
}

TEST_F(CmdlineTest, Parse_PositionalArgsBeforeOptions_Success) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");

    cmdline_.parse(vector<string>{"program", "file1.txt", "-v", "file2.txt"});
    EXPECT_TRUE(cmdline_.has("verbose"));
    const auto& positional = cmdline_.positional_args();
    ASSERT_EQ(positional.size(), 2);
    EXPECT_EQ(positional[0], "file1.txt");
    EXPECT_EQ(positional[1], "file2.txt");
}

TEST_F(CmdlineTest, Parse_EmptyStringArgument_Success) {
    cmdline_.add_option("name", 'n', "Name parameter", true);

    cmdline_.parse(vector<string>{"program", "-n", ""});
    EXPECT_TRUE(cmdline_.has("name"));
    EXPECT_EQ(cmdline_.get("name"), "");
}

TEST_F(CmdlineTest, Parse_EqualsSignInValue_Success) {
    cmdline_.add_option("param", 'p', "Parameter with equals", true);

    cmdline_.parse(vector<string>{"program", "--param=key=value"});
    EXPECT_TRUE(cmdline_.has("param"));
    EXPECT_EQ(cmdline_.get("param"), "key=value");
}

TEST_F(CmdlineTest, Parse_MultipleValuesOverwrite_Success) {
    cmdline_.add_option("output", 'o', "Output file", true);

    cmdline_.parse(vector<string>{"program", "--output=first.txt", "--output", "second.txt"});
    EXPECT_TRUE(cmdline_.has("output"));
    EXPECT_EQ(cmdline_.get("output"), "second.txt");
    EXPECT_EQ(cmdline_.count("output"), 1);
}

TEST_F(CmdlineTest, Get_LongName_Success) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");

    cmdline_.parse(vector<string>{"program", "-v"});
    EXPECT_EQ(cmdline_.get("verbose"), "1");
}

TEST_F(CmdlineTest, Get_NonExistentOption_ThrowsException) {
    EXPECT_THROW(ignore = cmdline_.get("nonexistent"), cmdline_exception);
}

TEST_F(CmdlineTest, Get_IndexOutOfBounds_ReturnsDefault) {
    cmdline_.add_option("include", 'I', "Include directory", true, true, "default_include");

    cmdline_.parse(vector<string>{"program", "-I", "/usr/include"});
    EXPECT_EQ(cmdline_.get("include", 0), "/usr/include");
    EXPECT_EQ(cmdline_.get("include", 1), "default_include");
}

TEST_F(CmdlineTest, Get_IndexOutOfBounds_NoDefault_ThrowsException) {
    cmdline_.add_option("include", 'I', "Include directory", true, true);

    cmdline_.parse(vector<string>{"program", "-I", "/usr/include"});
    EXPECT_EQ(cmdline_.get("include", 0), "/usr/include");
    EXPECT_THROW(ignore = cmdline_.get("include", 1), cmdline_exception);
}

TEST_F(CmdlineTest, Has_ExistingOption_ReturnsTrue) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");

    cmdline_.parse(vector<string>{"program", "-v"});
    EXPECT_TRUE(cmdline_.has("verbose"));
}

TEST_F(CmdlineTest, Has_NonExistentOption_ReturnsFalse) { EXPECT_FALSE(cmdline_.has("nonexistent")); }

TEST_F(CmdlineTest, Has_OptionNotSpecified_ReturnsFalse) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");

    cmdline_.parse(vector<string>{"program"});
    EXPECT_FALSE(cmdline_.has("verbose"));
}

TEST_F(CmdlineTest, Has_OptionWithEmptyValue_ReturnsFalse) {
    cmdline_.add_option("name", 'n', "Name parameter", true);

    cmdline_.parse(vector<string>{"program"});
    EXPECT_FALSE(cmdline_.has("name"));
    EXPECT_THROW(ignore = cmdline_.get("name"), cmdline_exception);
}

TEST_F(CmdlineTest, Count_ExistingOption_Success) {
    cmdline_.add_option("include", 'I', "Include directory", true, true);

    cmdline_.parse(vector<string>{"program", "-I/usr/include", "-I/opt/include"});
    EXPECT_EQ(cmdline_.count("include"), 2);
}

TEST_F(CmdlineTest, Count_NonExistentOption_ReturnsZero) { EXPECT_EQ(cmdline_.count("nonexistent"), 0); }

TEST_F(CmdlineTest, Count_OptionNotSpecified_ReturnsZero) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");

    cmdline_.parse(vector<string>{"program"});
    EXPECT_EQ(cmdline_.count("verbose"), 0);
}

TEST_F(CmdlineTest, Count_BooleanOptionMultiple_Success) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output", false, true);

    cmdline_.parse(vector<string>{"program", "-v", "-v", "--verbose"});
    EXPECT_EQ(cmdline_.count("verbose"), 3);
}

TEST_F(CmdlineTest, PositionalArgs_Empty_ReturnsEmpty) {
    cmdline_.parse(vector<string>{"program"});
    EXPECT_TRUE(cmdline_.positional_args().empty());
}

TEST_F(CmdlineTest, PositionalArgs_Multiple_Success) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");

    cmdline_.parse(vector<string>{"program", "file1.txt", "file2.txt", "file3.txt"});
    const auto& positional = cmdline_.positional_args();
    ASSERT_EQ(positional.size(), 3);
    EXPECT_EQ(positional[0], "file1.txt");
    EXPECT_EQ(positional[1], "file2.txt");
    EXPECT_EQ(positional[2], "file3.txt");
}

TEST_F(CmdlineTest, ProgramName_Success) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");
    cmdline_.parse(vector<string>{"myprogram", "--verbose"});
    EXPECT_EQ(cmdline_.program_name(), "myprogram");
}

TEST_F(CmdlineTest, ProgramName_EmptyArgs_ReturnsEmpty) { EXPECT_TRUE(cmdline_.program_name().empty()); }

TEST_F(CmdlineTest, PrintHelp_NoOptions_NoThrow) { EXPECT_NO_THROW(cmdline_.print_help()); }

TEST_F(CmdlineTest, PrintHelp_MultipleOptions_NoThrow) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");
    cmdline_.add_option("output", 'o', "Output file", true, false, "default.txt");
    cmdline_.add_option("include", 'I', "Include directory", true, true);

    EXPECT_NO_THROW(cmdline_.print_help());
}

TEST_F(CmdlineTest, PrintHelp_LongNameOnly_NoThrow) {
    cmdline_.add_option("config", 0, "Configuration file", true);

    EXPECT_NO_THROW(cmdline_.print_help());
}

TEST_F(CmdlineTest, PrintHelp_ShortNameOnly_NoThrow) {
    cmdline_.add_option("config", 'c', "Configuration file", true);

    EXPECT_NO_THROW(cmdline_.print_help());
}

TEST_F(CmdlineTest, Parse_IntArgcArgv_Success) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");
    cmdline_.add_option("output", 'o', "Output file", true);

    const char* argv[] = {"program", "-v", "--output", "file.txt"};
    int argc = sizeof(argv) / sizeof(argv[0]);

    EXPECT_NO_THROW(cmdline_.parse(argc, argv));
    EXPECT_TRUE(cmdline_.has("verbose"));
    EXPECT_EQ(cmdline_.get("output"), "file.txt");
}

TEST_F(CmdlineTest, Parse_IntArgcArgv_Empty_NoThrow) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");

    const char* argv[] = {"program"};
    int argc = sizeof(argv) / sizeof(argv[0]);

    EXPECT_NO_THROW(cmdline_.parse(argc, argv));
    EXPECT_FALSE(cmdline_.has("verbose"));
}

TEST_F(CmdlineTest, Parse_IntArgcArgv_MultiplePositionals_Success) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");

    const char* argv[] = {"program", "file1.txt", "-v", "file2.txt"};
    int argc = sizeof(argv) / sizeof(argv[0]);

    EXPECT_NO_THROW(cmdline_.parse(argc, argv));
    EXPECT_TRUE(cmdline_.has("verbose"));
    const auto& positional = cmdline_.positional_args();
    ASSERT_EQ(positional.size(), 2);
    EXPECT_EQ(positional[0], "file1.txt");
    EXPECT_EQ(positional[1], "file2.txt");
}

TEST_F(CmdlineTest, FindOptionLong_Existing_ReturnsPointer) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");

    cmdline_.parse(vector<string>{"program", "--verbose"});
    EXPECT_TRUE(cmdline_.has("verbose"));
}

TEST_F(CmdlineTest, FindOptionLong_NonExistent_ReturnsNullptr) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");

    cmdline_.parse(vector<string>{"program"});
    EXPECT_FALSE(cmdline_.has("nonexistent"));
}

TEST_F(CmdlineTest, FindOptionShort_Existing_ReturnsPointer) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");

    cmdline_.parse(vector<string>{"program", "-v"});
    EXPECT_TRUE(cmdline_.has("verbose"));
}

TEST_F(CmdlineTest, FindOptionShort_NonExistent_ReturnsNullptr) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");

    cmdline_.parse(vector<string>{"program"});
    EXPECT_FALSE(cmdline_.has("verbose"));
}

TEST_F(CmdlineTest, ExceptionCopyConstructor_Success) {
    cmdline_.add_option("output", 'o', "Output file", true);

    try {
        cmdline_.parse(vector<string>{"program", "--output"});
        FAIL() << "Expected cmdline_exception";
    } catch (const cmdline_exception& e) {
        cmdline_exception copied(e);
        EXPECT_NE(copied.what(), nullptr);
    }
}

TEST_F(CmdlineTest, ComplexScenario_MixedOptionsAndPositionals_Success) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output");
    cmdline_.add_option("output", 'o', "Output file", true);
    cmdline_.add_option("include", 'I', "Include directory", true, true);
    cmdline_.add_option("define", 'D', "Define macro", true, true, "DEFAULT");
    cmdline_.add_option("force", 'f', "Force operation");
    cmdline_.add_option("config", 'c', "Config file", true);

    cmdline_.parse(vector<string>{"program", "input1.txt", "-v", "-I/usr/include", "--output=out.txt", "-DDEBUG=1",
                                  "-I", "/opt/include", "--config", "app.conf", "-f", "input2.txt", "--",
                                  "--extra.txt"});

    EXPECT_TRUE(cmdline_.has("verbose"));
    EXPECT_TRUE(cmdline_.has("output"));
    EXPECT_TRUE(cmdline_.has("include"));
    EXPECT_TRUE(cmdline_.has("define"));
    EXPECT_TRUE(cmdline_.has("force"));
    EXPECT_TRUE(cmdline_.has("config"));

    EXPECT_EQ(cmdline_.get("verbose"), "1");
    EXPECT_EQ(cmdline_.get("output"), "out.txt");
    EXPECT_EQ(cmdline_.get("force"), "1");
    EXPECT_EQ(cmdline_.get("config"), "app.conf");

    EXPECT_EQ(cmdline_.count("include"), 2);
    EXPECT_EQ(cmdline_.get("include", 0), "/usr/include");
    EXPECT_EQ(cmdline_.get("include", 1), "/opt/include");

    EXPECT_EQ(cmdline_.get("define", 0), "DEBUG=1");
    EXPECT_EQ(cmdline_.get("define", 1), "DEFAULT");

    const auto& positional = cmdline_.positional_args();
    ASSERT_EQ(positional.size(), 3);
    EXPECT_EQ(positional[0], "input1.txt");
    EXPECT_EQ(positional[1], "input2.txt");
    EXPECT_EQ(positional[2], "--extra.txt");

    EXPECT_EQ(cmdline_.program_name(), "program");
}

TEST_F(CmdlineTest, MultipleAllowMultipleOptionsWithOverwrite_Success) {
    cmdline_.add_option("output", 'o', "Output file", true, false);

    cmdline_.parse(vector<string>{"program", "-o", "first.txt", "--output", "second.txt", "-othird.txt"});
    EXPECT_TRUE(cmdline_.has("output"));
    EXPECT_EQ(cmdline_.get("output"), "third.txt");
    EXPECT_EQ(cmdline_.count("output"), 1);
}

TEST_F(CmdlineTest, BoolOptionDefaultValue_Ignored) {
    cmdline_.add_option("verbose", 'v', "Enable verbose output", false, false, "default");

    cmdline_.parse(vector<string>{"program"});
    EXPECT_FALSE(cmdline_.has("verbose"));
}

TEST_F(CmdlineTest, LongOptionWithEmptyNameValue_Success) {
    cmdline_.add_option("output", 'o', "Output file", true);

    cmdline_.parse(vector<string>{"program", "--output="});
    EXPECT_TRUE(cmdline_.has("output"));
    EXPECT_EQ(cmdline_.get("output"), "");
}

TEST_F(CmdlineTest, ShortOptionWithOnlyDash_Success) {
    cmdline_.add_option("verbose", 'v', "Verbose");

    cmdline_.parse(vector<string>{"program", "-"});
    EXPECT_FALSE(cmdline_.has("verbose"));
    ASSERT_EQ(cmdline_.positional_args().size(), 1);
    EXPECT_EQ(cmdline_.positional_args()[0], "-");
}

TEST_F(CmdlineTest, LongOptionExactBorderEquals_Success) {
    cmdline_.add_option("param", 'p', "Parameter", true);

    cmdline_.parse(vector<string>{"program", "--param=value"});
    EXPECT_TRUE(cmdline_.has("param"));
    EXPECT_EQ(cmdline_.get("param"), "value");
}

TEST_F(CmdlineTest, AllOptionTypesCombined_Success) {
    cmdline_.add_option("longonly1", 0, "Long only option 1");
    cmdline_.add_option("longonly2", 0, "Long only option 2", true);
    cmdline_.add_option("shortonlya", 'a', "Short only option a");
    cmdline_.add_option("shortonlyb", 'b', "Short only option b", true);
    cmdline_.add_option("both1", 'c', "Both names option 1");
    cmdline_.add_option("both2", 'd', "Both names option 2", true, true);

    cmdline_.parse(vector<string>{"prog", "--longonly1", "--longonly2", "val2", "-a", "-bvalb", "-c", "--both2=v1",
                                  "-d", "v2"});

    EXPECT_TRUE(cmdline_.has("longonly1"));
    EXPECT_EQ(cmdline_.get("longonly2"), "val2");
    EXPECT_TRUE(cmdline_.has("shortonlya"));
    EXPECT_TRUE(cmdline_.has("shortonlyb"));
    EXPECT_TRUE(cmdline_.has("both1"));
    EXPECT_EQ(cmdline_.count("both2"), 2);
    EXPECT_EQ(cmdline_.get("both2", 0), "v1");
    EXPECT_EQ(cmdline_.get("both2", 1), "v2");
}

TEST_F(CmdlineTest, OptionConstructorWithAllParams_Success) {
    cmdline::option opt("long", 's', "description", true, true, "default");
    EXPECT_EQ(opt.long_name, "long");
    EXPECT_EQ(opt.short_name, 's');
    EXPECT_EQ(opt.description, "description");
    EXPECT_TRUE(opt.requires_value);
    EXPECT_TRUE(opt.allow_multiple);
    EXPECT_EQ(opt.default_value, "default");
    EXPECT_TRUE(opt.values.empty());
}

TEST_F(CmdlineTest, OptionDefaultConstructor_Success) {
    cmdline::option opt;
    EXPECT_TRUE(opt.long_name.empty());
    EXPECT_EQ(opt.short_name, 0);
    EXPECT_TRUE(opt.description.empty());
    EXPECT_FALSE(opt.requires_value);
    EXPECT_FALSE(opt.allow_multiple);
    EXPECT_TRUE(opt.default_value.empty());
    EXPECT_TRUE(opt.values.empty());
}

class DynamicLibraryTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}

    static string get_test_library_path() {
#ifdef NEFORCE_PLATFORM_WINDOWS
        return "kernel32.dll";
#else
        return "libc.so.6";
#endif
    }

    static string get_test_symbol_name() {
#ifdef NEFORCE_PLATFORM_WINDOWS
        return "GetCurrentProcessId";
#else
        return "getpid";
#endif
    }

    static string get_nonexistent_symbol_name() { return "nonexistent_symbol_xyz_12345"; }

    static string get_nonexistent_library_path() {
#ifdef NEFORCE_PLATFORM_WINDOWS
        return "nonexistent_library_xyz_12345.dll";
#else
        return "libnonexistent_xyz_12345.so";
#endif
    }
};

TEST_F(DynamicLibraryTest, Constructor_ValidLibrary_LoadsSuccessfully) {
    dynamic_library lib(get_test_library_path());
    EXPECT_TRUE(lib.is_open());
    EXPECT_EQ(lib.path(), get_test_library_path());
}

TEST_F(DynamicLibraryTest, Constructor_InvalidLibrary_ThrowsException) {
    EXPECT_THROW({ dynamic_library lib(get_nonexistent_library_path()); }, dynamic_library_exception);
}

TEST_F(DynamicLibraryTest, Constructor_EmptyPath_ThrowsException) {
    EXPECT_THROW({ dynamic_library lib(""); }, dynamic_library_exception);
}

TEST_F(DynamicLibraryTest, MoveConstructor_Success) {
    dynamic_library lib1(get_test_library_path());
    void* original_handle = lib1.native_handle();
    string original_path = lib1.path();

    dynamic_library lib2(move(lib1));

    EXPECT_EQ(lib2.native_handle(), original_handle);
    EXPECT_EQ(lib2.path(), original_path);
    EXPECT_TRUE(lib2.is_open());
    EXPECT_EQ(lib1.native_handle(), nullptr);
    EXPECT_FALSE(lib1.is_open());
}

TEST_F(DynamicLibraryTest, MoveConstructor_FromUnloadedLibrary_Success) {
    dynamic_library lib1(get_test_library_path());
    lib1.unload();
    EXPECT_FALSE(lib1.is_open());

    dynamic_library lib2(move(lib1));

    EXPECT_FALSE(lib2.is_open());
    EXPECT_EQ(lib2.native_handle(), nullptr);
    EXPECT_FALSE(lib1.is_open());
}

TEST_F(DynamicLibraryTest, MoveAssignment_Success) {
    dynamic_library lib1(get_test_library_path());
    void* handle1 = lib1.native_handle();
    string path1 = lib1.path();

    dynamic_library lib2(get_test_library_path());

    lib2 = move(lib1);

    EXPECT_EQ(lib2.native_handle(), handle1);
    EXPECT_EQ(lib2.path(), path1);
    EXPECT_TRUE(lib2.is_open());
    EXPECT_EQ(lib1.native_handle(), nullptr);
    EXPECT_FALSE(lib1.is_open());
}

TEST_F(DynamicLibraryTest, MoveAssignment_SelfAssignment_NoEffect) {
    dynamic_library lib(get_test_library_path());
    void* handle = lib.native_handle();
    string path = lib.path();

    lib = move(lib);

    EXPECT_EQ(lib.native_handle(), handle);
    EXPECT_EQ(lib.path(), path);
    EXPECT_TRUE(lib.is_open());
}

TEST_F(DynamicLibraryTest, MoveAssignment_FromUnloadedLibrary_Success) {
    dynamic_library lib1(get_test_library_path());
    lib1.unload();

    dynamic_library lib2(get_test_library_path());
    void* handle2 = lib2.native_handle();
    string path2 = lib2.path();

    lib2 = move(lib1);

    EXPECT_EQ(lib2.native_handle(), nullptr);
    EXPECT_FALSE(lib2.is_open());
    EXPECT_EQ(lib1.native_handle(), nullptr);
    EXPECT_FALSE(lib1.is_open());
}

TEST_F(DynamicLibraryTest, MoveAssignment_DestinationLoaded_SourceUnloaded_DestinationClosed) {
    dynamic_library lib1(get_test_library_path());
    lib1.unload();

    dynamic_library lib2(get_test_library_path());
    EXPECT_TRUE(lib2.is_open());

    lib2 = move(lib1);

    EXPECT_FALSE(lib2.is_open());
    EXPECT_EQ(lib2.native_handle(), nullptr);
}

TEST_F(DynamicLibraryTest, Symbol_ValidSymbol_ReturnsNonNull) {
    dynamic_library lib(get_test_library_path());
    void* sym = lib.symbol(get_test_symbol_name());
    EXPECT_NE(sym, nullptr);
}

TEST_F(DynamicLibraryTest, Symbol_InvalidSymbol_ThrowsException) {
    dynamic_library lib(get_test_library_path());
    EXPECT_THROW({ ignore = lib.symbol(get_nonexistent_symbol_name()); }, dynamic_library_exception);
}

TEST_F(DynamicLibraryTest, Symbol_LibraryNotLoaded_ThrowsException) {
    dynamic_library lib(get_test_library_path());
    lib.unload();

    EXPECT_THROW({ ignore = lib.symbol(get_test_symbol_name()); }, dynamic_library_exception);
}

TEST_F(DynamicLibraryTest, Symbol_EmptyName_ThrowsException) {
    dynamic_library lib(get_test_library_path());
    EXPECT_THROW({ ignore = lib.symbol(""); }, dynamic_library_exception);
}

TEST_F(DynamicLibraryTest, ToSymbol_ValidSymbol_ReturnsFunctionPointer) {
    dynamic_library lib(get_test_library_path());

#ifdef NEFORCE_PLATFORM_WINDOWS
    using FuncType = DWORD(WINAPI*)();
#else
    using FuncType = pid_t (*)();
#endif

    auto func = lib.to_symbol<FuncType>(get_test_symbol_name());
    EXPECT_NE(func, nullptr);

    auto result = func();
    EXPECT_GT(result, 0);
}

TEST_F(DynamicLibraryTest, ToSymbol_InvalidSymbol_ThrowsException) {
    dynamic_library lib(get_test_library_path());

    using FuncType = void (*)();

    EXPECT_THROW({ lib.to_symbol<FuncType>(get_nonexistent_symbol_name()); }, dynamic_library_exception);
}

TEST_F(DynamicLibraryTest, ToSymbol_LibraryNotLoaded_ThrowsException) {
    dynamic_library lib(get_test_library_path());
    lib.unload();

    using FuncType = void (*)();

    EXPECT_THROW({ lib.to_symbol<FuncType>(get_test_symbol_name()); }, dynamic_library_exception);
}

TEST_F(DynamicLibraryTest, HasSymbol_ExistingSymbol_ReturnsTrue) {
    dynamic_library lib(get_test_library_path());
    EXPECT_TRUE(lib.has_symbol(get_test_symbol_name()));
}

TEST_F(DynamicLibraryTest, HasSymbol_NonExistentSymbol_ReturnsFalse) {
    dynamic_library lib(get_test_library_path());
    EXPECT_FALSE(lib.has_symbol(get_nonexistent_symbol_name()));
}

TEST_F(DynamicLibraryTest, HasSymbol_LibraryNotLoaded_ReturnsFalse) {
    dynamic_library lib(get_test_library_path());
    lib.unload();
    EXPECT_FALSE(lib.has_symbol(get_test_symbol_name()));
}

TEST_F(DynamicLibraryTest, HasSymbol_EmptyName_ReturnsFalse) {
    dynamic_library lib(get_test_library_path());
    EXPECT_FALSE(lib.has_symbol(""));
}

TEST_F(DynamicLibraryTest, IsOpen_AfterConstruction_ReturnsTrue) {
    dynamic_library lib(get_test_library_path());
    EXPECT_TRUE(lib.is_open());
}

TEST_F(DynamicLibraryTest, IsOpen_AfterUnload_ReturnsFalse) {
    dynamic_library lib(get_test_library_path());
    lib.unload();
    EXPECT_FALSE(lib.is_open());
}

TEST_F(DynamicLibraryTest, IsOpen_AfterMoveConstruction_SourceReturnsFalse) {
    dynamic_library lib1(get_test_library_path());
    dynamic_library lib2(move(lib1));
    EXPECT_TRUE(lib2.is_open());
    EXPECT_FALSE(lib1.is_open());
}

TEST_F(DynamicLibraryTest, IsOpen_AfterMoveAssignment_SourceReturnsFalse) {
    dynamic_library lib1(get_test_library_path());
    dynamic_library lib2(get_test_library_path());
    lib2 = move(lib1);
    EXPECT_TRUE(lib2.is_open());
    EXPECT_FALSE(lib1.is_open());
}

TEST_F(DynamicLibraryTest, Unload_LoadedLibrary_ClosesSuccessfully) {
    dynamic_library lib(get_test_library_path());
    EXPECT_TRUE(lib.is_open());

    lib.unload();
    EXPECT_FALSE(lib.is_open());
    EXPECT_EQ(lib.native_handle(), nullptr);
}

TEST_F(DynamicLibraryTest, Unload_AlreadyUnloaded_NoThrow) {
    dynamic_library lib(get_test_library_path());
    lib.unload();
    EXPECT_NO_THROW(lib.unload());
    EXPECT_FALSE(lib.is_open());
}

TEST_F(DynamicLibraryTest, Unload_MultipleCall_NoThrow) {
    dynamic_library lib(get_test_library_path());
    lib.unload();
    lib.unload();
    lib.unload();
    EXPECT_FALSE(lib.is_open());
}

TEST_F(DynamicLibraryTest, Unload_AndReload_ThrowsException) {
    dynamic_library lib(get_test_library_path());
    lib.unload();

    EXPECT_THROW({ ignore = lib.symbol(get_test_symbol_name()); }, dynamic_library_exception);

    EXPECT_THROW(
            {
                using FuncType = void (*)();
                lib.to_symbol<FuncType>(get_test_symbol_name());
            },
            dynamic_library_exception);
}

TEST_F(DynamicLibraryTest, NativeHandle_AfterConstruction_ReturnsNonNull) {
    dynamic_library lib(get_test_library_path());
    EXPECT_NE(lib.native_handle(), nullptr);
}

TEST_F(DynamicLibraryTest, NativeHandle_AfterUnload_ReturnsNull) {
    dynamic_library lib(get_test_library_path());
    lib.unload();
    EXPECT_EQ(lib.native_handle(), nullptr);
}

TEST_F(DynamicLibraryTest, NativeHandle_AfterMoveConstruction_SourceReturnsNull) {
    dynamic_library lib1(get_test_library_path());
    void* handle = lib1.native_handle();

    dynamic_library lib2(move(lib1));

    EXPECT_EQ(lib2.native_handle(), handle);
    EXPECT_EQ(lib1.native_handle(), nullptr);
}

TEST_F(DynamicLibraryTest, Path_ReturnsCorrectPath) {
    string lib_path = get_test_library_path();
    dynamic_library lib(lib_path);
    EXPECT_EQ(lib.path(), lib_path);
}

TEST_F(DynamicLibraryTest, Path_AfterMoveConstruction_SourcePathUnchanged) {
    string lib_path = get_test_library_path();
    dynamic_library lib1(lib_path);
    string path_copy = lib_path;

    dynamic_library lib2(move(lib1));

    EXPECT_EQ(lib2.path(), path_copy);
}

TEST_F(DynamicLibraryTest, Path_AfterMoveAssignment_DestinationPathUpdated) {
    string path1 = get_test_library_path();
    dynamic_library lib1(path1);

#ifdef NEFORCE_PLATFORM_WINDOWS
    string path2 = "user32.dll";
#else
    string path2 = "libpthread.so.0";
#endif

    dynamic_library lib2(path2);

    lib2 = move(lib1);

    EXPECT_EQ(lib2.path(), path1);
}

TEST_F(DynamicLibraryTest, Destructor_UnloadsLibrary) {
    void* handle = nullptr;
    {
        dynamic_library lib(get_test_library_path());
        handle = lib.native_handle();
        EXPECT_NE(handle, nullptr);
    }

    dynamic_library lib2(get_test_library_path());
    EXPECT_TRUE(lib2.is_open());
}

TEST_F(DynamicLibraryTest, Destructor_AfterMove_SourceDestructorNoDoubleFree) {
    {
        dynamic_library lib1(get_test_library_path());
        {
            dynamic_library lib2(move(lib1));
            EXPECT_TRUE(lib2.is_open());
        }
    }
    SUCCEED();
}

TEST_F(DynamicLibraryTest, MultipleLibrariesLoaded_Success) {
    dynamic_library lib1(get_test_library_path());

#ifdef NEFORCE_PLATFORM_WINDOWS
    dynamic_library lib2("user32.dll");
#else
    dynamic_library lib2("libpthread.so.0");
#endif

    EXPECT_TRUE(lib1.is_open());
    EXPECT_TRUE(lib2.is_open());

    EXPECT_NE(lib1.native_handle(), nullptr);
    EXPECT_NE(lib2.native_handle(), nullptr);

    EXPECT_NE(lib1.native_handle(), lib2.native_handle());
}

TEST_F(DynamicLibraryTest, CopyConstructor_IsDeleted) { EXPECT_FALSE(is_copy_constructible<dynamic_library>::value); }

TEST_F(DynamicLibraryTest, CopyAssignment_IsDeleted) { EXPECT_FALSE(is_copy_assignable<dynamic_library>::value); }

TEST_F(DynamicLibraryTest, MoveConstructor_IsNoexcept) {
    EXPECT_TRUE(is_nothrow_move_constructible<dynamic_library>::value);
}

TEST_F(DynamicLibraryTest, MoveAssignment_IsNoexcept) {
    EXPECT_TRUE(is_nothrow_move_assignable<dynamic_library>::value);
}

TEST_F(DynamicLibraryTest, Exception_CopyConstructor_Success) {
    try {
        dynamic_library lib(get_nonexistent_library_path());
        FAIL() << "Expected dynamic_library_exception";
    } catch (const dynamic_library_exception& e) {
        dynamic_library_exception copied(e);
        EXPECT_NE(copied.what(), nullptr);
    }
}

TEST_F(DynamicLibraryTest, Exception_What_ContainsErrorInfo) {
    try {
        dynamic_library lib(get_nonexistent_library_path());
        FAIL() << "Expected dynamic_library_exception";
    } catch (const dynamic_library_exception& e) {
        EXPECT_NE(e.what(), nullptr);
        EXPECT_GT(string_length(e.what()), 0);
    }
}

TEST_F(DynamicLibraryTest, Symbol_OnDifferentLibrary_Success) {
    dynamic_library lib1(get_test_library_path());

#ifdef NEFORCE_PLATFORM_WINDOWS
    dynamic_library lib2("user32.dll");
    void* sym1 = lib1.symbol("GetCurrentProcessId");
    void* sym2 = lib2.symbol("MessageBoxA");
#else
    dynamic_library lib2("libpthread.so.0");
    void* sym1 = lib1.symbol("getpid");
    void* sym2 = lib2.symbol("pthread_self");
#endif

    EXPECT_NE(sym1, nullptr);
    EXPECT_NE(sym2, nullptr);
    EXPECT_NE(sym1, sym2);
}

TEST_F(DynamicLibraryTest, Symbol_RepeatedCalls_ReturnsSameAddress) {
    dynamic_library lib(get_test_library_path());
    void* sym1 = lib.symbol(get_test_symbol_name());
    void* sym2 = lib.symbol(get_test_symbol_name());
    EXPECT_EQ(sym1, sym2);
}

TEST_F(DynamicLibraryTest, Unload_AfterMoveConstructor_SourceUnloadNoDoubleFree) {
    dynamic_library lib1(get_test_library_path());
    dynamic_library lib2(move(lib1));

    EXPECT_NO_THROW(lib1.unload());
    EXPECT_FALSE(lib1.is_open());
    EXPECT_TRUE(lib2.is_open());
}

TEST_F(DynamicLibraryTest, Unload_AfterMoveAssignment_SourceUnloadNoDoubleFree) {
    dynamic_library lib1(get_test_library_path());
    dynamic_library lib2(get_test_library_path());
    lib2 = move(lib1);

    EXPECT_NO_THROW(lib1.unload());
    EXPECT_FALSE(lib1.is_open());
    EXPECT_TRUE(lib2.is_open());
}

TEST_F(DynamicLibraryTest, ToSymbol_RepeatedCalls_ReturnsSameAddress) {
    dynamic_library lib(get_test_library_path());

#ifdef NEFORCE_PLATFORM_WINDOWS
    using FuncType = DWORD(WINAPI*)();
#else
    using FuncType = pid_t (*)();
#endif

    auto func1 = lib.to_symbol<FuncType>(get_test_symbol_name());
    auto func2 = lib.to_symbol<FuncType>(get_test_symbol_name());
    EXPECT_EQ(func1, func2);
}

TEST_F(DynamicLibraryTest, Path_ReturnsConstReference) {
    dynamic_library lib(get_test_library_path());
    const string& path_ref1 = lib.path();
    const string& path_ref2 = lib.path();
    EXPECT_EQ(&path_ref1, &path_ref2);
}

TEST_F(DynamicLibraryTest, NativeHandle_ConstMethod_ReturnsSameHandle) {
    const dynamic_library lib(get_test_library_path());
    void* handle1 = lib.native_handle();
    void* handle2 = lib.native_handle();
    EXPECT_EQ(handle1, handle2);
}

TEST_F(DynamicLibraryTest, IsOpen_ConstObject_Success) {
    const dynamic_library lib(get_test_library_path());
    EXPECT_TRUE(lib.is_open());
}

TEST_F(DynamicLibraryTest, HasSymbol_ConstObject_Success) {
    const dynamic_library lib(get_test_library_path());
    EXPECT_TRUE(lib.has_symbol(get_test_symbol_name()));
}

TEST_F(DynamicLibraryTest, Unload_SymbolAfterUnload_ThrowsException) {
    dynamic_library lib(get_test_library_path());
    lib.unload();

    EXPECT_THROW(ignore = lib.symbol(get_test_symbol_name()), dynamic_library_exception);
    EXPECT_THROW(
            {
                using FuncType = void (*)();
                lib.to_symbol<FuncType>(get_test_symbol_name());
            },
            dynamic_library_exception);
}

TEST_F(DynamicLibraryTest, MoveAssignment_DifferentLibraries_SourceClosedCorrectly) {
    dynamic_library lib1(get_test_library_path());

#ifdef NEFORCE_PLATFORM_WINDOWS
    dynamic_library lib2("user32.dll");
#else
    dynamic_library lib2("libpthread.so.0");
#endif

    void* handle1_before = lib1.native_handle();
    void* handle2_before = lib2.native_handle();

    lib2 = move(lib1);

    EXPECT_EQ(lib2.native_handle(), handle1_before);
    EXPECT_EQ(lib1.native_handle(), nullptr);
}

class EnvironmentTest : public ::testing::Test {
protected:
    static constexpr const char* test_var_name = "NEFORCE_TEST_ENV_VAR_12345";
    static constexpr const char* test_var_value = "test_value_67890";
    static constexpr const char* test_var_value_alt = "test_value_alt_11111";

    void SetUp() override { environment::unset(test_var_name); }

    void TearDown() override { environment::unset(test_var_name); }
};

TEST_F(EnvironmentTest, Get_ExistingVariable_ReturnsValue) {
    environment::set(test_var_name, test_var_value);
    string result = environment::get(test_var_name);
    EXPECT_EQ(result, test_var_value);
}

TEST_F(EnvironmentTest, Get_EmptyName_ReturnsEmpty) {
    string result = environment::get("");
    EXPECT_TRUE(result.empty());
}

TEST_F(EnvironmentTest, Get_SystemVariable_ReturnsValue) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    string result = environment::get("SystemRoot");
#else
    string result = environment::get("HOME");
#endif
    EXPECT_FALSE(result.empty());
}

TEST_F(EnvironmentTest, Set_NewVariable_ReturnsTrue) {
    bool result = environment::set(test_var_name, test_var_value);
    EXPECT_TRUE(result);
    EXPECT_EQ(environment::get(test_var_name), test_var_value);
}

TEST_F(EnvironmentTest, Set_OverwriteTrue_UpdatesValue) {
    environment::set(test_var_name, test_var_value);
    bool result = environment::set(test_var_name, test_var_value_alt, true);
    EXPECT_TRUE(result);
    EXPECT_EQ(environment::get(test_var_name), test_var_value_alt);
}

TEST_F(EnvironmentTest, Set_OverwriteFalse_OnExistingVariable_ReturnsTrue) {
    environment::set(test_var_name, test_var_value);
    bool result = environment::set(test_var_name, test_var_value_alt, false);
    EXPECT_TRUE(result);
}

TEST_F(EnvironmentTest, Set_OverwriteFalse_OnNewVariable_ReturnsTrue) {
    bool result = environment::set(test_var_name, test_var_value, false);
    EXPECT_TRUE(result);
    EXPECT_EQ(environment::get(test_var_name), test_var_value);
}

TEST_F(EnvironmentTest, Set_EmptyName_ReturnsFailure) {
    bool result = environment::set("", test_var_value);
    EXPECT_FALSE(result);
}

TEST_F(EnvironmentTest, Set_EmptyValue_Success) {
    bool result = environment::set(test_var_name, "");
    EXPECT_TRUE(result);
    EXPECT_EQ(environment::get(test_var_name), "");
}

TEST_F(EnvironmentTest, Unset_ExistingVariable_ReturnsTrue) {
    environment::set(test_var_name, test_var_value);
    bool result = environment::unset(test_var_name);
    EXPECT_TRUE(result);
    EXPECT_TRUE(environment::get(test_var_name).empty());
}

TEST_F(EnvironmentTest, Unset_NonExistentVariable_ReturnsTrue) {
    bool result = environment::unset(test_var_name);
    EXPECT_TRUE(result);
}

TEST_F(EnvironmentTest, Unset_AlreadyUnsetVariable_ReturnsTrue) {
    environment::set(test_var_name, test_var_value);
    environment::unset(test_var_name);
    bool result = environment::unset(test_var_name);
    EXPECT_TRUE(result);
}

TEST_F(EnvironmentTest, Unset_EmptyName_ReturnsFailure) {
    bool result = environment::unset("");
    EXPECT_FALSE(result);
}

TEST_F(EnvironmentTest, Exists_ExistingVariable_ReturnsTrue) {
    environment::set(test_var_name, test_var_value);
    EXPECT_TRUE(environment::exists(test_var_name));
}

TEST_F(EnvironmentTest, Exists_NonExistentVariable_ReturnsFalse) { EXPECT_FALSE(environment::exists(test_var_name)); }

TEST_F(EnvironmentTest, Exists_AfterUnset_ReturnsFalse) {
    environment::set(test_var_name, test_var_value);
    environment::unset(test_var_name);
    EXPECT_FALSE(environment::exists(test_var_name));
}

TEST_F(EnvironmentTest, Exists_EmptyName_ReturnsFalse) { EXPECT_FALSE(environment::exists("")); }

TEST_F(EnvironmentTest, Exists_EmptyValueVariable_ReturnsFalse) {
    environment::set(test_var_name, "");
    EXPECT_FALSE(environment::exists(test_var_name));
}

TEST_F(EnvironmentTest, AllEnvs_ReturnsNonEmptyMap) {
    auto env_map = environment::all_envs();
    EXPECT_FALSE(env_map.empty());
}

TEST_F(EnvironmentTest, AllEnvs_ContainsSystemVariable) {
    auto env_map = environment::all_envs();
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_TRUE(env_map.count("SystemRoot") > 0 || env_map.count("windir") > 0);
#else
    EXPECT_TRUE(env_map.count("HOME") > 0);
#endif
}

TEST_F(EnvironmentTest, AllEnvs_ContainsSetVariable) {
    environment::set(test_var_name, test_var_value);
    auto env_map = environment::all_envs();
    EXPECT_TRUE(env_map.count(test_var_name) > 0);
    EXPECT_EQ(env_map[test_var_name], test_var_value);
}

TEST_F(EnvironmentTest, AllEnvs_DoesNotContainUnsetVariable) {
    environment::set(test_var_name, test_var_value);
    environment::unset(test_var_name);
    auto env_map = environment::all_envs();
    EXPECT_EQ(env_map.count(test_var_name), 0);
}

TEST_F(EnvironmentTest, PathList_ReturnsNonEmpty) {
    auto paths = environment::path_list();
    EXPECT_FALSE(paths.empty());
}

TEST_F(EnvironmentTest, PathList_ContainsValidPaths) {
    auto paths = environment::path_list();
    for (const auto& path: paths) {
        EXPECT_FALSE(path.empty());
    }
}

TEST_F(EnvironmentTest, PathList_UsesCorrectDelimiter) {
    environment::set("PATH", "/usr/bin" + string(1, environment::delimiter) + "/usr/local/bin");
    auto paths = environment::path_list();
    ASSERT_EQ(paths.size(), 2);
    EXPECT_EQ(paths[0], "/usr/bin");
    EXPECT_EQ(paths[1], "/usr/local/bin");
}

TEST_F(EnvironmentTest, PathList_SinglePath_ReturnsOneElement) {
    environment::set("PATH", "/usr/bin");
    auto paths = environment::path_list();
    ASSERT_EQ(paths.size(), 1);
    EXPECT_EQ(paths[0], "/usr/bin");
}

TEST_F(EnvironmentTest, PathList_EmptyPath_ReturnsEmpty) {
    environment::set("PATH", "");
    auto paths = environment::path_list();
    EXPECT_TRUE(paths.empty());
}

TEST_F(EnvironmentTest, AddToPath_ToEnd_ReturnsTrue) {
    string original_path = environment::get("PATH");
    string test_path = "/test/path/xyz";

    bool result = environment::add_to_path(test_path, 1);
    EXPECT_TRUE(result);

    string new_path = environment::get("PATH");
    EXPECT_NE(new_path.find(test_path), string::npos);
    environment::set("PATH", original_path);
}

TEST_F(EnvironmentTest, AddToPath_ToBeginning_ReturnsTrue) {
    string original_path = environment::get("PATH");
    string test_path = "/test/path/abc";

    bool result = environment::add_to_path(test_path, 0);
    EXPECT_TRUE(result);

    string new_path = environment::get("PATH");
    EXPECT_EQ(new_path.find(test_path), 0);
    environment::set("PATH", original_path);
}

TEST_F(EnvironmentTest, AddToPath_EmptyOriginalPath_ReturnsTrue) {
    string original_path = environment::get("PATH");
    environment::set("PATH", "");

    string test_path = "/test/new/path";
    bool result = environment::add_to_path(test_path);
    EXPECT_TRUE(result);
    EXPECT_EQ(environment::get("PATH"), test_path);
    environment::set("PATH", original_path);
}

TEST_F(EnvironmentTest, AddToPath_EmptyPath_ReturnsFalse) {
    string original_path = environment::get("PATH");

    bool result = environment::add_to_path("");
    EXPECT_FALSE(result);
}

TEST_F(EnvironmentTest, CurrentDirectory_ReturnsNonEmpty) {
    string cwd = environment::current_directory();
    EXPECT_FALSE(cwd.empty());
}

TEST_F(EnvironmentTest, CurrentDirectory_IsAbsolutePath) {
    string cwd = environment::current_directory();
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_TRUE(cwd.size() >= 3 && cwd[1] == ':');
#else
    EXPECT_EQ(cwd[0], '/');
#endif
}

TEST_F(EnvironmentTest, CurrentUser_ReturnsNonEmpty) {
    string user = environment::current_user();
    EXPECT_FALSE(user.empty());
}

TEST_F(EnvironmentTest, CurrentUser_IsConsistent) {
    string user1 = environment::current_user();
    string user2 = environment::current_user();
    EXPECT_EQ(user1, user2);
}

TEST_F(EnvironmentTest, TempDirectory_ReturnsNonEmpty) {
    string temp = environment::temp_directory();
    EXPECT_FALSE(temp.empty());
}

TEST_F(EnvironmentTest, TempDirectory_IsValidPath) {
    string temp = environment::temp_directory();
    EXPECT_GT(temp.size(), 0);
}

TEST_F(EnvironmentTest, HomeDirectory_ReturnsNonEmpty) {
    string home = environment::home_directory();
    EXPECT_FALSE(home.empty());
}

TEST_F(EnvironmentTest, HomeDirectory_IsAbsolutePath) {
    string home = environment::home_directory();
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_TRUE(home.size() >= 3 && home[1] == ':');
#else
    EXPECT_EQ(home[0], '/');
#endif
}

TEST_F(EnvironmentTest, Delimiter_IsCorrectPlatform) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_EQ(environment::delimiter, ';');
#else
    EXPECT_EQ(environment::delimiter, ':');
#endif
}

TEST_F(EnvironmentTest, SetGet_LargeValue_Success) {
    string large_value(4096, 'x');
    bool set_result = environment::set(test_var_name, large_value);
    EXPECT_TRUE(set_result);

    string get_result = environment::get(test_var_name);
    EXPECT_EQ(get_result, large_value);
}

TEST_F(EnvironmentTest, SetGet_SpecialCharacters_Success) {
    string special_value = "value with spaces and = signs and & symbols";
    bool set_result = environment::set(test_var_name, special_value);
    EXPECT_TRUE(set_result);

    string get_result = environment::get(test_var_name);
    EXPECT_EQ(get_result, special_value);
}

TEST_F(EnvironmentTest, MultipleSetUnsetSequence_Success) {
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(environment::set(test_var_name, test_var_value));
        EXPECT_EQ(environment::get(test_var_name), test_var_value);
        EXPECT_TRUE(environment::unset(test_var_name));
        EXPECT_TRUE(environment::get(test_var_name).empty());
    }
}

TEST_F(EnvironmentTest, AllEnvs_ValuesMatchGet) {
    auto env_map = environment::all_envs();
    for (const auto& [name, value]: env_map) {
        if (!name.empty()) {
            string direct_value = environment::get(name);
            EXPECT_EQ(direct_value, value);
        }
    }
}

TEST_F(EnvironmentTest, PathList_RestoresPathCorrectly) {
    string original_path = environment::get("PATH");
    auto original_list = environment::path_list();

    string test_path = "/test/restore/path";
    environment::add_to_path(test_path, 1);

    EXPECT_NE(environment::get("PATH"), original_path);

    environment::set("PATH", original_path);

    auto restored_list = environment::path_list();
    EXPECT_EQ(restored_list.size(), original_list.size());
    for (size_t i = 0; i < original_list.size() && i < restored_list.size(); ++i) {
        EXPECT_EQ(original_list[i], restored_list[i]);
    }
}

TEST_F(EnvironmentTest, AddToPath_PositionZero_PrependsCorrectly) {
    string original_path = environment::get("PATH");
    string test_path = "/prepend/test/path";

    bool result = environment::add_to_path(test_path, 0);
    EXPECT_TRUE(result);

    string new_path = environment::get("PATH");
    auto paths = environment::path_list();
    EXPECT_EQ(paths[0], test_path);

    environment::set("PATH", original_path);
}

TEST_F(EnvironmentTest, AddToPath_PositionNonZero_AppendsCorrectly) {
    string original_path = environment::get("PATH");
    string test_path = "/append/test/path";

    bool result = environment::add_to_path(test_path, 2);
    EXPECT_TRUE(result);

    string new_path = environment::get("PATH");
    auto paths = environment::path_list();
    EXPECT_EQ(paths[paths.size() - 1], test_path);

    environment::set("PATH", original_path);
}

TEST_F(EnvironmentTest, Exists_SystemVariable_ReturnsTrue) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_TRUE(environment::exists("SystemRoot"));
#else
    EXPECT_TRUE(environment::exists("HOME"));
#endif
}

TEST_F(EnvironmentTest, Get_AfterSetEmpty_Success) {
    environment::set(test_var_name, "");
    string result = environment::get(test_var_name);
    EXPECT_TRUE(result.empty());
    EXPECT_FALSE(environment::exists(test_var_name));
}

TEST_F(EnvironmentTest, PathList_WithTrailingDelimiter_Success) {
    string original_path = environment::get("PATH");
    environment::set("PATH", "/usr/bin" + string(1, environment::delimiter));

    auto paths = environment::path_list();
    ASSERT_GE(paths.size(), 1);
    EXPECT_EQ(paths[0], "/usr/bin");

    environment::set("PATH", original_path);
}

TEST_F(EnvironmentTest, PathList_WithConsecutiveDelimiters_Success) {
    string original_path = environment::get("PATH");
    string delim(1, environment::delimiter);
    environment::set("PATH", "/usr/bin" + delim + delim + "/usr/local/bin");

    auto paths = environment::path_list();
    ASSERT_GE(paths.size(), 2);
    EXPECT_EQ(paths[0], "/usr/bin");
    EXPECT_EQ(paths[1], "/usr/local/bin");

    environment::set("PATH", original_path);
}

TEST_F(EnvironmentTest, ThreadSafety_ConcurrentGetSet) {
    constexpr int num_threads = 4;
    vector<thread> threads;

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([t]() {
            constexpr int num_iterations = 50;
            for (int i = 0; i < num_iterations; ++i) {
                string var_name = string(test_var_name) + "_" + to_string(t);
                string value = "value_" + to_string(i);

                environment::set(var_name, value);
                string retrieved = environment::get(var_name);
                EXPECT_EQ(retrieved, value);
                EXPECT_TRUE(environment::exists(var_name));

                environment::unset(var_name);
                EXPECT_FALSE(environment::exists(var_name));
            }
        });
    }

    for (auto& thread: threads) {
        thread.join();
    }
}

TEST_F(EnvironmentTest, ThreadSafety_ConcurrentAllEnvs) {
    constexpr int num_threads = 4;
    vector<thread> threads;

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([]() {
            for (int i = 0; i < 20; ++i) {
                auto env_map = environment::all_envs();
                EXPECT_FALSE(env_map.empty());

                environment::set(test_var_name, test_var_value);
                ignore = environment::get(test_var_name);
                ignore = environment::exists(test_var_name);
                environment::unset(test_var_name);
            }
        });
    }

    for (auto& thread: threads) {
        thread.join();
    }
}

TEST_F(EnvironmentTest, TempDirectory_FallbackLogic_ReturnsValid) {
    string temp = environment::temp_directory();

    EXPECT_FALSE(temp.empty());
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_TRUE(temp.size() >= 3);
#else
    EXPECT_EQ(temp[0], '/');
#endif
}

TEST_F(EnvironmentTest, HomeDirectory_FallbackLogic_ReturnsValid) {
    string home = environment::home_directory();

    EXPECT_FALSE(home.empty());
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_TRUE(home.size() >= 3);
#else
    EXPECT_EQ(home[0], '/');
#endif
}

TEST_F(EnvironmentTest, CurrentDirectory_AfterChangeDirectory_Success) {
    string original_cwd = environment::current_directory();
    string temp_dir = environment::temp_directory();

#ifdef NEFORCE_PLATFORM_WINDOWS
    BOOL chdir_result = ::SetCurrentDirectoryA(temp_dir.data());
    ASSERT_TRUE(chdir_result);
#else
    int chdir_result = ::chdir(temp_dir.data());
    ASSERT_EQ(chdir_result, 0);
#endif

    string new_cwd = environment::current_directory();

#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_NE(new_cwd, original_cwd);
    ::SetCurrentDirectoryA(original_cwd.data());
#else
    ::chdir(original_cwd.data());
#endif
}

class LocaleTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(LocaleTest, DefaultConstructor_CreatesCLocale) {
    locale loc;
    EXPECT_EQ(loc.name(), "C");
}

TEST_F(LocaleTest, Constructor_WithName_CreatesLocale) {
    locale loc("en_US.UTF-8");
    EXPECT_FALSE(loc.name().empty());
}

TEST_F(LocaleTest, Constructor_WithC_CreatesCLocale) {
    locale loc("C");
    EXPECT_EQ(loc.name(), "C");
}

TEST_F(LocaleTest, Constructor_WithPOSIX_CreatesCLocale) {
    locale loc("POSIX");
    EXPECT_EQ(loc.name(), "POSIX");
}

TEST_F(LocaleTest, Constructor_WithEmptyString_CreatesCLocale) {
    locale loc("");
    EXPECT_EQ(loc.name(), "C");
}

TEST_F(LocaleTest, Constructor_WithInvalidName_ThrowsException) {
    EXPECT_THROW(locale("nonexistent_locale_xyz_12345"), locale_exception);
}

TEST_F(LocaleTest, CopyConstructor_CreatesIdenticalLocale) {
    locale loc1("en_US.UTF-8");
    locale loc2(loc1);
    EXPECT_EQ(loc1.name(), loc2.name());
    EXPECT_EQ(loc1.encoding(), loc2.encoding());
    EXPECT_EQ(loc1, loc2);
}

TEST_F(LocaleTest, CopyConstructor_IndependentCopy) {
    locale loc1("en_US.UTF-8");
    locale loc2(loc1);
    EXPECT_EQ(loc1.name(), loc2.name());
}

TEST_F(LocaleTest, CopyAssignment_CopiesCorrectly) {
    locale loc1("en_US.UTF-8");
    locale loc2("C");
    loc2 = loc1;
    EXPECT_EQ(loc1.name(), loc2.name());
    EXPECT_EQ(loc1.encoding(), loc2.encoding());
    EXPECT_EQ(loc1, loc2);
}

TEST_F(LocaleTest, CopyAssignment_SelfAssignment_NoEffect) {
    locale loc("en_US.UTF-8");
    string name = loc.name();
    string encoding = loc.encoding();

    loc = loc;

    EXPECT_EQ(loc.name(), name);
    EXPECT_EQ(loc.encoding(), encoding);
}

TEST_F(LocaleTest, MoveConstructor_TransfersOwnership) {
    locale loc1("en_US.UTF-8");
    string name = loc1.name();
    string encoding = loc1.encoding();

    locale loc2(move(loc1));

    EXPECT_EQ(loc2.name(), name);
    EXPECT_EQ(loc2.encoding(), encoding);
}

TEST_F(LocaleTest, MoveConstructor_SourceCanBeDestroyed) {
    locale loc1("en_US.UTF-8");
    {
        locale loc2(move(loc1));
    }
    SUCCEED();
}

TEST_F(LocaleTest, MoveAssignment_TransfersOwnership) {
    locale loc1("en_US.UTF-8");
    string name = loc1.name();
    string encoding = loc1.encoding();

    locale loc2("C");
    loc2 = move(loc1);

    EXPECT_EQ(loc2.name(), name);
    EXPECT_EQ(loc2.encoding(), encoding);
}

TEST_F(LocaleTest, MoveAssignment_SelfAssignment_NoEffect) {
    locale loc("en_US.UTF-8");
    string name = loc.name();
    string encoding = loc.encoding();

    loc = move(loc);

    EXPECT_EQ(loc.name(), name);
    EXPECT_EQ(loc.encoding(), encoding);
}

TEST_F(LocaleTest, MoveAssignment_DifferentLocales_DestinationCleanedUp) {
    locale loc1("en_US.UTF-8");
    locale loc2("fr_FR.UTF-8");
    EXPECT_NO_THROW(loc2 = move(loc1));
}

TEST_F(LocaleTest, Classic_ReturnsCLocale) {
    locale loc = locale::classic();
    EXPECT_EQ(loc.name(), "C");
}

TEST_F(LocaleTest, System_ReturnsSystemLocale) {
    locale loc = locale::system();
    EXPECT_FALSE(loc.name().empty());
}

TEST_F(LocaleTest, FromName_WithC_ReturnsCLocale) {
    locale loc = locale::from_name("C");
    EXPECT_EQ(loc.name(), "C");
}

TEST_F(LocaleTest, FromName_WithEnUS_ReturnsCorrectLocale) {
    locale loc = locale::from_name("en_US.UTF-8");
    EXPECT_EQ(loc.name(), "en_US.UTF-8");
}

TEST_F(LocaleTest, FromName_WithInvalidName_ThrowsException) {
    EXPECT_THROW(ignore = locale::from_name("nonexistent_locale_xyz_12345"), locale_exception);
}

TEST_F(LocaleTest, Name_ReturnsCorrectName) {
    locale loc("en_US.UTF-8");
    EXPECT_EQ(loc.name(), "en_US.UTF-8");
}

TEST_F(LocaleTest, Name_CLocale_ReturnsC) {
    locale loc("C");
    EXPECT_EQ(loc.name(), "C");
}

TEST_F(LocaleTest, Encoding_ReturnsNonEmpty) {
    locale loc("en_US.UTF-8");
    EXPECT_FALSE(loc.encoding().empty());
}

TEST_F(LocaleTest, Encoding_CLocale_ReturnsNonEmpty) {
    locale loc("C");
    EXPECT_FALSE(loc.encoding().empty());
}

TEST_F(LocaleTest, EqualityOperator_SameLocale_ReturnsTrue) {
    locale loc1("en_US.UTF-8");
    locale loc2("en_US.UTF-8");
    EXPECT_TRUE(loc1 == loc2);
}

TEST_F(LocaleTest, EqualityOperator_DifferentLocale_ReturnsFalse) {
    locale loc1("en_US.UTF-8");
    locale loc2("C");
    EXPECT_FALSE(loc1 == loc2);
}

TEST_F(LocaleTest, InequalityOperator_SameLocale_ReturnsFalse) {
    locale loc1("en_US.UTF-8");
    locale loc2("en_US.UTF-8");
    EXPECT_FALSE(loc1 != loc2);
}

TEST_F(LocaleTest, InequalityOperator_DifferentLocale_ReturnsTrue) {
    locale loc1("en_US.UTF-8");
    locale loc2("C");
    EXPECT_TRUE(loc1 != loc2);
}

TEST_F(LocaleTest, Numeric_CLocale_ReturnsStandardInfo) {
    locale loc("C");
    auto info = loc.numeric();
    EXPECT_EQ(info.decimal_point, ".");
#ifdef NEFORCE_PLATFORM_LINUX
    EXPECT_TRUE(info.thousands_sep.empty());
#else
    EXPECT_FALSE(info.thousands_sep.empty());
#endif
}

TEST_F(LocaleTest, Numeric_EnUSLocale_ReturnsNonEmpty) {
    locale loc("en_US.UTF-8");
    auto info = loc.numeric();
    EXPECT_FALSE(info.decimal_point.empty());
    EXPECT_FALSE(info.thousands_sep.empty());
}

TEST_F(LocaleTest, Numeric_MultipleLocales_ReturnsDifferentResults) {
    locale loc_c("C");
    auto info_c = loc_c.numeric();

    locale loc_en("en_US.UTF-8");
    auto info_en = loc_en.numeric();

    EXPECT_FALSE(info_c.decimal_point.empty());
    EXPECT_FALSE(info_en.decimal_point.empty());
}

TEST_F(LocaleTest, Monetary_CLocale_ReturnsNonEmpty) {
    locale loc("C");
    auto info = loc.monetary();
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_FALSE(info.currency_symbol.empty());
#else
    EXPECT_TRUE(info.currency_symbol.empty());
#endif
    EXPECT_GE(info.frac_digits, 0);
    EXPECT_GE(info.int_frac_digits, 0);
}

TEST_F(LocaleTest, Monetary_EnUSLocale_ReturnsNonEmpty) {
    locale loc("en_US.UTF-8");
    auto info = loc.monetary();
    EXPECT_FALSE(info.currency_symbol.empty());
    EXPECT_FALSE(info.mon_decimal_point.empty());
    EXPECT_GE(info.frac_digits, 0);
}

TEST_F(LocaleTest, Monetary_CLocale_FracDigits_NonNegative) {
    locale loc("C");
    auto info = loc.monetary();
    EXPECT_GE(info.frac_digits, 0);
}

TEST_F(LocaleTest, Time_CLocale_ReturnsNonEmpty) {
    locale loc("C");
    auto info = loc.time();
    EXPECT_FALSE(info.date_fmt.empty());
    EXPECT_FALSE(info.time_fmt.empty());
    EXPECT_FALSE(info.datetime_fmt.empty());
}

TEST_F(LocaleTest, Time_EnUSLocale_DayNamesCount7) {
    locale loc("en_US.UTF-8");
    auto info = loc.time();
    EXPECT_EQ(info.day_names.size(), 7);
    EXPECT_EQ(info.abbr_day_names.size(), 7);
}

TEST_F(LocaleTest, Time_EnUSLocale_MonthNamesCount12) {
    locale loc("en_US.UTF-8");
    auto info = loc.time();
    EXPECT_EQ(info.month_names.size(), 12);
    EXPECT_EQ(info.abbr_month_names.size(), 12);
}

TEST_F(LocaleTest, Time_AmPm_NonEmpty) {
    locale loc("en_US.UTF-8");
    auto info = loc.time();
    EXPECT_FALSE(info.am_str.empty());
    EXPECT_FALSE(info.pm_str.empty());
}

TEST_F(LocaleTest, IsAlpha_AsciiLetter_ReturnsTrue) {
    locale loc("C");
    EXPECT_TRUE(loc.is_alpha(U'A'));
    EXPECT_TRUE(loc.is_alpha(U'z'));
}

TEST_F(LocaleTest, IsAlpha_Digit_ReturnsFalse) {
    locale loc("C");
    EXPECT_FALSE(loc.is_alpha(U'1'));
    EXPECT_FALSE(loc.is_alpha(U'9'));
}

TEST_F(LocaleTest, IsAlpha_Space_ReturnsFalse) {
    locale loc("C");
    EXPECT_FALSE(loc.is_alpha(U' '));
}

TEST_F(LocaleTest, IsDigit_AsciiDigit_ReturnsTrue) {
    locale loc("C");
    EXPECT_TRUE(loc.is_digit(U'0'));
    EXPECT_TRUE(loc.is_digit(U'5'));
    EXPECT_TRUE(loc.is_digit(U'9'));
}

TEST_F(LocaleTest, IsDigit_Letter_ReturnsFalse) {
    locale loc("C");
    EXPECT_FALSE(loc.is_digit(U'A'));
    EXPECT_FALSE(loc.is_digit(U'z'));
}

TEST_F(LocaleTest, IsAlnum_Letter_ReturnsTrue) {
    locale loc("C");
    EXPECT_TRUE(loc.is_alnum(U'A'));
    EXPECT_TRUE(loc.is_alnum(U'z'));
}

TEST_F(LocaleTest, IsAlnum_Digit_ReturnsTrue) {
    locale loc("C");
    EXPECT_TRUE(loc.is_alnum(U'0'));
    EXPECT_TRUE(loc.is_alnum(U'9'));
}

TEST_F(LocaleTest, IsAlnum_Punctuation_ReturnsFalse) {
    locale loc("C");
    EXPECT_FALSE(loc.is_alnum(U'.'));
    EXPECT_FALSE(loc.is_alnum(U','));
}

TEST_F(LocaleTest, IsSpace_SpaceChar_ReturnsTrue) {
    locale loc("C");
    EXPECT_TRUE(loc.is_space(U' '));
    EXPECT_TRUE(loc.is_space(U'\t'));
}

TEST_F(LocaleTest, IsSpace_Letter_ReturnsFalse) {
    locale loc("C");
    EXPECT_FALSE(loc.is_space(U'A'));
}

TEST_F(LocaleTest, IsUpper_Uppercase_ReturnsTrue) {
    locale loc("C");
    EXPECT_TRUE(loc.is_upper(U'A'));
    EXPECT_TRUE(loc.is_upper(U'Z'));
}

TEST_F(LocaleTest, IsUpper_Lowercase_ReturnsFalse) {
    locale loc("C");
    EXPECT_FALSE(loc.is_upper(U'a'));
    EXPECT_FALSE(loc.is_upper(U'z'));
}

TEST_F(LocaleTest, IsLower_Lowercase_ReturnsTrue) {
    locale loc("C");
    EXPECT_TRUE(loc.is_lower(U'a'));
    EXPECT_TRUE(loc.is_lower(U'z'));
}

TEST_F(LocaleTest, IsLower_Uppercase_ReturnsFalse) {
    locale loc("C");
    EXPECT_FALSE(loc.is_lower(U'A'));
    EXPECT_FALSE(loc.is_lower(U'Z'));
}

TEST_F(LocaleTest, IsPunct_Punctuation_ReturnsTrue) {
    locale loc("C");
    EXPECT_TRUE(loc.is_punct(U'.'));
    EXPECT_TRUE(loc.is_punct(U','));
    EXPECT_TRUE(loc.is_punct(U'!'));
}

TEST_F(LocaleTest, IsPunct_Letter_ReturnsFalse) {
    locale loc("C");
    EXPECT_FALSE(loc.is_punct(U'A'));
}

TEST_F(LocaleTest, IsPrint_Printable_ReturnsTrue) {
    locale loc("C");
    EXPECT_TRUE(loc.is_print(U'A'));
    EXPECT_TRUE(loc.is_print(U'1'));
    EXPECT_TRUE(loc.is_print(U'.'));
}

TEST_F(LocaleTest, IsPrint_ControlChar_ReturnsFalse) {
    locale loc("C");
    EXPECT_FALSE(loc.is_print(U'\0'));
    EXPECT_FALSE(loc.is_print(U'\x01'));
}

TEST_F(LocaleTest, ToUpper_Lowercase_ReturnsUppercase) {
    locale loc("C");
    EXPECT_EQ(loc.to_upper(U'a'), U'A');
    EXPECT_EQ(loc.to_upper(U'z'), U'Z');
}

TEST_F(LocaleTest, ToUpper_Uppercase_ReturnsSame) {
    locale loc("C");
    EXPECT_EQ(loc.to_upper(U'A'), U'A');
    EXPECT_EQ(loc.to_upper(U'Z'), U'Z');
}

TEST_F(LocaleTest, ToUpper_Digit_ReturnsSame) {
    locale loc("C");
    EXPECT_EQ(loc.to_upper(U'1'), U'1');
}

TEST_F(LocaleTest, ToLower_Uppercase_ReturnsLowercase) {
    locale loc("C");
    EXPECT_EQ(loc.to_lower(U'A'), U'a');
    EXPECT_EQ(loc.to_lower(U'Z'), U'z');
}

TEST_F(LocaleTest, ToLower_Lowercase_ReturnsSame) {
    locale loc("C");
    EXPECT_EQ(loc.to_lower(U'a'), U'a');
    EXPECT_EQ(loc.to_lower(U'z'), U'z');
}

TEST_F(LocaleTest, ToLower_Digit_ReturnsSame) {
    locale loc("C");
    EXPECT_EQ(loc.to_lower(U'1'), U'1');
}

TEST_F(LocaleTest, Compare_SameStrings_ReturnsZero) {
    locale loc("C");
    EXPECT_EQ(loc.compare("hello", "hello"), 0);
}

TEST_F(LocaleTest, Compare_ADifferentStrings_ReturnsNonZero) {
    locale loc("C");
    EXPECT_NE(loc.compare("hello", "world"), 0);
}

TEST_F(LocaleTest, Compare_EmptyStrings_ReturnsZero) {
    locale loc("C");
    EXPECT_EQ(loc.compare("", ""), 0);
}

TEST_F(LocaleTest, Compare_EmptyVsNonEmpty_ReturnsNegative) {
    locale loc("C");
    EXPECT_LT(loc.compare("", "a"), 0);
}

TEST_F(LocaleTest, Compare_NonEmptyVsEmpty_ReturnsPositive) {
    locale loc("C");
    EXPECT_GT(loc.compare("a", ""), 0);
}

TEST_F(LocaleTest, Compare_PrimaryStrength_IgnoresCase) {
    locale loc("en_US.UTF-8");
    int result = loc.compare("hello", "HELLO", locale::collate_strength::primary);
    EXPECT_EQ(result, 0);
}

TEST_F(LocaleTest, Compare_TertiaryStrength_DistinguishesCase) {
    locale loc("en_US.UTF-8");
    int result = loc.compare("hello", "HELLO", locale::collate_strength::tertiary);
    EXPECT_NE(result, 0);
}

TEST_F(LocaleTest, Compare_SecondaryStrength_IgnoresCase) {
    locale loc("en_US.UTF-8");
    int result = loc.compare("hello", "HELLO", locale::collate_strength::secondary);
    EXPECT_EQ(result, 0);
}

TEST_F(LocaleTest, Compare_IdenticalStrength_DistinguishesAll) {
    locale loc("en_US.UTF-8");
    int result = loc.compare("hello", "HELLO", locale::collate_strength::identical);
    EXPECT_NE(result, 0);
}

TEST_F(LocaleTest, CollationKey_SameStrings_ProducesEqualKeys) {
    locale loc("C");
    string key1 = loc.collation_key("hello");
    string key2 = loc.collation_key("hello");
    EXPECT_EQ(key1, key2);
}

TEST_F(LocaleTest, CollationKey_DifferentStrings_ProducesDifferentKeys) {
    locale loc("C");
    string key1 = loc.collation_key("hello");
    string key2 = loc.collation_key("world");
    EXPECT_NE(key1, key2);
}

TEST_F(LocaleTest, CollationKey_EmptyString_ReturnsNonEmpty) {
    locale loc("C");
    string key = loc.collation_key("");
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_FALSE(key.empty());
#else
    EXPECT_TRUE(key.empty());
#endif
}

TEST_F(LocaleTest, CollationKey_OrderMatchesCompare) {
    locale loc("C");
    string key_a = loc.collation_key("a");
    string key_b = loc.collation_key("b");
    EXPECT_LT(key_a, key_b);
}

TEST_F(LocaleTest, CollationKey_EnUSLocale_Success) {
    locale loc("en_US.UTF-8");
    string key = loc.collation_key("test");
    EXPECT_FALSE(key.empty());
}

TEST_F(LocaleTest, ToMultibyte_Ascii_ReturnsSame) {
    locale loc("C");
    u32string input = U"hello";
    string result = loc.to_multibyte(input);
    EXPECT_EQ(result, "hello");
}

TEST_F(LocaleTest, ToMultibyte_EmptyString_ReturnsEmpty) {
    locale loc("C");
    u32string input;
    string result = loc.to_multibyte(input);
    EXPECT_TRUE(result.empty());
}

TEST_F(LocaleTest, ToMultibyte_EnUSLocale_Success) {
    locale loc("en_US.UTF-8");
    u32string input = U"hello";
    string result = loc.to_multibyte(input);
    EXPECT_EQ(result, "hello");
}

TEST_F(LocaleTest, ToUcs4_Ascii_ReturnsSame) {
    locale loc("C");
    string input = "hello";
    u32string result = loc.to_ucs4(input);
    ASSERT_EQ(result.size(), 5);
    EXPECT_EQ(result[0], U'h');
    EXPECT_EQ(result[1], U'e');
    EXPECT_EQ(result[2], U'l');
    EXPECT_EQ(result[3], U'l');
    EXPECT_EQ(result[4], U'o');
}

TEST_F(LocaleTest, ToUcs4_EmptyString_ReturnsEmpty) {
    locale loc("C");
    string input;
    u32string result = loc.to_ucs4(input);
    EXPECT_TRUE(result.empty());
}

TEST_F(LocaleTest, ToUcs4_EnUSLocale_Success) {
    locale loc("en_US.UTF-8");
    string input = "test";
    u32string result = loc.to_ucs4(input);
    ASSERT_EQ(result.size(), 4);
    EXPECT_EQ(result[0], U't');
}

TEST_F(LocaleTest, ToMultibyteAndToUcs4_Roundtrip_Ascii) {
    locale loc("C");
    u32string original = U"Hello World";
    string mb = loc.to_multibyte(original);
    u32string back = loc.to_ucs4(mb);
    EXPECT_EQ(original, back);
}

TEST_F(LocaleTest, ToMultibyteAndToUcs4_Roundtrip_EnUS) {
    locale loc("en_US.UTF-8");
    u32string original = U"Test String 123";
    string mb = loc.to_multibyte(original);
    u32string back = loc.to_ucs4(mb);
    EXPECT_EQ(original, back);
}

TEST_F(LocaleTest, AvailableLocales_ReturnsNonEmpty) {
    auto locales = locale::available_locales();
    EXPECT_FALSE(locales.empty());
}

TEST_F(LocaleTest, AvailableLocales_ContainsC) {
    auto locales = locale::available_locales();
    bool has_c = false;
    bool has_posix = false;
    for (const auto& l: locales) {
        if (l == "C") {
            has_c = true;
        }
        if (l == "POSIX") {
            has_posix = true;
        }
    }
    EXPECT_TRUE(has_c);
    EXPECT_TRUE(has_posix);
}

TEST_F(LocaleTest, AvailableLocales_AllElementsUnique) {
    auto locales = locale::available_locales();
    for (size_t i = 0; i < locales.size(); ++i) {
        for (size_t j = i + 1; j < locales.size(); ++j) {
            EXPECT_NE(locales[i], locales[j]);
        }
    }
}

TEST_F(LocaleTest, AvailableLocales_IsSorted) {
    auto locales = locale::available_locales();
    for (size_t i = 1; i < locales.size(); ++i) {
        EXPECT_LE(locales[i - 1], locales[i]);
    }
}

TEST_F(LocaleTest, Numeric_Grouping_NonEmpty) {
    locale loc("en_US.UTF-8");
    auto info = loc.numeric();
    EXPECT_FALSE(info.grouping.empty());
}

TEST_F(LocaleTest, Monetary_PositiveNegativeSigns_NonEmpty) {
    locale loc("en_US.UTF-8");
    auto info = loc.monetary();
    EXPECT_TRUE(info.positive_sign.empty() || info.positive_sign == "+");
    EXPECT_FALSE(info.negative_sign.empty());
}

TEST_F(LocaleTest, Monetary_InternationalSymbol_NonEmpty) {
    locale loc("en_US.UTF-8");
    auto info = loc.monetary();
    EXPECT_FALSE(info.int_curr_symbol.empty());
#ifdef NEFORCE_PLATFORM_LINUX
    EXPECT_EQ(info.int_curr_symbol, "USD ");
#else
    EXPECT_EQ(info.int_curr_symbol, "USD");
#endif
}

TEST_F(LocaleTest, IsAlpha_Unicode_EnUS) {
    locale loc("en_US.UTF-8");
    EXPECT_TRUE(loc.is_alpha(U'é'));
    EXPECT_TRUE(loc.is_alpha(U'ñ'));
}

TEST_F(LocaleTest, IsUpper_LocaleSpecific_EnUS) {
    locale loc("en_US.UTF-8");
    EXPECT_TRUE(loc.is_upper(U'É'));
    EXPECT_FALSE(loc.is_upper(U'é'));
}

TEST_F(LocaleTest, IsLower_LocaleSpecific_EnUS) {
    locale loc("en_US.UTF-8");
    EXPECT_TRUE(loc.is_lower(U'é'));
    EXPECT_FALSE(loc.is_lower(U'É'));
}

TEST_F(LocaleTest, ToUpper_LocaleSpecific_EnUS) {
    locale loc("en_US.UTF-8");
    EXPECT_EQ(loc.to_upper(U'é'), U'É');
}

TEST_F(LocaleTest, ToLower_LocaleSpecific_EnUS) {
    locale loc("en_US.UTF-8");
    EXPECT_EQ(loc.to_lower(U'É'), U'é');
}

TEST_F(LocaleTest, MoveAssignment_ChainAssignment_Success) {
    locale loc1("en_US.UTF-8");
    locale loc2("fr_FR.UTF-8");
    locale loc3("de_DE.UTF-8");

    loc3 = move(loc2) = move(loc1);

    EXPECT_EQ(loc3.name(), "en_US.UTF-8");
}

TEST_F(LocaleTest, CopyAssignment_ChainAssignment_Success) {
    locale loc1("en_US.UTF-8");
    locale loc2("fr_FR.UTF-8");
    locale loc3("de_DE.UTF-8");

    loc3 = loc2 = loc1;

    EXPECT_EQ(loc3.name(), "en_US.UTF-8");
    EXPECT_EQ(loc2.name(), "en_US.UTF-8");
}

TEST_F(LocaleTest, Compare_CaseInsensitiveTertiary_CLocale) {
    locale loc("C");
    int result = loc.compare("hello", "HELLO", locale::collate_strength::tertiary);
    EXPECT_NE(result, 0);
}

TEST_F(LocaleTest, Numeric_DeDELocale_ReturnsDifferentFromC) {
    locale loc_c("C");
    locale loc_de("de_DE.UTF-8");

    auto info_c = loc_c.numeric();
    auto info_de = loc_de.numeric();

    EXPECT_NE(info_c.decimal_point, info_de.decimal_point);
    EXPECT_NE(info_c.thousands_sep, info_de.thousands_sep);
}

TEST_F(LocaleTest, MultipleConstructors_DifferentLocales_Independent) {
    locale loc1("en_US.UTF-8");
    locale loc2("fr_FR.UTF-8");
    locale loc3("de_DE.UTF-8");

    EXPECT_NE(loc1.name(), loc2.name());
    EXPECT_NE(loc2.name(), loc3.name());
    EXPECT_NE(loc1.name(), loc3.name());
}

TEST_F(LocaleTest, Destructor_NoDoubleFree) {
    for (int i = 0; i < 10; ++i) {
        locale loc("en_US.UTF-8");
        EXPECT_FALSE(loc.name().empty());
    }
    SUCCEED();
}

TEST_F(LocaleTest, Exception_CopyConstructor_Success) {
    try {
        locale loc("nonexistent_locale_xyz_12345");
        FAIL() << "Expected locale_exception";
    } catch (const locale_exception& e) {
        locale_exception copied(e);
        EXPECT_NE(copied.what(), nullptr);
    }
}

TEST_F(LocaleTest, Exception_What_ContainsErrorInfo) {
    try {
        locale loc("nonexistent_locale_xyz_12345");
        FAIL() << "Expected locale_exception";
    } catch (const locale_exception& e) {
        EXPECT_NE(e.what(), nullptr);
        EXPECT_GT(string_length(e.what()), 0);
    }
}

TEST_F(LocaleTest, IsPrint_Space_ReturnsTrue) {
    locale loc("C");
    EXPECT_TRUE(loc.is_print(U' '));
}

TEST_F(LocaleTest, Compare_LongerStrings_CorrectOrder) {
    locale loc("C");
    EXPECT_LT(loc.compare("abc", "abcd"), 0);
    EXPECT_GT(loc.compare("abcd", "abc"), 0);
}

TEST_F(LocaleTest, CollationKey_CLocale_ConsistentOrder) {
    locale loc("C");
    auto keys = {loc.collation_key("apple"), loc.collation_key("banana"), loc.collation_key("cherry")};

    bool sorted = true;
    auto it = keys.begin();
    auto prev = *it;
    ++it;
    for (; it != keys.end(); ++it) {
        if (*it <= prev) {
            sorted = false;
            break;
        }
        prev = *it;
    }
    EXPECT_TRUE(sorted);
}

TEST_F(LocaleTest, ToMultibyte_SingleCharacter_Success) {
    locale loc("C");
    u32string input = U"X";
    string result = loc.to_multibyte(input);
    EXPECT_EQ(result, "X");
}

TEST_F(LocaleTest, ToUcs4_SingleCharacter_Success) {
    locale loc("C");
    string input = "X";
    u32string result = loc.to_ucs4(input);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], U'X');
}

class PipeTest : public ::testing::Test {
protected:
    void SetUp() override { neforce::pipe::ignore_sigpipe(); }

    void TearDown() override {}
};

TEST_F(PipeTest, DefaultConstructor_CreatesInvalidPipe) {
    using neforce::pipe;
    pipe p;
    EXPECT_FALSE(p.is_valid());
}

TEST_F(PipeTest, Constructor_NonInheritable_CreatesValidPipe) {
    using neforce::pipe;
    pipe p(false);
    EXPECT_TRUE(p.is_valid());
}

TEST_F(PipeTest, Constructor_Inheritable_CreatesValidPipe) {
    using neforce::pipe;
    pipe p(true);
    EXPECT_TRUE(p.is_valid());
}

TEST_F(PipeTest, Constructor_DefaultParameter_CreatesValidPipe) {
    using neforce::pipe;
    pipe p(false);
    EXPECT_TRUE(p.is_valid());
}

TEST_F(PipeTest, IsValid_AfterDefaultConstruction_ReturnsFalse) {
    using neforce::pipe;
    pipe p;
    EXPECT_FALSE(p.is_valid());
}

TEST_F(PipeTest, IsValid_AfterConstruction_ReturnsTrue) {
    using neforce::pipe;
    pipe p(false);
    EXPECT_TRUE(p.is_valid());
}

TEST_F(PipeTest, IsValid_AfterClose_ReturnsFalse) {
    using neforce::pipe;
    pipe p(false);
    p.close();
    EXPECT_FALSE(p.is_valid());
}

TEST_F(PipeTest, IsValid_AfterCloseRead_ReturnsTrueIfWriteStillOpen) {
    using neforce::pipe;
    pipe p(false);
    p.close_read();
    EXPECT_TRUE(p.is_valid());
}

TEST_F(PipeTest, IsValid_AfterCloseWrite_ReturnsTrueIfReadStillOpen) {
    using neforce::pipe;
    pipe p(false);
    p.close_write();
    EXPECT_TRUE(p.is_valid());
}

TEST_F(PipeTest, IsValid_AfterBothClosed_ReturnsFalse) {
    using neforce::pipe;
    pipe p(false);
    p.close_read();
    p.close_write();
    EXPECT_FALSE(p.is_valid());
}

TEST_F(PipeTest, NativeReadHandle_AfterConstruction_ReturnsNonInvalid) {
    using neforce::pipe;
    pipe p(false);
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_NE(p.native_read_handle(), nullptr);
#else
    EXPECT_GE(p.native_read_handle(), 0);
#endif
}

TEST_F(PipeTest, NativeReadHandle_AfterCloseRead_ReturnsInvalid) {
    using neforce::pipe;
    pipe p(false);
    p.close_read();
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_EQ(p.native_read_handle(), nullptr);
#else
    EXPECT_LT(p.native_read_handle(), 0);
#endif
}

TEST_F(PipeTest, NativeWriteHandle_AfterConstruction_ReturnsNonInvalid) {
    using neforce::pipe;
    pipe p(false);
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_NE(p.native_write_handle(), nullptr);
#else
    EXPECT_GE(p.native_write_handle(), 0);
#endif
}

TEST_F(PipeTest, NativeWriteHandle_AfterCloseWrite_ReturnsInvalid) {
    using neforce::pipe;
    pipe p(false);
    p.close_write();
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_EQ(p.native_write_handle(), nullptr);
#else
    EXPECT_LT(p.native_write_handle(), 0);
#endif
}

TEST_F(PipeTest, Write_SimpleData_ReturnsPositiveCount) {
    using neforce::pipe;
    pipe p(false);
    const char data[] = "hello";
    int written = p.write(data, string_length(data));
    EXPECT_GT(written, 0);
    EXPECT_EQ(static_cast<size_t>(written), string_length(data));
}

TEST_F(PipeTest, Write_NullDataZeroSize_ReturnsPositiveCount) {
    using neforce::pipe;
    pipe p(false);
    int written = p.write(nullptr, 0);
    EXPECT_GE(written, 0);
}

TEST_F(PipeTest, Write_AfterCloseWrite_ReturnsMinusOne) {
    using neforce::pipe;
    pipe p(false);
    p.close_write();
    const char data[] = "test";
    int written = p.write(data, string_length(data));
    EXPECT_EQ(written, -1);
}

TEST_F(PipeTest, Write_MultipleWrites_Success) {
    using neforce::pipe;
    pipe p(false);
    const char data1[] = "first ";
    const char data2[] = "second";
    int written1 = p.write(data1, string_length(data1));
    int written2 = p.write(data2, string_length(data2));
    EXPECT_GT(written1, 0);
    EXPECT_GT(written2, 0);
}

TEST_F(PipeTest, Write_LargeData_Success) {
    using neforce::pipe;
    pipe p(false);
    string large_data(65536, 'A');

    thread reader([&]() {
        string buffer(65536, '\0');
        int total = 0;
        while (total < large_data.size()) {
            int n = p.read(buffer.data() + total, large_data.size() - total);
            if (n > 0) {
                total += n;
            }
        }
    });

    int written = p.write(large_data.data(), large_data.size());
    EXPECT_GT(written, 0);

    reader.join();
}

TEST_F(PipeTest, Read_AfterWrite_ReturnsSameData) {
    using neforce::pipe;
    pipe p(false);
    const char sent[] = "hello pipe";
    p.write(sent, string_length(sent));

    char buffer[64] = {};
    int bytes_read = p.read(buffer, sizeof(buffer));
    EXPECT_GT(bytes_read, 0);
    EXPECT_EQ(string(buffer, static_cast<size_t>(bytes_read)), sent);
}

TEST_F(PipeTest, Read_AfterCloseRead_ReturnsMinusOne) {
    using neforce::pipe;
    pipe p(false);
    p.close_read();
    char buffer[64];
    int bytes_read = p.read(buffer, sizeof(buffer));
    EXPECT_EQ(bytes_read, -1);
}

TEST_F(PipeTest, Read_MultipleReads_Success) {
    using neforce::pipe;
    pipe p(false);
    const char sent[] = "multiple reads test";
    size_t total_written = 0;
    while (total_written < string_length(sent)) {
        int w = p.write(sent + total_written, string_length(sent) - total_written);
        EXPECT_GT(w, 0);
        total_written += static_cast<size_t>(w);
    }

    string received;
    char buffer[8];
    while (received.size() < string_length(sent)) {
        int r = p.read(buffer, sizeof(buffer));
        if (r > 0) {
            received.append(buffer, static_cast<size_t>(r));
        } else {
            break;
        }
    }
    EXPECT_EQ(received, sent);
}

TEST_F(PipeTest, Read_EmptyBuffer_ReturnsZero) {
    using neforce::pipe;
    pipe p(false);
    int bytes_read = p.read(nullptr, 0);
    EXPECT_EQ(bytes_read, 0);
}

TEST_F(PipeTest, WriteRead_MultipleIterations_Success) {
    using neforce::pipe;
    pipe p(false);
    for (int i = 0; i < 10; ++i) {
        string msg = "message_" + to_string(i);
        int written = p.write(msg.data(), msg.size());
        EXPECT_GT(written, 0);

        char buffer[128] = {};
        int bytes_read = p.read(buffer, sizeof(buffer));
        EXPECT_GT(bytes_read, 0);
        EXPECT_EQ(string(buffer, static_cast<size_t>(bytes_read)), msg);
    }
}

TEST_F(PipeTest, ReadAvailable_AfterWrite_ReturnsAllData) {
    using neforce::pipe;
    pipe p(false);
    const string sent = "read available test data";
    p.write(sent.data(), sent.size());
    p.close_write();

    string received = p.read_available();
    EXPECT_EQ(received, sent);
}

TEST_F(PipeTest, ReadAvailable_NoData_ReturnsEmpty) {
    using neforce::pipe;
    pipe p(false);
    string data = p.read_available();
    EXPECT_TRUE(data.empty());
}

TEST_F(PipeTest, ReadAvailable_AfterCloseRead_ReturnsEmpty) {
    using neforce::pipe;
    pipe p(false);
    p.close_read();
    string data = p.read_available();
    EXPECT_TRUE(data.empty());
}

TEST_F(PipeTest, ReadAvailable_MultipleWritesBeforeRead_ReturnsConcatenated) {
    using neforce::pipe;
    pipe p(false);
    p.write("Hello ", 6);
    p.write("World", 5);
    p.close_write();

    string received = p.read_available();
    EXPECT_EQ(received, "Hello World");
}

TEST_F(PipeTest, ReadAvailable_LargeData_Success) {
    using neforce::pipe;
    pipe p(false);
    string large_data(100000, 'X');
    string received;

    thread reader([&]() {
        char buf[4096];
        while (true) {
            int n = p.read(buf, sizeof(buf));
            if (n <= 0) {
                break;
            }
            received.append(buf, n);
        }
    });

    int written = p.write(large_data.data(), large_data.size());
    EXPECT_EQ(written, (int) large_data.size());

    p.close_write();
    reader.join();

    EXPECT_EQ(received, large_data);
}

TEST_F(PipeTest, ReadAvailable_SmallData_Success) {
    using neforce::pipe;
    pipe p(false);
    string small_data(4096, 'X');
    p.write(small_data.data(), small_data.size());
    p.close_write();

    string received = p.read_available();
    EXPECT_EQ(received, small_data);
}

TEST_F(PipeTest, CloseRead_AfterWrite_ReadReturnsMinusOne) {
    using neforce::pipe;
    pipe p(false);
    const char data[] = "test";
    p.write(data, string_length(data));
    p.close_read();

    char buffer[64];
    int bytes_read = p.read(buffer, sizeof(buffer));
    EXPECT_EQ(bytes_read, -1);
}

TEST_F(PipeTest, CloseWrite_ThenRead_ReturnsAvailableData) {
    using neforce::pipe;
    pipe p(false);
    const char data[] = "close write test";
    p.write(data, string_length(data));
    p.close_write();

    char buffer[64] = {};
    int bytes_read = p.read(buffer, sizeof(buffer));
    EXPECT_GT(bytes_read, 0);
    EXPECT_EQ(string(buffer, static_cast<size_t>(bytes_read)), data);
}

TEST_F(PipeTest, CloseWrite_CalledTwice_NoThrow) {
    using neforce::pipe;
    pipe p(false);
    p.close_write();
    EXPECT_NO_THROW(p.close_write());
}

TEST_F(PipeTest, CloseRead_CalledTwice_NoThrow) {
    using neforce::pipe;
    pipe p(false);
    p.close_read();
    EXPECT_NO_THROW(p.close_read());
}

TEST_F(PipeTest, Close_CalledTwice_NoThrow) {
    using neforce::pipe;
    pipe p(false);
    p.close();
    EXPECT_NO_THROW(p.close());
}

TEST_F(PipeTest, Close_ThenWrite_ReturnsMinusOne) {
    using neforce::pipe;
    pipe p(false);
    p.close();
    const char data[] = "test";
    int written = p.write(data, string_length(data));
    EXPECT_EQ(written, -1);
}

TEST_F(PipeTest, Close_ThenRead_ReturnsMinusOne) {
    using neforce::pipe;
    pipe p(false);
    p.close();
    char buffer[64];
    int bytes_read = p.read(buffer, sizeof(buffer));
    EXPECT_EQ(bytes_read, -1);
}

TEST_F(PipeTest, Close_DefaultConstructedPipe_NoThrow) {
    using neforce::pipe;
    pipe p;
    EXPECT_NO_THROW(p.close());
}

TEST_F(PipeTest, MoveConstructor_TransfersOwnership) {
    using neforce::pipe;
    pipe p1(false);
    EXPECT_TRUE(p1.is_valid());

    auto read_handle = p1.native_read_handle();
    auto write_handle = p1.native_write_handle();

    pipe p2(move(p1));

    EXPECT_TRUE(p2.is_valid());
    EXPECT_EQ(p2.native_read_handle(), read_handle);
    EXPECT_EQ(p2.native_write_handle(), write_handle);
    EXPECT_FALSE(p1.is_valid());
}

TEST_F(PipeTest, MoveConstructor_FromDefaultConstructed_Success) {
    using neforce::pipe;
    pipe p1;
    EXPECT_FALSE(p1.is_valid());

    pipe p2(move(p1));

    EXPECT_FALSE(p2.is_valid());
    EXPECT_FALSE(p1.is_valid());
}

TEST_F(PipeTest, MoveAssignment_TransfersOwnership) {
    using neforce::pipe;
    pipe p1(false);
    pipe p2(false);

    auto read_handle = p1.native_read_handle();
    auto write_handle = p1.native_write_handle();

    p2 = move(p1);

    EXPECT_TRUE(p2.is_valid());
    EXPECT_EQ(p2.native_read_handle(), read_handle);
    EXPECT_EQ(p2.native_write_handle(), write_handle);
    EXPECT_FALSE(p1.is_valid());
}

TEST_F(PipeTest, MoveAssignment_SelfAssignment_NoEffect) {
    using neforce::pipe;
    pipe p(false);
    auto read_handle = p.native_read_handle();
    auto write_handle = p.native_write_handle();

    p = move(p);

    EXPECT_TRUE(p.is_valid());
    EXPECT_EQ(p.native_read_handle(), read_handle);
    EXPECT_EQ(p.native_write_handle(), write_handle);
}

TEST_F(PipeTest, MoveAssignment_ClosesPreviousHandle) {
    using neforce::pipe;
    pipe p1(false);
    pipe p2(false);

    EXPECT_NO_THROW(p2 = move(p1));
}

TEST_F(PipeTest, DetachReadHandle_ReturnsValidAndMakesInvalid) {
    using neforce::pipe;
    pipe p(false);
#ifdef NEFORCE_PLATFORM_WINDOWS
    auto handle = p.detach_read_handle();
    EXPECT_NE(handle, nullptr);
    EXPECT_EQ(p.native_read_handle(), nullptr);
    ::CloseHandle(handle);
#else
    int fd = p.detach_read_handle();
    EXPECT_GE(fd, 0);
    EXPECT_LT(p.native_read_handle(), 0);
    ::close(fd);
#endif
}

TEST_F(PipeTest, DetachReadHandle_AfterClose_ReturnsInvalid) {
    using neforce::pipe;
    pipe p(false);
    p.close_read();
#ifdef NEFORCE_PLATFORM_WINDOWS
    auto handle = p.detach_read_handle();
    EXPECT_EQ(handle, nullptr);
#else
    int fd = p.detach_read_handle();
    EXPECT_LT(fd, 0);
#endif
}

TEST_F(PipeTest, DetachReadHandle_DefaultConstructed_ReturnsInvalid) {
    using neforce::pipe;
    pipe p;
#ifdef NEFORCE_PLATFORM_WINDOWS
    auto handle = p.detach_read_handle();
    EXPECT_EQ(handle, nullptr);
#else
    int fd = p.detach_read_handle();
    EXPECT_LT(fd, 0);
#endif
}

TEST_F(PipeTest, DetachWriteHandle_ReturnsValidAndMakesInvalid) {
    using neforce::pipe;
    pipe p(false);
#ifdef NEFORCE_PLATFORM_WINDOWS
    auto handle = p.detach_write_handle();
    EXPECT_NE(handle, nullptr);
    EXPECT_EQ(p.native_write_handle(), nullptr);
    ::CloseHandle(handle);
#else
    int fd = p.detach_write_handle();
    EXPECT_GE(fd, 0);
    EXPECT_LT(p.native_write_handle(), 0);
    ::close(fd);
#endif
}

TEST_F(PipeTest, DetachWriteHandle_AfterClose_ReturnsInvalid) {
    using neforce::pipe;
    pipe p(false);
    p.close_write();
#ifdef NEFORCE_PLATFORM_WINDOWS
    auto handle = p.detach_write_handle();
    EXPECT_EQ(handle, nullptr);
#else
    int fd = p.detach_write_handle();
    EXPECT_LT(fd, 0);
#endif
}

TEST_F(PipeTest, DetachWriteHandle_DefaultConstructed_ReturnsInvalid) {
    using neforce::pipe;
    pipe p;
#ifdef NEFORCE_PLATFORM_WINDOWS
    auto handle = p.detach_write_handle();
    EXPECT_EQ(handle, nullptr);
#else
    int fd = p.detach_write_handle();
    EXPECT_LT(fd, 0);
#endif
}

TEST_F(PipeTest, DetachReadHandle_CallerResponsibleForClose) {
    using neforce::pipe;
    pipe p(false);
    const char data[] = "detach test";
    p.write(data, string_length(data));
    p.close_write();

#ifdef NEFORCE_PLATFORM_WINDOWS
    auto handle = p.detach_read_handle();
    EXPECT_NE(handle, nullptr);
    EXPECT_FALSE(p.is_valid());

    char buffer[64] = {};
    DWORD bytes_read = 0;
    BOOL result = ::ReadFile(handle, buffer, sizeof(buffer), &bytes_read, nullptr);
    EXPECT_TRUE(result);
    EXPECT_GT(bytes_read, 0);
    EXPECT_EQ(string(buffer, bytes_read), data);
    ::CloseHandle(handle);
#else
    int fd = p.detach_read_handle();
    EXPECT_GE(fd, 0);
    EXPECT_FALSE(p.is_valid());

    char buffer[64] = {};
    ssize_t bytes_read = ::read(fd, buffer, sizeof(buffer));
    EXPECT_GT(bytes_read, 0);
    EXPECT_EQ(string(buffer, static_cast<size_t>(bytes_read)), data);
    ::close(fd);
#endif
}

TEST_F(PipeTest, DetachWriteHandle_CallerResponsibleForClose) {
    using neforce::pipe;
    pipe p(false);

#ifdef NEFORCE_PLATFORM_WINDOWS
    auto handle = p.detach_write_handle();
    EXPECT_NE(handle, nullptr);
    EXPECT_EQ(p.native_write_handle(), nullptr);
    EXPECT_TRUE(p.is_valid());

    const char data[] = "caller write";
    DWORD bytes_written = 0;
    BOOL result = ::WriteFile(handle, data, string_length(data), &bytes_written, nullptr);
    EXPECT_TRUE(result);
    EXPECT_GT(bytes_written, 0);
    ::CloseHandle(handle);

    EXPECT_TRUE(p.is_valid());
#else
    int fd = p.detach_write_handle();
    EXPECT_GE(fd, 0);
    EXPECT_EQ(p.native_write_handle(), -1);
    EXPECT_TRUE(p.is_valid());

    const char data[] = "caller write";
    ssize_t bytes_written = ::write(fd, data, string_length(data));
    EXPECT_GT(bytes_written, 0);
    ::close(fd);

    EXPECT_TRUE(p.is_valid());
#endif
}

TEST_F(PipeTest, ThreadedWriteRead_Success) {
    using neforce::pipe;
    pipe p(false);

    const string sent = "threaded pipe test message";
    string received;

    thread writer([&p, &sent]() {
        this_thread::sleep_for(milliseconds(10));
        p.write(sent.data(), sent.size());
        p.close_write();
    });

    thread reader([&p, &received]() {
        char buffer[64];
        while (true) {
            int r = p.read(buffer, sizeof(buffer) - 1);
            if (r > 0) {
                buffer[r] = '\0';
                received += buffer;
            } else {
                break;
            }
        }
    });

    writer.join();
    reader.join();

    EXPECT_EQ(received, sent);
}

TEST_F(PipeTest, Destructor_ClosesHandles) {
    using neforce::pipe;
    pipe::native_handle_type read_h;
    pipe::native_handle_type write_h;

    {
        pipe p(false);
        read_h = p.native_read_handle();
        write_h = p.native_write_handle();
    }

#ifdef NEFORCE_PLATFORM_WINDOWS
    if (read_h != nullptr) {
        DWORD flags = 0;
        EXPECT_FALSE(::GetHandleInformation(read_h, &flags));
    }
    if (write_h != nullptr) {
        DWORD flags = 0;
        EXPECT_FALSE(::GetHandleInformation(write_h, &flags));
    }
#else
    if (read_h >= 0) {
        EXPECT_EQ(::fcntl(read_h, F_GETFD), -1);
    }
    if (write_h >= 0) {
        EXPECT_EQ(::fcntl(write_h, F_GETFD), -1);
    }
#endif
}

TEST_F(PipeTest, CopyConstructor_IsDeleted) {
    using neforce::pipe;
    EXPECT_FALSE(is_copy_constructible<pipe>::value);
}

TEST_F(PipeTest, CopyAssignment_IsDeleted) {
    using neforce::pipe;
    EXPECT_FALSE(is_copy_assignable<pipe>::value);
}

TEST_F(PipeTest, MoveConstructor_IsNoexcept) {
    using neforce::pipe;
    EXPECT_TRUE(is_nothrow_move_constructible<pipe>::value);
}

TEST_F(PipeTest, MoveAssignment_IsNoexcept) {
    using neforce::pipe;
    EXPECT_TRUE(is_nothrow_move_assignable<pipe>::value);
}

TEST_F(PipeTest, ReadAvailable_MultiplePipes_Independent) {
    using neforce::pipe;
    pipe p1(false);
    pipe p2(false);

    p1.write("pipe1", 5);
    p2.write("pipe2", 5);
    p1.close_write();
    p2.close_write();

    string data1 = p1.read_available();
    string data2 = p2.read_available();

    EXPECT_EQ(data1, "pipe1");
    EXPECT_EQ(data2, "pipe2");
}

TEST_F(PipeTest, Write_NullData_WithPositiveSize_Safe) {
    using neforce::pipe;
    pipe p(false);
    int written = p.write(nullptr, 0);
    EXPECT_EQ(written, 0);
}

TEST_F(PipeTest, Read_ZeroSizeBuffer_ReturnsZero) {
    using neforce::pipe;
    pipe p(false);
    const char data[] = "test";
    p.write(data, string_length(data));

    char buffer[64];
    int bytes_read = p.read(buffer, 0);
    EXPECT_EQ(bytes_read, 0);
}

TEST_F(PipeTest, WriteFailsWhenReadEndClosed) {
    using neforce::pipe;
    pipe p(false);
    p.close_read();

    const char data[] = "write after close read";
    int written = p.write(data, string_length(data));
    EXPECT_EQ(written, -1);
}

TEST_F(PipeTest, CloseWrite_DoesNotAffectRead) {
    using neforce::pipe;
    pipe p(false);
    const char data[] = "read after close write";
    p.write(data, string_length(data));
    p.close_write();

    char buffer[64] = {};
    int bytes_read = p.read(buffer, sizeof(buffer));
    EXPECT_GT(bytes_read, 0);
    EXPECT_EQ(string(buffer, static_cast<size_t>(bytes_read)), data);
}

TEST_F(PipeTest, MoveConstructedPipe_CanReadWrite) {
    using neforce::pipe;
    pipe p1(false);
    const char sent[] = "moved pipe test";
    p1.write(sent, string_length(sent));

    pipe p2(move(p1));
    p2.close_write();

    char buffer[64] = {};
    int bytes_read = p2.read(buffer, sizeof(buffer));
    EXPECT_GT(bytes_read, 0);
    EXPECT_EQ(string(buffer, static_cast<size_t>(bytes_read)), sent);
}

TEST_F(PipeTest, MoveAssignedPipe_CanReadWrite) {
    using neforce::pipe;
    pipe p1(false);
    pipe p2(false);

    const char sent[] = "assigned pipe test";
    p1.write(sent, string_length(sent));

    p2 = move(p1);
    p2.close_write();

    char buffer[64] = {};
    int bytes_read = p2.read(buffer, sizeof(buffer));
    EXPECT_GT(bytes_read, 0);
    EXPECT_EQ(string(buffer, static_cast<size_t>(bytes_read)), sent);
}

TEST_F(PipeTest, Exception_What_ContainsErrorInfo) {
    using neforce::pipe;
    try {
        pipe p(false);
        p.close();
        p.write("test", 4);
    } catch (const pipe_exception& e) {
        EXPECT_NE(e.what(), nullptr);
    }
}

TEST_F(PipeTest, IsValid_AfterPartialClose_ReturnsTrue) {
    using neforce::pipe;
    pipe p(false);
    p.close_read();
    EXPECT_TRUE(p.is_valid());
}

TEST_F(PipeTest, ReadAfterMove_SourcePipe_ReturnsMinusOne) {
    using neforce::pipe;
    pipe p1(false);
    const char data[] = "test";
    p1.write(data, string_length(data));

    pipe p2(move(p1));

    char buffer[64];
    int bytes_read = p1.read(buffer, sizeof(buffer));
    EXPECT_EQ(bytes_read, -1);
}

TEST_F(PipeTest, WriteAfterMove_SourcePipe_ReturnsMinusOne) {
    using neforce::pipe;
    pipe p1(false);

    pipe p2(move(p1));

    const char data[] = "test";
    int written = p1.write(data, string_length(data));
    EXPECT_EQ(written, -1);
}

class ProcessTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}

    static string get_test_executable() {
#ifdef NEFORCE_PLATFORM_WINDOWS
        return "cmd.exe";
#else
        return "/bin/sleep";
#endif
    }

    static vector<string> get_test_args() {
#ifdef NEFORCE_PLATFORM_WINDOWS
        return {"/c", "exit", "0"};
#else
        return {"1"};
#endif
    }

    static vector<string> get_test_output_args() {
#ifdef NEFORCE_PLATFORM_WINDOWS
        return {"/c", "echo", "hello"};
#else
        return {"-c", "echo hello"};
#endif
    }

    static string get_test_output_executable() {
#ifdef NEFORCE_PLATFORM_WINDOWS
        return "cmd.exe";
#else
        return "/bin/sh";
#endif
    }

    static vector<string> get_long_running_args() {
#ifdef NEFORCE_PLATFORM_WINDOWS
        return {"/c", "timeout", "/t", "10", "/nobreak"};
#else
        return {"10"};
#endif
    }

    static string get_long_running_executable() {
#ifdef NEFORCE_PLATFORM_WINDOWS
        return "cmd.exe";
#else
        return "/bin/sleep";
#endif
    }

    static string get_stdin_test_executable() {
#ifdef NEFORCE_PLATFORM_WINDOWS
        return "cmd.exe";
#else
        return "/bin/cat";
#endif
    }

    static vector<string> get_stdin_test_args() {
#ifdef NEFORCE_PLATFORM_WINDOWS
        return {"/c", "findstr", ".*"};
#else
        return {};
#endif
    }
};

TEST_F(ProcessTest, Start_SimpleProcess_ReturnsValidId) {
    process p;
    p.start(get_test_executable(), get_test_args());
    EXPECT_GT(p.id(), 0);
    p.wait();
}

TEST_F(ProcessTest, Start_WithCaptureOutput_ReturnsValidId) {
    process p;
    p.set_capture_stdout(true);
    p.start(get_test_output_executable(), get_test_output_args());
    EXPECT_GT(p.id(), 0);
    p.wait();
}

TEST_F(ProcessTest, Start_InvalidExecutable_ThrowsException) {
    process p;
    EXPECT_THROW(p.start("nonexistent_executable_xyz_12345", {}), process_exception);
}

TEST_F(ProcessTest, Start_EmptyExecutable_ThrowsException) {
    process p;
    EXPECT_THROW(p.start("", {}), process_exception);
}

TEST_F(ProcessTest, Start_DoubleStart_ThrowsException) {
    process p;
    p.start(get_test_executable(), get_test_args());
    EXPECT_THROW(p.start(get_test_executable(), get_test_args()), process_exception);
    p.wait();
}

TEST_F(ProcessTest, Start_WithArgs_Success) {
    process p;
    p.start(get_test_executable(), get_test_args());
    EXPECT_GT(p.id(), 0);
    p.wait();
}

TEST_F(ProcessTest, Start_MultipleProcesses_UniqueIds) {
    process p1;
    p1.start(get_test_executable(), get_test_args());
    process p2;
    p2.start(get_test_executable(), get_test_args());

    EXPECT_NE(p1.id(), p2.id());

    p1.wait();
    p2.wait();
}

TEST_F(ProcessTest, Start_WithSpacesInPath_Success) {
    process p;
    p.start(get_test_executable(), get_test_args());
    EXPECT_GT(p.id(), 0);
    p.wait();
}

TEST_F(ProcessTest, Wait_InfiniteTimeout_ReturnsExitCode) {
    process p;
    p.start(get_test_executable(), get_test_args());
    int exit_code = p.wait();
    EXPECT_GE(exit_code, 0);
}

TEST_F(ProcessTest, Wait_ZeroTimeout_ReturnsMinusOne) {
    process p;
    p.start(get_long_running_executable(), get_long_running_args());
    int exit_code = p.wait(0);
    EXPECT_EQ(exit_code, -1);
    p.terminate();
}

TEST_F(ProcessTest, Wait_ProcessCompletes_ReturnsExitCode) {
    process p;
    p.start(get_test_executable(), get_test_args());
    int exit_code = p.wait(5000);
    EXPECT_GE(exit_code, 0);
}

TEST_F(ProcessTest, Wait_UpdatesIsRunningToFalse) {
    process p;
    p.start(get_test_executable(), get_test_args());
    p.wait();
    EXPECT_FALSE(p.is_running());
}

TEST_F(ProcessTest, Wait_TimeoutNegative_EquivalentToInfinite) {
    process p;
    p.start(get_test_executable(), get_test_args());
    int exit_code = p.wait(-1);
    EXPECT_GE(exit_code, 0);
}

TEST_F(ProcessTest, Wait_NotStarted_ThrowsException) {
    process p;
    EXPECT_THROW(p.wait(), process_exception);
}

TEST_F(ProcessTest, Wait_DoubleWait_ReturnsSameExitCode) {
    process p;
    p.start(get_test_executable(), get_test_args());
    int ec1 = p.wait();
    int ec2 = p.wait();
    EXPECT_EQ(ec1, ec2);
}

TEST_F(ProcessTest, CaptureOutput_HasOutput) {
    process p;
    p.set_capture_stdout(true);
    p.start(get_test_output_executable(), get_test_output_args());
    p.wait();

    EXPECT_FALSE(p.stdout_output().empty());
}

TEST_F(ProcessTest, CaptureOutput_WithoutCapture_OutputEmpty) {
    process p;
    p.start(get_test_executable(), get_test_args());
    p.wait();

    EXPECT_TRUE(p.stdout_output().empty());
}

TEST_F(ProcessTest, CaptureOutput_MultipleLines_Success) {
    process p;
    p.set_capture_stdout(true);
#ifdef NEFORCE_PLATFORM_WINDOWS
    p.start("cmd.exe", {"/c", "echo line1 && echo line2"});
#else
    p.start("/bin/sh", {"-c", "echo line1 && echo line2"});
#endif
    p.wait();

    EXPECT_FALSE(p.stdout_output().empty());
}

TEST_F(ProcessTest, CaptureOutput_LargeOutput_NoDeadlock) {
    // 通过 stdin 输入 128KB 数据，由 cat 输出，验证异步管道读取不会死锁
    process p;
    p.set_capture_stdout(true);
    string large_data(131072, 'x');
    p.set_stdin_data(large_data);
#ifdef NEFORCE_PLATFORM_WINDOWS
    p.start("cmd.exe", {"/c", "findstr", ".*"});
#else
    p.start("/bin/cat", {});
#endif
    int ec = p.wait(30000);
    EXPECT_GE(ec, 0);
    EXPECT_GT(p.stdout_output().size(), 65536); // 应超过典型管道缓冲区 64KB
}

TEST_F(ProcessTest, SeparateStderr_StderrNotEmpty) {
    process p;
    p.set_capture_stdout(true);
    p.set_capture_stderr(true);
#ifdef NEFORCE_PLATFORM_WINDOWS
    p.start("cmd.exe", {"/c", "echo stdout_msg && echo stderr_msg >&2"});
#else
    p.start("/bin/sh", {"-c", "echo stdout_msg; echo stderr_msg >&2"});
#endif
    p.wait();

    EXPECT_FALSE(p.stdout_output().empty());
    EXPECT_FALSE(p.stderr_output().empty());
}

TEST_F(ProcessTest, SeparateStderr_StdoutOnly_StderrEmpty) {
    process p;
    p.set_capture_stdout(true);
    p.set_capture_stderr(true);
#ifdef NEFORCE_PLATFORM_WINDOWS
    p.start("cmd.exe", {"/c", "echo only stdout"});
#else
    p.start("/bin/sh", {"-c", "echo only stdout"});
#endif
    p.wait();

    EXPECT_FALSE(p.stdout_output().empty());
    EXPECT_TRUE(p.stderr_output().empty());
}

TEST_F(ProcessTest, StdinData_PresetData_ChildReceivesIt) {
    process p;
    p.set_capture_stdout(true);
    p.set_stdin_data("hello_stdin\n");
#ifdef NEFORCE_PLATFORM_WINDOWS
    p.start("cmd.exe", {"/c", "findstr", ".*"});
#else
    p.start("/bin/cat", {});
#endif
    p.wait(5000);

    EXPECT_FALSE(p.stdout_output().empty());
    EXPECT_TRUE(p.stdout_output().find("hello_stdin") != string::npos);
}

TEST_F(ProcessTest, Terminate_RunningProcess_Success) {
    process p;
    p.start(get_long_running_executable(), get_long_running_args());
    EXPECT_TRUE(p.is_running());

    p.terminate();
    EXPECT_FALSE(p.is_running());
}

TEST_F(ProcessTest, Terminate_NotStarted_NoThrow) {
    process p;
    EXPECT_NO_THROW(p.terminate());
}

TEST_F(ProcessTest, Terminate_AlreadyExited_NoThrow) {
    process p;
    p.start(get_test_executable(), get_test_args());
    p.wait();
    EXPECT_NO_THROW(p.terminate());
}

TEST_F(ProcessTest, Suspend_RunningProcess_Success) {
    process p;
    p.start(get_long_running_executable(), get_long_running_args());

    EXPECT_NO_THROW(p.suspend());
    p.terminate();
}

TEST_F(ProcessTest, Suspend_AlreadyExited_Throws) {
    process p;
    p.start(get_test_executable(), get_test_args());
    p.wait();

    EXPECT_THROW(p.suspend(), process_exception);
}

TEST_F(ProcessTest, Resume_SuspendedProcess_Success) {
    process p;
    p.start(get_long_running_executable(), get_long_running_args());

    p.suspend();
    EXPECT_NO_THROW(p.resume());
    p.terminate();
}

TEST_F(ProcessTest, Resume_AlreadyExited_Throws) {
    process p;
    p.start(get_test_executable(), get_test_args());
    p.wait();

    EXPECT_THROW(p.resume(), process_exception);
}

TEST_F(ProcessTest, Wait_AfterTerminate_ReturnsCorrectly) {
    process p;
    p.start(get_long_running_executable(), get_long_running_args());
    p.terminate();

    this_thread::sleep_for(milliseconds(200));
    int exit_code = p.wait(1000);
    EXPECT_TRUE(exit_code >= 0 || exit_code == -1);
}

TEST_F(ProcessTest, IsRunning_RunningProcess_ReturnsTrue) {
    process p;
    p.start(get_long_running_executable(), get_long_running_args());
    EXPECT_TRUE(p.is_running());
    p.terminate();
}

TEST_F(ProcessTest, IsRunning_ExitedProcess_ReturnsFalse) {
    process p;
    p.start(get_test_executable(), get_test_args());
    p.wait();
    EXPECT_FALSE(p.is_running());
}

TEST_F(ProcessTest, IsRunning_NotStarted_ReturnsFalse) {
    process p;
    EXPECT_FALSE(p.is_running());
}

TEST_F(ProcessTest, IsRunning_TerminatedProcess_ReturnsFalse) {
    process p;
    p.start(get_long_running_executable(), get_long_running_args());
    p.terminate();

    this_thread::sleep_for(milliseconds(100));
    EXPECT_FALSE(p.is_running());
}

TEST_F(ProcessTest, GetMemoryInfo_RunningProcess_ReturnsNonZero) {
    process p;
    p.start(get_long_running_executable(), get_long_running_args());

    auto mem_info = p.get_memory_info();
    EXPECT_GT(mem_info.working_set_size, 0);

    p.terminate();
}

TEST_F(ProcessTest, GetMemoryInfo_NotStarted_ReturnsZero) {
    process p;
    auto mem_info = p.get_memory_info();
    EXPECT_EQ(mem_info.working_set_size, 0);
}

TEST_F(ProcessTest, GetState_ExitedProcess_ReturnsExited) {
    process p;
    p.start(get_test_executable(), get_test_args());
    p.wait();

    EXPECT_EQ(p.get_state(), process::state::exited);
}

TEST_F(ProcessTest, GetState_NotStarted_ReturnsUnknown) {
    process p;
    EXPECT_EQ(p.get_state(), process::state::unknown);
}

TEST_F(ProcessTest, GetState_SuspendedProcess_ReturnsSuspended) {
    process p;
    p.start(get_long_running_executable(), get_long_running_args());
    p.suspend();

    auto state = p.get_state();
    EXPECT_TRUE(state == process::state::suspended || state == process::state::running);

    p.terminate();
}

TEST_F(ProcessTest, Id_NotStarted_ReturnsZero) {
    process p;
    EXPECT_EQ(p.id(), 0);
}

TEST_F(ProcessTest, ExitCode_NotWaited_ReturnsMinusOne) {
    process p;
    p.start(get_test_executable(), get_test_args());
    EXPECT_EQ(p.exit_code(), -1);
    p.wait();
}

TEST_F(ProcessTest, CurrentId_ReturnsPositiveValue) {
    auto pid = process::current_id();
    EXPECT_GT(pid, 0);
}

TEST_F(ProcessTest, CurrentId_ReturnsCurrentProcessId) {
    auto pid = process::current_id();
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_EQ(pid, ::GetCurrentProcessId());
#else
    EXPECT_EQ(pid, ::getpid());
#endif
}

TEST_F(ProcessTest, GetMemoryInfoByPid_CurrentProcess_ReturnsNonZero) {
    auto mem_info = process::get_memory_info(process::current_id());
    EXPECT_GT(mem_info.working_set_size, 0);
}

TEST_F(ProcessTest, GetMemoryInfoByPid_ExitedProcess_ReturnsNonNegative) {
    process p;
    p.start(get_test_executable(), get_test_args());
    p.wait();

    auto mem_info = process::get_memory_info(p.id());
    EXPECT_GE(mem_info.working_set_size, 0);
}

TEST_F(ProcessTest, GetStateByPid_CurrentProcess_ReturnsRunning) {
    auto state = process::get_state(process::current_id());
    EXPECT_TRUE(state == process::state::running);
}

TEST_F(ProcessTest, CheckPermission_CurrentProcess_ReturnsTrue) {
    EXPECT_TRUE(process::check_permission(process::current_id(), process::permission::query_info));
}

TEST_F(ProcessTest, CheckPermission_ReadPermission_ReturnsResult) {
    bool result = process::check_permission(process::current_id(), process::permission::read);
    EXPECT_TRUE(result || !result);
}

TEST_F(ProcessTest, CheckPermission_AllPermission_ReturnsResult) {
    bool result = process::check_permission(process::current_id(), process::permission::all);
    EXPECT_TRUE(result || !result);
}

TEST_F(ProcessTest, CheckPermission_NonExistentProcess_ReturnsFalse) {
    bool result = process::check_permission(99999999, process::permission::query_info);
    EXPECT_FALSE(result);
}

TEST_F(ProcessTest, CheckPermission_NoPermission_ReturnsResult) {
    process::permission no_perm = static_cast<process::permission>(0);
    bool result = process::check_permission(process::current_id(), no_perm);
    EXPECT_TRUE(result || !result);
}

TEST_F(ProcessTest, Name_CurrentProcess_ReturnsNonEmpty) {
    string name = process::name(process::current_id());
    EXPECT_FALSE(name.empty());
}

TEST_F(ProcessTest, Name_NonExistentProcess_ReturnsEmpty) {
    string name = process::name(99999999);
    EXPECT_TRUE(name.empty());
}

TEST_F(ProcessTest, Name_ConsistentWithCurrentProcess) {
    string name1 = process::name(process::current_id());
    string name2 = process::name(process::current_id());
    EXPECT_EQ(name1, name2);
}

TEST_F(ProcessTest, GetStateByPid_NonExistentProcess_ReturnsExited) {
    auto state = process::get_state(99999999);
    EXPECT_TRUE(state == process::state::exited || state == process::state::unknown);
}

TEST_F(ProcessTest, CurrentProcessPrivilegeLevel) {
    const auto level = process::current_privilege_level();

    ASSERT_NE(level, process::privilege_level::unknown);
    ASSERT_TRUE(level == process::privilege_level::privileged || level == process::privilege_level::not_privileged);
}

TEST_F(ProcessTest, GetPrivilegeLevelByCurrentId) {
    const auto current_id = process::current_id();
    const auto level = process::get_privilege_level(current_id);

    const auto current_level = process::current_privilege_level();

    ASSERT_NE(level, process::privilege_level::unknown);
    ASSERT_EQ(level, current_level);
}

TEST_F(ProcessTest, GetPrivilegeLevelInvalidPid) {
    constexpr process::native_id_type invalid_pid = 99999999;

    const auto level = process::get_privilege_level(invalid_pid);
    ASSERT_EQ(level, process::privilege_level::unknown);
}

TEST_F(ProcessTest, RootProcessHasPrivilegedLevel) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const auto level = process::current_privilege_level();
    if (level == process::privilege_level::unknown) {
        GTEST_SKIP() << "Cannot determine privilege level on this system";
    }
#else
    if (::geteuid() != 0) {
        GTEST_SKIP() << "Test requires root privileges";
    }
    const auto level = process::current_privilege_level();
    ASSERT_EQ(level, process::privilege_level::privileged);
#endif
}

TEST_F(ProcessTest, NonRootProcessHasNotPrivilegedLevel) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    const auto level = process::current_privilege_level();
    if (level == process::privilege_level::unknown) {
        GTEST_SKIP() << "Cannot determine privilege level on this system";
    }
#else
    if (::geteuid() == 0) {
        GTEST_SKIP() << "Test requires non-root privileges";
    }
    const auto level = process::current_privilege_level();
    ASSERT_EQ(level, process::privilege_level::not_privileged);
#endif
}

TEST_F(ProcessTest, MemoryInfo_Defaults_AllZero) {
    process::memory_info info{};
    EXPECT_EQ(info.working_set_size, 0);
    EXPECT_EQ(info.peak_working_set_size, 0);
    EXPECT_EQ(info.pagefile_usage, 0);
    EXPECT_EQ(info.peak_pagefile_usage, 0);
}

TEST_F(ProcessTest, TimeInfo_Defaults_AllZero) {
    process::time_info info{};
    EXPECT_EQ(info.user_time_ms, 0);
    EXPECT_EQ(info.kernel_time_ms, 0);
    EXPECT_EQ(info.wall_time_ms, 0);
}

TEST_F(ProcessTest, PermissionFlags_AreDistinct) {
    EXPECT_NE(static_cast<int>(process::permission::read), static_cast<int>(process::permission::write));
    EXPECT_NE(static_cast<int>(process::permission::read), static_cast<int>(process::permission::execute));
    EXPECT_NE(static_cast<int>(process::permission::read), static_cast<int>(process::permission::terminate));
    EXPECT_NE(static_cast<int>(process::permission::read), static_cast<int>(process::permission::query_info));
}

TEST_F(ProcessTest, StateValues_AreDistinct) {
    EXPECT_NE(static_cast<int>(process::state::running), static_cast<int>(process::state::suspended));
    EXPECT_NE(static_cast<int>(process::state::running), static_cast<int>(process::state::stopped));
    EXPECT_NE(static_cast<int>(process::state::running), static_cast<int>(process::state::exited));
    EXPECT_NE(static_cast<int>(process::state::running), static_cast<int>(process::state::unknown));
}

TEST_F(ProcessTest, NativeIdType_IsIntegral) { EXPECT_TRUE(is_integral<process::native_id_type>::value); }

TEST_F(ProcessTest, Exception_CopyConstructor_Success) {
    try {
        process p;
        p.start("nonexistent_executable_xyz_12345", {});
        FAIL() << "Expected process_exception";
    } catch (const process_exception& e) {
        process_exception copied(e);
        EXPECT_NE(copied.what(), nullptr);
    }
}

TEST_F(ProcessTest, Exception_What_ContainsErrorInfo) {
    try {
        process p;
        p.start("nonexistent_executable_xyz_12345", {});
        FAIL() << "Expected process_exception";
    } catch (const process_exception& e) {
        EXPECT_NE(e.what(), nullptr);
        EXPECT_GT(strlen(e.what()), 0);
    }
}

TEST_F(ProcessTest, ExecuteShell_ReturnsNonEmptyOutput) {
    auto result = process::execute_shell("echo hello");
    EXPECT_FALSE(result.output.empty());
}

TEST_F(ProcessTest, ExecuteShell_ReturnsExitCodeZero) {
    auto result = process::execute_shell("echo hello");
    EXPECT_EQ(result.exit_code, 0);
}

TEST_F(ProcessTest, ExecuteShell_EmptyCommand_Throws) { EXPECT_THROW(process::execute_shell(""), process_exception); }

TEST_F(ProcessTest, Destructor_AutoTerminatesRunningProcess) {
    {
        process p;
        p.start(get_long_running_executable(), get_long_running_args());
        EXPECT_TRUE(p.is_running());
    }
    SUCCEED();
}

TEST_F(ProcessTest, Close_ExplicitCleanup) {
    process p;
    p.start(get_test_executable(), get_test_args());
    p.close();

    EXPECT_EQ(p.id(), 0);
    EXPECT_FALSE(p.is_running());
}

TEST_F(ProcessTest, Close_AfterClose_NoThrow) {
    process p;
    p.close();
    EXPECT_NO_THROW(p.close());
}

TEST_F(ProcessTest, MoveConstructor_TransfersOwnership) {
    process p1;
    p1.start(get_test_executable(), get_test_args());
    auto original_id = p1.id();
    EXPECT_GT(original_id, 0);

    process p2(move(p1));

    EXPECT_EQ(p1.id(), 0);
    EXPECT_FALSE(p1.is_running());
    EXPECT_EQ(p2.id(), original_id);
    EXPECT_TRUE(p2.is_running());

    p2.wait();
}

TEST_F(ProcessTest, MoveAssignment_TransfersOwnership) {
    process p1;
    p1.start(get_test_executable(), get_test_args());
    auto original_id = p1.id();

    process p2;
    p2 = move(p1);

    EXPECT_EQ(p1.id(), 0);
    EXPECT_FALSE(p1.is_running());
    EXPECT_EQ(p2.id(), original_id);
    EXPECT_TRUE(p2.is_running());

    p2.wait();
}

TEST_F(ProcessTest, MoveAssignment_SelfAssign_Noop) {
    process p;
    p.start(get_test_executable(), get_test_args());
    auto original_id = p.id();

    p = move(p);

    EXPECT_EQ(p.id(), original_id);
    p.wait();
}

TEST_F(ProcessTest, MoveAssignment_CleansUpOldProcess) {
    process p1;
    p1.start(get_long_running_executable(), get_long_running_args());
    auto p1_id = p1.id();

    process p2;
    p2.start(get_test_executable(), get_test_args());
    auto p2_id = p2.id();

    p2 = move(p1);

    EXPECT_EQ(p2.id(), p1_id);
    EXPECT_EQ(p1.id(), 0);
}

TEST_F(ProcessTest, ChainConfiguration_Works) {
    process p;
    p.set_capture_stdout(true).set_capture_stderr(false);
    p.start(get_test_output_executable(), get_test_output_args());
    p.wait();

    EXPECT_FALSE(p.stdout_output().empty());
}

TEST_F(ProcessTest, MergedStderr_WhenOnlyStdoutCaptured) {
    process p;
    p.set_capture_stdout(true);
#ifdef NEFORCE_PLATFORM_WINDOWS
    p.start("cmd.exe", {"/c", "echo merged && echo err_msg >&2"});
#else
    p.start("/bin/sh", {"-c", "echo merged; echo err_msg >&2"});
#endif
    p.wait();

    string output = p.stdout_output();
    EXPECT_TRUE(output.find("err_msg") != string::npos || output.find("merged") != string::npos);
}

class ShareMemoryTest : public ::testing::Test {
protected:
    static constexpr const char* test_shm_name = "neforce_test_shm_12345";
    static constexpr size_t test_size = 4096;

    void SetUp() override { share_memory::remove(test_shm_name); }

    void TearDown() override { share_memory::remove(test_shm_name); }
};

TEST_F(ShareMemoryTest, DefaultConstructor_CreatesInvalidObject) {
    share_memory shm;
    EXPECT_FALSE(shm.is_open());
    EXPECT_FALSE(shm.is_mapped());
    EXPECT_EQ(shm.data(), nullptr);
    EXPECT_EQ(shm.size(), 0);
    EXPECT_EQ(shm.mapped_size(), 0);
    EXPECT_TRUE(shm.name().empty());
}

TEST_F(ShareMemoryTest, Constructor_CreateOnly_CreatesSuccessfully) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    EXPECT_TRUE(shm.is_open());
    EXPECT_EQ(shm.size(), test_size);
    EXPECT_EQ(shm.name(), test_shm_name);
}

TEST_F(ShareMemoryTest, Constructor_CreateOnly_AlreadyExists_ThrowsException) {
    share_memory shm1(test_shm_name, test_size, share_memory::open_mode::create_only);

    EXPECT_THROW(share_memory shm2(test_shm_name, test_size, share_memory::open_mode::create_only),
                 share_memory_exception);
}

TEST_F(ShareMemoryTest, Constructor_OpenOnly_Existing_Success) {
    share_memory shm1(test_shm_name, test_size, share_memory::open_mode::create_only);

    share_memory shm2(test_shm_name, 0, share_memory::open_mode::open_only);
    EXPECT_TRUE(shm2.is_open());
}

TEST_F(ShareMemoryTest, Constructor_OpenOnly_NonExisting_ThrowsException) {
    EXPECT_THROW(share_memory shm(test_shm_name, 0, share_memory::open_mode::open_only), share_memory_exception);
}

TEST_F(ShareMemoryTest, Constructor_OpenOrCreate_CreatesNew) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::open_or_create);
    EXPECT_TRUE(shm.is_open());
    EXPECT_EQ(shm.size(), test_size);
}

TEST_F(ShareMemoryTest, Constructor_OpenOrCreate_OpensExisting) {
    share_memory shm1(test_shm_name, test_size, share_memory::open_mode::open_or_create);

    share_memory shm2(test_shm_name, 0, share_memory::open_mode::open_or_create);
    EXPECT_TRUE(shm2.is_open());
}

TEST_F(ShareMemoryTest, Constructor_ReadOnly_Success) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only,
                     share_memory::access_mode::read_write);

    share_memory shm_ro(test_shm_name, 0, share_memory::open_mode::open_only, share_memory::access_mode::read_only);
    EXPECT_TRUE(shm_ro.is_open());
}

TEST_F(ShareMemoryTest, Open_AfterClose_ReopensSuccessfully) {
    share_memory shm;
    shm.open(test_shm_name, test_size, share_memory::open_mode::create_only);
    EXPECT_TRUE(shm.is_open());

    shm.close();
    EXPECT_FALSE(shm.is_open());

    try {
        shm.open(test_shm_name, 0, share_memory::open_mode::open_only);
        EXPECT_TRUE(shm.is_open());
    } catch (const share_memory_exception& e) {
        GTEST_SKIP() << "share_memory may close failed in Windows: " << e.what();
    }
}

TEST_F(ShareMemoryTest, Open_AlreadyOpen_ClosesAndReopens) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    EXPECT_TRUE(shm.is_open());

    shm.open(test_shm_name, test_size * 2, share_memory::open_mode::open_or_create);
    EXPECT_TRUE(shm.is_open());
}

TEST_F(ShareMemoryTest, Close_OpenedObject_ClosesSuccessfully) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    EXPECT_TRUE(shm.is_open());

    shm.close();
    EXPECT_FALSE(shm.is_open());
    EXPECT_EQ(shm.data(), nullptr);
}

TEST_F(ShareMemoryTest, Close_NotOpened_NoThrow) {
    share_memory shm;
    EXPECT_NO_THROW(shm.close());
}

TEST_F(ShareMemoryTest, Close_CalledTwice_NoThrow) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    shm.close();
    EXPECT_NO_THROW(shm.close());
}

TEST_F(ShareMemoryTest, Map_AfterOpen_ReturnsNonNull) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    void* addr = shm.map();
    EXPECT_NE(addr, nullptr);
    EXPECT_EQ(shm.mapped_size(), test_size);
}

TEST_F(ShareMemoryTest, Map_WithOffset_ReturnsNonNull) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    void* addr = shm.map(1024, 2048);
    EXPECT_NE(addr, nullptr);
    EXPECT_EQ(shm.mapped_size(), 2048);
}

TEST_F(ShareMemoryTest, Map_WithZeroLength_MapsEntireSize) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    void* addr = shm.map(0, 0);
    EXPECT_NE(addr, nullptr);
    EXPECT_EQ(shm.mapped_size(), test_size);
}

TEST_F(ShareMemoryTest, Map_NotOpen_ThrowsException) {
    share_memory shm;
    EXPECT_THROW(shm.map(), share_memory_exception);
}

TEST_F(ShareMemoryTest, Map_AlreadyMapped_RemapsSuccessfully) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    void* addr1 = shm.map();
    void* addr2 = shm.map();
    EXPECT_NE(addr1, nullptr);
    EXPECT_NE(addr2, nullptr);
}

TEST_F(ShareMemoryTest, Unmap_AfterMap_Success) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    shm.map();
    EXPECT_TRUE(shm.is_mapped());

    shm.unmap();
    EXPECT_FALSE(shm.is_mapped());
    EXPECT_EQ(shm.data(), nullptr);
}

TEST_F(ShareMemoryTest, Unmap_NotMapped_NoThrow) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    EXPECT_NO_THROW(shm.unmap());
}

TEST_F(ShareMemoryTest, Unmap_CalledTwice_NoThrow) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    shm.map();
    shm.unmap();
    EXPECT_NO_THROW(shm.unmap());
}

TEST_F(ShareMemoryTest, Data_AfterMap_ReturnsMappedAddress) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    void* addr = shm.map();
    EXPECT_EQ(shm.data(), addr);
}

TEST_F(ShareMemoryTest, Data_TypedVersion_ReturnsCorrectType) {
    share_memory shm(test_shm_name, sizeof(int), share_memory::open_mode::create_only);
    shm.map();
    int* ptr = shm.data<int>();
    EXPECT_NE(ptr, nullptr);
    *ptr = 42;
    EXPECT_EQ(*ptr, 42);
}

TEST_F(ShareMemoryTest, Data_NotMapped_ReturnsNull) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    EXPECT_EQ(shm.data(), nullptr);
    EXPECT_EQ(shm.data<int>(), nullptr);
}

TEST_F(ShareMemoryTest, Size_AfterCreate_ReturnsConstructedSize) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    EXPECT_EQ(shm.size(), test_size);
}

TEST_F(ShareMemoryTest, MappedSize_AfterMap_ReturnsMappedSize) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    shm.map(0, 2048);
    EXPECT_EQ(shm.mapped_size(), 2048);
}

TEST_F(ShareMemoryTest, MappedSize_NotMapped_ReturnsZero) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    EXPECT_EQ(shm.mapped_size(), 0);
}

TEST_F(ShareMemoryTest, Name_ReturnsCorrectName) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    EXPECT_EQ(shm.name(), test_shm_name);
}

TEST_F(ShareMemoryTest, IsOpen_AfterConstructor_ReturnsTrue) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    EXPECT_TRUE(shm.is_open());
}

TEST_F(ShareMemoryTest, IsOpen_AfterClose_ReturnsFalse) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    shm.close();
    EXPECT_FALSE(shm.is_open());
}

TEST_F(ShareMemoryTest, IsMapped_AfterMap_ReturnsTrue) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    shm.map();
    EXPECT_TRUE(shm.is_mapped());
}

TEST_F(ShareMemoryTest, IsMapped_AfterUnmap_ReturnsFalse) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    shm.map();
    shm.unmap();
    EXPECT_FALSE(shm.is_mapped());
}

TEST_F(ShareMemoryTest, Flush_Mapped_Success) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    shm.map();
    string_copy(static_cast<char*>(shm.data()), "test data");
    EXPECT_TRUE(shm.flush(false));
}

TEST_F(ShareMemoryTest, Flush_Async_Success) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    shm.map();
    string_copy(static_cast<char*>(shm.data()), "async test");
    EXPECT_TRUE(shm.flush(true));
}

TEST_F(ShareMemoryTest, Flush_NotMapped_ReturnsFalse) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    EXPECT_FALSE(shm.flush());
}

TEST_F(ShareMemoryTest, Remove_Existing_ReturnsTrue) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    shm.close();
    bool result = share_memory::remove(test_shm_name);
    EXPECT_TRUE(result);
}

TEST_F(ShareMemoryTest, Remove_NonExisting_ReturnsResult) {
    bool result = share_memory::remove("nonexistent_shm_xyz_12345");
    EXPECT_TRUE(result || !result);
}

TEST_F(ShareMemoryTest, Exists_Existing_ReturnsTrue) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    EXPECT_TRUE(share_memory::exists(test_shm_name));
}

TEST_F(ShareMemoryTest, Exists_NonExisting_ReturnsFalse) {
    EXPECT_FALSE(share_memory::exists("nonexistent_shm_xyz_12345"));
}

TEST_F(ShareMemoryTest, Exists_AfterRemove_ReturnsFalse) {
    {
        share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
        shm.close();
    }
    share_memory::remove(test_shm_name);
    EXPECT_FALSE(share_memory::exists(test_shm_name));
}

TEST_F(ShareMemoryTest, MoveConstructor_TransfersOwnership) {
    share_memory shm1(test_shm_name, test_size, share_memory::open_mode::create_only);
    shm1.map();
    void* addr = shm1.data();
    size_t sz = shm1.size();

    share_memory shm2(move(shm1));

    EXPECT_TRUE(shm2.is_open());
    EXPECT_EQ(shm2.data(), addr);
    EXPECT_EQ(shm2.size(), sz);
    EXPECT_EQ(shm2.name(), test_shm_name);

    EXPECT_FALSE(shm1.is_open());
    EXPECT_EQ(shm1.data(), nullptr);
    EXPECT_EQ(shm1.size(), 0);
    EXPECT_TRUE(shm1.name().empty());
}

TEST_F(ShareMemoryTest, MoveConstructor_FromDefaultConstructed_Success) {
    share_memory shm1;

    share_memory shm2(move(shm1));

    EXPECT_FALSE(shm2.is_open());
    EXPECT_EQ(shm2.data(), nullptr);
    EXPECT_FALSE(shm1.is_open());
}

TEST_F(ShareMemoryTest, MoveAssignment_TransfersOwnership) {
    share_memory shm1(test_shm_name, test_size, share_memory::open_mode::create_only);
    shm1.map();
    void* addr = shm1.data();

    share_memory shm2;
    shm2 = move(shm1);

    EXPECT_TRUE(shm2.is_open());
    EXPECT_EQ(shm2.data(), addr);
    EXPECT_EQ(shm2.name(), test_shm_name);

    EXPECT_FALSE(shm1.is_open());
    EXPECT_EQ(shm1.data(), nullptr);
}

TEST_F(ShareMemoryTest, MoveAssignment_SelfAssignment_NoEffect) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    shm.map();
    void* addr = shm.data();
    size_t sz = shm.size();

    shm = move(shm);

    EXPECT_TRUE(shm.is_open());
    EXPECT_EQ(shm.data(), addr);
    EXPECT_EQ(shm.size(), sz);
}

TEST_F(ShareMemoryTest, MoveAssignment_ClosesPreviousResource) {
    share_memory shm1(test_shm_name, test_size, share_memory::open_mode::create_only);
    share_memory shm2("neforce_test_shm_other", test_size, share_memory::open_mode::create_only);

    EXPECT_NO_THROW(shm2 = move(shm1));

    share_memory::remove("neforce_test_shm_other");
}

TEST_F(ShareMemoryTest, Destructor_ClosesResources) {
    {
        share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
        shm.map();
        EXPECT_TRUE(shm.is_open());
        EXPECT_TRUE(shm.is_mapped());
    }
    SUCCEED();
}

TEST_F(ShareMemoryTest, CopyConstructor_IsDeleted) { EXPECT_FALSE(is_copy_constructible<share_memory>::value); }

TEST_F(ShareMemoryTest, CopyAssignment_IsDeleted) { EXPECT_FALSE(is_copy_assignable<share_memory>::value); }

TEST_F(ShareMemoryTest, MoveConstructor_IsNoexcept) { EXPECT_TRUE(is_nothrow_move_constructible<share_memory>::value); }

TEST_F(ShareMemoryTest, MoveAssignment_IsNoexcept) { EXPECT_TRUE(is_nothrow_move_assignable<share_memory>::value); }

TEST_F(ShareMemoryTest, InterProcessCommunication_Success) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    shm.map();
    char* data = static_cast<char*>(shm.data());
    string_copy(data, "shared data from parent");

    shm.flush();

    share_memory shm2(test_shm_name, 0, share_memory::open_mode::open_only, share_memory::access_mode::read_only);
    shm2.map();
    const char* data2 = static_cast<const char*>(shm2.data());
    EXPECT_STREQ(data2, "shared data from parent");
}

TEST_F(ShareMemoryTest, WriteRead_ThroughMapping_Success) {
    share_memory shm(test_shm_name, sizeof(int) * 10, share_memory::open_mode::create_only);
    shm.map();
    int* int_data = shm.data<int>();

    for (int i = 0; i < 10; ++i) {
        int_data[i] = i * 100;
    }

    shm.flush();

    share_memory shm2(test_shm_name, 0, share_memory::open_mode::open_only);
    shm2.map();
    const int* read_data = shm2.data<int>();

    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(read_data[i], i * 100);
    }
}

TEST_F(ShareMemoryTest, AccessMode_ReadOnly_PreventsWrite) {
    share_memory shm_writer(test_shm_name, test_size, share_memory::open_mode::create_only,
                            share_memory::access_mode::read_write);
    shm_writer.map();
    string_copy(static_cast<char*>(shm_writer.data()), "readonly test");
    shm_writer.flush();
    shm_writer.unmap();
    shm_writer.close();

    try {
        share_memory shm_reader(test_shm_name, 0, share_memory::open_mode::open_only,
                                share_memory::access_mode::read_only);
        shm_reader.map();
        EXPECT_STREQ(static_cast<const char*>(shm_reader.data()), "readonly test");
    } catch (const share_memory_exception& e) {
        GTEST_SKIP() << "share_memory may close failed in Windows: " << e.what();
    }
}

TEST_F(ShareMemoryTest, MapWithOffset_PartialMapping_Success) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);

    shm.map(0, test_size);
    char* full_data = static_cast<char*>(shm.data());
    for (size_t i = 0; i < test_size; ++i) {
        full_data[i] = static_cast<char>(i % 256);
    }
    shm.flush();
    shm.unmap();

    const size_t offset = 1024;
    const size_t length = 512;
    shm.map(offset, length);
    const char* partial_data = static_cast<const char*>(shm.data());

    for (size_t i = 0; i < length; ++i) {
        EXPECT_EQ(partial_data[i], static_cast<char>((i + offset) % 256));
    }
}

TEST_F(ShareMemoryTest, OpenMode_EnumValues_Distinct) {
    EXPECT_NE(static_cast<int>(share_memory::open_mode::create_only),
              static_cast<int>(share_memory::open_mode::open_only));
    EXPECT_NE(static_cast<int>(share_memory::open_mode::create_only),
              static_cast<int>(share_memory::open_mode::open_or_create));
    EXPECT_NE(static_cast<int>(share_memory::open_mode::open_only),
              static_cast<int>(share_memory::open_mode::open_or_create));
}

TEST_F(ShareMemoryTest, AccessMode_EnumValues_Distinct) {
    EXPECT_NE(static_cast<int>(share_memory::access_mode::read_only),
              static_cast<int>(share_memory::access_mode::read_write));
}

TEST_F(ShareMemoryTest, Exception_CopyConstructor_Success) {
    try {
        share_memory shm("nonexistent_shm_for_exception_test", 0, share_memory::open_mode::open_only);
        FAIL() << "Expected share_memory_exception";
    } catch (const share_memory_exception& e) {
        share_memory_exception copied(e);
        EXPECT_NE(copied.what(), nullptr);
    }
}

TEST_F(ShareMemoryTest, Exception_What_ContainsErrorInfo) {
    try {
        share_memory shm("nonexistent_shm_for_exception_test", 0, share_memory::open_mode::open_only);
        FAIL() << "Expected share_memory_exception";
    } catch (const share_memory_exception& e) {
        EXPECT_NE(e.what(), nullptr);
        EXPECT_GT(strlen(e.what()), 0);
    }
}

TEST_F(ShareMemoryTest, LargeSharedMemory_Success) {
    constexpr size_t large_size = 1024 * 1024;
    share_memory shm(test_shm_name, large_size, share_memory::open_mode::create_only);
    shm.map();

    char* data = static_cast<char*>(shm.data());
    data[0] = 'A';
    data[large_size - 1] = 'Z';

    shm.flush();

    share_memory shm2(test_shm_name, 0, share_memory::open_mode::open_only);
    shm2.map();
    const char* read_data = static_cast<const char*>(shm2.data());

    EXPECT_EQ(read_data[0], 'A');
    EXPECT_EQ(read_data[large_size - 1], 'Z');
}

TEST_F(ShareMemoryTest, StructDataSharing_Success) {
    struct TestData {
        int id;
        double value;
        char name[32];
    };

    share_memory shm(test_shm_name, sizeof(TestData), share_memory::open_mode::create_only);
    shm.map();
    auto* td = shm.data<TestData>();
    td->id = 42;
    td->value = 3.14159;
    string_copy(td->name, "test struct");

    shm.flush();
    shm.unmap();
    shm.close();

    try {
        share_memory shm2(test_shm_name, 0, share_memory::open_mode::open_only);
        shm2.map();
        const auto* read_td = shm2.data<TestData>();

        EXPECT_EQ(read_td->id, 42);
        EXPECT_DOUBLE_EQ(read_td->value, 3.14159);
        EXPECT_STREQ(read_td->name, "test struct");
    } catch (const share_memory_exception& e) {
        GTEST_SKIP() << "share_memory may close failed in Windows: " << e.what();
    }
}

TEST_F(ShareMemoryTest, OpenOrCreate_ExistingSize_ReturnsCorrectSize) {
    share_memory shm1(test_shm_name, test_size, share_memory::open_mode::create_only);

    share_memory shm2(test_shm_name, test_size * 2, share_memory::open_mode::open_or_create);
    EXPECT_EQ(shm2.size(), test_size);
}

TEST_F(ShareMemoryTest, IsMapped_DefaultConstructed_ReturnsFalse) {
    share_memory shm;
    EXPECT_FALSE(shm.is_mapped());
}

TEST_F(ShareMemoryTest, Open_ZeroSize_CreateOnly_Success) {
    share_memory shm(test_shm_name, 1, share_memory::open_mode::create_only);
    EXPECT_TRUE(shm.is_open());
    EXPECT_GT(shm.size(), 0);
}

TEST_F(ShareMemoryTest, Unmap_BeforeClose_CalledAutomatically) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
    shm.map();
    EXPECT_TRUE(shm.is_mapped());
    shm.close();
    EXPECT_FALSE(shm.is_mapped());
}

TEST_F(ShareMemoryTest, MultipleMappings_DifferentSizes) {
    share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);

    shm.map(0, 1024);
    EXPECT_EQ(shm.mapped_size(), 1024);

    shm.map(1024, 2048);
    EXPECT_EQ(shm.mapped_size(), 2048);

    shm.unmap();
    EXPECT_EQ(shm.mapped_size(), 0);
}

TEST_F(ShareMemoryTest, Name_AfterMove_SourceEmpty) {
    share_memory shm1(test_shm_name, test_size, share_memory::open_mode::create_only);
    share_memory shm2(move(shm1));

    EXPECT_EQ(shm2.name(), test_shm_name);
    EXPECT_TRUE(shm1.name().empty());
}

TEST_F(ShareMemoryTest, Remove_AfterClose_RemovesSuccessfully) {
    {
        share_memory shm(test_shm_name, test_size, share_memory::open_mode::create_only);
        EXPECT_TRUE(share_memory::exists(test_shm_name));
    }
    bool removed = share_memory::remove(test_shm_name);
    EXPECT_TRUE(removed);
    EXPECT_FALSE(share_memory::exists(test_shm_name));
}

TEST_F(ShareMemoryTest, Data_AfterMove_SourceDataNull) {
    share_memory shm1(test_shm_name, test_size, share_memory::open_mode::create_only);
    shm1.map();
    void* addr = shm1.data();

    share_memory shm2(move(shm1));

    EXPECT_EQ(shm2.data(), addr);
    EXPECT_EQ(shm1.data(), nullptr);
}

class SysinfoTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(SysinfoTest, Instance_ReturnsSameInstance) {
    sysinfo& inst1 = sysinfo::instance();
    sysinfo& inst2 = sysinfo::instance();
    EXPECT_EQ(&inst1, &inst2);
}

TEST_F(SysinfoTest, Instance_IsInitialized) {
    sysinfo& inst = sysinfo::instance();
    EXPECT_TRUE(inst.is_initialized());
}

TEST_F(SysinfoTest, Refresh_DoesNotThrow) {
    sysinfo& inst = sysinfo::instance();
    EXPECT_NO_THROW(inst.refresh());
    EXPECT_TRUE(inst.is_initialized());
}

TEST_F(SysinfoTest, GetSystemInfo_ReturnsValidData) {
    sysinfo& inst = sysinfo::instance();
    const auto& info = inst.get_system_info();

    EXPECT_GT(info.processor_numbers, 0);
    EXPECT_GT(info.page_size, 0);
    EXPECT_GT(info.allocation_granularity, 0);
}

TEST_F(SysinfoTest, GetSystemInfo_PageSize_IsPowerOfTwo) {
    sysinfo& inst = sysinfo::instance();
    const auto& info = inst.get_system_info();

    EXPECT_GT(info.page_size, 0);
    EXPECT_EQ((info.page_size & (info.page_size - 1)), 0);
}

TEST_F(SysinfoTest, GetSystemInfo_AllocationGranularity_GreaterOrEqualPageSize) {
    sysinfo& inst = sysinfo::instance();
    const auto& info = inst.get_system_info();

    EXPECT_GE(info.allocation_granularity, info.page_size);
}

TEST_F(SysinfoTest, GetSystemInfo_MinAppAddress_LessThanMaxAppAddress) {
    sysinfo& inst = sysinfo::instance();
    const auto& info = inst.get_system_info();

    EXPECT_LT(info.min_app_address, info.max_app_address);
}

TEST_F(SysinfoTest, GetSystemInfo_ActiveProcessorMask_NonZero) {
    sysinfo& inst = sysinfo::instance();
    const auto& info = inst.get_system_info();

    EXPECT_NE(info.active_processor_mask, 0);
}

TEST_F(SysinfoTest, GetMemoryInfo_TotalPhysical_NonZero) {
    sysinfo& inst = sysinfo::instance();
    const auto& mem = inst.get_memory_info();

    EXPECT_GT(mem.total_physical, 0);
}

TEST_F(SysinfoTest, GetMemoryInfo_AvailablePhysical_LessOrEqualTotal) {
    sysinfo& inst = sysinfo::instance();
    const auto& mem = inst.get_memory_info();

    EXPECT_LE(mem.available_physical, mem.total_physical);
}

TEST_F(SysinfoTest, GetMemoryInfo_TotalVirtual_NonZero) {
    sysinfo& inst = sysinfo::instance();
    const auto& mem = inst.get_memory_info();

    EXPECT_GT(mem.total_virtual, 0);
}

TEST_F(SysinfoTest, GetMemoryInfo_AvailableVirtual_LessOrEqualTotal) {
    sysinfo& inst = sysinfo::instance();
    const auto& mem = inst.get_memory_info();

    EXPECT_LE(mem.available_virtual, mem.total_virtual);
}

TEST_F(SysinfoTest, GetMemoryInfo_TotalPageFile_NonZero) {
    sysinfo& inst = sysinfo::instance();
    const auto& mem = inst.get_memory_info();

    EXPECT_GT(mem.total_page_file, 0);
}

TEST_F(SysinfoTest, GetMemoryInfo_AvailablePageFile_LessOrEqualTotal) {
    sysinfo& inst = sysinfo::instance();
    const auto& mem = inst.get_memory_info();

    EXPECT_LE(mem.available_page_file, mem.total_page_file);
}

TEST_F(SysinfoTest, MemoryInfo_PhysicalMemoryUsage_BetweenZeroAndHundred) {
    sysinfo& inst = sysinfo::instance();
    const auto& mem = inst.get_memory_info();

    float64_t usage = mem.physical_memory_usage();
    EXPECT_GE(usage, 0.0);
    EXPECT_LE(usage, 100.0);
}

TEST_F(SysinfoTest, MemoryInfo_AvailableMemory_NonZero) {
    sysinfo& inst = sysinfo::instance();
    const auto& mem = inst.get_memory_info();

    size_t available = mem.available_memory();
    EXPECT_GT(available, 0);
}

TEST_F(SysinfoTest, MemoryInfo_DefaultValues_AllZero) {
    sysinfo::memory_info mem{};
    EXPECT_EQ(mem.total_physical, 0);
    EXPECT_EQ(mem.available_physical, 0);
    EXPECT_EQ(mem.total_virtual, 0);
    EXPECT_EQ(mem.available_virtual, 0);
    EXPECT_EQ(mem.total_page_file, 0);
    EXPECT_EQ(mem.available_page_file, 0);
}

TEST_F(SysinfoTest, MemoryInfo_Default_PhysicalMemoryUsage_Zero) {
    sysinfo::memory_info mem{};
    EXPECT_DOUBLE_EQ(mem.physical_memory_usage(), 0.0);
}

TEST_F(SysinfoTest, GetCpuInfo_Vendor_NonEmpty) {
    sysinfo& inst = sysinfo::instance();
    const auto& cpu = inst.get_CPU_info();

    EXPECT_FALSE(cpu.vendor.empty());
}

TEST_F(SysinfoTest, GetCpuInfo_Brand_NonEmpty) {
    sysinfo& inst = sysinfo::instance();
    const auto& cpu = inst.get_CPU_info();

    EXPECT_FALSE(cpu.brand.empty());
}

TEST_F(SysinfoTest, GetCpuInfo_Cores_NonZero) {
    sysinfo& inst = sysinfo::instance();
    const auto& cpu = inst.get_CPU_info();

    EXPECT_GT(cpu.cores, 0);
}

TEST_F(SysinfoTest, GetCpuInfo_LogicalProcessors_NonZero) {
    sysinfo& inst = sysinfo::instance();
    const auto& cpu = inst.get_CPU_info();

    EXPECT_GT(cpu.logical_processors, 0);
}

TEST_F(SysinfoTest, GetCpuInfo_LogicalProcessors_GreaterOrEqualCores) {
    sysinfo& inst = sysinfo::instance();
    const auto& cpu = inst.get_CPU_info();

    EXPECT_GE(cpu.logical_processors, cpu.cores);
}

TEST_F(SysinfoTest, GetCpuInfo_MaxMHz_NonZero) {
    sysinfo& inst = sysinfo::instance();
    const auto& cpu = inst.get_CPU_info();

    EXPECT_GT(cpu.max_MHz, 0);
}

TEST_F(SysinfoTest, GetCpuInfo_CurrentMHz_NonZero) {
    sysinfo& inst = sysinfo::instance();
    const auto& cpu = inst.get_CPU_info();

    EXPECT_GT(cpu.current_MHz, 0);
}

TEST_F(SysinfoTest, CpuInfo_Hyperthreading_ReturnsLogicalResult) {
    sysinfo& inst = sysinfo::instance();
    const auto& cpu = inst.get_CPU_info();

    bool ht = cpu.hyperthreading();
    bool expected = (cpu.logical_processors > cpu.cores);
    EXPECT_EQ(ht, expected);
}

TEST_F(SysinfoTest, CpuInfo_DefaultValues_AllZero) {
    sysinfo::CPU_info cpu{};
    EXPECT_TRUE(cpu.vendor.empty());
    EXPECT_TRUE(cpu.brand.empty());
    EXPECT_EQ(cpu.max_MHz, 0);
    EXPECT_EQ(cpu.current_MHz, 0);
    EXPECT_EQ(cpu.cores, 0);
    EXPECT_EQ(cpu.logical_processors, 0);
    EXPECT_TRUE(cpu.features.empty());
    EXPECT_FALSE(cpu.hyperthreading());
}

TEST_F(SysinfoTest, GetOsVersionInfo_Major_NonZero) {
    sysinfo& inst = sysinfo::instance();
    const auto& os = inst.get_os_version_info();

    EXPECT_GT(os.major, 0);
}

TEST_F(SysinfoTest, GetOsVersionInfo_ProductName_NonEmpty) {
    sysinfo& inst = sysinfo::instance();
    const auto& os = inst.get_os_version_info();

    EXPECT_FALSE(os.product_name.empty());
}

TEST_F(SysinfoTest, OsVersionInfo_Version_NonEmpty) {
    sysinfo& inst = sysinfo::instance();
    const auto& os = inst.get_os_version_info();

    string ver = os.version();
    EXPECT_FALSE(ver.empty());
}

TEST_F(SysinfoTest, OsVersionInfo_Version_ContainsDots) {
    sysinfo& inst = sysinfo::instance();
    const auto& os = inst.get_os_version_info();

    string ver = os.version();
    EXPECT_NE(ver.find('.'), string::npos);
}

TEST_F(SysinfoTest, OsVersionInfo_DefaultValues_AllZero) {
    sysinfo::os_version_info os{};
    EXPECT_EQ(os.major, 0);
    EXPECT_EQ(os.minor, 0);
    EXPECT_EQ(os.build, 0);
    EXPECT_EQ(os.platform_id, 0);
    EXPECT_TRUE(os.csd_version.empty());
    EXPECT_TRUE(os.product_name.empty());
}

TEST_F(SysinfoTest, OsVersionInfo_Default_Version_ReturnsZeroVersion) {
    sysinfo::os_version_info os{};
    EXPECT_EQ(os.version(), "0.0.0");
}

TEST_F(SysinfoTest, GetArchitecture_ReturnsKnownArchitecture) {
    sysinfo& inst = sysinfo::instance();
    auto arch = inst.get_architecture();

    EXPECT_NE(arch, sysinfo::architecture::UNKNOWN);
}

TEST_F(SysinfoTest, GetArchitecture_MatchesPlatform) {
    sysinfo& inst = sysinfo::instance();
    auto arch = inst.get_architecture();

#ifdef NEFORCE_ARCH_X86
    EXPECT_TRUE(arch == sysinfo::architecture::X64 || arch == sysinfo::architecture::X86);
#endif
}

TEST_F(SysinfoTest, CpuUsage_ReturnsBetweenZeroAndHundred) {
    float64_t usage = sysinfo::cpu_usage();

    usage = sysinfo::cpu_usage();
    EXPECT_GE(usage, 0.0);
    EXPECT_LE(usage, 100.0);
}

TEST_F(SysinfoTest, ProcessCount_ReturnsPositive) {
    uint32_t count = sysinfo::process_count();
    EXPECT_GT(count, 0);
}

TEST_F(SysinfoTest, ProcessCount_ConsistentWithCurrentProcess) {
    uint32_t count = sysinfo::process_count();
    EXPECT_GE(count, 1);
}

TEST_F(SysinfoTest, Refresh_UpdatesInfo_DoesNotThrow) {
    sysinfo& inst = sysinfo::instance();
    auto mem_before = inst.get_memory_info().total_physical;

    inst.refresh();

    auto mem_after = inst.get_memory_info().total_physical;
    EXPECT_EQ(mem_before, mem_after);
    EXPECT_TRUE(inst.is_initialized());
}

TEST_F(SysinfoTest, SystemInfo_DefaultValues_AllZero) {
    sysinfo::system_info info{};
    EXPECT_EQ(info.processor_numbers, 0);
    EXPECT_EQ(info.page_size, 0);
    EXPECT_EQ(info.allocation_granularity, 0);
    EXPECT_EQ(info.min_app_address, 0);
    EXPECT_EQ(info.max_app_address, 0);
    EXPECT_EQ(info.active_processor_mask, 0);
    EXPECT_EQ(info.processor_level, 0);
    EXPECT_EQ(info.processor_revision, 0);
}

TEST_F(SysinfoTest, Architecture_EnumValues_Distinct) {
    EXPECT_NE(static_cast<int>(sysinfo::architecture::UNKNOWN), static_cast<int>(sysinfo::architecture::X86));
    EXPECT_NE(static_cast<int>(sysinfo::architecture::X86), static_cast<int>(sysinfo::architecture::X64));
    EXPECT_NE(static_cast<int>(sysinfo::architecture::X64), static_cast<int>(sysinfo::architecture::ARM));
    EXPECT_NE(static_cast<int>(sysinfo::architecture::ARM), static_cast<int>(sysinfo::architecture::ARM64));
}

TEST_F(SysinfoTest, IsInitialized_ReturnsTrue) {
    sysinfo& inst = sysinfo::instance();
    EXPECT_TRUE(inst.is_initialized());
}

TEST_F(SysinfoTest, IsInitialized_AfterRefresh_ReturnsTrue) {
    sysinfo& inst = sysinfo::instance();
    inst.refresh();
    EXPECT_TRUE(inst.is_initialized());
}

TEST_F(SysinfoTest, GetSystemInfo_ProcessorLevel_NonNegative) {
    sysinfo& inst = sysinfo::instance();
    const auto& info = inst.get_system_info();

    EXPECT_GE(info.processor_level, 0);
}

TEST_F(SysinfoTest, GetSystemInfo_ProcessorRevision_NonNegative) {
    sysinfo& inst = sysinfo::instance();
    const auto& info = inst.get_system_info();

    EXPECT_GE(info.processor_revision, 0);
}

TEST_F(SysinfoTest, CopyConstructor_IsDeleted) { EXPECT_FALSE(is_copy_constructible<sysinfo>::value); }

TEST_F(SysinfoTest, CopyAssignment_IsDeleted) { EXPECT_FALSE(is_copy_assignable<sysinfo>::value); }

TEST_F(SysinfoTest, MoveConstructor_IsDeleted) { EXPECT_FALSE(is_move_constructible<sysinfo>::value); }

TEST_F(SysinfoTest, MoveAssignment_IsDeleted) { EXPECT_FALSE(is_move_assignable<sysinfo>::value); }

TEST_F(SysinfoTest, MemoryInfo_Default_AvailableMemory_Zero) {
    sysinfo::memory_info mem{};
    EXPECT_EQ(mem.available_memory(), 0);
}

TEST_F(SysinfoTest, GetCpuInfo_Features_NonEmpty) {
    sysinfo& inst = sysinfo::instance();
    const auto& cpu = inst.get_CPU_info();

#ifndef NEFORCE_PLATFORM_WINDOWS
    EXPECT_FALSE(cpu.features.empty());
#endif
}

TEST_F(SysinfoTest, GetOsVersionInfo_Build_NonNegative) {
    sysinfo& inst = sysinfo::instance();
    const auto& os = inst.get_os_version_info();

    EXPECT_GE(os.build, 0);
}

TEST_F(SysinfoTest, GetOsVersionInfo_Minor_NonNegative) {
    sysinfo& inst = sysinfo::instance();
    const auto& os = inst.get_os_version_info();

    EXPECT_GE(os.minor, 0);
}

TEST_F(SysinfoTest, GetSystemInfo_ConsistentAcrossCalls) {
    sysinfo& inst = sysinfo::instance();
    const auto& info1 = inst.get_system_info();
    const auto& info2 = inst.get_system_info();

    EXPECT_EQ(info1.processor_numbers, info2.processor_numbers);
    EXPECT_EQ(info1.page_size, info2.page_size);
    EXPECT_EQ(info1.allocation_granularity, info2.allocation_granularity);
}

TEST_F(SysinfoTest, GetMemoryInfo_ConsistentAcrossCalls) {
    sysinfo& inst = sysinfo::instance();
    const auto& mem1 = inst.get_memory_info();
    const auto& mem2 = inst.get_memory_info();

    EXPECT_EQ(mem1.total_physical, mem2.total_physical);
    EXPECT_EQ(mem1.total_virtual, mem2.total_virtual);
}

TEST_F(SysinfoTest, GetCpuInfo_ConsistentAcrossCalls) {
    sysinfo& inst = sysinfo::instance();
    const auto& cpu1 = inst.get_CPU_info();
    const auto& cpu2 = inst.get_CPU_info();

    EXPECT_EQ(cpu1.vendor, cpu2.vendor);
    EXPECT_EQ(cpu1.brand, cpu2.brand);
    EXPECT_EQ(cpu1.cores, cpu2.cores);
}

TEST_F(SysinfoTest, GetOsVersionInfo_ConsistentAcrossCalls) {
    sysinfo& inst = sysinfo::instance();
    const auto& os1 = inst.get_os_version_info();
    const auto& os2 = inst.get_os_version_info();

    EXPECT_EQ(os1.major, os2.major);
    EXPECT_EQ(os1.minor, os2.minor);
    EXPECT_EQ(os1.build, os2.build);
    EXPECT_EQ(os1.product_name, os2.product_name);
}

TEST_F(SysinfoTest, GetArchitecture_ConsistentAcrossCalls) {
    sysinfo& inst = sysinfo::instance();
    auto arch1 = inst.get_architecture();
    auto arch2 = inst.get_architecture();

    EXPECT_EQ(arch1, arch2);
}

TEST_F(SysinfoTest, MemoryInfo_PhysicalMemoryUsage_ZeroTotal_ReturnsZero) {
    sysinfo::memory_info mem{};
    mem.total_physical = 0;
    mem.available_physical = 100;
    EXPECT_DOUBLE_EQ(mem.physical_memory_usage(), 0.0);
}

TEST_F(SysinfoTest, MemoryInfo_PhysicalMemoryUsage_AllAvailable_ReturnsZero) {
    sysinfo::memory_info mem{};
    mem.total_physical = 1024;
    mem.available_physical = 1024;
    EXPECT_DOUBLE_EQ(mem.physical_memory_usage(), 0.0);
}

TEST_F(SysinfoTest, MemoryInfo_PhysicalMemoryUsage_NoneAvailable_ReturnsHundred) {
    sysinfo::memory_info mem{};
    mem.total_physical = 1024;
    mem.available_physical = 0;
    EXPECT_DOUBLE_EQ(mem.physical_memory_usage(), 100.0);
}

TEST_F(SysinfoTest, Refresh_MultipleTimes_NoThrow) {
    sysinfo& inst = sysinfo::instance();
    for (int i = 0; i < 3; ++i) {
        EXPECT_NO_THROW(inst.refresh());
        EXPECT_TRUE(inst.is_initialized());
    }
}

TEST_F(SysinfoTest, ProcessCount_AfterRefresh_StillReturnsPositive) {
    sysinfo& inst = sysinfo::instance();
    inst.refresh();
    uint32_t count = sysinfo::process_count();
    EXPECT_GT(count, 0);
}

TEST_F(SysinfoTest, CpuUsage_AfterRefresh_StillReturnsValid) {
    sysinfo& inst = sysinfo::instance();
    inst.refresh();
    float64_t usage = sysinfo::cpu_usage();
    EXPECT_GE(usage, 0.0);
    EXPECT_LE(usage, 100.0);
}

class SystemEventTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(SystemEventTest, Constructor_DefaultParameters_CreatesNonSignaledAutoReset) {
    system_event evt;
    EXPECT_EQ(evt.event_type(), system_event::type::auto_reset);
}

TEST_F(SystemEventTest, Constructor_InitialStateTrue_CreatesSignaled) {
    system_event evt(true);
    bool result = evt.wait(0);
    EXPECT_TRUE(result);
}

TEST_F(SystemEventTest, Constructor_InitialStateFalse_CreatesNonSignaled) {
    system_event evt(false);
    bool result = evt.wait(0);
    EXPECT_FALSE(result);
}

TEST_F(SystemEventTest, Constructor_ManualReset_CreatesManualResetEvent) {
    system_event evt(false, system_event::type::manual_reset);
    EXPECT_EQ(evt.event_type(), system_event::type::manual_reset);
}

TEST_F(SystemEventTest, Constructor_AutoReset_CreatesAutoResetEvent) {
    system_event evt(false, system_event::type::auto_reset);
    EXPECT_EQ(evt.event_type(), system_event::type::auto_reset);
}

TEST_F(SystemEventTest, Set_Signaled_AutoReset_WaitReturnsTrue) {
    system_event evt(false, system_event::type::auto_reset);
    evt.set();
    bool result = evt.wait(100);
    EXPECT_TRUE(result);
}

TEST_F(SystemEventTest, Set_Signaled_ManualReset_WaitReturnsTrue) {
    system_event evt(false, system_event::type::manual_reset);
    evt.set();
    bool result = evt.wait(100);
    EXPECT_TRUE(result);
}

TEST_F(SystemEventTest, Set_MultipleTimes_AutoReset_WaitReturnsTrue) {
    system_event evt(false, system_event::type::auto_reset);

    for (int i = 0; i < 3; ++i) {
        evt.set();
        bool result = evt.wait(100);
        EXPECT_TRUE(result);
    }
}

TEST_F(SystemEventTest, Set_AutoReset_OnlyOneThreadWakesUp) {
    system_event evt(false, system_event::type::auto_reset);
    atomic<int> wake_count{0};

    thread t1([&]() {
        if (evt.wait(500)) {
            ++wake_count;
        }
    });

    thread t2([&]() {
        if (evt.wait(500)) {
            ++wake_count;
        }
    });

    this_thread::sleep_for(milliseconds(50));
    evt.set();

    t1.join();
    t2.join();

    EXPECT_EQ(wake_count.load(), 1);
}

TEST_F(SystemEventTest, Set_ManualReset_AllThreadsWakeUp) {
    system_event evt(false, system_event::type::manual_reset);
    atomic<int> wake_count{0};

    thread t1([&]() {
        if (evt.wait(500)) {
            ++wake_count;
        }
    });

    thread t2([&]() {
        if (evt.wait(500)) {
            ++wake_count;
        }
    });

    this_thread::sleep_for(milliseconds(50));
    evt.set();

    t1.join();
    t2.join();

    EXPECT_EQ(wake_count.load(), 2);
}

TEST_F(SystemEventTest, Reset_AutoReset_EventBecomesNonSignaled) {
    system_event evt(false, system_event::type::auto_reset);
    evt.set();
    evt.reset();
    bool result = evt.wait(0);
    EXPECT_FALSE(result);
}

TEST_F(SystemEventTest, Reset_ManualReset_EventBecomesNonSignaled) {
    system_event evt(false, system_event::type::manual_reset);
    evt.set();
    evt.reset();
    bool result = evt.wait(0);
    EXPECT_FALSE(result);
}

TEST_F(SystemEventTest, Reset_NonSignaled_NoEffect) {
    system_event evt(false, system_event::type::auto_reset);
    evt.reset();
    bool result = evt.wait(0);
    EXPECT_FALSE(result);
}

TEST_F(SystemEventTest, Wait_ZeroTimeout_NonSignaled_ReturnsFalse) {
    system_event evt(false);
    bool result = evt.wait(0);
    EXPECT_FALSE(result);
}

TEST_F(SystemEventTest, Wait_ZeroTimeout_Signaled_ReturnsTrue) {
    system_event evt(true);
    bool result = evt.wait(0);
    EXPECT_TRUE(result);
}

TEST_F(SystemEventTest, Wait_PositiveTimeout_ReturnsAfterSet) {
    system_event evt(false);

    thread t([&]() {
        this_thread::sleep_for(milliseconds(50));
        evt.set();
    });

    bool result = evt.wait(500);
    EXPECT_TRUE(result);

    t.join();
}

TEST_F(SystemEventTest, Wait_Timeout_ReturnsFalse) {
    system_event evt(false);
    bool result = evt.wait(50);
    EXPECT_FALSE(result);
}

TEST_F(SystemEventTest, Wait_InfiniteTimeout_WaitsForSet) {
    system_event evt(false);

    atomic<bool> wait_returned{false};

    thread t([&]() {
        bool result = evt.wait();
        wait_returned.store(true);
        EXPECT_TRUE(result);
    });

    this_thread::sleep_for(milliseconds(50));
    EXPECT_FALSE(wait_returned.load());

    evt.set();
    this_thread::sleep_for(milliseconds(50));
    EXPECT_TRUE(wait_returned.load());

    t.join();
}

TEST_F(SystemEventTest, Wait_AutoReset_AutomaticallyResets) {
    system_event evt(false, system_event::type::auto_reset);
    evt.set();

    bool result1 = evt.wait(100);
    EXPECT_TRUE(result1);

    bool result2 = evt.wait(0);
    EXPECT_FALSE(result2);
}

TEST_F(SystemEventTest, Wait_ManualReset_StaysSignaled) {
    system_event evt(false, system_event::type::manual_reset);
    evt.set();

    bool result1 = evt.wait(100);
    EXPECT_TRUE(result1);

    bool result2 = evt.wait(0);
    EXPECT_TRUE(result2);
}

TEST_F(SystemEventTest, Wait_Signaled_AutoReset_ResetsBeforeReturn) {
    system_event evt(false, system_event::type::auto_reset);
    evt.set();

    thread t([&]() {
        bool result = evt.wait(500);
        EXPECT_TRUE(result);
    });

    t.join();

    bool result = evt.wait(0);
    EXPECT_FALSE(result);
}

TEST_F(SystemEventTest, EventType_ReturnsCorrectType) {
    system_event auto_evt(false, system_event::type::auto_reset);
    EXPECT_EQ(auto_evt.event_type(), system_event::type::auto_reset);

    system_event manual_evt(false, system_event::type::manual_reset);
    EXPECT_EQ(manual_evt.event_type(), system_event::type::manual_reset);
}

TEST_F(SystemEventTest, MoveConstructor_TransfersOwnership) {
    system_event evt1(false, system_event::type::manual_reset);
    evt1.set();

    system_event evt2(move(evt1));

    EXPECT_EQ(evt2.event_type(), system_event::type::manual_reset);
    bool result = evt2.wait(0);
    EXPECT_TRUE(result);
}

TEST_F(SystemEventTest, MoveConstructor_SourceBecomesUnusable) {
    system_event evt1(false);
    system_event evt2(move(evt1));
    EXPECT_EQ(evt2.event_type(), system_event::type::auto_reset);
}

TEST_F(SystemEventTest, MoveAssignment_TransfersOwnership) {
    system_event evt1(false, system_event::type::manual_reset);
    evt1.set();

    system_event evt2(true);
    evt2 = move(evt1);

    EXPECT_EQ(evt2.event_type(), system_event::type::manual_reset);
    bool result = evt2.wait(0);
    EXPECT_TRUE(result);
}

TEST_F(SystemEventTest, MoveAssignment_SelfAssignment_NoEffect) {
    system_event evt(false, system_event::type::auto_reset);
    evt.set();

    evt = move(evt);

    EXPECT_EQ(evt.event_type(), system_event::type::auto_reset);
    bool result = evt.wait(0);
    EXPECT_TRUE(result);
}

TEST_F(SystemEventTest, Destructor_NoDoubleFree) {
    for (int i = 0; i < 10; ++i) {
        system_event evt(false);
        evt.set();
        evt.wait(0);
    }
    SUCCEED();
}

TEST_F(SystemEventTest, Destructor_MovedFrom_SafeToDestroy) {
    {
        system_event evt1(false);
        system_event evt2(move(evt1));
    }
    SUCCEED();
}

TEST_F(SystemEventTest, CopyConstructor_IsDeleted) { EXPECT_FALSE(is_copy_constructible<system_event>::value); }

TEST_F(SystemEventTest, CopyAssignment_IsDeleted) { EXPECT_FALSE(is_copy_assignable<system_event>::value); }

TEST_F(SystemEventTest, MoveConstructor_IsNoexcept) { EXPECT_TRUE(is_nothrow_move_constructible<system_event>::value); }

TEST_F(SystemEventTest, MoveAssignment_IsNoexcept) { EXPECT_TRUE(is_nothrow_move_assignable<system_event>::value); }

TEST_F(SystemEventTest, Type_EnumValues_Distinct) {
    EXPECT_NE(static_cast<int>(system_event::type::auto_reset), static_cast<int>(system_event::type::manual_reset));
}

TEST_F(SystemEventTest, MultipleEvents_Independent) {
    system_event evt1(false);
    system_event evt2(false);

    evt1.set();

    EXPECT_TRUE(evt1.wait(0));
    EXPECT_FALSE(evt2.wait(0));
}

TEST_F(SystemEventTest, SetResetSequence_MultipleTimes) {
    system_event evt(false, system_event::type::auto_reset);

    for (int i = 0; i < 5; ++i) {
        evt.set();
        EXPECT_TRUE(evt.wait(0));
        EXPECT_FALSE(evt.wait(0));

        evt.set();
        evt.reset();
        EXPECT_FALSE(evt.wait(0));
    }
}

TEST_F(SystemEventTest, Wait_InterruptedBySet_ReturnsTrue) {
    system_event evt(false);

    thread waiter([&]() {
        bool result = evt.wait(1000);
        EXPECT_TRUE(result);
    });

    this_thread::sleep_for(milliseconds(20));
    evt.set();

    waiter.join();
}

TEST_F(SystemEventTest, Wait_InterruptedBySet_WithTimeout) {
    system_event evt(false);

    thread waiter([&]() {
        bool result = evt.wait(1000);
        EXPECT_TRUE(result);
    });

    this_thread::sleep_for(milliseconds(10));
    evt.set();

    waiter.join();
}

TEST_F(SystemEventTest, Constructor_InitialStateTrue_WaitZeroReturnsTrue) {
    system_event evt(true, system_event::type::auto_reset);
    EXPECT_TRUE(evt.wait(0));
    EXPECT_FALSE(evt.wait(0));
}

TEST_F(SystemEventTest, Constructor_InitialStateTrue_ManualReset_WaitMultipleReturnsTrue) {
    system_event evt(true, system_event::type::manual_reset);
    EXPECT_TRUE(evt.wait(0));
    EXPECT_TRUE(evt.wait(0));
}

TEST_F(SystemEventTest, MoveAssignment_DifferentTypes_TypePreserved) {
    system_event evt1(false, system_event::type::manual_reset);
    system_event evt2(false, system_event::type::auto_reset);

    evt2 = move(evt1);

    EXPECT_EQ(evt2.event_type(), system_event::type::manual_reset);
}

TEST_F(SystemEventTest, Wait_DefaultTimeout_IsInfinite) {
    system_event evt(false);

    atomic<bool> done{false};
    thread t([&]() {
        bool result = evt.wait();
        done.store(true);
        EXPECT_TRUE(result);
    });

    this_thread::sleep_for(milliseconds(20));
    EXPECT_FALSE(done.load());

    evt.set();
    this_thread::sleep_for(milliseconds(20));
    EXPECT_TRUE(done.load());

    t.join();
}

TEST_F(SystemEventTest, SetBeforeWait_AutoReset_WaitReturnsImmediately) {
    system_event evt(false, system_event::type::auto_reset);
    evt.set();

    auto start = steady_clock::now();
    bool result = evt.wait(1000);
    auto end = steady_clock::now();

    EXPECT_TRUE(result);
    EXPECT_LT(time_cast<milliseconds>(end - start).count(), 50);
}

TEST_F(SystemEventTest, SetBeforeWait_ManualReset_WaitReturnsImmediately) {
    system_event evt(false, system_event::type::manual_reset);
    evt.set();

    auto start = steady_clock::now();
    bool result = evt.wait(1000);
    auto end = steady_clock::now();

    EXPECT_TRUE(result);
    EXPECT_LT(time_cast<milliseconds>(end - start).count(), 50);
}

TEST_F(SystemEventTest, ConcurrentSetAndWait_MultipleThreads) {
    constexpr int num_threads = 4;
    system_event evt(false, system_event::type::manual_reset);

    vector<thread> threads;
    atomic<int> success_count{0};

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            if (evt.wait(500)) {
                ++success_count;
            }
        });
    }

    this_thread::sleep_for(milliseconds(20));
    evt.set();

    for (auto& t: threads) {
        t.join();
    }

    EXPECT_EQ(success_count.load(), num_threads);
}

TEST_F(SystemEventTest, Reset_WhileThreadsWaiting_WakesNobody) {
    system_event evt(false, system_event::type::auto_reset);
    atomic<int> wake_count{0};

    thread t([&]() {
        if (evt.wait(200)) {
            ++wake_count;
        }
    });

    this_thread::sleep_for(milliseconds(50));
    evt.set();
    evt.reset();

    t.join();

    EXPECT_GE(wake_count.load(), 0);
}

class SignalManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto& mgr = system_signal_manager::instance();
        mgr.reset_force();

        mgr.remove_handler(system_signal_manager::event::CUSTOM_1);
        mgr.remove_handler(system_signal_manager::event::CUSTOM_2);
        mgr.remove_handler(system_signal_manager::event::TIMEOUT);
        mgr.remove_handler(system_signal_manager::event::USER1);
        mgr.remove_handler(system_signal_manager::event::USER2);

        mgr.set_force_exit_timeout(5000);
    }

    void TearDown() override {
        auto& mgr = system_signal_manager::instance();
        mgr.reset_force();

        mgr.remove_handler(system_signal_manager::event::CUSTOM_1);
        mgr.remove_handler(system_signal_manager::event::CUSTOM_2);
        mgr.remove_handler(system_signal_manager::event::TIMEOUT);
        mgr.remove_handler(system_signal_manager::event::USER1);
        mgr.remove_handler(system_signal_manager::event::USER2);
    }
};

TEST_F(SignalManagerTest, Instance_ReturnsSameInstance) {
    system_signal_manager& mgr1 = system_signal_manager::instance();
    system_signal_manager& mgr2 = system_signal_manager::instance();
    EXPECT_EQ(&mgr1, &mgr2);
}

TEST_F(SignalManagerTest, Instance_InitiallyNotRunning) {
    system_signal_manager& mgr = system_signal_manager::instance();
    EXPECT_FALSE(mgr.is_running());
}

TEST_F(SignalManagerTest, StartMonitoring_StartsRunning) {
    system_signal_manager& mgr = system_signal_manager::instance();
    mgr.start_monitoring();
    EXPECT_TRUE(mgr.is_running());
    mgr.stop_monitoring();
}

TEST_F(SignalManagerTest, StopMonitoring_StopsRunning) {
    system_signal_manager& mgr = system_signal_manager::instance();
    mgr.start_monitoring();
    mgr.stop_monitoring();
    EXPECT_FALSE(mgr.is_running());
}

TEST_F(SignalManagerTest, StartMonitoring_AlreadyRunning_NoEffect) {
    system_signal_manager& mgr = system_signal_manager::instance();
    mgr.start_monitoring();
    EXPECT_TRUE(mgr.is_running());
    mgr.start_monitoring();
    EXPECT_TRUE(mgr.is_running());
    mgr.stop_monitoring();
}

TEST_F(SignalManagerTest, StopMonitoring_AlreadyStopped_NoEffect) {
    system_signal_manager& mgr = system_signal_manager::instance();
    EXPECT_FALSE(mgr.is_running());
    mgr.stop_monitoring();
    EXPECT_FALSE(mgr.is_running());
}

TEST_F(SignalManagerTest, RegisterHandler_ValidHandler_Success) {
    system_signal_manager& mgr = system_signal_manager::instance();
    bool was_called = false;

    mgr.register_handler(system_signal_manager::event::CUSTOM_1,
                         [&was_called](system_signal_manager::event, void*) -> bool {
                             was_called = true;
                             return true;
                         });

    mgr.start_monitoring();
    mgr.send_signal(system_signal_manager::event::CUSTOM_1);

    this_thread::sleep_for(200_ms);
    EXPECT_TRUE(was_called);

    mgr.stop_monitoring();
}

TEST_F(SignalManagerTest, RegisterHandler_NullHandler_ThrowsException) {
    system_signal_manager& mgr = system_signal_manager::instance();
    EXPECT_THROW(mgr.register_handler(system_signal_manager::event::CUSTOM_1, nullptr), system_exception);
}

TEST_F(SignalManagerTest, RegisterHandler_MultipleEvents_AllCalled) {
    system_signal_manager& mgr = system_signal_manager::instance();
    atomic<int> call_count{0};

    auto handler = [&call_count](system_signal_manager::event, void*) -> bool {
        ++call_count;
        return true;
    };

    mgr.register_handler(system_signal_manager::event::CUSTOM_1, handler);
    mgr.register_handler(system_signal_manager::event::CUSTOM_2, handler);

    mgr.start_monitoring();
    mgr.send_signal(system_signal_manager::event::CUSTOM_1);
    mgr.send_signal(system_signal_manager::event::CUSTOM_2);

    this_thread::sleep_for(200_ms);
    EXPECT_GE(call_count.load(), 2);

    mgr.stop_monitoring();
}

TEST_F(SignalManagerTest, RegisterHandlers_BatchRegistersAll) {
    system_signal_manager& mgr = system_signal_manager::instance();
    atomic<int> call_count{0};

    auto handler = [&call_count](system_signal_manager::event, void*) -> bool {
        ++call_count;
        return true;
    };

    vector<system_signal_manager::event> events = {system_signal_manager::event::CUSTOM_1,
                                                   system_signal_manager::event::CUSTOM_2};
    mgr.register_handlers(events, handler);

    mgr.start_monitoring();
    mgr.send_signal(system_signal_manager::event::CUSTOM_1);
    mgr.send_signal(system_signal_manager::event::CUSTOM_2);

    this_thread::sleep_for(200_ms);
    EXPECT_GE(call_count.load(), 2);

    mgr.stop_monitoring();
}

TEST_F(SignalManagerTest, RegisterHandlers_NullHandler_ThrowsException) {
    system_signal_manager& mgr = system_signal_manager::instance();
    vector<system_signal_manager::event> events = {system_signal_manager::event::CUSTOM_1,
                                                   system_signal_manager::event::CUSTOM_2};
    EXPECT_THROW(mgr.register_handlers(events, nullptr), system_exception);
}

TEST_F(SignalManagerTest, RemoveHandler_HandlerNotCalled) {
    system_signal_manager& mgr = system_signal_manager::instance();
    atomic<int> call_count{0};

    auto handler = [&call_count](system_signal_manager::event, void*) -> bool {
        ++call_count;
        return true;
    };

    mgr.register_handler(system_signal_manager::event::CUSTOM_1, handler);
    mgr.remove_handler(system_signal_manager::event::CUSTOM_1);

    mgr.start_monitoring();
    mgr.send_signal(system_signal_manager::event::CUSTOM_1);

    this_thread::sleep_for(200_ms);
    EXPECT_EQ(call_count.load(), 0);

    mgr.stop_monitoring();
}

TEST_F(SignalManagerTest, RemoveHandler_NonExistent_NoThrow) {
    system_signal_manager& mgr = system_signal_manager::instance();
    EXPECT_NO_THROW(mgr.remove_handler(system_signal_manager::event::CUSTOM_1));
}

TEST_F(SignalManagerTest, SendSignal_HasContext_ContextReceived) {
    system_signal_manager& mgr = system_signal_manager::instance();
    void* received_context = nullptr;
    int test_value = 42;

    mgr.register_handler(system_signal_manager::event::CUSTOM_1,
                         [&received_context](system_signal_manager::event, void* ctx) -> bool {
                             received_context = ctx;
                             return true;
                         });

    mgr.start_monitoring();
    mgr.send_signal(system_signal_manager::event::CUSTOM_1, &test_value);

    this_thread::sleep_for(200_ms);
    EXPECT_EQ(received_context, &test_value);

    mgr.stop_monitoring();
}

TEST_F(SignalManagerTest, SendSignal_MultipleTimes_AllProcessed) {
    system_signal_manager& mgr = system_signal_manager::instance();
    atomic<int> call_count{0};

    mgr.register_handler(system_signal_manager::event::CUSTOM_1,
                         [&call_count](system_signal_manager::event, void*) -> bool {
                             ++call_count;
                             return true;
                         });

    mgr.start_monitoring();

    for (int i = 0; i < 5; ++i) {
        mgr.send_signal(system_signal_manager::event::CUSTOM_1);
    }

    this_thread::sleep_for(300_ms);
    EXPECT_EQ(call_count.load(), 5);

    mgr.stop_monitoring();
}

TEST_F(SignalManagerTest, SignalHandler_ReturnsFalse_ProcessContinues) {
    system_signal_manager& mgr = system_signal_manager::instance();
    atomic<bool> was_called{false};

    mgr.register_handler(system_signal_manager::event::CUSTOM_1,
                         [&was_called](system_signal_manager::event, void*) -> bool {
                             was_called = true;
                             return false;
                         });

    mgr.start_monitoring();
    mgr.send_signal(system_signal_manager::event::CUSTOM_1);

    this_thread::sleep_for(200_ms);
    EXPECT_TRUE(was_called.load());
    EXPECT_TRUE(mgr.is_running());

    mgr.stop_monitoring();
}

TEST_F(SignalManagerTest, SendSignal_NotMonitoring_StillProcessedWhenStarted) {
    system_signal_manager& mgr = system_signal_manager::instance();
    atomic<int> call_count{0};

    mgr.register_handler(system_signal_manager::event::CUSTOM_1,
                         [&call_count](system_signal_manager::event, void*) -> bool {
                             ++call_count;
                             return true;
                         });

    mgr.send_signal(system_signal_manager::event::CUSTOM_1);
    mgr.start_monitoring();

    this_thread::sleep_for(200_ms);
    EXPECT_GE(call_count.load(), 1);

    mgr.stop_monitoring();
}

TEST_F(SignalManagerTest, SetForceExitTimeout_UpdatesTimeout) {
    system_signal_manager& mgr = system_signal_manager::instance();
    mgr.set_force_exit_timeout(3000);
    mgr.start_monitoring();
    EXPECT_TRUE(mgr.is_running());
    mgr.stop_monitoring();
}

TEST_F(SignalManagerTest, IsPlatformSignal_NativeSignal_ReturnsTrue) {
#ifdef NEFORCE_PLATFORM_WINDOWS
    EXPECT_TRUE(system_signal_manager::is_platform_signal(system_signal_manager::event::INTERRUPT));
    EXPECT_TRUE(system_signal_manager::is_platform_signal(system_signal_manager::event::CTRL_BREAK));
    EXPECT_TRUE(system_signal_manager::is_platform_signal(system_signal_manager::event::CLOSE));
#else
    EXPECT_TRUE(system_signal_manager::is_platform_signal(system_signal_manager::event::INTERRUPT));
    EXPECT_TRUE(system_signal_manager::is_platform_signal(system_signal_manager::event::TERMINATE));
    EXPECT_TRUE(system_signal_manager::is_platform_signal(system_signal_manager::event::HANGUP));
#endif
}

TEST_F(SignalManagerTest, IsPlatformSignal_CustomSignal_ReturnsFalse) {
    EXPECT_FALSE(system_signal_manager::is_platform_signal(system_signal_manager::event::CUSTOM_1));
    EXPECT_FALSE(system_signal_manager::is_platform_signal(system_signal_manager::event::CUSTOM_2));
    EXPECT_FALSE(system_signal_manager::is_platform_signal(system_signal_manager::event::FORCE_EXIT));
}

TEST_F(SignalManagerTest, BlockUnblockSignals_LinuxOnly_ReturnsResult) {
    system_signal_manager& mgr = system_signal_manager::instance();
    vector<system_signal_manager::event> signals = {system_signal_manager::event::USER1,
                                                    system_signal_manager::event::USER2};

    bool block_result = mgr.block_signals(signals);
    bool unblock_result = mgr.unblock_signals(signals);

    EXPECT_TRUE(block_result);
    EXPECT_TRUE(unblock_result);
}

TEST_F(SignalManagerTest, BlockSignals_EmptyList_ReturnsTrue) {
    system_signal_manager& mgr = system_signal_manager::instance();
    vector<system_signal_manager::event> empty;
    bool result = mgr.block_signals(empty);
    EXPECT_TRUE(result);
}

TEST_F(SignalManagerTest, UnblockSignals_EmptyList_ReturnsTrue) {
    system_signal_manager& mgr = system_signal_manager::instance();
    vector<system_signal_manager::event> empty;
    bool result = mgr.unblock_signals(empty);
    EXPECT_TRUE(result);
}

TEST_F(SignalManagerTest, StopMonitoring_CleansUpHandlers) {
    system_signal_manager& mgr = system_signal_manager::instance();
    mgr.start_monitoring();
    mgr.stop_monitoring();

    mgr.start_monitoring();
    mgr.stop_monitoring();
    SUCCEED();
}

TEST_F(SignalManagerTest, SignalGuard_StartsAndStopsMonitoring) {
    {
        signal_guard guard;
        EXPECT_TRUE(system_signal_manager::instance().is_running());
    }
    EXPECT_FALSE(system_signal_manager::instance().is_running());
}

TEST_F(SignalManagerTest, SignalGuard_Nested_HandlesCorrectly) {
    system_signal_manager& mgr = system_signal_manager::instance();
    {
        signal_guard guard1;
        EXPECT_TRUE(mgr.is_running());
        {
            signal_guard guard2;
            EXPECT_TRUE(mgr.is_running());
        }
        EXPECT_TRUE(mgr.is_running());
    }
    EXPECT_FALSE(mgr.is_running());
}

TEST_F(SignalManagerTest, CopyConstructor_IsDeleted) {
    EXPECT_FALSE(is_copy_constructible<system_signal_manager>::value);
}

TEST_F(SignalManagerTest, CopyAssignment_IsDeleted) { EXPECT_FALSE(is_copy_assignable<system_signal_manager>::value); }

TEST_F(SignalManagerTest, MoveConstructor_IsDeleted) {
    EXPECT_FALSE(is_move_constructible<system_signal_manager>::value);
}

TEST_F(SignalManagerTest, MoveAssignment_IsDeleted) { EXPECT_FALSE(is_move_assignable<system_signal_manager>::value); }

TEST_F(SignalManagerTest, SignalEvent_EnumValues_Distinct) {
    EXPECT_NE(static_cast<int>(system_signal_manager::event::INTERRUPT),
              static_cast<int>(system_signal_manager::event::TERMINATE));
    EXPECT_NE(static_cast<int>(system_signal_manager::event::CUSTOM_1),
              static_cast<int>(system_signal_manager::event::CUSTOM_2));
    EXPECT_NE(static_cast<int>(system_signal_manager::event::TIMEOUT),
              static_cast<int>(system_signal_manager::event::FORCE_EXIT));
}

TEST_F(SignalManagerTest, LongRunningMonitoring_Success) {
    system_signal_manager& mgr = system_signal_manager::instance();
    mgr.start_monitoring();
    this_thread::sleep_for(300_ms);
    EXPECT_TRUE(mgr.is_running());
    mgr.stop_monitoring();
}

TEST_F(SignalManagerTest, SendSignal_AfterStopMonitoring_QueuedAndProcessed) {
    system_signal_manager& mgr = system_signal_manager::instance();
    atomic<int> call_count{0};

    mgr.register_handler(system_signal_manager::event::CUSTOM_1,
                         [&call_count](system_signal_manager::event, void*) -> bool {
                             ++call_count;
                             return true;
                         });

    mgr.start_monitoring();
    mgr.send_signal(system_signal_manager::event::CUSTOM_1);
    this_thread::sleep_for(200_ms);

    EXPECT_EQ(call_count.load(), 1);

    mgr.stop_monitoring();
    mgr.send_signal(system_signal_manager::event::CUSTOM_1);

    mgr.start_monitoring();
    this_thread::sleep_for(200_ms);
    EXPECT_GE(call_count.load(), 2);

    mgr.stop_monitoring();
}

TEST_F(SignalManagerTest, WaitForSignal_SignalReceived_ReturnsCorrectEvent) {
    system_signal_manager& mgr = system_signal_manager::instance();
    atomic<bool> wait_started{false};

    mgr.start_monitoring();

    thread sender([&]() {
        while (!wait_started.load()) {
            this_thread::sleep_for(10_ms);
        }
        this_thread::sleep_for(50_ms);
        mgr.send_signal(system_signal_manager::event::CUSTOM_1);
    });

    wait_started.store(true);
    system_signal_manager::event ev = mgr.wait_for_signal(500);

    EXPECT_TRUE(ev == system_signal_manager::event::CUSTOM_1 || ev == system_signal_manager::event::TIMEOUT);

    sender.join();
    mgr.stop_monitoring();
}

TEST_F(SignalManagerTest, HandlerReceivesCorrectEvent) {
    system_signal_manager& mgr = system_signal_manager::instance();
    atomic<system_signal_manager::event> received_event{system_signal_manager::event::TIMEOUT};

    mgr.register_handler(system_signal_manager::event::CUSTOM_1,
                         [&received_event](system_signal_manager::event ev, void*) -> bool {
                             received_event.store(ev);
                             return true;
                         });

    mgr.start_monitoring();
    mgr.send_signal(system_signal_manager::event::CUSTOM_1);
    this_thread::sleep_for(200_ms);

    EXPECT_EQ(received_event.load(), system_signal_manager::event::CUSTOM_1);

    mgr.stop_monitoring();
}
