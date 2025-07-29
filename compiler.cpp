#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <stdexcept>
#include <map>
#include <memory>
#include <variant>
#include <regex>

// --- Forward Declarations ---
struct ASTNode;
std::string generate_cpp_code(const std::vector<std::unique_ptr<ASTNode>>& ast);

// --- Helper Functions ---
bool is_variable(const std::string& token) {
    if (token == "밥") {
        return true;
    }
    // Using manual string checks instead of regex for UTF-8 compatibility
    // Assumes each Korean character is 3 bytes in UTF-8
    if (token.length() >= 6 && token.rfind("바", 0) == 0 && token.substr(token.length() - 3) == "압") {
        // Check if all characters in the middle are "아"
        for (size_t i = 3; i < token.length() - 3; i += 3) {
            if (token.substr(i, 3) != "아") {
                return false;
            }
        }
        return true;
    }
    return false;
}

// --- Tokenizer ---
enum class TokenType {
    KEYWORD, IDENTIFIER, NUMBER, STRING, SYMBOL, END_OF_FILE
};

struct Token {
    TokenType type;
    std::string value;
};

std::vector<Token> tokenize(const std::string& code) {
    std::vector<Token> tokens;
    for (size_t i = 0; i < code.length(); ) {
        if (isspace(code[i])) {
            i++;
            continue;
        }

        if (code[i] == '[' || code[i] == ']') {
            tokens.push_back({TokenType::SYMBOL, std::string(1, code[i])});
            i++;
            continue;
        }

        if (code[i] == '"' || code[i] == '\'') {
            char quote = code[i];
            i++;
            size_t start = i;
            while (i < code.length() && code[i] != quote) {
                i++;
            }
            tokens.push_back({TokenType::STRING, code.substr(start, i - start)});
            if (i < code.length()) {
                i++; // Skip closing quote
            }
            continue;
        }

        size_t start = i;
        while (i < code.length() && !isspace(code[i]) && code[i] != '[' && code[i] != ']') {
            i++;
        }
        std::string value = code.substr(start, i - start);
        
        if (value.empty()) continue;

        if (value == "은" || value == "입" || value == "몰" || value == "캠프" || value == "퇴근" || value == "스크럼" || value == "뭐먹" ||
            value == "덧셈" || value == "합" || value == "더하기" || value == "곱셈" || value == "곱" ||
            value == "같" || value == "작" || value == "같작" || value == "작같" ||
            value == "커서" || value == "지피티" || value == "제미나이" || value == "클로드" || value == "클라인" || value == "그록") {
            tokens.push_back({TokenType::KEYWORD, value});
        } else if (is_variable(value)) {
            tokens.push_back({TokenType::IDENTIFIER, value});
        } else {
            try {
                std::stoi(value);
                tokens.push_back({TokenType::NUMBER, value});
            } catch (const std::invalid_argument&) {
                // It can be a function name like 캠프1, 캠프2 etc.
                tokens.push_back({TokenType::IDENTIFIER, value});
            }
        }
    }
    tokens.push_back({TokenType::END_OF_FILE, ""});
    return tokens;
}


// --- AST ---
struct ASTNode {
    virtual ~ASTNode() = default;
    virtual std::string generate() const = 0;
};

// Global context for variable and function names
std::map<std::string, std::string> variable_map;
int var_counter = 0;
std::map<std::string, std::string> function_map;
int func_counter = 0;

std::string get_cpp_var(const std::string& mol_var) {
    if (variable_map.find(mol_var) == variable_map.end()) {
        variable_map[mol_var] = "var_" + std::to_string(var_counter++);
    }
    return variable_map[mol_var];
}

std::string get_cpp_func(const std::string& mol_func) {
    if (function_map.find(mol_func) == function_map.end()) {
        function_map[mol_func] = "func_" + std::to_string(func_counter++);
    }
    return function_map[mol_func];
}


struct NumberNode : ASTNode {
    std::string value;
    NumberNode(std::string v) : value(v) {}
    std::string generate() const override { return "MolObject(" + value + ")"; }
};

struct StringNode : ASTNode {
    std::string value;
    StringNode(std::string v) : value(v) {}
    std::string generate() const override { return "MolObject(std::string(\"" + value + "\"))"; }
};

struct VariableNode : ASTNode {
    std::string name;
    VariableNode(std::string n) : name(n) {}
    std::string generate() const override { return get_cpp_var(name); }
};

struct InputNode : ASTNode {
    std::string generate() const override { return "mollang_input()"; }
};

struct BinaryOpNode : ASTNode {
    std::unique_ptr<ASTNode> left;
    std::string op;
    std::unique_ptr<ASTNode> right;
    BinaryOpNode(std::unique_ptr<ASTNode> l, std::string o, std::unique_ptr<ASTNode> r)
        : left(std::move(l)), op(o), right(std::move(r)) {}
    std::string generate() const override {
        return "(" + left->generate() + " " + op + " " + right->generate() + ")";
    }
};

struct AssignNode : ASTNode {
    std::string var_name;
    std::unique_ptr<ASTNode> expr;
    AssignNode(std::string n, std::unique_ptr<ASTNode> e) : var_name(n), expr(std::move(e)) {}
    std::string generate() const override {
        return get_cpp_var(var_name) + " = " + expr->generate() + ";";
    }
};

struct PrintNode : ASTNode {
    std::unique_ptr<ASTNode> expr;
    PrintNode(std::unique_ptr<ASTNode> e) : expr(std::move(e)) {}
    std::string generate() const override {
        return "mollang_print(" + expr->generate() + ");";
    }
};

struct IfNode : ASTNode {
    std::unique_ptr<ASTNode> condition;
    std::vector<std::unique_ptr<ASTNode>> body;
    IfNode(std::unique_ptr<ASTNode> c, std::vector<std::unique_ptr<ASTNode>> b)
        : condition(std::move(c)), body(std::move(b)) {}
    std::string generate() const override {
        std::stringstream ss;
        ss << "if (std::get<bool>(" << condition->generate() << ".value)) {\n";
        for (const auto& stmt : body) {
            ss << "    " << stmt->generate() << "\n";
        }
        ss << "}";
        return ss.str();
    }
};

struct WhileNode : ASTNode {
    std::unique_ptr<ASTNode> condition;
    std::vector<std::unique_ptr<ASTNode>> body;
    WhileNode(std::unique_ptr<ASTNode> c, std::vector<std::unique_ptr<ASTNode>> b)
        : condition(std::move(c)), body(std::move(b)) {}
    std::string generate() const override {
        std::stringstream ss;
        ss << "while (std::get<bool>(" << condition->generate() << ".value)) {\n";
        for (const auto& stmt : body) {
            ss << "    " << stmt->generate() << "\n";
        }
        ss << "}";
        return ss.str();
    }
};

struct FuncDefNode : ASTNode {
    std::string name;
    std::vector<std::unique_ptr<ASTNode>> body;
    FuncDefNode(std::string n, std::vector<std::unique_ptr<ASTNode>> b)
        : name(n), body(std::move(b)) {}
    std::string generate() const override {
        std::stringstream ss;
        ss << "MolObject " << get_cpp_func(name) << "() {\n";
        for (const auto& stmt : body) {
            ss << stmt->generate() << "\n";
        }
        ss << "return MolObject();\n"; // Default return
        ss << "}";
        return ss.str();
    }
};

struct FuncCallNode : ASTNode {
    std::string name;
    FuncCallNode(std::string n) : name(n) {}
    std::string generate() const override {
        return get_cpp_func(name) + "();";
    }
};

struct ReturnNode : ASTNode {
    std::unique_ptr<ASTNode> expr;
    ReturnNode(std::unique_ptr<ASTNode> e) : expr(std::move(e)) {}
    std::string generate() const override {
        return "return " + expr->generate() + ";";
    }
};


// --- Parser ---
class Parser {
public:
    Parser(std::vector<Token> tokens) : tokens(std::move(tokens)), pos(0) {}

    std::vector<std::unique_ptr<ASTNode>> parse() {
        std::vector<std::unique_ptr<ASTNode>> statements;
        while (peek().type != TokenType::END_OF_FILE) {
            statements.push_back(parse_statement());
        }
        return statements;
    }

private:
    std::vector<Token> tokens;
    size_t pos;

    Token peek() {
        if (pos >= tokens.size()) {
            throw std::runtime_error("Parser error: Unexpected end of file while peeking.");
        }
        return tokens[pos];
    }
    Token consume() {
        if (pos >= tokens.size()) {
            throw std::runtime_error("Parser error: Unexpected end of file while consuming.");
        }
        return tokens[pos++];
    }
    
    std::unique_ptr<ASTNode> parse_statement();
    std::unique_ptr<ASTNode> parse_expression();
    std::unique_ptr<ASTNode> parse_simple_expr();
    std::vector<std::unique_ptr<ASTNode>> parse_block();
    std::string get_operator(const std::string& token);
};

std::unique_ptr<ASTNode> Parser::parse_statement() {
    Token token = peek();
    if (is_variable(token.value)) {
        std::string var_name = consume().value;
        consume(); // '은'
        auto expr = parse_expression();
        return std::make_unique<AssignNode>(var_name, std::move(expr));
    }
    if (token.value == "스크럼") {
        consume();
        auto expr = parse_expression();
        return std::make_unique<PrintNode>(std::move(expr));
    }
    if (token.value == "입") {
        consume();
        auto cond = parse_expression();
        auto body = parse_block();
        return std::make_unique<IfNode>(std::move(cond), std::move(body));
    }
    if (token.value == "몰") {
        consume();
        auto cond = parse_expression();
        auto body = parse_block();
        return std::make_unique<WhileNode>(std::move(cond), std::move(body));
    }
    if (token.value.rfind("캠프", 0) == 0) { // starts with 캠프
        std::string func_name = consume().value;
        if (peek().value == "[") {
            auto body = parse_block();
            return std::make_unique<FuncDefNode>(func_name, std::move(body));
        } else {
            return std::make_unique<FuncCallNode>(func_name);
        }
    }
    if (token.value == "퇴근") {
        consume();
        auto expr = parse_expression();
        return std::make_unique<ReturnNode>(std::move(expr));
    }
    throw std::runtime_error("Invalid statement start: '" + token.value + "'");
}

std::vector<std::unique_ptr<ASTNode>> Parser::parse_block() {
    consume(); // '['
    std::vector<std::unique_ptr<ASTNode>> statements;
    while (peek().value != "]") {
        statements.push_back(parse_statement());
    }
    consume(); // ']'
    return statements;
}

std::unique_ptr<ASTNode> Parser::parse_expression() {
    auto left = parse_simple_expr();
    while (peek().type == TokenType::KEYWORD) {
        std::string op_token = peek().value;
        if (op_token == "은" || op_token == "입" || op_token == "몰" || op_token == "스크럼" || op_token == "캠프" || op_token == "퇴근") break;
        if (peek().value == "[" || peek().value == "]") break;
        
        consume();
        std::string op = get_operator(op_token);
        auto right = parse_simple_expr();
        left = std::make_unique<BinaryOpNode>(std::move(left), op, std::move(right));
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parse_simple_expr() {
    Token token = consume();
    if (token.type == TokenType::NUMBER) {
        return std::make_unique<NumberNode>(token.value);
    }
    if (token.type == TokenType::STRING) {
        return std::make_unique<StringNode>(token.value);
    }
    if (token.type == TokenType::IDENTIFIER) {
        return std::make_unique<VariableNode>(token.value);
    }
    if (token.value == "뭐먹") {
        return std::make_unique<InputNode>();
    }
    if (token.value == "커서") return std::make_unique<StringNode>("커서는 신이야");
    if (token.value == "지피티") return std::make_unique<StringNode>("지피티는 요즘 애매해");
    if (token.value == "제미나이") return std::make_unique<StringNode>("제미나이는 잘 따라가는중");
    if (token.value == "클로드") return std::make_unique<StringNode>("클로드는 LLM 중 코딩 끝판왕");
    if (token.value == "클라인") return std::make_unique<StringNode>("클라인도 레전드입니다… 꼭 쓰세요");
    if (token.value == "그록") return std::make_unique<StringNode>("그록 누가씀?");
    
    throw std::runtime_error("Invalid expression term: " + token.value);
}

std::string Parser::get_operator(const std::string& token) {
    if (token == "덧셈" || token == "합" || token == "더하기") return "+";
    if (token == "곱셈" || token == "곱") return "*";
    if (token == "같") return "==";
    if (token == "작") return "<";
    if (token == "같작" || token == "작같") return "<=";
    throw std::runtime_error("Unknown operator: " + token);
}


// --- Code Generator ---
void collect_symbols(const ASTNode* node) {
    if (!node) return;

    if (const auto* assign_node = dynamic_cast<const AssignNode*>(node)) {
        get_cpp_var(assign_node->var_name);
        collect_symbols(assign_node->expr.get());
    } else if (const auto* print_node = dynamic_cast<const PrintNode*>(node)) {
        collect_symbols(print_node->expr.get());
    } else if (const auto* if_node = dynamic_cast<const IfNode*>(node)) {
        collect_symbols(if_node->condition.get());
        for (const auto& stmt : if_node->body) {
            collect_symbols(stmt.get());
        }
    } else if (const auto* while_node = dynamic_cast<const WhileNode*>(node)) {
        collect_symbols(while_node->condition.get());
        for (const auto& stmt : while_node->body) {
            collect_symbols(stmt.get());
        }
    } else if (const auto* func_def_node = dynamic_cast<const FuncDefNode*>(node)) {
        get_cpp_func(func_def_node->name);
        for (const auto& stmt : func_def_node->body) {
            collect_symbols(stmt.get());
        }
    } else if (const auto* func_call_node = dynamic_cast<const FuncCallNode*>(node)) {
        get_cpp_func(func_call_node->name);
    } else if (const auto* return_node = dynamic_cast<const ReturnNode*>(node)) {
        collect_symbols(return_node->expr.get());
    } else if (const auto* binary_op_node = dynamic_cast<const BinaryOpNode*>(node)) {
        collect_symbols(binary_op_node->left.get());
        collect_symbols(binary_op_node->right.get());
    } else if (const auto* var_node = dynamic_cast<const VariableNode*>(node)) {
        get_cpp_var(var_node->name);
    }
}

std::string generate_cpp_code(const std::vector<std::unique_ptr<ASTNode>>& ast) {
    std::stringstream ss;

    // Preamble
    ss << R"(#include <iostream>
#include <string>
#include <vector>
#include <variant>
#include <stdexcept>

// A variant type to handle Mollang's dynamic types
struct MolObject {
    std::variant<std::monostate, int, std::string, bool> value;

    MolObject() : value(std::monostate{}) {}
    MolObject(int v) : value(v) {}
    MolObject(const std::string& v) : value(v) {}
    MolObject(const char* v) : value(std::string(v)) {}
    MolObject(bool v) : value(v) {}
};

// Overloads for operators
MolObject operator+(const MolObject& a, const MolObject& b) {
    if (std::holds_alternative<int>(a.value) && std::holds_alternative<int>(b.value)) {
        return MolObject(std::get<int>(a.value) + std::get<int>(b.value));
    }
    if (std::holds_alternative<std::string>(a.value) && std::holds_alternative<std::string>(b.value)) {
        return MolObject(std::get<std::string>(a.value) + std::get<std::string>(b.value));
    }
    throw std::runtime_error("Unsupported operand types for +");
}

MolObject operator*(const MolObject& a, const MolObject& b) {
    if (std::holds_alternative<std::string>(a.value) && std::holds_alternative<int>(b.value)) {
        std::string s = "";
        for (int i = 0; i < std::get<int>(b.value); ++i) {
            s += std::get<std::string>(a.value);
        }
        return MolObject(s);
    }
    if (std::holds_alternative<int>(a.value) && std::holds_alternative<int>(b.value)) {
        return MolObject(std::get<int>(a.value) * std::get<int>(b.value));
    }
    throw std::runtime_error("Unsupported operand types for *");
}

MolObject operator<(const MolObject& a, const MolObject& b) {
    if (std::holds_alternative<int>(a.value) && std::holds_alternative<int>(b.value)) {
        return MolObject(std::get<int>(a.value) < std::get<int>(b.value));
    }
    throw std::runtime_error("Unsupported operand types for <");
}

MolObject operator<=(const MolObject& a, const MolObject& b) {
    if (std::holds_alternative<int>(a.value) && std::holds_alternative<int>(b.value)) {
        return MolObject(std::get<int>(a.value) <= std::get<int>(b.value));
    }
    throw std::runtime_error("Unsupported operand types for <=");
}

MolObject operator==(const MolObject& a, const MolObject& b) {
    if (std::holds_alternative<int>(a.value) && std::holds_alternative<int>(b.value)) {
        return MolObject(std::get<int>(a.value) == std::get<int>(b.value));
    }
    if (std::holds_alternative<std::string>(a.value) && std::holds_alternative<std::string>(b.value)) {
        return MolObject(std::get<std::string>(a.value) == std::get<std::string>(b.value));
    }
    return MolObject(false);
}

// Helper functions
void mollang_print(const MolObject& obj) {
    if (std::holds_alternative<int>(obj.value)) {
        std::cout << std::get<int>(obj.value);
    } else if (std::holds_alternative<std::string>(obj.value)) {
        std::cout << std::get<std::string>(obj.value);
    } else if (std::holds_alternative<bool>(obj.value)) {
        std::cout << (std::get<bool>(obj.value) ? "true" : "false");
    }
    std::cout << std::endl;
}

MolObject mollang_input() {
    std::string line;
    std::getline(std::cin, line);
    try {
        return MolObject(std::stoi(line));
    } catch (const std::invalid_argument&) {
        return MolObject(line);
    }
}
)";

    // Function Prototypes
    for (const auto& pair : function_map) {
        ss << "MolObject " << pair.second << "();\n";
    }
    ss << "\n";

    // Global Variables
    for (const auto& pair : variable_map) {
        ss << "MolObject " << pair.second << ";\n";
    }
    ss << "\n";

    // Function Definitions
    for (const auto& node : ast) {
        if (dynamic_cast<const FuncDefNode*>(node.get())) {
            ss << node->generate() << "\n\n";
        }
    }

    // Main function
    ss << "int main() {\n";
    for (const auto& node : ast) {
        if (!dynamic_cast<const FuncDefNode*>(node.get())) {
            ss << "    " << node->generate() << "\n";
        }
    }
    ss << "    return 0;\n";
    ss << "}\n";

    return ss.str();
}


// --- Main Compiler Logic ---
std::string translate_to_cpp(const std::string& mollang_code) {
    // Reset global state for each compilation
    variable_map.clear();
    var_counter = 0;
    function_map.clear();
    func_counter = 0;

    auto tokens = tokenize(mollang_code);
    Parser parser(std::move(tokens));
    auto ast = parser.parse();

    // Populate symbol maps
    for (const auto& node : ast) {
        collect_symbols(node.get());
    }

    return generate_cpp_code(ast);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "사용법: " << argv[0] << " <입력_파일.mol>" << std::endl;
        return 1;
    }

    std::string input_filename = argv[1];
    if (input_filename.size() <= 4 || input_filename.substr(input_filename.size() - 4) != ".mol") {
        std::cerr << "오류: 입력 파일은 '.mol' 확장자여야 합니다." << std::endl;
        return 1;
    }

    std::ifstream input_file(input_filename);
    if (!input_file) {
        std::cerr << "오류: '" << input_filename << "' 파일을 열 수 없습니다." << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << input_file.rdbuf();
    std::string mollang_code = buffer.str();

    try {
        std::string cpp_code = translate_to_cpp(mollang_code);

        std::string output_basename = input_filename.substr(0, input_filename.size() - 4);
        std::string cpp_filename = output_basename + ".cpp";
        std::string exe_filename = output_basename;

        std::ofstream cpp_file(cpp_filename);
        if (!cpp_file) {
            std::cerr << "오류: '" << cpp_filename << "' 파일을 생성할 수 없습니다." << std::endl;
            return 1;
        }
        cpp_file << cpp_code;
        cpp_file.close();

        std::cout << "Mollang 코드를 C++로 변환했습니다: " << cpp_filename << std::endl;

        std::string compile_command = "g++ -std=c++17 -o " + exe_filename + " " + cpp_filename;
        std::cout << "컴파일 중: " << compile_command << std::endl;

        int result = system(compile_command.c_str());

        if (result == 0) {
            std::cout << "컴파일 성공! 실행 파일 생성: " << exe_filename << std::endl;
        } else {
            std::cerr << "컴파일 오류가 발생했습니다." << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "오류: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
