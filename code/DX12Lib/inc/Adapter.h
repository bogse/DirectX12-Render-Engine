#pragma once

#include <wrl/client.h>

#include <memory>

class Adapter
{
public:
	/**
	* Create a GPU adapter.
	* 
	* @param gpuPreference The GPU preference.
	* By default, a high-performace GPU is preferred.
	* @param useWarp If true, create a WARP adapter.
	* 
	* @returns A shared pointer to a GPU adapter or 
	* nullptr if the adapter could not be created.
	*/
	static std::shared_ptr<Adapter> Create(
		DXGI_GPU_PREFERENCE gpuPreference = DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
		bool useWarp = false);

	/**
	* Get the IDXGIAdapter.
	*/
	Microsoft::WRL::ComPtr<IDXGIAdapter> GetDXGIAdapter() const
	{
		return m_dxgiAdapter;
	}

	/**
	* Get the description of the adapter.
	*/
	const std::wstring GetDescription() const
	{
		return m_Desc.Description;
	}

protected:
	Adapter(Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgiAdapter);
	virtual ~Adapter() = default;

private:
	Microsoft::WRL::ComPtr<IDXGIAdapter4> m_dxgiAdapter;
	DXGI_ADAPTER_DESC3					  m_Desc;
};