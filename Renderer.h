#pragma once

#include <directxtk/CommonStates.h>
#include <vector>

class Renderer
{
public:
    Renderer(ID3D11Device* device);
    virtual ~Renderer() = default;

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    virtual std::vector<float> Initialize(ID3D11DeviceContext* context) = 0;
    virtual void Render(ID3D11DeviceContext* context) = 0;

    static std::unique_ptr<DirectX::CommonStates> s_CommonStates;

protected:
    virtual void UpdateBuffer(ID3D11DeviceContext* context) = 0;

    ID3D11Device* m_Device = nullptr;

};

