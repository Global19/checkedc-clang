//=--Utils.cpp----------------------------------------------------*- C++-*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// Implementation of Utils methods.
//===----------------------------------------------------------------------===//

#include "llvm/Support/Path.h"

#include "Utils.h"
#include "ConstraintVariables.h"

using namespace clang;

const clang::Type *getNextTy(const clang::Type *Ty) {
  if(Ty->isPointerType()) {
    // TODO: how to keep the qualifiers around, and what qualifiers do
    //       we want to keep?
    QualType qtmp = Ty->getLocallyUnqualifiedSingleStepDesugaredType();
    return qtmp.getTypePtr()->getPointeeType().getTypePtr();
  }
  else
    return Ty;
}

ConstraintVariable *getHighest(std::set<ConstraintVariable*> Vs, ProgramInfo &Info) {
  if (Vs.size() == 0)
    return nullptr;

  ConstraintVariable *V = nullptr;

  for (auto &P : Vs) {
    if (V) {
      if (V->isLt(*P, Info))
        V = P;
    } else {
      V = P;
    }
  }

  return V;
}

// Walk the list of declarations and find a declaration that is NOT
// a definition and does NOT have a body.
FunctionDecl *getDeclaration(FunctionDecl *FD) {
  // optimization
  if(!FD->isThisDeclarationADefinition()) {
    return FD;
  }
  for (const auto &D : FD->redecls())
    if (FunctionDecl *tFD = dyn_cast<FunctionDecl>(D))
      if (!tFD->isThisDeclarationADefinition())
        return tFD;

  return nullptr;
}

// Walk the list of declarations and find a declaration accompanied by
// a definition and a function body.
FunctionDecl *getDefinition(FunctionDecl *FD) {
  // optimization
  if(FD->isThisDeclarationADefinition() && FD->hasBody()) {
    return FD;
  }
  for (const auto &D : FD->redecls())
    if (FunctionDecl *tFD = dyn_cast<FunctionDecl>(D))
      if (tFD->isThisDeclarationADefinition() && tFD->hasBody())
        return tFD;

  return nullptr;
}

SourceLocation
getFunctionDeclarationEnd(FunctionDecl *FD, SourceManager &S)
{
  const FunctionDecl *oFD = nullptr;

  if (FD->hasBody(oFD) && oFD == FD) {
    // Replace everything up to the beginning of the body.
    const Stmt *Body = FD->getBody(oFD);

    int Offset = 0;
    const char *Buf = S.getCharacterData(Body->getSourceRange().getBegin());

    while (*Buf != ')') {
      Buf--;
      Offset--;
    }

    return Body->getSourceRange().getBegin().getLocWithOffset(Offset);
  } else {
    return FD->getSourceRange().getEnd();
  }
}

clang::CheckedPointerKind getCheckedPointerKind(InteropTypeExpr *itypeExpr) {
  TypeSourceInfo * interopTypeInfo = itypeExpr->getTypeInfoAsWritten();
  const clang::Type *innerType = interopTypeInfo->getType().getTypePtr();
  if(innerType->isCheckedPointerNtArrayType()) {
    return CheckedPointerKind ::NtArray;
  }
  if(innerType->isCheckedPointerArrayType()) {
    return CheckedPointerKind ::Array;
  }
  if(innerType->isCheckedPointerType()) {
    return CheckedPointerKind ::Ptr;
  }
  return CheckedPointerKind::Unchecked;
}

// check if function body exists for the
// provided declaration.
bool hasFunctionBody(clang::Decl *param) {
  // if this a parameter?
  if(ParmVarDecl *PD = dyn_cast<ParmVarDecl>(param)) {
    if(DeclContext *DC = PD->getParentFunctionOrMethod()) {
      FunctionDecl *FD = dyn_cast<FunctionDecl>(DC);
      if (getDefinition(FD) != nullptr) {
        return true;
      }
    }
    return false;
  }
  // else this should be within body and
  // the function body should exist.
  return true;
}

static std::string storageClassToString(StorageClass SC) {
  switch(SC) {
    case StorageClass::SC_Static: return "static ";
    case StorageClass::SC_Extern: return "extern ";
    case StorageClass::SC_Register: return "register ";
    // for all other cases, we do not care.
    default: return "";
  }
}

// this method gets the storage qualifier for the
// provided declaration i.e., static, extern, etc.
std::string getStorageQualifierString(Decl *D) {
  if(FunctionDecl *FD = dyn_cast<FunctionDecl>(D)) {
    return storageClassToString(FD->getStorageClass());
  }
  if(VarDecl *VD = dyn_cast<VarDecl>(D)) {
    return storageClassToString(VD->getStorageClass());
  }
  return "";
}

bool isNULLExpression(clang::Expr *expr, ASTContext &Ctx) {
  // this checks if the expression is NULL. Specifically, (void*)0
  if(CStyleCastExpr *CS = dyn_cast<CStyleCastExpr>(expr)) {
    Expr *subExpr = CS->getSubExpr();

    return subExpr->isIntegerConstantExpr(Ctx) &&
           subExpr->isNullPointerConstant(Ctx, Expr::NPC_ValueDependentIsNotNull);
  }
  return false;
}

bool getAbsoluteFilePath(std::string fileName, std::string &absoluteFP) {
  // get absolute path of the provided file
  // returns true if successful else false
  SmallString<255> abs_path(fileName);
  std::error_code ec = llvm::sys::fs::make_absolute(abs_path);
  if(!ec) {
    absoluteFP = abs_path.str();
    return true;
  }
  return false;
}

bool functionHasVarArgs(clang::FunctionDecl *FD) {
  if(FD && FD->getFunctionType()->isFunctionProtoType()) {
    const FunctionProtoType *srcType = dyn_cast<FunctionProtoType>(FD->getFunctionType());
    return srcType->isVariadic();
  }
  return false;
}

bool isFunctionAllocator(std::string funcName) {
  return llvm::StringSwitch<bool>(funcName)
    .Cases("malloc", "calloc", "realloc", true)
    .Default(false);
}

float getTimeSpentInSeconds(clock_t startTime) {
  return float(clock() - startTime)/CLOCKS_PER_SEC;
}

bool isPointerType(clang::VarDecl *VD) {
  return VD->getType().getTypePtr()->isPointerType();
}

bool isStructOrUnionType(clang::VarDecl *VD) {
  return VD->getType().getTypePtr()->isStructureType() || VD->getType().getTypePtr()->isUnionType();
}

std::string tyToStr(const Type *T) {
  QualType QT(T, 0);
  return QT.getAsString();
}

Expr* removeAuxillaryCasts(Expr *srcExpr) {
  srcExpr = srcExpr->IgnoreParenImpCasts();
  if (CStyleCastExpr *C = dyn_cast<CStyleCastExpr>(srcExpr))
    srcExpr = C->getSubExpr();
  srcExpr = srcExpr->IgnoreParenImpCasts();
  return srcExpr;
}

unsigned longestCommonSubsequence(const char* str1, const char* str2, unsigned str1Len, unsigned str2Len) {
  if (str1Len == 0 || str2Len == 0)
    return 0;
  if (str1[str1Len - 1] == str2[str2Len - 1])
    return 1 + longestCommonSubsequence(str1, str2, str1Len - 1, str2Len - 1);
  else
    return std::max(longestCommonSubsequence(str1, str2, str1Len, str2Len - 1), longestCommonSubsequence(str1, str2, str1Len - 1, str2Len));
}
