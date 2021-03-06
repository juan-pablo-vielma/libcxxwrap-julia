﻿#include <type_traits>
#include <string>
#include <memory>
#include <iostream>

#include "jlcxx/jlcxx.hpp"
#include "jlcxx/functions.hpp"

namespace cpp_types
{

// Custom minimal smart pointer type
template<typename T>
struct MySmartPointer
{
  MySmartPointer(T* ptr) : m_ptr(ptr)
  {
  }

  MySmartPointer(std::shared_ptr<T> ptr) : m_ptr(ptr.get())
  {
  }

  T& operator*() const
  {
    return *m_ptr;
  }

  T* m_ptr;
};

struct DoubleData
{
  double a[4];
};

struct World
{
  World(const std::string& message = "default hello") : msg(message){}
  World(int_t) : msg("NumberedWorld") {}
  void set(const std::string& msg) { this->msg = msg; }
  const std::string& greet() const { return msg; }
  std::string msg;
  ~World() { std::cout << "Destroying World with message " << msg << std::endl; }
};

struct Array {};

struct NonCopyable
{
  NonCopyable() {}
  NonCopyable& operator=(const NonCopyable&) = delete;
  NonCopyable(const NonCopyable&) = delete;
};

struct AConstRef
{
  int value() const
  {
    return 42;
  }
};

struct ReturnConstRef
{
  const AConstRef& operator()()
  {
    return m_val;
  }

  AConstRef m_val;
};

struct CallOperator
{
  int operator()() const
  {
    return 43;
  }
};

struct ConstPtrConstruct
{
  ConstPtrConstruct(const World* w) : m_w(w)
  {
  }

  const std::string& greet() { return m_w->greet(); }

  const World* m_w;
};

// Call a function on a type that is defined in Julia
struct JuliaTestType {
  double a;
  double b;
};
void call_testype_function()
{
  JuliaTestType A = {2., 3.};
  jl_value_t* result = jl_new_struct_uninit((jl_datatype_t*)jlcxx::julia_type("JuliaTestType"));
  *reinterpret_cast<JuliaTestType*>(result) = A;
  jlcxx::JuliaFunction("julia_test_func")(result);
}

enum MyEnum
{
  EnumValA,
  EnumValB
};

struct Foo
{
  Foo(const std::wstring& n, jlcxx::ArrayRef<double,1> d) : name(n), data(d.begin(), d.end())
  {
  }

  std::wstring name;
  std::vector<double> data;
};

struct NullableStruct {};

struct ImmutableBits
{
  double a;
  double b;
};

} // namespace cpp_types

namespace jlcxx
{
  template<> struct IsBits<cpp_types::MyEnum> : std::true_type {};
  template<typename T> struct IsSmartPointerType<cpp_types::MySmartPointer<T>> : std::true_type { };
  template<typename T> struct ConstructorPointerType<cpp_types::MySmartPointer<T>> { typedef std::shared_ptr<T> type; };

  template<> struct IsImmutable<cpp_types::ImmutableBits> : std::true_type {};
  template<> struct IsBits<cpp_types::ImmutableBits> : std::true_type {};
}

JLCXX_MODULE define_julia_module(jlcxx::Module& types)
{
  using namespace cpp_types;

  types.method("call_testype_function", call_testype_function);

  types.add_type<DoubleData>("DoubleData");

  types.add_type<World>("World")
    .constructor<const std::string&>()
    .constructor<int_t>(false) // no finalizer
    .method("set", &World::set)
    .method("greet", &World::greet)
    .method("greet_lambda", [] (const World& w) { return w.greet(); } );

  types.add_type<Array>("Array");

  types.method("world_factory", []()
  {
    return new World("factory hello");
  });

  types.method("shared_world_factory", []() -> const std::shared_ptr<World>
  {
    return std::shared_ptr<World>(new World("shared factory hello"));
  });
  // Shared ptr overload for greet
  types.method("greet_shared", [](const std::shared_ptr<World>& w)
  {
    return w->greet();
  });
  types.method("greet_shared_const", [](const std::shared_ptr<const World>& w)
  {
    return w->greet();
  });

  types.method("shared_world_ref", []() -> std::shared_ptr<World>&
  {
    static std::shared_ptr<World> refworld(new World("shared factory hello ref"));
    return refworld;
  });

  types.method("reset_shared_world!", [](std::shared_ptr<World>& target, std::string message)
  {
    target.reset(new World(message));
  });

  types.method("smart_world_factory", []()
  {
    return MySmartPointer<World>(new World("smart factory hello"));
  });
  // smart ptr overload for greet
  types.method("greet_smart", [](const MySmartPointer<World>& w)
  {
    return (*w).greet();
  });

  // weak ptr overload for greet
  types.method("greet_weak", [](const std::weak_ptr<World>& w)
  {
    return w.lock()->greet();
  });

  types.method("unique_world_factory", []()
  {
    return std::unique_ptr<const World>(new World("unique factory hello"));
  });

  types.method("world_by_value", [] () -> World
  {
    return World("world by value hello");
  });

  types.method("boxed_world_factory", []()
  {
    static World w("boxed world");
    return jlcxx::box(w);
  });

  types.method("boxed_world_pointer_factory", []()
  {
    static World w("boxed world pointer");
    return jlcxx::box(&w);
  });

  types.method("world_ref_factory", []() -> World&
  {
    static World w("reffed world");
    return w;
  });

  types.add_type<NonCopyable>("NonCopyable");

  types.add_type<AConstRef>("AConstRef").method("value", &AConstRef::value);
  types.add_type<ReturnConstRef>("ReturnConstRef").method("value", &ReturnConstRef::operator());

  types.add_type<CallOperator>("CallOperator").method(&CallOperator::operator())
    .method([] (const CallOperator&, int i)  { return i; } );

  types.add_type<ConstPtrConstruct>("ConstPtrConstruct")
    .constructor<const World*>()
    .method("greet", &ConstPtrConstruct::greet);

  // Enum
  types.add_bits<MyEnum>("MyEnum", jlcxx::julia_type("CppEnum"));
  types.set_const("EnumValA", EnumValA);
  types.set_const("EnumValB", EnumValB);
  types.method("enum_to_int", [] (const MyEnum e) { return static_cast<int>(e); });
  types.method("get_enum_b", [] () { return EnumValB; });

  types.add_type<Foo>("Foo")
    .constructor<const std::wstring&, jlcxx::ArrayRef<double,1>>()
    .method("name", [](Foo& f) { return f.name; })
    .method("data", [](Foo& f) { return jlcxx::ArrayRef<double,1>(&(f.data[0]), f.data.size()); });

  types.method("print_foo_array", [] (jlcxx::ArrayRef<jl_value_t*> farr)
  {
    for(jl_value_t* v : farr)
    {
      const Foo& f = *jlcxx::unbox_wrapped_ptr<Foo>(v);
      std::wcout << f.name << ":";
      for(const double d : f.data)
      {
        std::wcout << " " << d;
      }
      std::wcout << std::endl;
    }
  });

  types.add_type<NullableStruct>("NullableStruct");
  types.method("return_ptr", [] () { return new NullableStruct; });
  types.method("return_null", [] () { return static_cast<NullableStruct*>(nullptr); });

  jlcxx::static_type_mapping<ImmutableBits>::set_julia_type((jl_datatype_t*)jlcxx::julia_type("ImmutableBits"));
  types.method("increment_immutable", [] (const ImmutableBits& x) { return ImmutableBits({x.a+1.0, x.b+1.0}); });
}
