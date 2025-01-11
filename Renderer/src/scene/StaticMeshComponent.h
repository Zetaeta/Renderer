#pragma once
#include "SceneComponent.h"
#include "common/SceneDataInterface.h"

class StaticMeshComponent : public SceneComponent
{
	DECLARE_RTTI(StaticMeshComponent, SceneComponent);
public:
	StaticMeshComponent() {}

	StaticMeshComponent(AssetPath const& path);

	template<typename TParent>
	StaticMeshComponent(TParent* parent)
		:SceneComponent(parent)//, m_MeshInst(-1)
	{
	}
	//StaticMeshComponent(SceneObject* parent, MeshInstanceRef minst, String const& name = "", Transform const& trans = Transform{})
	//	: SceneComponent(parent, name, trans), m_MeshInst(minst)
	//{}

	//StaticMeshComponent(SceneComponent* parent, MeshInstanceRef minst, String const& name = "", Transform const& trans = Transform{})
	//	: SceneComponent(parent, name, trans), m_MeshInst(minst)
	//{}

	enum class EType : u8
	{
		VISIBLE = 0,
		GADGET = 1
	};

	void OnInitialize() override;
	void OnDeinitialize() override;

	void SetMesh(CompoundMesh::Ref mesh);
	void SetMesh(AssetPath const& path);

	virtual void	OnUpdate(Scene& scene) override;

	CompoundMesh* GetMesh() const { return m_MeshRef.get(); }
	CompoundMesh::Ref GetMeshRef() const { return m_MeshRef; }

	bool ImGuiControls() override;

	void SetType(EType type)
	{
		m_Type = type;
		MarkDirty();
	}

	PrimitiveId GetPrimitiveId()
	{
		return mPrimId;
	}

	MaterialID GetMaterial(u32 meshPartIdx);

protected:
	EType m_Type = EType::VISIBLE;
	PrimitiveId mPrimId = InvalidPrimId();
	//	MeshInstanceRef m_MeshInst = -1;
	CompoundMesh::Ref m_MeshRef = CompoundMesh::INVALID_REF;
	SmallVector<MaterialID, 4> mMaterialOverrides;
	AssetPath m_Mesh;
};

DECLARE_CLASS_TYPEINFO(StaticMeshComponent)

using PrimitiveComponent = StaticMeshComponent;

