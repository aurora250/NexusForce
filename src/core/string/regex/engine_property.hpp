#ifndef NEFORCE_CORE_STRING_REGEX_ENGINE_PROPERTY_SRC__
#define NEFORCE_CORE_STRING_REGEX_ENGINE_PROPERTY_SRC__
#include "engine_types.hpp"
NEFORCE_BEGIN_NAMESPACE__

NEFORCE_NODISCARD inline bool property_categories(const string& name, uint32_t& mask) noexcept {
    if (name == "l" || name == "letter") {
        mask = cat_letters;
    } else if (name == "lu" || name == "uppercase_letter") {
        mask = cat_lu;
    } else if (name == "ll" || name == "lowercase_letter") {
        mask = cat_ll;
    } else if (name == "lt" || name == "titlecase_letter") {
        mask = cat_lt;
    } else if (name == "lm" || name == "modifier_letter") {
        mask = cat_lm;
    } else if (name == "lo" || name == "other_letter") {
        mask = cat_lo;
    } else if (name == "lc" || name == "cased_letter") {
        mask = cat_lu | cat_ll | cat_lt;
    } else if (name == "m" || name == "mark") {
        mask = cat_marks;
    } else if (name == "mn" || name == "nonspacing_mark") {
        mask = cat_mn;
    } else if (name == "mc" || name == "spacing_mark") {
        mask = cat_mc;
    } else if (name == "me" || name == "enclosing_mark") {
        mask = cat_me;
    } else if (name == "n" || name == "number") {
        mask = cat_numbers;
    } else if (name == "nd" || name == "decimal_number") {
        mask = cat_nd;
    } else if (name == "nl" || name == "letter_number") {
        mask = cat_nl;
    } else if (name == "no" || name == "other_number") {
        mask = cat_no;
    } else if (name == "p" || name == "punctuation") {
        mask = cat_punctuation;
    } else if (name == "pc" || name == "connector_punctuation") {
        mask = cat_pc;
    } else if (name == "pd" || name == "dash_punctuation") {
        mask = cat_pd;
    } else if (name == "ps" || name == "open_punctuation") {
        mask = cat_ps;
    } else if (name == "pe" || name == "close_punctuation") {
        mask = cat_pe;
    } else if (name == "pi" || name == "initial_punctuation") {
        mask = cat_pi;
    } else if (name == "pf" || name == "final_punctuation") {
        mask = cat_pf;
    } else if (name == "po" || name == "other_punctuation") {
        mask = cat_po;
    } else if (name == "s" || name == "symbol") {
        mask = cat_symbols;
    } else if (name == "sm" || name == "math_symbol") {
        mask = cat_sm;
    } else if (name == "sc" || name == "currency_symbol") {
        mask = cat_sc;
    } else if (name == "sk" || name == "modifier_symbol") {
        mask = cat_sk;
    } else if (name == "so" || name == "other_symbol") {
        mask = cat_so;
    } else if (name == "z" || name == "separator") {
        mask = cat_separators;
    } else if (name == "zs" || name == "space_separator") {
        mask = cat_zs;
    } else if (name == "zl" || name == "line_separator") {
        mask = cat_zl;
    } else if (name == "zp" || name == "paragraph_separator") {
        mask = cat_zp;
    } else if (name == "c" || name == "other") {
        mask = cat_others;
    } else if (name == "cc" || name == "control") {
        mask = cat_cc;
    } else if (name == "cf" || name == "format") {
        mask = cat_cf;
    } else if (name == "co" || name == "private_use") {
        mask = cat_co;
    } else if (name == "cs" || name == "surrogate") {
        mask = cat_cs;
    } else if (name == "cn" || name == "unassigned") {
        mask = cat_cn;
    } else {
        return false;
    }

    return true;
}

NEFORCE_NODISCARD inline int property_script(const string& name) noexcept {
    if (name == "han") {
        return 1;
    }
    if (name == "hiragana") {
        return 2;
    }
    if (name == "katakana") {
        return 3;
    }
    if (name == "hangul") {
        return 4;
    }
    return 0;
}

inline void apply_property(char_class& target, const string& name) {
    if (name == "any") {
        target.add(0, 0x10FFFF);
        return;
    }
    if (name == "ascii") {
        target.add(0, 0x7F);
        return;
    }

    const int script = property_script(name);
    if (script != 0) {
        target.add_script(script);
        return;
    }

    uint32_t mask = 0;
    if (property_categories(name, mask)) {
        target.add_categories(mask);
        return;
    }

    if (name == "xan" || name == "alnum") {
        target.add_categories(cat_letters | cat_numbers);
        return;
    }
    if (name == "xwd" || name == "word") {
        target.add_categories(cat_letters | cat_numbers);
        target.add('_', '_');
        return;
    }
    if (name == "xsp" || name == "space") {
        target.add_categories(cat_separators);
        target.add_ascii_class('s');
        target.add(0x85, 0x85);
        return;
    }
    if (name == "alpha") {
        target.add_categories(cat_letters);
        return;
    }
    if (name == "digit") {
        target.add_categories(cat_nd);
        return;
    }
    if (name == "upper") {
        target.add_categories(cat_lu);
        return;
    }
    if (name == "lower") {
        target.add_categories(cat_ll);
        return;
    }
    if (name == "punct") {
        target.add_categories(cat_punctuation | cat_symbols);
        return;
    }
    if (name == "graph") {
        target.add(0x21, 0x7E);
        return;
    }
    if (name == "print") {
        target.add(0x20, 0x7E);
        return;
    }
    if (name == "blank") {
        target.add(' ', ' ');
        target.add('\t', '\t');
        return;
    }
    if (name == "cntrl") {
        target.add_categories(cat_cc);
        return;
    }
    if (name == "xdigit") {
        target.add('0', '9');
        target.add('a', 'f');
        target.add('A', 'F');
        return;
    }

    throw compile_failure{err_unknown_property, invalid_offset};
}

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_STRING_REGEX_ENGINE_PROPERTY_SRC__
