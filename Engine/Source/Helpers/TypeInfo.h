
#pragma once

namespace RR
{
    struct TypeInfo
    {
        const char*     name = nullptr;
        const TypeInfo* base = nullptr;
    };
}


#define GAMEOBJECT(ClassName, BaseClass)                                  \
    static const RR::TypeInfo& StaticType()                               \
    {                                                                     \
        static const RR::TypeInfo info{ #ClassName,                       \
                                        &BaseClass::StaticType() };       \
        return info;                                                      \
    }                                                                     \
    const RR::TypeInfo& GetType() const override { return StaticType(); }


#define SUBSYSTEM(ClassName, BaseClass)                                   \
    static const RR::TypeInfo& StaticType()                               \
    {                                                                     \
        static const RR::TypeInfo info{ #ClassName,                       \
                                        &BaseClass::StaticType() };       \
        return info;                                                      \
    }                                                                     \
    const RR::TypeInfo& GetType() const override { return StaticType(); }


#define COMPONENT(ClassName, BaseClass)                                   \
    static const RR::TypeInfo& StaticType()                               \
    {                                                                     \
        static const RR::TypeInfo info{ #ClassName,                       \
                                        &BaseClass::StaticType() };       \
        return info;                                                      \
    }                                                                     \
    const RR::TypeInfo& GetType() const override { return StaticType(); }




