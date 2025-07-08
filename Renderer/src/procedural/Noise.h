#pragma once

#define IMPLEMENT_CLONE(Base, Type) \
	OwningPtr<Base> Clone() const override         \
	{                               \
		return MakeOwning<Type>(*this);   \
	}

#define COMMA ,

template<typename InType, typename OutType = float>
class NoiseFunction
{
public:
	virtual OutType Evaluate(const InType& coords) = 0;
	virtual NoiseFunction& operator*=(float mul) = 0;
	virtual OwningPtr<NoiseFunction> Clone() const = 0;
};


template<typename InType, typename OutType>
class CompoundNoise : public NoiseFunction<InType, OutType>
{
	using Base = NoiseFunction<InType, OutType>;
	Vector<float> mCoeffs;
	Vector<OwningPtr<Base>> mFunctions;
public:
	OutType Evaluate(const InType& coords) override
	{
		OutType result {};
		for (u32 i = 0; i < mFunctions.size(); ++i)
		{
			result += mCoeffs[i] * mFunctions[i]->Evaluate(coords);
		}
		return result;
	}

	Base& operator*=(float mul) override
	{
		for (float& coeff : mCoeffs)
		{
			coeff *= mul;
		}
		return *this;
	}

	CompoundNoise& operator+=(const Base& other)
	{
	;
		mFunctions.push_back(other.Clone());
		mCoeffs.push_back(1);
		return *this;
	}

	void Add(const Base& noiseFn, OutType coeff)
	{
		mFunctions.push_back(noiseFn.Clone());
		mCoeffs.push_back(coeff);
	}

	OwningPtr<Base> Clone() const override
	{
		auto result = MakeOwning<CompoundNoise>();
		for (int i = 0; i < mFunctions.size(); ++i)
		{
			result->mFunctions.push_back(mFunctions[i]->Clone());
			result->mCoeffs.push_back(mCoeffs[i]);
		}
		return result;
	}
};

template<typename InType, typename OutType>
CompoundNoise<InType, OutType> operator*(OutType mul, const NoiseFunction<InType, OutType>& a)
{
	CompoundNoise<InType, OutType> result;
	result.Add(a, mul);
	return result;
}

template<typename InType, typename OutType>
CompoundNoise<InType, OutType> operator*(const NoiseFunction<InType, OutType>& a, OutType mul)
{
	CompoundNoise<InType, OutType> result;
	result.Add(a, mul);
	return result;
}

template<typename InType, typename OutType>
CompoundNoise<InType, OutType> operator+(const NoiseFunction<InType, OutType>& a, const NoiseFunction<InType, OutType>& b)
{
	CompoundNoise<InType, OutType> result;
	result += a;
	result += b;
	return result;
}

template<typename InType, typename OutType>
CompoundNoise<InType, OutType> operator+(CompoundNoise<InType, OutType> a, const NoiseFunction<InType, OutType>& b)
{
	a += b;
	return a;
}

//template<typename InType, typename OutType>
//CompoundNoise<InType, OutType> operator+(const NoiseFunction<InType, OutType>& a, CompoundNoise<InType, OutType> b)
//{
//	b += a;
//	return a;
//}
