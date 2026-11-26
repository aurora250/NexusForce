#include <NeForce/core/string/regex.hpp>
#include <NeForce/core/string/string.hpp>
#include <NeForce/core/system/console.hpp>

using namespace neforce;

int main() {
    const regex expression(R"((?<user>[\w.+-]+)@(?<host>[\w.-]+\.[a-z]{2,}))");

    const string text = "Contact dev@nexusforce.dev or sales@example.com for details.";

    const auto first = expression.search(text);

    if (first.matched()) {
        println("first match: ", first.data());
        println("user: ", first[expression.group_index("user")], ", host: ", first[expression.group_index("host")]);
        println("prefix: ", first.prefix());
    }

    for (const auto& match: expression.find_all(text)) {
        println("found ", match.data(), " at ", match.position());
    }

    println("masked: ", expression.replace_all(text, "[$2]"));

    const regex nested("(a+)+b");
    println("nested quantifier: ", nested.analysis().has_nested_quantifier ? "yes" : "no",
            ", exponential risk: ", nested.analysis().may_backtrack_exponentially ? "yes" : "no");

    const regex fields(",");
    const auto parts = fields.split("id,name,email");
    println("fields: ", parts.size());

    return 0;
}
