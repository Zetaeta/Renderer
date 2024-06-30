#pragma once

template<bool IsConst>
template<typename T>
ApplyConst<T, IsConst>& TReflectedValue<IsConst>::GetAs() const
{
	RASSERT(m_Type->Contains(GetTypeInfo<T>()));
	return *static_cast<ApplyConst<T, IsConst>*>(m_Obj);
}


