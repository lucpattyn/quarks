// v8_embedded_demo.cpp : Defines the entry point for the console application.
//


#include <string.h>
#include <stdlib.h>

#include "v8engine.h"


v8::Handle<v8::ObjectTemplate> global;
v8::HandleScope handle_scope;
v8::Handle<v8::Context> context;

//local variables accessible inside script
char username[1024];
int x;

//get the value of x variable inside javascript
static v8::Handle<v8::Value> XGetter(v8::Local<v8::String> name, const v8::AccessorInfo& info) {
  return  v8::Number::New(x);
}

//set the value of x variable inside javascript
static void XSetter(v8::Local<v8::String> name,v8::Local<v8::Value> value,const v8::AccessorInfo& info) {
  x = value->Int32Value();
}

//get the value of username variable inside javascript
v8::Handle<v8::Value> userGetter(v8::Local<v8::String> name,const v8::AccessorInfo& info) {
	return v8::String::New((char*)&username,strlen((char*)&username));
}

//set the value of username variable inside javascript
void userSetter(v8::Local<v8::String> name, v8::Local<v8::Value> value,const v8::AccessorInfo& info) {
	v8::Local<v8::String> s = value->ToString();
	s->WriteAscii((char*)&username);
}

//convert unsigned int to value
static v8::Local<v8::Value> v8_uint32(unsigned int x) {
	return v8::Uint32::New(x);
}

// Add two numbers
v8::Handle<v8::Value> Plus(const v8::Arguments& args)
{
	unsigned int A = args[0]->Uint32Value();
	unsigned int B = args[1]->Uint32Value();
	return v8_uint32(A +  B);
}


// The callback that is invoked by v8 whenever the JavaScript 'print'
// function is called.  Prints its arguments on stdout separated by
// spaces and ending with a newline.
v8::Handle<v8::Value> Print(const v8::Arguments& args) {
	bool first = true;
	for (int i = 0; i < args.Length(); i++)
	{
		v8::HandleScope handle_scope;
		if (first)
		{
			first = false;
		}
		else
		{
			printf(" ");
		}
		//convert the args[i] type to normal char* string
		v8::String::AsciiValue str(args[i]);
		printf("%s", *str);
	}
	printf("\n");
	//returning Undefined is the same as returning void...
	return v8::Undefined();
}

// Executes a string within the current v8 context.
bool ExecuteString(v8::Handle<v8::String> source,v8::Handle<v8::String> name) {
	//access global context within this scope
	v8::Context::Scope context_scope(context);
	//exception handler
	v8::TryCatch try_catch;
	//compile script to binary code - JIT
	v8::Handle<v8::Script> script = v8::Script::Compile(source, name);
	bool print_result = true;

	//check if we got problems on compilation
	if (script.IsEmpty()) {

		// Print errors that happened during compilation.
		v8::String::AsciiValue error(try_catch.Exception());
		printf("%s\n", *error);
		return false;
	}
	else
	{
		//no errors , let's continue
		v8::Handle<v8::Value> result = script->Run();

		//check if execution ended with errors
		if(result.IsEmpty())
		{
			// Print errors that happened during execution.
			v8::String::AsciiValue error(try_catch.Exception());
			printf("%s\n", *error);
			return false;
		}
		else 
		{
			if (print_result && !result->IsUndefined())
			{
				// If all went well and the result wasn't undefined then print
				// the returned value.
				v8::String::AsciiValue str(result);
				printf("script result [%s]\n", *str);
			}
			return true;
		}
	}
}




int main(int argc, char* argv[])
{
	char *script = "print(\"begin script\");"
					"print(\"script executed by \"+user);"
					"if ( user == \"John Doe\"){"
					"print(\"\\tuser name is invalid. Changing name to Chuck Norris\");"
					"user = \"Chuck Norris\";}"
					"print(\"123 plus 27 = \" + plus(123,27));"
					"x = plus(3456789,6543211);"
					"print(\"end script\");";
	char *script_name = "internal_script";
	//convert the string with the script to a v8 string
	v8::Handle<v8::String> source =  v8::String::New(script, strlen(script));
	//each script name must be unique , for this demo I just run one embedded script, so the name can be fixed
	v8::Handle<v8::String> name = v8::String::New(script_name,strlen(script_name)); 

	// Create a template for the global object.
	global = v8::ObjectTemplate::New();

	//associates "plus" on script to the Plus function
	global->Set(v8::String::New("plus"), v8::FunctionTemplate::New(Plus));

	//associates "print" on script to the Print function
	global->Set(v8::String::New("print"), v8::FunctionTemplate::New(Print));

	//create accessor for string username
	global->SetAccessor(v8::String::New("user"),userGetter,userSetter);

	//create accessor for integer x
	global->SetAccessor(v8::String::New("x"), XGetter, XSetter);

	//create context for the script
	context = v8::Context::New(NULL, global);

	//fill username with something
	strcpy((char*)&username,"John Doe");

	//x initialization
	x=0;

	printf("################### BEFORE SCRIPT EXECUTION ###################\n");

	//simple javascritp to test
	ExecuteString(source,name);

	//check if my username changed...
	if(strcmp(username,"John Doe")!=0)
	{
		printf("################### AFTER SCRIPT EXECUTION ###################\n");
		printf("Script changed my username to %s\n",(char*)&username);
		printf("x value after script [%d]\n",x);
	}
    
    //v8Engine::match("7", "7");
    V8_ENGINE(ve);
    
    ve.invoke("matcher", "elem1", "[8, 7]");
    ve.invoke("matcher", "elem2", "[7, 7]");
    ve.invoke("matcher", "elem3", "[\"7\", \"7\"]");
   
	return 0;
}

// compile with g++ -std=c++11 v8engine.cpp main.cpp -lv8  -o v8test -lpthread

