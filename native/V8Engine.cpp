#include <jni.h>
#include "com_pwootage_oc_js_v8_V8Engine.h"
#include "com_pwootage_oc_js_v8_V8Static.h"
#include "V8EngineNative.h"

using namespace v8;

JNIEXPORT void JNICALL
Java_com_pwootage_oc_js_v8_V8Static_native_1init(JNIEnv *env, jclass clazz) {
    V8EngineNative::Initialize(env, clazz);
}

JNIEXPORT void JNICALL
Java_com_pwootage_oc_js_v8_V8Engine_native_1start(JNIEnv *env, jobject self) {
    printf("Native start\n");
    fflush(stdout);

    // Create a new Isolate and make it the current one.
    Isolate::CreateParams create_params;
    create_params.array_buffer_allocator =
            v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    Isolate* isolate = Isolate::New(create_params);
    {
        Isolate::Scope isolate_scope(isolate);
        // Create a stack-allocated handle scope.
        HandleScope handle_scope(isolate);
        // Create a new context.
        Local<Context> context = Context::New(isolate);
        // Enter the context for compiling and running the hello world script.
        Context::Scope context_scope(context);
        // Create a string containing the JavaScript source code.
        Local<String> source =
                String::NewFromUtf8(isolate, "'JS ' + ', RAN!'",
                                    NewStringType::kNormal).ToLocalChecked();
        // Compile the source code.
        Local<Script> script = Script::Compile(context, source).ToLocalChecked();
        // Run the script to get the result.
        Local<Value> result = script->Run(context).ToLocalChecked();
        // Convert the result to an UTF8 string and print it.
        String::Utf8Value utf8(result);
        printf("%s\n", *utf8);
        fflush(stdout);
    }
    isolate->Dispose();
    delete create_params.array_buffer_allocator;
}


JNIEXPORT void JNICALL
Java_com_pwootage_oc_js_v8_V8Engine_native_1destroy(JNIEnv *env, jobject self) {
    printf("Native destroy\n");
    fflush(stdout);
}
