## Intro

**ceto** is an experimental programming language transpiled to C++ but inspired by Python in variable declaration style, \*syntax, safe(ish) reference semantics for `class`, and generic programming as an exercise in forgetting the type annotations. Syntactically, every control structure is a function call and every expression might be a macro.

```python
# see the tests and selfhost directories for more examples

include <ranges>

defmacro ([x, for (y in z), if (c)], x, y, z, c:
    result = gensym()
    zz = gensym()

    pre_reserve_stmt = if (isinstance(c, EqualsCompareOp) and std.ranges.any_of(
                           c.args, lambda(a, a.equals(x) or a.equals(y))):
        # Don't bother pre-reserving a std.size(z) sized vector for simple searches 
        # e.g. [x, for (y in z), if (y == something)]
        dont_reserve: Node = quote(pass)
        dont_reserve
    else:
        reserve: Node = quote(maybe_reserve(unquote(result), unquote(zz)))
        reserve
    )

    return quote(lambda (:

        unquote(result): mut = []  # immutable by default (mostly!), so mark it "mut"

        unquote(zz): mut:auto:ref:ref = unquote(z)  # explicit use of "auto" or "ref" requires
                                                    # an explicit "mut" or "const" annotation.
        unquote(pre_reserve_stmt)

        for (unquote(y) in unquote(zz):
            unquote(if (c.name() == "True":
                # Omit literal if (True) check (reduce clutter for 2-arg case below)
                quote(unquote(result).append(unquote(x)))
            else:
                quote(if (unquote(c):
                    unquote(result).append(unquote(x))
                ))
            ))
        )

        unquote(result)
    ) ())
)

defmacro ([x, for (y in z)], x, y, z:
    # Use the existing 3-arg definition
    return quote([unquote(x), for (unquote(y) in unquote(z)), if (True)])
)

# metaprogramming in ceto/C++ templates rather than procedural macros is recommended:

def (maybe_reserve<T>, vec: mut:[T]:ref, sized: mut:auto:ref:ref:
    vec.reserve(std.size(std.forward<decltype(sized)>(sized)))
) : void:requires:requires(std.size(sized))

def (maybe_reserve<T>, vec: mut:[T]:ref, unsized: mut:auto:ref:ref:
    pass
) : void:requires:not requires(std.size(unsized))
```

```python
include <ranges>
include <iostream>
include <numeric>
include <future>
include <map>

include (macros_list_comprehension)

# You can also do your metaprogramming by omitting the type annotations!
# Classes have immutable shared reference semantics by default
# (A class with generic data members is implicitly a C++ template class:
#  - like an immutable by default Python 2 with good C++ interop/baggage and more parenthesese)

class (Foo:
    data_member

    def (method, param:
        std.cout << param.size() << std.endl
        return self  # implicit +1 refcount (shared_from_this)
    )

    def (size:
        return self.data_member.size()
    )
)

# A non type-annotated ceto function is implicitly a sligtly more
# unconstrained/generic than default C++ template function
# (calls_method's arg is generic over all ceto class types and structs
#  because "." is a maybe autoderef)

def (calls_method, arg:
    return arg.method(arg)
)

# Unique classes are implicitly managed by std.unique_ptr and use cppfront inspired 
# move from last use. Instance variables may be reassigned (allowing implicit move) but 
# point to immutable instances (aka unique_ptr to const by default)

class (UniqueFoo:
    consumed: [UniqueFoo] = []

    def (size:
        return self.consumed.size()
    )
    
    # For all classes and structs, a method that mutates its data members must be "mut".
    # Note that "u" is a passed by value std::unique_ptr<const UniqueFoo> in C++
    def (consuming_method: mut, u: UniqueFoo:
        
        # u.consuming_method(None)  # Compile time error:
        # "u" is not a UniqueFoo:mut so a "mut" method call is illegal

        # "u" is passed by reference to const to the generic method "method" here.
        Foo(42).method(u)

        self.consumed.append(u)  # Ownership transfer of "u" on last use (implicit std.move)
    )
) : unique

# std.string and vectors (implicit use of std.vector with square brackets) passed 
# by reference to const by default:
def (string_join, vec: [std.string], sep = ", "s:
    if (vec.empty():
        return ""
    )

    # Explicit lambda capture brackets required for 
    # unsafe (potentially dangling) ref capture:
    return std.accumulate(vec.cbegin() + 1, vec.cend(), vec[0],
        lambda[&sep] (a, b, a + sep + b))
): std.string

# defmacro param types use the ast Node subclasses defined in selfhost/ast.cth

defmacro (s.join(v), s: StringLiteral, v:
    return quote(string_join(unquote(v), unquote(s)))
)

def (main, argc: int, argv: const:char:ptr:const:ptr:

    # macro invocations:
    args = [std.string(a), for (a in std.span(argv, argc))]
    summary = ", ".join(args)

    f = Foo(summary)  # implicit make_shared / extra CTAD:
                      # in C++ f is a const std::shared_ptr<const Foo<decltype(summary)>>

    f.method(args)    # autoderef of f
    f.method(f)       # autoderef also in the body of 'method'
    calls_method(f)   # autoderef in the body of calls_method (and method)

    fut: mut = std.async(std.launch.async, lambda(:

        # Implicit copy capture (no capture list specified) for shared/weak
        # ceto-class instances, arithmetic types, and enums only.

        f.method(f).data_member
    ))

    std.cout << fut.get()

    u: mut = UniqueFoo()    # u is a "non-const" std::unique_ptr<non-const UniqueFoo> in C++
    u2 = UniqueFoo()        # u2 is a non-const std::unique_ptr<const UniqueFoo> in C++

    u.consuming_method(u2)  # Implicit std.move from last use of u2.
                            # :unique are non-const (allowing move) but
                            # unique_ptr-to-const by default.

    u.consuming_method(u)   # in C++: CETO_AUTODEREF(u).consuming_method(std::move(u))
)
```

## Usage

```bash
$ git clone https://github.com/ehren/ceto.git
$ cd ceto
$ pip install .
$ ceto ./tests/example.ctp a b c
./example a b c
4
18
18
18
./example, a, b, c
0
1
```

### Continued Intro

While you can express a great deal of existing C++ constructs in ceto code (you can even write ceto macros that output, and rely on for their compiled to DLL implementation, a mix of C++ template and C/C++ preprocessor metaprogramming - or even other ceto macros!) the emphasis is not on system programming but more so called "Pythonic glue code" (whether it's a good idea to write such code in C++ to begin with). One should be able to translate ordinary Python code to pythonic ceto just by adding the necessary parenthesese and viral ```:mut``` annotations but without worrying about additional complicated parameter passing rules, explicit reference/pointer type annotations, nor call site referencing/dereferencing syntax. While e.g. the keywords ```ref```, ```ptr``` and operator ```->``` and unary operators```*``` and ```&``` exist in ceto (for expressing native C++ constructs and interfacing with external C++) they should be regarded as constituents of a disconnected low level subset of the language that will even TODO require explicit ```unsafe``` blocks/contexts in the future. Though, lacking a complete ```unsafe``` blocks implementation, current ceto should be regarded as *unsafe ceto*, runtime safety checks are nevertheless performed for pythonic looking code: ```.``` (when a smart pointer or optional deref) is null checked and ```array[index]``` is runtime bounds checked when array is a contiguous container (the technique of checking if std::size is available for ```array``` using a ceto/C++ ```requires``` clause before inserting a runtime bounds check has been taken from Herb Sutter's cppfront - see include/boundscheck.cth).

### More Features:

- reference to const and value semantics emphasized.
- Python like class system but immutable by default (more like an immutable by default Java halfway naively implemented with std::shared_ptr and std::enable_shared_from_this).
- structs passed by reference to const by default, by value if just "mut" (raw pointers or references allowed via ref or ptr keywords but may be relegated to unsafe blocks in the future)
- Implicit swiftish lambda capture
- Implicit move from last use of unique (and TODO non-class mut)
- non-type annotated "Python" == unconstrained C++ function and class templates
- extra CTAD

## Features Explanation

### Autoderef (use *.* not *->*)

This works by compiling a generic / non-type-annotated function like

```python
def (calls_foo, f:
    return f.foo()
)
```

to the C++ template function

```c++
#include <ceto.h>

auto calls_foo(const auto& f) -> auto {
    return (*ceto::mad(f)).foo();
}
```

where `ceto::mad` (maybe allow dereference) amounts to just `f` (allowing the dereference via `*` to proceed) when `f` is a smart pointer or optional, otherwise returning the `std::addressof` of `f` to cancel the dereference for anything else (more or less equivalent to ordinary attribute access `f.foo()` in C++). This is adapted from this answer: https://stackoverflow.com/questions/14466620/c-template-specialization-calling-methods-on-types-that-could-be-pointers-or/14466705#14466705 except the ceto implementation (see include/ceto.h) avoids raw pointer autoderef (you may still use `*` and `->` when working with raw pointers). When `ceto::mad` allows a dereference, it also performs a terminating nullptr check (use `->` for an unsafe unchecked access).

### Less typing (at least as in your input device\*)

This project uses many of the ideas from the wonderful https://github.com/lukasmartinelli/py14 project such as the implicit insertion of *auto* (though in ceto it's implict *const auto* for untyped locals and *const auto&* for untyped params). The very notion of generic python functions as C++ template functions is also largely the same.

We've also derived our code generation of Python like lists as *std.vector* from the project.

For example, from [their README](https://github.com/lukasmartinelli/py14?tab=readme-ov-file#how-it-works):

```python
# Test Output: 123424681234123412341234


def (map, values, fun:
    results: mut = []
    for (v in values:  # implicit const auto&
        results.append(fun(v))
    )
    return results
)

def (foo, x:int:
    std.cout << x
    return x
)

def (foo_generic, x:
    std.cout << x
    return x
)

def (main:
    l = [1, 2, 3, 4]  # definition simply via CTAD (unavailable to py14)
    map(map(l, lambda (x:
        std.cout << x
        x*2
    )), lambda (x:
        std.cout << x
        x
    ))
    map(l, foo)
    # map(l, foo_generic)  # error
    map(l, lambda (x:int, foo_generic(x)))  # when lambda arg is typed, clang 14 -O3 produces same code as passing foo_generic<int>)
    map(l, lambda (x, foo_generic(x)))  # Although we can trick c++ into deducing the correct type for x here clang 14 -O3 produces seemingly worse code than passing foo_generic<int> directly. 
    map(l, foo_generic<int>)  # explicit template syntax
)
```

Though, we require a *mut* annotation and rely on *std.ranges*, the wacky forward inference via *decltype* to codegen the type of results above as *std::vector<decltype(fun(std::declval<std::ranges::range_value_t<decltype(values)>>()))>* derives from the py14 implementation.

(*tempered with the dubiously attainable goal of less typing in the language implementation)

### Classes, Inheritance

Class definitions are intended to resemble Python dataclasses

```python
# Test Output: 5555.0one

include <map>
include <string>

class (Generic:
    x  # implicit 1-arg constructor, deleted 0-arg constructor
)

class (Concrete(Generic):
    def (init, x: int:
        super.init(x)
    )
)

class (Generic2(Generic):
    y
    def (init, x, y:
        self.y = y
        super.init(x)
    )
)

class (Concrete2(Concrete):
    # no user defined init - inherits constructors
    pass
)

def (main:
    f = Generic("5")
    f2 = Concrete(5)
    #f2e = Concrete("5")  # error
    f3 = Generic2([5, 6], std.map<int, std.string> { {1, "one"} })
    f4 = Concrete2(42)
    std.cout << f.x << f2.x << f3.x[0] << f3.y.at(1) << f4.x
)
```

### Tuples, "tuple unpacking" (std::tuple / structured bindings / std::tie)

```python
# Test Output: 01
# Test Output: 12
# Test Output: 23
# Test Output: 34
# Test Output: 00
# Test Output: 56
# Test Output: 12
# Test Output: 71
# Test Output: 89
# Test Output: 910
# Test Output: 01

include <ranges>
include <iostream>

def (foo, tuple1: (int, int), tuple2 = (0, 1):
    # TODO perhaps Python like tuple1[0] notation for transpiler known tuples
    return (std.get<0>(tuple1), std.get<1>(tuple2))
)

def (main:
    tuples: mut = []

    for (i in std.ranges.iota_view(0, 10):
        tuples.append((i, i + 1))
    )

    (a, b) = (tuples[0], tuples[1])
    tuples.append(a)

    (tuples[4], tuples[6]) = ((0, 0), b)

    (std.get<0>(tuples[7]), std.get<1>(tuples[7])) = foo(tuples[7])

    for ((x, y) in tuples:  # const auto&
        std.cout << x << y << "\n"
    )

    for ((x, y):mut:auto:ref in tuples:  # auto&
        x += 1
        y += 2
    )

    for ((x, y):mut in tuples:  # just auto
        static_assert(std.is_same_v<decltype(x), int>)
        static_assert(std.is_same_v<decltype(y), int>)
    )
)
```

### Shared / weak

```python

# Test Output: action
# Test Output: action
# Test Output: action
# Test Output: Delegate destruct
# Test Output: Timer destruct

include <thread>

class (Delegate:
    def (action:
        std.cout << "action\n"
    )

    def (destruct:
        std.cout << "Delegate destruct\n"
    )
)

class (Timer:
    _delegate: Delegate

    _thread: std.thread = {}

    def (start: mut:
        w: weak:Delegate = self._delegate

        self._thread = std.thread(lambda(:
            while (True:
                std.this_thread.sleep_for(std.chrono.seconds(1))
                if ((s = w.lock()):  # implicit capture of "w"
                    s.action()
                else:
                    break
                )
            )
        ))
    )

    def (join: mut:
        self._thread.join()
    )

    def (clear_delegate: mut:
        self._delegate = None
    )

    def (destruct:
        std.cout << "Timer destruct\n"
    )
)

def (main:
    timer: mut = Timer(Delegate())
    timer.start()

    std.literals: using:namespace
    std.this_thread.sleep_for(3.5s)

    timer.clear_delegate()
    timer.join()
)
```

### Simple Visitor

This example demonstrates non-trivial use of self and mutable ceto-class instances

```python
class (Node)
class (Identifier)
class (BinOp)
class (Add)

class (Visitor:

    def (visit: virtual:mut, node: Node): void = 0

    def (visit: virtual:mut, node: Identifier): void = 0

    def (visit: virtual:mut, node: BinOp): void = 0

    def (visit: virtual:mut, node: Add): void = 0
)

class (Node:
    loc : int

    def (accept: virtual, visitor: Visitor:mut:
        visitor.visit(self)
    )
)

class (Identifier(Node):
    name : std.string

    def (init, name, loc=0:
        # a user defined constructor is present - 1-arg constructor of Node is not inherited
        self.name = name  # implicitly occurs in initializer list
        super.init(loc)   # same
    )

    def (accept: override, visitor: Visitor:mut:
        visitor.visit(self)
    )
)

class (BinOp(Node):
    args : [Node]

    def (init, args, loc=0:
        self.args = args
        super.init(loc)
    )

    # Note the virality of mut annotations:
    # (visitor must be a mut:Visitor because visit modifies the data member "record")

    def (accept: override, visitor: Visitor:mut:
        visitor.visit(self)
    )
)

class (Add(BinOp):
    # inherits 2-arg constructor from BinOp (because no user defined init is present)

    def (accept: override, visitor: Visitor:mut:
        visitor.visit(self)
    )
)

class (SimpleVisitor(Visitor):
    record = s""

    def (visit: override:mut, node: Node:
        self.record += "visiting Node\n"
    )

    def (visit: override:mut, ident: Identifier:
        self.record += "visiting Identifier " + ident.name + "\n"
    )

    def (visit: override:mut, node: BinOp:
        self.record += "visiting BinOp\n"

        for (arg in node.args:
            arg.accept(self)  # non-trivial use of self (hidden shared_from_this)
        )
    )

    def (visit: override:mut, node: Add:
        self.record += "visiting Add\n"

        for (arg in node.args:
            arg.accept(self)
        )
    )
)

def (main:
    node = Node(0)
    ident = Identifier("a", 5)
    args: [Node] = [ident, node, ident]
    add: Add = Add(args)

    simple_visitor: mut = SimpleVisitor()
    ident.accept(simple_visitor)
    add.accept(simple_visitor)

    std.cout << simple_visitor.record
)

# Output:
# visiting Identifier a
# visiting Add
# visiting Identifier a
# visiting Node
# visiting Identifier a
```

Note that this example illustrates mutable class instance variables, especially as
function parameters e.g. ```visitor``` of ```accept```.  However, compared to 
idiomatic C++ code, there is a considerable runtime overhead (though some safety benefits) in making
```Visitor``` and ```SimpleVisitor``` ceto-classes rather than ceto-structs (see below).

In selfhost/ast.cth and selfhost/visitor.cth, ```Visitor``` is defined as a ```struct``` and
the ```accept``` methods take ```visitor``` by ```mut:ref```:

So, for example, the top level ast node ```Module``` is defined in selfhost/ast.cth as:

```python
class (Module(Block):
    has_main_function = False

    def (accept: override, visitor: Visitor:mut:ref:
        visitor.visit(*this)
    )

    def (clone: override:
        c: mut = Module(self.cloned_args(), self.source)
        return c
    ) : Node:mut
)
```

This ```accept``` has better runtime perforance than ```SimpleVisitor```'s class heavy version above but 
note that raw pointer dereference e.g. ```*this``` and mutable C++ references in
function params (and elsewhere!) should be / will be TODO relegated to unsafe blocks!

### struct

"The struct is a class notion is what has stopped C++ from drifting into becoming a much higher level language with a disconnected low-level subset." - Bjarne Stroustrup

```python

include<string>

struct (Foo:
    x: std.string
)

def (by_const_ref, f: Foo:  # pass by const ref
    static_assert(std.is_same_v<decltype(f), const:Foo:ref>)
    static_assert(std.is_reference_v<decltype(f)>)
    static_assert(std.is_const_v<std.remove_reference_t<decltype(f)>>)
    std.cout << f.x
)

def (by_val, f: Foo:mut:  # pass by value (mut:Foo also fine)
    static_assert(std.is_same_v<decltype(f), Foo>)
    static_assert(not std.is_reference_v<decltype(f)>)
    static_assert(not std.is_const_v<std.remove_reference_t<decltype(f)>>)
    std.cout << f.x
)

def (by_const_val, f: Foo:const:  # pass by const value (west const also acceptable)
    # TODO this should perhaps be pass by const ref instead (or an error!) - bit of a perf gotcha. Same problem with std.string and [T])
    # Note that for the class case - Foo and Foo:mut are both passed by const ref (to shared_ptr)
    static_assert(std.is_same_v<decltype(f), const:Foo>)
    static_assert(not std.is_reference_v<decltype(f)>)
    static_assert(std.is_const_v<std.remove_reference_t<decltype(f)>>)
    std.cout << f.x
)

def (by_mut_ref, f: Foo:ref:mut:  # pass by non-const reference (mut:Foo:ref also fine - west mut)
    static_assert(std.is_same_v<decltype(f), Foo:ref>)
    static_assert(std.is_reference_v<decltype(f)>)
    static_assert(not std.is_const_v<std.remove_reference_t<decltype(f)>>)
    f.x += "hi"
    std.cout << f.x
)

# TODO: Note that using fully notated const pointers like below is recommended for all ceto code. 
# The const by default (unless :unique) for function parameters feature behaves a bit like add_const_t currently
# (the multiple mut syntax "mut:Foo:ptr:mut" is not even currently supported for Foo** in C++ -
#  while mut:Foo:ptr or Foo:ptr:mut works currently, future ceto versions may require additional mut/const annotations)
def (by_ptr, f: const:Foo:ptr:const:
    static_assert(std.is_same_v<decltype(f), const:Foo:ptr:const>)
    std.cout << f->x  # no autoderef for raw pointers
)

def (main:
    f = Foo("blah")
    by_const_ref(f)
    by_val(f)
    by_const_val(f)
    # by_mut_ref(f)  # error: binding reference of type ‘Foo&’ to ‘const Foo’ discards qualifiers
    fm : mut = f  # copy
    by_mut_ref(fm)
    by_ptr(&f)
)
```

### std.optional autoderef

In this example, ```optional_map.begin()``` suffices where C++ would require ```optional_map.value().begin()```:

```python
include <iostream>
include <map>
include <optional>

def (main:
    optional_map: std.optional<std.map<std.string, int>> = std.map<std.string, int> {
        {"zero", 0}, {"one", 1}}

    if (optional_map:
        updated: mut:std.map<std.string, int> = {{ "two", 2}}

        # Autoderef
        updated.insert(optional_map.begin(), optional_map.end())

        updated["three"] = 3
        for ((key, value) in updated:
            std.cout << key << value
        )
    )
)
```

For ```std.optional``` instances, no deref takes place when calling a method of ```std.optional```. That is, to call a method `value()` on the underlying value rather than the optional call `.value().value()`.

(this example also illustrates that for ceto classes and structs round parenthese must be used e.g.  ```Foo(x, y)``` even though the generated code makes use of curlies e.g. ```Foo{x, y}``` (to avoid narrowing conversions). For external C++ round means round - curly means curly (```std.vector<int>(50, 50)``` is a 50 element vector of 50)

### Evading autoderef

In contrast to the behavior of optionals above, for "class instances" or even explicit std.shared/unique_ptrs you must use a construct like 

```python
(&o)->get()
```

to get around the autoderef system and call the smart ptr `get` method (rather than a `get` method on the autoderefed instance).

Complete example:

```python
class (Foo:
    def (bar:
        std.cout << (&self)->use_count()             # +1 to use_count (non-trivial use of self)
        std.cout << lambda((&self)->use_count()) ()  # +1 copy capture of self
                                                     # note: this capture requires a lambda[&this] capture list
    )
)

def (main:
    f = Foo()
    f.bar()

    (refcount, addr) = lambda (:
        ((&f)->use_count(), (&f)->get())  # +1 copy capture of f
    ) ()

    std.cout << refcount << addr->bar()
)
```

Requiring the `&` and `->` syntax in these cases has the added benefit of signaling unsafety (a fully safe ceto would require no additional logic to ban all potentially unsafe use of smart pointer member functions outside of unsafe blocks: they're banned automatically by banning any occurence of operators ```*```, ```&```, and ```->``` outside of unsafe blocks).

### C++ templates

Writing simple templates can be achieved by Python style "generic" functions (see the first example). Explicit C++ template functions, classes, and variables may still be written:

```python
include <ranges>
include <algorithm>

namespace(myproject.utils)  # everything that follows (in this file only) is defined in this C++ namespace

# explicit template function
def (range: template<typename:...:Args>, args: mut:Args:rref:...:
    if ((sizeof...)(Args) == 1:
        zero : typename:std.tuple_element<0, std.tuple<Args...>>::type = 0
        return std.ranges.iota_view(zero, std.forward<Args>(args)...)
    else:
        return std.ranges.iota_view(std.forward<Args>(args)...)
    ) : constexpr
) : decltype(auto)

# generic "Python" style function (container is const auto&)
def (contains, container, element: const:typename:std.remove_reference_t<decltype(container)>::value_type:ref:
    return std.find(container.cbegin(), container.cend(), element) != container.cend()
)

# additional nested namespaces require a block:
namespace(extra.detail:

    # template variable example from https://stackoverflow.com/questions/69785562/c-map-and-unordered-map-template-parameter-check-for-common-behavior-using-c/69869007#69869007
    is_map_type: template<class:T>:concept = std.same_as<typename:T.value_type, std.pair<const:typename:T.key_type, typename:T.mapped_type>>
)
```

Assuming the above is written to a file myprojectutils.cth, we can include it:

```python
include <map>
include (myprojectutils)

def (main:
    for (x in myproject.utils.range(5, 10):
        if (myproject.utils.contains([2, 4, 6], x):
            std.cout << x
        )
    )

    m: std.unordered_map<string, int> = {}
    static_assert(myproject.utils.extra.detail.is_map_type<decltype(m)>)
)
```

### Macros

Macros should be used sparingly for extending the language. When possible, C++ templates or preprocessor macros should be preferred.

Macros are unhygienic (use gensym for locals to avoid horrific capture bugs). Automatic hygiene at least for simple local variables as well as automatic unquoting of params might be implemented in the future.

Continuing with our example of ```pyprojectutils.cth```, we can create a header called ```incontains.cth```:

```python
include (myprojectutils)

defmacro(a in b, a, b:
    if ((call = asinstance(a.parent().parent(), Call)):
        if (call.func.name() == "for":
            # don't rewrite the "in" of a for-in loop (pitfall of a general syntax!)
            return None
        )
    )

    # std.ranges.contains would be better if you're using c++23
    return quote(myproject.utils.contains(unquote(b), unquote(a)))
)
```

and include it:

```python
include <iostream>
include (incontains)
include (myprojectutils)  # unnecessary because incontains.cth already includes 
                          # (but good style to include what you use)

def (main:
    for (x in myproject.utils.range(10):
        if (x in [2, 4, 6]:
            std.cout << x
        )
    )
)
```

Once std.ranges.contains is accepted on all github actions runners in c++23 mode, we'll likely add our in-macro as a built-in. Note that redefining macros is acceptable (the latest definition in scope gets the first attempt at a match).

#### Alternational arguments

We can't dynamically redefine integer constants like Python (https://hforsten.com/redefining-the-number-2-in-python.html) but the next best thing is possible if not recommended:

```python
# Test Output: 1
# Test Output: 2
# Test Output: 1.5
# Test Output: 1.6

include <iostream>
include <cstdlib>

defmacro (a, a: IntegerLiteral|FloatLiteral:

    # getting at the alternatives requires downcasting
    # ('match' syntax and defmacro(..., elif ..., else, ...) a future possibility)
    if ((i = asinstance(a, IntegerLiteral)):
        if (i.integer_string == "2":
            # 2 is 1
            return quote(1)
        )
    else:
        f = asinstance(a, FloatLiteral)
        d = std.strtod(f.float_string.c_str(), nullptr)
        if (d >= 2.0 and d <= 3.0:
            # subtract 0.5 for kicks
            suffix = quote(l)
            n = FloatLiteral(std.to_string(d - 0.5), suffix)
            return n
        )
    )

    return None
)

def (main:
    std.cout << 2 << "\n"
    std.cout << 2 + 1 << "\n"
    std.cout << 1.5 << "\n"
    std.cout << 2.5 + 0.1 << "\n"  # Macro expansion iterates to a fixed point:
                                   # One application rewrites 2.5 to 2.0f, a second to 1.5f; no changes on third pass
)
```

#### Variadic arguments

```python
include <ranges>
include <iostream>

defmacro (summation(args), args: [Node]:
    if (not args.size():
        return quote(0)
    )

    if (defined(__clang__) and __clang_major__ < 16 and __APPLE__:
        # The below ranges example is still likely busted with the github actions runner's xcode apple clang 14

        sum = std.accumulate(args.cbegin() + 1, args.cend(), args[0], lambda(a, b, quote(unquote(a) + unquote(b))))
    else:
        sum: mut = args[0]
        for (arg in args|std.views.drop(1):
            sum = quote(unquote(sum) + unquote(arg))
        )
    ) : preprocessor

    return sum
)

def (main:
    std.cout << summation(1, 2, 3)
    c = "c"
    std.cout << summation("a"s, "b", c) << summation(5) << summation()
)
```

#### Optional arguments

Here's an example from the standard library macros located in the include directory. We use an optional match var for "specifier" to match virtual and otherwise decorated destructors as well as plain non-virtual destructors with a single macro pattern:

```python
# canonical empty destructor to default destructor:
# e.g.
# def (destruct:virtual:
#     pass
# )
# goes to
# def (destruct:virtual) = default
# For an empty non-default destructor
# use pass; pass
defmacro (def (destruct:specifier:
    pass
), specifier: Node|None:
    name: Node = quote(destruct)
    destructor = if (specifier:
        specified: Node = quote(unquote(name): unquote(specifier))
        specified
    else:
        name
    )
    return quote(def (unquote(destructor)) = default)
)
```

```python
# No "includes" needed to make use of the standard library macros

struct (Foo1:
    def (destruct:
        pass
        # pass  # uncomment for a non-default destructor
    )
)

struct (Foo2:
    # you may still write an explicitly default destructor if you must
    def (destruct:virtual) = default
)

class (Foo3:
    # non-None "specifier" match
    def (destruct:virtual:
        pass
    )
)

def (main:
    static_assert(not std.has_virtual_destructor_v<Foo1>)
    static_assert(std.has_virtual_destructor_v<Foo2>)
    static_assert(std.has_virtual_destructor_v<Foo3.class>)
)
```

### Kitchen Sink / mixing higher level and lower level ceto / external C++

Contrasting with the "Java style" / shared_ptr heavy visitor pattern shown above, the selfhost sources use a lower level version making use of C++ CRTP as well as the ```Foo.class``` syntax to access the underlying ```Foo``` in C++ (rather than ```shared_ptr<const Foo>```). This sidesteps the gotcha that ceto class instances aren't real "shared smart references" so **overriding** e.g. ```def (visit:override, node: BinOp)``` with ```def(visit: override, node: Add)``` is not possible because an **Add** (```std::shared_ptr<const Node>``` in C++) is not strictly speaking a derived class of ```std::shared_ptr<const BinOp>``` in C++. 

This code also demonstrates working with external C++ and more general/unsafe constructs like C++ iterators, raw pointers in combination with :unique classes, the C/C++ preprocessor, function pointers, and reinterpret_cast. This is an earlier version of the current selfhost/macro_expansion.cth:

```python

include <map>
include <unordered_map>
include <ranges>
include <functional>
include <span>

include (ast)
include (visitor)
include (range_utility)

if (_MSC_VER:
    include <windows.h>
    cpp'
    #define CETO_DLSYM GetProcAddress
    #define CETO_DLOPEN LoadLibraryA
    #define CETO_DLCLOSE FreeLibrary
    '
else:
    include <dlfcn.h>
    cpp'
    #define CETO_DLSYM dlsym
    #define CETO_DLOPEN(L) dlopen(L, RTLD_NOW)
    #define CETO_DLCLOSE dlclose
    '
) : preprocessor

struct (SemanticAnalysisError(std.runtime_error):
    pass
)

class (MacroDefinition:
    defmacro_node: Node
    pattern_node: Node
    parameters: std.map<string, Node>
    dll_path: std.string = {}
    impl_function_name: std.string = {}
)

class (MacroScope:
    parent: MacroScope.class:const:ptr = None

    macro_definitions: [MacroDefinition] = []

    def (add_definition: mut, defn: MacroDefinition:
        self.macro_definitions.push_back(defn)
    )

    def (enter_scope:
        s: mut = MacroScope()
        s.parent = this
        return s
    ) : MacroScope:mut
) : unique


def (macro_matches, node: Node, pattern: Node, params: const:std.map<std.string, Node>:ref:
    std.cout << "node: " << node.repr() << " pattern: " << pattern.repr() << "\n"

    if (isinstance(pattern, Identifier):
        search = params.find(pattern.name().value())
        if (search != params.end():

            param_name = search->first
            matched_param = search->second
            if (isinstance(matched_param, Identifier):
                # wildcard match
                return std.map<std.string, Node>{{param_name, node}}
            elif (typeop = asinstance(matched_param, TypeOp)):
                param_type = typeop.rhs()

                # constrained wildcard / match by type
                if (isinstance(param_type, Identifier):
                    if ((param_type.name() == "BinOp" and isinstance(node, BinOp) or  # base class handling
                         param_type.name() == "UnOp" and isinstance(node, UnOp) or    # same
                         param_type.name() == "Node" or                               # redundant but allowed
                         node.classname() == typeop.rhs().name()):                    # exact match
                        return std.map<std.string, Node>{{param_name, node}}
                    )
                elif (or_type = asinstance(param_type, BitwiseOrOp)):
                    lhs_alternate_param: std.map<std.string, Node> = { {param_name, TypeOp(":", [matched_param, or_type.lhs()])} }
                    if ((m = macro_matches(node, pattern, lhs_alternate_param)):
                        return m
                    )
                    rhs_alternate_param: std.map<std.string, Node> = { {param_name, TypeOp(":", [matched_param, or_type.rhs()])} }
                    if ((m = macro_matches(node, pattern, rhs_alternate_param)):
                        return m
                    )
                )
            )
        )
    )

    if (typeid(*node) != typeid(*pattern):
        return {}
    )

    if ((node.func == None) != (pattern.func == None):
        return {}
    )

    if (node.args.size() == 0 and node.func == None and pattern.func == None:
        if (node.repr() == pattern.repr():
            # leaf match
            return std.map<std.string, Node>{}
        )
        return {}
    )

    submatches: mut = std.map<std.string, Node> {}

    if (node.func:
        m = macro_matches(node.func, pattern.func, params)
        if (not m:
            return {}
        )
        submatches.insert(m.begin(), m.end())  # std::optional autoderef
    )

    pattern_iterator: mut = pattern.args.cbegin()
    arg_iterator: mut = node.args.cbegin()

    while (True:
        if (pattern_iterator == pattern.args.end():
            if (arg_iterator != node.args.end():
                # no match - no pattern for args
                return {}
            else:
                break
            )
        )

        subpattern: mut = *pattern_iterator

        if (isinstance(subpattern, Identifier):
            search = params.find(subpattern.name().value())

            if (search != params.end():
                param_name = search->first
                matched_param = search->second

                if (isinstance(matched_param, TypeOp):
                    if ((list_param = asinstance(matched_param.args[1], ListLiteral)):
                        # variadic wildcard pattern

                        if (list_param.args.size() != 1:
                            throw (SemanticAnalysisError("bad ListLiteral args in macro param"))
                        )

                        wildcard_list_type = list_param.args[0]
                        if (not isinstance(wildcard_list_type, Identifier):
                            throw (SemanticAnalysisError("bad ListLiteral arg type in macro param"))
                        )

                        wildcard_list_name = matched_param.args[0]
                        if (not isinstance(wildcard_list_name, Identifier):
                            throw (SemanticAnalysisError("arg of type ListLiteral must be an identifier"))
                        )

                        wildcard_type_op = TypeOp(":", [wildcard_list_name, wildcard_list_type]: Node)
                        wildcard_list_params: std.map<std.string, Node> = { {wildcard_list_name.name().value(), wildcard_type_op} }
                        wildcard_list_matches: mut:[Node] = []

                        while (arg_iterator != node.args.end():
                            arg = *arg_iterator
                            if (macro_matches(arg, wildcard_list_name, wildcard_list_params):
                                wildcard_list_matches.append(arg)
                            else:
                                break
                            )
                            arg_iterator += 1
                        )

                        submatches[param_name] = ListLiteral(wildcard_list_matches)

                        pattern_iterator += 1
                        if (pattern_iterator == pattern.args.end():
                            if (arg_iterator != node.args.end():
                                # no match - out of patterns, still have args
                                return {}
                            )
                            break
                        )
                        subpattern = *pattern_iterator
                    )
                )
            )
        )

        if (arg_iterator == node.args.end():
            if (pattern_iterator != pattern.args.end():
                # no match - out of args, still have patterns
                return {}
            )
            break
        )

        arg = *arg_iterator
        m = macro_matches(arg, subpattern, params)
        if (not m:
            return {}
        )
        submatches.insert(m.begin(), m.end())

        arg_iterator += 1
        pattern_iterator += 1
    )

    return submatches
) : std.optional<std.map<std.string, Node>>


def (call_macro_impl, definition: MacroDefinition, match: const:std.map<std.string, Node>:ref:
    handle = CETO_DLOPEN(definition.dll_path.c_str())  # just leak it for now
    if (not handle:
        throw (std.runtime_error("Failed to open macro dll: " + definition.dll_path))
    )
    fptr = CETO_DLSYM(handle, definition.impl_function_name.c_str())
    if (not fptr:
        throw (std.runtime_error("Failed to find symbol " + definition.impl_function_name + " in dll " + definition.dll_path))
    )
    f = reinterpret_cast<decltype(+(lambda(m: const:std.map<std.string, Node>:ref, None): Node))>(fptr)  # no explicit function ptr syntax yet/ever(?)
    return (*f)(match)
) : Node


struct (MacroDefinitionVisitor(BaseVisitor<MacroDefinitionVisitor>):
    on_visit_definition: std.function<void(MacroDefinition, const:std.unordered_map<Node, Node>:ref)>

    current_scope: MacroScope:mut = None
    replacements: std.unordered_map<Node, Node> = {}

    def (expand: mut, node: Node:
        scope: mut:auto:const:ptr = (&self.current_scope)->get()
        while (scope:
            for (definition in ceto.util.reversed(scope->macro_definitions):
                match = macro_matches(node, definition.pattern_node, definition.parameters)
                if (match:
                    std.cout << "found match\n"
                    replacement = call_macro_impl(definition, match.value())
                    if (replacement and replacement != node:
                        std.cout << "found replacement for " << node.repr() << ": " << replacement.repr() << std.endl
                        self.replacements[node] = replacement
                        replacement.accept(*this)
                        return True
                    )
                )
            )
            scope = scope->parent
        )
        return False
    )

    def (visit: override:mut, node: Node.class:
        if (self.expand(ceto.shared_from(&node)):
            return
        )

        if (node.func:
            node.func.accept(*this)
        )

        for (arg in node.args:
            arg.accept(*this)
        )
    )

    def (visit: override:mut, call_node: Call.class:
        node = ceto.shared_from(&call_node)
        if (self.expand(node):
            return
        )

        node.func.accept(*this)

        for (arg in node.args:
            arg.accept(*this)
        )

        if (node.func.name() != "defmacro":
            return
        )

        if (node.args.size() < 2:
            throw (SemanticAnalysisError("bad defmacro args"))
        )

        pattern = node.args[0]

        if (not isinstance(node.args.back(), Block):
            throw (SemanticAnalysisError("last defmacro arg must be a Block"))
        )

        parameters: mut = std.map<std.string, Node>{}

        if (defined(__clang__) and __clang_major__ < 16:
            match_args = std.vector(node.args.cbegin() + 1, node.args.cend() - 1)
        else:
            match_args = std.span(node.args.cbegin() + 1, node.args.cend() - 1)
        ) : preprocessor

        for (arg in match_args:
            name = if (isinstance(arg, Identifier):
                arg.name().value()
            elif not isinstance(arg, TypeOp):
                throw (SemanticAnalysisError("bad defmacro param type"))
            elif not isinstance(arg.args[0], Identifier):
                throw (SemanticAnalysisError("bad typed defmacro param"))
            else:
                arg.args[0].name().value()
            )
            i = parameters.find(name)
            if (i != parameters.end():
                throw (SemanticAnalysisError("duplicate defmacro params"))
            )
            parameters.emplace(name, arg)
        )

        defn = MacroDefinition(node, pattern, parameters)
        self.current_scope.add_definition(defn)
        self.on_visit_definition(defn, self.replacements)
    )

    def (visit: override:mut, node: Module.class:
        s: mut = MacroScope()
        self.current_scope = s

        for (arg in node.args:
            arg.accept(*this)
        )
    )

    def (visit: override:mut, node: Block.class:
        outer: mut:MacroScope = std.move(self.current_scope)
        self.current_scope = outer.enter_scope()
        if (self.expand(ceto.shared_from(&node)):
            return
        )
        for (arg in node.args:
            arg.accept(*this)
        )
        self.current_scope = outer  # automatic move from last use
        # TODO: if outer is just 'mut' above we should still automatically std::move it? OTOH maybe not - keep need for an explicit type for something that is to be auto moved? Also, if you just write "outer2 = outer": Currently outer2 is a const auto definition created from std::moveing outer (creating a unique_ptr<non-const MacroScope>). I'm not so keen on making outer2 implicitly mut without a type annotation
    )
)

def (expand_macros, node: Module, on_visit: std.function<void(MacroDefinition, const:std.unordered_map<Node, Node>:ref)>:
    visitor: mut = MacroDefinitionVisitor(on_visit)
    node.accept(visitor)
    return visitor.replacements
) : std.unordered_map<Node, Node>
```

## Gotchas

### class reference semantics, shared\_ptr apologia

We take [this C++ Core guideline](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rr-sharedptrparam-const) to heart:

    R.36: Take a const shared_ptr<widget>& parameter to express that it might retain a reference count to the object ???

For example, `x` and `y` are both passed to `func` by reference to const in this example:

```python
class (Foo:
    pass
)

def (func, x, y: Foo:
    static_assert(std.is_reference_v<decltype(x)>)
    static_assert(std.is_const_v<std.remove_reference_t<decltype(x)>>)
    static_assert(std.is_reference_v<decltype(y)>)
    static_assert(std.is_const_v<std.remove_reference_t<decltype(y)>>)
    static_assert(std.is_same_v<decltype(y), const:std.shared_ptr<const:Foo.class>:ref>)
)

def (main:
    x = Foo()
    y = Foo()
    func(x, y)
    func(1, x)
)
```

Because we want (non-unique) class instances to behave roughly as class instances do in Python (that is, to be "maybe retained" when passed as parameters), we make this core guideline advice the implicit default for untyped/generic params and params of explicit class type. As an example, consider the function ```expand``` in a typical computer algebra system. Given say ```expand(x - x)```, the result is `0` (parameter not retained). Given another expression, ```expand``` might return it unchanged or even return an expression retaining a subexpression of its input. "Maybe retain" is the right choice for such an API (see e.g. [the signature of expand in symengine](https://github.com/symengine/symengine/blob/ed1e3e4fd8260097fa25aa1282e1d3a4ac4527f3/symengine/expand.cpp#L369)).

Note however that we don't entirely embrace the suggestions of this core guideline R.36 especially with regard to a suggested warning that

    (Simple) ((Foundation)) Warn if a function takes a Shared_pointer<T> by value or by reference to const and does not copy or move it to another Shared_pointer on at least one code path. Suggest taking a T* or T& instead.

If passing by T* or T& suffices in C++ (especially const T&), maybe you should be using `struct` instead of `class` in ceto anyway (autoderef, though not implicit lambda capture, still works for explicit std.shared\_ptrs). The annoying asymmetry of `x->foo` vs `x.foo`, one of the better reasons to embrace R.36 fully in *C++*, is gone in ceto. When the above warning applies we're also paying only for an extra indirection not an unnecessary refcount bump due to passing by reference to const (even for ```Foo:mut```). Unnecessarily enforcing parameter lifetimes when unowned raw pointers or mutable references suffice is debatably a bug or feature.

Consider also [this stackoverflow comment chain](https://stackoverflow.com/questions/3310737/should-we-pass-a-shared-ptr-by-reference-or-by-value#comment63125143_8741626), where it's suggested that even though passing by ref to const is the (somewhat more) performant choice in some cases, the extra verbosity makes passing by value attractive (I'm aware of several programming language implementations and even an educational graphics engine that follow this advice). None of the replies at present suggesting to make a typedef for ```shared_ptr<Foo>``` mention that 2 typedefs would be better (with the ```shared_ptr<const Foo>``` typedef preferred!)

Note also that e.g. in this case:

```python
class (Foo:
    x: Foo
    y: Foo
)

def (main:
    x = Foo(None, None)
    Foo(x, x)
)
```

the generated C++ for Foo contains a 2-arg constructor taking x and y as shared_ptrs by value and initializing the data members via std::move in the initializer list. It's debatable whether a future optimization should be added to ceto so that parameters of ceto-class type used only once are taken by value but std::moved to their destination (it further complicates the meaning of ```Foo``` and may require some kind of ```export``` keyword (perhaps the existing ```noinline``` can be used) given our current support for forward function declarations).

One may also object to the unnecessary performance overhead of std.shared\_ptr's atomic counters in single threaded code. The view of the C++ committee applies doubly: the main deficiency of a given boatload of pythOnic ceto/C++ is probably not "too much thread safety" (see GOTCHAs)
    
### Implicit Scope Resolution

For the expression

```python
x.y.z()
```

The rules for `.` in ceto are that

- If `x` has a variable or parameter definition (in ceto code), the entire expression is treated as a chain of (maybe autoderefed) attribute accesses.

- Otherwise, if `x` has no variable definition in ceto (even if defined in external C++ code), then the above is equivalent to `x::y::z()` in C++ (and ceto)

These rules allow us to avoid requiring constructs like say `import_namespace(std, views)` (which would be too much boilerplate even if ceto ever supported C++ modules and `import std`; parsing C++ headers or maintaining a big crazy list of all useful C++ namespaces is also not an option)

There is a gotcha with these lookup based rewriting rules however. For example if you've written code like this:

```python
def (function, parameter:
    return parameter.do_something(100)
)
```

and then decide to modify it by adding  a new class definition with maybe a little refactoring:

```python 

class (parameter:  # flounting PEP-8 naming conventions in this case was a bad move
    def (do_something: static, severity_level: int:
        corrupt_disk_with_severity(severity_level)
    )
)

def (function, parameter_renamed:
    parameter.do_something(100)  # oops forgot to rename here
)
```

resuling in the generated C++ code calling `parameter::do_something(100)` - a perhaps unexpected scope resolved or static member call.

Nevertheless, this is no argument for `::`. I tire from the 4 strained key engagements typing it even now.

Also note the number and severity of C++ misfeatures meant to alleviate the ugliness of `::`.  Cheif among them is encouraging `using` declarations (even locally).

For example, writing


```python
safe.do_something(param)
# or even
safe::do_something(param)
```

is generally preferable to the ceto code

```python
safe: using
do_something(param)
```

because calling `do_something` as a free function suffers not just from the possibility that an unexpected definition of `do_something` is available in the global namespace (upon the removal of the `using` declaration) but also that an unexpected definition in an unexpected namespace is found via [ADL](https://stackoverflow.com/a/4241547/1391250)!

This is not to mention the namespace pollution problems of using declarations. Arguably they should even be considered another unsafe C++ compatibility feature requiring unsafe block in the future (aside: the backwards PyThOn syntax for using declarations is based on the principle that if a ceto construct already codegens as something like the desired C++, then that construct should be used for that C++)

There's another C++ mispattern where a global variable of a `struct` type with a few data members is defined in a header allowing the `utility.foo()` syntax instead of `::`. If one forgets the ```inline``` or ```constexpr``` in such a definition then say hello to IFNDR. Calling such a (C++ header defined) global variable is actually impossible in ceto unless the PyThOn style ```global``` keyword is used:

```python

include"utility.h"

def (main:
    utility: global
    utility.foo()    # maybe autoderef but not a scope resolution
)

```

There is also a GOTCHA/TODO that an error should be issued attempting to mix ceto class types with C++ typedefs or using declarations (Foo_typdef won't follow the same parameter passing rules as Foo when used as a function param type).

Finally, note that scope resolution is still necessary in a few places where `.` is not rewritten to `::`:

```python

class (Foo:
    pass
)

def (main:
    x = Foo() 
    # It's not a great idea to write code dependent on a particular 
    # smart pointer implementation for `class` instances
    static_assert(decltype(x)::element_type, const:Foo.class)
)
```

### Classes instances aren't real smart references / Limitations of "Java" style visitor pattern

In the "just like Java" visitor example we write

```python
visitor.visit(self)
```

rather than 

```
visitor.visit(*this)
```

This relies on the meaning of *self*. For simple attribute accesses `self.foo` is rewritten to `this->foo`. When `self` is used in other contexts (including any use in a capturing lambda) a `const` definition of `self` using `shared_from_this` is provided to the body of the method (compile time error when used with *struct* or *:unique* instances and TODO a transpile time error when non-trivial use of *self* occurs in *init*).

There are performance concerns with the hidden use of *shared_ptr* in this visitor example which we discuss below (TODO) however a more pressing problem with this example is that ceto class instances aren't real "smart references". That is, they're not C++ references backed by a refcount that behave the same as ordinary C++ references with respect to function overloading especially). 

In particular, this means that

```python
def (visit: override: mut, node: BinOp:
    ...
)

```

is not overridden by 

```python
def (visit: override: mut, node: Add:
    ...
)

```

A note on development status: we are partially selfhosted but only by cheating! That is, we've implemented an ast in ceto then briged it to our original python bootstrap compiler via the pybind11 bindings here.

For our "selfhost" ast (TODO link), we heavily rely on (shared) ceto class instances (primarily to integrate with our bootstrap python compiler via the pybind11 bindings here (TODO link). Using :unique classes for our ast nodes would have certain benefits in the future

(Though an implementation of our ast using :unique is possible in the future, 

   integration with our exising to be replaced   (for  a (TODO link) CRTP visitor implementation that relies on overriding visit Node.


TODO: clean up and integrate the below text better

"Just like Java" (with differerent syntax and the complication of const/mut) because the two crucial method calls of the visitor pattern above are written

```python
arg.accept(self)  # java equivalent: arg.accept(this)
```

and

```python
visitor.visit(self)  # java equivalent: visitor.visit(this)
```

rather than the the idiomatic C++ which would be something like

```c++
visitor.visit(*this)
``` 

This brings us to the meaning of `self`: For simple attribute accesses `self.foo` is rewritten to `this->foo`. When `self` is used in other contexts (including any use in a capturing lambda) a `const` definition of `self` using `shared_from_this` is provided to the body of the method.

At this point, you might be saying "automatic make_shared, and automatic shared_from_this??!" This would be slower than Java! (at least once the JVM starts up)"

To alleviate these performance concerns, we can first change the definition of `Visitor` and `SimpleVisitor` to use `struct` instead of `class`. We would then change calls in the `visit` methods like `arg.accept(self)` to `arg.accept(*this)  # note *self works but might cause an unnecessary refcount bump`. `accept` methods must be changed so that `Visitor` (now just a `struct`) is passed by `mut:ref` rather than just `mut` (note that when `Visitor` was a (non-`unique`) `class`, `Visitor:mut` as a function param meant in C++ `const std::shared_ptr<Visitor>&` (that is a const-ref shared_ptr to non-const)

Note that these changes introducing unary `*` as well as the keyword `ref` (especially `mut:ref` !) might require an `unsafe` block in the future (cost of performance).

We're then left with `Node` and its derived classes. If changed to `struct` we'll be forced to redesign this class (err struct) hierarchy either in terms of raw pointers or of explicitly smart pointer managed instances. Smart pointer managed struct instances still benefit from autoderef; that is `arg.accept` is never required to be rewritten `arg->accept` unless one is unwisely avoiding a null check). 

For ceto's selfhost ast implementation we define the visitor pattern using struct but (for better or worse) keep the class hierarchy as shared (for compatibility with our existing python bootstrap compiler). However we define the visitor pattern visit methods without smart pointer parameter passing by using 'Node.class' to get at the underlying class (just `Node` in C++). This also requires changing the accept methods to call `visitor.accept(*this)`. Note that like struct instances, a `Foo.class` is still passed by `const:ref` automatically.

See here for our ast
Here for our visitor implmentation and a CRTP visitor subclass for visiting only certain derived classes conveniently (stolen from symengine)
See here for our macro expansion pass which uses the CRTP utility (and also has a bit of everything e.g. `MacroScope` is `:unique` and we rely on all 3 kinds - shared, unique, and optional too.

There is also the possibility to define the `Node` hierarchy using `unique`. The 'smart ptr heavy' version of the visitor pattern would then benefit from our handling of `const:Node:ref` in a function param (when `Node` is `:unique`) as `const unique_ptr<const Node>&` (there is not such a convenient way to operate with non-ptr-to-const unique_ptr managed instances passed by const ref - nor is there a convenient way to operate with non-const references to smart pointers - as an intended safety feature!)

While you are right, this is not the worst thing with the above visitor example! When making slightly more complicated use of the visitor pattern you'll quickly realize that  

```python
def (visit: override: mut, node: BinOp:
    ...
)

```

is not "overridden" by 

```python
def (visit: override: mut, node: Add:
    ...
)

```


At this point, you might be objecting on performance grounds alone. While the above visitor example 

While this is true (though we claim zero overhead because you can always avoid `class`


### Misc


## Features/TODO

- Classes
- [ ] Special member functions
    - [ ] init method
        - [x] implicit init (but ```explicit``` as in the C++ keyword constructor). [x] params taken by value but std::moved to destination for implicit init
        - [x] explicit init methods (same parameter passing defaults as other functions/methods)
            - [x] super init call. works with template base classes via decltype thing (possibly unnecessary in c++23) 
        - [ ] should take params by value but std::move in the explicit init method case so long as params not used outside of implicit initializer list (initial block of self.x assignments + optional super.init call)
        - [ ] multiple init methods / designated initializers (need to check that data members properly initialized)
    - [ ] other special member functions
        - [ ] ```def (copyinit, other)``` with other always untyped. [ ] raise CodeGenError upon encountering a copy constructor defined like in C++ e.g. with a param ```other: Foo``` (```other``` implicitly ```const:Foo:ref``` for Foo a struct) or ```other: Foo.class``` (for ```Foo``` a ceto class)
        - [ ] moveinit, copyassign, moveassign (no parameter types or return types specifiable; leave e.g. member functions that are conditionally copy constructors etc to external C++?) 

```python
# last example at https://en.cppreference.com/w/cpp/language/rule_of_three
class (BaseOfFiveDefaults:
    def (copyinit, other) = default
    def (moveinit, other) = default
    def (copyassign, other) = default
    def (moveassign, other) = default
    def (destruct: virtual) = default
)
```
    - [x] empty canonical destructor is default (aka use ```pass; pass``` for non-default). Implemented via builtin macro - see include/convenience.cth
    - [x] macro system in current state sufficient to add special member functions (defaulted deleted or otherwise) to classes of certain "type"? Ideally need:
        - [ ] more fine grained pattern matching (subpattern is ZeroOrMore/OneOrMore of a particular pattern rather than just [x] a particular Node subclass)

- Macros
    - [x] generic (implicit Node) and specific class type params (see ast.cth)
    - [x] Alternational params: e.g. ```BinOp|Call|ArrayAccess```
    - [x] Optional via pattern_var: ```Node|None``` syntax. Rename to ```Optional(Node)``` or ```ZeroOrOne(Node)``` [ ]? Note: ```defmacro (foo: optional_var, optional_var: Node|None, ...)``` matches both ```foo``` and ```foo:whatever + 1*some([thing])``` - perhaps implied not obvious  `Optional` syntax
        - [ ] TODO Allow Optional in list-like context
    - [x] Zero or more via ```exprs: [Node]``` syntax
    - [ ] finer grained macro matching e.g. Zero or more applied to a particular subpattern (not just a Node subclass). Either need to allow special functions (perhaps with mandatory namespacing) e.g. ceto.ast.ZeroOrMore to occur in the first pattern param or allow pattern params e.g. ()```param : [pattern(1 + x, x)]
    - [ ] defmacro elif else
    - [ ] Better/canonical way to avoid processing of a node by the same or other macros (especially without having to access the C/C++ preprocessor via cpp"strings") - `Forget` / `Launder` ? Note: comparing Node addresses is problematic due to some cloning out of your control - [ ] should be reiged in bit.
    - [ ] Compile two or more macros in the same file into the same DLL (unless the second macro contains patterns potentially expandable by the first...). This would simplify shared memory between macro implementation bodies (whether a good idea or not)
    - [ ] IMPORTANT TODO flattening of ast BinOp args after/during parse: `1 + 2 + 3` should be a single `Add` with 3 args not 2 `Add` with 2 left/right args. WARNING: macro code that depends on the ```lhs``` or ```rhs``` ```BinOp``` methods (or the assumption that ```some_binop.args.size() == 2```) will likely be broken in the future. Why the bother in that a nested ast simplifies code printing and perhaps other things? Codegen (and macros) very frequently wants to manipulate multi-word "types" like ```mut:static:int``` (to e.g. remove the ```mut``` prior to generating C++ code or to determine if ```mut``` is merely present). This is much nicer with the flattened representation. Macros that add function attributes or keywords as the default (only if they're not already present) benefit from the flattened representation.
    - [ ] Macros should have access to scope/lookup table info. Especially necessary for macro patterns involving ```.``` (AttributeAccess) including method-call macros that don't want to also override implicit scope resolution via ".". For example, ```defmacro(string_var.split(optional_delim), ...)``` is problematic because it overrides the implicit scope resolution for code like e.g. ```my_library.split(str)```. Though ```my_library::split(str)``` can still be written, it's ugly and should be avoided. 
        - [ ] Note current scope.cth in selfhost is a direct port of the bootstrap Python scope.py and is intentionally unavailable to the macro system. The main deficiency of this code is relying on the unflattened ast for ```TypeOp``` (it also interacts with other poor handling of TypeOp - particularly semanticanalysys.py and codegen.py relying on a .declared_type property of Node - rather than something like the Identifier|TypeOp|None optional matching available to the macro system). Also while we do need to attach scopes to each node prior to beginning codegen scope should not be an actual attribute of Node. Note that the empty list append forward inference via decltype requires every Node to have an associated Scope prior to codegen - to write a simlar macro all scope info must be computed before the first attempted macro invocation.
            - [ ] Updating scope info after a macro expand. Probably can do better than a complete re-traversal?


-----

### Further Explanation


## Features

- [x] Autoderef (call methods on "class instances" using `.` instead of `->`)
   - [x] Using dot on a std::shared_ptr, std::unique_ptr, or std::optional autoderefed (in the case of std::optional no deref takes place when calling a method of std::optional - that is, to call a method `value()` call `.value().value()`). For std::shared/unique_ptr you must use a construct like `(&o)->get` to call the smart ptr `get` method.
- implicit scope resolution using `.` (`::` may still be used)
- auto make_shared / make_unique for ceto defined classes (with hidden CTAD for templated classes). Write `f = Foo(x, y)` like python regardless of whether `Foo` is a (unique) class or struct (and regardless of whether `Foo` has generic/untyped data members or is an explicit template).
- (const) auto everywhere and nowhere
    - locals `const:auto` by default
    - parameters `const` by default and maybe `const:ref` by default depending on their type (for example all shared_ptrs transparently managing ceto defined class instances are passed by const ref, all ceto defined structs are passed by `const:ref`, as well as `std::vector` and `std::tuple` when using the ceto python style `[list, literal]` and `(tuple, literal)` notation)
    - methods const by default
- `:` as a first class binary operator in ceto for use by macro defined or built-in constructs e.g. one-liner ifs `if (cond: 1 else: 0)`. Variable type declaration syntax mimicks python type annotations e.g. `x: int` but `:` acts as a type separator character for multi-word C++ types (and type-like / space separated things) e.g. `x: static:std.array<unsigned:int, 4> = {1, 2, 3, 4}`

## Features

- [x] Autoderef (call methods on class instances using `.` instead of `->` or `*`)
    - [x] `.` performs  `std::shared_ptr`, `std::unique_ptr`, and `std::optional` autoderef in addition to ordinary C++ member access. 
- [x] `.` performs C++ scope resolution (like namespace access in Python)

----

Extended Feature List
- [x] Autoderef (call methods on 'class instances' using '.' instead of '->')
   - [x] Might as well autoderef every smart pointer and optionals too. Except, when calling a method of std::optional, yo
- implicit scope resolution using `.` (`m: std.unordered_map<int, int> = {{0,1}}` but you can still write `m: std::unordered_map<int, int>` if you insist
- auto make_shared / make_unique for ceto defined classes (with hidden CTAD for templated classes)
- (const) auto everywhere and nowhere
    - locals `const:auto` by default
    - parameters `const` by default and maybe `const:ref` by default depending on their type (for example all shared_ptrs transparently managing ceto defined class instances are passed by const ref, all ceto defined structs are passed by `const:ref`, as well as `std::vector` and `std::tuple` when using the ceto python style `[list, literal]` and `(tuple, literal)` notation)

    - methods
- `:` as a first class binary operator in ceto (for creation of user defined constructs in the macro system and some tom foolery in built-in ceto constructs like one-liner ifs `if (cond: 1 else: 0)`. Types are annoted with 


Informally, one can think of the language as "Python with two parenthesese moved or inserted" (per control structure). This is a good approach for those less familliar with C++, for those wanting to avoid certain explicitly unsafe C++ operations such as unary `*` (present in C++/ceto but not Python and which might TODO require an `unsafe` block in in the future), and for those wishing to prototype with Python style ceto (heavily using `class`) with an easier path to integrating more precise/performant C++ (whether ceto defined or not) later.

## Syntax: 

Every Python statement is present but represented as a function call that takes zero or more indented blocks in addition to any ordinary parameters. Blocks begin with an end of line `:`. Every other occurence of `:` is a first class binary operator (TypeOp in the ast). The other operators retain their precedence and syntax from C++ (see https://en.cppreference.com/w/cpp/language/operator_precedence) with the exceptions of `not`, `and`, and `or` which require the Python spelling but C++ precedence. Some C++ operators such as pre-increment and post-increment are intentionally not present (you can't have a fake Python with `++`).

`def`, `class`, `while`, `for`, `if`, `try`, etc are merely `Identifier` instances in the ast (and macro system) not special keywords in the grammar.

Simple python expressions such as `[list, literals]`, `{curly, braced, literals}` and `(tuple, literals)` are present as well as array[access] notation. We also support C++ templates and curly braced calls.

For example:

```python
include <cassert>  # parsed as templates
include <unordered_map>
include <optional>

def (main:
    s = [1, 2]  # ast: Assign with rhs a ListLiteral
    
    for (x in {1, 2, 3, 4}:  # ast: BracedLiteral as rhs of InOp and first arg to call with func "for" (second arg a Block)
        pass
    )

    m: std.unordered_map<int, std.string> = {{0, "zero"}, {1, "one"}}
    m2 = std.unordered_map <int, std.string> {{0, "zero"}, {1, "one"}}
    assert(m == m2)

    v = std.vector<int> {1, 2}  # ast: BracedCall with func a Template
    v2: std.vector<int> = {1, 2}
 
    assert(v == v2)
    assert(v == s)

    v3: std.vector<int> (1, 2)  # ast: Call with func a template
    assert(v != v3)
    assert(v3.size() == 1 and v3[0] == 2)

    opt: std.optional<std.string> = {}  # empty
    if (opt:
        assert(opt.size() >= 0)  # aside: std.optional autoderef here
    )

    it:mut = s.begin()
    it.operator("++")(1)   # while there's no ++ and -- you can do this
    it.operator("--")()    # or this (which one is the preincrement anyway?)
    cpp"
        --it--  // if you really insist
        #define PREINCREMENT(x) (++x)  // even worse
    "
    PREINCREMENT(it)
    
    # of course the pythonic option should be encouraged
    it += 1  

    # note the utility of ++ is diminished when C-style for loops are unavailable anyway
    assert(it == s.end())
)

```

