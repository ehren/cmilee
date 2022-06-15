from parser import parse
from parser import Node, Module, Call, Block, UnOp, BinOp, ColonBinOp
import io


class RebuiltBlock(Block):

    def __init__(self, args):
        self.func = "Block"
        self.args = args


class RebuiltCall(Call):
    def __init__(self, func, args):
        self.func = func
        self.args = args


class SemanticAnalysisError(Exception):
    pass
    #def __init__(self, message, line_number):
    #    super().__init__("{}. Line {}.".format(message, line_number))

cpp_preamble = """
#include <memory>
#include <cstdio>

class object {
    virtual ~{
    };
};

"""


def codegen_if(ifnode, cpp):
    assert ifnode.func == "if"
    cpp.write("if (0")


def codegen_block(block: Block, cpp):
    assert isinstance(block, Block)
    for b in block.args:
        if isinstance(b, Call):
            if b.func == "if":
                codegen_if(b, cpp)
        elif isinstance(b, BinOp):
            pass
        elif isinstance(b, UnOp):
            pass

        print(b)


def codegen_def(defnode: Call, cpp):
    assert defnode.func == "def"
    name = defnode.args[0]
    cpp.write(f"std::shared_ptr<object> {name} (")
    args = defnode.args[1:]
    block = args.pop()
    assert isinstance(block, Block)
    if len(args):
        cpp_args = ", ".join([f"std::shared_ptr<object> {a}" for a in args])
        cpp.write(cpp_args)
        # for i in range(len(args)):
        #    cpp.write(f"std::shared_ptr<object> {args[i]},")
    cpp.write(") {\n")
    codegen_block(block, cpp)
    cpp.write("}")


def codegen(parsed):
    cpp = io.StringIO()
    cpp.write(cpp_preamble)
    #cpp.seek(io.SEEK_END)

    assert isinstance(parsed, Module) # enforced by parser
    for moddef in parsed.args:
        assert isinstance(moddef, Call) # enforced by parser

        print(moddef.func)
        if moddef.func not in ["def", "class"]:
            raise SemanticAnalysisError("Only defs or classes at module level")

        if moddef.func == "def":
            codegen_def(moddef, cpp)

    return cpp.getvalue()


def one_liner_expander(parsed):

    def ifreplacer(ifop):

        if len(ifop.args) < 1:
            raise SemanticAnalysisError("not enough if args")

        if len(ifop.args) == 1 or not isinstance(ifop.args[1],
                                                 Block):
            if isinstance(ifop.args[0], ColonBinOp):
                # convert second arg of outermost colon to one element block
                rebuilt = [ifop.args[0].args[0], RebuiltBlock(
                    args=[ifop.args[0].args[1]])] + ifop.args[2:]
                return RebuiltCall(func="if", args=rebuilt)
            else:
                raise SemanticAnalysisError("bad first if-args")

        for i, a in enumerate(ifop.args[2:]):
            if isinstance(a, Block):
                if not ifop.args[i - 1] in ["elif", "else"]:
                    raise SemanticAnalysisError(
                        f"unexpected if arg. Found block at position {i} but it's not preceded by 'if' or 'elif'")
            elif a in ["elif", "else"]:
                if i == len(ifop.args) - 1:
                    raise SemanticAnalysisError(
                        "found {} as last arg of if".format(a))
                elif not isinstance(ifop.args[i + 1], Block):
                    # convert single expression to 1-element block (hope this is desirable!)
                    rebuilt = [ifop.args[0:i + 1], RebuiltBlock(
                        args=[ifop.args[i + 1]])] + ifop.args[i + 1:]
                    return RebuiltCall(func="if", args=rebuilt)
                elif isinstance(a, ColonBinOp):
                    if isinstance(a, ColonBinOp):
                        # convert second arg of outermost colon to one element block
                        rebuilt = [a.args[0], RebuiltBlock(
                            args=[a.args[1]])] + rebuilt[2:]
                        return RebuiltCall(func="if", args=rebuilt)
                else:
                    raise SemanticAnalysisError(
                        f"bad if-arg {a} at position {i}")

        return ifop

    def visitor(op):

        if not isinstance(op, Node):
            return False, op

        if isinstance(op, Call):
            if op.func == "def":
                if len(op.args) < 2:
                    raise SemanticAnalysisError("not enough def args")
                if not isinstance(op.args[0], str):
                    raise SemanticAnalysisError("bad def args (first arg must be an identifier)")
                if not isinstance(op.args[-1], Block):
                    # last arg becomes one-element block
                    return True, RebuiltCall(func=op.func, args=op.args[0:-1] + [RebuiltBlock(args=[op.args[-1]])])
            elif op.func == "if":
                changed = False
                while True:
                    new = ifreplacer(op)
                    if new is op:
                        break
                    else:
                        changed = True
                        op = new
                if changed:
                    return True, op

        rebuilt = []
        changed = False

        for arg in op.args:
            changed, arg = visitor(arg)
            rebuilt.append(arg)

        if changed:
            op.args = rebuilt

        return changed, op

    while True:
        changed, parsed = visitor(parsed)
        if not changed:
            break

    return parsed


def compile(s):
    parsed = parse(s)
    parsed = one_liner_expander(parsed)
    print("one_liner_expander", parsed)
    code = codegen(parsed)
    print("code:\n", code)


if __name__ == "__main__":
    compile("""
def (main:
    if (1:1)
    def (x,1)
)""")

    0 and compile("""
def (bar, x, y, 0)
def (foo, x, y:
    # parser: [Module([def(['bar', 'x', 'y', 0]), def(['foo', 'x', 'y', Block([[if([:(x,int), :(1,int), :(elif,:(-(x,1),int)), :(5,int), :(else,:(0,int))])], 
    #[if([:(1,1), 
    # :(elif,:(:(x,int),:(5,int))), 
    # :(else,:(0,int))])]])])])]

    #if (x:int, 1:int, elif:x-1:int, 5:int, else: 0:int)
    #[if([:(1,1), 
    # :(elif, :(x,:(int,:(5,int)))), 
    # :(else,0)])]
    #if (1:1, elif:x:int:5:int, else: 0:int)
    if (1:1)
    if (elif:x:int:5:int) # should result in "unknown identifier 'int'"
    if (elif:(x:int):5:int)
    #if (1:1, elif:(x:int):5:int, else: 0:int)
    #if (1:1, elif:x:5, else: 0)
)""")

    0 and compile("""
def (bar, x, y, 0)
def (foo, x, y:
    if (1:
        print("1")
        2
    )

    if (2:
        5
    elif: y: 2, else: 0)

    if (x:int, 5:int, elif: x = y : int, 0, else: 0)
    if (x:int, 1, elif:x-1:int, 5, else: 0)
    if (x:int, 1:int, elif:x-1:int, 5:int, else: 0:int)
    #if (x:int, 1, elif:x+1: 0:int)  # invalid if semantics
    if (x:int, 1, elif:x+1, 0:int, else: 5)
    if (x, 1, elif: y, 2)
    x=if (x, 1)
    lambda (x, y, z:int, 0)
    lambda (x:int, y:int, z:int:
        0
    )
    lambda(1)
    lambda(:
        1
    )
    lambda(x:int:
        1
    )
    lambda(x:int, 1)
)

    """)


    0 and compile("""
if (x: 
    y
    x+1
    3
    z = 5+4
)
foo(x,y)
foo(x,y)""")
