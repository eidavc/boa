#ifndef __BOA_CONSTRAINT_H
#define __BOA_CONSTRAINT_H /* */

#include <limits>
#include <map>
#include <set>
#include <string>

#include <glpk.h>

#include "Buffer.h"
#include "Helpers.h"

using std::string;
using std::set;
using std::map;

// DEBUG
#include <sstream>

namespace boa {
/**
  Model a single constraint.

  A constraint is a simple linear inequality of the form BIG >= SMALL
  Use addBig in order to add an expression, literal or a constant
  to the BIG side of the inequality, or the equivalent functions for the SMALL
  side.

  For internal use, the constraint is stored in the form of -

  C >= aX + bY ...

  Where C is a constant (referred as "left"), a small letter (a, b...) is an
  integer value ("num") and a capital letter (X, Y...) is a string name of variable.
*/
class Constraint {
 private:
  const static int MAX_SIZE = 100;
  double left_;
  map<string, double> literals_;
  string blame_;

  void addLiteral(double num, string var) {
    literals_[var] += num;
  }

  void addLeft(double left) {
    left_ += left;
  }

  // TODO(tzafrir): Disallow copying and assignment.

 public:
  class Expression {
    double val_;
    map<string, double> vars_;

   public:
    friend class Constraint;
    Expression() : val_(0.0) {}
    void add(const Expression& expr) {
      for (map<string, double>::const_iterator it = expr.vars_.begin();
           it != expr.vars_.end();
           ++it) {
        add(it->first, it->second);
      }
      add(expr.val_);
    }
    void add(const string& var, double num = 1.0) {vars_[var] += num;}
    void add(double num) {val_ += num;}

    void sub(const Expression& expr) {
      for (map<string, double>::const_iterator it = expr.vars_.begin();
           it != expr.vars_.end();
           ++it) {
        add(it->first, -it->second);
      }
      add(-expr.val_);
    }

    void mul(double num) {
      for (map<string, double>::iterator it = vars_.begin(); it != vars_.end(); ++it) {
        it->second *= num;
      }
      val_ *= num;
    }

    void div(double num) {
      for (map<string, double>::iterator it = vars_.begin(); it != vars_.end(); ++it) {
        it->second /= num;
      }
      val_ /= num;
    }

    /**
      Does the expression contain only a free element (no literals)?
    */
    bool IsConst() {
      for (map<string, double>::const_iterator it = vars_.begin(); it != vars_.end(); ++it) {
        if (it->second != 0) {
          return false;
        }
      }
      return true;
    }

    double GetConst() {
      return val_;
    }

    string toString() const {
      string s;
      for (map<string, double>::const_iterator it = vars_.begin(); it != vars_.end(); ++it) {
        if ((!s.empty()) && (it->second >= 0)) s += "+ ";
        s += Helpers::DoubleToString(it->second) + "*" + it->first + " ";
      }
      if (s.empty() || (val_ != 0)) {
        if ((!s.empty()) && (val_ >= 0)) s += "+ ";
        s += Helpers::DoubleToString(val_);
      }
      return s;
    }
  };

  Constraint() : left_(0), blame_("") {}
  Constraint(const string &blame, const string &location = "") : left_(0), blame_(blame) {}

  Constraint(const string &varExpr, const Expression &value, VarLiteral::ExpressionDir direction) {
	switch (direction) {
	  case VarLiteral::MAX:
		  addSmall(varExpr);
		  addBig(value);
		  break;
	  case VarLiteral::MIN:
		  addBig(value);
		  addSmall(varExpr);
		  break;
	}
  }

  void SetBlame(const string &blame, const string &location = "") {
    blame_ = blame;
  }

  string Blame() {
    return blame_;
  }

  void addBig(const Expression& expr) {
    for (map<string, double>::const_iterator it = expr.vars_.begin();
         it != expr.vars_.end();
         ++it) {
      addBig(it->first, it->second);
    }
    addBig(expr.val_);
  }

  void addBig(const string& var, double num = 1.0) {
    addLiteral(-num, var);
  }

  void addBig(double num) {
    addLeft(num);
  }

  void addSmall(const Expression& expr) {
    for (map<string, double>::const_iterator it = expr.vars_.begin();
         it != expr.vars_.end();
         ++it) {
      addSmall(it->first, it->second);
    }
    addSmall(expr.val_);
  }

  void addSmall(const string& var, double num = 1.0) {
    addLiteral(num, var);
  }

  void addSmall(double num) {
    addLeft(-num);
  }

  void Clear() {
   left_ = 0;
   literals_.clear();
  }

  void GetVars(set<string>& vars) const {
    for (map<string, double>::const_iterator it = literals_.begin(); it != literals_.end(); ++it) {
      vars.insert(it->first);
    }
  }

  void AddToLPP(glp_prob *lp, int row, map<string, int>& colNumbers) const {
    int indices[MAX_SIZE + 1];
    double values[MAX_SIZE + 1];

    // TODO if size > MAX_SIZE...

    int count = 1;
    for (map<string, double>::const_iterator it = literals_.begin();
         it != literals_.end();
         ++it, ++count) {
      indices[count] = colNumbers[it->first];
      values[count] = it->second;
    }
    glp_set_row_bnds(lp, row, GLP_UP, 0.0, left_);
    glp_set_mat_row(lp, row, literals_.size(), indices, values);
    glp_set_row_name(lp, row, blame_.c_str());
  }

};

} //namespace boa
#endif /* __BOA_CONSTRAINT_H */

