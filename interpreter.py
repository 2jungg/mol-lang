import sys
import re

# --- Helper to identify variables ---
def is_variable(token):
    """Checks if a token is a valid variable name."""
    return token == '밥' or re.fullmatch(r'바아*압', token)

# --- Pre-defined Strings ---
PREDEFINED_STRINGS = {
    "커서": "커서는 신이야",
    "지피티": "지피티는 요즘 애매해",
    "제미나이": "제미나이는 잘 따라가는중",
    "클로드": "클로드는 LLM 중 코딩 끝판왕",
    "클라인": "클라인도 레전드입니다… 꼭 쓰세요",
    "그록": "그록 누가씀?",
}

# --- Custom Exception for Return ---
class ReturnValue(Exception):
    def __init__(self, value):
        self.value = value

# --- Manual Tokenizer V2 ---
def tokenize(code):
    code = code.replace('[', ' [ ').replace(']', ' ] ')
    tokens = []
    i = 0
    while i < len(code):
        char = code[i]
        if char.isspace():
            i += 1
            continue
        
        if char in ['"', "'", '`']:
            quote_char = char
            string_token = char
            i += 1
            while i < len(code) and code[i] != quote_char:
                string_token += code[i]
                i += 1
            if i < len(code): # Add the closing quote
                string_token += code[i]
                i += 1
            tokens.append(string_token)
            continue

        # For other tokens
        other_token = char
        i += 1
        while i < len(code) and not code[i].isspace() and code[i] not in ['"', "'", '`', '[', ']']:
            other_token += code[i]
            i += 1
        tokens.append(other_token)

    return tokens

# --- Parser (creates AST) ---
class Parser:
    def __init__(self, tokens):
        self.tokens = tokens
        self.pos = 0

    def peek(self):
        return self.tokens[self.pos] if self.pos < len(self.tokens) else None

    def consume(self, expected_token=None):
        token = self.peek()
        if expected_token and token != expected_token:
            raise SyntaxError(f"Expected '{expected_token}' but found '{token}'")
        self.pos += 1
        return token

    def parse_expression(self):
        # This is a simple precedence-free, left-to-right parser
        left_node = self.parse_simple_expr()
        
        # An expression ends if we see a keyword that starts a new statement, or a block marker
        statement_starters = ['입', '몰', '스크럼', '캠프', '퇴근']
        
        while self.peek() and self.peek() not in ['[', ']'] and not is_variable(self.peek()) and self.peek() not in statement_starters:
            op_token = self.consume()
            op = self.get_operator(op_token)
            right_node = self.parse_simple_expr()
            left_node = ('bin_op', op, left_node, right_node)
            
        return left_node

    def parse_simple_expr(self):
        token = self.consume()
        # Quoted string literal
        if (token.startswith('"') and token.endswith('"')) or \
           (token.startswith("'") and token.endswith("'")) or \
           (token.startswith('`') and token.endswith('`')):
            return ('string', token[1:-1]) # Return content without quotes
        
        if token == '뭐먹':
            return ('input',)
        if token in PREDEFINED_STRINGS:
            return ('string', PREDEFINED_STRINGS[token])
        if is_variable(token):
            return ('variable', token)
        try:
            return ('number', int(token))
        except ValueError:
            raise SyntaxError(f"Unknown value or variable: {token}")

    def get_operator(self, token):
        if token in ['덧셈', '합', '더하기']: return '+'
        if token in ['곱셈', '곱']: return '*'
        if token == '같': return '=='
        if token == '작': return '<'
        if token in ['같작', '작같']: return '<='
        raise SyntaxError(f"Unknown operator: {token}")

    def parse_block(self):
        self.consume('[')
        statements = []
        while self.peek() != ']':
            statements.append(self.parse_statement())
        self.consume(']')
        return statements

    def parse_statement(self):
        token = self.peek()
        if is_variable(token):
            var_name = self.consume()
            self.consume('은')
            expr = self.parse_expression()
            return ('assign', var_name, expr)
        if token == '스크럼':
            self.consume('스크럼')
            expr = self.parse_expression()
            return ('print', expr)
        if token == '입':
            self.consume('입')
            condition = self.parse_expression()
            body = self.parse_block()
            return ('if', condition, body)
        if token == '몰':
            self.consume('몰')
            condition = self.parse_expression()
            body = self.parse_block()
            return ('while', condition, body)
        if token.startswith('캠프'):
            func_name = self.consume()
            if self.peek() == '[':
                body = self.parse_block()
                return ('func_def', func_name, [], body)
            else:
                return ('func_call', func_name, [])
        if token == '퇴근':
            self.consume('퇴근')
            expr = self.parse_expression()
            return ('return', expr)
        raise SyntaxError(f"Unexpected statement start: {token}")

    def parse(self):
        statements = []
        while self.peek():
            statements.append(self.parse_statement())
        return statements

# --- Interpreter (executes AST) ---
class Interpreter:
    def __init__(self):
        self.variables = {}
        self.functions = {}

    def visit(self, node):
        node_type = node[0]
        method = getattr(self, f'visit_{node_type}')
        return method(node)

    def visit_number(self, node): return node[1]
    def visit_string(self, node): return node[1]
    
    def visit_variable(self, node):
        var_name = node[1]
        if var_name not in self.variables:
            raise NameError(f"Variable '{var_name}' is not defined.")
        return self.variables[var_name]

    def visit_input(self, node):
        value = input()
        try:
            return int(value)
        except ValueError:
            return value

    def visit_assign(self, node):
        var_name = node[1]
        value = self.visit(node[2])
        self.variables[var_name] = value
        return value

    def visit_print(self, node):
        value = self.visit(node[1])
        print(value)

    def visit_bin_op(self, node):
        op, left_node, right_node = node[1], node[2], node[3]
        left = self.visit(left_node)
        right = self.visit(right_node)
        if op == '+': return left + right
        if op == '*': return left * right
        if op == '==': return left == right
        if op == '<': return left < right
        if op == '<=': return left <= right
        raise ValueError(f"Unsupported operator: {op}")

    def visit_if(self, node):
        condition = self.visit(node[1])
        if condition:
            for stmt in node[2]:
                self.visit(stmt)

    def visit_while(self, node):
        while self.visit(node[1]):
            for stmt in node[2]:
                self.visit(stmt)

    def visit_func_def(self, node):
        func_name, params, body = node[1], node[2], node[3]
        self.functions[func_name] = {'params': params, 'body': body}

    def visit_func_call(self, node):
        func_name = node[1]
        if func_name not in self.functions:
            raise NameError(f"Function '{func_name}' is not defined.")
        
        func = self.functions[func_name]
        return_value = None
        try:
            for stmt in func['body']:
                self.visit(stmt)
        except ReturnValue as rv:
            return_value = rv.value
        return return_value

    def visit_return(self, node):
        value = self.visit(node[1])
        raise ReturnValue(value)

    def run(self, code):
        tokens = tokenize(code)
        ast = Parser(tokens).parse()
        for stmt in ast:
            self.visit(stmt)

# --- Main Execution ---
def main():
    if len(sys.argv) > 1:
        filepath = sys.argv[1]
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                code = f.read()
            interpreter = Interpreter()
            interpreter.run(code)
        except FileNotFoundError:
            print(f"Error: File not found at '{filepath}'")
        except Exception as e:
            print(f"An error occurred: {type(e).__name__}: {e}")
    else:
        print("Usage: python interpreter.py <filename.mol>")

if __name__ == "__main__":
    main()
