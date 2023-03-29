#pragma once

#include <d3d11.h>
#include <exception>
#include <cstdio>
#include <d3dcompiler.h>
#include <fstream>

namespace DirectX
{
    // Helper class for COM exceptions
    class com_exception : public std::exception
    {
    public:
        com_exception(HRESULT hr) : result(hr) {}

        [[nodiscard]] const char* what() const noexcept override
        {
            static char s_str[64] = {};
            sprintf_s(s_str, "Failure with HRESULT of %08X",
                static_cast<unsigned int>(result));
            return s_str;
        }

    private:
        HRESULT result;
    };

    // Helper utility converts D3D API failures into exceptions.
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            throw com_exception(hr);
        }
    }

    constexpr UINT CalculateBufferSize(UINT byteSize)
    {
	    return (byteSize + 255) & ~255;
    }

    inline Microsoft::WRL::ComPtr<ID3DBlob> LoadBinary(const std::wstring& filename)
    {
	    std::ifstream fin(filename, std::ios::binary);

	    fin.seekg(0, std::ios_base::end);
	    const std::ifstream::pos_type size = fin.tellg();
	    fin.seekg(0, std::ios_base::beg);

	    Microsoft::WRL::ComPtr<ID3DBlob> blob;
	    D3DCreateBlob(size, blob.GetAddressOf());

	    fin.read(static_cast<char*>(blob->GetBufferPointer()), size);
	    fin.close();

	    return blob;
    }
}
