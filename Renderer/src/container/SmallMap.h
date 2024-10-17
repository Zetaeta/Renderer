#pragma once
#include <unordered_map>

template<typename Key, typename Value>
class SmallMap : public std::unordered_map<Key, Value>
{
//public:
//	FlatMap(size_t size = 0)
//		: mKeys(size), mValues(size) {}
//
//
//	template<bool IsConst>
//	class iterator
//	{
//		bool operator==(const iterator& other)
//		{
//			return mIndex == other.mIndex;
//		}
//
//		std::pair<TApplyConst<Key, IsConst>&, TApplyConst<Value, IsConst>&> operator*()
//		{
//			return
//		}
//
//		FlatMap* map;
//		size_t mIndex;
//	};
//
//private:
//	std::vector<Key> mKeys;
//	std::vector<Value> mValues;
};

