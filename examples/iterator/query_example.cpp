/**
 * @brief 链式查询示例——惰性算子与终端算子
 *
 * 演示 query 链式查询的典型用法：
 * - 由容器构造惰性查询链并与管道写法互操作
 * - 惰性算子的组合、短路与重复求值
 * - 终端算子求值、收集与聚合
 */

#include <NeForce/core/container/vector.hpp>
#include <NeForce/core/iterator/query.hpp>
#include <NeForce/core/iterator/ranges/views.hpp>
#include <NeForce/core/string/string.hpp>
#include <NeForce/core/system/console.hpp>
#include <NeForce/core/utility/pair.hpp>
using namespace neforce;

namespace {
    struct employee {
        string name;
        int age;
        int salary;
    };

    vector<employee> make_employees() {
        return {{"alice", 31, 13000}, {"bob", 24, 8000}, {"carol", 45, 21000}, {"dave", 19, 5000}};
    }

    size_t count_elements(const auto& range) {
        size_t total = 0;
        for (auto&& value: range) {
            (void) value;
            ++total;
        }
        return total;
    }

    void print_names(const char* title, const vector<string>& names) {
        print(title, "：");
        for (size_t index = 0; index < names.size(); ++index) {
            if (index != 0) {
                print(", ");
            }
            print(names[index]);
        }
        println("");
    }
} // namespace


int main() {
    vector<employee> staff = make_employees();

    println("== 惰性链：过滤 + 变换 + 截断 ==");
    auto adults = ranges::from(staff)
                          .filter([](const employee& e) { return e.age >= 30; })
                          .transform([](const employee& e) { return e.name; })
                          .take(2);
    print_names("  年龄不小于 30 的前两位", adults.to_vector());

    println("== 终端算子 ==");
    auto ages = ranges::from(staff).transform([](const employee& e) { return e.age; });
    auto salaries = ranges::from(staff).transform([](const employee& e) { return e.salary; });
    printfln("  总人数 {}", ranges::from(staff).count());
    printfln("  平均年龄 {}", ages.average());
    printfln("  年龄区间 {} ~ {}", ages.min().value_or(0), ages.max().value_or(0));
    printfln("  最高薪资 {}", salaries.max().value_or(0));
    printfln("  薪资合计 {}", salaries.sum());
    printfln("  是否存在 20 岁以下 {}", ranges::from(staff).any_of([](const employee& e) { return e.age < 20; }));
    printfln("  是否全部达到法定年龄 {}", ranges::from(staff).all_of([](const employee& e) { return e.age >= 18; }));
    printfln("  第一位 30 岁以上员工 {}",
             ranges::from(staff).filter([](const employee& e) { return e.age >= 30; }).first_or(employee{}).name);

    println("== 聚合与收集 ==");
    printfln("  年龄合计 {}",
             ranges::from(staff).aggregate(0, [](int total, const employee& e) { return total + e.age; }));
    auto names = ranges::from(staff).transform([](const employee& e) { return e.name; }).to_vector();
    print_names("  全部姓名", names);
    printfln("  姓名去重后 {} 个", ranges::from(names).to_set().size());

    println("== 键值视图 ==");
    vector<pair<string, int>> scores = {{"alice", 90}, {"bob", 75}};
    auto keys = ranges::from(scores).keys().to_vector();
    auto values = ranges::from(scores).values().to_vector();
    printfln("  键 {} 个，值 {} 个，首值 {}", keys.size(), values.size(), values.front());

    println("== 展平与拼接 ==");
    vector<vector<int>> groups = {{1, 2}, {3}, {4, 5}};
    printfln("  展平后元素 {} 个", ranges::from(groups).join().count());
    vector<int> head = {1, 2};
    printfln("  拼接后元素 {} 个，末元素 {}", ranges::from(head).concat(vector<int>{3, 4, 5}).count(),
             ranges::from(head).concat(vector<int>{3, 4, 5}).last_or(-1));

    println("== 链式与管道等价 ==");
    auto predicate = [](const employee& e) { return e.salary > 9000; };
    auto chained = ranges::from(staff).filter(predicate).count();
    size_t piped_count = 0;
    for (auto&& element: staff | ranges::views::filter(predicate)) {
        (void) element;
        ++piped_count;
    }
    printfln("  链式 {} 个，管道 {} 个", chained, piped_count);

    println("== 物化与顺序算子 ==");
    vector<int> rolls = {5, 3, 5, 1, 2, 3};
    print("  去重并降序：");
    for (auto value: ranges::from(rolls).distinct().order_by_descending([](int x) { return x; })) {
        print(value, " ");
    }
    println("");
    size_t group_total = 0;
    for (auto group: ranges::from(rolls).group_by([](int x) { return x % 2; })) {
        printfln("  奇偶分组 键 {} 元素 {} 个", group.key(), count_elements(group));
        ++group_total;
    }
    printfln("  分组 {} 个，分块（每块 2 个）{} 块，滑动窗口（宽 3）{} 个", group_total,
             ranges::from(rolls).chunk(2).count(), ranges::from(rolls).sliding(3).count());
    print("  前缀和：");
    for (auto value: ranges::from(rolls).scan(0, [](int acc, int x) { return acc + x; })) {
        print(value, " ");
    }
    println("");
    printfln("  首项下标 {}", ranges::from(rolls).enumerate().elements<0>().first_or(0U));
    auto lookup = ranges::from(rolls).to_lookup([](int x) { return x % 2; });
    printfln("  奇偶查找表：奇数 {} 个，偶数 {} 个", lookup.at(1).size(), lookup.at(0).size());

    println("== 重复求值与源变化 ==");
    auto query = ranges::from(staff).filter([](const employee& e) { return e.age >= 24; });
    printfln("  第一次求值 {} 个", query.count());
    staff.push_back({"erin", 28, 11000});
    printfln("  源新增一名员工后第二次求值 {} 个", query.count());
    printfln("  查询本身仍是视图，可继续参与管道：{} 个",
             (query | ranges::views::take(1)).begin() != (query | ranges::views::take(1)).end() ? 1 : 0);
    return 0;
}
