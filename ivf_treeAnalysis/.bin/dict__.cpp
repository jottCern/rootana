// Generated at Tue Apr  1 18:56:37 2014. Do not modify it

#ifdef _WIN32
#pragma warning ( disable : 4786 )
#pragma warning ( disable : 4345 )
#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)) && !defined(__INTEL_COMPILER) 
# pragma GCC diagnostic ignored "-Warray-bounds"
#endif
#include "src/classes.h"
#ifdef CONST
# undef CONST
#endif
#include "Reflex/Builder/ReflexBuilder.h"
#include <typeinfo>

namespace {
  ::Reflex::NamespaceBuilder nsb0( Reflex::Literal("std") );
  ::Reflex::NamespaceBuilder nsb1( Reflex::Literal("ROOT::Math") );
  ::Reflex::Type type_39 = ::Reflex::TypeBuilder(Reflex::Literal("int"));
  ::Reflex::Type type_1663 = ::Reflex::TypeBuilder(Reflex::Literal("TV"));
  ::Reflex::Type type_111 = ::Reflex::TypeBuilder(Reflex::Literal("long"));
  ::Reflex::Type type_113 = ::Reflex::TypeBuilder(Reflex::Literal("void"));
  ::Reflex::Type type_void = ::Reflex::TypeBuilder(Reflex::Literal("void"));
  ::Reflex::Type type_636 = ::Reflex::TypeBuilder(Reflex::Literal("float"));
  ::Reflex::Type type_69 = ::Reflex::TypeBuilder(Reflex::Literal("double"));
  ::Reflex::Type type_65 = ::Reflex::TypeBuilder(Reflex::Literal("unsigned long"));
  ::Reflex::Type type_663 = ::Reflex::TypeBuilder(Reflex::Literal("Basic3DVector<float>"));
  ::Reflex::Type type_1785 = ::Reflex::TypeBuilder(Reflex::Literal("std::vector<double>"));
  ::Reflex::Type type_2079 = ::Reflex::TypeBuilder(Reflex::Literal("std::allocator<double>"));
  ::Reflex::Type type_6586 = ::Reflex::TypeBuilder(Reflex::Literal("ROOT::Math::RowOffsets<3>"));
  ::Reflex::Type type_888 = ::Reflex::TypeBuilder(Reflex::Literal("Vector3DBase<float,GlobalTag>"));
  ::Reflex::Type type_2772 = ::Reflex::TypeBuilder(Reflex::Literal("ROOT::Math::PxPyPzE4D<double>"));
  ::Reflex::Type type_2595 = ::Reflex::TypeBuilder(Reflex::Literal("ROOT::Math::MatRepSym<double,3>"));
  ::Reflex::Type type_2987 = ::Reflex::TypeBuilder(Reflex::Literal("ROOT::Math::Cartesian3D<double>"));
  ::Reflex::Type type_2912 = ::Reflex::TypeBuilder(Reflex::Literal("Geom::Spherical2Cartesian<float>"));
  ::Reflex::Type type_2910 = ::Reflex::TypeBuilder(Reflex::Literal("Geom::Cylindrical2Cartesian<float>"));
  ::Reflex::Type type_1707 = ::Reflex::TypeBuilder(Reflex::Literal("PV3DBase<float,VectorTag,GlobalTag>"));
  ::Reflex::Type type_2989 = ::Reflex::TypeBuilder(Reflex::Literal("ROOT::Math::DefaultCoordinateSystemTag"));
  ::Reflex::Type type_2170 = ::Reflex::TypeBuilder(Reflex::Literal("std::_Vector_base<double,std::allocator<double> >"));
  ::Reflex::Type type_2696 = ::Reflex::TypeBuilder(Reflex::Literal("__gnu_cxx::__alloc_traits<std::allocator<double> >"));
  ::Reflex::Type type_529 = ::Reflex::TypeBuilder(Reflex::Literal("ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> >"));
  ::Reflex::Type type_2709 = ::Reflex::TypeBuilder(Reflex::Literal("__gnu_cxx::__normal_iterator<double*,std::vector<double> >"));
  ::Reflex::Type type_394 = ::Reflex::TypeBuilder(Reflex::Literal("ROOT::Math::SMatrix<double,3,3,ROOT::Math::MatRepSym<double,3> >"));
  ::Reflex::Type type_2710 = ::Reflex::TypeBuilder(Reflex::Literal("__gnu_cxx::__normal_iterator<const double*,std::vector<double> >"));
  ::Reflex::Type type_2162 = ::Reflex::TypeBuilder(Reflex::Literal("std::reverse_iterator<__gnu_cxx::__normal_iterator<double*,std::vector<double> > >"));
  ::Reflex::Type type_2161 = ::Reflex::TypeBuilder(Reflex::Literal("std::reverse_iterator<__gnu_cxx::__normal_iterator<const double*,std::vector<double> > >"));
  ::Reflex::Type type_799 = ::Reflex::TypeBuilder(Reflex::Literal("ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>,ROOT::Math::DefaultCoordinateSystemTag>"));
  ::Reflex::Type type_2774 = ::Reflex::TypeBuilder(Reflex::Literal("ROOT::Math::DisplacementVector3D<ROOT::Math::Cartesian3D<double>,ROOT::Math::DefaultCoordinateSystemTag>"));
  ::Reflex::Type type_395 = ::Reflex::TypedefTypeBuilder(Reflex::Literal("CovarianceMatrix"), type_394);
  ::Reflex::Type type_530 = ::Reflex::TypedefTypeBuilder(Reflex::Literal("LorentzVector"), type_529);
  ::Reflex::Type type_1657 = ::Reflex::TypedefTypeBuilder(Reflex::Literal("Global3DVector"), type_888);
  ::Reflex::Type type_889 = ::Reflex::TypedefTypeBuilder(Reflex::Literal("GlobalVector"), type_1657);
  ::Reflex::Type type_663c = ::Reflex::ConstBuilder(type_663);
  ::Reflex::Type type_6400 = ::Reflex::ReferenceBuilder(type_663c);
  ::Reflex::Type type_800 = ::Reflex::TypedefTypeBuilder(Reflex::Literal("Point"), type_799);
  ::Reflex::Type type_1663c = ::Reflex::ConstBuilder(type_1663);
  ::Reflex::Type type_6730 = ::Reflex::ReferenceBuilder(type_1663c);
  ::Reflex::Type type_1707c = ::Reflex::ConstBuilder(type_1707);
  ::Reflex::Type type_6732 = ::Reflex::ReferenceBuilder(type_1707c);
  ::Reflex::Type type_2558 = ::Reflex::PointerBuilder(type_69);
  ::Reflex::Type type_69c = ::Reflex::ConstBuilder(type_69);
  ::Reflex::Type type_2598 = ::Reflex::PointerBuilder(type_69c);
  ::Reflex::Type type_3313 = ::Reflex::ReferenceBuilder(type_69);
  ::Reflex::Type type_3315 = ::Reflex::ReferenceBuilder(type_69c);
  ::Reflex::Type type_2155 = ::Reflex::TypedefTypeBuilder(Reflex::Literal("std::size_t"), type_65);
  ::Reflex::Type type_2098 = ::Reflex::TypedefTypeBuilder(Reflex::Literal("std::ptrdiff_t"), type_111);
  ::Reflex::Type type_1785c = ::Reflex::ConstBuilder(type_1785);
  ::Reflex::Type type_6734 = ::Reflex::ReferenceBuilder(type_1785c);
  ::Reflex::Type type_7259 = ::Reflex::ArrayBuilder(type_69, 6);
  ::Reflex::Type type_6586c = ::Reflex::ConstBuilder(type_6586);
  ::Reflex::Type type_7260 = ::Reflex::PointerBuilder(type_6586c);
  ::Reflex::Type type_2595c = ::Reflex::ConstBuilder(type_2595);
  ::Reflex::Type type_7261 = ::Reflex::ReferenceBuilder(type_2595c);
  ::Reflex::Type type_7263 = ::Reflex::ReferenceBuilder(type_6586c);
  ::Reflex::Type type_394c = ::Reflex::ConstBuilder(type_394);
  ::Reflex::Type type_5966 = ::Reflex::ReferenceBuilder(type_394c);
  ::Reflex::Type type_529c = ::Reflex::ConstBuilder(type_529);
  ::Reflex::Type type_6347 = ::Reflex::ReferenceBuilder(type_529c);
  ::Reflex::Type type_888c = ::Reflex::ConstBuilder(type_888);
  ::Reflex::Type type_6473 = ::Reflex::ReferenceBuilder(type_888c);
  ::Reflex::Type type_799c = ::Reflex::ConstBuilder(type_799);
  ::Reflex::Type type_6453 = ::Reflex::ReferenceBuilder(type_799c);
  ::Reflex::Type type_799f = ::Reflex::TypedefTypeBuilder(Reflex::Literal("ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<Double32_t>,ROOT::Math::DefaultCoordinateSystemTag>"), type_799);
} // unnamed namespace

#ifndef __CINT__

// Shadow classes to obtain the data member offsets 
namespace __shadow__ {
#ifdef __Basic3DVector_float_
#undef __Basic3DVector_float_
#endif
class __Basic3DVector_float_ {
  public:
  __Basic3DVector_float_();
  float theX;
  float theY;
  float theZ;
  float theW;
};
#ifdef __TV
#undef __TV
#endif
struct __TV {
  public:
  __TV();
  ::Point tvPos;
  int pdgIdMother;
  ::LorentzVector p4daughters;
  int numberChargedDaughters;
};
#ifdef __PV3DBase_float_VectorTag_GlobalTag_
#undef __PV3DBase_float_VectorTag_GlobalTag_
#endif
class __PV3DBase_float_VectorTag_GlobalTag_ {
  public:
  __PV3DBase_float_VectorTag_GlobalTag_();
  ::Basic3DVector<float> theVector;
};
#ifdef __std__vector_double_
#undef __std__vector_double_
#endif
class __std__vector_double_ : protected ::std::_Vector_base<double,std::allocator<double> > {
  public:
  __std__vector_double_();
};
#ifdef __ROOT__Math__MatRepSym_double_3_
#undef __ROOT__Math__MatRepSym_double_3_
#endif
class __ROOT__Math__MatRepSym_double_3_ {
  public:
  __ROOT__Math__MatRepSym_double_3_();
  double fArray[6];
  void* fOff;
};
#ifdef __ROOT__Math__RowOffsets_3_
#undef __ROOT__Math__RowOffsets_3_
#endif
struct __ROOT__Math__RowOffsets_3_ {
  public:
  __ROOT__Math__RowOffsets_3_();
};
#ifdef __ROOT__Math__SMatrix_double_3_3_ROOT__Math__MatRepSym_double_3_s_
#undef __ROOT__Math__SMatrix_double_3_3_ROOT__Math__MatRepSym_double_3_s_
#endif
class __ROOT__Math__SMatrix_double_3_3_ROOT__Math__MatRepSym_double_3_s_ {
  public:
  __ROOT__Math__SMatrix_double_3_3_ROOT__Math__MatRepSym_double_3_s_();
  ::ROOT::Math::MatRepSym<double,3> fRep;
};
#ifdef __ROOT__Math__LorentzVector_ROOT__Math__PxPyPzE4D_double_s_
#undef __ROOT__Math__LorentzVector_ROOT__Math__PxPyPzE4D_double_s_
#endif
class __ROOT__Math__LorentzVector_ROOT__Math__PxPyPzE4D_double_s_ {
  public:
  __ROOT__Math__LorentzVector_ROOT__Math__PxPyPzE4D_double_s_();
  ::ROOT::Math::PxPyPzE4D<double> fCoordinates;
};
#ifdef __Vector3DBase_float_GlobalTag_
#undef __Vector3DBase_float_GlobalTag_
#endif
class __Vector3DBase_float_GlobalTag_ : public ::PV3DBase<float,VectorTag,GlobalTag> {
  public:
  __Vector3DBase_float_GlobalTag_();
};
#ifdef __ROOT__Math__PositionVector3D_ROOT__Math__Cartesian3D_double__ROOT__Math__DefaultCoordinateSystemTag_
#undef __ROOT__Math__PositionVector3D_ROOT__Math__Cartesian3D_double__ROOT__Math__DefaultCoordinateSystemTag_
#endif
class __ROOT__Math__PositionVector3D_ROOT__Math__Cartesian3D_double__ROOT__Math__DefaultCoordinateSystemTag_ {
  public:
  __ROOT__Math__PositionVector3D_ROOT__Math__Cartesian3D_double__ROOT__Math__DefaultCoordinateSystemTag_();
  ::ROOT::Math::Cartesian3D<double> fCoordinates;
};
}


#endif // __CINT__
namespace {
//------Stub functions for class Basic3DVector<float> -------------------------------
static void destructor_2915(void*, void * o, const std::vector<void*>&, void *) {
(((::Basic3DVector<float>*)o)->::Basic3DVector<float>::~Basic3DVector)();
}
static void constructor_2917( void* retaddr, void* mem, const std::vector<void*>&, void*) {
  if (retaddr) *(void**)retaddr = ::new(mem) ::Basic3DVector<float>();
  else ::new(mem) ::Basic3DVector<float>();
}

static void constructor_2918( void* retaddr, void* mem, const std::vector<void*>& arg, void*) {
  if (retaddr) *(void**)retaddr = ::new(mem) ::Basic3DVector<float>(*(const ::Basic3DVector<float>*)arg[0]);
  else ::new(mem) ::Basic3DVector<float>(*(const ::Basic3DVector<float>*)arg[0]);
}

static void method_newdel_663( void* retaddr, void*, const std::vector<void*>&, void*)
{
  static ::Reflex::NewDelFunctions s_funcs;
  s_funcs.fNew         = ::Reflex::NewDelFunctionsT< ::Basic3DVector<float> >::new_T;
  s_funcs.fNewArray    = ::Reflex::NewDelFunctionsT< ::Basic3DVector<float> >::newArray_T;
  s_funcs.fDelete      = ::Reflex::NewDelFunctionsT< ::Basic3DVector<float> >::delete_T;
  s_funcs.fDeleteArray = ::Reflex::NewDelFunctionsT< ::Basic3DVector<float> >::deleteArray_T;
  s_funcs.fDestructor  = ::Reflex::NewDelFunctionsT< ::Basic3DVector<float> >::destruct_T;
  if (retaddr) *(::Reflex::NewDelFunctions**)retaddr = &s_funcs;
}

//------Dictionary for class Basic3DVector<float> -------------------------------
void __Basic3DVector_float__db_datamem(Reflex::Class*);
void __Basic3DVector_float__db_funcmem(Reflex::Class*);
Reflex::GenreflexMemberBuilder __Basic3DVector_float__datamem_bld(&__Basic3DVector_float__db_datamem);
Reflex::GenreflexMemberBuilder __Basic3DVector_float__funcmem_bld(&__Basic3DVector_float__db_funcmem);
void __Basic3DVector_float__dict() {
  ::Reflex::ClassBuilder(Reflex::Literal("Basic3DVector<float>"), typeid(::Basic3DVector<float>), sizeof(::Basic3DVector<float>), ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL, ::Reflex::CLASS)
  .AddTypedef(type_636, Reflex::Literal("Basic3DVector<float>::ScalarType"))
  .AddTypedef(type_2910, Reflex::Literal("Basic3DVector<float>::Cylindrical"))
  .AddTypedef(type_2912, Reflex::Literal("Basic3DVector<float>::Spherical"))
  .AddTypedef(type_2912, Reflex::Literal("Basic3DVector<float>::Polar"))
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void), Reflex::Literal("~Basic3DVector"), destructor_2915, 0, 0, ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL | ::Reflex::DESTRUCTOR )
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void), Reflex::Literal("Basic3DVector"), constructor_2917, 0, 0, ::Reflex::PUBLIC | ::Reflex::EXPLICIT | ::Reflex::CONSTRUCTOR)
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void, type_6400), Reflex::Literal("Basic3DVector"), constructor_2918, 0, "p", ::Reflex::PUBLIC | ::Reflex::CONSTRUCTOR)
  .AddFunctionMember<void*(void)>(Reflex::Literal("__getNewDelFunctions"), method_newdel_663, 0, 0, ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL)
  .AddOnDemandDataMemberBuilder(&__Basic3DVector_float__datamem_bld);
}

//------Delayed data member builder for class Basic3DVector<float> -------------------
void __Basic3DVector_float__db_datamem(Reflex::Class* cl) {
  ::Reflex::ClassBuilder(cl)
  .AddDataMember(type_636, Reflex::Literal("theX"), OffsetOf(__shadow__::__Basic3DVector_float_, theX), ::Reflex::PRIVATE)
  .AddDataMember(type_636, Reflex::Literal("theY"), OffsetOf(__shadow__::__Basic3DVector_float_, theY), ::Reflex::PRIVATE)
  .AddDataMember(type_636, Reflex::Literal("theZ"), OffsetOf(__shadow__::__Basic3DVector_float_, theZ), ::Reflex::PRIVATE)
  .AddDataMember(type_636, Reflex::Literal("theW"), OffsetOf(__shadow__::__Basic3DVector_float_, theW), ::Reflex::PRIVATE);
}
//------Delayed function member builder for class Basic3DVector<float> -------------------
void __Basic3DVector_float__db_funcmem(Reflex::Class*) {

}
//------Stub functions for class TV -------------------------------
static void destructor_3270(void*, void * o, const std::vector<void*>&, void *) {
(((::TV*)o)->::TV::~TV)();
}
static void constructor_3272( void* retaddr, void* mem, const std::vector<void*>& arg, void*) {
  if (retaddr) *(void**)retaddr = ::new(mem) ::TV(*(const ::TV*)arg[0]);
  else ::new(mem) ::TV(*(const ::TV*)arg[0]);
}

static void constructor_3273( void* retaddr, void* mem, const std::vector<void*>&, void*) {
  if (retaddr) *(void**)retaddr = ::new(mem) ::TV();
  else ::new(mem) ::TV();
}

static void method_newdel_1663( void* retaddr, void*, const std::vector<void*>&, void*)
{
  static ::Reflex::NewDelFunctions s_funcs;
  s_funcs.fNew         = ::Reflex::NewDelFunctionsT< ::TV >::new_T;
  s_funcs.fNewArray    = ::Reflex::NewDelFunctionsT< ::TV >::newArray_T;
  s_funcs.fDelete      = ::Reflex::NewDelFunctionsT< ::TV >::delete_T;
  s_funcs.fDeleteArray = ::Reflex::NewDelFunctionsT< ::TV >::deleteArray_T;
  s_funcs.fDestructor  = ::Reflex::NewDelFunctionsT< ::TV >::destruct_T;
  if (retaddr) *(::Reflex::NewDelFunctions**)retaddr = &s_funcs;
}

//------Dictionary for class TV -------------------------------
void __TV_db_datamem(Reflex::Class*);
void __TV_db_funcmem(Reflex::Class*);
Reflex::GenreflexMemberBuilder __TV_datamem_bld(&__TV_db_datamem);
Reflex::GenreflexMemberBuilder __TV_funcmem_bld(&__TV_db_funcmem);
void __TV_dict() {
  ::Reflex::ClassBuilder(Reflex::Literal("TV"), typeid(::TV), sizeof(::TV), ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL, ::Reflex::STRUCT)
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void), Reflex::Literal("~TV"), destructor_3270, 0, 0, ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL | ::Reflex::DESTRUCTOR )
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void, type_6730), Reflex::Literal("TV"), constructor_3272, 0, "", ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL | ::Reflex::CONSTRUCTOR)
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void), Reflex::Literal("TV"), constructor_3273, 0, 0, ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL | ::Reflex::CONSTRUCTOR)
  .AddFunctionMember<void*(void)>(Reflex::Literal("__getNewDelFunctions"), method_newdel_1663, 0, 0, ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL)
  .AddOnDemandDataMemberBuilder(&__TV_datamem_bld);
}

//------Delayed data member builder for class TV -------------------
void __TV_db_datamem(Reflex::Class* cl) {
  ::Reflex::ClassBuilder(cl)
  .AddDataMember(type_800, Reflex::Literal("tvPos"), OffsetOf(__shadow__::__TV, tvPos), ::Reflex::PUBLIC)
  .AddDataMember(type_39, Reflex::Literal("pdgIdMother"), OffsetOf(__shadow__::__TV, pdgIdMother), ::Reflex::PUBLIC)
  .AddDataMember(type_530, Reflex::Literal("p4daughters"), OffsetOf(__shadow__::__TV, p4daughters), ::Reflex::PUBLIC)
  .AddDataMember(type_39, Reflex::Literal("numberChargedDaughters"), OffsetOf(__shadow__::__TV, numberChargedDaughters), ::Reflex::PUBLIC);
}
//------Delayed function member builder for class TV -------------------
void __TV_db_funcmem(Reflex::Class*) {

}
//------Stub functions for class PV3DBase<float,VectorTag,GlobalTag> -------------------------------
static void destructor_3282(void*, void * o, const std::vector<void*>&, void *) {
(((::PV3DBase<float,VectorTag,GlobalTag>*)o)->::PV3DBase<float,VectorTag,GlobalTag>::~PV3DBase)();
}
static void constructor_3284( void* retaddr, void* mem, const std::vector<void*>& arg, void*) {
  if (retaddr) *(void**)retaddr = ::new(mem) ::PV3DBase<float,VectorTag,GlobalTag>(*(const ::PV3DBase<float,VectorTag,GlobalTag>*)arg[0]);
  else ::new(mem) ::PV3DBase<float,VectorTag,GlobalTag>(*(const ::PV3DBase<float,VectorTag,GlobalTag>*)arg[0]);
}

static void constructor_3285( void* retaddr, void* mem, const std::vector<void*>&, void*) {
  if (retaddr) *(void**)retaddr = ::new(mem) ::PV3DBase<float,VectorTag,GlobalTag>();
  else ::new(mem) ::PV3DBase<float,VectorTag,GlobalTag>();
}

static void method_newdel_1707( void* retaddr, void*, const std::vector<void*>&, void*)
{
  static ::Reflex::NewDelFunctions s_funcs;
  s_funcs.fNew         = ::Reflex::NewDelFunctionsT< ::PV3DBase<float,VectorTag,GlobalTag> >::new_T;
  s_funcs.fNewArray    = ::Reflex::NewDelFunctionsT< ::PV3DBase<float,VectorTag,GlobalTag> >::newArray_T;
  s_funcs.fDelete      = ::Reflex::NewDelFunctionsT< ::PV3DBase<float,VectorTag,GlobalTag> >::delete_T;
  s_funcs.fDeleteArray = ::Reflex::NewDelFunctionsT< ::PV3DBase<float,VectorTag,GlobalTag> >::deleteArray_T;
  s_funcs.fDestructor  = ::Reflex::NewDelFunctionsT< ::PV3DBase<float,VectorTag,GlobalTag> >::destruct_T;
  if (retaddr) *(::Reflex::NewDelFunctions**)retaddr = &s_funcs;
}

//------Dictionary for class PV3DBase<float,VectorTag,GlobalTag> -------------------------------
void __PV3DBase_float_VectorTag_GlobalTag__db_datamem(Reflex::Class*);
void __PV3DBase_float_VectorTag_GlobalTag__db_funcmem(Reflex::Class*);
Reflex::GenreflexMemberBuilder __PV3DBase_float_VectorTag_GlobalTag__datamem_bld(&__PV3DBase_float_VectorTag_GlobalTag__db_datamem);
Reflex::GenreflexMemberBuilder __PV3DBase_float_VectorTag_GlobalTag__funcmem_bld(&__PV3DBase_float_VectorTag_GlobalTag__db_funcmem);
void __PV3DBase_float_VectorTag_GlobalTag__dict() {
  ::Reflex::ClassBuilder(Reflex::Literal("PV3DBase<float,VectorTag,GlobalTag>"), typeid(::PV3DBase<float,VectorTag,GlobalTag>), sizeof(::PV3DBase<float,VectorTag,GlobalTag>), ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL, ::Reflex::CLASS)
  .AddTypedef(type_636, Reflex::Literal("PV3DBase<float,VectorTag,GlobalTag>::ScalarType"))
  .AddTypedef(type_663, Reflex::Literal("PV3DBase<float,VectorTag,GlobalTag>::BasicVectorType"))
  .AddTypedef(type_2910, Reflex::Literal("PV3DBase<float,VectorTag,GlobalTag>::Cylindrical"))
  .AddTypedef(type_2912, Reflex::Literal("PV3DBase<float,VectorTag,GlobalTag>::Spherical"))
  .AddTypedef(type_2912, Reflex::Literal("PV3DBase<float,VectorTag,GlobalTag>::Polar"))
  .AddTypedef(type_663, Reflex::Literal("PV3DBase<float,VectorTag,GlobalTag>::MathVector"))
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void), Reflex::Literal("~PV3DBase"), destructor_3282, 0, 0, ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL | ::Reflex::DESTRUCTOR )
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void, type_6732), Reflex::Literal("PV3DBase"), constructor_3284, 0, "", ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL | ::Reflex::CONSTRUCTOR)
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void), Reflex::Literal("PV3DBase"), constructor_3285, 0, 0, ::Reflex::PUBLIC | ::Reflex::EXPLICIT | ::Reflex::CONSTRUCTOR)
  .AddFunctionMember<void*(void)>(Reflex::Literal("__getNewDelFunctions"), method_newdel_1707, 0, 0, ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL)
  .AddOnDemandDataMemberBuilder(&__PV3DBase_float_VectorTag_GlobalTag__datamem_bld);
}

//------Delayed data member builder for class PV3DBase<float,VectorTag,GlobalTag> -------------------
void __PV3DBase_float_VectorTag_GlobalTag__db_datamem(Reflex::Class* cl) {
  ::Reflex::ClassBuilder(cl)
  .AddDataMember(type_663, Reflex::Literal("theVector"), OffsetOf(__shadow__::__PV3DBase_float_VectorTag_GlobalTag_, theVector), ::Reflex::PROTECTED);
}
//------Delayed function member builder for class PV3DBase<float,VectorTag,GlobalTag> -------------------
void __PV3DBase_float_VectorTag_GlobalTag__db_funcmem(Reflex::Class*) {

}
//------Stub functions for class vector<double,std::allocator<double> > -------------------------------
static void constructor_3324( void* retaddr, void* mem, const std::vector<void*>&, void*) {
  if (retaddr) *(void**)retaddr = ::new(mem) ::std::vector<double>();
  else ::new(mem) ::std::vector<double>();
}

static void constructor_3327( void* retaddr, void* mem, const std::vector<void*>& arg, void*) {
  if (retaddr) *(void**)retaddr = ::new(mem) ::std::vector<double>(*(const ::std::vector<double>*)arg[0]);
  else ::new(mem) ::std::vector<double>(*(const ::std::vector<double>*)arg[0]);
}

static void destructor_3328(void*, void * o, const std::vector<void*>&, void *) {
(((::std::vector<double>*)o)->::std::vector<double>::~vector)();
}
static  void method_3339( void* retaddr, void* o, const std::vector<void*>&, void*)
{
  if (retaddr) new (retaddr) (std::size_t)((((const ::std::vector<double>*)o)->size)());
  else   (((const ::std::vector<double>*)o)->size)();
}

static  void method_3341( void*, void* o, const std::vector<void*>& arg, void*)
{
  if ( arg.size() == 1 ) {
    (((::std::vector<double>*)o)->resize)(*(::std::size_t*)arg[0]);
  }
  else if ( arg.size() == 2 ) { 
    (((::std::vector<double>*)o)->resize)(*(::std::size_t*)arg[0],
      *(double*)arg[1]);
  }
}

static  void method_3348( void* retaddr, void* o, const std::vector<void*>& arg, void*)
{
  if (retaddr) *(void**)retaddr = (void*)&(((::std::vector<double>*)o)->at)(*(::std::size_t*)arg[0]);
  else   (((::std::vector<double>*)o)->at)(*(::std::size_t*)arg[0]);
}

static  void method_3349( void* retaddr, void* o, const std::vector<void*>& arg, void*)
{
  if (retaddr) *(void**)retaddr = (void*)&(((const ::std::vector<double>*)o)->at)(*(::std::size_t*)arg[0]);
  else   (((const ::std::vector<double>*)o)->at)(*(::std::size_t*)arg[0]);
}

static  void method_3363( void*, void* o, const std::vector<void*>&, void*)
{
  (((::std::vector<double>*)o)->clear)();
}

static void method_newdel_1785( void* retaddr, void*, const std::vector<void*>&, void*)
{
  static ::Reflex::NewDelFunctions s_funcs;
  s_funcs.fNew         = ::Reflex::NewDelFunctionsT< ::std::vector<double> >::new_T;
  s_funcs.fNewArray    = ::Reflex::NewDelFunctionsT< ::std::vector<double> >::newArray_T;
  s_funcs.fDelete      = ::Reflex::NewDelFunctionsT< ::std::vector<double> >::delete_T;
  s_funcs.fDeleteArray = ::Reflex::NewDelFunctionsT< ::std::vector<double> >::deleteArray_T;
  s_funcs.fDestructor  = ::Reflex::NewDelFunctionsT< ::std::vector<double> >::destruct_T;
  if (retaddr) *(::Reflex::NewDelFunctions**)retaddr = &s_funcs;
}

static void method_x4( void* retaddr, void*, const std::vector<void*>&, void*)
{
  typedef std::vector<std::pair< ::Reflex::Base, int> > Bases_t;
  static Bases_t s_bases;
  if ( !s_bases.size() ) {
    s_bases.push_back(std::make_pair(::Reflex::Base( ::Reflex::TypeBuilder(Reflex::Literal("std::_Vector_base<double,std::allocator<double> >")), ::Reflex::BaseOffset< ::std::vector<double>,::std::_Vector_base<double,std::allocator<double> > >::Get(),::Reflex::PROTECTED), 0));
  }
  if (retaddr) *(Bases_t**)retaddr = &s_bases;
}

static void method_x5( void* retaddr, void*, const std::vector<void*>&, void*)
{
  if (retaddr) *(void**) retaddr = ::Reflex::Proxy< ::std::vector<double> >::Generate();
  else ::Reflex::Proxy< ::std::vector<double> >::Generate();
}

//------Dictionary for class vector<double,std::allocator<double> > -------------------------------
void __std__vector_double__db_datamem(Reflex::Class*);
void __std__vector_double__db_funcmem(Reflex::Class*);
Reflex::GenreflexMemberBuilder __std__vector_double__datamem_bld(&__std__vector_double__db_datamem);
Reflex::GenreflexMemberBuilder __std__vector_double__funcmem_bld(&__std__vector_double__db_funcmem);
void __std__vector_double__dict() {
  ::Reflex::ClassBuilder(Reflex::Literal("std::vector<double>"), typeid(::std::vector<double>), sizeof(::std::vector<double>), ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL, ::Reflex::CLASS)
  .AddBase(type_2170, ::Reflex::BaseOffset< ::std::vector<double>, ::std::_Vector_base<double,std::allocator<double> > >::Get(), ::Reflex::PROTECTED)
  .AddTypedef(type_69, Reflex::Literal("std::vector<double>::_Alloc_value_type"))
  .AddTypedef(type_2170, Reflex::Literal("std::vector<double>::_Base"))
  .AddTypedef(type_2079, Reflex::Literal("std::vector<double>::_Tp_alloc_type"))
  .AddTypedef(type_2696, Reflex::Literal("std::vector<double>::_Alloc_traits"))
  .AddTypedef(type_69, Reflex::Literal("std::vector<double>::value_type"))
  .AddTypedef(type_2558, Reflex::Literal("std::vector<double>::pointer"))
  .AddTypedef(type_2598, Reflex::Literal("std::vector<double>::const_pointer"))
  .AddTypedef(type_3313, Reflex::Literal("std::vector<double>::reference"))
  .AddTypedef(type_3315, Reflex::Literal("std::vector<double>::const_reference"))
  .AddTypedef(type_2709, Reflex::Literal("std::vector<double>::iterator"))
  .AddTypedef(type_2710, Reflex::Literal("std::vector<double>::const_iterator"))
  .AddTypedef(type_2161, Reflex::Literal("std::vector<double>::const_reverse_iterator"))
  .AddTypedef(type_2162, Reflex::Literal("std::vector<double>::reverse_iterator"))
  .AddTypedef(type_2155, Reflex::Literal("std::vector<double>::size_type"))
  .AddTypedef(type_2098, Reflex::Literal("std::vector<double>::difference_type"))
  .AddTypedef(type_2079, Reflex::Literal("std::vector<double>::allocator_type"))
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void), Reflex::Literal("vector"), constructor_3324, 0, 0, ::Reflex::PUBLIC | ::Reflex::EXPLICIT | ::Reflex::CONSTRUCTOR)
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void, type_6734), Reflex::Literal("vector"), constructor_3327, 0, "__x", ::Reflex::PUBLIC | ::Reflex::CONSTRUCTOR)
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void), Reflex::Literal("~vector"), destructor_3328, 0, 0, ::Reflex::PUBLIC | ::Reflex::DESTRUCTOR )
  .AddFunctionMember<void*(void)>(Reflex::Literal("__getNewDelFunctions"), method_newdel_1785, 0, 0, ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL)
  .AddFunctionMember<void*(void)>(Reflex::Literal("__getBasesTable"), method_x4, 0, 0, ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL)
  .AddFunctionMember<void*(void)>(Reflex::Literal("createCollFuncTable"), method_x5, 0, 0, ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL)
  .AddOnDemandFunctionMemberBuilder(&__std__vector_double__funcmem_bld);
}

//------Delayed data member builder for class vector<double,std::allocator<double> > -------------------
void __std__vector_double__db_datamem(Reflex::Class*) {

}
//------Delayed function member builder for class vector<double,std::allocator<double> > -------------------
void __std__vector_double__db_funcmem(Reflex::Class* cl) {
  ::Reflex::ClassBuilder(cl)
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_2155), Reflex::Literal("size"), method_3339, 0, 0, ::Reflex::PUBLIC | ::Reflex::CONST)
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_113, type_2155, type_69), Reflex::Literal("resize"), method_3341, 0, "__new_size;__x=_Tp()", ::Reflex::PUBLIC)
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_3313, type_2155), Reflex::Literal("at"), method_3348, 0, "__n", ::Reflex::PUBLIC)
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_3315, type_2155), Reflex::Literal("at"), method_3349, 0, "__n", ::Reflex::PUBLIC | ::Reflex::CONST)
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_113), Reflex::Literal("clear"), method_3363, 0, 0, ::Reflex::PUBLIC);
}
//------Stub functions for class MatRepSym<double,3> -------------------------------
static void destructor_5949(void*, void * o, const std::vector<void*>&, void *) {
(((::ROOT::Math::MatRepSym<double,3>*)o)->::ROOT::Math::MatRepSym<double,3>::~MatRepSym)();
}
static void constructor_5950( void* retaddr, void* mem, const std::vector<void*>& arg, void*) {
  if (retaddr) *(void**)retaddr = ::new(mem) ::ROOT::Math::MatRepSym<double,3>(*(const ::ROOT::Math::MatRepSym<double,3>*)arg[0]);
  else ::new(mem) ::ROOT::Math::MatRepSym<double,3>(*(const ::ROOT::Math::MatRepSym<double,3>*)arg[0]);
}

static void constructor_5951( void* retaddr, void* mem, const std::vector<void*>&, void*) {
  if (retaddr) *(void**)retaddr = ::new(mem) ::ROOT::Math::MatRepSym<double,3>();
  else ::new(mem) ::ROOT::Math::MatRepSym<double,3>();
}

static void method_newdel_2595( void* retaddr, void*, const std::vector<void*>&, void*)
{
  static ::Reflex::NewDelFunctions s_funcs;
  s_funcs.fNew         = ::Reflex::NewDelFunctionsT< ::ROOT::Math::MatRepSym<double,3> >::new_T;
  s_funcs.fNewArray    = ::Reflex::NewDelFunctionsT< ::ROOT::Math::MatRepSym<double,3> >::newArray_T;
  s_funcs.fDelete      = ::Reflex::NewDelFunctionsT< ::ROOT::Math::MatRepSym<double,3> >::delete_T;
  s_funcs.fDeleteArray = ::Reflex::NewDelFunctionsT< ::ROOT::Math::MatRepSym<double,3> >::deleteArray_T;
  s_funcs.fDestructor  = ::Reflex::NewDelFunctionsT< ::ROOT::Math::MatRepSym<double,3> >::destruct_T;
  if (retaddr) *(::Reflex::NewDelFunctions**)retaddr = &s_funcs;
}

//------Dictionary for class MatRepSym<double,3> -------------------------------
void __ROOT__Math__MatRepSym_double_3__db_datamem(Reflex::Class*);
void __ROOT__Math__MatRepSym_double_3__db_funcmem(Reflex::Class*);
Reflex::GenreflexMemberBuilder __ROOT__Math__MatRepSym_double_3__datamem_bld(&__ROOT__Math__MatRepSym_double_3__db_datamem);
Reflex::GenreflexMemberBuilder __ROOT__Math__MatRepSym_double_3__funcmem_bld(&__ROOT__Math__MatRepSym_double_3__db_funcmem);
void __ROOT__Math__MatRepSym_double_3__dict() {
  ::Reflex::ClassBuilder(Reflex::Literal("ROOT::Math::MatRepSym<double,3>"), typeid(::ROOT::Math::MatRepSym<double,3>), sizeof(::ROOT::Math::MatRepSym<double,3>), ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL, ::Reflex::CLASS)
  .AddTypedef(type_69, Reflex::Literal("ROOT::Math::MatRepSym<double,3>::value_type"))
  .AddEnum(Reflex::Literal("_77"), Reflex::Literal("kRows=3;kCols=3;kSize=6"), &typeid(::Reflex::UnnamedEnum), ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL)
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void), Reflex::Literal("~MatRepSym"), destructor_5949, 0, 0, ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL | ::Reflex::DESTRUCTOR )
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void, type_7261), Reflex::Literal("MatRepSym"), constructor_5950, 0, "", ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL | ::Reflex::CONSTRUCTOR)
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void), Reflex::Literal("MatRepSym"), constructor_5951, 0, 0, ::Reflex::PUBLIC | ::Reflex::EXPLICIT | ::Reflex::CONSTRUCTOR)
  .AddFunctionMember<void*(void)>(Reflex::Literal("__getNewDelFunctions"), method_newdel_2595, 0, 0, ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL)
  .AddOnDemandDataMemberBuilder(&__ROOT__Math__MatRepSym_double_3__datamem_bld);
}

//------Delayed data member builder for class MatRepSym<double,3> -------------------
void __ROOT__Math__MatRepSym_double_3__db_datamem(Reflex::Class* cl) {
  ::Reflex::ClassBuilder(cl)
  .AddDataMember(type_7259, Reflex::Literal("fArray"), OffsetOf(__shadow__::__ROOT__Math__MatRepSym_double_3_, fArray), ::Reflex::PRIVATE)
  .AddDataMember(type_7260, Reflex::Literal("fOff"), OffsetOf(__shadow__::__ROOT__Math__MatRepSym_double_3_, fOff), ::Reflex::PRIVATE);
}
//------Delayed function member builder for class MatRepSym<double,3> -------------------
void __ROOT__Math__MatRepSym_double_3__db_funcmem(Reflex::Class*) {

}
//------Stub functions for class RowOffsets<3> -------------------------------
static void destructor_7459(void*, void * o, const std::vector<void*>&, void *) {
(((::ROOT::Math::RowOffsets<3>*)o)->::ROOT::Math::RowOffsets<3>::~RowOffsets)();
}
static void constructor_7461( void* retaddr, void* mem, const std::vector<void*>& arg, void*) {
  if (retaddr) *(void**)retaddr = ::new(mem) ::ROOT::Math::RowOffsets<3>(*(const ::ROOT::Math::RowOffsets<3>*)arg[0]);
  else ::new(mem) ::ROOT::Math::RowOffsets<3>(*(const ::ROOT::Math::RowOffsets<3>*)arg[0]);
}

static void constructor_7462( void* retaddr, void* mem, const std::vector<void*>&, void*) {
  if (retaddr) *(void**)retaddr = ::new(mem) ::ROOT::Math::RowOffsets<3>();
  else ::new(mem) ::ROOT::Math::RowOffsets<3>();
}

static void method_newdel_6586( void* retaddr, void*, const std::vector<void*>&, void*)
{
  static ::Reflex::NewDelFunctions s_funcs;
  s_funcs.fNew         = ::Reflex::NewDelFunctionsT< ::ROOT::Math::RowOffsets<3> >::new_T;
  s_funcs.fNewArray    = ::Reflex::NewDelFunctionsT< ::ROOT::Math::RowOffsets<3> >::newArray_T;
  s_funcs.fDelete      = ::Reflex::NewDelFunctionsT< ::ROOT::Math::RowOffsets<3> >::delete_T;
  s_funcs.fDeleteArray = ::Reflex::NewDelFunctionsT< ::ROOT::Math::RowOffsets<3> >::deleteArray_T;
  s_funcs.fDestructor  = ::Reflex::NewDelFunctionsT< ::ROOT::Math::RowOffsets<3> >::destruct_T;
  if (retaddr) *(::Reflex::NewDelFunctions**)retaddr = &s_funcs;
}

//------Dictionary for class RowOffsets<3> -------------------------------
void __ROOT__Math__RowOffsets_3__db_datamem(Reflex::Class*);
void __ROOT__Math__RowOffsets_3__db_funcmem(Reflex::Class*);
Reflex::GenreflexMemberBuilder __ROOT__Math__RowOffsets_3__datamem_bld(&__ROOT__Math__RowOffsets_3__db_datamem);
Reflex::GenreflexMemberBuilder __ROOT__Math__RowOffsets_3__funcmem_bld(&__ROOT__Math__RowOffsets_3__db_funcmem);
void __ROOT__Math__RowOffsets_3__dict() {
  ::Reflex::ClassBuilder(Reflex::Literal("ROOT::Math::RowOffsets<3>"), typeid(::ROOT::Math::RowOffsets<3>), sizeof(::ROOT::Math::RowOffsets<3>), ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL, ::Reflex::STRUCT)
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void), Reflex::Literal("~RowOffsets"), destructor_7459, 0, 0, ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL | ::Reflex::DESTRUCTOR )
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void, type_7263), Reflex::Literal("RowOffsets"), constructor_7461, 0, "", ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL | ::Reflex::CONSTRUCTOR)
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void), Reflex::Literal("RowOffsets"), constructor_7462, 0, 0, ::Reflex::PUBLIC | ::Reflex::EXPLICIT | ::Reflex::CONSTRUCTOR)
  .AddFunctionMember<void*(void)>(Reflex::Literal("__getNewDelFunctions"), method_newdel_6586, 0, 0, ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL);
}

//------Delayed data member builder for class RowOffsets<3> -------------------
void __ROOT__Math__RowOffsets_3__db_datamem(Reflex::Class*) {

}
//------Delayed function member builder for class RowOffsets<3> -------------------
void __ROOT__Math__RowOffsets_3__db_funcmem(Reflex::Class*) {

}
//------Stub functions for class SMatrix<double,3,3,ROOT::Math::MatRepSym<double, 3> > -------------------------------
static void destructor_2603(void*, void * o, const std::vector<void*>&, void *) {
(((::ROOT::Math::SMatrix<double,3,3,ROOT::Math::MatRepSym<double,3> >*)o)->::ROOT::Math::SMatrix<double,3,3,ROOT::Math::MatRepSym<double,3> >::~SMatrix)();
}
static void constructor_2605( void* retaddr, void* mem, const std::vector<void*>&, void*) {
  if (retaddr) *(void**)retaddr = ::new(mem) ::ROOT::Math::SMatrix<double,3,3,ROOT::Math::MatRepSym<double,3> >();
  else ::new(mem) ::ROOT::Math::SMatrix<double,3,3,ROOT::Math::MatRepSym<double,3> >();
}

static void constructor_2607( void* retaddr, void* mem, const std::vector<void*>& arg, void*) {
  if (retaddr) *(void**)retaddr = ::new(mem) ::ROOT::Math::SMatrix<double,3,3,ROOT::Math::MatRepSym<double,3> >(*(const ::ROOT::Math::SMatrix<double,3,3,ROOT::Math::MatRepSym<double,3> >*)arg[0]);
  else ::new(mem) ::ROOT::Math::SMatrix<double,3,3,ROOT::Math::MatRepSym<double,3> >(*(const ::ROOT::Math::SMatrix<double,3,3,ROOT::Math::MatRepSym<double,3> >*)arg[0]);
}

static void method_newdel_394( void* retaddr, void*, const std::vector<void*>&, void*)
{
  static ::Reflex::NewDelFunctions s_funcs;
  s_funcs.fNew         = ::Reflex::NewDelFunctionsT< ::ROOT::Math::SMatrix<double,3,3,ROOT::Math::MatRepSym<double,3> > >::new_T;
  s_funcs.fNewArray    = ::Reflex::NewDelFunctionsT< ::ROOT::Math::SMatrix<double,3,3,ROOT::Math::MatRepSym<double,3> > >::newArray_T;
  s_funcs.fDelete      = ::Reflex::NewDelFunctionsT< ::ROOT::Math::SMatrix<double,3,3,ROOT::Math::MatRepSym<double,3> > >::delete_T;
  s_funcs.fDeleteArray = ::Reflex::NewDelFunctionsT< ::ROOT::Math::SMatrix<double,3,3,ROOT::Math::MatRepSym<double,3> > >::deleteArray_T;
  s_funcs.fDestructor  = ::Reflex::NewDelFunctionsT< ::ROOT::Math::SMatrix<double,3,3,ROOT::Math::MatRepSym<double,3> > >::destruct_T;
  if (retaddr) *(::Reflex::NewDelFunctions**)retaddr = &s_funcs;
}

//------Dictionary for class SMatrix<double,3,3,ROOT::Math::MatRepSym<double, 3> > -------------------------------
void __ROOT__Math__SMatrix_double_3_3_ROOT__Math__MatRepSym_double_3_s__db_datamem(Reflex::Class*);
void __ROOT__Math__SMatrix_double_3_3_ROOT__Math__MatRepSym_double_3_s__db_funcmem(Reflex::Class*);
Reflex::GenreflexMemberBuilder __ROOT__Math__SMatrix_double_3_3_ROOT__Math__MatRepSym_double_3_s__datamem_bld(&__ROOT__Math__SMatrix_double_3_3_ROOT__Math__MatRepSym_double_3_s__db_datamem);
Reflex::GenreflexMemberBuilder __ROOT__Math__SMatrix_double_3_3_ROOT__Math__MatRepSym_double_3_s__funcmem_bld(&__ROOT__Math__SMatrix_double_3_3_ROOT__Math__MatRepSym_double_3_s__db_funcmem);
void __ROOT__Math__SMatrix_double_3_3_ROOT__Math__MatRepSym_double_3_s__dict() {
  ::Reflex::ClassBuilder(Reflex::Literal("ROOT::Math::SMatrix<double,3,3,ROOT::Math::MatRepSym<double,3> >"), typeid(::ROOT::Math::SMatrix<double,3,3,ROOT::Math::MatRepSym<double,3> >), sizeof(::ROOT::Math::SMatrix<double,3,3,ROOT::Math::MatRepSym<double,3> >), ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL, ::Reflex::CLASS)
  .AddTypedef(type_69, Reflex::Literal("ROOT::Math::SMatrix<double,3,3,ROOT::Math::MatRepSym<double,3> >::value_type"))
  .AddTypedef(type_2595, Reflex::Literal("ROOT::Math::SMatrix<double,3,3,ROOT::Math::MatRepSym<double,3> >::rep_type"))
  .AddTypedef(type_2558, Reflex::Literal("ROOT::Math::SMatrix<double,3,3,ROOT::Math::MatRepSym<double,3> >::iterator"))
  .AddTypedef(type_2598, Reflex::Literal("ROOT::Math::SMatrix<double,3,3,ROOT::Math::MatRepSym<double,3> >::const_iterator"))
  .AddEnum(Reflex::Literal("_78"), Reflex::Literal("kRows=3;kCols=3;kSize=9"), &typeid(::Reflex::UnnamedEnum), ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL)
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void), Reflex::Literal("~SMatrix"), destructor_2603, 0, 0, ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL | ::Reflex::DESTRUCTOR )
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void), Reflex::Literal("SMatrix"), constructor_2605, 0, 0, ::Reflex::PUBLIC | ::Reflex::EXPLICIT | ::Reflex::CONSTRUCTOR)
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void, type_5966), Reflex::Literal("SMatrix"), constructor_2607, 0, "rhs", ::Reflex::PUBLIC | ::Reflex::CONSTRUCTOR)
  .AddFunctionMember<void*(void)>(Reflex::Literal("__getNewDelFunctions"), method_newdel_394, 0, 0, ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL)
  .AddOnDemandDataMemberBuilder(&__ROOT__Math__SMatrix_double_3_3_ROOT__Math__MatRepSym_double_3_s__datamem_bld);
}

//------Delayed data member builder for class SMatrix<double,3,3,ROOT::Math::MatRepSym<double, 3> > -------------------
void __ROOT__Math__SMatrix_double_3_3_ROOT__Math__MatRepSym_double_3_s__db_datamem(Reflex::Class* cl) {
  ::Reflex::ClassBuilder(cl)
  .AddDataMember(type_2595, Reflex::Literal("fRep"), OffsetOf(__shadow__::__ROOT__Math__SMatrix_double_3_3_ROOT__Math__MatRepSym_double_3_s_, fRep), ::Reflex::PUBLIC);
}
//------Delayed function member builder for class SMatrix<double,3,3,ROOT::Math::MatRepSym<double, 3> > -------------------
void __ROOT__Math__SMatrix_double_3_3_ROOT__Math__MatRepSym_double_3_s__db_funcmem(Reflex::Class*) {

}
//------Stub functions for class LorentzVector<ROOT::Math::PxPyPzE4D<double> > -------------------------------
static void destructor_2776(void*, void * o, const std::vector<void*>&, void *) {
(((::ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> >*)o)->::ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> >::~LorentzVector)();
}
static void constructor_2778( void* retaddr, void* mem, const std::vector<void*>& arg, void*) {
  if (retaddr) *(void**)retaddr = ::new(mem) ::ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> >(*(const ::ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> >*)arg[0]);
  else ::new(mem) ::ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> >(*(const ::ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> >*)arg[0]);
}

static void constructor_2779( void* retaddr, void* mem, const std::vector<void*>&, void*) {
  if (retaddr) *(void**)retaddr = ::new(mem) ::ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> >();
  else ::new(mem) ::ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> >();
}

static void method_newdel_529( void* retaddr, void*, const std::vector<void*>&, void*)
{
  static ::Reflex::NewDelFunctions s_funcs;
  s_funcs.fNew         = ::Reflex::NewDelFunctionsT< ::ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> > >::new_T;
  s_funcs.fNewArray    = ::Reflex::NewDelFunctionsT< ::ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> > >::newArray_T;
  s_funcs.fDelete      = ::Reflex::NewDelFunctionsT< ::ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> > >::delete_T;
  s_funcs.fDeleteArray = ::Reflex::NewDelFunctionsT< ::ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> > >::deleteArray_T;
  s_funcs.fDestructor  = ::Reflex::NewDelFunctionsT< ::ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> > >::destruct_T;
  if (retaddr) *(::Reflex::NewDelFunctions**)retaddr = &s_funcs;
}

//------Dictionary for class LorentzVector<ROOT::Math::PxPyPzE4D<double> > -------------------------------
void __ROOT__Math__LorentzVector_ROOT__Math__PxPyPzE4D_double_s__db_datamem(Reflex::Class*);
void __ROOT__Math__LorentzVector_ROOT__Math__PxPyPzE4D_double_s__db_funcmem(Reflex::Class*);
Reflex::GenreflexMemberBuilder __ROOT__Math__LorentzVector_ROOT__Math__PxPyPzE4D_double_s__datamem_bld(&__ROOT__Math__LorentzVector_ROOT__Math__PxPyPzE4D_double_s__db_datamem);
Reflex::GenreflexMemberBuilder __ROOT__Math__LorentzVector_ROOT__Math__PxPyPzE4D_double_s__funcmem_bld(&__ROOT__Math__LorentzVector_ROOT__Math__PxPyPzE4D_double_s__db_funcmem);
void __ROOT__Math__LorentzVector_ROOT__Math__PxPyPzE4D_double_s__dict() {
  ::Reflex::ClassBuilder(Reflex::Literal("ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> >"), typeid(::ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> >), sizeof(::ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> >), ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL, ::Reflex::CLASS)
  .AddTypedef(type_69, Reflex::Literal("ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> >::Scalar"))
  .AddTypedef(type_2772, Reflex::Literal("ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> >::CoordinateType"))
  .AddTypedef(type_2774, Reflex::Literal("ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> >::BetaVector"))
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void), Reflex::Literal("~LorentzVector"), destructor_2776, 0, 0, ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL | ::Reflex::DESTRUCTOR )
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void, type_6347), Reflex::Literal("LorentzVector"), constructor_2778, 0, "", ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL | ::Reflex::CONSTRUCTOR)
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void), Reflex::Literal("LorentzVector"), constructor_2779, 0, 0, ::Reflex::PUBLIC | ::Reflex::EXPLICIT | ::Reflex::CONSTRUCTOR)
  .AddFunctionMember<void*(void)>(Reflex::Literal("__getNewDelFunctions"), method_newdel_529, 0, 0, ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL)
  .AddOnDemandDataMemberBuilder(&__ROOT__Math__LorentzVector_ROOT__Math__PxPyPzE4D_double_s__datamem_bld);
}

//------Delayed data member builder for class LorentzVector<ROOT::Math::PxPyPzE4D<double> > -------------------
void __ROOT__Math__LorentzVector_ROOT__Math__PxPyPzE4D_double_s__db_datamem(Reflex::Class* cl) {
  ::Reflex::ClassBuilder(cl)
  .AddDataMember(type_2772, Reflex::Literal("fCoordinates"), OffsetOf(__shadow__::__ROOT__Math__LorentzVector_ROOT__Math__PxPyPzE4D_double_s_, fCoordinates), ::Reflex::PRIVATE);
}
//------Delayed function member builder for class LorentzVector<ROOT::Math::PxPyPzE4D<double> > -------------------
void __ROOT__Math__LorentzVector_ROOT__Math__PxPyPzE4D_double_s__db_funcmem(Reflex::Class*) {

}
//------Stub functions for class Vector3DBase<float,GlobalTag> -------------------------------
static void destructor_3064(void*, void * o, const std::vector<void*>&, void *) {
(((::Vector3DBase<float,GlobalTag>*)o)->::Vector3DBase<float,GlobalTag>::~Vector3DBase)();
}
static void constructor_3066( void* retaddr, void* mem, const std::vector<void*>& arg, void*) {
  if (retaddr) *(void**)retaddr = ::new(mem) ::Vector3DBase<float,GlobalTag>(*(const ::Vector3DBase<float,GlobalTag>*)arg[0]);
  else ::new(mem) ::Vector3DBase<float,GlobalTag>(*(const ::Vector3DBase<float,GlobalTag>*)arg[0]);
}

static void constructor_3067( void* retaddr, void* mem, const std::vector<void*>&, void*) {
  if (retaddr) *(void**)retaddr = ::new(mem) ::Vector3DBase<float,GlobalTag>();
  else ::new(mem) ::Vector3DBase<float,GlobalTag>();
}

static void method_newdel_888( void* retaddr, void*, const std::vector<void*>&, void*)
{
  static ::Reflex::NewDelFunctions s_funcs;
  s_funcs.fNew         = ::Reflex::NewDelFunctionsT< ::Vector3DBase<float,GlobalTag> >::new_T;
  s_funcs.fNewArray    = ::Reflex::NewDelFunctionsT< ::Vector3DBase<float,GlobalTag> >::newArray_T;
  s_funcs.fDelete      = ::Reflex::NewDelFunctionsT< ::Vector3DBase<float,GlobalTag> >::delete_T;
  s_funcs.fDeleteArray = ::Reflex::NewDelFunctionsT< ::Vector3DBase<float,GlobalTag> >::deleteArray_T;
  s_funcs.fDestructor  = ::Reflex::NewDelFunctionsT< ::Vector3DBase<float,GlobalTag> >::destruct_T;
  if (retaddr) *(::Reflex::NewDelFunctions**)retaddr = &s_funcs;
}

static void method_x11( void* retaddr, void*, const std::vector<void*>&, void*)
{
  typedef std::vector<std::pair< ::Reflex::Base, int> > Bases_t;
  static Bases_t s_bases;
  if ( !s_bases.size() ) {
    s_bases.push_back(std::make_pair(::Reflex::Base( ::Reflex::TypeBuilder(Reflex::Literal("PV3DBase<float,VectorTag,GlobalTag>")), ::Reflex::BaseOffset< ::Vector3DBase<float,GlobalTag>,::PV3DBase<float,VectorTag,GlobalTag> >::Get(),::Reflex::PUBLIC), 0));
  }
  if (retaddr) *(Bases_t**)retaddr = &s_bases;
}

//------Dictionary for class Vector3DBase<float,GlobalTag> -------------------------------
void __Vector3DBase_float_GlobalTag__db_datamem(Reflex::Class*);
void __Vector3DBase_float_GlobalTag__db_funcmem(Reflex::Class*);
Reflex::GenreflexMemberBuilder __Vector3DBase_float_GlobalTag__datamem_bld(&__Vector3DBase_float_GlobalTag__db_datamem);
Reflex::GenreflexMemberBuilder __Vector3DBase_float_GlobalTag__funcmem_bld(&__Vector3DBase_float_GlobalTag__db_funcmem);
void __Vector3DBase_float_GlobalTag__dict() {
  ::Reflex::ClassBuilder(Reflex::Literal("Vector3DBase<float,GlobalTag>"), typeid(::Vector3DBase<float,GlobalTag>), sizeof(::Vector3DBase<float,GlobalTag>), ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL, ::Reflex::CLASS)
  .AddBase(type_1707, ::Reflex::BaseOffset< ::Vector3DBase<float,GlobalTag>, ::PV3DBase<float,VectorTag,GlobalTag> >::Get(), ::Reflex::PUBLIC)
  .AddTypedef(type_1707, Reflex::Literal("Vector3DBase<float,GlobalTag>::BaseClass"))
  .AddTypedef(type_2910, Reflex::Literal("Vector3DBase<float,GlobalTag>::Cylindrical"))
  .AddTypedef(type_2912, Reflex::Literal("Vector3DBase<float,GlobalTag>::Spherical"))
  .AddTypedef(type_2912, Reflex::Literal("Vector3DBase<float,GlobalTag>::Polar"))
  .AddTypedef(type_663, Reflex::Literal("Vector3DBase<float,GlobalTag>::BasicVectorType"))
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void), Reflex::Literal("~Vector3DBase"), destructor_3064, 0, 0, ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL | ::Reflex::DESTRUCTOR )
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void, type_6473), Reflex::Literal("Vector3DBase"), constructor_3066, 0, "", ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL | ::Reflex::CONSTRUCTOR)
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void), Reflex::Literal("Vector3DBase"), constructor_3067, 0, 0, ::Reflex::PUBLIC | ::Reflex::EXPLICIT | ::Reflex::CONSTRUCTOR)
  .AddFunctionMember<void*(void)>(Reflex::Literal("__getNewDelFunctions"), method_newdel_888, 0, 0, ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL)
  .AddFunctionMember<void*(void)>(Reflex::Literal("__getBasesTable"), method_x11, 0, 0, ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL);
}

//------Delayed data member builder for class Vector3DBase<float,GlobalTag> -------------------
void __Vector3DBase_float_GlobalTag__db_datamem(Reflex::Class*) {

}
//------Delayed function member builder for class Vector3DBase<float,GlobalTag> -------------------
void __Vector3DBase_float_GlobalTag__db_funcmem(Reflex::Class*) {

}
//------Stub functions for class PositionVector3D<ROOT::Math::Cartesian3D<double>,ROOT::Math::DefaultCoordinateSystemTag> -------------------------------
static void destructor_2991(void*, void * o, const std::vector<void*>&, void *) {
(((::ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>,ROOT::Math::DefaultCoordinateSystemTag>*)o)->::ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>,ROOT::Math::DefaultCoordinateSystemTag>::~PositionVector3D)();
}
static void constructor_2993( void* retaddr, void* mem, const std::vector<void*>& arg, void*) {
  if (retaddr) *(void**)retaddr = ::new(mem) ::ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>,ROOT::Math::DefaultCoordinateSystemTag>(*(const ::ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>,ROOT::Math::DefaultCoordinateSystemTag>*)arg[0]);
  else ::new(mem) ::ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>,ROOT::Math::DefaultCoordinateSystemTag>(*(const ::ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>,ROOT::Math::DefaultCoordinateSystemTag>*)arg[0]);
}

static void constructor_2994( void* retaddr, void* mem, const std::vector<void*>&, void*) {
  if (retaddr) *(void**)retaddr = ::new(mem) ::ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>,ROOT::Math::DefaultCoordinateSystemTag>();
  else ::new(mem) ::ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>,ROOT::Math::DefaultCoordinateSystemTag>();
}

static void method_newdel_799( void* retaddr, void*, const std::vector<void*>&, void*)
{
  static ::Reflex::NewDelFunctions s_funcs;
  s_funcs.fNew         = ::Reflex::NewDelFunctionsT< ::ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>,ROOT::Math::DefaultCoordinateSystemTag> >::new_T;
  s_funcs.fNewArray    = ::Reflex::NewDelFunctionsT< ::ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>,ROOT::Math::DefaultCoordinateSystemTag> >::newArray_T;
  s_funcs.fDelete      = ::Reflex::NewDelFunctionsT< ::ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>,ROOT::Math::DefaultCoordinateSystemTag> >::delete_T;
  s_funcs.fDeleteArray = ::Reflex::NewDelFunctionsT< ::ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>,ROOT::Math::DefaultCoordinateSystemTag> >::deleteArray_T;
  s_funcs.fDestructor  = ::Reflex::NewDelFunctionsT< ::ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>,ROOT::Math::DefaultCoordinateSystemTag> >::destruct_T;
  if (retaddr) *(::Reflex::NewDelFunctions**)retaddr = &s_funcs;
}

//------Dictionary for class PositionVector3D<ROOT::Math::Cartesian3D<double>,ROOT::Math::DefaultCoordinateSystemTag> -------------------------------
void __ROOT__Math__PositionVector3D_ROOT__Math__Cartesian3D_double__ROOT__Math__DefaultCoordinateSystemTag__db_datamem(Reflex::Class*);
void __ROOT__Math__PositionVector3D_ROOT__Math__Cartesian3D_double__ROOT__Math__DefaultCoordinateSystemTag__db_funcmem(Reflex::Class*);
Reflex::GenreflexMemberBuilder __ROOT__Math__PositionVector3D_ROOT__Math__Cartesian3D_double__ROOT__Math__DefaultCoordinateSystemTag__datamem_bld(&__ROOT__Math__PositionVector3D_ROOT__Math__Cartesian3D_double__ROOT__Math__DefaultCoordinateSystemTag__db_datamem);
Reflex::GenreflexMemberBuilder __ROOT__Math__PositionVector3D_ROOT__Math__Cartesian3D_double__ROOT__Math__DefaultCoordinateSystemTag__funcmem_bld(&__ROOT__Math__PositionVector3D_ROOT__Math__Cartesian3D_double__ROOT__Math__DefaultCoordinateSystemTag__db_funcmem);
void __ROOT__Math__PositionVector3D_ROOT__Math__Cartesian3D_double__ROOT__Math__DefaultCoordinateSystemTag__dict() {
  ::Reflex::ClassBuilder(Reflex::Literal("ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>,ROOT::Math::DefaultCoordinateSystemTag>"), typeid(::ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>,ROOT::Math::DefaultCoordinateSystemTag>), sizeof(::ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>,ROOT::Math::DefaultCoordinateSystemTag>), ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL, ::Reflex::CLASS)
  .AddProperty(Reflex::Literal("o_name"), "ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<Double32_t>,ROOT::Math::DefaultCoordinateSystemTag>")
  .AddTypedef(type_69, Reflex::Literal("ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>,ROOT::Math::DefaultCoordinateSystemTag>::Scalar"))
  .AddTypedef(type_2987, Reflex::Literal("ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>,ROOT::Math::DefaultCoordinateSystemTag>::CoordinateType"))
  .AddTypedef(type_2989, Reflex::Literal("ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>,ROOT::Math::DefaultCoordinateSystemTag>::CoordinateSystemTag"))
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void), Reflex::Literal("~PositionVector3D"), destructor_2991, 0, 0, ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL | ::Reflex::DESTRUCTOR )
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void, type_6453), Reflex::Literal("PositionVector3D"), constructor_2993, 0, "", ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL | ::Reflex::CONSTRUCTOR)
  .AddFunctionMember(::Reflex::FunctionTypeBuilder(type_void), Reflex::Literal("PositionVector3D"), constructor_2994, 0, 0, ::Reflex::PUBLIC | ::Reflex::EXPLICIT | ::Reflex::CONSTRUCTOR)
  .AddFunctionMember<void*(void)>(Reflex::Literal("__getNewDelFunctions"), method_newdel_799, 0, 0, ::Reflex::PUBLIC | ::Reflex::ARTIFICIAL)
  .AddOnDemandDataMemberBuilder(&__ROOT__Math__PositionVector3D_ROOT__Math__Cartesian3D_double__ROOT__Math__DefaultCoordinateSystemTag__datamem_bld);
}

//------Delayed data member builder for class PositionVector3D<ROOT::Math::Cartesian3D<double>,ROOT::Math::DefaultCoordinateSystemTag> -------------------
void __ROOT__Math__PositionVector3D_ROOT__Math__Cartesian3D_double__ROOT__Math__DefaultCoordinateSystemTag__db_datamem(Reflex::Class* cl) {
  ::Reflex::ClassBuilder(cl)
  .AddDataMember(type_2987, Reflex::Literal("fCoordinates"), OffsetOf(__shadow__::__ROOT__Math__PositionVector3D_ROOT__Math__Cartesian3D_double__ROOT__Math__DefaultCoordinateSystemTag_, fCoordinates), ::Reflex::PRIVATE);
}
//------Delayed function member builder for class PositionVector3D<ROOT::Math::Cartesian3D<double>,ROOT::Math::DefaultCoordinateSystemTag> -------------------
void __ROOT__Math__PositionVector3D_ROOT__Math__Cartesian3D_double__ROOT__Math__DefaultCoordinateSystemTag__db_funcmem(Reflex::Class*) {

}
namespace {
  struct Dictionaries {
    Dictionaries() {
      Reflex::Instance initialize_reflex;
      __Basic3DVector_float__dict(); 
      __TV_dict(); 
      __PV3DBase_float_VectorTag_GlobalTag__dict(); 
      __std__vector_double__dict(); 
      __ROOT__Math__MatRepSym_double_3__dict(); 
      __ROOT__Math__RowOffsets_3__dict(); 
      __ROOT__Math__SMatrix_double_3_3_ROOT__Math__MatRepSym_double_3_s__dict(); 
      __ROOT__Math__LorentzVector_ROOT__Math__PxPyPzE4D_double_s__dict(); 
      __Vector3DBase_float_GlobalTag__dict(); 
      __ROOT__Math__PositionVector3D_ROOT__Math__Cartesian3D_double__ROOT__Math__DefaultCoordinateSystemTag__dict(); 
    }
    ~Dictionaries() {
      type_663.Unload(); // class Basic3DVector<float> 
      type_1663.Unload(); // class TV 
      type_1707.Unload(); // class PV3DBase<float,VectorTag,GlobalTag> 
      type_1785.Unload(); // class std::vector<double> 
      type_2595.Unload(); // class ROOT::Math::MatRepSym<double,3> 
      type_6586.Unload(); // class ROOT::Math::RowOffsets<3> 
      type_394.Unload(); // class ROOT::Math::SMatrix<double,3,3,ROOT::Math::MatRepSym<double,3> > 
      type_529.Unload(); // class ROOT::Math::LorentzVector<ROOT::Math::PxPyPzE4D<double> > 
      type_888.Unload(); // class Vector3DBase<float,GlobalTag> 
      type_799.Unload(); // class ROOT::Math::PositionVector3D<ROOT::Math::Cartesian3D<double>,ROOT::Math::DefaultCoordinateSystemTag> 
    }
  };
  static Dictionaries instance;
}
} // unnamed namespace
