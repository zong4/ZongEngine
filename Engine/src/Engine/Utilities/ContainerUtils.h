#pragma once

namespace Engine::Utils {

	template<typename Vector, typename T>
	inline bool Contains(const Vector& v, T&& item) // TODO: before we update to C++20
	{
		return std::find(std::begin(v), std::end(v), std::forward<T>(item)) != std::end(v);
	}

	template<typename Vector, typename T>
	inline bool AppendIfNotPresent(Vector& v, T&& item)
	{
		if (Contains(v, item))
			return false;

		v.push_back(item);
		return true;
	}

	template<typename T, typename ConditionFunction>
	inline bool RemoveIf(std::vector<T>& vector, ConditionFunction condition)
	{
		for (auto it = vector.begin(); it != vector.end(); it++)
		{
			if (condition(*it))
			{
				vector.erase(it);
				return true;
			}
		}

		return false;
	}

	template <typename Vector, typename ItemType>
	inline bool Remove(Vector& v, ItemType&& itemToRemove)
	{
		auto found = std::find(std::begin(v), std::end(v), itemToRemove);

		if (found == std::end(v))
			return false;

		v.erase(found);
		return true;
	}

} // Engine::Utils
