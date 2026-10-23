#include <NeForce/tui/dom/latex.hpp>
#include <gtest/gtest.h>
using namespace neforce;
using namespace neforce::tui;

// Expected strings were verified against the TypeScript reference
// (dsh-TUI/src/terminal-utils/latex.ts) through node's type stripping.  Inline
// input whose layout would need several lines (multi-row matrices, cases and
// aligned rows) is rejected because render_latex promises a single line when
// display is false.
namespace {
    string join_lines(const optional<latex_layout>& layout) {
        if (!layout.has_value()) {
            return "<none>";
        }
        string result;
        for (size_t i = 0; i < layout->lines.size(); ++i) {
            if (i > 0) {
                result += '\n';
            }
            result += layout->lines[i];
        }
        return result;
    }

    string render_text(const string_view tex, const bool display = false) {
        return join_lines(render_latex(tex, display));
    }
} // namespace

TEST(TuiLatexTest, PinnedVectors) {
    ASSERT_EQ(render_text("\\mathbb{C}^3 \\to \\mathbb{C}^3"), "ℂ³ → ℂ³");
    ASSERT_EQ(render_text("F_1 = -\\frac{1}{4x^2}."), "F₁ = -1/(4x²).");
    ASSERT_EQ(render_text("e^{i\\pi}+1=0"), "e^(iπ)+1 = 0");
    ASSERT_EQ(render_text("\\alpha + \\beta \\leq \\sqrt{x^2+y^2}"), "α + β ≤ √(x²+y²)");
    ASSERT_EQ(render_text("x = \\frac{-b\\pm\\sqrt{b^2-4ac}}{2a}", true), "    -b±√(b²-4ac)\n"
                                                                          "x = ────────────\n"
                                                                          "         2a");
    ASSERT_EQ(render_text("\\sum_{i=0}^n x_i", true), " n\n"
                                                      " ∑  xᵢ\n"
                                                      "i=0");
    ASSERT_EQ(render_text("\\begin{cases} x^2 & x\\ge 0 \\\\ -x & x<0 \\end{cases}", true), "⎧ x² if x ≥ 0\n"
                                                                                            "⎨\n"
                                                                                            "⎩ -x if x < 0");
    ASSERT_EQ(render_text("\\hat{x}"), "x\xCC\x82");
}

TEST(TuiLatexTest, GreekAndSymbols) {
    ASSERT_EQ(render_text("\\alpha\\beta\\gamma\\delta\\epsilon\\varepsilon"), "αβγδϵε");
    ASSERT_EQ(render_text("\\forall x \\in \\mathbb{R}"), "∀ x ∈ ℝ");
    ASSERT_EQ(render_text("A \\subseteq B"), "A ⊆ B");
    ASSERT_EQ(render_text("x \\neq y"), "x ≠ y");
    ASSERT_EQ(render_text("a \\cdot b \\times c"), "a · b × c");
    ASSERT_EQ(render_text("\\mathbb{CHNPQRZ}"), "ℂℍℕℙℚℝℤ");
    ASSERT_EQ(render_text("\\not\\in"), "∉");
    ASSERT_EQ(render_text("\\not="), "≠");
}

TEST(TuiLatexTest, NamedOperatorsAndLimits) {
    ASSERT_EQ(render_text("\\sin x + \\cos y"), "sin x + cos y");
    ASSERT_EQ(render_text("\\log_2 n"), "log₂ n");
    ASSERT_EQ(render_text("\\det A"), "det A");
    ASSERT_EQ(render_text("\\lim_{x\\to0}"), "lim[x→0]");
    ASSERT_EQ(render_text("\\int_0^1"), "∫₀¹");
    ASSERT_EQ(render_text("\\operatorname{argmax}_{x} f(x)"), "argmax[x] f(x)");
    ASSERT_EQ(render_text("\\sum_{i=0}^{n} i^2", true), " n\n"
                                                        " ∑  i²\n"
                                                        "i=0");
    ASSERT_EQ(render_text("\\lim_{x\\to0} \\frac{\\sin x}{x}", true), "     sin x\n"
                                                                      "lim  ─────\n"
                                                                      "x→0    x");
    ASSERT_EQ(render_text("\\int_0^1 x^2\\,dx", true), "1\n"
                                                       "∫ x² dx\n"
                                                       "0");
    ASSERT_EQ(render_text("\\operatorname*{argmax}_{x} f(x)", true), "argmax f(x)\n"
                                                                     "  x");
}

TEST(TuiLatexTest, FractionsAndRootsInline) {
    ASSERT_EQ(render_text("\\frac{a}{b}"), "a/b");
    ASSERT_EQ(render_text("\\frac{x+1}{y-1}"), "(x+1)/(y-1)");
    ASSERT_EQ(render_text("\\frac{\\frac{1}{2}}{3}"), "(1/2)/3");
    ASSERT_EQ(render_text("\\sqrt{x}"), "√x");
    ASSERT_EQ(render_text("\\sqrt{x+1}"), "√(x+1)");
    ASSERT_EQ(render_text("\\sqrt[3]{y}"), "∛y");
    ASSERT_EQ(render_text("\\sqrt[4]{z}"), "∜z");
    ASSERT_EQ(render_text("\\sqrt[5]{x+1}"), "⁵√(x+1)");
    ASSERT_EQ(render_text("\\binom{n}{k}"), "(n choose k)");
    ASSERT_EQ(render_text("\\boxed{x+1}"), "[x+1]");
}

TEST(TuiLatexTest, FractionsDisplay) {
    ASSERT_EQ(render_text("\\frac{a}{b}", true), "a\n"
                                                 "─\n"
                                                 "b");
    ASSERT_EQ(render_text("\\frac{1}{2} + \\frac{3}{4}", true), "1   3\n"
                                                                "─ + ─\n"
                                                                "2   4");
    ASSERT_EQ(render_text("\\frac{\\frac{1}{2}}{3}", true), "1/2\n"
                                                            "───\n"
                                                            " 3");
    ASSERT_EQ(render_text("\\sqrt{\\frac{1}{2}}", true), "   1\n"
                                                         "√( ─ )\n"
                                                         "   2");
    ASSERT_EQ(render_text("\\begin{equation} \\frac{1}{2} \\end{equation}", true), "1\n"
                                                                                   "─\n"
                                                                                   "2");
}

TEST(TuiLatexTest, Scripts) {
    ASSERT_EQ(render_text("x_{i+1}"), "xᵢ₊₁");
    ASSERT_EQ(render_text("a^{n+1}"), "aⁿ⁺¹");
    ASSERT_EQ(render_text("x^A"), "x^A");
    ASSERT_EQ(render_text("x^{AB}"), "x^(AB)");
    ASSERT_EQ(render_text("x^{12}"), "x¹²");
    ASSERT_EQ(render_text("x^{a/b}", true), "x^(a/b)");
    ASSERT_EQ(render_text("x^{\\alpha}", true), " α\n"
                                                "x");
    ASSERT_EQ(render_text("x_{\\alpha}^{\\beta}", true), " β\n"
                                                         "x\n"
                                                         " α");
}

TEST(TuiLatexTest, Matrices) {
    ASSERT_EQ(render_text("\\begin{pmatrix} a & b \\\\ c & d \\end{pmatrix}", true), "⎛ a │ b ⎞\n"
                                                                                     "⎝ c │ d ⎠");
    ASSERT_EQ(render_text("\\begin{bmatrix} 1 & 2 \\\\ 3 & 4 \\end{bmatrix}", true), "⎡ 1 │ 2 ⎤\n"
                                                                                     "⎣ 3 │ 4 ⎦");
    ASSERT_EQ(render_text("\\begin{vmatrix} a & b \\\\ c & d \\end{vmatrix}", true), "│ a │ b │\n"
                                                                                     "│ c │ d │");
    ASSERT_EQ(render_text("\\begin{Bmatrix} x \\end{Bmatrix}", true), "⎧ x ⎫");
    ASSERT_EQ(render_text("\\begin{Vmatrix} x \\end{Vmatrix}", true), "║ x ║");
    ASSERT_EQ(render_text("\\begin{matrix} a & b \\\\ c & d \\end{matrix}", true), "a │ b\n"
                                                                                   "c │ d");
    ASSERT_EQ(render_text("\\begin{array}{c|c} a & b \\\\ c & d \\end{array}", true), "a │ b\n"
                                                                                      "c │ d");
    ASSERT_EQ(render_text("A = \\begin{pmatrix} a \\\\ b \\end{pmatrix}", true), "A = ⎛ a ⎞\n"
                                                                                 "    ⎝ b ⎠");
    ASSERT_EQ(render_text("\\begin{pmatrix} a \\\\ b \\end{pmatrix}.", true), "⎛ a ⎞\n"
                                                                              "⎝ b ⎠.");
    ASSERT_EQ(render_text("\\begin{pmatrix} a \\end{pmatrix}"), "⎛ a ⎞");
    ASSERT_FALSE(render_latex("\\begin{pmatrix} a \\\\ b \\end{pmatrix}", false).has_value());
}

TEST(TuiLatexTest, MatrixColumnsAreAlignedAndRightTrimmed) {
    ASSERT_EQ(render_text("\\begin{matrix} a & bb \\\\ ccc & d \\end{matrix}", true), "a   │ bb\n"
                                                                                      "ccc │ d");
}

TEST(TuiLatexTest, CasesAndAlign) {
    ASSERT_EQ(render_text("\\begin{cases} a & b \\\\ c & d \\\\ e & f \\end{cases}", true), "⎧ a if b\n"
                                                                                            "⎨ c if d\n"
                                                                                            "⎩ e if f");
    ASSERT_EQ(render_text("\\begin{cases} x & y \\end{cases}", true), "⎧ x if y");
    ASSERT_EQ(render_text("\\begin{cases} x & \\text{if } x>0 \\\\ -x & \\text{otherwise} \\end{cases}", true),
              "⎧ x  if x > 0\n"
              "⎨\n"
              "⎩ -x otherwise");
    ASSERT_EQ(render_text("\\begin{cases} x, & x>0 \\end{cases}", true), "⎧ x if x > 0");
    ASSERT_EQ(render_text("\\begin{aligned} a &= b+c \\\\ d &= e \\end{aligned}", true), "a = b+c\n"
                                                                                         "d = e");
    ASSERT_EQ(render_text("\\begin{alignat}{2} x &= 1 & y &= 2 \\end{alignat}", true), "x = 1 y = 2");
}

TEST(TuiLatexTest, Accents) {
    ASSERT_EQ(render_text("\\bar{x}"), "x\xCC\x85");
    ASSERT_EQ(render_text("\\vec{v}"), "v\xE2\x83\x97");
    ASSERT_EQ(render_text("\\tilde{n}"), "n\xCC\x83");
    ASSERT_EQ(render_text("\\underline{x}"), "x\xCC\xB2");
    ASSERT_EQ(render_text("\\hat{\\alpha}"), "α\xCC\x82");
    ASSERT_EQ(render_text("\\overline{AB}"), "overline(AB)");
    ASSERT_EQ(render_text("\\vec{AB}"), "vec(AB)");
}

TEST(TuiLatexTest, TextAndSpacing) {
    ASSERT_EQ(render_text("\\text{hello world}"), "hello world");
    ASSERT_EQ(render_text("\\mathrm{d}x"), "dx");
    ASSERT_EQ(render_text("\\left( x \\right)"), "( x )");
    ASSERT_EQ(render_text("\\left\\{ x \\right\\}"), "{ x }");
    ASSERT_EQ(render_text("\\big( x \\big)"), "( x )");
    ASSERT_EQ(render_text("\\mathbf{x}"), "x");
    ASSERT_EQ(render_text("a\\,b\\;c\\quad d"), "a b c d");
    ASSERT_EQ(render_text("a\\!b"), "ab");
    ASSERT_EQ(render_text("a~b"), "a b");
}

TEST(TuiLatexTest, DecorationsAndModuli) {
    ASSERT_EQ(render_text("\\overset{def}{=}"), "=ᵈᵉᶠ");
    ASSERT_EQ(render_text("\\underset{x}{\\min}"), "minₓ");
    ASSERT_EQ(render_text("a \\equiv b \\pmod{7}"), "a ≡ b (mod 7)");
    ASSERT_EQ(render_text("a \\bmod b"), "a mod b");
    ASSERT_EQ(render_text("\\not\\alpha"), "α\xCC\xB8");
    ASSERT_EQ(render_text("\\displaystyle\\sum_{i=1}^n i", true), " n\n"
                                                                  " ∑  i\n"
                                                                  "i=1");
}

TEST(TuiLatexTest, FailuresReturnNone) {
    ASSERT_FALSE(render_latex("\\unknowncmd").has_value());
    ASSERT_FALSE(render_latex("x}").has_value());
    ASSERT_FALSE(render_latex("\\frac{1}{2").has_value());
    ASSERT_FALSE(render_latex("\\sqrt").has_value());
    ASSERT_FALSE(render_latex("\\frac{1}").has_value());
    ASSERT_FALSE(render_latex("\\frac").has_value());
    ASSERT_FALSE(render_latex("\\pmod").has_value());
    ASSERT_FALSE(render_latex("\\begin{foo} bar \\end{foo}").has_value());
    ASSERT_FALSE(render_latex("\\begin{matrix} a").has_value());
    ASSERT_FALSE(render_latex("\\end{matrix}").has_value());
}
