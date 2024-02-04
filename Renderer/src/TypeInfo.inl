#pragma once

template<bool IsConst>
template<typename T>
ApplyConst<T, IsConst>& TValuePtr<IsConst>::GetAs()
{
	RASSERT(m_Type->Contains(GetTypeInfo<T>()));
	return static_cast<ApplyConst<T, IsConst>&>(*m_Obj);
}
