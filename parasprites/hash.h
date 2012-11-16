#pragma once
#define BEGIN_HASH(MyType) namespace std {template <> struct hash<MyType>{ size_t operator()(const MyType& arg) const
#define END_HASH };}
