#pragma once

#include "TypeDescriptor.h"

#include <string>
#include <vector>

namespace Hazel {
	namespace Reflection {

		struct Member
		{
			std::string Name;
			size_t Size;		// 0 if function

			std::string TypeName;

			enum EType
			{
				Function,
				Data
			} Type;

			bool operator==(const Member& other) const
			{
				return Name == other.Name
					&& Size == other.Size
					&& TypeName == other.TypeName
					&& Type == other.Type;
			}
		};

		struct ClassInfo
		{
			std::string Name;
			size_t Size;					// sizeof(T)

			std::vector<Member> Members;	// "described" members

			template<class T>
			static ClassInfo Of()
			{
				static_assert(Type::Described<T>::value, "Type must be 'Described'.");

				// Parse info using type Description

				ClassInfo info;
				using Descr = Type::Description<T>;

				info.Name = Descr::ClassName;
				info.Size = sizeof(T);

				for (const auto& memberName : Descr::MemberNames)
				{
					const bool isFunction = *Descr::IsFunctionByName(memberName);
					const auto typeName = *Descr::GetTypeNameByName(memberName);

					info.Members.push_back(
						Member{
							/*.Name*/{ memberName },
							/*.Size*/{ *Descr::GetMemberSizeByName(memberName) * !isFunction },
							/*.TypeName*/{ std::string_view(typeName.data(), typeName.size())},
							/*.Type*/{ isFunction ? Member::Function : Member::Data }
						});
				}

				return info;
			}

			bool operator==(const ClassInfo& other) const
			{
				return Name == other.Name
					&& Size == other.Size
					&& Members == other.Members;
			}
		};
	} // namespace Reflection

	struct TestStruct
	{
		int i = 5;
		float f = 3.2f;
		char ch = 'c';
		int* pi = nullptr;

		void vfunc() {}
		bool bfunc() const { return true; }
	};

	namespace Type {
		DESCRIBED(TestStruct,
					&TestStruct::i,
					&TestStruct::f,
					&TestStruct::ch,
					&TestStruct::pi,
					&TestStruct::vfunc,
					&TestStruct::bfunc
		);
	} // namespace Type

	namespace Reflection {
		bool ClassInfoTest()
		{
			static const ClassInfo cl = ClassInfo::Of<TestStruct>();

			static const ClassInfo ExpectedInfo
			{
				/*.Name =*/ "TestStruct",
				/*.Size =*/ 24,
				/*.Members =*/ std::vector<Member>
				{
					{/*.Name =*/ "i", /*.Size =*/ 4, /*.TypeName =*/ "int", /*.Type =*/ Member::Data },
					{/*.Name =*/ "f", /*.Size =*/ 4, /*.TypeName =*/ "float", /*.Type =*/ Member::Data},
					{/*.Name =*/ "ch", /*.Size =*/ 1, /*.TypeName =*/ "char", /*.Type =*/ Member::Data },
					{/*.Name =*/ "pi", /*.Size =*/ 8, /*.TypeName =*/ "int*", /*.Type =*/ Member::Data},

					{/*.Name =*/ "vfunc", /*.Size =*/ 0, /*.TypeName =*/ "void", /*.Type =*/ Member::EType::Function },
					{/*.Name =*/ "bfunc", /*.Size =*/ 0, /*.TypeName =*/ "bool", /*.Type =*/ Member::EType::Function },
				}
			};

			return (cl == ExpectedInfo);
		}
	} // namespace Reflection

} // namespace Hazel
