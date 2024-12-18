#pragma once


template<typename T>
struct TypeDestructor
{
	static void Destruct(void* address)
	{
		static_cast<T*>(address)->~T();
	}
};

template<size_t Size = 8, size_t Alignment = 8>
class InplaceObject
{
public:
	template<typename T, typename... Args>
	requires(sizeof(T) <= Size && alignof(T) <= Alignment)
	void Emplace(Args... args)
	{
		DestroyExisting();
		new(&mStorage) std::remove_cvref_t<T>(std::forward<Args>(args)...);
		mDestructor = TypeDestructor<T>::Destruct;
	}

	~InplaceObject()
	{
		DestroyExisting();
	}

private:
	void DestroyExisting()
	{
		if (mDestructor)
		{
			mDestructor(&mStorage);
			memset(this, 0, *sizeof(this));
		}
	}

	typedef void (* Destructor)(void*);
	std::aligned_storage<Size, Alignment> mStorage;
	Destructor mDestructor = nullptr;

};
