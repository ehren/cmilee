from parser import Node, Module, Call, Block, UnOp, BinOp, ColonBinOp, Assign, RedundantParens, Identifier, IntegerLiteral, RebuiltColon, SyntaxColonBinOp
import sys

def isa_or_wrapped(node, NodeClass):
    return isinstance(node, NodeClass) or (isinstance(node, ColonBinOp) and isinstance(node.args[0], NodeClass))


# def _monkey_ident(self, name:str):
#     self.func = name


class RebuiltIdentifer(Identifier):
    def __init__(self, name):
        self.func = name
        self.args = []
        self.name = name

# Identifier.__init__ = _monkey_ident


class RebuiltBlock(Block):

    def __init__(self, args):
        self.func = "Block"
        self.args = args


class RebuiltCall(Call):
    def __init__(self, func, args):
        self.func = func
        self.args = args


# def _monkey(self, func, args):
#     self.func = func
#     self.args = args

# Assign.__init__ = _monkey
class RebuiltAssign(Assign):
    def __init__(self, args):
        self.func = "Assign"
        self.args = args


class NamedParameter(Node):
    def __init__(self, args):
        self.func = "NamedParameter"
        self.args = args  # [lhs, rhs]

    def __repr__(self):
        return "{}({})".format(self.func, ",".join(map(str, self.args)))


# should be renamed "IfWrapper"
class IfNode:#(Call):  # just a helper class for now (avoid adding to ast)

    def __repr__(self):
        return "{}({})".format(self.func, ",".join(map(str, self.args)))

    def __init__(self, func, args):
        self.func = func
        self._build(args)

    @property
    def args(self):
        return self._args

    @args.setter
    def args(self, args):
        self._build(args)

    def _build(self, args):
        # Assumes that func and args have already been processed by `one_liner_expander`
        self._args = list(args)
        self.cond = args.pop(0)
        self.thenblock = args.pop(0)
        assert isinstance(self.thenblock, Block)
        self.eliftuples = []
        if args:
            self.elseblock = args.pop()
            elseidentifier = args.pop()
            assert isinstance(elseidentifier, Identifier) and elseidentifier.name == "else"
            while args:
                elifcond = args.pop(0)
                elifblock = args.pop(0)
                assert isinstance(elifcond, ColonBinOp)
                assert elifcond.args[0].name == "elif"
                elifcond = elifcond.args[1]
                self.eliftuples.append((elifcond, elifblock))
        else:
            self.elseblock = None


class SemanticAnalysisError(Exception):
    pass
    #def __init__(self, message, line_number):
    #    super().__init__("{}. Line {}.".format(message, line_number))


def build_parents(node: Node):

    def visitor(node):
        if isinstance(node, Module):
            node.parent = None
        if not isinstance(node, Node):
            return node
        if not hasattr(node, "name"):
            node.name = None
        rebuilt = []
        for arg in node.args:
            if isinstance(arg, Node):
                arg.parent = node
                # if arg.scopes = []
                # arg.rebuild(arg.func, arg.args)
                arg = visitor(arg)
            rebuilt.append(arg)
        node.args = rebuilt
        if isinstance(node.func, Node):
            node.func.parent = node
            node.func = visitor(node.func)
        # node.rebuild(node.func, node.args)
        return node
    return visitor(node)


def build_types(node: Node):

    def visitor(node):
        if not isinstance(node, Node):
            return node

        # TODO add NonTypeColonBinOp or SyntaxColonBinOp (to be swapped with e.g. elif ColonBinOp at some stage)
        #if isinstance(node, ColonBinOp) and not (isinstance(node.args[0], Identifier) and node.args[0].name == "elif"):  # sure hope you're using 'elif' responsibly!
        if isinstance(node, ColonBinOp) and not isinstance(node, SyntaxColonBinOp):
            lhs, rhs = node.args
            node = lhs
            node.type = rhs  # leaving open possibility this is still a ColonBinOp

        node.args = [visitor(arg) for arg in node.args]
        # node.rebuild(node.func, node.args)

        return node

    return visitor(node)



def build_if_nodes(expr):
    assert False

    def visitor(node):
        if not isinstance(node, Node):
            return node

        # visit args before conversion to IfNode
        # (should no longer be necessary)

        if isinstance(node, Call) and node.func.name == "if":
            node = IfNode(node.func, node.args)

        node.args = [visitor(arg) for arg in node.args]

        return node

    return visitor(expr)


def one_liner_expander(parsed):

    def ifreplacer(ifop):

        if len(ifop.args) < 1:
            raise SemanticAnalysisError("not enough if args")

        if len(ifop.args) == 1 or not isinstance(ifop.args[1],
                                                 Block):
            if isinstance(ifop.args[0], ColonBinOp):
                # convert second arg of outermost colon to one element block
                block_arg = ifop.args[0].args[1]
                if isinstance(block_arg, Assign):
                    raise SemanticAnalysisError("no assignment statements in if one liners")
                rebuilt = [ifop.args[0].args[0], RebuiltBlock(args=[block_arg])] + ifop.args[1:]
                return RebuiltCall(func=ifop.func, args=rebuilt)
            else:
                raise SemanticAnalysisError("bad first if-args")

        for i, a in enumerate(list(ifop.args[2:]), start=2):
            if isinstance(a, Block):
                if not (isinstance(ifop.args[i - 1], Identifier) and ifop.args[i - 1].name == "else") and not (isinstance(ifop.args[i - 1], ColonBinOp) and (isinstance(elifliteral := ifop.args[i - 1].args[0], Identifier) and elifliteral.name == "elif")):
                    raise SemanticAnalysisError(
                        f"Unexpected if arg. Found block at position {i} but it's not preceded by 'else' or 'elif'")
            elif isinstance(a, ColonBinOp):
                if not a.args[0].name in ["elif", "else"]:
                    raise SemanticAnalysisError(
                        f"Unexpected if arg {a} at position {i}")
                if a.args[0].name == "else":
                    rebuilt = ifop.args[0:i] + [a.args[0], RebuiltBlock(
                        args=[a.args[1]])] + ifop.args[i + 1:]
                    return RebuiltCall(ifop.func, args=rebuilt)
                elif a.args[0].name == "elif":
                    if i == len(ifop.args) - 1 or not isinstance(ifop.args[i + 1], Block):
                        c = a.args[1]
                        if not isinstance(c, ColonBinOp):
                            raise SemanticAnalysisError("bad if args")
                        cond, rest = c.args
                        new_elif = RebuiltColon(a.func, [a.args[0], cond])
                        new_block = RebuiltBlock(args=[rest])
                        rebuilt = ifop.args[0:i] + [new_elif, new_block] + ifop.args[i + 1:]
                        return RebuiltCall(ifop.func, args=rebuilt)
            elif isinstance(a, Identifier) and a.name == "else":
                if not i == len(ifop.args) - 2:
                    raise SemanticAnalysisError("bad else placement")
                if not isinstance(ifop.args[-1], Block):
                    raise SemanticAnalysisError("bad arg after else")
            else:
                raise SemanticAnalysisError(
                    f"bad if-arg {a} at position {i}")

        return ifop

    def visitor(op):

        if not isinstance(op, Node):
            return False, op

        if isinstance(op, ColonBinOp) and not isinstance(op, SyntaxColonBinOp) and isinstance(op.args[0], Identifier) and op.args[0].name in ["except", "return", "else", "elif"]:
            return True, SyntaxColonBinOp(op.func, op.args)

        if isinstance(op, Call):
            if isinstance(op.func, Identifier) and op.func.name == "def":
                if len(op.args) < 2:
                    raise SemanticAnalysisError("not enough def args")
                if not isinstance(op.args[0], Identifier):
                    raise SemanticAnalysisError("bad def args (first arg must be an identifier)")
                if not isinstance(op.args[-1], Block):
                    # last arg becomes one-element block
                    return True, RebuiltCall(func=op.func, args=op.args[0:-1] + [RebuiltBlock(args=[op.args[-1]])])
            elif isinstance(op.func, Identifier) and op.func.name == "if":
                new = ifreplacer(op)
                if new is not op:
                    return True, new

        rebuilt = []
        changed = False

        for arg in op.args:
            arg_change, arg = visitor(arg)
            if arg_change:
                changed = True
            rebuilt.append(arg)

        if changed:
            op.args = rebuilt

        return changed, op

    while True:
        did_change, parsed = visitor(parsed)
        if not did_change:
            break

    return parsed


def assign_to_named_parameter(expr):

    def replacer(op):
        if not isinstance(op, Node):
            return op
        if isinstance(op, Call):
            rebuilt = []
            for arg in op.args:
                if isinstance(arg, ColonBinOp):
                    if isinstance(arg.args[0], Assign):
                        rebuilt.append(RebuiltColon(func=arg.func, args=[NamedParameter(args=arg.args[0].args), arg.args[1]]))
                    else:
                        rebuilt.append(arg)
                elif isinstance(arg, Assign):
                    rebuilt.append(NamedParameter(args=arg.args))
                elif isinstance(arg, RedundantParens) and isa_or_wrapped(arg.args[0], Assign):
                    rebuilt.append(arg.args[0])
                else:
                    rebuilt.append(arg)
            op.args = rebuilt

        op.args = [replacer(arg) for arg in op.args]
        return op

    return replacer(expr)


def warn_and_remove_redundant_parens(expr, error=False):

    def replacer(op):
        if isinstance(op, RedundantParens):
            op = op.args[0]
            msg = f"warning: redundant parens {op}"
            if error:
                raise SemanticAnalysisError(msg)
            else:
                print(msg, file=sys.stderr)
        if not isinstance(op, Node):
            return op
        op.args = [replacer(arg) for arg in op.args]
        op.func = replacer(op.func)
        return op

    return replacer(expr)


# class Environment:
#     pass


def build_environment(node):
    return node
    if not isinstance(node, Node):
        return
    if not hasattr(node, "scopes"):
        node.scopes = []

r"""

y = 5

x = 1

x = 2

x = y + 1

print(x)

"""



def _find_def(parent, child, node_to_find):
    def _find_assign(r, node_to_find):
        if not isinstance(r, Node):
            return None
        if isinstance(r, Block):
            return None
        if isinstance(r, Assign) and isinstance(r.lhs, Identifier) and r.lhs.name == node_to_find.name and r.lhs is not node_to_find:
            return r.lhs, r
        else:
            for a in r.args:
                if f := _find_assign(a, node_to_find):
                    return f
        return None

    if parent is None:
        return None
    if not isinstance(node_to_find, Identifier):
        return None
    if isinstance(parent, Module):
        return None
    elif isinstance(parent, Block):
        index = parent.args.index(child)
        preceding = parent.args[0:index]
        for r in reversed(preceding):
            # if isinstance(r, Assign) and isinstance(r.lhs, Identifier) and r.lhs.name == node_to_find.name:
            #     return r.lhs, r
            f = _find_assign(r, node_to_find)
            if f is not None:
                return f

        # call = parent.parent
        # assert isinstance(call, Call)
        # for a in call.args:
        #     if a.name == node_to_find.name:
        #         return a, call

        return _find_def(parent.parent, parent, node_to_find)
    elif isinstance(parent, Call):
        if parent.func.name == "def":
            for callarg in parent.args:
                if callarg.name == node_to_find.name and callarg is not node_to_find:
                    return callarg, parent
                elif isinstance(callarg, NamedParameter) and callarg.args[0].name == node_to_find.name:
                    return callarg, parent

        # index = node.parent.args.index(node)
        # if
        return _find_def(parent.parent, parent, node_to_find)
    elif isinstance(parent, Assign) and parent.lhs.name == node_to_find.name and parent.lhs is not node_to_find:
        return parent.lhs, parent
    else:
        return _find_def(parent.parent, parent, node_to_find)


def find_def(node):
    if not isinstance(node, Node):
        return None
    res = _find_def(node.parent, node, node)
    # print(res)
    # if res is not None and res[0] is node:
    #     return None
    return res


class LookupTable:
    def __int__(self):
        self.parent = None
        self.defs = {}

        pass


def build_scopes(expr):
    def visitor(node, outer_table):
        if not isinstance(node, Node):
            return node
        rebuilt = []

        if isinstance(node, Assign):
            outer_table.append(node)
        else:
            node.symbol_table = list(outer_table)  # shallow copy

        if isinstance(node, Block):
            for arg in node.args:
                if isinstance(node, Assign):
                    pass

        else:

            for arg in node.args:
                visitor(arg, node.symbol_table)

        #     if isinstance(arg, Node):
        #         if not hasattr(node, "symbol_table")
        #             node.symbol_table = []
        #         arg.parent = node
        #         # arg.rebuild(arg.func, arg.args)
        #         arg = visitor(arg)
        #     rebuilt.append(arg)
        # node.args = rebuilt
        # # node.rebuild(node.func, node.args)
        return node
    return visitor(expr, [])


def semantic_analysis(expr: Module):
    assert isinstance(expr, Module) # enforced by parser

    for modarg in expr.args:
        if isinstance(modarg, Call):
            if modarg.func.name not in ["def", "class"]:
                raise SemanticAnalysisError("Only defs or classes at module level (for now)")
        elif isinstance(modarg, Assign):
            pass
        else:
            raise SemanticAnalysisError("Only calls and assignments at module level (for now)")

    expr = one_liner_expander(expr)
    expr = assign_to_named_parameter(expr)
    expr = warn_and_remove_redundant_parens(expr)

    # remove all non-"type" use of ColonBinOp (scratch that for now - leave elif as lhs of ColonBinOp)
    # expr = build_if_nodes(expr) # in fact don't even do this

    # convert ColonBinOp (but not SyntaxColonBinOp) to types
    expr = build_types(expr)
    expr = build_parents(expr)
    # expr = build_scopes(node)

    print("after lowering", expr)

    def defs(node):
        if not isinstance(node, Node):
            return

        x = find_def(node)
        if x:
            print("found def", node, x)
        else:
            print("no def for", node)

        for a in node.args:
            defs(a)
            defs(a.func)

    defs(expr)

    return expr
