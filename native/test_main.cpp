#include "include/v8.h"
#include "include/libplatform/libplatform.h"
#include <thread>

using namespace v8;
using namespace std;

void runSomeJS();

int main(int argc, char* argv[]) {
    V8::InitializeICUDefaultLocation("/home/pwootage/projects/oc-js/src/main/resources/assets/oc-js/v8/");
    V8::InitializeExternalStartupData("/home/pwootage/projects/oc-js/src/main/resources/assets/oc-js/v8/");
    Platform* platform = platform::CreateDefaultPlatform();
    V8::InitializePlatform(platform);
    V8::Initialize();
    int num_threads = 1000;
    std::thread t[num_threads];
    for (int i = 0; i < num_threads; i++) {
        t[i] = thread(runSomeJS);
    }
    for (int i = 0; i < num_threads; i++) {
        t[i].join();
    }

    std::this_thread::sleep_for(std::chrono::seconds(10));
    V8::Dispose();
    V8::ShutdownPlatform();
    delete platform;

    return 0;
}

void runSomeJS() {
    Isolate::CreateParams create_params;
    create_params.array_buffer_allocator =
            v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    Isolate* isolate = Isolate::New(create_params);
    {
        isolate->Enter();
        Isolate::Scope isolate_scope(isolate);
        // Create a stack-allocated handle scope.
        HandleScope handle_scope(isolate);
        // Create a new context.
        Local<Context> context = Context::New(isolate);
        // Enter the context for compiling and running the hello world script.
        Context::Scope context_scope(context);
        // Create a string containing the JavaScript source code.
        Local<String> source =
                String::NewFromUtf8(isolate, "function fib(i){"
                                            "return i <= 1 ? 1 : fib(i -1) + fib(i-2);"
                                            "} "
                                            "[1,2,3,4,5,12,36].map(fib).join(',');",
                                    NewStringType::kNormal).ToLocalChecked();
        // Compile the source code.
        Local<Script> script = Script::Compile(context, source).ToLocalChecked();
        // Run the script to get the result.
        Local<Value> result = script->Run(context).ToLocalChecked();
        // Convert the result to an UTF8 string and print it.
        String::Utf8Value utf8(result);
        printf("%s\n", *utf8);
        isolate->Exit();
    }
    // Dispose the isolate and tear down V8.
    isolate->Dispose();
    delete create_params.array_buffer_allocator;
}

