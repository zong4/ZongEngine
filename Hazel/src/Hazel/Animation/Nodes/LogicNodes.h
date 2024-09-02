#pragma once
#include "Hazel/Animation/NodeProcessor.h"
#include "Hazel/Core/UUID.h"

namespace Hazel::AnimationGraph {

	template<typename T>
	struct CheckEqual : public NodeProcessor
	{
		explicit CheckEqual(const char* dbgName, UUID id);

		void Init() final;

		T* in_Value1 = nullptr; //? this indirection is still not ideal
		T* in_Value2 = nullptr;
		bool out_Out{ 0 };

		float Process(float timestep) final
		{
			out_Out = (*in_Value1 == *in_Value2);
			return timestep;
		}
	};


	template<typename T>
	struct CheckNotEqual : public NodeProcessor
	{
		explicit CheckNotEqual(const char* dbgName, UUID id);

		void Init() final;

		T* in_Value1 = nullptr; //? this indirection is still not ideal
		T* in_Value2 = nullptr;
		bool out_Out{ 0 };

		float Process(float timestep) final
		{
			out_Out = (*in_Value1 != *in_Value2);
			return timestep;
		}
	};


	template<typename T>
	struct CheckLess : public NodeProcessor
	{
		explicit CheckLess(const char* dbgName, UUID id);

		void Init() final;

		T* in_Value1 = nullptr; //? this indirection is still not ideal
		T* in_Value2 = nullptr;
		bool out_Out{ 0 };

		float Process(float timestep) final
		{
			out_Out = (*in_Value1 < *in_Value2);
			return timestep;
		}
	};


	template<typename T>
	struct CheckLessEqual : public NodeProcessor
	{
		explicit CheckLessEqual(const char* dbgName, UUID id);

		void Init() final;

		T* in_Value1 = nullptr; //? this indirection is still not ideal
		T* in_Value2 = nullptr;
		bool out_Out{ 0 };

		float Process(float timestep) final
		{
			out_Out = (*in_Value1 <= *in_Value2);
			return timestep;
		}
	};


	template<typename T>
	struct CheckGreater : public NodeProcessor
	{
		explicit CheckGreater(const char* dbgName, UUID id);

		void Init() final;

		T* in_Value1 = nullptr; //? this indirection is still not ideal
		T* in_Value2 = nullptr;
		bool out_Out{ 0 };

		float Process(float timestep) final
		{
			out_Out = (*in_Value1 > *in_Value2);
			return timestep;
		}
	};


	template<typename T>
	struct CheckGreaterEqual : public NodeProcessor
	{
		explicit CheckGreaterEqual(const char* dbgName, UUID id);

		void Init() final;

		T* in_Value1 = nullptr; //? this indirection is still not ideal
		T* in_Value2 = nullptr;
		bool out_Out{ 0 };

		float Process(float timestep) final
		{
			out_Out = (*in_Value1 >= *in_Value2);
			return timestep;
		}
	};


	struct And : public NodeProcessor
	{
		explicit And(const char* dbgName, UUID id);

		void Init() final;

		bool* in_Value1 = nullptr; //? this indirection is still not ideal
		bool* in_Value2 = nullptr;
		bool out_Out{ 0 };

		float Process(float timestep) final
		{
			out_Out = *in_Value1 && *in_Value2;
			return timestep;
		}
	};


	struct Or : public NodeProcessor
	{
		explicit Or(const char* dbgName, UUID id);

		void Init() final;

		bool* in_Value1 = nullptr; //? this indirection is still not ideal
		bool* in_Value2 = nullptr;
		bool out_Out{ 0 };

		float Process(float timestep) final
		{
			out_Out = *in_Value1 || *in_Value2;
			return timestep;
		}
	};


	struct Not : public NodeProcessor
	{
		explicit Not(const char* dbgName, UUID id);

		void Init() final;

		bool* in_Value = nullptr; //? this indirection is still not ideal
		bool out_Out{ 0 };

		float Process(float timestep) final
		{
			out_Out = !*in_Value;
			return timestep;
		}
	};

} //Hazel::AnimationGraph
