#pragma once

#include <d3d11.h>
#include <memory>

#include "structures.h"
#include "mathematics.h"



namespace skincut
{
	class Camera
	{
	public: // constants
		static const float cFieldOfView;
		static const float cNearPlane;
		static const float cFarPlane;
		static const float cDistanceMin;
		static const float cDistanceMax;
		static const float cPitchMin;
		static const float cPitchMax;


	public:
		math::Vector3 mEye;
		math::Vector3 mTarget;

		math::Matrix mView;
		math::Matrix mProjection;


	private:
		float mYaw;
		float mPitch;
		float mDistance;
		float mPanX, mPanY;
		float mWidth, mHeight;

		// backup parameters for resetting
		float mOrigYaw;
		float mOrigPitch;
		float mOrigDistance;


	public:
		Camera(UINT width, UINT height, float yaw, float pitch, float distance);

		void Update();
		void Resize(UINT width, UINT height);
		void Reset();
	};

}

