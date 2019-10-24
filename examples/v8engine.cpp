// v8engine.cpp : Defines the entry point for the console application.
//


#include <v8engine.h>
#include <string.h>
#include <stdlib.h>

v8Engine::v8Engine(v8::Persistent<v8::Context>& ctx){
    
    
    //context->AllowCodeGenerationFromStrings(true);
    
    // Enter the created context for compiling and
    // running the hello world script.
    v8::Handle<v8::String> source;
    v8::Handle<v8::Script> script;
    v8::Handle<v8::Value> result;
    
    
    // Create a string containing the JavaScript source code.
    source = v8::String::New("function matcher() { var elem = arguments[0]; \
                             var args = JSON.parse(arguments[1]); \
                             var match = 0;if(args[0] == args[1]) { match = 1; } return match; }");
    
    // Compile the source code.
    script = v8::Script::Compile(source);
    
    // Run the script to get the result.
    result = script->Run();
    
    // Dispose the persistent context.
    ctx.Dispose();
    
    global = ctx->Global();
    

}

v8Engine::~v8Engine(){
    
}

int v8Engine::invoke(const char* fnc, const char* element, const char* arguments){
    
    v8::Handle<v8::Value> value = global->Get(v8::String::New(fnc));
    v8::Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(value);
    
    v8::Handle<v8::Value> args[2];
    args[0] = v8::String::New(element);
    args[1] = v8::String::New(arguments);
    
    v8::Handle<v8::Value> js_result = func->Call(global, 2, args);
    v8::String::AsciiValue ascii(js_result);
    
    int final_result = atoi(*ascii);
    
    if(final_result == 1) {
        
        printf("Matched\n");
        
    } else {
        
        printf("NOT Matched\n");
        
    }
    
    
    return final_result;
}

// example codes
/*
int v8Engine::match(const char* arg1, const char* arg2){
    
    // Create a stack-allocated handle scope.
    v8::HandleScope handle_scope;

    // Create a new context.
    v8::Persistent<v8::Context> context = v8::Context::New();

    //context->AllowCodeGenerationFromStrings(true);

    // Enter the created context for compiling and
    // running the hello world script.
    v8::Context::Scope context_scope(context);
    v8::Handle<v8::String> source;
    v8::Handle<v8::Script> script;
    v8::Handle<v8::Value> result;


    // Create a string containing the JavaScript source code.
    source = v8::String::New("function match_function() { var match = 0;if(arguments[0] == arguments[1]) { match = 1; } return match; }");

    // Compile the source code.
    script = v8::Script::Compile(source);

    // Run the script to get the result.
    result = script->Run();

    // Dispose the persistent context.
    context.Dispose();
    
    // Convert the result to an ASCII string and print it.
    //String::AsciiValue ascii(result);
    //printf("%s\n", *ascii);

    v8::Handle<v8::Object> global = context->Global();
    v8::Handle<v8::Value> value = global->Get(v8::String::New("match_function"));
    v8::Handle<v8::Function> func = v8::Handle<v8::Function>::Cast(value);
    v8::Handle<v8::Value> args[2];
    v8::Handle<v8::Value> js_result;
    int final_result;

    args[0] = v8::String::New(arg1);
    args[1] = v8::String::New(arg2);

    js_result = func->Call(global, 2, args);
    v8::String::AsciiValue ascii(js_result);

    final_result = atoi(*ascii);

    if(final_result == 1) {
    
        printf("Matched\n");
    
    } else {
    
        printf("NOT Matched\n");
    
    }

   
    return final_result;
}
    
*/


