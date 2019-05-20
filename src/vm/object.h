#ifndef OBJECT_H
#define OBJECT_H

#include "blang.h"
#include "chunk.h"
#include "hashtable.h"
#include "util.h"
#include "value.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * Object system of the Blang language.
 * Every object shares the base fields of the Obj struct, including it as the
 * first field in their declaration. This permits the casting of any pointer to
 * to Obj* and back, implementing a sort of manual polymorphism.
 *
 * In addition to objects' definitions, this files define macros for testing and
 * casting Obj* pointers.
 * Note that casting macros do not perform any checking, thus an Obj* pointer
 * should be tested before casting.
 **/

typedef struct BlangVM BlangVM;

#ifdef DBG_PRINT_GC
DECLARE_TO_STRING(ObjType);
#endif

// Macros for testing and casting objects
#define OBJ_TYPE(o) (AS_OBJ(o)->type)

#define IS_BOUND_METHOD(o) (IS_OBJ(o) && OBJ_TYPE(o) == OBJ_BOUND_METHOD)
#define IS_LIST(o) (IS_OBJ(o) && OBJ_TYPE(o) == OBJ_LIST)
#define IS_STRING(o) (IS_OBJ(o) && OBJ_TYPE(o) == OBJ_STRING)
#define IS_FUNC(o) (IS_OBJ(o) && OBJ_TYPE(o) == OBJ_FUNCTION)
#define IS_NATIVE(o) (IS_OBJ(o) && OBJ_TYPE(o) == OBJ_NATIVE)
#define IS_CLASS(o) (IS_OBJ(o) && OBJ_TYPE(o) == OBJ_CLASS)
#define IS_INSTANCE(o) (IS_OBJ(o) && OBJ_TYPE(o) == OBJ_INST)
#define IS_MODULE(o) (IS_OBJ(o) && OBJ_TYPE(o) == OBJ_MODULE)
#define IS_CLOSURE(o) (IS_OBJ(o) && OBJ_TYPE(o) == OBJ_CLOSURE)
#define IS_TUPLE(o) (IS_OBJ(o) && OBJ_TYPE(o) == OBJ_TUPLE)
#define IS_RANGE(o) (IS_OBJ(o) && OBJ_TYPE(o) == OBJ_RANGE)
#define IS_STACK_TRACE(o) (IS_OBJ(o) && OBJ_TYPE(o) == OBJ_STACK_TRACE)

#define AS_BOUND_METHOD(o) ((ObjBoundMethod *)AS_OBJ(o))
#define AS_LIST(o) ((ObjList *)AS_OBJ(o))
#define AS_STRING(o) ((ObjString *)AS_OBJ(o))
#define AS_FUNC(o) ((ObjFunction *)AS_OBJ(o))
#define AS_NATIVE(o) ((ObjNative *)AS_OBJ(o))
#define AS_CLASS(o) ((ObjClass *)AS_OBJ(o))
#define AS_INSTANCE(o) ((ObjInstance *)AS_OBJ(o))
#define AS_MODULE(o) ((ObjModule *)AS_OBJ(o))
#define AS_CLOSURE(o) ((ObjClosure *)AS_OBJ(o))
#define AS_TUPLE(o) ((ObjTuple *)AS_OBJ(o))
#define AS_RANGE(o) ((ObjRange *)AS_OBJ(o))
#define AS_STACK_TRACE(o) ((ObjStackTrace *)AS_OBJ(o))

#define STRING_GET_HASH(s) (s->hash == 0 ? s->hash = hashString(s->data, s->length) : s->hash)

// These types are used internally by the object system and are
// Never exposed to the user, to whom all values behave like
// class instances
#define OBJTYPE(X)      \
    X(OBJ_STRING)       \
    X(OBJ_NATIVE)       \
    X(OBJ_FUNCTION)     \
    X(OBJ_CLASS)        \
    X(OBJ_INST)         \
    X(OBJ_MODULE)       \
    X(OBJ_LIST)         \
    X(OBJ_BOUND_METHOD) \
    X(OBJ_STACK_TRACE)  \
    X(OBJ_CLOSURE)      \
    X(OBJ_UPVALUE)      \
    X(OBJ_TUPLE)

DEFINE_ENUM(ObjType, OBJTYPE);

typedef struct ObjClass ObjClass;

// Base class of all the Objects.
// Defines shared properties of all objects, such as the type and the class
// field, as well as fields used for garbage collection, such as the reached
// flag (used to test when an object is reachable, and thus not collectable)
// and the next pointer, that points to the next object in the global linked
// list of all allocated objects (set up by the allocator in memory.c).
typedef struct Obj {
    ObjType type;         // The type of the object
    bool reached;         // Flag used to signal that an object is reachable during a GC
    struct ObjClass *cls; // The class of the Object
    struct Obj *next;     // Next object in the linked list of all allocated objects
} Obj;

// A Blang String. In Blang Strings are immutable and can contain arbitrary
// bytes since we explicitly store the string's length instead of relying on
// NUL termination. Nevertheless, a NUL byte is appended for ease of use in
// the C api.
typedef struct ObjString {
    Obj base;
    size_t length; // Length of the string
    uint32_t hash; // The string's hash (gets calculated once at allocation)
    bool interned; // Whether the string is interned or not
    char *data;    // The actual data of the string (NUL terminated)
} ObjString;

typedef struct ObjModule {
    Obj base;
    ObjString *name;   // Name of the module
    HashTable globals; // HashTable containing the global variables of the module
} ObjModule;

// Fields shared by all callable objects (functions and natives)
typedef struct Callable {
    bool vararg;       // Whether the function is a vararg one
    uint8_t argsCount; // The arity of the function
    uint8_t defaultc;  // Number of default args of the function (0 if none)
    Value *defaults;   // Array of default arguments (NULL if no defaults)
    ObjModule *module; // The module of the function
    ObjString *name;   // The name of the function
} Callable;

// A compiled Blang function
typedef struct ObjFunction {
    Obj base;
    Callable c;
    Chunk chunk;      // The actual code chunk containing bytecodes
    uint8_t upvaluec; // The number of upvalues the function closes over
} ObjFunction;

// A C function callable from Blang
typedef struct ObjNative {
    Obj base;
    Callable c;
    Native fn; // The C function that gets called
} ObjNative;

// A user defined class
typedef struct ObjClass {
    Obj base;
    ObjString *name;           // The name of the class
    struct ObjClass *superCls; // Pointer to the parent class (or NULL)
    HashTable methods;         // HashTable containing methods (ObjFunction/ObjNative)
} ObjClass;

// An instance of a user defined Class
typedef struct ObjInstance {
    Obj base;
    HashTable fields; // HashTable containing the fields of the instance
} ObjInstance;

typedef struct ObjList {
    Obj base;
    size_t size;  // Size of the List (how much space is currently allocated)
    size_t count; // How many objects are currently in the list
    Value *arr;   // List elements
} ObjList;

typedef struct ObjTuple {
    Obj base;
    size_t size; // Number of elements of the tuple
    Value arr[]; // Tuple elements (flexible array)
} ObjTuple;

// A bound method. It contains a method with an associated target.
typedef struct ObjBoundMethod {
    Obj base;
    Value bound; // The value to which the method is bound
    Obj *method; // The actual method
} ObjBoundMethod;

// An upvalue is a variable captured from an outer scope by a closure.
// when a closure of a function is created, it instantiates an ObjUpvalue
// for all  variables used in the function but declared in an outer one,
// and stores the stack address of such variable in the addr field.
// When a variable that is also an upvalue gets out of scope, its Value
// is copied in the closed field, and the addr field is set to &closed.
// This way the variable can continue to be used even if the stack frame
// that originally stored it has benn popped.
typedef struct ObjUpvalue {
    Obj base;
    Value *addr;             // The address of the upvalue
    Value closed;            // Stores the upvalue when closed
    struct ObjUpvalue *next; // Pointer to the next open upvalue. NULL when closed
} ObjUpvalue;

// A closure always wraps an ObjFunction and stores the flattened hierarchy of
// Upvalues that the function closes over.
typedef struct ObjClosure {
    Obj base;
    ObjFunction *fn;        // The function
    uint8_t upvalueCount;   // The number of Upvalues the function closes over
    ObjUpvalue *upvalues[]; // the actual Upvalues
} ObjClosure;

// Object that contains the dump of the stack's frames.
// Used for storing the trace of an unhandled exception
typedef struct ObjStackTrace {
    Obj base;
    int lastTracedFrame;
    BlBuffer stacktrace;
} ObjStackTrace;

// Prints an Obj in a human readable form
void printObj(Obj *o);

#endif
