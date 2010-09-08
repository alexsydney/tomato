#include "tomato.h"

RB_DEFINE_ALLOC_FUNC(TomatoContext);

static VALUE fContext_run(int argc, VALUE *argv, VALUE self);
static VALUE fContext_version(VALUE self);
static void fContext_mark(TomatoContext *v8);
static void fContext_free(TomatoContext *v8);
static v8::Handle<v8::Value> debug(const Arguments &args);

VALUE cTomatoContext;

TomatoContext::TomatoContext()
{
  trace("Allocate Tomato::Context.new %x", this);
  INC_TRACE_DEPTH;
    this->rb_instance = Data_Wrap_Struct(cTomatoContext, fContext_mark, fContext_free, this);
    HandleScope handle_scope;
    Handle<ObjectTemplate> global = ObjectTemplate::New();
    global->Set(String::New("debug"), FunctionTemplate::New(debug));
    this->js_context = Context::New(NULL, global);
    trace("- finished allocating Tomato::Context");
  DEC_TRACE_DEPTH;
}

TomatoContext::~TomatoContext()
{
  trace("destroy TomatoContext %x", this);
  INC_TRACE_DEPTH;
    this->js_context.Dispose();
  DEC_TRACE_DEPTH;
}

Handle<Object> TomatoContext::global()
{
  trace("TomatoContext::global()");
  return this->js_context->Global();
}

VALUE TomatoContext::execute(const char *javascript, const char *filename)
{
  trace("Executing JavaScript code '%s':\n%s", filename, javascript);
  Context::Scope context_scope(this->context());
  HandleScope handle_scope;
  Handle<String> source_code = String::New(javascript);
  Handle<String> name = String::New(filename);
  return compile_and_run(source_code, name);
}

VALUE TomatoContext::compile_and_run(Handle<String> source, Handle<Value> name)
{
  HandleScope handle_scope;
  TryCatch try_catch;
  Handle<Script> script = Script::Compile(source, name);
  if (script.IsEmpty())
  {
    throw_error(&try_catch);
    return Qnil;
  }
  else
  {
    Handle<Value> result = script->Run();
    if (result.IsEmpty())
    {
      throw_error(&try_catch);
      return Qnil;
    }
    else
    {
      JavascriptValue value(result);
      try { return value.toRuby(); }
      catch(const std::string &what)
      {
        if (what == "json")
        {
          Handle<Value> json = this->js_context->Global()->Get(String::New("JSON"));
          String::Utf8Value str(value.toJSON(json));
          return rb_str_new2(ToCString(str));
        }
        else
        {
          throw what;
        }
      }
      //return ruby_value_of(this, result);
    }
  }
}


void Init_v8(void)
{
  cTomatoContext = rb_define_class_under(cTomato, "Context", rb_cObject);

  rb_define_method(cTomatoContext, "run", (VALUE(*)(...))&fContext_run, -1);
  rb_define_module_function(cTomatoContext, "version", (VALUE(*)(...))&fContext_version, 0);
  rb_define_alloc_func(cTomatoContext, (VALUE(*)(VALUE))&RB_ALLOC_FUNC(TomatoContext));
}

static void fContext_mark(TomatoContext *v8) {}

static void fContext_free(TomatoContext *v8)
{
  delete v8;
}

static VALUE fContext_run(int argc, VALUE *argv, VALUE self)
{
  RB_EXTRACT_STRUCT(self, TomatoContext, v8);

  if (argc == 0)
  {
    rb_raise(rb_eArgError, "expected at least 1 argument: the JavaScript to be executed");
    return Qnil;
  }
  else if (argc > 2)
  {
    rb_raise(rb_eArgError, "expected at most 2 arguments: the JavaScript to be executed, and an optional file name");
    return Qnil;
  }

  char *javascript = StringValuePtr(argv[0]);
  char *filename;
  if (argc == 2)
    filename = StringValuePtr(argv[1]);
  else
    filename = (char *)"(dynamic)";

  WRAP_CPP(
    return v8->execute(javascript, filename);
  )
}

static VALUE fContext_version(VALUE self)
{
  return rb_str_new2(V8::GetVersion());
}

static v8::Handle<v8::Value> debug(const Arguments &args)
{
  Handle<Value> arg;
  Handle<Value> json = args.Holder()->Get(String::New("JSON"));
  
  int len = args.Length();
  for (int i = 0; i < len; i++)
  {
    arg = args[i];
    if (i > 0) printf(", ");
    JavascriptValue jsv(args[i]);
    String::Utf8Value str(jsv.toJSON(json));
    printf("<%s>", ToCString(str));
  }
  printf("\n");
  return Null();
}