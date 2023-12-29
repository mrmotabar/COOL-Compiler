

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "semant.h"
#include "utilities.h"

extern int semant_debug;
extern char *curr_filename;

const int class_number_log = 20;
int semant_errors = 0;
ostream &error_stream = cerr;
std::map<Symbol, Class_> class_table;
std::map<Symbol, std::vector<method_class *>> method_table;
std::map<Symbol, std::vector<Symbol>> adj;
std::map<Symbol, bool> visit;
std::map<Symbol, int> dis;
std::map<Symbol, Symbol[class_number_log]> LCA_table;
SymbolTable<Symbol, Symbol> id_type;
Class_ curr_class;

void install_basic_classes();
void install_classes();
void check_inheritance_graph();
void make_LCA_table(Symbol, Symbol);
void install_methods();
void is_main_exists();
Symbol get_par(Symbol, int);
Symbol LCA(Symbol, Symbol);
bool match(Symbol, Symbol);
ostream &semant_error();
ostream &semant_error(Class_ c);
ostream &semant_error(Symbol filename, tree_node *t);

//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////
static Symbol
    arg,
    arg2,
    Bool,
    concat,
    cool_abort,
    copy,
    Int,
    in_int,
    in_string,
    IO,
    length,
    Main,
    main_meth,
    No_class,
    No_type,
    Object,
    out_int,
    out_string,
    prim_slot,
    self,
    SELF_TYPE,
    Str,
    str_field,
    substr,
    type_name,
    val;
//
// Initializing the predefined symbols.
//
static void initialize_constants(void)
{
    arg = idtable.add_string("arg");
    arg2 = idtable.add_string("arg2");
    Bool = idtable.add_string("Bool");
    concat = idtable.add_string("concat");
    cool_abort = idtable.add_string("abort");
    copy = idtable.add_string("copy");
    Int = idtable.add_string("Int");
    in_int = idtable.add_string("in_int");
    in_string = idtable.add_string("in_string");
    IO = idtable.add_string("IO");
    length = idtable.add_string("length");
    Main = idtable.add_string("Main");
    main_meth = idtable.add_string("main");
    //   _no_class is a symbol that can't be the name of any
    //   user-defined class.
    No_class = idtable.add_string("_no_class");
    No_type = idtable.add_string("_no_type");
    Object = idtable.add_string("Object");
    out_int = idtable.add_string("out_int");
    out_string = idtable.add_string("out_string");
    prim_slot = idtable.add_string("_prim_slot");
    self = idtable.add_string("self");
    SELF_TYPE = idtable.add_string("SELF_TYPE");
    Str = idtable.add_string("String");
    str_field = idtable.add_string("_str_field");
    substr = idtable.add_string("substr");
    type_name = idtable.add_string("type_name");
    val = idtable.add_string("_val");
}

void install_basic_classes()
{

    // The tree package uses these globals to annotate the classes built below.
    // curr_lineno  = 0;
    Symbol filename = stringtable.add_string("<basic class>");

    // The following demonstrates how to create dummy parse trees to
    // refer to basic Cool classes.  There's no need for method
    // bodies -- these are already built into the runtime system.

    // IMPORTANT: The results of the following expressions are
    // stored in local variables.  You will want to do something
    // with those variables at the end of this method to make this
    // code meaningful.

    //
    // The Object class has no parent class. Its methods are
    //        abort() : Object    aborts the program
    //        type_name() : Str   returns a string representation of class name
    //        copy() : SELF_TYPE  returns a copy of the object
    //
    // There is no need for method bodies in the basic classes---these
    // are already built in to the runtime system.

    Class_ Object_class =
        class_(Object,
               No_class,
               append_Features(
                   append_Features(
                       single_Features(method(cool_abort, nil_Formals(), Object, no_expr())),
                       single_Features(method(type_name, nil_Formals(), Str, no_expr()))),
                   single_Features(method(copy, nil_Formals(), SELF_TYPE, no_expr()))),
               filename);

    //
    // The IO class inherits from Object. Its methods are
    //        out_string(Str) : SELF_TYPE       writes a string to the output
    //        out_int(Int) : SELF_TYPE            "    an int    "  "     "
    //        in_string() : Str                 reads a string from the input
    //        in_int() : Int                      "   an int     "  "     "
    //
    Class_ IO_class =
        class_(IO,
               Object,
               append_Features(
                   append_Features(
                       append_Features(
                           single_Features(method(out_string, single_Formals(formal(arg, Str)),
                                                  SELF_TYPE, no_expr())),
                           single_Features(method(out_int, single_Formals(formal(arg, Int)),
                                                  SELF_TYPE, no_expr()))),
                       single_Features(method(in_string, nil_Formals(), Str, no_expr()))),
                   single_Features(method(in_int, nil_Formals(), Int, no_expr()))),
               filename);

    //
    // The Int class has no methods and only a single attribute, the
    // "val" for the integer.
    //
    Class_ Int_class =
        class_(Int,
               Object,
               single_Features(attr(val, prim_slot, no_expr())),
               filename);

    //
    // Bool also has only the "val" slot.
    //
    Class_ Bool_class =
        class_(Bool, Object, single_Features(attr(val, prim_slot, no_expr())), filename);

    //
    // The class Str has a number of slots and operations:
    //       val                                  the length of the string
    //       str_field                            the string itself
    //       length() : Int                       returns length of the string
    //       concat(arg: Str) : Str               performs string concatenation
    //       substr(arg: Int, arg2: Int): Str     substring selection
    //
    Class_ Str_class =
        class_(Str,
               Object,
               append_Features(
                   append_Features(
                       append_Features(
                           append_Features(
                               single_Features(attr(val, Int, no_expr())),
                               single_Features(attr(str_field, prim_slot, no_expr()))),
                           single_Features(method(length, nil_Formals(), Int, no_expr()))),
                       single_Features(method(concat,
                                              single_Formals(formal(arg, Str)),
                                              Str,
                                              no_expr()))),
                   single_Features(method(substr,
                                          append_Formals(single_Formals(formal(arg, Int)),
                                                         single_Formals(formal(arg2, Int))),
                                          Str,
                                          no_expr()))),
               filename);
    class_table[Object_class->get_name()] = Object_class;
    class_table[IO_class->get_name()] = IO_class;
    class_table[Int_class->get_name()] = Int_class;
    class_table[Bool_class->get_name()] = Bool_class;
    class_table[Str_class->get_name()] = Str_class;
}

void install_classes(Classes classes)
{
    for (int i = classes->first(); classes->more(i); i = classes->next(i))
    {
        Class_ curr = classes->nth(i);
        Symbol par_name = curr->get_parent();
        if (curr->get_name() == SELF_TYPE)
        {
            semant_error(curr) << "error!\n";
        }
        else if (class_table.count(curr->get_name()) > 0)
        {
            semant_error(curr) << "error!\n";
        }
        else
        {
            class_table[curr->get_name()] = curr;
        }
    }
}

void check_inheritance_graph()
{
    for (std::map<Symbol, Class_>::iterator iter = class_table.begin(); iter != class_table.end(); iter++)
    {
        if (iter->first != Object && class_table.count(iter->second->get_parent()) == 0)
        {
            semant_error(iter->second) << "error!\n";
        }
        else if (iter->second->get_parent() == Bool || iter->second->get_parent() == Str)
        {
            semant_error(iter->second) << "error!\n";
        }
    }
    for (std::map<Symbol, Class_>::iterator iter = class_table.begin(); iter != class_table.end(); iter++)
    {
        if (iter->first == Object)
        {
            continue;
        }
        if (class_table.count(iter->second->get_parent()) == 0 || iter->second->get_parent() == Bool || iter->second->get_parent() == Str)
        {
            adj[Object].push_back(iter->first);
        }
        else
        {
            adj[iter->second->get_parent()].push_back(iter->first);
        }
    }
    for (std::map<Symbol, Class_>::iterator iter = class_table.begin(); iter != class_table.end(); iter++)
    {
        for (int i = 0; i < class_number_log; i++)
        {
            LCA_table[iter->first][i] = Object;
        }
    }
    make_LCA_table(Object, Object);
    for (std::map<Symbol, Class_>::iterator iter = class_table.begin(); iter != class_table.end(); iter++)
    {
        if (visit[iter->first] != true)
        {
            semant_error(iter->second) << "error!\n";
        }
    }
}

/*
    I used an advanced but straightforward algorithm to find the LCA in O(log n).
    However, using this algorithm is unnecessary, and you can use a loop to find LCA.
    The description of the algorithm I used can be found in the following link.
    https://cp-algorithms.com/graph/lca_binary_lifting.html
*/
void make_LCA_table(Symbol v, Symbol par)
{
    visit[v] = true;
    if (v != Object)
    {
        dis[v] = dis[class_table[v]->get_parent()] + 1;
        LCA_table[v][0] = par;
        for (int i = 1; i < class_number_log; i++)
        {
            LCA_table[v][i] = LCA_table[LCA_table[v][i - 1]][i - 1];
        }
    }
    for (Symbol u : adj[v])
    {
        if (u != par)
        {
            make_LCA_table(u, v);
        }
    }
}

Symbol get_par(Symbol v, int p)
{
    for (int i = class_number_log - 1; i >= 0; i--)
    {
        if (p >> i & 1)
        {
            v = LCA_table[v][i];
        }
    }
    return v;
}

Symbol LCA(Symbol v, Symbol u)
{
    if (v == SELF_TYPE && u == SELF_TYPE)
    {
        return SELF_TYPE;
    }
    else if (v == SELF_TYPE)
    {
        v = curr_class->get_name();
    }
    else if (u == SELF_TYPE)
    {
        u = curr_class->get_name();
    }
    if (dis[v] < dis[u])
    {
        Symbol temp = v;
        v = u;
        u = temp;
    }
    v = get_par(v, dis[v] - dis[u]);
    if (u == v)
    {
        return v;
    }
    for (int i = class_number_log - 1; i >= 0; i--)
    {
        if (LCA_table[v][i] != LCA_table[u][i])
        {
            v = LCA_table[v][i];
            u = LCA_table[u][i];
        }
    }
    return LCA_table[v][0];
}

void install_methods()
{
    for (std::map<Symbol, Class_>::iterator iter = class_table.begin(); iter != class_table.end(); iter++)
    {
        Features features = iter->second->get_features();
        for (int i = features->first(); features->more(i); i = features->next(i))
            if (features->nth(i)->is_method())
            {
                method_class *method = static_cast<method_class *>(features->nth(i));
                bool redefined_flag = false;
                for (size_t j = 0; j < method_table[iter->first].size(); j++)
                    if (method_table[iter->first][j]->get_name() == method->get_name())
                        redefined_flag = true;

                if (redefined_flag)
                    semant_error(iter->second) << "error!\n";
                else
                    method_table[iter->first].push_back(method);
            }
    }
}

void is_main_exists()
{
    if (class_table.count(Main) == 0)
    {
        semant_error() << "Class Main is not defined.\n";
        return;
    }

    Features features = class_table[Main]->get_features();

    bool main_flag = false;
    for (int i = features->first(); features->more(i); i = features->next(i))
        if (features->nth(i)->is_method() && static_cast<method_class *>(features->nth(i))->get_name() == main_meth)
            main_flag = true;

    if (!main_flag)
        semant_error(class_table[Main]) << "error!\n";
}

bool match(Symbol T1, Symbol T2)
{
    if (T1 == SELF_TYPE && T2 == SELF_TYPE)
    {
        return true;
    }
    if (T1 == No_type)
    {
        return true;
    }
    if (T1 != SELF_TYPE && T2 == SELF_TYPE)
    {
        return false;
    }

    if (T1 == SELF_TYPE)
    {
        T1 = curr_class->get_name();
    }

    if (LCA(T1, T2) == T2)
    {
        return true;
    }

    return false;
}

Symbol object_class::inference_type()
{
    if (name == self)
    {
        type = SELF_TYPE;
    }
    else if (id_type.lookup(name))
    {
        type = *id_type.lookup(name);
    }
    else
    {
        semant_error(curr_class->get_filename(), this) << "error!\n";
        type = Object;
    }
    return type;
}

Symbol assign_class::inference_type()
{
    Symbol right_type = expr->inference_type();
    if (id_type.lookup(name) == 0)
    {
        semant_error(curr_class->get_filename(), this) << "error!\n";
        type = right_type;
        return type;
    }
    Symbol left_type = *id_type.lookup(name);
    if (!match(right_type, left_type))
    {
        semant_error(curr_class->get_filename(), this) << "error\n";
        type = left_type;
        return type;
    }

    type = right_type;
    return type;
}

Symbol bool_const_class::inference_type()
{
    type = Bool;
    return type;
}

Symbol int_const_class::inference_type()
{
    type = Int;
    return type;
}

Symbol string_const_class::inference_type()
{
    type = Str;
    return type;
}

Symbol new__class::inference_type()
{
    if (this->type_name != SELF_TYPE && class_table.count(this->type_name) == 0)
    {
        semant_error(curr_class->get_filename(), this) << "error!\n";
        this->type_name = Object;
    }
    type = this->type_name;
    return type;
}

Symbol isvoid_class::inference_type()
{
    e1->inference_type();
    type = Bool;
    return type;
}

Symbol no_expr_class::inference_type()
{
    type = No_type;
    return type;
}

Symbol comp_class::inference_type()
{
    type = e1->inference_type();
    if (type != Bool)
    {
        semant_error(curr_class->get_filename(), this) << "error!\n";
    }
    type = Bool;
    return type;
}

Symbol leq_class::inference_type()
{
    Symbol T1 = e1->inference_type();
    Symbol T2 = e2->inference_type();
    if (T1 != Int || T2 != Int)
    {
        semant_error(curr_class->get_filename(), this) << "error!\n";
    }
    type = Bool;
    return type;
}

Symbol eq_class::inference_type()
{
    Symbol T1 = e1->inference_type();
    Symbol T2 = e2->inference_type();
    if ((T1 == Int || T1 == Bool || T1 == Str || T2 == Int || T2 == Bool || T2 == Str) && T1 != T2)
    {
        semant_error(curr_class->get_filename(), this) << "error!\n";
    }
    type = Bool;
    return type;
}

Symbol lt_class::inference_type()
{
    Symbol T1 = e1->inference_type();
    Symbol T2 = e2->inference_type();
    if (T1 != Int || T2 != Int)
    {
        semant_error(curr_class->get_filename(), this) << "error!\n";
    }
    type = Bool;
    return type;
}

Symbol neg_class::inference_type()
{
    type = e1->inference_type();
    if (type != Int)
    {
        semant_error(curr_class->get_filename(), this) << "error!\n";
    }
    type = Int;
    return type;
}

Symbol divide_class::inference_type()
{
    Symbol T1 = e1->inference_type();
    Symbol T2 = e2->inference_type();
    if (T1 != Int || T2 != Int)
    {
        semant_error(curr_class->get_filename(), this) << "error!\n";
    }
    type = Int;
    return type;
}

Symbol mul_class::inference_type()
{
    Symbol T1 = e1->inference_type();
    Symbol T2 = e2->inference_type();
    if (T1 != Int || T2 != Int)
    {
        semant_error(curr_class->get_filename(), this) << "error!\n";
    }
    type = Int;
    return type;
}

Symbol sub_class::inference_type()
{
    Symbol T1 = e1->inference_type();
    Symbol T2 = e2->inference_type();
    if (T1 != Int || T2 != Int)
    {
        semant_error(curr_class->get_filename(), this) << "error!\n";
    }
    type = Int;
    return type;
}

Symbol plus_class::inference_type()
{
    Symbol T1 = e1->inference_type();
    Symbol T2 = e2->inference_type();
    if (T1 != Int || T2 != Int)
    {
        semant_error(curr_class->get_filename(), this) << "error!\n";
    }
    type = Int;
    return type;
}

Symbol let_class::inference_type()
{
    id_type.enterscope();
    if (identifier == self)
    {
        semant_error(curr_class->get_filename(), this) << "error!\n";
    }
    else
    {
        id_type.addid(identifier, &type_decl);

        Symbol init_type = init->inference_type();
        if (type_decl != SELF_TYPE && class_table.count(type_decl) == 0)
        {
            semant_error(curr_class->get_filename(), this) << "error!\n";
        }
        else if (!match(init_type, type_decl))
        {
            semant_error(curr_class->get_filename(), this) << "error!\n";
        }
    }
    type = body->inference_type();
    id_type.exitscope();
    return type;
}

Symbol block_class::inference_type()
{
    for (int i = body->first(); body->more(i); i = body->next(i))
    {
        type = body->nth(i)->inference_type();
    }
    return type;
}

Symbol branch_class::inference_type()
{
    id_type.enterscope();
    id_type.addid(name, &type_decl);
    type = expr->inference_type();
    id_type.exitscope();
    return type;
}

Symbol typcase_class::inference_type()
{
    Symbol expr_type = expr->inference_type();
    std::set<Symbol> check_duplicate;
    for (int i = cases->first(); cases->more(i); i = cases->next(i))
    {
        branch_class *branch = static_cast<branch_class *>(cases->nth(i));
        Symbol branch_type = branch->inference_type();
        if (check_duplicate.find(branch->get_type_decl()) != check_duplicate.end())
        {
            semant_error(curr_class->get_filename(), this) << "error!\n";
        }
        else
        {
            check_duplicate.insert(branch->get_type_decl());
        }
        if (i == cases->first())
        {
            type = branch_type;
        }
        else
        {
            type = LCA(type, branch_type);
        }
    }
    return type;
}

Symbol loop_class::inference_type()
{
    Symbol pred_type = pred->inference_type();
    if (pred_type != Bool)
    {
        semant_error(curr_class->get_filename(), this) << "error!\n";
    }
    body->inference_type();
    type = Object;
    return type;
}

Symbol cond_class::inference_type()
{
    Symbol pred_type = pred->inference_type();
    if (pred_type != Bool)
    {
        semant_error(curr_class->get_filename(), this) << "error!\n";
    }
    Symbol then_type = then_exp->inference_type();
    Symbol else_type = else_exp->inference_type();

    type = LCA(then_type, else_type);
    return type;
}

Symbol dispatch_class::inference_type()
{
    Symbol expr_type = expr->inference_type();

    method_class *method = NULL;
    Symbol curr = expr_type;
    if (curr == SELF_TYPE)
    {
        curr = curr_class->get_name();
    }
    while (true)
    {
        std::vector<method_class *> methods = method_table[curr];
        for (size_t i = 0; i < methods.size(); i++)
        {
            if (methods[i]->get_name() == name)
            {
                method = methods[i];
                break;
            }
        }
        if (method != NULL)
        {
            break;
        }
        if (curr == Object)
        {
            break;
        }
        curr = LCA_table[curr][0];
    }

    if (method == NULL)
    {
        semant_error(curr_class->get_filename(), this) << "error!\n";
        type = Object;
        return type;
    }

    bool error = false;
    Formals formals = method->get_formals();
    int ai = actual->first(), fi = formals->first();
    if (actual->more(ai) xor formals->more(fi))
    {
        error = true;
        semant_error(curr_class->get_filename(), this) << "error!\n";
    }
    while (actual->more(ai) && formals->more(fi))
    {
        Symbol actual_type = actual->nth(ai)->inference_type();
        Symbol formal_type = formals->nth(fi)->get_type();
        if (!match(actual_type, formal_type))
        {
            error = true;
            semant_error(curr_class->get_filename(), this) << "error!\n";
        }
        ai = actual->next(ai);
        fi = formals->next(fi);
        if (actual->more(ai) xor formals->more(fi))
        {
            error = true;
            semant_error(curr_class->get_filename(), this) << "error!\n";
        }
    }

    if (error)
    {
        type = Object;
    }
    else
    {
        type = method->get_return_type();
        if (type == SELF_TYPE)
        {
            type = expr_type;
        }
    }

    return type;
}

Symbol static_dispatch_class::inference_type()
{
    Symbol expr_type = expr->inference_type();

    if (this->type_name != SELF_TYPE && class_table.count(this->type_name) == 0)
    {
        semant_error(curr_class->get_filename(), this) << "error!\n";
        type = Object;
        return type;
    }

    if (expr_type != SELF_TYPE && class_table.count(expr_type) == 0)
    {
        type = Object;
        return type;
    }

    bool error = false;
    if (!match(expr_type, this->type_name))
    {
        error = true;
        semant_error(curr_class->get_filename(), this) << "error!\n";
    }

    method_class *method = NULL;
    Symbol curr = type_name;
    while (true)
    {
        std::vector<method_class *> methods = method_table[curr];
        for (size_t i = 0; i < methods.size(); i++)
        {
            if (methods[i]->get_name() == name)
            {
                method = methods[i];
                break;
            }
        }
        if (method != NULL)
        {
            break;
        }
        if (curr == Object)
        {
            break;
        }
        curr = LCA_table[curr][0];
    }

    if (method == NULL)
    {
        semant_error(curr_class->get_filename(), this) << "error!\n";
        type = Object;
        return type;
    }

    if (method == NULL)
    {
        semant_error(curr_class->get_filename(), this) << "error!\n";
        type = Object;
        return type;
    }

    Formals formals = method->get_formals();
    int ai = actual->first(), fi = formals->first();
    if (actual->more(ai) xor formals->more(fi))
    {
        error = true;
        semant_error(curr_class->get_filename(), this) << "error!\n";
    }
    while (actual->more(ai) && formals->more(fi))
    {
        Symbol actual_type = actual->nth(ai)->inference_type();
        Symbol formal_type = formals->nth(fi)->get_type();
        if (!match(actual_type, formal_type))
        {
            error = true;
            semant_error(curr_class->get_filename(), this) << "error!\n";
        }
        ai = actual->next(ai);
        fi = formals->next(fi);
        if (actual->more(ai) xor formals->more(fi))
        {
            error = true;
            semant_error(curr_class->get_filename(), this) << "error!\n";
        }
    }

    if (error)
    {
        type = Object;
    }
    else
    {
        type = method->get_return_type();
        if (type == SELF_TYPE)
        {
            type = expr_type;
        }
    }

    return type;
}

void method_class::inference_type()
{
    id_type.enterscope();
    for (int i = formals->first(); formals->more(i); i = formals->next(i))
    {
        if (formals->nth(i)->get_name() == self)
        {
            semant_error(curr_class->get_filename(), this) << "error!\n";
        }
        else if (id_type.probe(formals->nth(i)->get_name()))
        {
            semant_error(curr_class->get_filename(), this) << "error!\n";
        }
        else if (class_table.find(formals->nth(i)->get_type()) == class_table.end())
        {
            semant_error(curr_class->get_filename(), this) << "error!\n";
        }
        else
        {
            id_type.addid(formals->nth(i)->get_name(), new Symbol(formals->nth(i)->get_type()));
        }
    }
    Symbol expr_type = expr->inference_type();
    if (return_type != SELF_TYPE && class_table.count(return_type) == 0)
    {
        semant_error(curr_class->get_filename(), this) << "error!\n";
    }
    else if (!match(expr_type, return_type))
    {
        semant_error(curr_class->get_filename(), this) << "error!\n";
    }
    id_type.exitscope();
}

static void check_features()
{
    for (std::map<Symbol, Class_>::iterator iter = class_table.begin(); iter != class_table.end(); iter++)
    {
        if (iter->first == Object || iter->first == IO || iter->first == Int || iter->first == Bool || iter->first == Str)
        {
            continue;
        }

        curr_class = iter->second;

        Symbol curr = curr_class->get_name();
        while (curr != Object)
        {
            Features features = class_table[curr]->get_features();
            id_type.enterscope();
            for (int i = features->first(); features->more(i); i = features->next(i))
            {
                if (features->nth(i)->is_method())
                {
                    continue;
                }
                attr_class *attr = static_cast<attr_class *>(features->nth(i));
                if (id_type.lookup(attr->get_name()))
                {
                    semant_error(curr_class) << "error!\n";
                    continue;
                }

                id_type.addid(attr->get_name(), new Symbol(attr->get_type()));
            }
            curr = LCA_table[curr][0];
        }

        Features features = curr_class->get_features();
        for (int i = features->first(); features->more(i); i = features->next(i))
        {
            if (features->nth(i)->is_method())
            {
                method_class *curr_method = static_cast<method_class *>(features->nth(i));
                curr_method->inference_type();

                method_class *method = NULL;
                Symbol curr = curr_class->get_name();
                while (true)
                {
                    method_class *method = NULL;
                    Symbol temp = curr;
                    while (true)
                    {
                        std::vector<method_class *> methods = method_table[temp];
                        for (size_t j = 0; j < methods.size(); j++)
                        {
                            if (methods[j]->get_name() == curr_method->get_name())
                            {
                                method = methods[j];
                                break;
                            }
                        }
                        if (method != NULL)
                        {
                            break;
                        }
                        if (temp == Object)
                        {
                            break;
                        }
                        temp = LCA_table[temp][0];
                    }
                    if (method == NULL)
                    {
                        break;
                    }
                    if (curr_method->get_return_type() != method->get_return_type())
                    {
                        semant_error(curr_class->get_filename(), curr_method) << "error!\n";
                    }
                    Formals curr_formals = curr_method->get_formals();
                    Formals formals = method->get_formals();

                    int cf = curr_formals->first(), f = formals->first();
                    while (curr_formals->more(cf) && formals->more(f))
                    {
                        if (curr_formals->nth(cf)->get_type() != formals->nth(f)->get_type())
                        {
                            semant_error(curr_class->get_filename(), curr_method) << "error!\n";
                        }
                        cf = curr_formals->next(cf);
                        f = formals->next(f);
                        if (curr_formals->more(cf) xor formals->more(f))
                        {
                            semant_error(curr_class->get_filename(), curr_method) << "error!\n";
                        }
                    }
                    if (curr == Object)
                    {
                        break;
                    }
                    curr = LCA_table[curr][0];
                }
            }
            else
            {
                attr_class *curr_attr = static_cast<attr_class *>(features->nth(i));
                Symbol expr_type = curr_attr->get_expr()->inference_type();

                if (class_table.count(expr_type) != 0 && !match(expr_type, curr_attr->get_type()))
                {
                    semant_error(curr_class->get_filename(), curr_attr) << "error!\n";
                }
                if (curr_attr->get_name() == self)
                {
                    semant_error(curr_class->get_filename(), curr_attr) << "error!\n";
                }
            }
        }

        curr = curr_class->get_name();
        while (curr != Object)
        {
            id_type.exitscope();
            curr = LCA_table[curr][0];
        }
    }
}

////////////////////////////////////////////////////////////////////
//
// semant_error is an overloaded function for reporting errors
// during semantic analysis.  There are three versions:
//
//    ostream& ClassTable::semant_error()
//
//    ostream& ClassTable::semant_error(Class_ c)
//       print line number and filename for `c'
//
//    ostream& ClassTable::semant_error(Symbol filename, tree_node *t)
//       print a line number and filename
//
///////////////////////////////////////////////////////////////////

ostream &semant_error(Class_ c)
{
    return semant_error(c->get_filename(), c);
}

ostream &semant_error(Symbol filename, tree_node *t)
{
    error_stream << filename << ":" << t->get_line_number() << ": ";
    return semant_error();
}

ostream &semant_error()
{
    semant_errors++;
    return error_stream;
}

int errors()
{
    return semant_errors;
}
/*   This is the entry point to the semantic checker.

     Your checker should do the following two things:

     1) Check that the program is semantically correct
     2) Decorate the abstract syntax tree with type information
        by setting the `type' field in each Expression node.
        (see `tree.h')

     You are free to first do 1), make sure you catch all semantic
     errors. Part 2) can be done in a second stage, when you want
     to build mycoolc.
 */
void program_class::semant()
{
    initialize_constants();

    /* ClassTable constructor may do some semantic analysis */

    /* some semantic analysis code may go here */

    install_basic_classes();
    install_classes(classes);
    check_inheritance_graph();
    install_methods();
    is_main_exists();
    check_features();

    if (errors())
    {
        cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);
    }
}
