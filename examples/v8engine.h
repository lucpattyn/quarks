// v8engine.cpp : Defines the entry point for the console application.
//

#ifndef _V8ENGINE_H_
#define _V8ENGINE_H_

#include <v8.h>

// Create a stack-allocated handle scope.
#define V8_ENGINE(x) \
    v8::HandleScope handle_scope; \
    v8::Persistent<v8::Context> context = v8::Context::New(); \
    v8::Context::Scope context_scope(context); \
    v8Engine x(context)

struct v8Engine{
    v8Engine(v8::Persistent<v8::Context>& ctx);
    ~v8Engine();
    
    v8::Handle<v8::Object> global;
    
    int invoke(const char* fnc, const char* element, const char* arguments);
    
    static int match(const char* arg1, const char* arg2);

};

#endif


