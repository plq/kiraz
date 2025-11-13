
#include <regex>

#include <gtest/gtest.h>

#include <lexer.hpp>
#include <main.h>

#include <kiraz/Compiler.h>
#include <kiraz/Node.h>

extern int yydebug;

namespace kiraz {

struct CompilerFixture : public ::testing::Test {
    void SetUp() override {}
    void TearDown() override {}

    /**
     * @brief verify_first: Verifies the AST of the given module.
     * @param code: Kiraz source code, as a string.
     * @param ast:  Expected AST for the given code.
     */
    void verify_root(const std::string &code, const std::string &ast) {
        Compiler compiler;

        std::stringstream ostr;

        /* perform */
        compiler.compile_string(code);

        /* verify */
        if (! Node::get_root_before()) {
            fmt::print("{}\n", compiler.get_error());
        }

        ASSERT_TRUE(Node::get_root_before());
        auto root = Node::get_root_before();
        ASSERT_EQ(FF("{}", *root), ast);
    }

    void verify_ok(const std::string &code) {
        Compiler compiler;

        std::stringstream ostr;

        /* perform */
        compiler.compile_string(code);

        /* verify */
        if (! Node::get_root_before()) {
            fmt::print("{}\n", compiler.get_error());
        }

        ASSERT_TRUE(Node::get_root_before());
    }

    /**
     * @brief verify_first: Verifies the AST of the first statement in the given
     * module.
     * @param code: Kiraz source code, as a string.
     * @param ast:  Expected AST for the given code.
     */
    void verify_first(const std::string &code, const std::string &ast) {
        Compiler compiler;

        std::stringstream ostr;

        /* perform */
        compiler.compile_string(code);

        /* verify */
        if (! Node::get_root_before()) {
            fmt::print("{}\n", compiler.get_error());
        }

        ASSERT_TRUE(Node::get_root_before());
        ASSERT_TRUE(Node::get_first_before());
        auto first = Node::get_first_before();
        ASSERT_EQ(FF("{}", *first), ast);
    }

    void verify_error(const std::string &code) {
        Compiler compiler;

        std::stringstream ostr;

        /* perform */
        compiler.compile_string(code);

        /* verify */
        if (Node::get_root_before()) {
            fmt::print("ERR?: {}\n", *Node::get_root_before());
        }

        ASSERT_FALSE(Node::get_root_before());
    }

    void verify_error(const std::string &code, const std::string &err) {
        Compiler compiler;

        std::stringstream ostr;

        /* perform */
        compiler.compile_string(code);

        /* verify */
        if (Node::get_root_before()) {
            fmt::print("ERR?: {}\n", *Node::get_root_before());
        }

        ASSERT_FALSE(Node::get_root_before());
        ASSERT_EQ(std::regex_replace(compiler.get_error(),
                          std::regex("^Error at [0-9]+:[0-9]+: (.*)\n$"), "$1"),
                err);
    }
};

TEST_F(CompilerFixture, class_subsymbol) {
    verify_ok("class C { let i = 0; }; func f() : Null { let c: C; c.i;};");
}

TEST_F(CompilerFixture, class_subsymbol_missing) {
    verify_error("class C { let i = 0; }; func f():Null{ let c: C; c.j;};",
            "Identifier 'c.j' is not found");
}

TEST_F(CompilerFixture, import_missing) {
    verify_error("func f() : Null { io.foo.bar();\n };", "Identifier 'io' is not found");
}

TEST_F(CompilerFixture, func_rettype_missing) {
    verify_error("func f() : R { let a = 5; return a + b; };",
            "Return type 'R' of function 'f' is not found");
}

TEST_F(CompilerFixture, func_argtype_missing) {
    verify_error("func f(a: A) : Null { };",
            "Identifier 'A' in type of argument 'a' in function 'f' is not found");
}

TEST_F(CompilerFixture, func_arg_dup) {
    verify_error("func f(a: Integer64, a: Integer64) : Null { };",
            "Identifier 'a' in argument list of function 'f' is already in symtab");
}

TEST_F(CompilerFixture, func_arg_name_dup) {
    verify_error("func f(f: Integer64) : Null { };",
            "Identifier 'f' in argument list of function 'f' is already in symtab");
}

TEST_F(CompilerFixture, func_adjacent) {
    verify_ok("func f(a: Integer64) : Null { };"
              "func g(a: Integer64) : Null { };");
}

TEST_F(CompilerFixture, var_missing) {
    verify_error("func f() : Null { a; };", "Identifier 'a' is not found");
}

TEST_F(CompilerFixture, var_found) {
    verify_ok("func f() : Null { let a = 5; a; };");
}

TEST_F(CompilerFixture, var_scoped_conflict) {
    verify_error(
            "let a = 5; func f() : Null { let a = 5; };", "Identifier 'a' is already in symtab");
}

TEST_F(CompilerFixture, var_scoped) {
    verify_ok("func f() : Null { let a = 5; }; let a = 5;");
}

TEST_F(CompilerFixture, class_scope) {
    verify_ok("class A { let a = 5; };");
}

TEST_F(CompilerFixture, class_scope_var_var_conflict) {
    verify_error("class A { let a = 5; let a = 5; };", "Identifier 'a' is already in symtab");
}

TEST_F(CompilerFixture, class_scope_var_func_conflict) {
    verify_error(
            "class A { let a = 5; func a() : Null {}; };", "Identifier 'a' is already in symtab");
}

TEST_F(CompilerFixture, func_name_conflict) {
    verify_error("class A { }; func A() :Null {};", "Identifier 'A' is already in symtab");
}

TEST_F(CompilerFixture, op_add_int) {
    verify_ok("func f() : Null { let a = 5; let b = 10; let c = a + b; };");
}

TEST_F(CompilerFixture, op_add_str) {
    verify_ok(R"(func f() : Null { let a = "5"; let b = "10"; let c = a + b; };)");
}

TEST_F(CompilerFixture, op_add_type_mismatch) {
    verify_error(R"(func f() : Null { let a = 5; let b = "10"; let c = a + b; };)",
            "Operator '+' not defined for types 'Integer64' and 'String'");
}

TEST_F(CompilerFixture, class_method_conflict) {
    verify_error("class A { func A() :Null {}; };", "Identifier 'A' is already in symtab");
}

TEST_F(CompilerFixture, class_member_conflict) {
    verify_error("class A { let A: Integer64; };", "Identifier 'A' is already in symtab");
}

TEST_F(CompilerFixture, class_name_conflict) {
    verify_error("func C() :Null {}; class C { };", "Identifier 'C' is already in symtab");
}

TEST_F(CompilerFixture, class_scope_var_func_parent_scope) {
    verify_ok("class A { let a = 5; func f() : Null { a; }; };");
}

TEST_F(CompilerFixture, class_scoped_func_conflict) {
    verify_error("class A { let a = 5; func f() : Null { let a = 5; }; };",
            "Identifier 'a' is already in symtab");
}

TEST_F(CompilerFixture, class_use_before_definition) {
    verify_ok("let a : A; class A { };");
}

TEST_F(CompilerFixture, func_has_and) {
    verify_ok("func m() : Null { and(true, true); };");
}

TEST_F(CompilerFixture, func_has_or) {
    verify_ok("func m() : Null { or(false, false); };");
}

TEST_F(CompilerFixture, func_has_not) {
    verify_ok("func m() : Null { not(true); };");
}

TEST_F(CompilerFixture, func_has_Boolean) {
    verify_ok("func m() : Boolean { return true; };");
}

TEST_F(CompilerFixture, func_has_Integer64) {
    verify_ok("func m() : Integer64 { return 1; };");
}

TEST_F(CompilerFixture, func_has_String) {
    verify_ok("func m() : String { return \"s\"; };");
}

TEST_F(CompilerFixture, func_no_class_def_as_var_name) {
    verify_error("class f {};", "Class name 'f' can not start with an lowercase letter");
}

TEST_F(CompilerFixture, func_no_builtin_assignment_and) {
    verify_error("func m() : Null { and = or; };", "Overriding builtin 'and' is not allowed");
}

TEST_F(CompilerFixture, func_no_builtin_assignment_or) {
    verify_error("func m() : Null { or = and; };", "Overriding builtin 'or' is not allowed");
}

TEST_F(CompilerFixture, func_no_builtin_assignment_not) {
    verify_error("func m() : Null { not = not; };", "Overriding builtin 'not' is not allowed");
}

TEST_F(CompilerFixture, func_no_builtin_assignment_bool_00) {
    verify_error("func m() : Null { false = false; };");
}
TEST_F(CompilerFixture, func_no_builtin_assignment_bool_01) {
    verify_error("func m() : Null { false = true; };");
}
TEST_F(CompilerFixture, func_no_builtin_assignment_bool_10) {
    verify_error("func m() : Null { true = false; };");
}
TEST_F(CompilerFixture, func_no_builtin_assignment_bool_11) {
    verify_error("func m() : Null { true = true; };");
}

TEST_F(CompilerFixture, func_no_assignment_module_int) {
    verify_error("import io; func m() : Null { io=5; };",
            "Left type 'Module' of assignment does not match the right type 'Integer64'");
}

TEST_F(CompilerFixture, func_no_assignment_module_string) {
    verify_error("import io; func m() : Null { io=\"5\"; };",
            "Left type 'Module' of assignment does not match the right type 'String'");
}

TEST_F(CompilerFixture, func_no_assignment_module_module) {
    verify_error("import io; func m() : Null { io=io; };",
            "Overriding imported module 'io' is not allowed");
}

TEST_F(CompilerFixture, func_hello_world) {
    verify_ok(R"(import io; func main() : Null { io.print("Hello world!\n"); };)");
}

TEST_F(CompilerFixture, func_return) {
    verify_ok(R"(func f() : Integer64 { return 0; };)");
}

TEST_F(CompilerFixture, func_return_mismatch) {
    verify_error(R"(func f() : Integer64 { return "0"; };)",
            "Return statement type 'String' does not match function return "
            "type 'Integer64'");
}

TEST_F(CompilerFixture, func_return_missing) {
    verify_error(R"(func f() : Integer64 {};)",
            "Function is missing return value");
}

TEST_F(CompilerFixture, class_return_this) {
    verify_ok(R"(class C { let i = 0; func f() : C { i = i + 1; return this; };};)");
}

TEST_F(CompilerFixture, class_return_this_type_mismatch) {
    verify_error(R"(class C { func f() : Integer64 { return this; };};)", 
        "Return statement type 'C' does not match function return type 'Integer64'");
}

TEST_F(CompilerFixture, return_misplaced_module) {
    verify_error(R"(return a;)");
}

TEST_F(CompilerFixture, return_misplaced_class) {
    verify_error(R"(class A{return a;};)");
}

TEST_F(CompilerFixture, while_simple) {
    verify_ok(R"(func f() : Null {while(true) {};};)");
}

TEST_F(CompilerFixture, while_test_int) {
    verify_error(
            R"(func f() : Null {while(1) {};};)", "While only accepts tests of type 'Boolean'");
}

TEST_F(CompilerFixture, while_test_str) {
    verify_error(
            R"(func f() : Null {while("1") {};};)", "While only accepts tests of type 'Boolean'");
}

TEST_F(CompilerFixture, while_misplaced_module) {
    verify_error(R"(while(true) {};)");
}

TEST_F(CompilerFixture, while_misplaced_class) {
    verify_error(R"(class A{ while(true) {}; };)");
}

TEST_F(CompilerFixture, while_complex) {
    verify_ok(R"(class A{ let i = 0; func f():Null { while(i<5) {i = i + 1;}; };};)");
}

TEST_F(CompilerFixture, if_complex_lt) {
    verify_ok(R"(class A{ let i = 0; func f():Null { if(i<5) {i = i + 1;} else {}; }; };)");
}

TEST_F(CompilerFixture, if_complex_le) {
    verify_ok(R"(class A{ let i = 0; func f():Null { if(i<=5) {i = i + 1;} else {}; }; };)");
}

TEST_F(CompilerFixture, if_complex_ge) {
    verify_ok(R"(class A{ let i = 0; func f():Null { if(i>=5) {i = i + 1;} else {}; }; };)");
}

TEST_F(CompilerFixture, if_complex_gt) {
    verify_ok(R"(class A{ let i = 0; func f():Null { if(i>5) {i = i + 1;} else {}; }; };)");
}

TEST_F(CompilerFixture, if_complex_and) {
    verify_ok(
            "class A{ let i = 0; func f():Null { if(and(false, true)) {i = i + 1;} else {}; }; };");
}

TEST_F(CompilerFixture, if_complex_and_gt_lt) {
    verify_ok("class A{ let i = 0; func f():Null { if(and(i>5, i<10)) {i = i + 1;} else {}; }; };");
}

TEST_F(CompilerFixture, let_while) {
    verify_error(R"(auto let a = while(true) {};)");
}

TEST_F(CompilerFixture, let_if_the_infamous) {
    verify_error(R"(auto let a = if(true) {};)");
}

TEST_F(CompilerFixture, if_simple) {
    verify_ok(R"(func f():Null{if(true) {};};)");
}

TEST_F(CompilerFixture, if_misplaced_module) {
    verify_error(R"(if(true) {};)");
}

TEST_F(CompilerFixture, if_misplaced_class) {
    verify_error(R"(class A{if(true) {};};)");
}

TEST_F(CompilerFixture, assignment_misplaced_class) {
    verify_error(R"(let a=5; class A{a=6;};)");
}
TEST_F(CompilerFixture, div_misplaced_class) {
    verify_error(R"(let a=5; class A{a/6;};)");
}
TEST_F(CompilerFixture, mul_misplaced_class) {
    verify_error(R"(let a=5; class A{a*6;};)");
}
TEST_F(CompilerFixture, add_misplaced_class) {
    verify_error(R"(let a=5; class A{a+6;};)");
}
TEST_F(CompilerFixture, sub_misplaced_class) {
    verify_error(R"(let a=5; class A{a-6;};)");
}
TEST_F(CompilerFixture, lt_misplaced_class) {
    verify_error(R"(let a=5; class A{a<6;};)");
}
TEST_F(CompilerFixture, le_misplaced_class) {
    verify_error(R"(let a=5; class A{a<=6;};)");
}
TEST_F(CompilerFixture, gt_misplaced_class) {
    verify_error(R"(let a=5; class A{a>6;};)");
}
TEST_F(CompilerFixture, ge_misplaced_class) {
    verify_error(R"(let a=5; class A{a>=6;};)");
}

TEST_F(CompilerFixture, assignment_misplaced_module) {
    verify_error(R"(let a=5; {a=6;};)");
}
TEST_F(CompilerFixture, div_misplaced_module) {
    verify_error(R"(let a=5; {a/6;};)");
}
TEST_F(CompilerFixture, mul_misplaced_module) {
    verify_error(R"(let a=5; {a*6;};)");
}
TEST_F(CompilerFixture, add_misplaced_module) {
    verify_error(R"(let a=5; {a+6;};)");
}
TEST_F(CompilerFixture, sub_misplaced_module) {
    verify_error(R"(let a=5; {a-6;};)");
}
TEST_F(CompilerFixture, lt_misplaced_module) {
    verify_error(R"(let a=5; {a<6;};)");
}
TEST_F(CompilerFixture, le_misplaced_module) {
    verify_error(R"(let a=5; {a<=6;};)");
}
TEST_F(CompilerFixture, gt_misplaced_module) {
    verify_error(R"(let a=5; {a>6;};)");
}
TEST_F(CompilerFixture, ge_misplaced_module) {
    verify_error(R"(let a=5; {a>=6;};)");
}

TEST_F(CompilerFixture, anonscope_var_module) {
    verify_ok(R"({let a=5;}; {let a=5;};)");
}

TEST_F(CompilerFixture, anonscope_var_func) {
    verify_ok(R"(func f():Null{ {let a=5;}; {let a=5;}; };)");
}

TEST_F(CompilerFixture, class_scope_pure) {
    verify_error(R"(class A{}; func f():Null{let a:A; a.Integer64;};)",
            "Identifier 'a' has no subsymbol 'Integer64'");
}

TEST_F(CompilerFixture, if_test_int) {
    verify_error(R"(func f():Null{if(1) {};};)", "If only accepts tests of type 'Boolean'");
}

TEST_F(CompilerFixture, if_test_str) {
    verify_error(R"(func f():Null{if(1){};};)", "If only accepts tests of type 'Boolean'");
}

TEST_F(CompilerFixture, if_else_test_str) {
    verify_ok(R"(func f():Null{if(false){} else {};};)");
}

TEST_F(CompilerFixture, func_use_before_definition) {
    verify_ok(R"(import io;
        func say_hello() : Null {
            let h = get_hello();
        };
        func get_hello() : String {
            return "Hello, World!\n";
        };
        )");
}

TEST_F(CompilerFixture, assignment_type_mismatch_func) {
    verify_error(R"(
        func f() : Null {
            let h : Integer64;
            h = g();
        };
        func g() : String { };
        )",
            "Left type 'Integer64' of assignment does not match the right type 'String'");
}

TEST_F(CompilerFixture, let_type_mismatch_func) {
    verify_error(
            R"(
        func f() : Null {
            let h : Integer64 = g();
            h = g();
        };
        func g() : String { };
        )",
            "Initializer type 'String' does not match explicit type 'Integer64'");
}

TEST_F(CompilerFixture, let_type_mismatch_var) {
    verify_error(R"(func f() : Null { let h : Integer64 = "string"; };)",
            "Initializer type 'String' does not match explicit type 'Integer64'");
}

TEST_F(CompilerFixture, assignment_type_mismatch_var) {
    verify_error(R"(func f() : Null { let h : Integer64; h = "string"; };)",
            "Left type 'Integer64' of assignment does not match the right "
            "type 'String'");
}

TEST_F(CompilerFixture, func_missing) {
    verify_error("func say_hello() : Null { let h = get_hello(); };",
            "Identifier 'get_hello' is not found");
}

TEST_F(CompilerFixture, import_subsymbol_missing) {
    verify_error("import io; func f() : Null { io.missing(); };",
            "Identifier 'io.missing' is not found");
}

TEST_F(CompilerFixture, func_call_argnum_mismatch) {
    verify_error("import io; func f() : Null { io.print(); };",
            "Call to function 'io.print' has wrong number of arguments");
}

TEST_F(CompilerFixture, func_call_type_mismatch) {
    verify_error("func g(s: String): Null{ }; func f() : Null { g(42); };",
            "Argument 1 in call to function 'g' has type 'Integer64'"
            " which does not match definition type 'String'");
}

TEST_F(CompilerFixture, io_print_call_overload_int) {
    verify_ok("import io; func f() : Null { io.print(42); };");
}

TEST_F(CompilerFixture, io_print_call_overload_bool) {
    verify_ok("import io; func f() : Null { io.print(true); };");
}

TEST_F(CompilerFixture, io_print_call_overload_custom) {
    verify_error("import io; class C {}; func f() : Null { let c: C; io.print(c); };");
}

} // namespace kiraz
