// Tests for 3C.
//
// Tests for rewriting global declarations.
//
// RUN: 3c -addcr %s -- | FileCheck -match-full-lines %s
// RUN: 3c -addcr %s -- | %clang_cc1  -fno-builtin -verify -fcheckedc-extension -x c -
// RUN: 3c -addcr -output-postfix=checked %s 
// RUN: 3c -addcr %S/global.checked.c -- | count 0
// RUN: rm %S/global.checked.c
// expected-no-diagnostics
#include <stddef.h> 
char *c;
//CHECK: char *c;
int *p,*q;
//CHECK: _Ptr<int> p = ((void *)0);
//CHECK-NEXT: int *q;
int *p1, *q1;
//CHECK: _Ptr<int> p1 = ((void *)0);
//CHECK-NEXT: _Ptr<int> q1 = ((void *)0);
int main(void) { 
  q = (int*)(0xdeadbeef);
  c = (char*)(0xdeadbeef);
  return 0;
}
