
#include <gtest/gtest.h>

#include <lexer.hpp>
#include <main.h>

#include <kiraz/Node.h>

extern std::shared_ptr<Token> curtoken;

struct ParserFixture : public testing::Test {
    YY_BUFFER_STATE buffer = nullptr;

    void SetUp() override {
        Node::reset_root();
        // yydebug = 1; // uncomment to your heart's content
    }

    void TearDown() override {
        // Tear down code after each test, even if assertions fail.
        // This will be executed even in the face of assertion failures.
        if (buffer) {
            yy_delete_buffer(buffer);
            buffer = nullptr;
        }

        yydebug = 0;
        Token::colno = 1;
        curtoken.reset();
    }

    void verify_root(const std::string &code, const std::string &ast) {
        buffer = yy_scan_string(code.data());

        /* perform */
        yyparse();

        /* verify */
        ASSERT_TRUE(Node::current_root());
        auto root = Node::current_root();
        ASSERT_EQ(FF("{}", root->as_string()), ast);
    }

    void verify_single(const std::string &code, const std::string &ast) {
        buffer = yy_scan_string(code.data());

        /* perform */
        yyparse();

        /* verify */
        ASSERT_TRUE(Node::current_root());
        auto root = Node::current_root();
        auto root_ast = root->as_string();
        ASSERT_FALSE(root_ast.empty());
        ASSERT_TRUE(root_ast.starts_with("Module(["));
        ASSERT_TRUE(root_ast.ends_with("])"));
        ASSERT_EQ(root_ast.substr(8, root_ast.size() - 10), ast);
    }

    void verify_no_root(const std::string &code) {
        buffer = yy_scan_string(code.data());

        /* perform */
        yyparse();

        /* verify */
        ASSERT_FALSE(Node::current_root());
    }
};

TEST_F(ParserFixture, empty) {
    verify_no_root("");
}

TEST_F(ParserFixture, invalid_char) {
    verify_no_root("=");
}

TEST_F(ParserFixture, plus) {
    verify_single("1+2;", "Add(l=Int(1), r=Int(2))");
}

TEST_F(ParserFixture, plus_s) {
    verify_single("1+2;", "Add(l=Int(1), r=Int(2))");
}

TEST_F(ParserFixture, plus_sb) {
    verify_single("1 +2;", "Add(l=Int(1), r=Int(2))");
}

TEST_F(ParserFixture, plus_ss) {
    verify_single("1 + 2;", "Add(l=Int(1), r=Int(2))");
}

TEST_F(ParserFixture, plus_sa) {
    verify_single("1+ 2;", "Add(l=Int(1), r=Int(2))");
}

TEST_F(ParserFixture, plus_n) {
    verify_single("1+\n2;", "Add(l=Int(1), r=Int(2))");
}

TEST_F(ParserFixture, plus_t) {
    verify_single("1+\t2;", "Add(l=Int(1), r=Int(2))");
}

TEST_F(ParserFixture, minus) {
    verify_single("1-2;", "Sub(l=Int(1), r=Int(2))");
}

TEST_F(ParserFixture, mult) {
    verify_single("1*2;", "Mult(l=Int(1), r=Int(2))");
}

TEST_F(ParserFixture, divf) {
    verify_single("1/2;", "DivF(l=Int(1), r=Int(2))");
}

TEST_F(ParserFixture, muladd) {
    verify_single("1+2*3;", "Add(l=Int(1), r=Mult(l=Int(2), r=Int(3)))");
}

TEST_F(ParserFixture, muladd_paren) {
    verify_single("1+(2*3);", "Add(l=Int(1), r=Mult(l=Int(2), r=Int(3)))");
}

TEST_F(ParserFixture, addmul_paren) {
    verify_single("(1+2)*3;", "Mult(l=Add(l=Int(1), r=Int(2)), r=Int(3))");
}

TEST_F(ParserFixture, string) {
    verify_single(R"("a";)", "Str(a)");
}

TEST_F(ParserFixture, string_with_newline) {
    verify_single(" \"a\\nb\"; ", "Str(a\nb)");
}

TEST_F(ParserFixture, add_signed) {
    verify_single("1 + -2;", "Add(l=Int(1), r=Signed(OP_MINUS, Int(2)))");
}

TEST_F(ParserFixture, signed_add) {
    verify_single("-1 + 2;", "Add(l=Signed(OP_MINUS, Int(1)), r=Int(2))");
}

TEST_F(ParserFixture, signed_sub_chain) {
    verify_single("7 - 2 - 1;", "Sub(l=Sub(l=Int(7), r=Int(2)), r=Int(1))");
}

TEST_F(ParserFixture, add_mul) {
    verify_single("1 + 2 * 3;",
            "Add("
            "l=Int(1), "
            "r=Mult(l=Int(2), r=Int(3))"
            ")");
}

TEST_F(ParserFixture, mul_add) {
    verify_single("1 * 2 + 3;",
            "Add("
            "l=Mult(l=Int(1), r=Int(2)), "
            "r=Int(3)"
            ")");
}

TEST_F(ParserFixture, add__paren_mul) {
    verify_single("1 + (2 * 3);",
            "Add("
            "l=Int(1), "
            "r=Mult(l=Int(2), r=Int(3))"
            ")");
}

TEST_F(ParserFixture, paren_mul__add) {
    verify_single("(1 * 2) + 3;",
            "Add("
            "l=Mult(l=Int(1), r=Int(2)), "
            "r=Int(3)"
            ")");
}

TEST_F(ParserFixture, mul__paren_add) {
    verify_single("1 * (2 + 3);",
            "Mult("
            "l=Int(1), "
            "r=Add(l=Int(2), r=Int(3))"
            ")");
}

TEST_F(ParserFixture, paren_add__mul) {
    verify_single("(1 + 2) * 3;",
            "Mult("
            "l=Add(l=Int(1), r=Int(2)), "
            "r=Int(3)"
            ")");
}

TEST_F(ParserFixture, neg_paren_add__mul) {
    verify_single("-(1 + 2) * 3;",
            "Mult("
            "l=Signed(OP_MINUS, Add(l=Int(1), r=Int(2))), "
            "r=Int(3)"
            ")");
}

TEST_F(ParserFixture, paren_add__mul__paren_add) {
    verify_single("(1 + 2) * (3 + 4);",
            "Mult("
            "l=Add(l=Int(1), r=Int(2)), "
            "r=Add(l=Int(3), r=Int(4))"
            ")");
}

TEST_F(ParserFixture, paren_mul__sub__paren_mul) {
    verify_single("(1 * 2) - (3 * 4);",
            "Sub("
            "l=Mult(l=Int(1), r=Int(2)), "
            "r=Mult(l=Int(3), r=Int(4))"
            ")");
}

TEST_F(ParserFixture, paren_mul__sub__paren_mul__with_iden) {
    verify_single("(a * b) - (c * d);", "Sub(l=Mult(l=Id(a), r=Id(b)), r=Mult(l=Id(c), r=Id(d)))");
}

TEST_F(ParserFixture, eq) {
    verify_single("a == b;", "OpEq(l=Id(a), r=Id(b))");
}

TEST_F(ParserFixture, gt) {
    verify_single("a > b;", "OpGt(l=Id(a), r=Id(b))");
}

TEST_F(ParserFixture, ge) {
    verify_single("a >= b;", "OpGe(l=Id(a), r=Id(b))");
}

TEST_F(ParserFixture, lt) {
    verify_single("a < b; ", "OpLt(l=Id(a), r=Id(b))");
}

TEST_F(ParserFixture, le) {
    verify_single("a <= b;", "OpLe(l=Id(a), r=Id(b))");
}

TEST_F(ParserFixture, assignment) {
    verify_single("a = 5;", "Assign(l=Id(a), r=Int(5))");
}

TEST_F(ParserFixture, assignment_iden) {
    verify_single("a = b + 5;", "Assign(l=Id(a), r=Add(l=Id(b), r=Int(5)))");
}

TEST_F(ParserFixture, assignment_iden_neg_2) {
    verify_single("a = -(b / 5);", "Assign(l=Id(a), r=Signed(OP_MINUS, DivF(l=Id(b), r=Int(5))))");
}

TEST_F(ParserFixture, assignment_iden_neg_3) {
    verify_single("a = -(b / 5) + 3;",
            "Assign("
            "l=Id(a), "
            "r=Add(l=Signed(OP_MINUS, DivF(l=Id(b), r=Int(5))), r=Int(3))"
            ")");
}

TEST_F(ParserFixture, let_with_val) {
    verify_single("let a = 5;", "Let(n=Id(a), i=Int(5))");
}

TEST_F(ParserFixture, let_with_type) {
    verify_single("let a : Int64;", "Let(n=Id(a), t=Id(Int64))");
}

TEST_F(ParserFixture, let_with_type_val) {
    verify_single("let a : Int64 = 5;", "Let(n=Id(a), t=Id(Int64), i=Int(5))");
}

TEST_F(ParserFixture, let_invalid) {
    verify_no_root("let a;");
}

TEST_F(ParserFixture, let_valid_assignment) {
    verify_single("let a:Int64 = null;", "Let(n=Id(a), t=Id(Int64), i=Id(null))");
}

TEST_F(ParserFixture, let_with_stmt) {
    verify_single("let a = 2 * 3;", "Let(n=Id(a), i=Mult(l=Int(2), r=Int(3)))");
}

TEST_F(ParserFixture, let_with_stmt_invalid) {
    verify_single("let a = 2 * 3;", "Let(n=Id(a), i=Mult(l=Int(2), r=Int(3)))");
}

TEST_F(ParserFixture, func) {
    verify_single("func f() : T {};", "Func(n=Id(f), a=[], r=Id(T), s=[])");
}

TEST_F(ParserFixture, func_args) {
    verify_single("func f(a1 : A1) : T {};",
            "Func(n=Id(f), a=FuncArgs([FArg(n=Id(a1), t=Id(A1))]), r=Id(T), s=[])");
}

TEST_F(ParserFixture, func_args_stmts) {
    verify_single("func f(a1 : A1) : T { let v : A1 = 5; };",
            "Func("
            "n=Id(f), "
            "a=FuncArgs([FArg(n=Id(a1), t=Id(A1))]), "
            "r=Id(T), "
            "s=[Let(n=Id(v), t=Id(A1), i=Int(5))]"
            ")");
}

TEST_F(ParserFixture, class) {
    verify_single("class A {};", "Class(n=Id(A), s=[])");
}

TEST_F(ParserFixture, class_method) {
    // clang-format off
    verify_single("class A { func f(b: I, c: I) : B { a = b + c; }; };",
            "Class("
                "n=Id(A), "
                "s=["
                    "Func("
                        "n=Id(f), "
                        "a=FuncArgs("
                            "["
                                "FArg(n=Id(b), t=Id(I)), "
                                "FArg(n=Id(c), t=Id(I))"
                            "]"
                        "), "
                        "r=Id(B), "
                        "s=["
                            "Assign("
                                "l=Id(a), "
                                "r=Add(l=Id(b), r=Id(c))"
                            ")"
                        "]"
                    ")"
                "]"
            ")"
        );
    // clang-format on
}

TEST_F(ParserFixture, class_attr_and_method) {
    verify_single("class A { let a = 5; func f() : B { }; };",
            "Class(n=Id(A), s=[Let(n=Id(a), i=Int(5)), "
            "Func(n=Id(f), a=[], r=Id(B), s=[])"
            "])");
}

TEST_F(ParserFixture, import) {
    verify_single("import a;", "Import(Id(a))");
}

TEST_F(ParserFixture, call) {
    verify_single("a.b.c(d,e);",
            "Call(n=Dot(l=Dot(l=Id(a), r=Id(b)), r=Id(c)), a=FuncArgs([Id(d), Id(e)]))");
}

TEST_F(ParserFixture, module) {
    verify_root("import a; class B {};", "Module([Import(Id(a)), Class(n=Id(B), s=[])])");
}

TEST_F(ParserFixture, if_then_empty) {
    verify_single("if (a) {};", "If(?=Id(a), then=[], else=[])");
}

TEST_F(ParserFixture, if_then_stuff) {
    verify_single("if (a) { a; b; c; };", "If(?=Id(a), then=[Id(a), Id(b), Id(c)], else=[])");
}

TEST_F(ParserFixture, if_then_empty_else_empty) {
    verify_single("if (a) { } else { };", "If(?=Id(a), then=[], else=[])");
}

TEST_F(ParserFixture, if_then_stuff_else_empty) {
    verify_single("if (a) { s; } else { };", "If(?=Id(a), then=[Id(s)], else=[])");
}

TEST_F(ParserFixture, if_then_empty_else_stuff) {
    verify_single("if (a) { } else { s; };", "If(?=Id(a), then=[], else=[Id(s)])");
}

TEST_F(ParserFixture, if_then_stuff_else_stuff) {
    verify_single("if (a) { s; } else { s; };", "If(?=Id(a), then=[Id(s)], else=[Id(s)])");
}

TEST_F(ParserFixture, if_then_empty_else_nested_if) {
    verify_single("if (a) { } else { s1; if (b) { s2; } else { s3; }; s4; };",
            "If(?=Id(a), then=[], else=["
            "Id(s1), "
            "If(?=Id(b), then=[Id(s2)], else=[Id(s3)]), "
            "Id(s4)"
            "])");
}

TEST_F(ParserFixture, if_then_stuff_else_if_stuff_else_stuff) {
    verify_single("if (a) { s1; } else if (b) { s2; } else { s3; };",
            "If(?=Id(a), then=[Id(s1)], else=If(?=Id(b), then=[Id(s2)], else=[Id(s3)]))");
}

TEST_F(ParserFixture, while_repeat_empty) {
    verify_single("while (a) { };", "While(?=Id(a), repeat=[])");
}

TEST_F(ParserFixture, while_repeat_stuff) {
    verify_single("while (a) { s; };", "While(?=Id(a), repeat=[Id(s)])");
}

TEST_F(ParserFixture, while_literal) {
    verify_single("while (a > b) {};", "While(?=OpGt(l=Id(a), r=Id(b)), repeat=[])");
}

TEST_F(ParserFixture, while_bad_operand) {
    verify_no_root("while (let a : b) {};");
}

TEST_F(ParserFixture, class_bad_name) {
    verify_no_root("class a.b { };");
}

TEST_F(ParserFixture, func_bad_name) {
    verify_no_root("func f.g() : Null { };");
}

TEST_F(ParserFixture, let_bad_name) {
    verify_no_root("let f.g = a;");
}

TEST_F(ParserFixture, let_bad_initializer) {
    verify_no_root("let a = return b;");
}

TEST_F(ParserFixture, gt_bad_operand) {
    verify_no_root("let a < return b;");
}

TEST_F(ParserFixture, assignment_bad_operand_import) {
    verify_no_root("a = import b;");
}

TEST_F(ParserFixture, assignment_bad_operand_class) {
    verify_no_root("a = class A {};");
}

TEST_F(ParserFixture, assignment_bad_operand_let) {
    verify_no_root("a = let a : b;");
}

TEST_F(ParserFixture, assignment_bad_operand_if) {
    verify_no_root("a = if (true) {};");
}

TEST_F(ParserFixture, assignment_bad_operand_func) {
    verify_no_root("a = func a() {};");
}

TEST_F(ParserFixture, assignment_bad_operand_while) {
    verify_no_root("a = while (true) {};");
}

TEST_F(ParserFixture, assignment_bad_operand_return) {
    verify_no_root("func f() { a = return b; }");
}

TEST_F(ParserFixture, func_bad_arg_name) {
    verify_no_root("func f(foo.bar: A) : R { };");
}

TEST_F(ParserFixture, return_literal) {
    verify_single("return 1;", "Return(Int(1))");
}

TEST_F(ParserFixture, return_func_call) {
    verify_single("return a.b(c,d,e);",
            "Return(Call(n=Dot(l=Id(a), r=Id(b)), a=FuncArgs([Id(c), Id(d), Id(e)])))");
}

TEST_F(ParserFixture, anonscope_func) {
    verify_single(R"(func f():Null{ { let a=5; }; { let a=5; }; };)",
            "Func(n=Id(f), a=[], r=Id(Null), s="
            "[[Let(n=Id(a), i=Int(5))],"
            " [Let(n=Id(a), i=Int(5))]])");
}

TEST_F(ParserFixture, return_logic) {
    verify_single("return a > b;", "Return(OpGt(l=Id(a), r=Id(b)))");
}

TEST_F(ParserFixture, return_bad_operand) {
    verify_no_root("return let a: b;");
}

TEST_F(ParserFixture, bonus) {
    verify_no_root("1---2;");
}
