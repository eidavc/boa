#ifndef __BOA_POINTERANALYZER_H
#define __BOA_POINTERANALYZER_H

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/AST.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"

#include "log.h"
#include "buffer.h"
#include "pointer.h"

#include <list>
using std::list;
#include <map>
using std::map;

#include "constraint.h"

using namespace clang;

namespace boa {

class PointerASTVisitor : public RecursiveASTVisitor<PointerASTVisitor> {
private:
  list<Buffer> Buffers_;
  list<Pointer> Pointers_;
  map< Pointer, list<Buffer>* > Pointer2Buffers_;
  SourceManager &sm_;
public:
  PointerASTVisitor(SourceManager &SM)
    : sm_(SM) {}

  bool VisitStmt(Stmt* S) {
    if (DeclStmt* dec = dyn_cast<DeclStmt>(S)) {
      for (DeclGroupRef::iterator i = dec->decl_begin(), end = dec->decl_end(); i != end; ++i) {
        findVarDecl(*i);
      }
    }
    else if (CallExpr* funcCall = dyn_cast<CallExpr>(S)) {
      if (FunctionDecl* funcDec = funcCall->getDirectCallee())
      {
         if (funcDec->getNameInfo().getAsString() == "malloc")
         {
            addMallocToSet(funcCall,funcDec);
         }
      }
    }
    return true;
  }

  void findVarDecl(Decl *d) {
    if (VarDecl* var = dyn_cast<VarDecl>(d)) {
      Type* varType = var->getType().getTypePtr();
      // FIXME - This code only detects an array of chars
      // Array of array of chars will probably NOT detected
      if (ArrayType* arr = dyn_cast<ArrayType>(varType)) {
        if (arr->getElementType().getTypePtr()->isAnyCharacterType()) {
          addBufferToSet(var);
        }
      }
      else if (PointerType* pType = dyn_cast<PointerType>(varType)) {
        if (pType->getPointeeType()->isAnyCharacterType()) {
           addPointerToSet(var);
        }
      }
    }
  }

  void addBufferToSet(VarDecl* var) {
    Buffer b((void*)var, var->getNameAsString(), sm_.getBufferName(var->getLocation()), sm_.getSpellingLineNumber(var->getLocation()));
    Buffers_.push_back(b);

    log::os() << "Adding static buffer -" << endl;
    log::os() << " code name     = " + b.getReadableName() << endl;
    log::os() << " \"clang ID\"    = " + b.getUniqueName() << endl;
    log::os() << " code location = " + b.getSourceLocation() << endl;
  }

  void addMallocToSet(CallExpr* funcCall, FunctionDecl* func) {
    log::os() << "malloc on line " + sm_.getSpellingLineNumber(funcCall->getExprLoc()) << endl;

    Buffer b((void*)funcCall, "MALLOC", sm_.getBufferName(funcCall->getLocStart()), sm_.getSpellingLineNumber(funcCall->getLocStart()));
    Buffers_.push_back(b);
  }

  void addPointerToSet(VarDecl* var) {
    Pointer p((void*)var);
    Pointers_.push_back(p);
    Pointer2Buffers_[p] = &Buffers_;

    log::os() << "Adding pointer -" << endl;
    log::os() << " code name     = " + var->getNameAsString() << endl;
//    cerr << " \"clang ID\" = " << (void*)var << endl;
    log::os() << " line number = " + sm_.getSpellingLineNumber(var->getLocation()) << endl;
  }

  const list<Buffer>& getBuffers() const {
    return Buffers_;
  }

  const list<Pointer>& getPointers() const {
    return Pointers_;
  }

  const map< Pointer, list<Buffer>* > & getMapping() const {
    return Pointer2Buffers_;
  }
};

}  // namespace boa

#endif  // __BOA_POINTERANALYZER_H
