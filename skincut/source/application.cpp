#include "application.h"

#include <io.h>
#include <math.h>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include "ImGui/imgui.h"
#include "SimpleJSON/JSON.h"

#include "light.h"
#include "model.h"
#include "camera.h"
#include "shader.h"
#include "target.h"
#include "utility.h"
#include "renderer.h"
#include "dashboard.h"
#include "generator.h"
#include "stopwatch.h"


#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")


using namespace skincut;
using namespace skincut::math;



namespace skincut
{
	Configuration gConfig;
}


#define X(a, b) b,
wchar_t* gPickName[] = { PICK_TABLE };
#undef X

#define X(a, b) b,
wchar_t* gSplitName[] = { SPLIT_TABLE };
#undef X

#define X(a, b) b,
wchar_t* gRenderName[] = { RENDER_TABLE };
#undef X


// number of runs for performance test
#define TEST_NUMRUNS 100


Application::Application()
{
	mHwnd = nullptr;
	_setmode(_fileno(stdout), _O_U16TEXT);
}


Application::~Application()
{
}


bool Application::Initialize(HWND hWnd, const std::string& respath)
{
	mHwnd = hWnd;
	if (!mHwnd) return false;
	gConfig.ResourcePath = respath;

	Stopwatch sw("init", CLOCK_QPC_MS);
	
	if (!LoadConfig()) return false;
	if (!SetupRenderer()) return false;
	if (!LoadScene()) return false;
	if (!SetupDashboard()) return false;

	sw.Stop("init");
	std::stringstream ss;
	ss << "Initialization done (took " << sw.ElapsedTime("init") << " ms)" << std::endl;
	Utility::ConsoleMessage(ss.str());

	return true;
}


bool Application::LoadConfig()
{
// 	Utility::ConsoleMessage("Loading settings...");

	std::string configfile = gConfig.ResourcePath + std::string("config.json");

	std::string contents;
	std::ifstream in(configfile, std::ios::in);
	if (!in) return false;

	in.seekg(0, std::ios::end);
	contents.resize(static_cast<unsigned int>(in.tellg()));
	in.seekg(0, std::ios::beg);
	in.read(&contents[0], contents.size());
	in.close();

	auto data = std::unique_ptr<JSONValue>(JSON::Parse(contents.c_str()));
	if (!data || !data->IsObject()) return false;
	JSONObject root = data->AsObject();

	gConfig.EnableWireframe = root.at(L"bWireframe")->AsBool();
	gConfig.EnableDashboard = root.at(L"bDashboard")->AsBool();

	gConfig.EnableColor = root.at(L"bColor")->AsBool();
	gConfig.EnableBumps = root.at(L"bBumps")->AsBool();
	gConfig.EnableShadows = root.at(L"bShadows")->AsBool();
	gConfig.EnableSpeculars = root.at(L"bSpeculars")->AsBool();
	gConfig.EnableOcclusion = root.at(L"bOcclusion")->AsBool();
	gConfig.EnableIrradiance = root.at(L"bIrradiance")->AsBool();
	gConfig.EnableScattering = root.at(L"bScattering")->AsBool();

	gConfig.Ambient = (float)root.at(L"fAmbient")->AsNumber();
	gConfig.Fresnel = (float)root.at(L"fFresnel")->AsNumber();
	gConfig.Roughness = (float)root.at(L"fRoughness")->AsNumber();
	gConfig.Bumpiness = (float)root.at(L"fBumpiness")->AsNumber();
	gConfig.Specularity = (float)root.at(L"fSpecularity")->AsNumber();
	gConfig.Scattering = (float)root.at(L"fScattering")->AsNumber();
	gConfig.Translucency = (float)root.at(L"fTranslucency")->AsNumber();

	std::wstring pickmode = root.at(L"sPick")->AsString();
	if      (Utility::CompareString(pickmode, L"draw")) gConfig.PickMode = PICK_PAINT;
	else if (Utility::CompareString(pickmode, L"fuse")) gConfig.PickMode = PICK_MERGE;
	else if (Utility::CompareString(pickmode, L"carve")) gConfig.PickMode = PICK_CARVE;
	else gConfig.PickMode = PICK_CARVE;

	std::wstring splitmode = root.at(L"sSplit")->AsString();
	if      (Utility::CompareString(splitmode, L"3split")) gConfig.SplitMode = SPLIT_3;
	else if (Utility::CompareString(splitmode, L"4split")) gConfig.SplitMode = SPLIT_4;
	else if (Utility::CompareString(splitmode, L"6split")) gConfig.SplitMode = SPLIT_6;
	else gConfig.SplitMode = SPLIT_3;
	
	std::wstring rendermode = root.at(L"sRenderer")->AsString();
	if      (Utility::CompareString(rendermode, L"kelemen")) gConfig.RenderMode = (int)RENDER_KELEMEN;
	else if (Utility::CompareString(rendermode, L"phong")) gConfig.RenderMode = (int)RENDER_PHONG;
	else if (Utility::CompareString(rendermode, L"lambert")) gConfig.RenderMode = (int)RENDER_LAMBERT;
	else gConfig.RenderMode = RENDER_KELEMEN;

	return true;
}


bool Application::LoadScene()
{
	std::string scenefile = gConfig.ResourcePath + std::string("scene.json");

	RECT rect; GetClientRect(mHwnd, &rect);
	UINT width = UINT(rect.right - rect.left);
	UINT height = UINT(rect.bottom - rect.top);

	std::string contents;
	std::ifstream in(scenefile, std::ios::in);
	if (!in) return false;

	in.seekg(0, std::ios::end);
	contents.resize(static_cast<unsigned int>(in.tellg()));
	in.seekg(0, std::ios::beg);
	in.read(&contents[0], contents.size());
	in.close();

	auto jdata = std::unique_ptr<JSONValue>(JSON::Parse(contents.c_str()));
	if (!jdata || !jdata->IsObject()) return false;
	auto jcamera = jdata->AsObject().at(L"camera")->AsObject();
	auto jlights = jdata->AsObject().at(L"lights")->AsArray();
	auto jmodels = jdata->AsObject().at(L"models")->AsArray();


	// camera
	float cy = (float)jcamera.at(L"position")->AsArray().at(0)->AsNumber();
	float cp = (float)jcamera.at(L"position")->AsArray().at(1)->AsNumber();
	float cd = (float)jcamera.at(L"position")->AsArray().at(2)->AsNumber();
	mCamera = std::make_unique<Camera>(width, height, cy, cp, cd);

	// lights
	for (auto jlight : jlights)
	{
		std::string name = Utility::WideStringToString(jlight->AsObject().at(L"name")->AsString());

		float y = (float)jlight->AsObject().at(L"position")->AsArray().at(0)->AsNumber();
		float p = (float)jlight->AsObject().at(L"position")->AsArray().at(1)->AsNumber();
		float d = (float)jlight->AsObject().at(L"position")->AsArray().at(2)->AsNumber();

		float r = (float)jlight->AsObject().at(L"color")->AsArray().at(0)->AsNumber();
		float g = (float)jlight->AsObject().at(L"color")->AsArray().at(1)->AsNumber();
		float b = (float)jlight->AsObject().at(L"color")->AsArray().at(2)->AsNumber();

		mLights.push_back(std::make_shared<Light>(mDevice, mContext, y, p, d, Color(r,g,b), name));
	}

	// models
	for (auto jmodel : jmodels)
	{
		float x = (float)jmodel->AsObject().at(L"position")->AsArray().at(0)->AsNumber();
		float y = (float)jmodel->AsObject().at(L"position")->AsArray().at(1)->AsNumber();
		float z = (float)jmodel->AsObject().at(L"position")->AsArray().at(2)->AsNumber();

		float rx = (float)jmodel->AsObject().at(L"rotation")->AsArray().at(0)->AsNumber();
		float ry = (float)jmodel->AsObject().at(L"rotation")->AsArray().at(1)->AsNumber();

		std::string name = Utility::WideStringToString(jmodel->AsObject().at(L"name")->AsString());

		std::wstring respath = Utility::StringToWideString(gConfig.ResourcePath);
		std::wstring meshpath = respath + jmodel->AsObject().at(L"mesh")->AsString();
		std::wstring colorpath = respath + jmodel->AsObject().at(L"color")->AsString();
		std::wstring normalpath = respath + jmodel->AsObject().at(L"normal")->AsString();
		std::wstring specularpath = respath + jmodel->AsObject().at(L"specular")->AsString();
		std::wstring discolorpath = respath + jmodel->AsObject().at(L"discolor")->AsString();
		std::wstring occlusionpath = respath + jmodel->AsObject().at(L"occlusion")->AsString();

		mModels.push_back(std::make_shared<Model>(mDevice, Vector3(x,y,z), Vector2(rx, ry), 
			meshpath, colorpath, normalpath, specularpath, discolorpath, occlusionpath));
	}

	return true;
}


bool Application::SetupRenderer()
{
	RECT rect; GetClientRect(mHwnd, &rect);
	UINT width = UINT(rect.right - rect.left);
	UINT height = UINT(rect.bottom - rect.top);

	mRenderer = std::make_unique<Renderer>(mHwnd, width, height);

	mDevice = mRenderer->mDevice;
	mContext = mRenderer->mContext;
	mSwapChain = mRenderer->mSwapChain;

	mGenerator = std::make_unique<Generator>(mDevice, mContext);

	return true;
}


bool Application::SetupDashboard()
{
	std::wstring respath = Utility::StringToWideString(gConfig.ResourcePath);
	std::wstring fontfile = respath + L"fonts\\arial12.spritefont";
	
	mDashboard = std::make_unique<Dashboard>(mHwnd, mDevice, mContext);

	// Sprite batch and sprite font enable simple text rendering
	mSpriteBatch = std::make_unique<SpriteBatch>(mContext.Get());
	mSpriteFont = std::make_unique<SpriteFont>(mDevice.Get(), fontfile.c_str());

	return true;
}




bool Application::Update()
{
	if (!mRenderer) 
		throw std::exception("Renderer was not initialized properly");

	ImGuiIO& io = ImGui::GetIO();

	if (!io.KeyCtrl && !io.KeyShift && !mPointA && !mPointB && !io.WantCaptureMouse && !io.WantCaptureKeyboard)
		mCamera->Update();

	for (auto& light : mLights) light->Update();
	for (auto& model : mModels) model->Update(mCamera->mView, mCamera->mProjection);

	mDashboard->Update();

	return true;
}


bool Application::Render()
{
	if (!mRenderer) throw std::exception("Renderer was not initialized properly");

	// render scene
	mRenderer->Render(mModels, mLights, mCamera);

	// render user interface
	if (gConfig.EnableDashboard)
	{
		std::vector<Light*> lights;
		for (auto light : mLights)
			lights.push_back(light.get());


		// render dashboard (user interface panels)
		mDashboard->Render(lights);

		// render text
		RECT rect; GetClientRect(mHwnd, &rect);
		uint32_t width = rect.right - rect.left;
		uint32_t height = rect.bottom - rect.top;

		std::wstring picktext = std::wstring(L"ick mode: ") + gPickName[gConfig.PickMode];
		std::wstring splittext = std::wstring(L"plit mode: ") + gSplitName[gConfig.SplitMode];

		DirectX::XMFLOAT2 ptv, stv;
		DirectX::XMStoreFloat2(&ptv, mSpriteFont->MeasureString(picktext.c_str()));
		DirectX::XMStoreFloat2(&stv, mSpriteFont->MeasureString(splittext.c_str()));

		mSpriteBatch->Begin();
		{
			mSpriteFont->DrawString(mSpriteBatch.get(), L"P", Vector2(float(width - ptv.x - 22), float(height - 44)), DirectX::Colors::Orange);
			mSpriteFont->DrawString(mSpriteBatch.get(), picktext.c_str(), Vector2(float(width - ptv.x - 11), float(height - 44)), DirectX::Colors::LightGray);

			mSpriteFont->DrawString(mSpriteBatch.get(), L"S", Vector2(float(width - stv.x - 22), float(height - 22)), DirectX::Colors::Orange);
			mSpriteFont->DrawString(mSpriteBatch.get(), splittext.c_str(), Vector2(float(width - stv.x - 11), float(height - 22)), DirectX::Colors::LightGray);
		}
		mSpriteBatch->End();
	}

	// Present rendered image on screen.
	HREXCEPT(mRenderer->mSwapChain->Present(0, 0));
	
	return true;
}


bool Application::Reload()
{
	mCamera->Reset();

	for (auto light : mLights)
		light->Reset();

	for (auto model : mModels)
		model->Reload();

	return true;
}



void Application::Pick()
{
	RECT rect;
	GetClientRect(mHwnd, &rect);
	ImGuiIO& io = ImGui::GetIO();

	// Acquire viewport dimensions, cursor position, window dimensions
	Vector2 cursor(io.MousePos.x, io.MousePos.y); // in window coordinates
	Vector2 resolution((float)mRenderer->mWidth, (float)mRenderer->mHeight);
	Vector2 window((float)rect.right - (float)rect.left - 1, (float)rect.bottom - (float)rect.top - 1);

	// Find closest ray-mesh intersection
	Intersection intersection = FindIntersection(cursor, resolution, window, mCamera->mProjection, mCamera->mView);
	if (!intersection.hit) throw std::exception("No intersection");

	// Keep track of number of points selected
	if      (!mPointA) mPointA = std::make_unique<Intersection>(intersection);
	else if (!mPointB) mPointB = std::make_unique<Intersection>(intersection);

	// Create new cut when two points were selected
	if (mPointA && mPointB)
	{
		CreateCut(*mPointA.get(), *mPointB.get());
		mPointA.reset();
		mPointB.reset();
	}
}


void Application::CreateCut(Intersection& ia, Intersection& ib)
{
	if (!ia.model || !ib.model || ia.model.get() != ib.model.get())
	{
		Utility::DialogMessage("Invalid selection");
		return;
	}

	Stopwatch sw(CLOCK_QPC_MS);
	auto model = ia.model;
	Quadrilateral cutquad;
	std::list<Link> cutline;
	std::vector<Edge*> cutedges;
	std::shared_ptr<Target> patch;

	// Find all triangles intersected by the cutting quad, and order them into a chain of segments
	sw.Start("1] Form cutting line");
	model->FormCutline(ia, ib, cutline, cutquad);
	sw.Stop("1] Form cutting line");

	// Generate wound patch texture (must be done first because mesh elements will be deleted later)
	sw.Start("2] Generate wound patch");
	CreateWound(cutline, model, patch);
	sw.Stop("2] Generate wound patch");

	// Paint wound patch onto mesh color texture
	sw.Start("3] Paint wound patch");
	PaintWound(cutline, model, patch);
	sw.Stop("3] Paint wound patch");

	// Only draw wound texture
	if (gConfig.PickMode == PICK_PAINT)
		return;

	// Fuse cutting line into mesh
	if (gConfig.PickMode >= PICK_MERGE)
	{
		sw.Start("4] Fuse cutting line");
		model->FuseCutline(cutline, cutedges);
		sw.Stop("4] Fuse cutting line");
	}

	// Open carve cutting line into mesh
	if (gConfig.PickMode == PICK_CARVE)
	{
		sw.Start("5] Carve incision");
		model->OpenCutline(cutedges, cutquad);
		sw.Stop("5] Carve incision");
	}

#ifdef _DEBUG
	sw.Report();
#endif
}


void Application::Split()
{
	RECT rect;
	GetClientRect(mHwnd, &rect);
	ImGuiIO& io = ImGui::GetIO();

	Vector2 cursor = Vector2(io.MousePos.x, io.MousePos.y);
	Vector2 resolution((float)mRenderer->mWidth, (float)mRenderer->mHeight);
	Vector2 window = Vector2((float)rect.right - (float)rect.left - 1, (float)rect.bottom - (float)rect.top - 1);

	Intersection intersection = FindIntersection(cursor, resolution, window, mCamera->mProjection, mCamera->mView);
	if (!intersection.hit) throw std::exception("No intersection");
	intersection.model->Subdivide(intersection.face, gConfig.SplitMode, intersection.pos_os);
}


void Application::DrawDecal()
{
	RECT rect;
	GetClientRect(mHwnd, &rect);
	ImGuiIO& io = ImGui::GetIO();

	Vector2 cursor(io.MousePos.x, io.MousePos.y);
	Vector2 resolution((float)mRenderer->mWidth, (float)mRenderer->mHeight);
	Vector2 window((float)rect.right - (float)rect.left - 1, (float)rect.bottom - (float)rect.top - 1);

	Intersection intersection = FindIntersection(cursor, resolution, window, mCamera->mProjection, mCamera->mView);
	if (!intersection.hit) throw std::exception("No intersection");
	mRenderer->CreateWoundDecal(intersection);
}


Intersection Application::FindIntersection(Vector2 cursor, Vector2 resolution, Vector2 window, Matrix proj, Matrix view)
{
	// convert screen-space position into viewport space
	Vector2 screenpos = Vector2((cursor.x * resolution.x) / window.x, (cursor.y * resolution.y) / window.y);

	// Create an object-space ray from the given screen-space position.
	Ray ray = CreateRay(screenpos, resolution, proj, view);


	Intersection intersection;
	intersection.hit = false;
	intersection.ray = ray;
	intersection.model = nullptr;
	intersection.pos_ss = screenpos;
	intersection.nearz = Camera::cNearPlane;
	intersection.farz = Camera::cFarPlane;

	// find closest model that intersects with ray
	float tmin = std::numeric_limits<float>::max();

	for (auto model : mModels) 
	{
		if (model->RayIntersection(ray, intersection))
		{
			intersection.hit = true;
			if (intersection.dist < tmin) 
				intersection.model = model;
		}
	}

	if (intersection.hit)
	{
		intersection.pos_ws = Vector3::Transform(intersection.pos_os, intersection.model->mMatrixWorld);
	}

	return intersection;
}


void Application::CreateWound(std::list<Link>& cutline, std::shared_ptr<Model>& model, std::shared_ptr<Target>& patch)
{
	// determine height/width of color map
	D3D11_TEXTURE2D_DESC colorDesc;
	ComPtr<ID3D11Texture2D> colorTex;
	Utility::GetTexture2D(model->mColorMap, colorTex, colorDesc);
	float texWidth = (float)colorDesc.Width;
	float texHeight = (float)colorDesc.Height;

	// convert y range from [1,0] to [0,1]
	Vector2 p0 = Vector2(cutline.front().x0.x, 1.0f - cutline.front().x0.y);
	Vector2 p1 = Vector2(cutline.back().x1.x, 1.0f - cutline.back().x1.y);

	// target texture width/height in pixels
	UINT pixelWidth = UINT(Vector2::Distance(p0, p1) * texWidth);
	UINT pixelHeight = UINT(2.0f * std::log10f((float)pixelWidth) * std::sqrtf((float)pixelWidth));

	// generate wound patch
	patch = mGenerator->GenerateWoundPatch(pixelWidth, pixelHeight);
}


void Application::PaintWound(std::list<Link>& cutline, std::shared_ptr<Model>& model, std::shared_ptr<Target>& patch)
{
	// Compute cut length and height of wound patch in texture-space
	float cutlength = 0;
	for (auto link : cutline)
		cutlength += Vector2::Distance(link.x0, link.x1);

	// height of cut is based on ratio of pixel height to pixel width of the wound patch texture
	float cutheight = cutlength * patch->mViewport.Height / patch->mViewport.Width;

	// Find faces closest to each line segment
	LinkFaceMap cf;
	model->ChainFaces(cutline, cf, cutheight);

	// Paint to color and discolor maps
	mRenderer->PaintWoundPatch(model, patch, cf, cutlength, cutheight);
	mRenderer->PaintDiscoloration(model, cf, cutheight);
}



LRESULT CALLBACK Application::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ImGuiIO& io = ImGui::GetIO();

	try
	{
		switch (msg)
		{
			case WM_CLOSE:
			{
				DestroyWindow(hWnd);
				break;
			}

			case WM_DESTROY:
			{
				PostQuitMessage(WM_QUIT);
				break;
			}

			case WM_KILLFOCUS: // focus lost
			{
				io.KeyCtrl = false;
				io.KeyShift = false;
				io.MouseDown[0] = false;
				io.MouseDown[1] = false;
				io.MouseDown[2] = false;
				io.MouseDown[3] = false;
				io.MouseDown[4] = false;
				break;
			}

			case WM_SETFOCUS: // focus (re)gained
			{
				break;
			}

			case WM_SIZE:
			{
				if (mDevice && wParam != SIZE_MINIMIZED)
				{
					UINT width = (UINT)LOWORD(lParam);
					UINT height = (UINT)HIWORD(lParam);

					mCamera->Resize(width, height);
					mRenderer->Resize(width, height);
				}

				break;
			}

			case WM_KEYDOWN:
			{
				if (wParam < 256)
					io.KeysDown[wParam] = 1;

				switch (wParam)
				{
					case VK_ESCAPE:
					{
						DestroyWindow(hWnd);
						break;
					}
					case VK_CONTROL:
					{
						io.KeyCtrl = true;
						break;
					}
					case VK_SHIFT:
					{
						io.KeyShift = true;
						break;
					}
				}

				break;
			}

			case WM_KEYUP:
			{
				if (wParam < 256)
					io.KeysDown[wParam] = 0;

				if (wParam == VK_F1)
					gConfig.EnableDashboard = !gConfig.EnableDashboard;

				if (wParam == VK_CONTROL)
					io.KeyCtrl = false;

				if (wParam == VK_SHIFT)
					io.KeyShift = false;

				if (io.WantCaptureKeyboard)
					break;

				switch (wParam)
				{
					case 'P': // cycle through pick modes
					{
						if (gConfig.PickMode == (PICKMODE)PICK_CARVE) gConfig.PickMode = PICK_PAINT;
						else gConfig.PickMode = PICKMODE((int)gConfig.PickMode + 1);
						mPointA.release();
						mPointB.release();
						break;
					}

					case 'S': // cycle through split modes
					{
						if (gConfig.SplitMode == (SPLITMODE)SPLIT_6) gConfig.SplitMode = SPLIT_3;
						else gConfig.SplitMode = SPLITMODE((int)gConfig.SplitMode + 1);
						mPointA.release();
						mPointB.release();
						break;
					}
					
					case 'R': // reload scene
					{
						Reload();
						break;
					}

					case 'W': // toggle wireframe mode
					{
						gConfig.EnableWireframe = !gConfig.EnableWireframe;
						break;
					}

					case 'T': // test performance
					{
						PerformanceTest();
						break;
					}
				}

				break;
			}

			case WM_LBUTTONDOWN:
			{
				io.MouseDown[0] = true;
				if (io.WantCaptureMouse) break;
				break;
			}

			case WM_LBUTTONUP:
			{
				io.MouseDown[0] = false;
				if (io.WantCaptureMouse) break;

				if (io.KeyShift) Pick();
				if (io.KeyCtrl) Split();
				//if (io.KeyAlt) Draw();

				break;
			}

			case WM_RBUTTONDOWN:
			{
				io.MouseDown[1] = true;
				break;
			}

			case WM_RBUTTONUP:
			{
				io.MouseDown[1] = false;
				break;
			}

			case WM_MBUTTONDOWN:
			{
				io.MouseDown[2] = true;
				break;
			}

			case WM_MBUTTONUP:
			{
				io.MouseDown[2] = false;
				break;
			}

			case WM_MOUSEMOVE:
			{
				io.MousePos.x = (signed short)(lParam);
				io.MousePos.y = (signed short)(lParam >> 16);
				break;
			}

			case WM_MOUSEWHEEL:
			{
				io.MouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f;
				break;
			}
		}
	}
	catch (std::exception& e) // error handling (reload model: yes|ignore|exit)
	{
		int mbid = Utility::ErrorMessage(e);

		if (mbid == IDYES) Reload();
		if (mbid == IDCANCEL) PostQuitMessage(WM_QUIT);

		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}


void Application::PerformanceTest()
{
	RECT rect; GetClientRect(mHwnd, &rect);
	Vector2 resolution(float(mRenderer->mWidth), float(mRenderer->mHeight));
	Vector2 window(float(rect.right) - float(rect.left - 1), float(rect.bottom) - float(rect.top - 1));

	Matrix proj(3.047189f, 0.00000000f, 0.000000000f, 0.0f, 
	            0.000000f, 5.67128229f, 0.000000000f, 0.0f, 
	            0.000000f, 0.00000000f, 1.005025150f, 1.0f, 
	            0.000000f, 0.00000000f,-0.100502513f, 0.0f);

	Matrix view(1.0f, 0.0f, 0.0f, 0.0f, 
	            0.0f, 1.0f, 0.0f, 0.0f, 
	            0.0f, 0.0f, 1.0f, 0.0f, 
	            0.0f, 0.0f, 5.0f, 1.0f);

	// screen-space locations and normalized direction vectors based on manual samples
	std::vector<std::pair<Vector2, Vector2>> sample_locations =
	{
		std::make_pair(Vector2(638.0f, 175.0f), Vector2( 0.999650240f, 0.026445773f)),
		std::make_pair(Vector2(754.0f, 342.0f), Vector2(-0.138322249f, 0.990387261f)),
		std::make_pair(Vector2(618.0f, 618.0f), Vector2( 0.899437010f, 0.437050372f)),
		std::make_pair(Vector2(692.0f, 375.0f), Vector2( 0.474099845f, 0.880471110f)),
		std::make_pair(Vector2(582.0f, 346.0f), Vector2(-0.651344180f, 0.758782387f)),
		std::make_pair(Vector2(631.0f, 467.0f), Vector2( 0.978677809f, 0.205401510f))
	};


	auto create_samples = [&](std::vector<std::pair<Vector2, Vector2>>& locations, std::vector<float>& lengths, std::wstring setname)
	{
		std::vector<std::tuple<std::wstring, Vector2, Vector2>> samples;
		if (lengths.size() != locations.size()) return samples;

		for (uint32_t i = 0; i < locations.size(); ++i)
		{
			float hlength = 0.5f * lengths[i];

			Vector2 p = locations[i].first;		// position
			Vector2 d = locations[i].second;	// direction

			Vector2 p0 = p - d * hlength;
			Vector2 p1 = p + d * hlength;

			std::wstringstream wss;
			wss << setname << " " << i+1;

			samples.push_back(std::make_tuple(wss.str(), p0, p1));
		}

		return samples;
	};


	auto run_test = [&](std::vector<std::tuple<std::wstring, Vector2, Vector2>>& samples)
	{
		for (auto s : samples)
		{
			std::array<long long, 5> stage_time = {}; // init values to zero
			
			for (uint32_t ri = 0; ri < (uint32_t)TEST_NUMRUNS; ++ri)
			{
				Stopwatch sw;

				Intersection ia = FindIntersection(std::get<1>(s), resolution, window, proj, view);
				Intersection ib = FindIntersection(std::get<2>(s), resolution, window, proj, view);

				Quadrilateral cutquad;
				std::list<Link> cutline;
				std::vector<Edge*> cutedges;
				std::shared_ptr<Target> patch;

				sw.Start("1");
				ia.model->FormCutline(ia, ib, cutline, cutquad);
				sw.Stop("1");

				sw.Start("2");
				CreateWound(cutline, ia.model, patch);
				sw.Stop("2");

				sw.Start("3");
				PaintWound(cutline, ia.model, patch);
				sw.Stop("3");

				sw.Start("4");
				ia.model->FuseCutline(cutline, cutedges);
				sw.Stop("4");

				sw.Start("5");
				ia.model->OpenCutline(cutedges, cutquad);
				sw.Stop("5");

				stage_time[0] += sw.ElapsedTime("1");
				stage_time[1] += sw.ElapsedTime("2");
				stage_time[2] += sw.ElapsedTime("3");
				stage_time[3] += sw.ElapsedTime("4");
				stage_time[4] += sw.ElapsedTime("5");

				ia.model->Reload();
			}


			std::wstringstream wss;
			wss << std::get<0>(s) << std::endl;

			double total_time = 0.0;
			for (uint32_t ti = 0; ti < stage_time.size(); ti++)
			{
				double stage_time_average = stage_time[ti] / (double)TEST_NUMRUNS / 1000.0;
				wss << stage_time_average << std::endl;
				total_time += stage_time_average;
			}

			wss << total_time << std::endl;
			Utility::ConsoleMessageW(wss.str());
		}
	};


	std::vector<float> cut_size_large(sample_locations.size(), 160.0f);
	std::vector<float> cut_size_medium(sample_locations.size(), 80.0f);
	std::vector<float> cut_size_small(sample_locations.size(), 40.0f);

	run_test(create_samples(sample_locations, cut_size_large, L"large"));		// large-sized cuts
	run_test(create_samples(sample_locations, cut_size_medium, L"medium"));		// medium-sized cuts (50% of large)
	run_test(create_samples(sample_locations, cut_size_small, L"small"));		// small-sized cuts  (25% of large)
}
