#include "ModelRenderer2.h"
#include "ObjectInstance.h"

ModelRenderer2::ModelRenderer2(
	ID3D11Device* device, const std::shared_ptr<GpuConstants>& constants,
	const std::shared_ptr<std::vector<SubmeshInstance>>& subIns,
	const std::shared_ptr<std::vector<ObjectInstance>>& objIns, const std::shared_ptr<const AssetLibrary>& assetLibrary,
	const std::shared_ptr<std::vector<MergedSubmeshInstance>>& mergedSubmeshInstances,
	const std::shared_ptr<std::vector<DividedSubmeshInstance>>& divideSmIns) :
	ModelRenderer1(device, constants, subIns, objIns, assetLibrary, mergedSubmeshInstances),
	m_DividedSubmeshInstances(divideSmIns) {}

void ModelRenderer2::Initialize(ID3D11DeviceContext* context)
{
	ModelRenderer1::Initialize(context);

	const auto& dividedTriList = m_AssetLibrary->GetMergedTriangleListDivide();
	for (const auto& [faceStride, triList] : dividedTriList)
	{
		m_Vt2s[faceStride] = std::make_unique<DirectX::StructuredBuffer<Vertex>>(m_Device, triList.data(),
			triList.size());
	}
}

void ModelRenderer2::Render(ID3D11DeviceContext* context)
{
	context->IASetInputLayout(nullptr);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
	context->VSSetShader(m_Vs.Get(), nullptr, 0);
	context->PSSetShader(m_Ps.Get(), nullptr, 0);

	{
		const auto buffer = m_Vc0->GetBuffer();
		context->VSSetConstantBuffers(0, 1, &buffer);
		ID3D11ShaderResourceView* srv[] = { m_Vt1->GetSrv() };
		context->VSSetShaderResources(1, _countof(srv), srv);
	}

	{
		ID3D11ShaderResourceView* srv[] = { m_Pt0->GetSrv(), m_Pt1->GetSrv() };
		context->PSSetShaderResources(0, _countof(srv), srv);
		const auto sampler = s_CommonStates->AnisotropicWrap();
		context->PSSetSamplers(0, 1, &sampler);
	}
	const auto opaque = s_CommonStates->Opaque();
	context->OMSetBlendState(opaque, nullptr, 0xffffffff);
	const auto depthTest = s_CommonStates->DepthDefault();
	context->OMSetDepthStencilState(depthTest, 0);
	context->RSSetState(s_CommonStates->CullCounterClockwise());

	const auto tmp = m_Constants->VertexPerMesh;
	for (const auto& [faceStride, insBuff] : *m_DividedSubmeshInstances)
	{
		m_Constants->VertexPerMesh = faceStride * 3;
		m_Vc0->SetData(context, *m_Constants);
		m_Vt0->SetData(context, insBuff.data(), insBuff.size());

		ID3D11ShaderResourceView* srv = m_Vt2s[faceStride]->GetSrv();
		context->VSSetShaderResources(2, 1, &srv);
		srv = m_Vt0->GetSrv();
		context->VSSetShaderResources(0, 1, &srv);

		context->DrawInstanced(m_Constants->VertexPerMesh, insBuff.size(), 0, 0);
	}
	m_Constants->VertexPerMesh = tmp;
}

void ModelRenderer2::UpdateBuffer(ID3D11DeviceContext* context)
{
	m_Vt1->SetData(context, m_ObjectInstances->data(), m_ObjectInstances->size());
}
