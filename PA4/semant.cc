

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <set>
#include <queue>
#include "semant.h"
#include "utilities.h"


extern int semant_debug;
extern char *curr_filename;

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
    arg         = idtable.add_string("arg");
    arg2        = idtable.add_string("arg2");
    Bool        = idtable.add_string("Bool");
    concat      = idtable.add_string("concat");
    cool_abort  = idtable.add_string("abort");
    copy        = idtable.add_string("copy");
    Int         = idtable.add_string("Int");
    in_int      = idtable.add_string("in_int");
    in_string   = idtable.add_string("in_string");
    IO          = idtable.add_string("IO");
    length      = idtable.add_string("length");
    Main        = idtable.add_string("Main");
    main_meth   = idtable.add_string("main");
    //   _no_class is a symbol that can't be the name of any 
    //   user-defined class.
    No_class    = idtable.add_string("_no_class");
    No_type     = idtable.add_string("_no_type");
    Object      = idtable.add_string("Object");
    out_int     = idtable.add_string("out_int");
    out_string  = idtable.add_string("out_string");
    prim_slot   = idtable.add_string("_prim_slot");
    self        = idtable.add_string("self");
    SELF_TYPE   = idtable.add_string("SELF_TYPE");
    Str         = idtable.add_string("String");
    str_field   = idtable.add_string("_str_field");
    substr      = idtable.add_string("substr");
    type_name   = idtable.add_string("type_name");
    val         = idtable.add_string("_val");
}



void ClassTable::install_class(Class_ c)
{
    Symbol name = c->get_name();

    if (name == SELF_TYPE) {
	    semant_error(c) << "Class name cannot be SELF_TYPE.\n";
	return;
    }

    if (name_to_node.find(name) != name_to_node.end()) {
	    semant_error(c) << "Class " << name << " was previously defined.\n";
	return;
    }

    name_to_node[name] = c;
}

Class_ ClassTable::get_class(Symbol name)
{
    auto it = name_to_node.find(name);
    if (it == name_to_node.end()) return NULL;
    return it->second;
}

Symbol ClassTable::resolve_type(Symbol t, Class_ cur)
{
    if (t == SELF_TYPE)
	return cur->get_name();
    return t;
}

bool ClassTable::walk_subtype(Symbol child, Symbol parent)
{
    while (child != No_class) {
	if (child == parent)
	    return true;
	Class_ child_class = get_class(child);
	if (child_class == NULL)
	    return false;
	child = child_class->get_parent();
    }
    return false;
}

bool ClassTable::is_subtype(Symbol child, Symbol parent, Class_ cur)
{
    if (child == parent)
	return true;

    // T <= SELF_TYPE only when T is SELF_TYPE
    if (parent == SELF_TYPE)
	return child == SELF_TYPE;

    if (child == SELF_TYPE)
	child = cur->get_name();

    if (child == parent)
	return true;

    return walk_subtype(child, parent);
}

bool ClassTable::conforms(Symbol child, Symbol parent, Class_ cur)
{
    child = resolve_type(child, cur);
    parent = resolve_type(parent, cur);
    if (child == parent)
	return true;
    return walk_subtype(child, parent);
}

bool ClassTable::type_is_valid(Symbol t, bool allow_self_type)
{
    if (t == SELF_TYPE)
	return allow_self_type;
    if (t == prim_slot)
	return true;
    return get_class(t) != NULL;
}

Symbol ClassTable::lub(Symbol t1, Symbol t2, Class_ cur) {
    if (t1 == No_type)
	return t2;
    if (t2 == No_type)
	return t1;

    std::set<Symbol> ancestors;
    Symbol c = t1;
    for (;;) {
	Symbol type = (c == SELF_TYPE) ? cur->get_name() : c;
	ancestors.insert(type);
	if (type == Object)
	    break;
	Class_ type_class = get_class(type);
	if (type_class == NULL)
	    break;
	c = type_class->get_parent();
    }

    c = t2;
    for (;;) {
	Symbol type = (c == SELF_TYPE) ? cur->get_name() : c;
	if (ancestors.find(type) != ancestors.end())
	    return type;
	if (type == Object)
	    return Object;
	Class_ type_class = get_class(type);
	if (type_class == NULL)
	    return Object;
	c = type_class->get_parent();
    }
}

void ClassTable::build_children_map()
{
    for (auto &[name, c] : name_to_node) {
	Symbol parent = c->get_parent();
	if (parent != No_class)
	    children_map[parent].push_back(c);
    }
}

void ClassTable::check_attr(Class_ c, attr_class *a, std::set<Symbol> &local_names)
{
    Symbol cname = c->get_name();
    Symbol aname = a->get_name();
    Symbol atype = a->get_type_decl();

    if (aname == self) {
	semant_error(c) << "'self' cannot be the name of an attribute.\n";
	return;
    }

    if (local_names.find(aname) != local_names.end()) {
	semant_error(c) << "Attribute " << aname << " is multiply defined in class " << cname << ".\n";
	return;
    }
    local_names.insert(aname);

    if (attr_env[cname].find(aname) != attr_env[cname].end()) {
	semant_error(c) << "Attribute " << aname << " is an attribute of class "
			<< cname << ".\n";
	return;
    }

    if (!type_is_valid(atype, true)) {
	semant_error(c) << "Class " << cname << " has attribute " << aname
			<< " of unknown type " << atype << ".\n";
	return;
    }

    attr_env[cname][aname] = atype;
}

void ClassTable::check_method(Class_ c, method_class *m, std::set<Symbol> &local_names)
{
    Symbol cname = c->get_name();
    Symbol mname = m->get_name();
    Symbol rtype = m->get_return_type();
    Formals formals = m->get_formals();

    if (local_names.find(mname) != local_names.end()) {
	semant_error(c) << "Method " << mname << " is multiply defined in class " << cname << ".\n";
	return;
    }
    local_names.insert(mname);

    if (!type_is_valid(rtype, true)) {
	semant_error(c) << "Class " << cname << " has method " << mname
			<< " with unknown return type " << rtype << ".\n";
	return;
    }

    std::set<Symbol> formal_names;
    for (int i = formals->first(); formals->more(i); i = formals->next(i)) {
	Formal f = formals->nth(i);
	Symbol fname = f->get_name();
	Symbol ftype = f->get_type_decl();

	if (fname == self) {
	    semant_error(c) << "'self' cannot be the name of a formal parameter.\n";
	    return;
	}

	if (formal_names.find(fname) != formal_names.end()) {
	    semant_error(c) << "Formal parameter " << fname << " is multiply defined.\n";
	    return;
	}
	formal_names.insert(fname);

	if (!type_is_valid(ftype, false)) {
	    semant_error(c) << "Class " << cname << " has formal parameter "
			    << fname << " of unknown type " << ftype << ".\n";
	    return;
	}
    }

    if (method_env[cname].find(mname) != method_env[cname].end()) {
	method_class *parent_m = method_env[cname][mname];
	Formals pformals = parent_m->get_formals();

	if (pformals->len() != formals->len()) {
	    semant_error(c) << "Incompatible number of formal parameters in redefined method.\n";
	    return;
	}

	for (int i = formals->first(), j = pformals->first();
	     formals->more(i);
	     i = formals->next(i), j = pformals->next(j)) {
	    Symbol ctype = formals->nth(i)->get_type_decl();
	    Symbol ptype = pformals->nth(j)->get_type_decl();
	    if (ctype != ptype) {
		semant_error(c) << "In redefined method, parameter type "
				<< ctype << " is different from original type "
				<< ptype << ".\n";
		return;
	    }
	}

	if (!conforms(rtype, parent_m->get_return_type(), c)) {
	    semant_error(c) << "In redefined method, return type "
			    << rtype << " is not a subtype of "
			    << parent_m->get_return_type() << ".\n";
	    return;
	}
    }

    method_env[cname][mname] = m;
}

void ClassTable::check_class_features(Class_ c)
{
    Symbol cname = c->get_name();
    Symbol parent = c->get_parent();

    if (parent != No_class) {
	method_env[cname] = method_env[parent];
	attr_env[cname] = attr_env[parent];
    } else {
	method_env[cname].clear();
	attr_env[cname].clear();
    }

    std::set<Symbol> local_names;
    Features features = c->get_features();
    for (int i = features->first(); features->more(i); i = features->next(i)) {
	Feature f = features->nth(i);
	if (method_class *m = dynamic_cast<method_class *>(f))
	    check_method(c, m, local_names);
	else if (attr_class *a = dynamic_cast<attr_class *>(f))
	    check_attr(c, a, local_names);
    }
}

void ClassTable::build_feature_tables()
{
    build_children_map();

    std::queue<Class_> pending;
    Class_ object_class = get_class(Object);
    if (object_class == NULL)
	return;

    pending.push(object_class);
    while (!pending.empty()) {
	Class_ c = pending.front();
	pending.pop();
	check_class_features(c);

	for (Class_ child : children_map[c->get_name()])
	    pending.push(child);
    }
}

method_class *ClassTable::lookup_method(Symbol class_name, Symbol method_name)
{
    auto class_it = method_env.find(class_name);
    if (class_it == method_env.end())
	return NULL;
    auto method_it = class_it->second.find(method_name);
    if (method_it == class_it->second.end())
	return NULL;
    return method_it->second;
}

Symbol ClassTable::lookup_attr(Symbol class_name, Symbol attr_name)
{
    auto class_it = attr_env.find(class_name);
    if (class_it == attr_env.end())
	return NULL;
    auto attr_it = class_it->second.find(attr_name);
    if (attr_it == class_it->second.end())
	return NULL;
    return attr_it->second;
}

void ClassTable::check_main()
{
    Class_ main_class = get_class(Main);
    if (main_class == NULL) {
	semant_error() << "Class Main is not defined.\n";
	return;
    }

    method_class *main_method = lookup_method(Main, main_meth);
    if (main_method == NULL) {
	semant_error(main_class) << "No 'main' method in class Main.\n";
	return;
    }

    if (main_method->get_formals()->len() != 0) {
	semant_error(main_class) << "Main method in class Main has formal parameters.\n";
    }
}

void ClassTable::check_class_hierarchy() {
    for (auto &[_, c] : name_to_node) {
        Symbol parent = c->get_parent();

        //First handle Object
        if (parent == No_class) continue;

        if (parent == Int || parent == Bool || parent == Str || parent == SELF_TYPE) {
            semant_error(c) << "Class " << c->get_name() << " cannot inherit from " << parent << ".\n";
            continue;
        } else if (name_to_node.find(parent) == name_to_node.end()) {
            semant_error(c) << "Class " << c->get_name() << " inherits from undefined class " << parent << ".\n";
            continue;
        }

        //check for cycles
        Symbol current = parent;
        std::set<Symbol> visited;
        while (current != No_class) {
            if (visited.find(current) != visited.end()) {
                semant_error(c) << "Inheritance cycle found for " << c->get_name() << endl;
                break;
            }
            visited.insert(current);
            auto it = name_to_node.find(current);
            if (it == name_to_node.end()) break;
            current = it->second->get_parent();
        }
        
    }
}


ClassTable::ClassTable(Classes classes) : semant_errors(0) , error_stream(cerr) {
    install_basic_classes();

    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
	install_class(classes->nth(i));
    }
    check_class_hierarchy();
    if (errors())
	return;
    build_feature_tables();
    check_main();
}

void ClassTable::install_basic_classes() {

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
	class_(Bool, Object, single_Features(attr(val, prim_slot, no_expr())),filename);

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

    install_class(Object_class);
    install_class(IO_class);
    install_class(Int_class);
    install_class(Bool_class);
    install_class(Str_class);
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

ostream& ClassTable::semant_error(Class_ c)
{                                                             
    return semant_error(c->get_filename(),c);
}    

ostream& ClassTable::semant_error(Symbol filename, tree_node *t)
{
    error_stream << filename << ":" << t->get_line_number() << ": ";
    return semant_error();
}

ostream& ClassTable::semant_error()                  
{                                                 
    semant_errors++;                            
    return error_stream;
} 

static bool is_basic_type(Symbol t)
{
    return t == Int || t == Bool || t == Str;
}

void ClassTable::init_class(type_env_t &env)
{
    env.objenv->enterscope();
    Symbol cname = env.curr->get_name();
    env.objenv->addid(self, new Symbol(cname));

    for (auto const &entry : attr_env[cname])
	env.objenv->addid(entry.first, new Symbol(entry.second));
}

void ClassTable::type_check_attr(type_env_t &env, attr_class *a)
{
    Expression init = a->get_init();
    if (dynamic_cast<no_expr_class *>(init) != NULL)
	return;

    Symbol init_type = init->type_check(env);
    if (!conforms(init_type, a->get_type_decl(), env.curr)) {
	semant_error(env.curr) << "Inferred type " << init_type
			       << " of attribute initializer does not conform to declared attribute type "
			       << a->get_type_decl() << ".\n";
    }
}

void ClassTable::type_check_method(type_env_t &env, method_class *m)
{
    env.objenv->enterscope();
    Formals formals = m->get_formals();
    for (int i = formals->first(); formals->more(i); i = formals->next(i)) {
	Formal f = formals->nth(i);
	env.objenv->addid(f->get_name(), new Symbol(f->get_type_decl()));
    }

    Symbol body_type = m->get_expr()->type_check(env);
    Symbol ret_type = m->get_return_type();
    if (!conforms(body_type, ret_type, env.curr)) {
	semant_error(env.curr) << "Inferred return type " << body_type
			       << " of method " << m->get_name()
			       << " does not conform to declared return type "
			       << ret_type << ".\n";
    }
    env.objenv->exitscope();
}

Symbol ClassTable::type_check_dispatch(type_env_t &env, Expression caller,
				       Expression expr, Symbol static_type,
				       Symbol name, Expressions actual,
				       bool is_static)
{
    Symbol expr_type = expr->type_check(env);
    Symbol lookup_type = resolve_type(static_type, env.curr);

    if (is_static) {
	if (!is_subtype(expr_type, lookup_type, env.curr)) {
	    semant_error(env.curr->get_filename(), caller) << "Expression type " << expr_type
					   << " is not compatible with declared static dispatch type "
					   << lookup_type << ".\n";
	    caller->set_type(Object);
	    return Object;
	}
    } else {
	lookup_type = resolve_type(expr_type, env.curr);
    }

    method_class *method = lookup_method(lookup_type, name);
    if (method == NULL) {
	if (is_static)
	    semant_error(env.curr->get_filename(), caller) << "Static dispatch to undefined method "
					   << name << ".\n";
	else
	    semant_error(env.curr->get_filename(), caller) << "Dispatch to undefined method "
					   << name << ".\n";
	caller->set_type(Object);
	return Object;
    }

    Formals formals = method->get_formals();
    if (formals->len() != actual->len()) {
	semant_error(env.curr->get_filename(), caller) << "Method " << name
				       << " called with wrong number of arguments.\n";
	caller->set_type(Object);
	return Object;
    }

    for (int i = formals->first(), j = actual->first();
	 formals->more(i);
	 i = formals->next(i), j = actual->next(j)) {
	Symbol formal_type = formals->nth(i)->get_type_decl();
	Symbol actual_type = actual->nth(j)->type_check(env);
	if (!conforms(actual_type, formal_type, env.curr)) {
	    semant_error(env.curr->get_filename(), caller) << "In call of method " << name
					   << ", type " << actual_type
					   << " of parameter " << formals->nth(i)->get_name()
					   << " does not conform to declared type "
					   << formal_type << ".\n";
	}
    }

    Symbol ret_type = method->get_return_type();
    if (ret_type == SELF_TYPE)
	ret_type = lookup_type;
    else
	ret_type = resolve_type(ret_type, env.curr);

    caller->set_type(ret_type);
    return ret_type;
}

void ClassTable::type_check_class(Class_ c)
{
    type_env_t env;
    env.ct = this;
    env.curr = c;
    env.objenv = new SymbolTable<Symbol, Symbol>();
    init_class(env);

    Features features = c->get_features();
    for (int i = features->first(); features->more(i); i = features->next(i)) {
	Feature f = features->nth(i);
	if (attr_class *a = dynamic_cast<attr_class *>(f))
	    type_check_attr(env, a);
	else if (method_class *m = dynamic_cast<method_class *>(f))
	    type_check_method(env, m);
    }
}

void ClassTable::type_check_classes(Classes classes)
{
    for (int i = classes->first(); classes->more(i); i = classes->next(i))
	type_check_class(classes->nth(i));
}

Symbol assign_class::type_check(type_env_t &env)
{
    Symbol *var_type = env.objenv->lookup(name);
    if (var_type == NULL) {
	env.ct->semant_error(env.curr->get_filename(), this) << "Assignment to undefined variable "
					     << name << ".\n";
	set_type(Object);
	return type;
    }

    Symbol expr_type = expr->type_check(env);
    if (!env.ct->conforms(expr_type, *var_type, env.curr)) {
	env.ct->semant_error(env.curr->get_filename(), this) << "Type " << expr_type
					     << " of assigned expression does not conform to declared type "
					     << *var_type << " of identifier " << name << ".\n";
    }
    set_type(expr_type);
    return type;
}

Symbol static_dispatch_class::type_check(type_env_t &env)
{
    return env.ct->type_check_dispatch(env, this, expr, type_name, name, actual, true);
}

Symbol dispatch_class::type_check(type_env_t &env)
{
    return env.ct->type_check_dispatch(env, this, expr, SELF_TYPE, name, actual, false);
}

Symbol cond_class::type_check(type_env_t &env)
{
    Symbol pred_type = pred->type_check(env);
    if (pred_type != Bool) {
	env.ct->semant_error(env.curr->get_filename(), this) << "Predicate of 'if' does not have type Bool.\n";
    }

    Symbol then_type = then_exp->type_check(env);
    Symbol else_type = else_exp->type_check(env);
    Symbol result = env.ct->lub(then_type, else_type, env.curr);
    set_type(result);
    return type;
}

Symbol loop_class::type_check(type_env_t &env)
{
    Symbol pred_type = pred->type_check(env);
    if (pred_type != Bool) {
	env.ct->semant_error(env.curr->get_filename(), this) << "Loop condition does not have type Bool.\n";
    }
    body->type_check(env);
    set_type(Object);
    return type;
}

Symbol typcase_class::type_check(type_env_t &env)
{
    Symbol expr_type = expr->type_check(env);
    Symbol result = No_type;
    std::set<Symbol> seen;

    for (int i = cases->first(); cases->more(i); i = cases->next(i)) {
	branch_class *branch = dynamic_cast<branch_class *>(cases->nth(i));
	Symbol branch_type = branch->get_type_decl();

	if (!env.ct->type_is_valid(branch_type, false)) {
	    env.ct->semant_error(env.curr->get_filename(), this) << "In case branch, type "
						 << branch_type << " is undefined.\n";
	    continue;
	}

	if (!env.ct->is_subtype(branch_type, expr_type, env.curr)) {
	    env.ct->semant_error(env.curr->get_filename(), this) << "In case branch, type "
						 << branch_type
						 << " is not compatible with type "
						 << expr_type << " of expression.\n";
	}

	if (seen.find(branch_type) != seen.end()) {
	    env.ct->semant_error(env.curr->get_filename(), this) << "Duplicate branch type "
						 << branch_type << " in case statement.\n";
	}
	seen.insert(branch_type);

	env.objenv->enterscope();
	env.objenv->addid(branch->get_name(), new Symbol(branch_type));
	Symbol branch_expr_type = branch->get_expr()->type_check(env);
	env.objenv->exitscope();

	result = env.ct->lub(result, branch_expr_type, env.curr);
    }

    if (result == No_type)
	result = Object;
    set_type(result);
    return type;
}

Symbol block_class::type_check(type_env_t &env)
{
    Symbol result = Object;
    for (int i = body->first(); body->more(i); i = body->next(i))
	result = body->nth(i)->type_check(env);
    set_type(result);
    return type;
}

Symbol let_class::type_check(type_env_t &env)
{
    if (identifier == self) {
	env.ct->semant_error(env.curr->get_filename(), this) << "'self' cannot be bound in a 'let' expression.\n";
	set_type(Object);
	return type;
    }

    if (!env.ct->type_is_valid(type_decl, false)) {
	env.ct->semant_error(env.curr->get_filename(), this) << "Let variable " << identifier
					     << " has undefined type " << type_decl << ".\n";
	set_type(Object);
	return type;
    }

    if (dynamic_cast<no_expr_class *>(init) == NULL) {
	Symbol init_type = init->type_check(env);
	if (!env.ct->conforms(init_type, type_decl, env.curr)) {
	    env.ct->semant_error(env.curr->get_filename(), this) << "Inferred type " << init_type
						 << " of initialization of " << identifier
						 << " does not conform to declared type "
						 << type_decl << ".\n";
	}
    }

    env.objenv->enterscope();
    if (env.objenv->probe(identifier) != NULL) {
	env.ct->semant_error(env.curr->get_filename(), this) << "Variable " << identifier
					     << " is bound twice in the same block.\n";
    } else {
	env.objenv->addid(identifier, new Symbol(type_decl));
    }

    Symbol body_type = body->type_check(env);
    env.objenv->exitscope();
    set_type(body_type);
    return type;
}

static Symbol check_arith(type_env_t &env, Expression e1, Expression e2, Expression t)
{
    Symbol t1 = e1->type_check(env);
    Symbol t2 = e2->type_check(env);
    if (t1 != Int || t2 != Int)
	env.ct->semant_error(env.curr->get_filename(), t) << "non-Int arguments: " << t1 << " + " << t2 << ".\n";
    t->set_type(Int);
    return Int;
}

Symbol plus_class::type_check(type_env_t &env)
{
    return check_arith(env, e1, e2, this);
}

Symbol sub_class::type_check(type_env_t &env)
{
    return check_arith(env, e1, e2, this);
}

Symbol mul_class::type_check(type_env_t &env)
{
    return check_arith(env, e1, e2, this);
}

Symbol divide_class::type_check(type_env_t &env)
{
    return check_arith(env, e1, e2, this);
}

Symbol neg_class::type_check(type_env_t &env)
{
    Symbol t1 = e1->type_check(env);
    if (t1 != Int)
	env.ct->semant_error(env.curr->get_filename(), this) << "Argument of '~' has type " << t1
					     << " instead of Int.\n";
    set_type(Int);
    return type;
}

Symbol lt_class::type_check(type_env_t &env)
{
    Symbol t1 = e1->type_check(env);
    Symbol t2 = e2->type_check(env);
    if (t1 != Int || t2 != Int)
	env.ct->semant_error(env.curr->get_filename(), this) << "non-Int arguments: " << t1 << " < " << t2 << ".\n";
    set_type(Bool);
    return type;
}

Symbol leq_class::type_check(type_env_t &env)
{
    Symbol t1 = e1->type_check(env);
    Symbol t2 = e2->type_check(env);
    if (t1 != Int || t2 != Int)
	env.ct->semant_error(env.curr->get_filename(), this) << "non-Int arguments: " << t1 << " <= " << t2 << ".\n";
    set_type(Bool);
    return type;
}

Symbol eq_class::type_check(type_env_t &env)
{
    Symbol t1 = e1->type_check(env);
    Symbol t2 = e2->type_check(env);
    if (!is_basic_type(t1) || !is_basic_type(t2) || t1 != t2) {
	env.ct->semant_error(env.curr->get_filename(), this) << "Illegal comparison with type "
					     << t1 << " = " << t2 << ".\n";
    }
    set_type(Bool);
    return type;
}

Symbol comp_class::type_check(type_env_t &env)
{
    Symbol t1 = e1->type_check(env);
    if (t1 != Bool)
	env.ct->semant_error(env.curr->get_filename(), this) << "Argument of 'not' has type "
					     << t1 << " instead of Bool.\n";
    set_type(Bool);
    return type;
}

Symbol int_const_class::type_check(type_env_t &)
{
    set_type(Int);
    return type;
}

Symbol bool_const_class::type_check(type_env_t &)
{
    set_type(Bool);
    return type;
}

Symbol string_const_class::type_check(type_env_t &)
{
    set_type(Str);
    return type;
}

Symbol new__class::type_check(type_env_t &env)
{
    if (type_name == SELF_TYPE) {
	env.ct->semant_error(env.curr->get_filename(), this) << "'new SELF_TYPE' is not meaningful.\n";
	set_type(Object);
	return type;
    }

    if (!env.ct->type_is_valid(type_name, false)) {
	env.ct->semant_error(env.curr->get_filename(), this) << "'new' used with undefined type "
					     << type_name << ".\n";
	set_type(Object);
	return type;
    }

    set_type(type_name);
    return type;
}

Symbol isvoid_class::type_check(type_env_t &env)
{
    e1->type_check(env);
    set_type(Bool);
    return type;
}

Symbol no_expr_class::type_check(type_env_t &)
{
    set_type(No_type);
    return type;
}

Symbol object_class::type_check(type_env_t &env)
{
    if (name == self) {
	set_type(SELF_TYPE);
	return type;
    }

    Symbol *obj_type = env.objenv->lookup(name);
    if (obj_type == NULL) {
	env.ct->semant_error(env.curr->get_filename(), this) << "Object " << name
					     << " does not have a type.\n";
	set_type(Object);
	return type;
    }

    set_type(*obj_type);
    return type;
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
    ClassTable *classtable = new ClassTable(classes);

    if (!classtable->errors())
	classtable->type_check_classes(classes);

    if (classtable->errors()) {
	cerr << "Compilation halted due to static semantic errors." << endl;
	exit(1);
    }
}

