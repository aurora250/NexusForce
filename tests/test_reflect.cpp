#include "test.h"

class shape {
public:
    string name = "shape";
    double x = 0.0;
    double y = 0.0;

    virtual ~shape() = default;

    void move(double dx, double dy) {
        x += dx;
        y += dy;
    }

    virtual double area() const { return 0.0; }

    string position() const { return "(" + to_string(x) + ", " + to_string(y) + ")"; }
};

class circle : public shape {
public:
    double radius = 1.0;

    circle() = default;
    circle(double r, double px, double py) :
    radius(r) {
        x = px;
        y = py;
        name = "circle";
    }

    double area() const override { return 3.14159 * radius * radius; }

    double circumference() const { return 2.0 * 3.14159 * radius; }

    void set_radius(double r) { radius = r; }

    void scale(double factor_x, double factor_y) { radius *= square_root(factor_x * factor_y); }
};

class rectangle : public shape {
public:
    double width = 1.0;
    double height = 1.0;

    rectangle() = default;
    rectangle(double w, double h, double px, double py) :
    width(w),
    height(h) {
        x = px;
        y = py;
        name = "rectangle";
    }

    double area() const override { return width * height; }

    double perimeter() const { return 2.0 * (width + height); }
};


template <> struct neforce::reflect::type_name<shape> {
    static constexpr string_view value = "shape";
};

template <> struct neforce::reflect::type_name<circle> {
    static constexpr string_view value = "circle";
};

template <> struct neforce::reflect::type_name<rectangle> {
    static constexpr string_view value = "rectangle";
};


NEFORCE_REFLECT_REGISTER(shape)
        .constructor()
        .property("name", &shape::name)
        .property("x", &shape::x)
        .property("y", &shape::y)
        .function("move", &shape::move)
        .function("area", &shape::area)
        .function("position", &shape::position);

NEFORCE_REFLECT_REGISTER_DERIVED(circle, shape)
        .constructor()
        // .constructor<double, double, double>()
        .property("radius", &circle::radius)
        .function("area", &circle::area)
        .function("circumference", &circle::circumference)
        .function("set_radius", &circle::set_radius)
        .function("scale", &circle::scale);

NEFORCE_REFLECT_REGISTER_DERIVED(rectangle, shape)
        .constructor()
        .property("width", &rectangle::width)
        .property("height", &rectangle::height)
        .function("area", &rectangle::area)
        .function("perimeter", &rectangle::perimeter);


void test_reflect() {
    auto* circle_meta = reflect::registry::instance().find("circle");

    println("circle is derived from shape: ", circle_meta->is_derived_from("shape"));

    reflect::meta_any obj = circle_meta->create();
    circle* c = obj.cast<circle>();

    if (auto* func = circle_meta->get_function("move")) {
        vector<reflect::meta_any> args = {reflect::meta_any(10.0), reflect::meta_any(20.0)};
        func->invoke(c, args);
        println("After move:", c->position());
    }

    if (auto* func = circle_meta->get_function("scale")) {
        vector<reflect::meta_any> args = {reflect::meta_any(2.0), reflect::meta_any(0.5)};
        func->invoke(c, args);
        println("After scale: radius =", c->radius);
    }

    if (auto* prop = circle_meta->get_property("name")) {
        auto val = prop->get(c);
        if (auto* s = val.cast<string>()) {
            println("shape name:", *s);
        }
    }

    println("\nAll properties of circle (including base):");
    for (const auto& property: circle_meta->all_properties()) {
        println("  -", property.first);
    }

    println("\nAll functions of circle (including base):");
    for (const auto& f: circle_meta->all_functions()) {
        println("  -", f.first);
    }

    auto* rect_meta = reflect::registry::instance().find("rectangle");
    reflect::meta_any rect_obj = rect_meta->create();
    rectangle* r = rect_obj.cast<rectangle>();

    r->width = 10.0;
    r->height = 5.0;

    if (auto* func = rect_meta->get_function("move")) {
        vector<reflect::meta_any> args = {reflect::meta_any(100.0), reflect::meta_any(200.0)};
        func->invoke(r, args);
        println("rectangle position:", r->position());
    }

    println("rectangle area:", r->area());
    println("rectangle perimeter:", r->perimeter());
}
