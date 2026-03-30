#include "test.h"

void test_list() {
    list<int> lls{ 1,2,3,4,5,6,7 };
    println(lls);
    lls.push_back(3);
    lls.push_back(4);
    lls.push_front(10);
    println(lls);
    lls.reverse();
    println(lls);
    lls.sort();
    lls.pop_back();
    lls.pop_front();
    println(lls);
    list<int> lls2 = { 5,3,2,1,1 };
    println(lls2);
    lls2.remove(5);
    lls2.sort();
    println(lls2);
    lls2.unique();
    println(lls2);
    list<unique_ptr<int>> nocopy;
    // nocopy.emplace_back(2); also not support in std
    lls.clear();

    list<int> long_list;
    constexpr size_t element_count = 100000;
    for (size_t i = 0; i < element_count; ++i) {
        if (i % 2 == 0) {
            long_list.push_back(i);
        } else {
            long_list.push_front(i);
        }
    }
    for (size_t i = 0; i < element_count; ++i) {
        if (i % 2 == 0) {
            long_list.pop_back();
        } else {
            long_list.pop_back();
        }
    }
    // println(long_list);
}

void test_check() {
    println(check_type<string>());
    println(check_type<const volatile void* const*&>());
    println(check_type<int(*)[]>());
    println(check_type<const volatile void* (&)[10]>()); // void const volatile * (&) [10]
    println(check_type<int[1][2][3]>());              // int [1] [2] [3]
    println(check_type<char(*(* const)(const int(&)[10]))[10]>());
    println(check_type<int (integer16::* const)[3]>());
    println(check_type<int (integer16::* const)(int, integer16&&, int) volatile>());
    string cstr("const string");
    const string* sr = new string("hai");
    println(check_type<decltype((cstr))>());
    println(check_type<decltype(move(cstr))>());
    println(check_type<decltype(sr)>());
    delete sr;
}

void test_deque() {
    deque<int> a{1,2,3,4,5,6,7,8,9,10};
    println(a);
    a.push_back(2);
    a.push_front(10);
    a.push_back(3);
    a.push_back(7);
    a.push_back(6);
    a.insert(a.end(), 100);
    a.emplace(a.begin(), 0);
    println(a);
    a.pop_back();
    a.pop_front();
    println(a);
    a.assign(10, 5);
    println(a);
    deque<int> b{ 1,2,3,4,5 };
    println(b);
    deque<int> c(move(b));
    c.resize(10, 6);
    println(c);

    deque<int> long_deque;
    constexpr size_t element_count = 100000;
    for (size_t i = 0; i < element_count; ++i) {
        if (i % 2 == 0) {
            long_deque.push_back(i);
        } else {
            long_deque.push_front(i);
        }
    }
    // println(long_deque);
    for (size_t i = 0; i < element_count; ++i) {
        if (i % 2 == 0) {
            long_deque.pop_back();
        } else {
            long_deque.pop_front();
        }
    }
    // println(long_deque);
}

void test_stack() {
    stack<int> s;
    s.push(2);
    s.push(3);
    s.push(5);
    s.push(4);
    s.pop();

    stack<int> long_stack;
    constexpr size_t element_count = 100000;
    for (size_t i = 0; i < element_count; ++i) {
        long_stack.push(i);
    }
    for (size_t i = 0; i < element_count; ++i) {
        long_stack.pop();
    }
    // println(long_stack);
}

void test_vector() {
    try{
        vector<int> v{ 1,2,3,4 };
        v.push_back(3);
        v.push_back(4);
        println(v);
        vector<int> v2(v);
        v.insert(v.end(), v2.cbegin(), v2.cend());
        println(v);
        v.pop_back();
        v.clear();
        println(v.empty());
        v.insert(v.end(), v2.cbegin(), v2.cend());
        println(v);
        const auto v3 = move(v2);
        println(v3);
        vector<int> v4 = { 3,2,1 };
        v4.shrink_to_fit();
        v4.emplace(v4.begin() + 1, 5);
        v4.erase(--v4.end());
        println(v4);

        vector<int> vec;
        vec.assign(5, 10);
        println(vec);
        vec.assign({ 1, 2, 3, 4, 5 });
        println(vec);
        vector<int> anotherVec = { 6, 7, 8 };
        vec.assign(anotherVec.begin(), anotherVec.end());
        println(vec);
    } catch (exception& error) {
        println(error.what());
    }

    vector<int> long_vector;
    constexpr size_t element_count = 100000;
    for (size_t i = 0; i < element_count; ++i) {
        long_vector.push_back(i);
    }
    for (size_t i = 0; i < element_count; ++i) {
        long_vector.pop_back();
    }
    // println(long_vector);
}

void test_pqueue() {
    priority_queue<int> q;
    q.push(6); q.push(9); q.push(1); q.push(5);
    q.push(8); q.push(4); q.emplace(7); // 9 8 7 5 6 1 4
    q.pop();

    priority_queue<int> long_pque;
    constexpr size_t element_count = 1000;
    random_lcd rand;
    for (int i = 0; i < element_count; ++i) {
        long_pque.push(rand.next_int(1000));
    }
    for (int i = 0; i < element_count; ++i) {
        long_pque.pop();
    }
}

void test_rbtree() {
    map<int, char> m;
    m.insert(pair<int, char>(1, 'c'));
    m.emplace(3, 'c');
    m.emplace_hint(m.end(), 4, 'd');
    m[1] = 'a';
    m[100] = 'x';
    m[2] = 'b';
    println(m);
    m.erase(m.begin());
    println(m);
    m.clear();

    map<int, float> long_map;
    random_lcd rand;
    for (int i = 0; i < 100000; ++i) {
        int key = i;
        long_map.insert({key, rand.next_float(0.f, 10000.f)});
    }
    for (int i = 0; i < 100000; ++i) {
        long_map.erase(i);
    }
    // println(long_map);


    multimap<int, const char*> mm;
    mm.emplace(1, "c");
    mm.emplace(2, "b");
    mm.emplace(1, "a");
    println(mm);
    mm.erase(mm.begin());
    mm.insert(mm.begin(), pair<int, const char*>(1, "a"));
    println(mm);
    mm.clear();

    multimap<int, float> long_multimap;
    for (int i = 0; i < 100000; ++i) {
        int key = i;
        long_multimap.insert({key, rand.next_float(0.f, 10000.f)});
    }
    for (int i = 0; i < 100000; ++i) {
        long_multimap.erase(i);
    }
    // println(long_multimap);


    set<int> s{ 1,2,3,4,5 };
    s.insert(s.begin(), 1);
    s.emplace(2);
    println(s);
    s.erase(s.begin());
    println(s);
    s.clear();
    println(s);

    set<int> long_set;
    for (int i = 0; i < 100000; ++i) {
        long_set.emplace(i);
    }
    for (int i = 0; i < 100000; ++i) {
        long_set.erase(i);
    }
    // println(long_set);


    multiset<int> ms{ 4,5,6,7,8,8 };
    ms.insert(ms.begin(), 9);
    ms.emplace(10);
    println(ms);

    multiset<int> long_multiset;
    for (int i = 0; i < 100000; ++i) {
        long_multiset.emplace(i);
    }
    for (int i = 0; i < 100000; ++i) {
        long_multiset.erase(i);
    }
    // println(long_multiset);
}

void test_tuple() {
    tuple<int, char, const char*> t(1, 't', "NeForce");
    auto a = get<0>(t);
    println(get<1>(t));
    auto forw = make_tuple(9, 0);

    pair<int, double> pair1(1, 3.14);
#ifdef NEFORCE_STANDARD_17
    auto [p1, p2] = pair1;
    println(p1, p2);
#endif
    tuple<int, double> tuple1(pair1);
    tuple<string> tuple2("hello");
    tuple<char> tuple3('A');

    auto combinedTuple = tuple_cat(tuple1, tuple2, tuple3);
    println(check_type<decltype(combinedTuple)>());

    println("Combined tuple elements:");
    println(get<0>(combinedTuple));
    println(get<1>(combinedTuple));
    println(get<2>(combinedTuple));
    println(get<3>(combinedTuple));

    tuple<int, int, int> args(1, 2, 3);
#ifdef NEFORCE_STANDARD_17
    auto [av, bv, cv] = args;
    println(av, bv, cv);
#endif

    int sum = apply([](int a, int b, int c) {
        return a + b + c;
    }, args);
    println("Sum:", sum);

    tuple<int, int> mulArgs(4, 5);
    int product = apply(multiplies<int>(), mulArgs);
    println("Product:", product);
}

void test_hashtable() {
    unordered_map<int, char> m;
    m[1] = 'a';
    m[2] = 'b';
    m.insert(pair<int, char>(3, 'c'));
    m.emplace(2, 'c');
    m.insert(pair<int, char>(1, 'b'));
    println(m);
    unordered_map<int, char> m2;
    m2.insert(m.begin(), m.end());
    println(m2);
    unordered_multimap<string, int> mm;
    mm.emplace("a", 1);
    mm.emplace("a", 2);
    mm.insert(pair<string, int>(string("a"), 1));
    println(mm);
    mm.clear();
    println(mm);
    unordered_map<int, unique_ptr<int>> uncopy;
    uncopy.emplace(1, make_unique<int>(1));
    uncopy.erase(uncopy.begin());

    unordered_set<pair<int, char>> us;
    us.emplace(1, 'c');
    us.insert(pair<int, char>(4, 'r'));
    println(us);
    us.erase(pair<int, char>(4, 'r'));
    us.erase(us.begin());
    println(us);

    unordered_multiset<pair<int, const char*>> ms;
    ms.emplace(1, "234");
    ms.insert(make_pair(2, "345"));
    ms.emplace(1, "234");
    println(ms);
    ms.erase(ms.begin());
    println(ms);

    unordered_set<float> fus;
    fus.insert(1.5);
    fus.insert(2.5);
    fus.insert(3.5);
    fus.insert(1.5);
    println(fus);

    unordered_multiset<float> fus2;
    fus2.insert(1.5);
    fus2.insert(2.5);
    fus2.insert(3.5);
    fus2.insert(1.5);
    string fus2_str = to_string(fus2);
    println(fus2_str);
}

void test_cache() {
    lru_cache<int, string> lru(2);
    lru.put(1, "one");
    lru.put(2, "two");
    println(lru.get(1).value_or("(null)"));
    lru.put(3, "three");
    println(lru.get(2).value_or("(null)"));

    ttl_cache<int, string> ttl(3, seconds(5));
    ttl.put(1, "one");
    ttl.put(2, "two", seconds(1));
    this_thread::sleep_for(milliseconds(1100));
    println(ttl.get(1).value_or("(null)"));
    println(ttl.get(2).value_or("(null)"));
    ttl.cleanup();
}

void test_math() {
    try {
        println(power(2, 10));
        println(power(3, 10));
        println(factorial(10));
        println(sine(1));
        println(cosine(angular2radian(270)));
        println(remainder(73.263, 0.9973));
        println(float_part(constants::PI));
        println(exponential(3));
        println(logarithm_e(165.f));
        println(logarithm_10(147.f));
        println(logarithm_2(500.f));
        println(arctangent(100));
        println(radian2angular(arctangent(100)));
        println(arcsine(1), arcsine(0), arcsine(-1));
        println(arccosine(1), arccosine(0), arccosine(-1));
        println(arctangent(numeric_traits<decimal_t>::max()), arctangent(numeric_traits<decimal_t>::min_nega()));
        // println(tangent(constants::PI / 2));  // MathError
        println(tangent(0));
        println(around_pi(constants::PI), ":", around_pi(6.28));
    } catch (const exception& e) {
        println(e.what());
    }
}

void test_sort() {
    vector<int> vec{ 6,9,1,5,8,4,7 };
    //insertion_sort(vec.begin(), vec.end());
    //bubble_sort(vec.begin(), vec.end());
    //select_sort(vec.begin(), vec.end());
    //shell_sort(vec.begin(), vec.end());
    //partial_sort(vec.begin(), vec.end(), vec.end());
    //counting_sort(vec.begin(), vec.end());
    //sort(vec.begin(), vec.end());
    //introspective_sort(vec.begin(), vec.end(), (size_t)logarithm_2(vec.end() - vec.begin()) * 2);
    //quick_sort(vec.begin(), vec.end());
    //merge_sort(vec.begin(), vec.end());
    //bucket_sort(vec.begin(), vec.end());
    radix_sort(vec.begin(), vec.end());
    //tim_sort(vec.begin(), vec.end());
    //monkey_sort(vec.begin(), vec.end());
    //smooth_sort(vec.begin(), vec.end());
    //cocktail_sort(vec.begin(), vec.end());
    println(vec);
    //vector<Person> people = {
    //{"Alice", 25},
    //{"Bob", 20},
    //{"Charlie", 30},
    //{"David", 20}
    //};
    //counting_sort(people.begin(), people.end(),
    //    [](const Person& a, const Person& b) -> bool { return a.age < b.age; },
    //    [](const Person& p) -> int { return p.age; });
    //radix_sort_greater(people.begin(), people.end(), [](const Person& x) -> int { return x.age; });
    //println(people);
}

struct var_visitor {
    int operator ()(int arg) const { return arg * 2; }
    int operator ()(const string& arg) const { return arg.length(); }
};

void test_variant() {
    variant<int, string> v1;
    println(v1.index());
    variant<int, string> v2;
    v2.emplace<1>("hello");
    println(v2.index());

    auto& str = v2.get<string>();
    println(str);
    v2.emplace<int>(42);
    println(v2.index(), ":", v2.get<int>());
    int result = v2.visit(var_visitor());
    println(result);

    hash<variant<int, string>> hasher{};
    println(hasher(v1));

    variant<none_t, int> v = none;

    if (v.holds_alternative<none_t>()) {
        println("hold none");
    }
    println(v.to_hash());
}


string generate_random_string(size_t length) {
    constexpr string_view chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    string s;
    s.reserve(length);
    random_lcd rand;
    for (size_t i = 0; i < length; ++i) {
        s += chars[rand.next_int<int>() % chars.size()];
    }
    return s;
}

void test_short_strings(size_t count, size_t length) {
    vector<string> strings;
    strings.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        if (i % 10000 == 0)
            print(i, " ");
        strings.emplace_back(generate_random_string(length));
    }

    strings.clear();
    strings.shrink_to_fit();

    println("Test 1:", count, "short strings (", length, " chars)");
}

void test_long_string_concat(size_t iterations, size_t chunk_size) {
    string long_str;
    long_str.reserve(iterations * chunk_size);

    for (size_t i = 0; i < iterations; ++i) {
        long_str += generate_random_string(chunk_size);
    }

    println("Test 2: Long string concat (", iterations, "chunks,"
              , chunk_size, "chars each, total: ", long_str.size(), " chars)");
}

void test_string_modification(size_t initial_length, size_t operations) {
    string str = generate_random_string(initial_length);
    random_lcd rand;
    for (size_t i = 0; i < operations; ++i) {
        if (i % 2 == 0) {
            size_t pos = rand.next_int<int>() % (str.size() + 1);
            str.insert(pos, 1, 'X');
        } else {
            if (str.empty()) break;
            size_t pos = rand.next_int<int>() % str.size();
            str.erase(pos, 1);
        }
    }

    println("Test 3: ", operations, " modify operations (initial: "
              , initial_length, " chars, final: ", str.size(), " chars)");
}

void test_string_search_replace(size_t str_length, size_t pattern_count) {
    string str = generate_random_string(str_length);
    const string pattern = "ABC";
    const string replacement = "XYZ";
    random_lcd rand;
    for (size_t i = 0; i < pattern_count; ++i) {
        size_t pos = rand.next_int<int>() % (str.size() - pattern.size() + 1);
        str.replace(pos, pattern.size(), pattern);
    }

    size_t replace_count = 0;
    size_t pos = 0;
    while ((pos = str.find(pattern, pos)) != string::npos) {
        str.replace(pos, pattern.size(), replacement);
        pos += replacement.size();
        replace_count++;
    }

    println("Test 4: Search & replace (str length: ", str_length
              , ", patterns found: ", replace_count, ")");
}

void test_max_memory_string() {
    try {
        size_t available_memory = sysinfo::instance()
            .get_memory_info().available_memory();
        size_t max_test_size = available_memory / 2;

        const size_t upper_limit =
#ifdef NEFORCE_ARCH_BITS_64
            4ULL * 1024 * 1024 * 1024;  // 4GB
#else
            1ULL * 1024 * 1024 * 1024;  // 1GB
#endif
        max_test_size = min(max_test_size, upper_limit);

        if (max_test_size == 0) {
            NEFORCE_THROW_EXCEPTION(memory_exception("Insufficient system memory for test."));
        }

        string huge_str;
        huge_str.reserve(max_test_size);

        size_t chunk = 1024 * 1024;
        size_t total_written = 0;

        println("Testing max memory string (target size: "
                  , max_test_size / (1024 * 1024), " MB)");

        while (total_written < max_test_size) {
            size_t write = min(chunk, max_test_size - total_written);
            huge_str.append(write, 'A');
            total_written += write;

            if (total_written % (100 * 1024 * 1024) == 0) {
                println("Allocated ", total_written / (1024 * 1024), " MB...");
            }
        }
        println("Test 5: Success. Allocated "
                  , total_written / (1024 * 1024), " MB string.");
    } catch (const exception& e) {
        println("Test 5: ", e.what());
    }
}

void test_string() {
    test_short_strings(1000000, 32);
    test_long_string_concat(100000, 1024);
    test_string_modification(100000, 500000);
    test_string_search_replace(1000000, 10000);
    test_max_memory_string();

    const string result = to_string("a", 'b', 333, 9.333, "hello", false);
    println(result);
}


void test_option() {
    optional<int> opt1{0};
    println(opt1.value());

    optional<int> opt2(none);
    try {
        println(opt2.value());
    } catch (const exception& e) {
        println(e.what());
    }

    optional<int> opt3(42);
    println(opt3.value());

    opt1 = 100;
    println(opt1.value());

    optional<int> opt4(opt3);
    println(opt4.value());

    opt2 = opt3;
    println(opt2.value());

    optional<string> opt5(inplace_construct_tag{}, "Hello, World!");
    println(opt5.value());

    opt1.emplace(200);
    println(opt1.value());

    opt1.reset();
    try {
        println(opt1.value());
    } catch (const exception& e) {
        println(e.what());
    }

    if (opt3.has_value()) {
        println("opt3 has a value.");
    } else {
        println("opt3 has no value.");
    }

    int default_val = opt1.value_or(300);
    println("Value of opt1 or default: ", default_val);

    auto result1 = opt1.or_else([]() { return optional<int>(400); });
    println(result1.value());

    auto result2 = opt3.and_then([](int x) { return optional<int>(x * 2); });
    println(result2.value());

    auto result3 = opt3.transform([](int x) { return x + 1; });
    println(result3.value());
}

void test_st(){
    trace_allocator<int> alloc;
    auto* ptr = alloc.allocate();
}

void test_any() {
    any a1;
    println("Testing default constructor:");
    println("Has value: ", (a1.has_value() ? "Yes" : "No"));

    any a2(42);
    println("\nTesting constructor with value:");
    println("Has value: ", (a2.has_value() ? "Yes" : "No"));
    const int* ptr = any_cast<int>(&a2);
    if (ptr) {
        println("Value: ", *ptr);
    }

    any a3(a2);
    println("\nTesting copy constructor:");
    println("Has value: ", (a3.has_value() ? "Yes" : "No"));
    ptr = any_cast<int>(&a3);
    if (ptr) {
        println("Value: ", *ptr);
    }

    any a4(any(123));
    println("\nTesting move constructor:");
    println("Has value: ", (a4.has_value() ? "Yes" : "No"));
    ptr = any_cast<int>(&a4);
    if (ptr) {
        println("Value: ", *ptr);
    }

    a1 = a4;
    println("\nTesting assignment operator:");
    println("Has value: ", (a1.has_value() ? "Yes" : "No"));
    ptr = any_cast<int>(&a1);
    if (ptr) {
        println("Value: ", *ptr);
    }

    string str = "Hello, World!";
    string result = a1.emplace<string>(str);
    println("\nTesting emplace method:");
    println(result);
    println("Has value: ", (a1.has_value() ? "Yes" : "No"));
    const string* strPtr = any_cast<string>(&a1);
    if (strPtr) {
        println("Value: ", *strPtr);
    }

    a1.reset();
    println("\nTesting reset method:");
    println("Has value: ", (a1.has_value() ? "Yes" : "No"));

    a1 = 10;
    a2 = 20;
    a1.swap(a2);
    println("a1: ", *any_cast<int>(&a1));
    println("a2: ", *any_cast<int>(&a2));
}
