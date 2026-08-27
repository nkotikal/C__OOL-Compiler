#ifndef SEMANT_H_
#define SEMANT_H_

#include <assert.h>
#include <iostream>  
#include "cool-tree.h"
#include "stringtab.h"
#include "symtab.h"
#include "list.h"
#include <map>
#include <set>
#include <vector>
#include <queue>

#define TRUE 1
#define FALSE 0

class ClassTable;
typedef ClassTable *ClassTableP;

struct type_env_t {
  ClassTable *ct;
  Class_ curr;
  SymbolTable<Symbol, Symbol> *objenv;
};

class ClassTable {
private:
  int semant_errors;
  void install_basic_classes();
  void install_class(Class_ c);
  void check_class_hierarchy();
  void build_children_map();
  void build_feature_tables();
  void check_class_features(Class_ c);
  void check_attr(Class_ c, attr_class *a, std::set<Symbol> &local_names);
  void check_method(Class_ c, method_class *m, std::set<Symbol> &local_names);
  void check_main();
  void type_check_class(Class_ c);
  void init_class(type_env_t &env);
  void type_check_attr(type_env_t &env, attr_class *a);
  void type_check_method(type_env_t &env, method_class *m);
  ostream& error_stream;
  std::map<Symbol, Class_> name_to_node;
  std::map<Symbol, std::vector<Class_> > children_map;
  std::map<Symbol, std::map<Symbol, method_class *> > method_env;
  std::map<Symbol, std::map<Symbol, Symbol> > attr_env;

  Class_ get_class(Symbol name);
  bool walk_subtype(Symbol child, Symbol parent);

public:
  ClassTable(Classes);
  int errors() { return semant_errors; }
  ostream& semant_error();
  ostream& semant_error(Class_ c);
  ostream& semant_error(Symbol filename, tree_node *t);
  method_class *lookup_method(Symbol class_name, Symbol method_name);
  Symbol lookup_attr(Symbol class_name, Symbol attr_name);
  bool type_is_valid(Symbol t, bool allow_self_type);
  void type_check_classes(Classes classes);
  Symbol type_check_dispatch(type_env_t &env, Expression caller, Expression expr,
			     Symbol static_type, Symbol name, Expressions actual,
			     bool is_static);
  Symbol resolve_type(Symbol t, Class_ cur);
  bool is_subtype(Symbol child, Symbol parent, Class_ cur);
  bool conforms(Symbol child, Symbol parent, Class_ cur);
  Symbol lub(Symbol t1, Symbol t2, Class_ cur);
};


#endif
