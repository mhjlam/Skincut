#pragma once

#include <map>
#include <array>
#include <memory>
#include <d3d11.h>
#include <wrl/client.h>

#include "mathematics.h"


using Microsoft::WRL::ComPtr;



namespace skincut
{
	class FrameBuffer;

	struct LightLoadInfo
	{
		float yaw;
		float fovy;
		float pitch;
		float distance;
		math::Color color;
	};


	class Light
	{
	private:
		LightLoadInfo mLoadInfo;
		ComPtr<ID3D11Device> mDevice;
		ComPtr<ID3D11DeviceContext> mContext;
		
	public:
		float mYaw;
		float mFovy;
		float mPitch;
		float mDistance;
		float mFarPlane;
		float mNearPlane;
		float mAttenuation;
		float mFieldOfView;
		float mFalloffStart;
		float mFalloffWidth;

		float mBrightness;
		float mBrightnessPrev;

		std::string mName;
		math::Color mColor;
		math::Vector3 mPosition;
		math::Vector3 mDirection;

		math::Matrix mView;
		math::Matrix mProjection;
		math::Matrix mViewProjection;
		math::Matrix mViewProjectionLinear;

		std::shared_ptr<FrameBuffer> mShadowMap;


	public:
		Light(ComPtr<ID3D11Device>& device, 
			  ComPtr<ID3D11DeviceContext>& context, 
			  float yaw, float pitch, float distance, 
			  math::Color color = math::Color(0.5f, 0.5f, 0.5f), 
			  std::string name = "Light", float fovy = 45.0f, UINT shadowsize = 2048);

		void Update();
		void Reset();


	private:
		void SetViewProjection();
	};
}

