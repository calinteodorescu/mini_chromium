// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MINI_CHROMIUM_CRBASE_BIT_CAST_H_
#define MINI_CHROMIUM_CRBASE_BIT_CAST_H_

#include <string.h>

// bit_cast<Dest,Source> ��һ��ʵ������"*reinterpret_cast<Dest*>(&source)"��Ч
// ��ģ�庯��. ������Ҫ������ԭʼ�����Ϳ�����ѧ֧�ֵķǳ��ͼ�����
//
//   float f = 3.14159265358979;
//   int i = bit_cast<int32_t>(f);
//   // i = 0x40490fdb
//
// ���羭��ĵ�ַת������:
//
//   // ����
//   float f = 3.14159265358979;            // ����
//   int i = * reinterpret_cast<int*>(&f);  // ����
//
// ����ISO C++98 �淶, ��Լ�ڵ�3.10��15����(����C++11��Ҳû�л����ı�), �˵�ַת��ʵ
// ���ϲ�����δ�������Ϊ. ���佲��: ���һ���������ڴ�ӵ��һ�����Ͳ��ҳ���������һ����
// ͬ�����ͷ�����, �������ڴ������ֵ��˵��δ����ı���
// 
// ������κ�ת���﷨��˵�����, ������*(int*)&f ���� *reinterpret_cast<int*>(&f),
// �������ڻ�����ֵ�͸�����ֵת����ʱ��.
// 
// �淶����Ŀ����ȥ�����Ż�������ȥ������в������͵ı��ʽ���ò�ͬ���ڴ�, ��������֪��
// ȥ������һ��, ���һ�����ϸ�ĳ�������Ĵ�³�Ĳ����������
//
// ���ⲻ����reinterpret_cast��ʹ��, ����������˫���:
// һ���������ڴ���ӵ��һ������, Ȼ����һ����ͬ�����Ͷ�ȡ����λ����
//
// C++ ��׼�����΢��͸���, �������ǻ������뷨
//
// ������ô˵....
//
// bit_cast<>����memcpy()Ҳ�����Ա�׼��ĸ���, �ر���ͨ����3.9�ڵ�����, 
// ��Ȼ��bit_cast<>Ҳ�����������߼���װ��һ���ط���
//
// ���˵���memcpy�ٶȷǳ���, ���Ż�ģʽ��, ������������������������滻memcpy, ���ߴ�
// ������һ�������ڳ�����ʱ��, ��32λϵͳ��, memcpy(d,s,4) �ᱻ�����һ������һ�����أ�
// memcpy(d,s,8) �ᱻ���������������������
//
// ����: ���Ŀ��(Dest)��Ŀ��(Source) ��һ�� non-POD����, memcpy�Ľ���ܿ��ܻ�����
// ����
// POD: ��ͨ�ɰ�����, ����int, float��C��������, C++ ��Ľṹ��,�� ���Ƿ���ͨ�ɰ�����


// bit_cast<Dest,Source> is a template function that implements the equivalent
// of "*reinterpret_cast<Dest*>(&source)".  We need this in very low-level
// functions like the protobuf library and fast math support.
//
//   float f = 3.14159265358979;
//   int i = bit_cast<int32_t>(f);
//   // i = 0x40490fdb
//
// The classical address-casting method is:
//
//   // WRONG
//   float f = 3.14159265358979;            // WRONG
//   int i = * reinterpret_cast<int*>(&f);  // WRONG
//
// The address-casting method actually produces undefined behavior according to
// the ISO C++98 specification, section 3.10 ("basic.lval"), paragraph 15.
// (This did not substantially change in C++11.)  Roughly, this section says: if
// an object in memory has one type, and a program accesses it with a different
// type, then the result is undefined behavior for most values of "different
// type".
//
// This is true for any cast syntax, either *(int*)&f or
// *reinterpret_cast<int*>(&f).  And it is particularly true for conversions
// between integral lvalues and floating-point lvalues.
//// The purpose of this paragraph is to allow optimizing compilers to assume that
// expressions with different types refer to different memory.  Compilers are
// known to take advantage of this.  So a non-conforming program quietly
// produces wildly incorrect output.
//
// The problem is not the use of reinterpret_cast.  The problem is type punning:
// holding an object in memory of one type and reading its bits back using a
// different type.
//
// The C++ standard is more subtle and complex than this, but that is the basic
// idea.
//
// Anyways ...
//
// bit_cast<> calls memcpy() which is blessed by the standard, especially by the
// example in section 3.9 .  Also, of course, bit_cast<> wraps up the nasty
// logic in one place.
//
// Fortunately memcpy() is very fast.  In optimized mode, compilers replace
// calls to memcpy() with inline object code when the size argument is a
// compile-time constant.  On a 32-bit system, memcpy(d,s,4) compiles to one
// load and one store, and memcpy(d,s,8) compiles to two loads and two stores.
//
// WARNING: if Dest or Source is a non-POD type, the result of the memcpy
// is likely to surprise you.

namespace crbase {

template <class Dest, class Source>
inline Dest bit_cast(const Source& source) {
  static_assert(sizeof(Dest) == sizeof(Source),
                "bit_cast requires source and destination to be the same size");

  Dest dest;
  memcpy(&dest, &source, sizeof(dest));
  return dest;
}

}  // namespace crbase

#endif  // MINI_CHROMIUM_CRBASE_BIT_CAST_H_